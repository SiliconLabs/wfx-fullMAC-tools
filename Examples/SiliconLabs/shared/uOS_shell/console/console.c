#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "ecode.h"
#include "dmadrv.h"

#include <cpu/include/cpu.h>
#include <kernel/include/os.h>
#include <kernel/include/os_trace.h>
#include <common/include/common.h>
#include <common/include/lib_def.h>
#include <common/include/rtos_utils.h>
#include <common/include/toolchains.h>

#include "console.h"

typedef struct console_s {
  char *buffer_addr;
  uint32_t buffer_size;
  console_is_rx_end_cb_t rx_end_cb;
} console_t;

static OS_SEM console_sem;
static char *ptr_prev_call_buffer = NULL;
static unsigned int usart_rx_dma_channel = 0;
static unsigned int usart_echo_dma_channel = 0;
static bool usart_initialized = false;
static console_config_t *console_config = NULL;

/**************************************************************************//**
 * Manage the USART reception using the DMA.
 *****************************************************************************/
static bool usart_rx_dma_callback (unsigned int channel,
                                   unsigned int sequenceNo,
                                   void *userParam)
{
  console_t *console = (console_t *)userParam;
  char *ptr_last_char;
  RTOS_ERR  err;
  bool rx_end = true;

  (void)channel;
  (void)sequenceNo;
  (void)userParam;

  ptr_last_char = ((char *)LDMA->CH[usart_rx_dma_channel].DST - 1);

  // Is an end of line ?
  if ((*ptr_last_char == '\r') || (*ptr_last_char == '\n')) {
    // Call the user callback looking for the reception end, if defined
    // otherwise reception stopped by default on the first end of line.
    if (console->rx_end_cb) {
      rx_end = console->rx_end_cb(ptr_prev_call_buffer);
    }

    if (rx_end) {
      // End of reception detected, unlock the task waiting the console input
      OSSemPost(&console_sem, OS_OPT_POST_NONE, &err);
    }

    // Move the pointer on the buffer storing the input
    // to potentially speed up the processing of looking for the reception end
    ptr_prev_call_buffer = ptr_last_char;

  } else if ((*ptr_last_char == '\b' /*BS*/)
             || (*ptr_last_char == 0x7F /*DEL*/)) {

    uint8_t backspace_offset = 1;

    if (ptr_last_char > console->buffer_addr) {
      // Remove the character preceding the backspace
      *(ptr_last_char-1) = '\0';
      backspace_offset++;
    }
    // Remove the backspace character
    *ptr_last_char = '\0';

    // Move accordingly the DMA pointer references
    LDMA->CH[usart_rx_dma_channel].DST = LDMA->CH[usart_rx_dma_channel].DST
                                         - backspace_offset;

    if (console_config->echo) {
      LDMA->CH[usart_echo_dma_channel].SRC = LDMA->CH[usart_rx_dma_channel].DST;
    }
  }

  // Check the buffer to avoid an overflow
  if (ptr_last_char < &console->buffer_addr[console->buffer_size]) {
    // Re-enable the DMA
    LDMA->CHDONE &= ~(1 << usart_rx_dma_channel);
    LDMA->CHEN |= (1 << usart_rx_dma_channel);
  } else {
    //FIXME find a way to catch the overflow before it's too late
    printf("Console: input overflow !!!\r\n");
  }

  return false;
}

/**************************************************************************//**
 * Manage the terminal echo using the DMA.
 *****************************************************************************/
static bool usart_echo_dma_callback (unsigned int channel,
                                     unsigned int sequenceNo,
                                     void *userParam)
{
  char *ptr_last_char;

  (void)channel;
  (void)sequenceNo;
  (void)userParam;

  ptr_last_char = ((char *)LDMA->CH[usart_echo_dma_channel].SRC - 1);

  // Is an end of line ?
  if (*ptr_last_char == '\r') {
    // Add a new line to have a consistent display
    //FIXME: terminal with CRLF input
    ptr_last_char = ((char *)LDMA->CH[usart_echo_dma_channel].DST);
    *ptr_last_char = '\n';
  }

  // Re-enable the DMA
  LDMA->CHDONE &= ~(1 << usart_echo_dma_channel);
  LDMA->CHEN |= (1 << usart_echo_dma_channel);

  return false;
}

/**************************************************************************//**
 * Retrieve several lines from the console with a timeout, a user callback
 * defines the reception end.
 *****************************************************************************/
