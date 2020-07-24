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
#include "bsp.h"
#include "em_cmu.h"
#include "wifi_app.h"
#include "bluetooth_app.h"
#include "interface.h"
#include "app.h"

void HardFault_Handler( void ) __attribute__( ( naked ) );
void GenericFault_Handler_C(unsigned long * svc_args, unsigned int lr_value);

static volatile uint32_t ms_ticks = 0;

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


/* Main application */
void appMain(gecko_configuration_t *pconfig)
{
  int res;

  // Configure 1ms Systick
  SysTick_Config(CMU_ClockFreqGet(cmuClock_CORE) / 1000);
  NVIC_EnableIRQ(SysTick_IRQn);

  interface_init();

  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn);
  NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
  NVIC_EnableIRQ(GPIO_EVEN_IRQn);

  // Clear the console and buffer
  printf("\033\143");
  printf("\033[3J");
  printf("Multiprotocol Bare Metal example\r\n");

  // Initialize and start the Bluetooth stack
  res = bluetooth_app_init(pconfig);
  if (res != 0) {
    printf("BLE initialization error\r\n");
  }

  // Initialize and start the Wi-Fi stack
  res = wifi_app_init();
  if (res != 0) {
    printf("WiFi initialization error\r\n");
  }

  while (1) {
    bluetooth_app_process();
    wifi_app_process();
  }
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
  wifi_app_treat_interrupt(interrupt_mask);

  if (interrupt_mask & (1 << BSP_BUTTON0_PIN)) {
    interface_light_toggle(interface_light_trigger_src_button, NULL);
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

  printf("[HardFault]\r\n");
  printf ("- Stack frame:\r\n");
  printf (" R0  = %08lX\r\n", stacked_r0);
  printf (" R1  = %08lX\r\n", stacked_r1);
  printf (" R2  = %08lX\r\n", stacked_r2);
  printf (" R3  = %08lX\r\n", stacked_r3);
  printf (" R12 = %08lX\r\n", stacked_r12);
  printf (" LR  = %08lX\r\n", stacked_lr);
  printf (" PC  = %08lX\r\n", stacked_pc);
  printf (" PSR = %08lX\r\n", stacked_psr);
  printf ("- FSR/FAR:\r\n");
  printf (" CFSR = %08lX\r\n", cfsr);
  printf (" HFSR = %08lX\r\n", SCB->HFSR);
  printf (" DFSR = %08lX\r\n", SCB->DFSR);
  printf (" AFSR = %08lX\r\n", SCB->AFSR);
  if (cfsr & 0x0080) printf (" MMFAR = %08lX\r\n", memmanage_fault_address);
  if (cfsr & 0x8000) printf (" BFAR = %08lX\r\n", bus_fault_address);
  printf ("- Misc\r\n");
  printf (" LR/EXC_RETURN= %08X\r\n", lr_value);

  while(1); // endless loop
}
