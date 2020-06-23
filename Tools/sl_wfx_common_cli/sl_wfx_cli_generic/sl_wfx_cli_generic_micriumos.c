/**************************************************************************//**
 * Copyright 2019, Silicon Laboratories Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

#ifdef MICRIUMOS

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <common/include/kal.h>
#include <common/include/shell.h>
#include <common/include/rtos_err.h>
#include "ecode.h"
#include "dmadrv.h"
#include "retargetserial.h"
#include "sl_wfx_host_events.h"
#include "sl_wfx_cli_generic.h"

#ifndef SL_WFX_CLI_GEN_TASK_STK_SIZE
#define SL_WFX_CLI_GEN_TASK_STK_SIZE      2048u
#endif
#ifndef SL_WFX_CLI_GEN_TASK_PRIO
#define SL_WFX_CLI_GEN_TASK_PRIO            40u
#endif
#ifndef SL_WFX_CLI_GEN_INPUT_BUF_SIZE
#define SL_WFX_CLI_GEN_INPUT_BUF_SIZE      128u
#endif
#ifndef SL_WFX_CLI_GEN_OUTPUT_BUF_SIZE
#define SL_WFX_CLI_GEN_OUTPUT_BUF_SIZE     128u
#endif

static int sl_wfx_cli_generic_micriumos_init(void *config);
static int sl_wfx_cli_generic_micriumos_register_cmd(sl_wfx_cli_generic_command_t const *cmd);
static int sl_wfx_cli_generic_micriumos_config_wait(uint32_t event_flag);
static int sl_wfx_cli_generic_micriumos_wait(uint32_t timeout_ms);
static int sl_wfx_cli_generic_micriumos_resume(void);

static unsigned int usart_rx_dma_channel = 0;
static unsigned int usart_echo_dma_channel = 0;

sl_wfx_cli_generic_functions_t sl_wfx_cli_gen_funcs = {
  sl_wfx_cli_generic_micriumos_init,
  sl_wfx_cli_generic_micriumos_register_cmd,
  sl_wfx_cli_generic_micriumos_config_wait,
  sl_wfx_cli_generic_micriumos_wait,
  sl_wfx_cli_generic_micriumos_resume
};

// -----------------------------------------------------------------------------
// Local defines



// -----------------------------------------------------------------------------
// Local structures

typedef struct sl_wfx_cli_gen_int_cmd_s {
  SHELL_CMD cmd[1+1/*End item*/];
  sl_wfx_cli_generic_command_callback_t callback;
  char *help;
  int8_t nb_param;
} sl_wfx_cli_gen_int_cmd_t;

// -----------------------------------------------------------------------------
// Local global variables

// Shell Task Stack
static CPU_STK sl_wfx_cli_gen_task_stk[SL_WFX_CLI_GEN_TASK_STK_SIZE];
static OS_TCB sl_wfx_cli_gen_task_tcb;

static SHELL_CMD_PARAM shell_cmd_param = { 0 };
static SHELL_CFG_CMD_USAGE shell_cfg_cmd_usage = {
  .CmdTblItemNbrInit = 10u,
  .CmdTblItemNbrMax = SL_WFX_CLI_GEN_NB_MAX_CMD,
  .CmdArgNbrMax = 10u,
  .CmdNameLenMax = SL_WFX_CLI_CMD_NAME_MAX_LEN
};

static char sl_wfx_cli_gen_input_buf[SL_WFX_CLI_GEN_INPUT_BUF_SIZE];
static char sl_wfx_cli_gen_output_buf[SL_WFX_CLI_GEN_OUTPUT_BUF_SIZE];

static sl_wfx_cli_gen_int_cmd_t sl_wfx_cli_gen_cmd_registered[SL_WFX_CLI_GEN_NB_MAX_CMD] = {0};
static uint8_t nb_command_registered = 0;


// -----------------------------------------------------------------------------
// Local functions