static int console_get_input (char *buffer,
                              uint32_t buffer_size,
                              uint32_t timeout,
                              console_is_rx_end_cb_t rx_end_cb)
{
  int ret = -1;
  RTOS_ERR err;
  Ecode_t ecode;
  console_t console = {.buffer_addr = buffer,
                       .buffer_size = buffer_size,
                       .rx_end_cb = rx_end_cb};

  // Reset the key buffer
  memset(buffer, 0, buffer_size);
  ptr_prev_call_buffer = buffer;

  ecode = DMADRV_PeripheralMemory(usart_rx_dma_channel,
                                  console_config->dma_peripheral_signal,
                                  buffer,
                                  (void *)&console_config->usart_instance->RXDATA,
                                  true,
                                  1, // Each character produces an interrupt
                                  dmadrvDataSize1 /*Byte*/,
                                  usart_rx_dma_callback,
                                  (void *)&console);

  if (console_config->echo) {
    // The current priority executes the echo DMA after the RX DMA.
    ecode |= DMADRV_MemoryPeripheral(usart_echo_dma_channel,
                                     console_config->dma_peripheral_signal,
                                     (void *)&console_config->usart_instance->TXDATA,
                                     buffer,
                                     true,
                                     1, // Each character produces an interrupt
                                     dmadrvDataSize1 /*Byte*/,
                                     usart_echo_dma_callback,
                                     (void *)&console);
  }

  if (ecode == ECODE_EMDRV_DMADRV_OK) {
    OSSemPend(&console_sem,
              OSCfg_TickRate_Hz * timeout,
              OS_OPT_PEND_BLOCKING,
              NULL,
              &err);

    ecode |= DMADRV_StopTransfer(usart_rx_dma_channel);
    ecode |= DMADRV_StopTransfer(usart_echo_dma_channel);

    if ((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE)
        && (ecode == ECODE_OK)) {
      ret = strlen(buffer);

      // Only one line retrieved, replace EOL characters by
      // a NUL termination to ease further processing.
      if (rx_end_cb == NULL) {
        for (int8_t i=2; i>0; i--) {
          if (((buffer[ret-i]) == '\r') || ((buffer[ret-i]) == '\n')) {
            buffer[ret-i] = '\0';
            ret -= i;
            break;
          }
        }
      }
    }
  } else {
    printf("USART DMA start error (%ld)\n", ecode);
  }

  return ret;
}

/**************************************************************************//**
 * Retrieve several lines from the console, a user callback defines the
 * reception end.
 *
 * @param buffer        buffer where to store the input lines.
 * @param buffer_size   Size of the buffer where to store the input lines.
 * @param rx_end_cb     User callback defining the reception end
 *
 * @return  -1: error
 *         0<=: size of the input retrieved
 *
 * @note: this is a blocking function.
 * @note: ensure to provide a buffer with sufficient size, otherwise an
 *        overflow will occur.
 *****************************************************************************/
int console_get_lines (char *buffer,
                       uint32_t buffer_size,
                       console_is_rx_end_cb_t rx_end_cb)
{
  int ret = -1;

  if (rx_end_cb != NULL) {
    ret = console_get_input(buffer, buffer_size, 0, rx_end_cb);
  }

  return ret;
}

/**************************************************************************//**
 * Retrieve several lines from the console, a user callback defines the
 * reception end.
 *
 * @param buffer        buffer where to store the input lines.
 * @param buffer_size   Size of the buffer where to store the input lines.
 * @param timeout       Duration (in seconds) to wait for a console input.
 * @param rx_end_cb     User callback defining the reception end
 *
 * @return  -1: error
 *         0<=: size of the input retrieved
 *
 * @note: this is a blocking function.
 * @note: ensure to provide a buffer with sufficient size, otherwise an
 *        overflow will occur.
 *****************************************************************************/
int console_get_lines_tmo (char *buffer,
                           uint32_t buffer_size,
                           uint32_t timeout,
                           console_is_rx_end_cb_t rx_end_cb)
{
  int ret = -1;

  if (rx_end_cb != NULL) {
    ret = console_get_input(buffer, buffer_size, timeout, rx_end_cb);
  }

  return ret;
}

/**************************************************************************//**
 * Retrieve an input line from the console.
 *
 * @param buffer        buffer where to store the input line.
 * @param buffer_size   Size of the buffer where to store the input line.
 *
 * @return  -1: error
 *         0<=: size of the input line retrieved (without EOL characters).
 *
 * @note: this is a blocking function.
 * @note: EOL characters are replaced by a NUL termination in this function.
 * @note: ensure to provide a buffer with sufficient size, otherwise an
 *        overflow will occur.
 *****************************************************************************/
int console_get_line (char *buffer, uint32_t buffer_size)
{
  return console_get_input(buffer, buffer_size, 0, NULL);
}

/**************************************************************************//**
 * Retrieve an input line from the console with a timeout.
 *
 * @param buffer        buffer where to store the input line.
 * @param buffer_size   Size of the buffer where to store the input line.
 * @param timeout       Duration (in seconds) to wait for a console input.
 *
 * @return  -1: error
 *         0<=: size of the input line retrieved (without EOL characters).
 *
 * @note: this is a blocking function.
 * @note: EOL characters are replaced by a NUL termination in this function.
 * @note: ensure to provide a buffer with sufficient size, otherwise an
 *        overflow will occur.
 *****************************************************************************/
int console_get_line_tmo (char *buffer, uint32_t buffer_size, uint32_t timeout)
{
  return console_get_input(buffer, buffer_size, timeout, NULL);
}

/**************************************************************************//**
 * Initialize the console.
 *
 * @param config    Console configuration.
 *
 * @return 0 on success, -1 otherwise
 *****************************************************************************/
int console_init (console_config_t *config)
{
  int ret = -1;
  RTOS_ERR err;
  Ecode_t ecode;

  if (config != NULL) {
    // Initialize the DMA driver
    DMADRV_Init();

    // Update the configuration
    console_config = config;

    if (!usart_initialized) {
      // Allocate DMA resources
      ecode  = DMADRV_AllocateChannel(&usart_rx_dma_channel, NULL);
      ecode |= DMADRV_AllocateChannel(&usart_echo_dma_channel, NULL);

      // Create a semaphore to notify the reception end
      OSSemCreate(&console_sem, "console_sem", 0, &err);

      if ((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE)
          && (ecode == ECODE_EMDRV_DMADRV_OK)) {
        // Initialization success
        usart_initialized = true;
      }
    }

    ret = 0;
  }

  return ret;
}
