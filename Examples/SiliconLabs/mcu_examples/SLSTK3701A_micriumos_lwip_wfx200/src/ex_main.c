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

/*
*********************************************************************************************************
*
*                                             EXAMPLE MAIN
*
* File : ex_main.c
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/
#include "lwip_micriumos.h"
#include  <bsp_os.h>
#include  "bsp.h"
#include  <cpu/include/cpu.h>
#include  <kernel/include/os.h>
#include  <kernel/include/os_trace.h>
#include  <common/include/common.h>
#include  <common/include/lib_def.h>
#include  <common/include/auth.h>
#include  <common/include/shell.h>
#include  <common/include/rtos_utils.h>
#include  <common/include/toolchains.h>
#include  "retargetserial.h"
#include  "em_cmu.h"
#include  "em_emu.h"
#include  "em_chip.h"
#include  "wfx_pin_config.h"
#include  "wifi_cli.h"
#include "io.h"
#include "sleep.h"
#include "wfx_task.h"
#include <stdio.h>
/*
*********************************************************************************************************
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  EX_MAIN_START_TASK_PRIO              30u
#define  EX_MAIN_START_TASK_STK_SIZE         512u


/*
*********************************************************************************************************
*********************************************************************************************************
*                                        LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

                                                                /* Start Task Stack.                                    */
static  CPU_STK  Ex_MainStartTaskStk[EX_MAIN_START_TASK_STK_SIZE];
                                                                /* Start Task TCB.                                      */
static  OS_TCB   Ex_MainStartTaskTCB;



/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/
#ifdef SLEEP_ENABLED
static void idleHook(void);
static void setupHooks(void);
#endif
static  void  Ex_MainStartTask (void  *p_arg);

void WF200BusCommStart(void);
void wf200_host_setup_memory_pools(void);

#ifdef SLEEP_ENABLED
static bool sleepCallback(SLEEP_EnergyMode_t emode)
{
#ifdef SL_WFX_USE_SPI
	if (GPIO_PinInGet(BSP_EXP_SPI_WIRQPORT,  BSP_EXP_SPI_WIRQPIN))//wf200 messages pending
#else
	if (GPIO_PinInGet(BSP_EXP_WIRQPORT,  BSP_EXP_WIRQPIN)) //wf200 messages pending
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
/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C applications. It is assumed that your code will
*               call main() once you have performed all necessary initialization.
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

int  main (void)
{
    RTOS_ERR  err;
    // Set the HFRCO Frequency
    CMU_HFRCOFreqSet(cmuHFRCOFreq_72M0Hz);
    BSP_SystemInit();                                           /* Initialize System.                                   */
    CPU_Init();                                                 /* Initialize CPU.                                      */
#ifdef SL_WFX_USE_SPI
    CMU_ClockPrescSet(cmuClock_HFPER,0);
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
    printf ("\033[3J");


    OS_TRACE_INIT();                                            /* Initialize trace if enabled                          */
    OSInit(&err);                                               /* Initialize the Kernel.                               */
                                                                /*   Check error code.                                  */
    APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

    BSP_TickInit();
    /* Initialize Kernel tick source.                       */
#ifdef SLEEP_ENABLED
    setupHooks();
#endif
    OSTaskCreate(&Ex_MainStartTaskTCB,                          /* Create the Start Task.                               */
                 "Ex Main Start Task",
                  Ex_MainStartTask,
                  DEF_NULL,
                  EX_MAIN_START_TASK_PRIO,
                 &Ex_MainStartTaskStk[0],
                 (EX_MAIN_START_TASK_STK_SIZE / 10u),
                  EX_MAIN_START_TASK_STK_SIZE,
                  0u,
                  0u,
                  DEF_NULL,
                 (OS_OPT_TASK_STK_CLR),
                 &err);
                                                                /*   Check error code.                                  */
    APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);


    OSStart(&err);                                              /* Start the kernel.                                    */
                                                                /*   Check error code.                                  */
    APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);


    return (1);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/
/***************************************************************************//**
 * @brief Unified GPIO Interrupt handler (pushbuttons)
 *        PB0 Next test
 *        PB1 Previous
 ******************************************************************************/
