/**************************************************************************//**
 * Copyright 2018, Silicon Laboratories Inc.
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

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "bsp.h"
#include "bsp_trace.h"
#include "bspconfig.h"
#include "retargetserial.h"
#include "dmadrv.h"
#include "wfx_host_cfg.h"
#include "host_resources.h"

#define HOST_NB_MAX_LEDS    2

static void host_gpio_setup(void);
static int host_dma_init(unsigned int *channel_id);

static unsigned int buffer_size = 0;
static volatile uint32_t ms_ticks = 0;
static unsigned int rx_dma_channel = 0;

extern uint8_t wf200_interrupt_event;

int host_init (void)
{
  EMU_DCDCInit_TypeDef dcdcInit = EMU_DCDCINIT_STK_DEFAULT;
  int res;

  /* Chip errata */
  CHIP_Init();

  EMU_DCDCInit(&dcdcInit);

  RETARGET_SerialInit();
  RETARGET_SerialCrLf(1);

  /* Initialize LED driver */
  BSP_LedsInit();

  /* Configure the application GPIOs */
  host_gpio_setup();

  /* Setup SysTick Timer for 1 msec interrupts  */
  res = SysTick_Config(CMU_ClockFreqGet(cmuClock_CORE) / 1000);

  /* Allocate a DMA channel for the UART */
  res |= host_dma_init(&rx_dma_channel);

  return res;
}

int host_dma_start (char *rx_buffer, int rx_buffer_size)
{
  int ret = -1;
  Ecode_t err;

  err = DMADRV_PeripheralMemory(rx_dma_channel,
                                dmadrvPeripheralSignal_USART4_RXDATAV,
                                rx_buffer,
                                (void *)&USART4->RXDATA,
                                true,
                                rx_buffer_size,
                                dmadrvDataSize1 /*Byte*/,
                                NULL,
                                NULL);

  if (err == ECODE_OK) {
    buffer_size = rx_buffer_size;
    ret = 0;
  }

  return ret;
}

int host_dma_stop (void)
{
  int ret = -1;
  Ecode_t err;

  err = DMADRV_StopTransfer(rx_dma_channel);
  if (err == ECODE_OK) {
    ret = 0;
  }

  return ret;
}

int host_dma_get_nb_bytes_received (int *nb_rx_bytes)
{
  int nb_remaining_bytes;
  int ret = -1;
  Ecode_t err;

  *nb_rx_bytes = 0;

  err = DMADRV_TransferRemainingCount(rx_dma_channel, &nb_remaining_bytes);
  if (err == ECODE_OK) {
    *nb_rx_bytes = buffer_size - nb_remaining_bytes;
    ret = 0;
  }

  return ret;
}

int host_led_toggle (unsigned int led_nb)
{
  int ret = -1;

  if (led_nb < HOST_NB_MAX_LEDS) {
    BSP_LedToggle(led_nb);
    ret = 0;
  }

  return ret;
}

int host_led_set_state (unsigned int led_nb, int state)
{
  int ret = -1;

  if (led_nb < HOST_NB_MAX_LEDS) {
    if (state == 0) {
      BSP_LedClear(led_nb);
    } else {
      BSP_LedSet(led_nb);
    }
    ret = 0;
  }

  return ret;
}

/***************************************************************************//**
 * @brief Delays number of msTick Systicks (typically 1 ms)
 * @param nb_ticks Number of ticks to delay
 ******************************************************************************/
void host_delay (uint32_t nb_ticks)
{
  uint32_t current_ticks;

  current_ticks = ms_ticks;
  while ((ms_ticks - current_ticks) < nb_ticks) ;
}

/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/
/***************************************************************************//**
 * @brief SysTick_Handler
 * Interrupt Service Routine for system tick counter
 ******************************************************************************/
void SysTick_Handler (void)
{
  ms_ticks++;       /* increment counter necessary in Delay()*/
}

/***************************************************************************//**
 * @brief Unified GPIO Interrupt handler (pushbuttons)
 ******************************************************************************/
