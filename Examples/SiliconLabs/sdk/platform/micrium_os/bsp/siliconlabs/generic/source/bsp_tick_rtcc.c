/***************************************************************************//**
 * @file
 * @brief BSP Dynamic Tick using RTC driver
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc.  Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement.  This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

/********************************************************************************************************
 ********************************************************************************************************
 *                                            INCLUDE FILES
 ********************************************************************************************************
 *******************************************************************************************************/

#include "../include/bsp_tick_rtcc.h"
#include <kernel/include/os.h>
#include <common/include/rtos_utils.h>
#include <common/include/toolchains.h>

#if (OS_CFG_DYN_TICK_EN == DEF_ENABLED)

#include <rtcdriver.h>
#include <rtcdrv_config.h>

/********************************************************************************************************
 ********************************************************************************************************
 *                                            LOCAL DEFINES
 ********************************************************************************************************
 *******************************************************************************************************/

#ifndef EMDRV_RTCDRV_WALLCLOCK_CONFIG
#error "EMDRV_RTCDRV_WALLCLOCK_CONFIG MUST be set to 1 when OS_CFG_DYN_TICK_EN is set to DEF_ENABLED in os_cfg.h"
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                       LOCAL GLOBAL VARIABLES
 ********************************************************************************************************
 *******************************************************************************************************/

// The RTCC timer ID
static RTCDRV_TimerID_t RTCC_Id = DEF_INT_32U_MAX_VAL;

// The number of OS ticks that have passed at the last time we updated the OS time.
// This is stored in OS Tick units
static OS_TICK OSLastTick = 0;

// Store the last fetched value of the WallClock counter.
static uint64_t WallClockLastVal = 0;

// Store the last OS Tick value when a WallClock counter overflow is detected.
static uint32_t OSTickOverflowVal = 0;

// Store the last WallClock value when a WallClock counter overflow is detected.
static uint64_t WallClockOverflowVal = 0;

// The number of WallClock ticks that represents when the OS Tick counter overflows
// Should be set at the Initialization and never change afterwards.
static uint64_t WallClockTickNbrForOSTickMax = 0;

/********************************************************************************************************
 ********************************************************************************************************
 *                                      LOCAL FUNCTION PROTOTYPES
 ********************************************************************************************************
 *******************************************************************************************************/

static void RTCC_TickHandler(RTCDRV_TimerID_t id, void *user);

static uint32_t convertWallClockToOSTick(void);

/********************************************************************************************************
 ********************************************************************************************************
 *                                          GLOBAL FUNCTIONS
 ********************************************************************************************************
 *******************************************************************************************************/

/****************************************************************************************************//**
 *                                          BSP_RTCC_TickInit()
 *
 * @brief    Initialize the RTCC module to enable dynamic ticking.
 *
 * @note     (1) This function is called at the start of the Application Start Task, and so other
 *               tasks with more priority could be executed before this Initialization. Therefore, this
 *               function can be use for a lazy Initialization by being called when a RTC timer is needed.
 *******************************************************************************************************/
void BSP_RTCC_TickInit(void)
{
  Ecode_t rtnCode;
  CPU_SR_ALLOC();

  // Check if required OS Tick Rate is respected
  APP_RTOS_ASSERT_DBG(OSCfg_TickRate_Hz == 1000,; );

  // Initialize RTCC module
  (void)RTCDRV_Init();

  // Check if the RTCC module was already initialized
  CPU_CRITICAL_ENTER();
  if (RTCC_Id != DEF_INT_32U_MAX_VAL) {
    CPU_CRITICAL_EXIT();
    return;
  }

  // Allocate a RTCC timer
  rtnCode = RTCDRV_AllocateTimer(&RTCC_Id);

  CPU_CRITICAL_EXIT();

  // Set the number of WallClock ticks that represents when the OS Tick counter overflows
  WallClockTickNbrForOSTickMax = RTCDRV_MsecsToTicks(DEF_INT_32U_MAX_VAL) + RTCDRV_MsecsToTicks(1);

  APP_RTOS_ASSERT_DBG(rtnCode == ECODE_EMDRV_RTCDRV_OK,; );
}

/****************************************************************************************************//**
 *                                           BSP_OS_TickGet()
 *
 * @brief    Get the OS Tick Counter as if it was running continuously.
 *
 * @return   The effective OS Tick Count.
 *******************************************************************************************************/
OS_TICK BSP_OS_TickGet(void)
{
  uint32_t tickElapsed;

  // Get the OS Tick count from the WallClock counter
  tickElapsed = convertWallClockToOSTick();

  return (tickElapsed);
}

