/***************************************************************************//**
 * @file
 * @brief Example main
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
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
#include "app_version.h"
#include "demo_config.h"
#include <bsp_os.h>
#include "bsp.h"
#include <cpu/include/cpu.h>
#include <kernel/include/os.h>
#include <kernel/include/os_trace.h>
#include <common/include/common.h>
#include <common/include/lib_def.h>
#include <common/include/rtos_utils.h>
#include <common/include/toolchains.h>
#include <common/include/auth.h>
#include <common/include/shell.h>
#include "retargetserial.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_chip.h"
#include "io.h"
#include "sl_wfx_task.h"
#include "sl_wfx_host.h"
#include "sl_wfx_host_cfg.h"
#include "sl_wfx_host_events.h"
#include "lwipopts.h"
#include "sl_wfx_cli_generic.h"

#include <mbedtls/threading.h>


#define  EX_MAIN_START_TASK_PRIO              30u
#define  EX_MAIN_START_TASK_STK_SIZE         512u

#ifdef SL_WFX_USE_SECURE_LINK
extern void sl_wfx_securelink_start(void);
#endif

#ifdef EFM32GG11B820F2048GM64
  static sl_wfx_cli_generic_micriumos_config_t cli_config = {
    .dma_peripheral_signal = dmadrvPeripheralSignal_USART0_RXDATAV,
    .echo = 1
  };
#else
  static sl_wfx_cli_generic_micriumos_config_t cli_config = {
    .dma_peripheral_signal = dmadrvPeripheralSignal_USART4_RXDATAV,
    .echo = 1
  };
#endif

/* Start task stack.*/
static  CPU_STK  main_start_task_stk[EX_MAIN_START_TASK_STK_SIZE];
/* Start task TCB.*/
static  OS_TCB   main_start_task_tcb;
static  void     main_start_task (void  *p_arg);

/**************************************************************************//**
 * Main function
 *****************************************************************************/
int  main (void) {
  RTOS_ERR  err;
  // Set the HFRCO frequency.
  CMU_HFRCOFreqSet(cmuHFRCOFreq_72M0Hz);
  BSP_SystemInit(); /* Initialize System.*/
  CPU_Init();       /* Initialize CPU.*/
#ifdef SL_WFX_USE_SPI
  CMU_ClockPrescSet(cmuClock_HFPER, 0);
#endif
  RETARGET_SerialInit();
  RETARGET_SerialCrLf(1);

  /* Clear the console and buffer*/
  printf("\033\143");
  printf("\033[3J");

  OS_TRACE_INIT(); /* Initialize trace if enabled*/
  OSInit(&err);    /* Initialize the Kernel.*/

  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

  /* Enable mbedtls Micrium OS support*/
  THREADING_setup();


  OSTaskCreate(&main_start_task_tcb, /* Create the Start Task.*/
               "Ex Main Start Task",
               main_start_task,
               DEF_NULL,
               EX_MAIN_START_TASK_PRIO,
               &main_start_task_stk[0],
               (EX_MAIN_START_TASK_STK_SIZE / 10u),
               EX_MAIN_START_TASK_STK_SIZE,
               0u,
               0u,
               DEF_NULL,
               (OS_OPT_TASK_STK_CLR),
               &err);

  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

  OSStart(&err); /* Start the kernel.*/

  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

  return (1);
}


/**************************************************************************//**
 * Unified GPIO interrupt handler.
 *****************************************************************************/
static void GPIO_Unified_IRQ (void) {
  RTOS_ERR err;

  /* Get and clear all pending GPIO interrupts*/
  uint32_t interrupt_mask = GPIO_IntGet();
  GPIO_IntClear(interrupt_mask);

  /* Act on interrupts*/
  if (interrupt_mask & 0x400) {
    OSSemPost(&wfx_wakeup_sem, OS_OPT_POST_ALL, &err);
#ifdef SL_WFX_USE_SPI
    OSFlagPost(&bus_events, SL_WFX_BUS_EVENT_FLAG_RX, OS_OPT_POST_FLAG_SET, &err);
#endif
  }
  if (interrupt_mask & (1 << BSP_GPIO_PB0_PIN)) {
    lwip_button_handler(0);
  }

  if (interrupt_mask & (1 << BSP_GPIO_PB1_PIN)) {
    lwip_button_handler(1);
  }
}

/**************************************************************************//**
 * GPIO even interrupt handler.
 *****************************************************************************/
void GPIO_EVEN_IRQHandler (void) {
  GPIO_Unified_IRQ();
}

/**************************************************************************//**
 * GPIO odd interrupt handler.
 *****************************************************************************/
void GPIO_ODD_IRQHandler (void) {
  GPIO_Unified_IRQ();
}

/**************************************************************************//**
 * Configure the GPIO pins.
 *****************************************************************************/
