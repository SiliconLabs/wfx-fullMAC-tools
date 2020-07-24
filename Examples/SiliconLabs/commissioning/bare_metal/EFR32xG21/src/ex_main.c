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

#include <stdio.h>
#include "lwip_bm.h"
#include "bsp.h"
#include "retargetserial.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_chip.h"
#include "sl_wfx_host_cfg.h"
#include "sl_wfx_host.h"

static void gpio_setup(void);
void HardFault_Handler( void ) __attribute__( ( naked ) );
void GenericFault_Handler_C(unsigned long * svc_args, unsigned int lr_value);

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
  CMU_HFXOInit_TypeDef hfxoInit = CMU_HFXOINIT_DEFAULT;
  CMU_DPLLInit_TypeDef dpllInit = { .autoRecover = true,
                                    .m = 720,
                                    .n = 1499,
                                    .frequency = 80000000,
                                    .refClk = cmuSelect_HFXO};

  // Chip errata
  CHIP_Init();

  // Enable the DPLL, HFXO is used as reference clock
  CMU_HFXOInit(&hfxoInit);
  while (CMU_DPLLLock(&dpllInit) != true);
  CMU_ClockSelectSet(cmuClock_SYSCLK, cmuSelect_HFRCODPLL);

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
  printf("WFx Wi-Fi Commissioning example\r\n");

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
  if (interrupt_mask & (1 << SL_WFX_HOST_CFG_SPI_WIRQPIN)) {
    wf200_interrupt_event = 1;
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

  // Configure WF200 reset pin.
  GPIO_PinModeSet(SL_WFX_HOST_CFG_RESET_PORT, SL_WFX_HOST_CFG_RESET_PIN, gpioModePushPull, 0);
  // Configure WF200 WUP pin.
  GPIO_PinModeSet(SL_WFX_HOST_CFG_WUP_PORT, SL_WFX_HOST_CFG_WUP_PIN, gpioModePushPull, 0);
  // GPIO used as IRQ.
  GPIO_PinModeSet(SL_WFX_HOST_CFG_SPI_WIRQPORT, SL_WFX_HOST_CFG_SPI_WIRQPIN, gpioModeInputPull, 0);

  CMU_OscillatorEnable(cmuOsc_LFXO, true, true);

  // Reset and enable associated CPU interrupt vector.
  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn);
  NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
  NVIC_EnableIRQ(GPIO_EVEN_IRQn);
}

void HardFault_Handler(void)
{
  __asm volatile
  (
      " tst lr, #4                                                \n"
      " ite eq                                                    \n"
      " mrseq r0, msp                                             \n"
      " mrsne r0, psp                                             \n"
      " mov r1, lr                                                \n"
      " b GenericFault_Handler_C                                  \n"
  );
}

void GenericFault_Handler_C(unsigned long * hardfault_args, unsigned int lr_value)
{
  unsigned long stacked_r0;
  unsigned long stacked_r1;
  unsigned long stacked_r2;
  unsigned long stacked_r3;
  unsigned long stacked_r12;
  unsigned long stacked_lr;
  unsigned long stacked_pc;
  unsigned long stacked_psr;
  unsigned long cfsr;
  unsigned long bus_fault_address;
  unsigned long memmanage_fault_address;

  bus_fault_address       = SCB->BFAR;
  memmanage_fault_address = SCB->MMFAR;
  cfsr                    = SCB->CFSR;

  stacked_r0  = ((unsigned long) hardfault_args[0]);
  stacked_r1  = ((unsigned long) hardfault_args[1]);
  stacked_r2  = ((unsigned long) hardfault_args[2]);
  stacked_r3  = ((unsigned long) hardfault_args[3]);
  stacked_r12 = ((unsigned long) hardfault_args[4]);
  stacked_lr  = ((unsigned long) hardfault_args[5]);
  stacked_pc  = ((unsigned long) hardfault_args[6]);
  stacked_psr = ((unsigned long) hardfault_args[7]);

  printf ("[HardFault]\n");
  printf ("- Stack frame:\n");
  printf (" R0  = %08lX\n", stacked_r0);
  printf (" R1  = %08lX\n", stacked_r1);
  printf (" R2  = %08lX\n", stacked_r2);
  printf (" R3  = %08lX\n", stacked_r3);
  printf (" R12 = %08lX\n", stacked_r12);
  printf (" LR  = %08lX\n", stacked_lr);
  printf (" PC  = %08lX\n", stacked_pc);
  printf (" PSR = %08lX\n", stacked_psr);
  printf ("- FSR/FAR:\n");
  printf (" CFSR = %08lX\n", cfsr);
  printf (" HFSR = %08lX\n", SCB->HFSR);
  printf (" DFSR = %08lX\n", SCB->DFSR);
  printf (" AFSR = %08lX\n", SCB->AFSR);
  if (cfsr & 0x0080) printf (" MMFAR = %08lX\n", memmanage_fault_address);
  if (cfsr & 0x8000) printf (" BFAR = %08lX\n", bus_fault_address);
  printf ("- Misc\n");
  printf (" LR/EXC_RETURN= %08X\n", lr_value);

  while(1); // endless loop
}