static sl_wfx_cli_gen_int_cmd_t* micriumos_cmd_search (const char *cmd_name)
{
  sl_wfx_cli_gen_int_cmd_t *cmd = NULL;
  uint8_t i;

  // Iterate through the command list
  for (i=0; i<nb_command_registered; i++) {
    if ((strncmp(cmd_name,
                 sl_wfx_cli_gen_cmd_registered[i].cmd[0].Name,
                 strlen(sl_wfx_cli_gen_cmd_registered[i].cmd[0].Name)) == 0)
        && ((cmd_name[strlen(sl_wfx_cli_gen_cmd_registered[i].cmd[0].Name)] == '\0')
            || (cmd_name[strlen(sl_wfx_cli_gen_cmd_registered[i].cmd[0].Name)] == ' '))) {
      cmd = &sl_wfx_cli_gen_cmd_registered[i];
      break;
    }
  }

  return cmd;
}

static int16_t micriumos_wrapper_cb (uint16_t argc,
                                     char **argv,
                                     SHELL_OUT_FNCT out_fnct,
                                     SHELL_CMD_PARAM *p_cmd_param)
{
  sl_wfx_cli_gen_int_cmd_t *sl_wfx_cli_gen_cmd;
  char *msg = NULL;
  int ret = SHELL_EXEC_ERR;
  bool is_param_nb_error = false;

  (void)p_cmd_param;

  // Retrieve the actual callback function associated to the command name
  sl_wfx_cli_gen_cmd = micriumos_cmd_search(argv[0]);

  if ((sl_wfx_cli_gen_cmd != NULL)
      && (sl_wfx_cli_gen_cmd->callback != NULL)) {

    // Check the number of parameters
    if ((sl_wfx_cli_gen_cmd->nb_param < 0) /* Variable number */
        || (sl_wfx_cli_gen_cmd->nb_param == (argc - 1))) {

      // Number of parameter consistent, execute the command
      ret = sl_wfx_cli_gen_cmd->callback(argc,
                                         argv,
                                         sl_wfx_cli_gen_output_buf,
                                         SL_WFX_CLI_GEN_OUTPUT_BUF_SIZE);
    } else {
      is_param_nb_error = true;
    }

    if (ret != 0) {
      if ((argc > 1)
          && ((strncmp(argv[1], "help", 4) == 0) && (argv[1][4] == '\0'))) {
        // Command help requested, display it
        printf(sl_wfx_cli_gen_cmd->help);
        // Erase the potential error message set by a module
        sl_wfx_cli_gen_output_buf[0] = '\0';
        ret = 0;

      } else if (sl_wfx_cli_gen_output_buf[0] == '\0') {
        // Ensure to display an error message
        if (is_param_nb_error) {
          msg = (char *)invalid_command_msg;
        } else {
          msg = (char *)command_error_msg;
        }
        strncpy(sl_wfx_cli_gen_output_buf, msg, SL_WFX_CLI_GEN_OUTPUT_BUF_SIZE);
        if (SL_WFX_CLI_GEN_OUTPUT_BUF_SIZE > 0) {
          sl_wfx_cli_gen_output_buf[SL_WFX_CLI_GEN_OUTPUT_BUF_SIZE -1] = '\0';
        }
        ret = 0;

      } else {
        ret = 0;
      }
    }

    // Display the command output
    out_fnct(sl_wfx_cli_gen_output_buf, SL_WFX_CLI_GEN_OUTPUT_BUF_SIZE, NULL);
    // Artificially flush the output buffer
    sl_wfx_cli_gen_output_buf[0] = '\0';
  }

  return ret;
}

static int16_t mircriumos_wrapper_output (char *buf, uint16_t len, void *opt)
{
  (void)len;
  (void)opt;

  return sl_wfx_cli_generic_output(buf);
}

static int usart_rx_dma_init (void)
{
  int ret = -1;
  Ecode_t result;

  DMADRV_Init();

  result  = DMADRV_AllocateChannel(&usart_rx_dma_channel, NULL);
  result |= DMADRV_AllocateChannel(&usart_echo_dma_channel, NULL);
  if (result == ECODE_EMDRV_DMADRV_OK) {
    // Initialization success
    ret = 0;
  }

  return ret;
}

