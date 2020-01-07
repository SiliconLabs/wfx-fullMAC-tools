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
#include "wfx_host_cfg.h"
#include "wfx_host_events.h"
#include "io.h"
#include "wfx_task.h"
#include "wfx_host.h"
#include "lwipopts.h"
#if LWIP_APP_TLS_ENABLED
#include <mbedtls/threading.h>
#include MBEDTLS_CONFIG_FILE
#endif

#define  EX_MAIN_START_TASK_PRIO              30u
#define  EX_MAIN_START_TASK_STK_SIZE         512u

#ifdef SL_WFX_USE_SECURE_LINK
extern void wfx_securelink_task_start(void);
#endif

/// Start task stack.
static  CPU_STK  main_start_task_stk[EX_MAIN_START_TASK_STK_SIZE];
/// Start task TCB.
static  OS_TCB   main_start_task_tcb;
static  void     main_start_task (void  *p_arg);

#ifdef SLEEP_ENABLED
#include "sleep.h"

static  void     idleHook(void);
static  void     setupHooks(void);

static bool sleepCallback(SLEEP_EnergyMode_t emode)
{
#ifdef SL_WFX_USE_SPI
  if (GPIO_PinInGet(WFX_HOST_CFG_SPI_WIRQPORT,  WFX_HOST_CFG_SPI_WIRQPIN))//wf200 messages pending
#else
  if (GPIO_PinInGet(WFX_HOST_CFG_WIRQPORT,  WFX_HOST_CFG_WIRQPIN)) //wf200 messages pending
#endif
  {
    return false;
  }

  return true;
}

static void wakeupCallback(SLEEP_EnergyMode_t emode)
{

}
#endif

/**************************************************************************//**
 * Main function
 *****************************************************************************/
int  main(void)
{
  RTOS_ERR  err;
  // Set the HFRCO frequency.
  CMU_HFRCOFreqSet(cmuHFRCOFreq_72M0Hz);
  BSP_SystemInit(); // Initialize System.
  CPU_Init();       // Initialize CPU.
#ifdef SL_WFX_USE_SPI
  CMU_ClockPrescSet(cmuClock_HFPER, 0);
#endif
  RETARGET_SerialInit();
  RETARGET_SerialCrLf(1);
#ifdef SLEEP_ENABLED
  const SLEEP_Init_t sleepInit =
  {
    .sleepCallback = sleepCallback,
    .wakeupCallback = wakeupCallback,
    .restoreCallback = 0
  };
  SLEEP_InitEx(&sleepInit);
#endif
  // Clear the console and buffer
  printf("\033\143");
  printf("\033[3J");

  OS_TRACE_INIT(); // Initialize trace if enabled
  OSInit(&err);    // Initialize the Kernel.

#if LWIP_APP_TLS_ENABLED
  // Enable mbedtls Micrium OS support
  THREADING_setup();
#endif

  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
  // Initialize Kernel tick source.
//  BSP_TickInit();
#ifdef SLEEP_ENABLED
  setupHooks();
#endif
  OSTaskCreate(&main_start_task_tcb, // Create the Start Task.
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

  OSStart(&err); // Start the kernel.

  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

  return (1);
}


/**************************************************************************//**
 * Unified GPIO interrupt handler.
 *****************************************************************************/
static void GPIO_Unified_IRQ(void)
{
  RTOS_ERR err;

  // Get and clear all pending GPIO interrupts
  uint32_t interrupt_mask = GPIO_IntGet();
  GPIO_IntClear(interrupt_mask);

  // Act on interrupts
  if (interrupt_mask & 0x400) {
#ifdef SL_WFX_USE_SPI
    OSFlagPost(&wfxtask_evts, WFX_EVENT_FLAG_RX, OS_OPT_POST_FLAG_SET, &err);
#endif
#ifdef SL_WFX_USE_SDIO
#ifdef SLEEP_ENABLED
    OSFlagPost(&wfxtask_evts, WFX_EVENT_FLAG_RX,OS_OPT_POST_FLAG_SET,&err);
#endif
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
  RTOS_ERR  err;
  PP_UNUSED_PARAM(p_arg); // Prevent compiler warning.



#ifdef SL_WFX_USE_SDIO
#ifdef RTOS_MODULE_IO_AVAIL
  // Initialize the IO module.
  IO_Init(&err);
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
#endif
#endif

  // Enable GPIO clock.
  CMU_ClockEnable(cmuClock_GPIO, true);

  gpio_setup();

#ifdef CPU_CFG_INT_DIS_MEAS_EN
  // Initialize interrupts disabled measurement.
  CPU_IntDisMeasMaxCurReset();
#endif

  // Call common module initialization.
  Common_Init(&err);
  APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE,; );

#ifdef  RTOS_MODULE_COMMON_SHELL_AVAIL
  Shell_Init(&err);
  APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE,; );
#endif

  Auth_Init(&err);
  APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE,; );

  // Initialize the BSP.
  BSP_OS_Init();
  BSP_LedsInit();

  printf("WF200 Micrium OS LwIP Example\n");

  //start wfx bus communication task.
  wfxtask_start();
#ifdef SL_WFX_USE_SECURE_LINK
  wfx_securelink_task_start(); // start securelink key renegotiation task
#endif //WFX_USE_SECURE_LINK

  wfx_events_task_start();
  lwip_start();

  // Delete the init thread.
  OSTaskDel(0, &err);

}
#ifdef SLEEP_ENABLED
/***************************************************************************//**
 * @brief
 *   This is the idle hook.
 *
 * @detail
 *   This will be called by the Micrium OS idle task when there is no other
 *   task ready to run. We just enter the lowest possible energy mode.
 ******************************************************************************/
static void idleHook(void)
{
  SLEEP_Sleep();
}

/***************************************************************************//**
 * @brief
 *   Setup the Micrium OS hooks. We are only using the idle task hook.
 *   See the Mcirium OS documentation for more information on the
 *   other available hooks.
 ******************************************************************************/
static void setupHooks(void)
{
  CPU_SR_ALLOC();

  CPU_CRITICAL_ENTER();
  /* Don't allow EM3, since we use LF clocks. */
  SLEEP_SleepBlockBegin(sleepEM3);
  OS_AppIdleTaskHookPtr = idleHook;
  CPU_CRITICAL_EXIT();
}
#endif

