/***************************************************************************//**
 * @file
 * @brief Example main
 *******************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement.
 * The software is governed by the sections of the MSLA applicable to Micrium
 * Software.
 *
 ******************************************************************************/

#include "lwip_bm.h"
#include  "bsp.h"
#include  "retargetserial.h"
#include  "em_cmu.h"
#include  "em_emu.h"
#include  "em_chip.h"
#include  "wfx_host_cfg.h"
#include "wfx_host.h"
#include <stdio.h>
#ifdef SL_WFX_USE_SECURE_LINK
#include  <mbedtls/threading.h>
#endif

static void gpio_setup(void);

static volatile uint32_t ms_ticks = 0;

extern volatile uint8_t wf200_interrupt_event;

void SysTick_Handler (void)
{
  ms_ticks++;
}

/***************************************************************************//**
 * @brief Delays number of msTick Systicks (typically 1 ms)
 * @param nb_ticks Number of ticks to delay
 ******************************************************************************/
void delay_ms (uint32_t nb_ticks)
{
  uint32_t current_ticks;

  current_ticks = ms_ticks;
  while ((ms_ticks - current_ticks) < nb_ticks) ;
}

uint32_t sys_now(void)
{
  return ms_ticks;
}

/**************************************************************************//**
 * Main function
 *****************************************************************************/
int  main(void)
{
  // Chip errata
  CHIP_Init();

  // Set the HFRCO frequency.
  CMU_HFRCOFreqSet(cmuHFRCOFreq_72M0Hz);

#ifdef SL_WFX_USE_SPI
  CMU_ClockPrescSet(cmuClock_HFPER, 0);
#endif

#ifdef SL_WFX_USE_SDIO
  CMU_AUXHFRCOFreqSet(cmuAUXHFRCOFreq_48M0Hz);
  CMU_OscillatorEnable(cmuOsc_AUXHFRCO, true, true);
#endif

  // Configure 1ms Systick
  SysTick_Config(CMU_ClockFreqGet(cmuClock_CORE) / 1000);
  NVIC_EnableIRQ(SysTick_IRQn);

  // Initialize LED driver
  BSP_LedsInit();

  RETARGET_SerialInit();
  RETARGET_SerialCrLf(1);

  // Clear the console and buffer
  printf("\033\143");
  printf("\033[3J");

  gpio_setup();

  lwip_bm_init();

  while (1)
  {
    sl_wfx_process();
    lwip_bm_process();
  }

  return (1);
}


/**************************************************************************//**
 * Unified GPIO interrupt handler.
 *****************************************************************************/
static void GPIO_Unified_IRQ(void)
{
  // Get and clear all pending GPIO interrupts
  uint32_t interrupt_mask = GPIO_IntGet();
  GPIO_IntClear(interrupt_mask);

  // Act on interrupts
  if (interrupt_mask & 0x400) {
#ifdef SL_WFX_USE_SPI
    wf200_interrupt_event = 1;
#endif
#ifdef SL_WFX_USE_SDIO
#ifdef SLEEP_ENABLED
    wf200_interrupt_event = 1;
#endif
#endif
  }
  if (interrupt_mask & (1 << BSP_GPIO_PB0_PIN)) {
    BSP_LedToggle(0);
  }

  if (interrupt_mask & (1 << BSP_GPIO_PB1_PIN)) {
    BSP_LedToggle(1);
  }
}

/**************************************************************************//**
 * GPIO even interrupt handler.
 *****************************************************************************/
void GPIO_EVEN_IRQHandler(void)
{
  GPIO_Unified_IRQ();
}

/**************************************************************************//**
 * GPIO odd interrupt handler.
 *****************************************************************************/
void GPIO_ODD_IRQHandler(void)
{
  GPIO_Unified_IRQ();
}

/**************************************************************************//**
 * Configure the GPIO pins.
 *****************************************************************************/
static void gpio_setup(void)
{
  // Enable GPIO clock.
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Configure PB0 and PB1 as inputs (present on the Wireless Radio board in WGM160P case).
  GPIO_PinModeSet(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, gpioModeInput, 0);
  GPIO_PinModeSet(BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN, gpioModeInput, 0);
  // Enable interrupts.
  GPIO_IntConfig(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, false, true, true);
  GPIO_IntConfig(BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN, false, true, true);

  // Configure WF200 reset pin.
  GPIO_PinModeSet(WFX_HOST_CFG_RESET_PORT, WFX_HOST_CFG_RESET_PIN, gpioModePushPull, 0);
  // Configure WF200 WUP pin.
  GPIO_PinModeSet(WFX_HOST_CFG_WUP_PORT, WFX_HOST_CFG_WUP_PIN, gpioModePushPull, 0);
#ifdef  SL_WFX_USE_SPI
  // GPIO used as IRQ.
  GPIO_PinModeSet(WFX_HOST_CFG_SPI_WIRQPORT, WFX_HOST_CFG_SPI_WIRQPIN, gpioModeInputPull, 0);
#endif
  CMU_OscillatorEnable(cmuOsc_LFXO, true, true);
#ifdef EFM32GG11B820F2048GM64 //WGM160PX22KGA2
  // GPIO used as IRQ
  GPIO_PinModeSet(WFX_HOST_CFG_WIRQPORT,  WFX_HOST_CFG_WIRQPIN,  gpioModeInputPull,  0);
  // SDIO Pull-ups
  GPIO_PinModeSet(gpioPortD,  0,  gpioModeDisabled,  1);
  GPIO_PinModeSet(gpioPortD,  1,  gpioModeDisabled,  1);
  GPIO_PinModeSet(gpioPortD,  2,  gpioModeDisabled,  1);
  GPIO_PinModeSet(gpioPortD,  3,  gpioModeDisabled,  1);
  GPIO_PinModeSet(gpioPortD,  5,  gpioModeDisabled,  1);
  //WF200 LF CLK
  CMU->CTRL      |= CMU_CTRL_CLKOUTSEL0_LFXO;
  CMU->ROUTEPEN  |= CMU_ROUTEPEN_CLKOUT0PEN;
  CMU->ROUTELOC0 |= CMU_ROUTELOC0_CLKOUT0LOC_LOC5;
  GPIO_PinModeSet(LP_CLK_PORT,  LP_CLK_PIN,  gpioModePushPull,  0);
#endif
  // Reset and enable associated CPU interrupt vector.
  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn);
  NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
  NVIC_EnableIRQ(GPIO_EVEN_IRQn);
}