static bool usart_rx_dma_callback (unsigned int channel,
                                   unsigned int sequenceNo,
                                   void *userParam)
{
  char *ptr_last_char;
  uint8_t echo = (uint8_t)((uint32_t)userParam);
  RTOS_ERR  err;

  ptr_last_char = ((char *)LDMA->CH[usart_rx_dma_channel].DST - 1);

  // Is an end of line ?
  if ((*ptr_last_char == '\r') || (*ptr_last_char == '\n')) {
    // Replace the end of line character
    *ptr_last_char = '\0';

    // Notify the task
    OSTaskSemPost(&sl_wfx_cli_gen_task_tcb, OS_OPT_POST_NONE, &err);
  } else if ((*ptr_last_char == '\b' /*BS*/)
             || (*ptr_last_char == 0x7F /*DEL*/)) {
    uint8_t backspace_offset = 1;

    if (ptr_last_char > sl_wfx_cli_gen_input_buf) {
      // Remove the character preceding the backspace
      *(ptr_last_char-1) = '\0';
      backspace_offset++;
    }
    // Remove the backspace character
    *ptr_last_char = '\0';

    // Move accordingly the DMA pointer references
    LDMA->CH[usart_rx_dma_channel].DST = LDMA->CH[usart_rx_dma_channel].DST
                                         - backspace_offset;

    if (echo) {
      LDMA->CH[usart_echo_dma_channel].SRC = LDMA->CH[usart_rx_dma_channel].DST;
    }
  }

  // Check the buffer to avoid an overflow
  if (ptr_last_char < &sl_wfx_cli_gen_input_buf[SL_WFX_CLI_GEN_INPUT_BUF_SIZE]) {
    // Re-enable the DMA
    LDMA->CHDONE &= ~(1 << usart_rx_dma_channel);
    LDMA->CHEN |= (1 << usart_rx_dma_channel);
  }

  return false;
}

static bool usart_echo_dma_callback (unsigned int channel,
                                     unsigned int sequenceNo,
                                     void *userParam)
{
  // Re-enable the DMA
  LDMA->CHDONE &= ~(1 << usart_echo_dma_channel);
  LDMA->CHEN |= (1 << usart_echo_dma_channel);

  return false;
}

static int usart_rx_dma_start (sl_wfx_cli_generic_micriumos_config_t *config)
{
  int ret = -1;
  Ecode_t err;

  err  = DMADRV_PeripheralMemory(usart_rx_dma_channel,
                                 config->dma_peripheral_signal,
                                 sl_wfx_cli_gen_input_buf,
                                 (void *)&RETARGET_UART->RXDATA,
                                 true,
                                 1, // Each character produces an interrupt
                                 dmadrvDataSize1 /*Byte*/,
                                 usart_rx_dma_callback,
                                 (void *)((uint32_t)config->echo));

  // Increase the DMA round robin arbitration in order not to loose
  // any characters, assuming that other potential DMA channels
  // have an arbitration to one.
  LDMA->CH[usart_rx_dma_channel].CFG |= LDMA_CH_CFG_ARBSLOTS_TWO;

  if (config->echo) {
    // The current priority executes the echo DMA after the RX DMA.
    err |= DMADRV_MemoryPeripheral(usart_echo_dma_channel,
                                   config->dma_peripheral_signal,
                                   (void *)&RETARGET_UART->TXDATA,
                                   sl_wfx_cli_gen_input_buf,
                                   true,
                                   1, // Each character produces an interrupt
                                   dmadrvDataSize1 /*Byte*/,
                                   usart_echo_dma_callback,
                                   NULL);
  }

  if (err == ECODE_OK) {
    ret = 0;
  }

  return ret;
}

static int usart_rx_dma_stop (sl_wfx_cli_generic_micriumos_config_t *config)
{
  int ret = -1;
  Ecode_t err;

  (void) config;

  err  = DMADRV_StopTransfer(usart_rx_dma_channel);
  err |= DMADRV_StopTransfer(usart_echo_dma_channel);
  if (err == ECODE_OK) {
    ret = 0;
  }

  return ret;
}