static void GPIO_Unified_IRQ (void)
{
  // Get and clear all pending GPIO interrupts
  uint32_t interruptMask = GPIO_IntGet();
  GPIO_IntClear(interruptMask);

  // Act on interrupts
#ifdef  SL_WFX_USE_SPI
  if (interruptMask & (1<<WFX_HOST_CFG_SPI_IRQ)) {
    wf200_interrupt_event = 1;
  }
#endif

  if (interruptMask & (1<<BSP_GPIO_PB0_PIN)) {
    BSP_LedToggle(0);
  }

  if (interruptMask & (1<<BSP_GPIO_PB1_PIN)) {
    BSP_LedToggle(1);
  }
}
/***************************************************************************//**
 * @brief GPIO Interrupt handler (PB0)
 ******************************************************************************/
void GPIO_EVEN_IRQHandler (void)
{
  GPIO_Unified_IRQ();
}

/***************************************************************************//**
 * @brief GPIO Interrupt handler (PB1/SPI)
 ******************************************************************************/
void GPIO_ODD_IRQHandler (void)
{
  GPIO_Unified_IRQ();
}
/****************************************************************************************************//**
 *                                          host_gpio_setup()
 *
 * @brief  Configure the GPIO pins to read the push buttons.
 *
 * @param  pin  GPIO Pin Number.
 *******************************************************************************************************/
static void host_gpio_setup (void)
{
  // Enable GPIO clock.
  CMU_ClockEnable(cmuClock_GPIO, true);
#ifdef EFM32GG11B820F2048GL192
  // Configure PB0 and PB1 as inputs.
  GPIO_PinModeSet(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, gpioModeInput, 0);
  GPIO_PinModeSet(BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN, gpioModeInput, 0);
  // Enable interrupts.
  GPIO_IntConfig(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, false, true, true);
  GPIO_IntConfig(BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN, false, true, true);
#endif

  // Configure WF200 Reset Pin
  GPIO_PinModeSet(WFX_HOST_CFG_RESET_PORT,
                  WFX_HOST_CFG_RESET_PIN,
                  gpioModePushPull,
                  0);

  // Configure WF200 WUP Pin
  GPIO_PinModeSet(WFX_HOST_CFG_WUP_PORT,
                  WFX_HOST_CFG_WUP_PIN,
                  gpioModePushPull,
                  0);

#ifdef  SL_WFX_USE_SPI
  // GPIO used as IRQ
  GPIO_PinModeSet(WFX_HOST_CFG_SPI_WIRQPORT,
                  WFX_HOST_CFG_SPI_WIRQPIN,
                  gpioModeInputPull,
                  0);

  // Redirect SPI WIRQ to the interrupt n°10 (n°8 already used by PB0)
  GPIO_ExtIntConfig(WFX_HOST_CFG_SPI_WIRQPORT,
                    WFX_HOST_CFG_SPI_WIRQPIN,
                    WFX_HOST_CFG_SPI_IRQ,
                    true,
                    false,
                    false);
#endif

  CMU_OscillatorEnable(cmuOsc_LFXO,true,true);

  // Reset and enable associated CPU interrupt vector
  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn);
  NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
  NVIC_EnableIRQ(GPIO_EVEN_IRQn);
}

/****************************************************************************************************//**
 *                                                host_dma_init()
 *
 * @brief  Configure the DMA to treat incoming commands on the Virtual Communication port (USART4).
 *
 * @param[out] channel_id Id of the DMA channel allocated for the transfer
 *
 * @return
 *    0 on success, -1 on failure.
 *******************************************************************************************************/
static int host_dma_init (unsigned int *channel_id)
{
  int ret = -1;
  Ecode_t result;

  DMADRV_Init();

  result = DMADRV_AllocateChannel(channel_id, NULL);
  if (result == ECODE_EMDRV_DMADRV_OK ) {
    // Initialization success
    ret = 0;
  }

  return ret;
}