static void GPIO_Unified_IRQ(void)
{
  RTOS_ERR err;
  // Get and clear all pending GPIO interrupts
  uint32_t interruptMask = GPIO_IntGet();
  GPIO_IntClear(interruptMask);

  // Act on interrupts
  if (interruptMask & 0x400) {
#ifdef  SL_WFX_USE_SPI
	  OSFlagPost(&wf200_evts, WF200_EVENT_FLAG_RX,OS_OPT_POST_FLAG_SET,&err);
#endif
#ifdef SL_WFX_USE_SDIO
#ifdef SLEEP_ENABLED
	  OSFlagPost(&wf200_evts, WF200_EVENT_FLAG_RX,OS_OPT_POST_FLAG_SET,&err);
#endif
#endif
  }
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
void GPIO_EVEN_IRQHandler(void)
{
  GPIO_Unified_IRQ();
}

/***************************************************************************//**
 * @brief GPIO Interrupt handler (PB1)
 ******************************************************************************/
void GPIO_ODD_IRQHandler(void)
{
  GPIO_Unified_IRQ();
}
/****************************************************************************************************//**
 *                                          AppGPIOSetup()
 *
 * @brief  Configure the GPIO pins to read the push buttons.
 *
 * @param  pin  GPIO Pin Number.
 *******************************************************************************************************/
static void AppGPIOSetup(void)
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
  GPIO_PinModeSet(BSP_EXP_RESET_PORT,  BSP_EXP_RESET_PIN,  gpioModePushPull,  0);
  // Configure WF200 WUP Pin
  GPIO_PinModeSet(BSP_EXP_WUP_PORT,  BSP_EXP_WUP_PIN,  gpioModePushPull,  0);
#ifdef  SL_WFX_USE_SPI
  // GPIO used as IRQ
  GPIO_PinModeSet(BSP_EXP_SPI_WIRQPORT,  BSP_EXP_SPI_WIRQPIN,  gpioModeInputPull,  0);
#endif
  CMU_OscillatorEnable(cmuOsc_LFXO,true,true);

#ifdef EFM32GG11B820F2048GM64
  // GPIO used as IRQ
  GPIO_PinModeSet(BSP_EXP_WIRQPORT,  BSP_EXP_WIRQPIN,  gpioModeInputPull,  0);
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
  // Reset and enable associated CPU interrupt vector
  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn);
  NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
  NVIC_EnableIRQ(GPIO_EVEN_IRQn);
}
/*
*********************************************************************************************************
*                                          Ex_MainStartTask()
*
* Description : This is the task that will be called by the Startup when all services are initializes
*               successfully.
*
* Argument(s) : p_arg   Argument passed from task creation. Unused, in this case.
*
* Return(s)   : None.
*
* Notes       : None.
*********************************************************************************************************
*/

static  void  Ex_MainStartTask (void  *p_arg)
{
    RTOS_ERR  err;
    PP_UNUSED_PARAM(p_arg);                                     /* Prevent compiler warning.                            */


#ifdef SL_WFX_USE_SDIO
#ifdef RTOS_MODULE_IO_AVAIL
    IO_Init(&err);
    APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
#endif
#endif

    // Enable GPIO clock.
    CMU_ClockEnable(cmuClock_GPIO, true);

    AppGPIOSetup();
#if (OS_CFG_STAT_TASK_EN == DEF_ENABLED)
    OSStatTaskCPUUsageInit(&err);                               /* Initialize CPU Usage.                                */
                                                                /*   Check error code.                                  */
    APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), ;);
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();                                /* Initialize interrupts disabled measurement.          */
#endif

    Common_Init(&err);                                          /* Call common module initialization.           */
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

#ifdef  RTOS_MODULE_COMMON_SHELL_AVAIL
    Shell_Init(&err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE,; );
#endif

#ifdef  RTOS_MODULE_COMMON_CLK_AVAIL
    Clk_Init(&err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE,; );
#endif

    Auth_Init(&err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE,; );

    BSP_OS_Init();                                              /* Initialize the BSP. It is expected that the BSP ...  */
                                                                /* ... will register all the hardware controller to ... */
                                                     /* ... the platform manager at this moment.             */

    printf ("WF200 Micrium OS LwIP Example\n");
    wf200_host_setup_memory_pools();
    WF200BusCommStart(); //start bus comm task
#ifdef SLEEP_ENABLED
    SLEEP_SleepBlockBegin(sleepEM2);
#endif
    WIFI_CLI_CfgDialog();                                       // Prompt the user for a chance to override the configuration
#ifdef SLEEP_ENABLED
    SLEEP_SleepBlockEnd(sleepEM2);
#endif

    lwip_start();

    while (DEF_ON) {
#if 0

                                                                /* Delay Start Task execution for                       */
        OSTimeDly( 1000,                                        /*   1000 OS Ticks                                      */
                   OS_OPT_TIME_DLY,                             /*   from now.                                          */
                  &err);


        BSP_LedToggle(0);

                                                                /*   Check error code.                                  */
        APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), ;);
#else
        /* Delete the Init Thread */
        OSTaskDel (0, &err);
#endif
    }
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