/****************************************************************************************************//**
 *                                         BSP_OS_TickNextSet()
 *
 * @brief    Set the number of OS Ticks to wait before calling OSTimeDynTick.
 *
 * @param    ticks   Number of OS Ticks to wait.
 *
 * @return   Number of effective OS Ticks until next OSTimeDynTick.
 *******************************************************************************************************/
OS_TICK BSP_OS_TickNextSet(OS_TICK ticks)
{
  Ecode_t  rtnCode;
  uint32_t timeoutMs = 0;
  OS_TICK  ticksToGo = 0;

  // Convert tick to milliseconds
  if ((ticks != (OS_TICK) -1) && (ticks != 0)) {
    // Do not count pending ticks twice
    ticks -= OSTickCtrPend;

    timeoutMs = ticks;
  }

  if (timeoutMs != 0) {
    uint32_t tickElapsed;
    uint32_t diff;

    // Lazy Initialization of the RTCC module
    BSP_RTCC_TickInit();

    // Get the real timeout value by removing any extra time since the last time the OS Tick was updated
    tickElapsed = convertWallClockToOSTick();
    diff = tickElapsed - OSLastTick;
    if (timeoutMs < diff) {
      timeoutMs = 0;
    } else {
      timeoutMs -= diff;
    }

    // Stop the timer in case it is still running
    rtnCode = RTCDRV_StopTimer(RTCC_Id);
    APP_RTOS_ASSERT_CRITICAL(rtnCode == ECODE_EMDRV_RTCDRV_OK,; );

    // Start the RTCC timer with the new timeout value
    rtnCode = RTCDRV_StartTimer(RTCC_Id, rtcdrvTimerTypeOneshot, timeoutMs, RTCC_TickHandler, DEF_NULL);
    APP_RTOS_ASSERT_CRITICAL(rtnCode == ECODE_EMDRV_RTCDRV_OK,; );

    // Save the OS ticks value associated with the RTCC timeout value
    ticksToGo = ticks;
  }

  return (ticksToGo);
}

/********************************************************************************************************
 ********************************************************************************************************
 *                                           LOCAL FUNCTIONS
 ********************************************************************************************************
 *******************************************************************************************************/

/****************************************************************************************************//**
 *                                          RTCC_TickHandler()
 *
 * @brief    Callback for the RTCC timer.
 *
 * @param    id      The ID of the RTCC timer that timed out.
 *
 * @param    user    Extra callback function parameter for the user application.
 *
 * @note     (1) Called from the RTCC ISR handler in the rtcdriver.
 *******************************************************************************************************/
static void RTCC_TickHandler(RTCDRV_TimerID_t id, void *user)
{
  uint32_t tickElapsed;
  uint32_t tick;

  PP_UNUSED_PARAM(user);

  if (id == RTCC_Id) {
    // Get the OS Tick count from the WallClock counter
    tickElapsed = convertWallClockToOSTick();

    // Get the number of OS Tick since the last update
    tick = tickElapsed - OSLastTick;

    // Update Last OS Tick reference
    OSLastTick = tickElapsed;

    // Tell the OS how many ticks that have passed
    OSTimeDynTick(tick);
  }
}

/****************************************************************************************************//**
 *                                      convertWallClockToOSTick()
 *
 * @brief    Get WallClock counter value and convert it to OS Tick.
 *
 * @return   The effective OS Tick Count.
 *******************************************************************************************************/
static uint32_t convertWallClockToOSTick(void)
{
  uint64_t wclockTick;
  uint64_t modulo;
  uint32_t wclockOSTick;

  // Get the WallClock value
  wclockTick = RTCDRV_GetWallClockTicks64();

  // Check if no time has elapsed since last time
  if (WallClockLastVal == wclockTick) {
    return (OSLastTick);
  }

  // Check if the WallClock counter overflowed
  if (wclockTick < WallClockLastVal) {
    WallClockOverflowVal = WallClockLastVal;
    OSTickOverflowVal = OSLastTick;
  }

  WallClockLastVal = wclockTick;

  // Get the WallClock value in ms/OSTick by taking into account previous WallClock overflows
  modulo = (wclockTick - WallClockOverflowVal) % WallClockTickNbrForOSTickMax;
  wclockOSTick = RTCDRV_TicksToMsec64(modulo) + OSTickOverflowVal;

  return (wclockOSTick);
}

#endif // (OS_CFG_DYN_TICK_EN == DEF_ENABLED)