static void sl_wfx_cli_gen_task (void *p_arg)
{
  RTOS_ERR  err;
  sl_wfx_cli_generic_micriumos_config_t *config;

  config = (sl_wfx_cli_generic_micriumos_config_t *)p_arg;
  shell_cmd_param.SessionActiveFlagsPtr = &config->echo;

  // Initialize and start the DMA
  usart_rx_dma_init();
  usart_rx_dma_start(config);

  // Display the prompt
  printf(welcome);
  printf(prompt);

  while (1) {
    // Reset the semaphore and wait for a new line to process
    OSTaskSemSet(&sl_wfx_cli_gen_task_tcb, 0, &err);
    OSTaskSemPend(0, OS_OPT_PEND_BLOCKING, DEF_NULL, &err);

    // Display a new line
    printf(newline);

    if (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) {
        // Stop DMA while processing the line
        usart_rx_dma_stop(config);

        // Check if the buffer contains characters
        if (strlen(sl_wfx_cli_gen_input_buf)) {
          // Execute the received command
          Shell_Exec(sl_wfx_cli_gen_input_buf,
                     mircriumos_wrapper_output,
                     (SHELL_CMD_PARAM *)&shell_cmd_param,
                     &err);

          if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
            // The Micrium sl_wfx_cli outputs a message in case of error
            // without adding a new line
            printf(newline);
          }
        }

        // Update the echo flag
        config->echo = shell_cmd_param.SessionActiveFlagsPtr[0];

        // Re-enable the DMA
        usart_rx_dma_start(config);

        // Display the prompt
        printf(prompt);
    }
  }
}

static int help_cmd_cb (int argc,
                        char **argv,
                        char *output_buf,
                        uint32_t output_buf_len)
{
  (void)argc;
  (void)argv;
  (void)output_buf;
  (void)output_buf_len;

  for (uint8_t i=0; i<nb_command_registered; i++) {
    printf(sl_wfx_cli_gen_cmd_registered[i].help);
  }

  return 0;
}

static const sl_wfx_cli_generic_command_t help_cmd =
{
  "help",
  "help                     : Lists all the registered commands\r\n",
  help_cmd_cb,
  0
};

static int cpu_reset_cmd_cb (int argc,
                             char **argv,
                             char *output_buf,
                             uint32_t output_buf_len)
{
  (void)argc;
  (void)argv;
  (void)output_buf;
  (void)output_buf_len;

  printf("CPU Reset\r\n");

  // Ensure all outstanding memory accesses included buffered write are completed before reset
  __DSB();
  SCB->AIRCR = (uint32_t)( (0x5FAUL << SCB_AIRCR_VECTKEY_Pos)
                          |(SCB->AIRCR & SCB_AIRCR_PRIGROUP_Msk)
                          | SCB_AIRCR_SYSRESETREQ_Msk);
  // Ensure completion of memory access
  __DSB();

  // Wait until reset
  for(;;) {
    __NOP();
  }
  return 0;
}

static const sl_wfx_cli_generic_command_t cpu_reset_cmd =
{
  "cpu-reset",
  "cpu-reset                : Reset the host CPU\r\n",
  cpu_reset_cmd_cb,
  0
};

static int sl_wfx_cli_generic_micriumos_init (void *config)
{
  RTOS_ERR  err;
  int res = SL_WFX_CLI_ERROR;

  // Initialize the Micrium Shell
  Shell_ConfigureCmdUsage(&shell_cfg_cmd_usage);
  Shell_Init(&err);
  // Remove the commands automatically added during the shell initialization
  Shell_CmdTblRem("Sh", &err);
  Shell_CmdTblRem("mem", &err);

  if (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) {
    // Create & start the Shell task
    OSTaskCreate(&sl_wfx_cli_gen_task_tcb,
                 "Shell Generic Task",
                 sl_wfx_cli_gen_task,
                 config,
                 SL_WFX_CLI_GEN_TASK_PRIO,
                 &sl_wfx_cli_gen_task_stk[0],
                 (SL_WFX_CLI_GEN_TASK_STK_SIZE / 10u),
                 SL_WFX_CLI_GEN_TASK_STK_SIZE,
                 0u,
                 0u,
                 DEF_NULL,
                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_NO_TLS),
                 &err);

    if (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) {
      res  = sl_wfx_cli_generic_micriumos_register_cmd(&help_cmd);
      res |= sl_wfx_cli_generic_micriumos_register_cmd(&cpu_reset_cmd);
    }
  }

  return res;
}