static void gpio_setup (void) {
  /* Enable GPIO clock.*/
  CMU_ClockEnable(cmuClock_GPIO, true);

  /* Configure PB0 and PB1 as inputs (present on the Wireless Radio board in WGM160P case).*/
  GPIO_PinModeSet(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, gpioModeInput, 0);
  GPIO_PinModeSet(BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN, gpioModeInput, 0);
  /* Enable interrupts.*/
  GPIO_IntConfig(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, false, true, true);
  GPIO_IntConfig(BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN, false, true, true);

  /* Configure WF200 reset pin.*/
  GPIO_PinModeSet(SL_WFX_HOST_CFG_RESET_PORT, SL_WFX_HOST_CFG_RESET_PIN, gpioModePushPull, 0);
  /* Configure WF200 WUP pin.*/
  GPIO_PinModeSet(SL_WFX_HOST_CFG_WUP_PORT, SL_WFX_HOST_CFG_WUP_PIN, gpioModePushPull, 0);
#ifdef  SL_WFX_USE_SPI
  /* GPIO used as IRQ.*/
  GPIO_PinModeSet(SL_WFX_HOST_CFG_SPI_WIRQPORT, SL_WFX_HOST_CFG_SPI_WIRQPIN, gpioModeInputPull, 0);
#endif
  CMU_OscillatorEnable(cmuOsc_LFXO, true, true);
#ifdef EFM32GG11B820F2048GM64 //WGM160PX22KGA2
  /* GPIO used as IRQ*/
  GPIO_PinModeSet(SL_WFX_HOST_CFG_WIRQPORT,  SL_WFX_HOST_CFG_WIRQPIN,  gpioModeInputPull,  0);
  /* SDIO Pull-ups*/
  GPIO_PinModeSet(gpioPortD,  0,  gpioModeDisabled,  1);
  GPIO_PinModeSet(gpioPortD,  1,  gpioModeDisabled,  1);
  GPIO_PinModeSet(gpioPortD,  2,  gpioModeDisabled,  1);
  GPIO_PinModeSet(gpioPortD,  3,  gpioModeDisabled,  1);
  GPIO_PinModeSet(gpioPortD,  5,  gpioModeDisabled,  1);
  /*WF200 LF CLK*/
  CMU->CTRL      |= CMU_CTRL_CLKOUTSEL0_LFXO;
  CMU->ROUTEPEN  |= CMU_ROUTEPEN_CLKOUT0PEN;
  CMU->ROUTELOC0 |= CMU_ROUTELOC0_CLKOUT0LOC_LOC5;
  GPIO_PinModeSet(LP_CLK_PORT,  LP_CLK_PIN,  gpioModePushPull,  0);
#endif
  /* Reset and enable associated CPU interrupt vector.*/
  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn);
  NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
  NVIC_EnableIRQ(GPIO_EVEN_IRQn);
}

/**************************************************************************//**
 * main_start_task()
 *
 * @param p_arg Argument passed from task creation. Unused.
 *
 *  This is the task that will be called by the startup when all services
 *  are initialized successfully.
 *
 *****************************************************************************/
static  void  main_start_task(void  *p_arg)
{
  int res;
  RTOS_ERR  err;
  PP_UNUSED_PARAM(p_arg); // Prevent compiler warning.



#ifdef SL_WFX_USE_SDIO
#ifdef RTOS_MODULE_IO_AVAIL
  // Initialize the IO module.
  IO_Init(&err);
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
#endif
#endif

  /* Enable GPIO clock.*/
  CMU_ClockEnable(cmuClock_GPIO, true);

  gpio_setup();

#ifdef CPU_CFG_INT_DIS_MEAS_EN
  /* Initialize interrupts disabled measurement.*/
  CPU_IntDisMeasMaxCurReset();
#endif

  /* Call common module initialization.*/
  Common_Init(&err);
  APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE,; );

#ifdef  RTOS_MODULE_COMMON_SHELL_AVAIL
  res = sl_wfx_cli_generic_init((void *)&cli_config);
  APP_RTOS_ASSERT_CRITICAL(res == 0,; );
#endif

  Auth_Init(&err);
  APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE,; );

  /* Initialize the BSP.*/
  BSP_OS_Init();
  BSP_LedsInit();

  printf("WFX UART CLI Example (v%u.%u.%u)\r\n",
         WFX_UART_CLI_APP_VERSION_MAJOR,
         WFX_UART_CLI_APP_VERSION_MINOR,
         WFX_UART_CLI_APP_VERSION_BUILD);

  /* Start tasks.*/

  wfx_events_start();

#ifdef SL_WFX_USE_SECURE_LINK
  sl_wfx_securelink_start(); /* start securelink key renegotiation task*/
#endif //WFX_USE_SECURE_LINK
  sl_wfx_task_start();

  lwip_start();

  /* Delete the init thread.*/
  OSTaskDel(0, &err);

}