static int sl_wfx_cli_generic_micriumos_register_cmd (sl_wfx_cli_generic_command_t const *cmd)
{
  RTOS_ERR err;
  int res = SL_WFX_CLI_ERROR;

  // Check that there is some place left
  if (nb_command_registered < SL_WFX_CLI_GEN_NB_MAX_CMD) {

    // Make all the commands point to the same wrapper function
    // which will call the related callback afterwards.
    sl_wfx_cli_gen_cmd_registered[nb_command_registered].cmd[0].Name = cmd->name;
    sl_wfx_cli_gen_cmd_registered[nb_command_registered].cmd[0].Fnct = micriumos_wrapper_cb;
    sl_wfx_cli_gen_cmd_registered[nb_command_registered].cmd[1].Name = NULL;
    sl_wfx_cli_gen_cmd_registered[nb_command_registered].cmd[1].Fnct = NULL;
    sl_wfx_cli_gen_cmd_registered[nb_command_registered].callback = cmd->callback;
    sl_wfx_cli_gen_cmd_registered[nb_command_registered].help = cmd->help;
    sl_wfx_cli_gen_cmd_registered[nb_command_registered].nb_param = cmd->nb_param;

    Shell_CmdTblAdd(DEF_NULL, sl_wfx_cli_gen_cmd_registered[nb_command_registered].cmd, &err);
    if (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) {
      nb_command_registered++;
      res = SL_WFX_CLI_ERROR_NONE;
    }
  } else {
    res = SL_WFX_CLI_ERROR_NB_MAX_COMMANDS;
  }

  return res;
}

static void event_notification_callback (uint32_t event_flag)
{
  sl_wfx_host_events_unnotify(event_flag, (void *)&sl_wfx_cli_gen_task_tcb);
  sl_wfx_cli_generic_micriumos_resume();
}

static int sl_wfx_cli_generic_micriumos_config_wait (uint32_t event_flag)
{
  RTOS_ERR err;
  int res = 0;

  if ((event_flag & SL_WFX_EVENT_ALL_FLAGS) != 0) {
    res = sl_wfx_host_events_notify(event_flag,
                                    event_notification_callback,
                                    (void *)&sl_wfx_cli_gen_task_tcb);
  }

  // Reset the semaphore counter
  OSTaskSemSet(&sl_wfx_cli_gen_task_tcb, 0, &err);
  if ((res == 0)
      && (err.Code == RTOS_ERR_NONE)) {
    res = SL_WFX_CLI_ERROR_NONE;
  } else {
    res = SL_WFX_CLI_ERROR;
  }

  return res;
}

static int sl_wfx_cli_generic_micriumos_wait (uint32_t timeout_ms)
{
  OS_TICK tmo_ticks;
  RTOS_ERR err;
  int res = SL_WFX_CLI_ERROR;

  tmo_ticks = (OS_TICK)(((uint64_t)timeout_ms * OSCfg_TickRate_Hz) / 1000);

  OSTaskSemPend(tmo_ticks, OS_OPT_PEND_BLOCKING, NULL, &err);
  if (err.Code == RTOS_ERR_TIMEOUT) {
    res = SL_WFX_CLI_ERROR_TIMEOUT;
  } else if (err.Code == RTOS_ERR_NONE) {
    res = SL_WFX_CLI_ERROR_NONE;
  }
  return res;
}

static int sl_wfx_cli_generic_micriumos_resume (void)
{
  RTOS_ERR err;
  int res = SL_WFX_CLI_ERROR;

  OSTaskSemPost(&sl_wfx_cli_gen_task_tcb, OS_OPT_POST_NONE, &err);
  if (err.Code == RTOS_ERR_NONE) {
    res = SL_WFX_CLI_ERROR_NONE;
  }
  return res;
}

#endif //MICRIUMOS

