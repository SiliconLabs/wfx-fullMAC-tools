/***************************************************************************//**
 * @file
 * @brief Common - Clock - Calendar
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
 *                                               INCLUDE FILES
 ********************************************************************************************************
 *******************************************************************************************************/

#include  <common/include/clk.h>
#include  <common/include/rtos_err.h>
#include  <common/include/rtos_prio.h>
#include  <common/include/toolchains.h>

#if (CLK_CFG_EXT_EN == DEF_DISABLED)
#include  <common/source/kal/kal_priv.h>
#endif

#include  <common/source/clk/clk_cmd_priv.h>
#include  <common/source/rtos/rtos_utils_priv.h>
#include  <common/source/logging/logging_priv.h>

/********************************************************************************************************
 ********************************************************************************************************
 *                                               LOCAL DEFINES
 ********************************************************************************************************
 *******************************************************************************************************/

#define  LOG_DFLT_CH                               (COMMON, CLK)
#define  RTOS_MODULE_CUR                            RTOS_CFG_MODULE_COMMON

#if ((CLK_CFG_EXT_EN == DEF_DISABLED) \
  && (CLK_CFG_SIGNAL_EN == DEF_DISABLED))
#define  CLK_KAL_TASK_NAME                         "Real time Clk"

#define  CLK_INIT_CFG_DFLT                          { \
    .StkSizeElements = 128u,                          \
    .StkPtr = DEF_NULL,                               \
    .MemSegPtr = DEF_NULL                             \
}
#else
#define  CLK_INIT_CFG_DFLT                          { \
    .MemSegPtr = DEF_NULL                             \
}
#endif

#define  CLK_INITIAL_TZ_SEC_DFLT                    CLK_TZ_SEC_FROM_UTC_GET(0u)

/********************************************************************************************************
 ********************************************************************************************************
 *                                           LOCAL DATA TYPES
 ********************************************************************************************************
 *******************************************************************************************************/

/********************************************************************************************************
 *                                   CLOCK PERIODIC TICK COUNTER DATA TYPE
 *******************************************************************************************************/

#if ((CLK_CFG_EXT_EN == DEF_DISABLED) \
  && (CLK_CFG_SIGNAL_EN == DEF_ENABLED))
typedef CPU_INT32U CLK_TICK_CTR;
#endif

/********************************************************************************************************
 *                                           CLOCK DATA DATA TYPE
 *******************************************************************************************************/

typedef struct clk_data {
#if  (CLK_CFG_EXT_EN == DEF_DISABLED)
  CLK_TS_SEC      Clk_TS_UTC_sec;
#if  (CLK_CFG_SIGNAL_EN == DEF_ENABLED)
  CLK_TICK_CTR    Clk_TickCtr;
#else
  KAL_TASK_HANDLE Clk_TaskHandle;
#endif
#endif

  CLK_TZ_SEC Clk_TZ_sec;
} CLK_DATA;

/********************************************************************************************************
 ********************************************************************************************************
 *                                               LOCAL TABLES
 ********************************************************************************************************
 *******************************************************************************************************/

static const CLK_DAY Clk_DaysInYr[2u] = {
  DEF_TIME_NBR_DAY_PER_YR, DEF_TIME_NBR_DAY_PER_YR_LEAP
};

static const CLK_DAY Clk_DaysInMonth[2u][CLK_MONTH_PER_YR] = {
  //Jan  Feb  Mar  Apr  May  Jun  Jul  Aug  Sep  Oct  Nov  Dec
  { 31u, 28u, 31u, 30u, 31u, 30u, 31u, 31u, 30u, 31u, 30u, 31u },
  { 31u, 29u, 31u, 30u, 31u, 30u, 31u, 31u, 30u, 31u, 30u, 31u }
};

/********************************************************************************************************
 *                                           CLK STR CONV TBLS
 *******************************************************************************************************/

#if (CLK_CFG_STR_CONV_EN == DEF_ENABLED)
static const CPU_CHAR *  const Clk_StrMonth[CLK_MONTH_PER_YR] = {
  // 1
  // 01234567890
  (const  CPU_CHAR *)"January",
  (const  CPU_CHAR *)"February",
  (const  CPU_CHAR *)"March",
  (const  CPU_CHAR *)"April",
  (const  CPU_CHAR *)"May",
  (const  CPU_CHAR *)"June",
  (const  CPU_CHAR *)"July",
  (const  CPU_CHAR *)"August",
  (const  CPU_CHAR *)"September",
  (const  CPU_CHAR *)"October",
  (const  CPU_CHAR *)"November",
  (const  CPU_CHAR *)"December"
};

static const CPU_CHAR *  const Clk_StrDayOfWk[DEF_TIME_NBR_DAY_PER_WK] = {
  // 1
  // 01234567890
  (const  CPU_CHAR *)"Sunday",
  (const  CPU_CHAR *)"Monday",
  (const  CPU_CHAR *)"Tuesday",
  (const  CPU_CHAR *)"Wednesday",
  (const  CPU_CHAR *)"Thursday",
  (const  CPU_CHAR *)"Friday",
  (const  CPU_CHAR *)"Saturday"
};
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                       LOCAL GLOBAL VARIABLES
 ********************************************************************************************************
 *******************************************************************************************************/

static CLK_DATA *Clk_Ptr = DEF_NULL;

#if (RTOS_CFG_EXTERNALIZE_OPTIONAL_CFG_EN == DEF_DISABLED)
const CLK_INIT_CFG  Clk_InitCfgDflt = CLK_INIT_CFG_DFLT;
static CLK_INIT_CFG Clk_InitCfg = CLK_INIT_CFG_DFLT;
#else
extern const CLK_INIT_CFG Clk_InitCfg;
#endif

/********************************************************************************************************
 *                                               CLK CACHE DATA
 ********************************************************************************************************
 * Note(s) : (1) Since global performance can be affected by the calculation of the days of week some
 *               caches have been implemented:
 *
 *               (a) Clk_CacheMonth store the last month requested.
 *
 *               (b) Clk_CacheMonthDays store the last number of days calculated for the month stored in
 *                   Clk_CacheMonth.
 *
 *               (c) Clk_CacheYr store the last year requested.
 *
 *               (d) Clk_CacheYrDays store the last number of days calculated for the year stored in
 *                   Clk_CacheYr.
 *******************************************************************************************************/

static CLK_MONTH    Clk_CacheMonth;                             // See Note #1a.
static CLK_DAY      Clk_CacheMonthDays;                         // See Note #1b.
static CLK_YR       Clk_CacheYr;                                // See Note #1c.
static CLK_NBR_DAYS Clk_CacheYrDays;                            // See Note #1d.

/********************************************************************************************************
 ********************************************************************************************************
 *                                       LOCAL FUNCTIONS PROTOTYPES
 ********************************************************************************************************
 *******************************************************************************************************/

static CPU_BOOLEAN Clk_IsLeapYr(CLK_YR yr);

static CPU_BOOLEAN Clk_IsDateValid(CLK_YR    yr,
                                   CLK_MONTH month,
                                   CLK_DAY   day,
                                   CLK_YR    yr_start,
                                   CLK_YR    yr_end);

static CPU_BOOLEAN Clk_IsDayOfYrValid(CLK_YR  yr,
                                      CLK_DAY day_of_yr);

static CPU_BOOLEAN Clk_IsDayOfWkValid(CLK_DAY day_of_wk);

static CPU_BOOLEAN Clk_IsTimeValid(CLK_HR    hr,
                                   CLK_MONTH min,
                                   CLK_DAY   sec);

static CPU_BOOLEAN Clk_IsTZValid(CLK_TZ_SEC tz_sec);

static CPU_BOOLEAN Clk_IsDateTimeValidHandler(CLK_DATE_TIME *p_date_time,
                                              CLK_YR        yr_start,
                                              CLK_YR        yr_end);

static CLK_DAY Clk_GetDayOfYrHandler(CLK_YR    yr,
                                     CLK_MONTH month,
                                     CLK_DAY   day);

static CLK_DAY Clk_GetDayOfWkHandler(CLK_YR    yr,
                                     CLK_MONTH month,
                                     CLK_DAY   day);

static void Clk_SetTZ_Handler(CLK_TZ_SEC tz_sec);

static CPU_BOOLEAN Clk_TS_ToDateTimeHandler(CLK_TS_SEC    ts_sec,
                                            CLK_TZ_SEC    tz_sec,
                                            CLK_DATE_TIME *p_date_time,
                                            CLK_YR        yr_start,
                                            CLK_YR        yr_end);

static CPU_BOOLEAN Clk_DateTimeToTS_Handler(CLK_TS_SEC    *p_ts_sec,
                                            CLK_DATE_TIME *p_date_time,
                                            CLK_YR        yr_start);

static CPU_BOOLEAN Clk_DateTimeMakeHandler(CLK_DATE_TIME *p_date_time,
                                           CLK_YR        yr,
                                           CLK_MONTH     month,
                                           CLK_DAY       day,
                                           CLK_HR        hr,
                                           CLK_MIN       min,
                                           CLK_SEC       sec,
                                           CLK_TZ_SEC    tz_sec,
                                           CLK_YR        yr_start,
                                           CLK_YR        yr_end);

#if ((CLK_CFG_EXT_EN == DEF_DISABLED) \
  && (CLK_CFG_SIGNAL_EN == DEF_DISABLED))
static void Clk_TaskHandler(void *p_arg);
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                           GLOBAL FUNCTIONS
 ********************************************************************************************************
 *******************************************************************************************************/

/****************************************************************************************************//**
 *                                           Clk_ConfigureTaskStk()
 *
 * @brief    Configure the clock task stack properties to use the parameters contained in the passed
 *           structure instead of the default parameters.
 *
 * @param    stk_size_elements   Size of the stack, in CPU_STK elements.
 *
 * @param    p_stk_base          Pointer to base of the stack.
 *
 * @note     (1) This function is optional. If it is called, it must be called before Clk_Init(). If
 *               it is not called, default values will be used to initialize the module.
 *******************************************************************************************************/

#if ((CLK_CFG_EXT_EN == DEF_DISABLED)    \
  && (CLK_CFG_SIGNAL_EN == DEF_DISABLED) \
  && (RTOS_CFG_EXTERNALIZE_OPTIONAL_CFG_EN == DEF_DISABLED))
void Clk_ConfigureTaskStk(CPU_STK_SIZE stk_size_elements,
                          CPU_STK      *p_stk_base)
{
  RTOS_ASSERT_DBG((Clk_Ptr == DEF_NULL), RTOS_ERR_ALREADY_INIT,; );

  Clk_InitCfg.StkSizeElements = stk_size_elements;
  Clk_InitCfg.StkPtr = p_stk_base;
}
#endif

/****************************************************************************************************//**
 *                                           Clk_ConfigureMemSeg()
 *
 * @brief    Configure the memory segment that will be used to allocate internal data needed by Clock
 *           instead of the default memory segment.
 *
 * @param    p_mem_seg   Pointer to the memory segment from which the internal data will be allocated.
 *                       If DEF_NULL, the internal data will be allocated from the global Heap.
 *
 * @note     (1) This function is optional. If it is called, it must be called before Clk_Init(). If
 *               it is not called, default values will be used to initialize the module.
 *******************************************************************************************************/

#if (RTOS_CFG_EXTERNALIZE_OPTIONAL_CFG_EN == DEF_DISABLED)
void Clk_ConfigureMemSeg(MEM_SEG *p_mem_seg)
{
  RTOS_ASSERT_DBG((Clk_Ptr == DEF_NULL), RTOS_ERR_ALREADY_INIT,; );

  Clk_InitCfg.MemSegPtr = p_mem_seg;
}
#endif

/****************************************************************************************************//**
 *                                               Clk_Init()
 *
 * @brief    Initializes the Clock module as follows :
 *               - (a) Initialize     the Clock module as follows :
 *                   - (1) Retrieving an external timestamp, if available.
 *                   - (2) Initializing the clock/operating system.
 *               - (b) Initialize the Clock module variables.
 *
 * @param    p_err   Pointer to the variable that will receive one of the following error code(s) from
 *                   this function:
 *                       - RTOS_ERR_NONE
 *                       - RTOS_ERR_OWNERSHIP
 *                       - RTOS_ERR_ALREADY_EXISTS
 *                       - RTOS_ERR_BLK_ALLOC_CALLBACK
 *                       - RTOS_ERR_SEG_OVF
 *                       - RTOS_ERR_OS_SCHED_LOCKED
 *                       - RTOS_ERR_OS_ILLEGAL_RUN_TIME
 *                       - RTOS_ERR_NOT_AVAIL
 *                       - RTOS_ERR_POOL_EMPTY
 *                       - RTOS_ERR_WOULD_OVF
 *                       - RTOS_ERR_OS_OBJ_DEL
 *                       - RTOS_ERR_WOULD_BLOCK
 *                       - RTOS_ERR_INVALID_ARG
 *                       - RTOS_ERR_NO_MORE_RSRC
 *                       - RTOS_ERR_IS_OWNER
 *                       - RTOS_ERR_ABORT
 *                       - RTOS_ERR_TIMEOUT
 *
 * @note     (1) The functions Clk_Configure...() can be used to configure more specific properties of
 *               the Clk sub-module, when RTOS_CFG_EXTERNALIZE_OPTIONAL_CFG_EN is set to DEF_DISABLED.
 *               If set to DEF_ENABLED, the structure Clk_InitCfg needs to be declared and filled by
 *               the application to configure these specific properties for the module.
 *******************************************************************************************************/
void Clk_Init(RTOS_ERR *p_err)
{
  RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err,; );

  Clk_Ptr = (CLK_DATA *)Mem_SegAlloc("Clk - Data",              // Allocate data needed by Clk.
                                     Clk_InitCfg.MemSegPtr,
                                     sizeof(CLK_DATA),
                                     p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }

  //                                                               ---------------- INIT CLK VARIABLES ----------------
#if (CLK_CFG_EXT_EN == DEF_DISABLED)
  Clk_Ptr->Clk_TS_UTC_sec = CLK_TS_SEC_NONE;                    // Clk epoch = 2000-01-01 00:00:00 UTC
#if (CLK_CFG_SIGNAL_EN == DEF_ENABLED)
  Clk_Ptr->Clk_TickCtr = CLK_TICK_NONE;
#endif
#endif
  Clk_Ptr->Clk_TZ_sec = CLK_INITIAL_TZ_SEC_DFLT;                // Clk TZ = UTC offset
  Clk_CacheMonth = CLK_MONTH_NONE;
  Clk_CacheMonthDays = CLK_DAY_NONE;
  Clk_CacheYr = CLK_YR_NONE;
  Clk_CacheYrDays = CLK_DAY_NONE;

#if ((CLK_CFG_EXT_EN == DEF_DISABLED) \
  && (CLK_CFG_SIGNAL_EN == DEF_DISABLED))                       // ------------------- CLK/OS INIT --------------------
  {
    CPU_BOOLEAN kal_feat_is_ok;

    kal_feat_is_ok = KAL_FeatureQuery(KAL_FEATURE_TASK_CREATE, KAL_OPT_NONE);
    kal_feat_is_ok &= KAL_FeatureQuery(KAL_FEATURE_DLY, KAL_OPT_DLY_NONE);
    kal_feat_is_ok &= KAL_FeatureQuery(KAL_FEATURE_PEND_TIMEOUT, KAL_OPT_NONE);
    kal_feat_is_ok &= KAL_FeatureQuery(KAL_FEATURE_TICK_GET, KAL_OPT_NONE);

    RTOS_ASSERT_DBG_ERR_SET((kal_feat_is_ok == DEF_OK), *p_err, RTOS_ERR_NOT_AVAIL,; );

    //                                                             ----------------- INITIALIZE CLK TASK -----------------
    Clk_Ptr->Clk_TaskHandle = KAL_TaskAlloc(CLK_KAL_TASK_NAME,
                                            Clk_InitCfg.StkPtr,
                                            Clk_InitCfg.StkSizeElements,
                                            DEF_NULL,
                                            p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
      return;
    }

    KAL_TaskCreate(Clk_Ptr->Clk_TaskHandle,
                   Clk_TaskHandler,
                   DEF_NULL,
                   CLK_TASK_CFG_PRIO_DFLT,
                   DEF_NULL,
                   p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
      return;
    }
  }
#elif (CLK_CFG_EXT_EN == DEF_ENABLED)                           // ------------------- INIT EXT TS --------------------
  Clk_ExtTS_Init();
#endif

#ifdef  RTOS_MODULE_COMMON_SHELL_AVAIL
  ClkCmd_Init(p_err);                                           // Add Clk commands table.
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }
#endif
}

/****************************************************************************************************//**
 *                                               Clk_TaskPrioSet()
 *
 * @brief    Sets the priority of the internal clock task.
 *
 * @param    prio    New priority at which to set the clock task.
 *
 * @param    p_err   Pointer to the variable that will receive one of the following error code(s) from
 *                   this function:
 *                       - RTOS_ERR_NONE
 *                       - RTOS_ERR_INVALID_ARG
 *
 * @note     (1) This function must be called after Clk_Init().
 *******************************************************************************************************/

#if ((CLK_CFG_EXT_EN == DEF_DISABLED) \
  && (CLK_CFG_SIGNAL_EN == DEF_DISABLED))
void Clk_TaskPrioSet(RTOS_TASK_PRIO prio,
                     RTOS_ERR       *p_err)
{
  RTOS_ASSERT_DBG((Clk_Ptr != DEF_NULL), RTOS_ERR_NOT_INIT,; );

  KAL_TaskPrioSet(Clk_Ptr->Clk_TaskHandle, prio, p_err);
}
#endif

/****************************************************************************************************//**
 *                                               Clk_SignalClk()
 *
 * @brief    Increments the clock tick counter as follows :
 *               - (a) Increment the clock tick counter.
 *               - (b) Signal the clock task when one second has elapsed.
 *
 * @param    p_err   Pointer to the variable that will receive one of the following error code(s) from
 *                   this function:
 *                       - RTOS_ERR_NONE
 *                       - RTOS_ERR_NOT_AVAIL
 *                       - RTOS_ERR_WOULD_OVF
 *
 * @note     (1) CLK_CFG_SIGNAL_FREQ_HZ must be set to the number of times Clk_SignalClk() will be
 *               called per second.
 *
 * @note     (2) 'Clk_TickCtr' MUST ALWAYS be accessed exclusively in critical sections.
 *******************************************************************************************************/

#if ((CLK_CFG_EXT_EN == DEF_DISABLED) \
  && (CLK_CFG_SIGNAL_EN == DEF_ENABLED))
void Clk_SignalClk(RTOS_ERR *p_err)
{
  CPU_SR_ALLOC();

  RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err,; );

  CPU_CRITICAL_ENTER();                                         // See Note #2.
  Clk_Ptr->Clk_TickCtr++;
  if (Clk_Ptr->Clk_TickCtr >= CLK_CFG_SIGNAL_FREQ_HZ) {
    Clk_Ptr->Clk_TickCtr -= CLK_CFG_SIGNAL_FREQ_HZ;             // See Note #1.
                                                                // -------------------- UPDATE TS ---------------------
    if (Clk_Ptr->Clk_TS_UTC_sec < CLK_TS_SEC_MAX) {
      Clk_Ptr->Clk_TS_UTC_sec++;
    }
  }
  CPU_CRITICAL_EXIT();
}
#endif

/****************************************************************************************************//**
 *                                               Clk_GetTS()
 *
 * @brief    Gets the current Clock timestamp.
 *
 * @return   Current timestamp (in seconds, UTC+00).
 *
 * @note     (1) Internal Clock timestamp ('Clk_TS_UTC_sec') should be set to UTC+00. When calling
 *               Clk_GetTS(), the local time zone offset MUST be applied.
 *
 * @note     (2) 'Clk_TS_UTC_sec' MUST ALWAYS be accessed exclusively in critical sections.
 *******************************************************************************************************/
CLK_TS_SEC Clk_GetTS(void)
{
  CLK_TS_SEC ts_sec;
#if (CLK_CFG_EXT_EN == DEF_DISABLED)
  CPU_SR_ALLOC();

  CPU_CRITICAL_ENTER();
  ts_sec = Clk_Ptr->Clk_TS_UTC_sec;
  CPU_CRITICAL_EXIT();
#else
  ts_sec = Clk_ExtTS_Get();
#endif

  return (ts_sec);
}

/****************************************************************************************************//**
 *                                               Clk_SetTS()
 *
 * @brief    Sets the Clock timestamp.
 *
 * @param    ts_sec  Current timestamp to set (in seconds, UTC+00).
 *
 * @return   DEF_OK, if the timestamp is successfully set.
 *           DEF_FAIL, otherwise.
 *
 * @note     (1) Internal Clock timestamp ('Clk_TS_UTC_sec') should be set to UTC+00 and should
 *               NOT include a local time zone offset.
 *
 * @note     (2) 'Clk_TS_UTC_sec' MUST ALWAYS be accessed exclusively in critical sections.
 *******************************************************************************************************/
CPU_BOOLEAN Clk_SetTS(CLK_TS_SEC ts_sec)
{
  CPU_BOOLEAN valid;
#if (CLK_CFG_EXT_EN == DEF_DISABLED)
  CPU_SR_ALLOC();

  CPU_CRITICAL_ENTER();
  Clk_Ptr->Clk_TS_UTC_sec = ts_sec;
  CPU_CRITICAL_EXIT();

  valid = DEF_OK;
#else
  valid = Clk_ExtTS_Set(ts_sec);
#endif

  return (valid);
}

/****************************************************************************************************//**
 *                                               Clk_GetTZ()
 *
 * @brief    Gets the Clock time zone offset.
 *
 * @return   Time zone offset (in seconds, +|- from UTC).
 *
 * @note     (1) Time zone is based on Coordinated Universal Time (UTC) and uses the following values :
 *               - (a) Between  +|- 12 hours (+|- 43200 seconds)
 *               - (b) Multiples of a 15 minute interval
 *
 * @note     (2) 'Clk_TZ_sec' MUST ALWAYS be accessed exclusively in critical sections.
 *******************************************************************************************************/
CLK_TZ_SEC Clk_GetTZ(void)
{
  CLK_TZ_SEC tz_sec;
  CPU_SR_ALLOC();

  CPU_CRITICAL_ENTER();
  tz_sec = Clk_Ptr->Clk_TZ_sec;
  CPU_CRITICAL_EXIT();

  return (tz_sec);
}

/****************************************************************************************************//**
 *                                               Clk_SetTZ()
 *
 * @brief    Sets the Clock time zone offset.
 *
 * @param    tz_sec  Time zone offset (in seconds, +|- from UTC).
 *
 * @return   DEF_OK, if the time zone is valid and set.
 *           DEF_FAIL, otherwise.
 *
 * @note     (1) Time zone is based on Coordinated Universal Time (UTC) and uses the following values :
 *               - (a) Between  +|- 12 hours (+|- 43200 seconds)
 *               - (b) Multiples of 15 minute intervals
 *
 * @note     (2) Time zone is not applied on the current Clock timestamp.
 *******************************************************************************************************/
CPU_BOOLEAN Clk_SetTZ(CLK_TZ_SEC tz_sec)
{
  CPU_BOOLEAN valid;

  //                                                               ------------------- VALIDATE TZ --------------------
  valid = Clk_IsTZValid(tz_sec);
  if (valid != DEF_YES) {
    return  (DEF_FAIL);
  }

  //                                                               ---------------------- SET TZ ----------------------
  Clk_SetTZ_Handler(tz_sec);

  return (DEF_OK);
}

/****************************************************************************************************//**
 *                                               Clk_GetDateTime()
 *
 * @brief    Gets the current Clock timestamp as a date/time structure.
 *
 * @param    p_date_time     Pointer to the variable that will receive the date/time structure.
 *
 * @return   DEF_OK, if the timestamp is valid & date/time structure successfully returned.
 *           DEF_FAIL, otherwise.
 *
 * @note     (1) Internal Clock time zone offset ('Clk_TZ_sec') calculates the returned local
 *               date/time ('p_date_time'). This means the local date/time is returned at UTC+TZ,
 *               where Clock time zone offset (TZ) is returned as a local time zone offset
 *               ('p_date_time->TZ_sec').
 *******************************************************************************************************/
CPU_BOOLEAN Clk_GetDateTime(CLK_DATE_TIME *p_date_time)
{
  CLK_TS_SEC  ts_sec;
  CLK_TZ_SEC  tz_sec;
  CPU_BOOLEAN valid;

  ts_sec = Clk_GetTS();                                         // ---------------------- GET TS ----------------------
  tz_sec = Clk_GetTZ();                                         // ---------------------- GET TZ ----------------------

  //                                                               ------------- CONV CLK TS TO DATE/TIME -------------
  valid = Clk_TS_ToDateTime(ts_sec,
                            tz_sec,
                            p_date_time);
  if (valid != DEF_OK) {
    return  (DEF_FAIL);
  }

  return (DEF_OK);
}

/****************************************************************************************************//**
 *                                               Clk_SetDateTime()
 *
 * @brief    Sets the Clock timestamp using a date/time structure.
 *
 * @param    p_date_time     Pointer to the variable that contains the date/time structure.
 *
 * @return   DEF_OK, if the Clock date/time is successfully set.
 *           DEF_FAIL, otherwise.
 *
 * @note     (1) Date/time ('p_date_time') should be set to local time with the correct time zone
 *               offset ('p_date_time->TZ_sec'). Clk_SetDateTime() removes the time zone offset
 *               from the date/time to calculate the Clock timestamp at UTC+00.
 *               @n
 *               Internal Clock time zone offset ('Clk_TZ_sec') is set to the local time zone
 *               offset ('p_date_time->TZ_sec').
 *               @n
 *               Time zone is based on Coordinated Universal Time (UTC) and uses the following values :
 *               - (a) Between  +|- 12 hours (+|- 43200 seconds)
 *               - (b) Multiples of a 15 minute interval
 *******************************************************************************************************/
CPU_BOOLEAN Clk_SetDateTime(CLK_DATE_TIME *p_date_time)
{
  CLK_TS_SEC  ts_sec;
  CPU_BOOLEAN valid;

  //                                                               ------------- CONV DATE/TIME TO CLK TS -------------
  valid = Clk_DateTimeToTS(&ts_sec, p_date_time);               // Validates date/time & TZ (see Note #1c).
  if (valid != DEF_OK) {
    return  (DEF_FAIL);
  }

  //                                                               ---------------------- SET TS ----------------------
  valid = Clk_SetTS(ts_sec);                                    // See Note #1.
  if (valid != DEF_OK) {
    return  (DEF_FAIL);
  }

  //                                                               ---------------------- SET TZ ----------------------
  Clk_SetTZ_Handler(p_date_time->TZ_sec);                       // See Note #1.

  return (DEF_OK);
}

/****************************************************************************************************//**
 *                                           Clk_TS_ToDateTime()
 *
 * @brief    Converts the Clock timestamp to a date/time structure.
 *
 * @param    ts_sec          Timestamp to convert (in seconds,          UTC+00).
 *
 * @param    tz_sec          Time zone offset     (in seconds, +|- from UTC).
 *
 * @param    p_date_time     Pointer to the variable that receives the date/time structure.
 *
 * @return   DEF_OK, if date/time structure successfully returned.
 *           DEF_FAIL, otherwise.
 *
 * @note     (1) Timestamp ('ts_sec') MUST be set for UTC+00 & should not include the time
 *               zone offset ('tz_sec') since Clk_TS_ToDateTime() includes the time zone
 *               offset in its date/time calculation. Whether before or after calling
 *               Clk_TS_ToDateTime(), the time zone offset should NOT be applied.
 *               @n
 *               Time zone field of the date/time structure ('p_date_time->TZ_sec') is set
 *               to the value of the time zone argument ('tz_sec').
 *               @n
 *               Time zone is based on Coordinated Universal Time (UTC) and uses the following values :
 *               - (a) Between  +|- 12 hours (+|- 43200 seconds)
 *               - (b) Multiples of a 15 minute interval
 *******************************************************************************************************/
CPU_BOOLEAN Clk_TS_ToDateTime(CLK_TS_SEC    ts_sec,
                              CLK_TZ_SEC    tz_sec,
                              CLK_DATE_TIME *p_date_time)
{
  CPU_BOOLEAN valid;

  //                                                               ------------- CONV CLK TS TO DATE/TIME -------------
  valid = Clk_TS_ToDateTimeHandler(ts_sec,
                                   tz_sec,
                                   p_date_time,
                                   CLK_EPOCH_YR_START,
                                   CLK_EPOCH_YR_END);
  if (valid != DEF_OK) {
    return  (DEF_FAIL);
  }

  return (DEF_OK);
}

/****************************************************************************************************//**
 *                                           Clk_DateTimeToTS()
 *
 * @brief    Converts the date/time structure to the Clock timestamp.
 *
 * @param    p_ts_sec        Pointer to the variable that receives the Clock timestamp :
 *                           In seconds UTC+00, if NO error(s);
 *                           CLK_TS_SEC_NONE, otherwise.
 *
 * @param    p_date_time     Pointer to the variable that contains date/time structure to convert.
 *
 * @return   DEF_OK, if the timestamp has successfully returned.
 *           DEF_FAIL, otherwise.
 *
 * @note     (1) Date/time structure ('p_date_time') MUST be representable in the Clock timestamp,
 *               so the date to convert MUST be :
 *               - (a) >= CLK_EPOCH_YR_START
 *               - (b) <  CLK_EPOCH_YR_END
 *
 * @note     (2) Date/time ('p_date_time') should be set to local time with correct time zone
 *               offset ('p_date_time->TZ_sec'). Clk_DateTimeToTS() removes the time zone
 *               offset from the date/time to calculate and return a Clock timestamp at UTC+00.
 *               @n
 *               Time zone is based on Coordinated Universal Time (UTC) and uses the following values :
 *               - (a) Between  +|- 12 hours (+|- 43200 seconds)
 *               - (b) Multiples of a 15 minute interval
 *
 * @note     (3) In case of errors, pointers to variables that return values MUST be initialized
 *               before all other validation or function handling.
 *******************************************************************************************************/
CPU_BOOLEAN Clk_DateTimeToTS(CLK_TS_SEC    *p_ts_sec,
                             CLK_DATE_TIME *p_date_time)
{
  CPU_BOOLEAN valid;

  RTOS_ASSERT_DBG((p_ts_sec != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);

  *p_ts_sec = CLK_TS_SEC_NONE;                                  // Init to ts none for err (see Note #3).

  //                                                               ---------------- VALIDATE DATE/TIME ----------------
  valid = Clk_IsDateTimeValid(p_date_time);
  if (valid != DEF_OK) {
    return  (DEF_FAIL);
  }

  //                                                               ------------- CONV DATE/TIME TO CLK TS -------------
  valid = Clk_DateTimeToTS_Handler(p_ts_sec,
                                   p_date_time,
                                   CLK_EPOCH_YR_START);
  if (valid != DEF_OK) {
    return  (DEF_FAIL);
  }

  return (DEF_OK);
}

/****************************************************************************************************//**
 *                                           Clk_DateTimeMake()
 *
 * @brief    Builds a valid Clock epoch date/time structure.
 *
 * @param    p_date_time     Pointer to the variable that will receive the date/time structure.
 *
 * @param    yr              Year    value [CLK_EPOCH_YR_START to CLK_EPOCH_YR_END) [see Note #1].
 *
 * @param    month           Month   value [     CLK_MONTH_JAN to    CLK_MONTH_DEC].
 *
 * @param    day             Day     value [                 1 to               31].
 *
 * @param    hr              Hours   value [                 0 to               23].
 *
 * @param    min             Minutes value [                 0 to               59].
 *
 * @param    sec             Seconds value [                 0 to               60] (see Note #3).
 *
 * @param    tz_sec          Time zone offset (in seconds, +|- from UTC) [-43200 to 43200].
 *
 * @return   DEF_OK, if date/time structure successfully returned.
 *           DEF_FAIL, otherwise.
 *
 * @note     (1) Date/time structure ('p_date_time') MUST be representable in the Clock timestamp,
 *               so the date to convert MUST be :
 *               - (a) >= CLK_EPOCH_YR_START
 *               - (b) <  CLK_EPOCH_YR_END
 *
 * @note     (2) If the date is valid, the Day of week ('p_date_time->DayOfWk') and Day of year
 *               ('p_date_time->DayOfYr') are internally calculated and set in the date/time structure.
 *
 * @note     (3) A Seconds value of 60 is valid to ensure compatiblity with leap second adjustment and
 *               the atomic clock time structure.
 *
 * @note     (4) Time zone is based ('tz_sec') on Coordinated Universal Time (UTC) and uses the
 *               following values :
 *               - (a) Between  +|- 12 hours (+|- 43200 seconds)
 *               - (b) Multiples of a 15 minute interval
 *******************************************************************************************************/
CPU_BOOLEAN Clk_DateTimeMake(CLK_DATE_TIME *p_date_time,
                             CLK_YR        yr,
                             CLK_MONTH     month,
                             CLK_DAY       day,
                             CLK_HR        hr,
                             CLK_MIN       min,
                             CLK_SEC       sec,
                             CLK_TZ_SEC    tz_sec)
{
  CPU_BOOLEAN valid;

  //                                                               ---------- VALIDATE & CONV CLK DATE/TIME -----------
  valid = Clk_DateTimeMakeHandler(p_date_time,
                                  yr,
                                  month,
                                  day,
                                  hr,
                                  min,
                                  sec,
                                  tz_sec,
                                  CLK_EPOCH_YR_START,
                                  CLK_EPOCH_YR_END);
  if (valid != DEF_YES) {
    return  (DEF_FAIL);
  }

  return (DEF_OK);
}

/****************************************************************************************************//**
 *                                           Clk_IsDateTimeValid()
 *
 * @brief    Determines if the date/time structure is valid in the Clock epoch.
 *
 * @param    p_date_time     Pointer to the variable that contains the date/time structure to validate.
 *
 * @return   DEF_YES, if date/time structure is valid.
 *           DEF_NO, otherwise.
 *
 * @note     (1) Date/time structure ('p_date_time') MUST be representable in Clock epoch, so
 *               the date to validate MUST be :
 *               - (a) >= CLK_EPOCH_YR_START
 *               - (b) <  CLK_EPOCH_YR_END
 *
 * @note     (2) Time zone is based ('p_date_time->TZ_sec') on Coordinated Universal Time (UTC)
 *               and uses the following values :
 *               - (a) Between  +|- 12 hours (+|- 43200 seconds)
 *               - (b) Multiples of a 15 minute interval
 *******************************************************************************************************/
CPU_BOOLEAN Clk_IsDateTimeValid(CLK_DATE_TIME *p_date_time)
{
  CPU_BOOLEAN valid;

  //                                                               -------------- VALIDATE CLK DATE/TIME --------------
  valid = Clk_IsDateTimeValidHandler(p_date_time,
                                     CLK_EPOCH_YR_START,
                                     CLK_EPOCH_YR_END);
  if (valid != DEF_YES) {
    return  (DEF_NO);
  }

  return (DEF_YES);
}

/****************************************************************************************************//**
 *                                               Clk_GetDayOfWk()
 *
 * @brief    Gets the day of week.
 *
 * @param    yr      Year  value [1900 to 2135] (see Note #1).
 *
 * @param    month   Month value [   1 to   12] / [CLK_MONTH_JAN to CLK_MONTH_DEC].
 *
 * @param    day     Day   value [   1 to   31].
 *
 * @return   Day of week [1 to 7] / [CLK_DAY_OF_WK_SUN to CLK_DAY_OF_WK_SAT], if NO error(s).
 *           CLK_DAY_OF_WK_NONE, otherwise.
 *
 * @note     (1) It's only possible to get a day of week of an epoch supported by the Clock :
 *               - (a) Earliest year is the NTP epoch start year, so the Year ('yr') MUST be greater
 *                     than or equal to 'CLK_NTP_EPOCH_YR_START'.
 *               - (b) Latest year is the Clock epoch end year, so the Year ('yr') MUST be less than
 *                    'CLK_EPOCH_YR_END'.
 *******************************************************************************************************/
CLK_DAY Clk_GetDayOfWk(CLK_YR    yr,
                       CLK_MONTH month,
                       CLK_DAY   day)
{
  CLK_DAY     day_of_wk;
  CPU_BOOLEAN valid;

  //                                                               ------------------ VALIDATE DATE -------------------
  valid = Clk_IsDateValid(yr,
                          month,
                          day,
                          CLK_NTP_EPOCH_YR_START,
                          CLK_EPOCH_YR_END);
  if (valid != DEF_YES) {
    return (CLK_DAY_OF_WK_NONE);
  }

  //                                                               ------------------ GET DAY OF WK -------------------
  day_of_wk = Clk_GetDayOfWkHandler(yr, month, day);

  return (day_of_wk);
}

/****************************************************************************************************//**
 *                                               Clk_GetDayOfYr()
 *
 * @brief    Gets the day of year.
 *
 * @param    yr      Year  value [1900 to 2135] (see Note #1).
 *
 * @param    month   Month value [   1 to   12] / [CLK_MONTH_JAN to CLK_MONTH_DEC].
 *
 * @param    day     Day   value [   1 to   31].
 *
 * @return   Day of year [1 to 366], if NO error(s).
 *           CLK_DAY_OF_WK_NONE, otherwise.
 *
 * @note     (1) It's only possible to get a day of year of an epoch supported by Clock :
 *               - (a) Earliest year is the NTP epoch start year, so the Year ('yr') MUST be greater
 *                     than or equal to 'CLK_NTP_EPOCH_YR_START'.
 *               - (b) Latest year is the Clock epoch end year, so the Year ('yr') MUST be less
 *                     than 'CLK_EPOCH_YR_END'.
 *******************************************************************************************************/
CLK_DAY Clk_GetDayOfYr(CLK_YR    yr,
                       CLK_MONTH month,
                       CLK_DAY   day)
{
  CLK_DAY     day_of_wk;
  CPU_BOOLEAN valid;

  //                                                               ------------------ VALIDATE DATE -------------------
  valid = Clk_IsDateValid(yr,
                          month,
                          day,
                          CLK_NTP_EPOCH_YR_START,
                          CLK_EPOCH_YR_END);
  if (valid != DEF_YES) {
    return (CLK_DAY_OF_WK_NONE);
  }

  //                                                               ------------------ GET DAY OF YR -------------------
  day_of_wk = Clk_GetDayOfYrHandler(yr, month, day);

  return (day_of_wk);
}

/****************************************************************************************************//**
 *                                           Clk_DateTimeToStr()
 *
 * @brief    Converts a date/time structure to an ASCII string.
 *
 * @param    p_date_time     Pointer to the variable that contains the date/time structure to convert.
 *
 * @param    fmt             Desired string format :
 *                               - CLK_STR_FMT_YYYY_MM_DD_HH_MM_SS_UTC     "YYYY-MM-DD HH:MM:SS UTC+TZ"
 *                               - CLK_STR_FMT_YYYY_MM_DD_HH_MM_SS         "YYYY-MM-DD HH:MM:SS"
 *                               - CLK_STR_FMT_MM_DD_YY_HH_MM_SS           "MM-DD-YY HH:MM:SS"
 *                               - CLK_STR_FMT_YYYY_MM_DD                  "YYYY-MM-DD"
 *                               - CLK_STR_FMT_MM_DD_YY                    "MM-DD-YY"
 *                               - CLK_STR_FMT_DAY_MONTH_DD_YYYY           "Day Month DD, YYYY"
 *                               - CLK_STR_FMT_DAY_MONTH_DD_HH_MM_SS_YYYY  "Day Mon DD HH:MM:SS YYYY"
 *                               - CLK_STR_FMT_HH_MM_SS                    "HH:MM:SS"
 *                               - CLK_STR_FMT_HH_MM_SS_AM_PM              "HH:MM:SS AM|PM"
 *
 * @param    p_str           Pointer to the buffer that will receive the formated string (see Note #2).
 *
 * @param    str_len         Maximum number of characters the string can contains.
 *
 * @return   DEF_OK, if the string successfully returned.
 *           DEF_FAIL, otherwise.
 *
 * @note     (1) It is only possible to convert date supported by the Clock :
 *               - (a) Earliest year is the NTP epoch start year, so the Year ('yr') MUST be greater
 *                     than or equal to 'CLK_NTP_EPOCH_YR_START'.
 *               - (b) Latest year is the Clock epoch end year, so the Year ('yr') MUST be less than
 *                     'CLK_EPOCH_YR_END'.
 *
 * @note     (2) The size of the string buffer that will receive the returned string address MUST be
 *               greater than or equal to CLK_STR_FMT_MAX_LEN.
 *
 * @note     (3) Pointers to variables that return values MUST be initialized before all other
 *               validation or function handling in case of any error(s).
 *
 * @note     (4) Absolute value of the time zone offset is stored into 'CLK_TS_SEC' data type to be
 *               compliant with unsigned integer verification/operations.
 *******************************************************************************************************/

#if (CLK_CFG_STR_CONV_EN == DEF_ENABLED)
CPU_BOOLEAN Clk_DateTimeToStr(CLK_DATE_TIME *p_date_time,
                              CLK_STR_FMT   fmt,
                              CPU_CHAR      *p_str,
                              CPU_SIZE_T    str_len)
{
  CPU_CHAR    yr[CLK_STR_DIG_YR_LEN     + 1u];
  CPU_CHAR    month[CLK_STR_DIG_MONTH_LEN  + 1u];
  CPU_CHAR    day[CLK_STR_DIG_DAY_LEN    + 1u];
  CPU_CHAR    hr[CLK_STR_DIG_HR_LEN     + 1u];
  CPU_CHAR    min[CLK_STR_DIG_MIN_LEN    + 1u];
  CPU_CHAR    sec[CLK_STR_DIG_SEC_LEN    + 1u];
  CPU_CHAR    tz[CLK_STR_DIG_TZ_MAX_LEN + 1u];
  CPU_CHAR    am_pm[CLK_STR_AM_PM_LEN      + 1u];
  CPU_CHAR    *p_yr;
  CPU_BOOLEAN valid;
  CLK_HR      half_day_hr;
  CLK_TS_SEC  tz_hr_abs;                                        // See Note #4.
  CLK_TS_SEC  tz_min_abs;                                       // See Note #4.
  CLK_TS_SEC  tz_sec_rem_abs;                                   // See Note #4.

  RTOS_ASSERT_DBG((p_str != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);

  RTOS_ASSERT_DBG((str_len >= sizeof("")), RTOS_ERR_NULL_PTR, DEF_FAIL);

  *p_str = '\0';                                                // Init to NULL str for err (see Note #3).

  //                                                               ----------------- VALIDATE STR LEN -----------------
  switch (fmt) {
    case CLK_STR_FMT_YYYY_MM_DD_HH_MM_SS_UTC:
      RTOS_ASSERT_DBG((str_len >= CLK_STR_FMT_YYYY_MM_DD_HH_MM_SS_UTC_LEN), RTOS_ERR_INVALID_ARG, DEF_FAIL);
      break;

    case CLK_STR_FMT_YYYY_MM_DD_HH_MM_SS:
      RTOS_ASSERT_DBG((str_len >= CLK_STR_FMT_YYYY_MM_DD_HH_MM_SS_LEN), RTOS_ERR_INVALID_ARG, DEF_FAIL);
      break;

    case CLK_STR_FMT_MM_DD_YY_HH_MM_SS:
      RTOS_ASSERT_DBG((str_len >= CLK_STR_FMT_MM_DD_YY_HH_MM_SS_LEN), RTOS_ERR_INVALID_ARG, DEF_FAIL);
      break;

    case CLK_STR_FMT_YYYY_MM_DD:
      RTOS_ASSERT_DBG((str_len >= CLK_STR_FMT_YYYY_MM_DD_LEN), RTOS_ERR_INVALID_ARG, DEF_FAIL);
      break;

    case CLK_STR_FMT_MM_DD_YY:
      RTOS_ASSERT_DBG((str_len >= CLK_STR_FMT_MM_DD_YY_LEN), RTOS_ERR_INVALID_ARG, DEF_FAIL);
      break;

    case CLK_STR_FMT_DAY_MONTH_DD_YYYY:
      RTOS_ASSERT_DBG((str_len >= CLK_STR_FMT_DAY_MONTH_DD_YYYY_MAX_LEN), RTOS_ERR_INVALID_ARG, DEF_FAIL);
      break;

    case CLK_STR_FMT_DAY_MONTH_DD_HH_MM_SS_YYYY:
      RTOS_ASSERT_DBG((str_len >= CLK_STR_FMT_DAY_MONTH_DD_HH_MM_SS_YYYY_LEN), RTOS_ERR_INVALID_ARG, DEF_FAIL);
      break;

    case CLK_STR_FMT_HH_MM_SS:
      RTOS_ASSERT_DBG((str_len >= CLK_STR_FMT_HH_MM_SS_LEN), RTOS_ERR_INVALID_ARG, DEF_FAIL);
      break;

    case CLK_STR_FMT_HH_MM_SS_AM_PM:
      RTOS_ASSERT_DBG((str_len >= CLK_STR_FMT_HH_MM_SS_AM_PM_LEN), RTOS_ERR_INVALID_ARG, DEF_FAIL);
      break;

    case CLK_STR_FMT_HH_MM_AM_PM:
      RTOS_ASSERT_DBG((str_len >= CLK_STR_FMT_HH_MM_AM_PM_LEN), RTOS_ERR_INVALID_ARG, DEF_FAIL);
      break;

    default:
      return (DEF_FAIL);
  }

  //                                                               ---------------- VALIDATE DATE/TIME ----------------
  valid = Clk_IsDateTimeValidHandler(p_date_time,
                                     CLK_NTP_EPOCH_YR_START,
                                     CLK_EPOCH_YR_END);
  if (valid != DEF_YES) {
    return  (DEF_FAIL);
  }

  //                                                               -------------- CREATE DATE/TIME STRS ---------------
  (void)Str_FmtNbr_Int32U(p_date_time->Yr,                      // Create yr str.
                          CLK_STR_DIG_YR_LEN,
                          DEF_NBR_BASE_DEC,
                          (CPU_CHAR)'\0',
                          DEF_NO,
                          DEF_YES,
                          yr);

  (void)Str_FmtNbr_Int32U(p_date_time->Month,                   // Create month (dig) str.
                          CLK_STR_DIG_MONTH_LEN,
                          DEF_NBR_BASE_DEC,
                          (CPU_CHAR)'0',
                          DEF_NO,
                          DEF_YES,
                          month);

  (void)Str_FmtNbr_Int32U(p_date_time->Day,                     // Create day str.
                          CLK_STR_DIG_DAY_LEN,
                          DEF_NBR_BASE_DEC,
                          (CPU_CHAR)'0',
                          DEF_NO,
                          DEF_YES,
                          day);

  (void)Str_FmtNbr_Int32U(p_date_time->Hr,                      // Create hr str.
                          CLK_STR_DIG_HR_LEN,
                          DEF_NBR_BASE_DEC,
                          (CPU_CHAR)'0',
                          DEF_NO,
                          DEF_YES,
                          hr);

  (void)Str_FmtNbr_Int32U(p_date_time->Min,                     // Create min str.
                          CLK_STR_DIG_MIN_LEN,
                          DEF_NBR_BASE_DEC,
                          (CPU_CHAR)'0',
                          DEF_NO,
                          DEF_YES,
                          min);

  (void)Str_FmtNbr_Int32U(p_date_time->Sec,                     // Create sec str.
                          CLK_STR_DIG_SEC_LEN,
                          DEF_NBR_BASE_DEC,
                          (CPU_CHAR)'0',
                          DEF_NO,
                          DEF_YES,
                          sec);

  switch (fmt) {                                                // ---------------- FMT DATE/TIME STR -----------------
    case CLK_STR_FMT_YYYY_MM_DD_HH_MM_SS_UTC:                   // ------ BUILD STR "YYYY-MM-DD HH:MM:SS UTC+TZ" ------
      LOG_VRB(("Date/time to string : YYYY-MM-DD HH:MM:SS UTC+TZ (+|-hh:mm)"));

      (void)Str_Copy_N(p_str, yr, CLK_STR_DIG_YR_LEN + 1u);
      (void)Str_Cat_N(p_str, "-", 1u);

      (void)Str_Cat_N(p_str, month, CLK_STR_DIG_MONTH_LEN);
      (void)Str_Cat_N(p_str, "-", 1u);

      (void)Str_Cat_N(p_str, day, CLK_STR_DIG_DAY_LEN);
      (void)Str_Cat_N(p_str, " ", 1u);

      (void)Str_Cat_N(p_str, hr, CLK_STR_DIG_HR_LEN);
      (void)Str_Cat_N(p_str, ":", 1u);

      (void)Str_Cat_N(p_str, min, CLK_STR_DIG_MIN_LEN);
      (void)Str_Cat_N(p_str, ":", 1u);

      (void)Str_Cat_N(p_str, sec, CLK_STR_DIG_SEC_LEN);
      (void)Str_Cat_N(p_str, " ", 1u);

      (void)Str_Cat_N(p_str, "UTC", 3u);

      if (p_date_time->TZ_sec >= 0) {
        (void)Str_Cat_N(p_str, "+", 1u);
      } else {
        (void)Str_Cat_N(p_str, "-", 1u);
      }

      tz_sec_rem_abs = (CLK_TS_SEC)DEF_ABS(p_date_time->TZ_sec);
      tz_hr_abs = tz_sec_rem_abs / DEF_TIME_NBR_SEC_PER_HR;
      tz_sec_rem_abs = tz_sec_rem_abs % DEF_TIME_NBR_SEC_PER_HR;
      (void)Str_FmtNbr_Int32U(tz_hr_abs,
                              CLK_STR_DIG_TZ_HR_LEN,
                              DEF_NBR_BASE_DEC,
                              (CPU_CHAR)'0',
                              DEF_NO,
                              DEF_YES,
                              tz);

      (void)Str_Cat_N(p_str, tz, CLK_STR_DIG_TZ_HR_LEN);
      (void)Str_Cat_N(p_str, ":", 1u);

      tz_min_abs = tz_sec_rem_abs / DEF_TIME_NBR_SEC_PER_MIN;
      (void)Str_FmtNbr_Int32U(tz_min_abs,
                              CLK_STR_DIG_TZ_MIN_LEN,
                              DEF_NBR_BASE_DEC,
                              (CPU_CHAR)'0',
                              DEF_NO,
                              DEF_YES,
                              tz);

      (void)Str_Cat_N(p_str, tz, CLK_STR_DIG_TZ_MIN_LEN);
      break;

    case CLK_STR_FMT_YYYY_MM_DD_HH_MM_SS:                       // --------- BUILD STR "YYYY-MM-DD HH:MM:SS" ----------
      LOG_VRB(("Date/time to string : YYYY-MM-DD HH:MM:SS"));

      (void)Str_Copy_N(p_str, yr, CLK_STR_DIG_YR_LEN + 1u);
      (void)Str_Cat_N(p_str, "-", 1u);

      (void)Str_Cat_N(p_str, month, CLK_STR_DIG_MONTH_LEN);
      (void)Str_Cat_N(p_str, "-", 1u);

      (void)Str_Cat_N(p_str, day, CLK_STR_DIG_DAY_LEN);
      (void)Str_Cat_N(p_str, " ", 1u);

      (void)Str_Cat_N(p_str, hr, CLK_STR_DIG_HR_LEN);
      (void)Str_Cat_N(p_str, ":", 1u);

      (void)Str_Cat_N(p_str, min, CLK_STR_DIG_MIN_LEN);
      (void)Str_Cat_N(p_str, ":", 1u);

      (void)Str_Cat_N(p_str, sec, CLK_STR_DIG_SEC_LEN);
      break;

    case CLK_STR_FMT_MM_DD_YY_HH_MM_SS:                         // ---------- BUILD STR "MM-DD-YY HH:MM:SS" -----------
      LOG_VRB(("Date/time to string : MM-DD-YY HH:MM:SS"));

      (void)Str_Copy_N(p_str, month, CLK_STR_DIG_MONTH_LEN + 1u);
      (void)Str_Cat_N(p_str, "-", 1u);

      (void)Str_Cat_N(p_str, day, CLK_STR_DIG_DAY_LEN);
      (void)Str_Cat_N(p_str, "-", 1u);

      p_yr = yr + CLK_STR_DIG_YR_TRUNC_LEN;
      (void)Str_Cat_N(p_str, p_yr, CLK_STR_DIG_YR_TRUNC_LEN);
      (void)Str_Cat_N(p_str, " ", 1u);

      (void)Str_Cat_N(p_str, hr, CLK_STR_DIG_HR_LEN);
      (void)Str_Cat_N(p_str, ":", 1u);

      (void)Str_Cat_N(p_str, min, CLK_STR_DIG_MIN_LEN);
      (void)Str_Cat_N(p_str, ":", 1u);

      (void)Str_Cat_N(p_str, sec, CLK_STR_DIG_SEC_LEN);
      break;

    case CLK_STR_FMT_YYYY_MM_DD:                                // -------------- BUILD STR "YYYY-MM-DD" --------------
      LOG_VRB(("Date/time to string : YYYY-MM-DD"));

      (void)Str_Copy_N(p_str, yr, CLK_STR_DIG_YR_LEN + 1u);
      (void)Str_Cat_N(p_str, "-", 1u);

      (void)Str_Cat_N(p_str, month, CLK_STR_DIG_MONTH_LEN);
      (void)Str_Cat_N(p_str, "-", 1u);

      (void)Str_Cat_N(p_str, day, CLK_STR_DIG_DAY_LEN);
      break;

    case CLK_STR_FMT_MM_DD_YY:                                  // --------------- BUILD STR ""MM-DD-YY" --------------
      LOG_VRB(("Date/time to string : MM-DD-YY"));

      (void)Str_Copy_N(p_str, month, CLK_STR_DIG_MONTH_LEN + 1u);
      (void)Str_Cat_N(p_str, "-", 1u);

      (void)Str_Cat_N(p_str, day, CLK_STR_DIG_DAY_LEN);
      (void)Str_Cat_N(p_str, "-", 1u);

      p_yr = yr + CLK_STR_DIG_YR_TRUNC_LEN;
      (void)Str_Cat_N(p_str, p_yr, CLK_STR_DIG_YR_TRUNC_LEN);
      break;

    case CLK_STR_FMT_DAY_MONTH_DD_YYYY:                         // ---------- BUILD STR "Day Month DD, YYYY" ----------
      LOG_VRB(("Date/time to string : Day Month DD, YYYY"));

      (void)Str_Copy_N(p_str,
                       Clk_StrDayOfWk[p_date_time->DayOfWk - CLK_FIRST_DAY_OF_WK],
                       CLK_STR_DAY_OF_WK_MAX_LEN + 1u);
      (void)Str_Cat_N(p_str, " ", 1u);

      (void)Str_Cat_N(p_str,
                      Clk_StrMonth[p_date_time->Month - CLK_FIRST_DAY_OF_MONTH],
                      CLK_STR_MONTH_MAX_LEN);
      (void)Str_Cat_N(p_str, " ", 1u);

      (void)Str_Cat_N(p_str, day, CLK_STR_DIG_DAY_LEN);
      (void)Str_Cat_N(p_str, ", ", 2u);

      (void)Str_Cat_N(p_str, yr, CLK_STR_DIG_YR_LEN);
      break;

    case CLK_STR_FMT_DAY_MONTH_DD_HH_MM_SS_YYYY:                // ------- BUILD STR "Day Mon DD HH:MM:SS YYYY" -------
      LOG_VRB(("Date/time to string : Day Mon DD HH:MM:SS YYYY"));

      (void)Str_Copy_N(p_str,
                       Clk_StrDayOfWk[p_date_time->DayOfWk - CLK_FIRST_DAY_OF_WK],
                       CLK_STR_DAY_OF_WK_TRUNC_LEN + 1u);
      p_str[3] = '\0';
      (void)Str_Cat_N(p_str, " ", 1u);

      (void)Str_Cat_N(p_str,
                      Clk_StrMonth[p_date_time->Month - CLK_FIRST_DAY_OF_MONTH],
                      CLK_STR_MONTH_TRUNC_LEN);
      (void)Str_Cat_N(p_str, " ", 1u);

      (void)Str_Cat_N(p_str, day, CLK_STR_DIG_DAY_LEN);
      (void)Str_Cat_N(p_str, " ", 1u);

      (void)Str_Cat_N(p_str, hr, CLK_STR_DIG_HR_LEN);
      (void)Str_Cat_N(p_str, ":", 1u);

      (void)Str_Cat_N(p_str, min, CLK_STR_DIG_MIN_LEN);
      (void)Str_Cat_N(p_str, ":", 1u);

      (void)Str_Cat_N(p_str, sec, CLK_STR_DIG_SEC_LEN);
      (void)Str_Cat_N(p_str, " ", 1u);

      (void)Str_Cat_N(p_str, yr, CLK_STR_DIG_YR_LEN);
      break;

    case CLK_STR_FMT_HH_MM_SS:                                  // --------------- BUILD STR "HH:MM:SS" ---------------
      LOG_VRB(("Date/time to string : HH:MM:SS"));

      (void)Str_Copy_N(p_str, hr, CLK_STR_DIG_HR_LEN + 1u);
      (void)Str_Cat_N(p_str, ":", 1u);

      (void)Str_Cat_N(p_str, min, CLK_STR_DIG_MIN_LEN);
      (void)Str_Cat_N(p_str, ":", 1u);

      (void)Str_Cat_N(p_str, sec, CLK_STR_DIG_SEC_LEN);
      break;

    case CLK_STR_FMT_HH_MM_SS_AM_PM:                            // ------------ BUILD STR "HH:MM:SS AM|PM" ------------
      LOG_VRB(("Date/time to string : HH:MM:SS AM|PM"));

      if (p_date_time->Hr < CLK_HR_PER_HALF_DAY) {              // Chk for AM or PM.
        (void)Str_Copy_N(am_pm, "AM", CLK_STR_AM_PM_LEN + 1u);
        if (p_date_time->Hr == 0u) {
          half_day_hr = CLK_HR_PER_HALF_DAY;
        } else {
          half_day_hr = p_date_time->Hr;
        }
      } else {
        (void)Str_Copy_N(am_pm, "PM", CLK_STR_AM_PM_LEN + 1u);
        half_day_hr = p_date_time->Hr - CLK_HR_PER_HALF_DAY;
      }

      (void)Str_FmtNbr_Int32U(half_day_hr,
                              CLK_STR_DIG_HR_LEN,
                              DEF_NBR_BASE_DEC,
                              (CPU_CHAR)'0',
                              DEF_NO,
                              DEF_YES,
                              hr);

      (void)Str_Copy_N(p_str, hr, CLK_STR_DIG_HR_LEN + 1u);
      (void)Str_Cat_N(p_str, ":", 1u);

      (void)Str_Cat_N(p_str, min, CLK_STR_DIG_MIN_LEN);
      (void)Str_Cat_N(p_str, ":", 1u);

      (void)Str_Cat_N(p_str, sec, CLK_STR_DIG_SEC_LEN);
      (void)Str_Cat_N(p_str, " ", 1u);

      (void)Str_Cat_N(p_str, am_pm, CLK_STR_AM_PM_LEN);
      break;

    case CLK_STR_FMT_HH_MM_AM_PM:
      LOG_VRB(("Date/time to string : HH:MM AM|PM"));

      if (p_date_time->Hr < CLK_HR_PER_HALF_DAY) {              // Chk for AM or PM.
        (void)Str_Copy_N(am_pm, "AM", CLK_STR_AM_PM_LEN + 1u);
        if (p_date_time->Hr == 0u) {
          half_day_hr = CLK_HR_PER_HALF_DAY;
        } else {
          half_day_hr = p_date_time->Hr;
        }
      } else {
        (void)Str_Copy_N(am_pm, "PM", CLK_STR_AM_PM_LEN + 1u);
        half_day_hr = p_date_time->Hr - CLK_HR_PER_HALF_DAY;
      }

      (void)Str_FmtNbr_Int32U(half_day_hr,
                              CLK_STR_DIG_HR_LEN,
                              DEF_NBR_BASE_DEC,
                              (CPU_CHAR)'0',
                              DEF_NO,
                              DEF_YES,
                              hr);

      (void)Str_Copy_N(p_str, hr, CLK_STR_DIG_HR_LEN + 1u);
      (void)Str_Cat_N(p_str, ":", 1u);

      (void)Str_Cat_N(p_str, min, CLK_STR_DIG_MIN_LEN);
      (void)Str_Cat_N(p_str, " ", 1u);

      (void)Str_Cat_N(p_str, am_pm, CLK_STR_AM_PM_LEN);
      break;

    default:                                                    // ------------------- INVALID FMT --------------------
      LOG_ERR(("Date/time to string : Invalid format"));
      return (DEF_FAIL);
  }

  return (DEF_OK);
}
#endif

/****************************************************************************************************//**
 *                                               Clk_GetTS_NTP()
 *
 * @brief    Gets the current Clock timestamp as an NTP timestamp.
 *
 * @param    p_ts_ntp_sec    Pointer to the variable that will receive the NTP timestamp :
 *                               - In seconds UTC+00,  if NO error(s);
 *                               - CLK_TS_SEC_NONE,    otherwise.
 *
 * @return   DEF_OK, if the timestamp successfully returned.
 *           DEF_FAIL, otherwise.
 *
 * @note     (1) NTP timestamp is converted from the internal Clock timestamp (should
 *               be set at UTC+00), so the NTP timestamp is returned at UTC+00.
 *               NTP timestamp does NOT include the internal Clock time zone, so any
 *               local time zone offset MUST be applied after calling Clk_GetTS_NTP().
 *
 * @note     (2) NTP timestamp will eventually overflow, so it is not possible to get an NTP
 *               timestamp for years on or after CLK_NTP_EPOCH_YR_END.
 *******************************************************************************************************/

#if (CLK_CFG_NTP_EN == DEF_ENABLED)
CPU_BOOLEAN Clk_GetTS_NTP(CLK_TS_SEC *p_ts_ntp_sec)
{
  CLK_TS_SEC  ts_sec;
  CPU_BOOLEAN valid;

  //                                                               -------------------- GET CLK TS --------------------
  ts_sec = Clk_GetTS();

  //                                                               -------------- CONV CLK TS TO NTP TS ---------------
  valid = Clk_TS_ToTS_NTP(ts_sec, p_ts_ntp_sec);
  if (valid != DEF_OK) {
    return  (DEF_FAIL);
  }

  return (DEF_OK);
}
#endif

/****************************************************************************************************//**
 *                                               Clk_SetTS_NTP()
 *
 * @brief    Sets the Clock timestamp from an NTP timestamp.
 *
 * @param    ts_ntp_sec  Current NTP timestamp to set (in seconds, UTC+00).
 *
 * @return   DEF_OK, if the timestamp successfully set.
 *           DEF_FAIL, otherwise.
 *
 * @note     (1) Internal Clock timestamp should be set for UTC+00 and should NOT include any local
 *               time zone offset.
 *
 * @note     (2) Only years supported by Clock & NTP can be set, so the timestamp date MUST be :
 *               - (a) >= CLK_EPOCH_YR_START
 *               - (b) <  CLK_NTP_EPOCH_YR_END
 *******************************************************************************************************/

#if (CLK_CFG_NTP_EN == DEF_ENABLED)
CPU_BOOLEAN Clk_SetTS_NTP(CLK_TS_SEC ts_ntp_sec)
{
  CLK_TS_SEC  ts_sec;
  CPU_BOOLEAN valid;

  //                                                               -------------- CONV NTP TS TO CLK TS ---------------
  valid = Clk_TS_NTP_ToTS(&ts_sec, ts_ntp_sec);
  if (valid != DEF_OK) {
    return  (DEF_FAIL);
  }

  //                                                               -------------------- SET CLK TS --------------------
  valid = Clk_SetTS(ts_sec);

  return (valid);
}
#endif

/****************************************************************************************************//**
 *                                               Clk_TS_ToTS_NTP()
 *
 * @brief    Converts the Clock timestamp to an NTP timestamp.
 *
 * @param    ts_sec          Timestamp to convert (in seconds, UTC+00).
 *
 * @param    p_ts_ntp_sec    Pointer to the variable that will receive the NTP timestamp :
 *                               - In seconds UTC+00,  if NO error(s);
 *                               - CLK_TS_SEC_NONE, otherwise.
 *
 * @return   DEF_OK, if the timestamp successfully converted.
 *           DEF_FAIL, otherwise.
 *
 * @note     (1) Returned timestamp does NOT include any time zone offset, so any local time
 *               zone offset should be applied before or after calling Clk_TS_ToTS_NTP().
 *
 * @note     (2) Only years supported by Clock & NTP can be converted, so the timestamp date
 *               MUST be :
 *               - (a) >= CLK_EPOCH_YR_START
 *               - (b) <  CLK_NTP_EPOCH_YR_END
 *
 * @note     (3) In case of errors, pointers to variables that return values MUST be initialized
 *               before all other validation or function handling.
 *******************************************************************************************************/

#if (CLK_CFG_NTP_EN == DEF_ENABLED)
CPU_BOOLEAN Clk_TS_ToTS_NTP(CLK_TS_SEC ts_sec,
                            CLK_TS_SEC *p_ts_ntp_sec)
{
  CLK_TS_SEC ts_ntp_sec;

  RTOS_ASSERT_DBG((p_ts_ntp_sec != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);

  *p_ts_ntp_sec = CLK_TS_SEC_NONE;                              // Init to ts none for err (see Note #3).

  //                                                               -------------- CONV CLK TS TO NTP TS ---------------
  ts_ntp_sec = ts_sec + CLK_NTP_EPOCH_OFFSET_SEC;
  if (ts_ntp_sec < CLK_NTP_EPOCH_OFFSET_SEC) {                  // Chk for ovf.
    LOG_ERR(("TS to NTP TS: conversion has failed"));
    return (DEF_FAIL);
  }

  *p_ts_ntp_sec = ts_ntp_sec;

  return (DEF_OK);
}
#endif

/****************************************************************************************************//**
 *                                               Clk_TS_NTP_ToTS()
 *
 * @brief    Converts the NTP timestamp to the Clock timestamp.
 *
 * @param    p_ts_sec    Pointer to the variable that will receive the Clock timestamp :
 *                           - In seconds UTC+00,  if NO error(s);
 *                           - CLK_TS_SEC_NONE, otherwise.
 *
 * @param    ts_ntp_sec  NTP timestamp value to convert (in seconds, UTC+00).
 *
 * @return   DEF_OK, if the timestamp is successfully converted.
 *           DEF_FAIL, otherwise.
 *
 * @note     (1) Returned timestamp does NOT include any time zone offset, so any local time zone
 *               offset should be applied before or after calling Clk_TS_NTP_ToTS().
 *
 * @note     (2) Only years supported by Clock & NTP can be converted, so the timestamp date MUST be :
 *               - (a) >= CLK_EPOCH_YR_START
 *               - (b) <  CLK_NTP_EPOCH_YR_END
 *
 * @note     (3) In case of errors, pointers to variables that return values MUST be initialized
 *               before all other validation or function handling.
 *******************************************************************************************************/

#if (CLK_CFG_NTP_EN == DEF_ENABLED)
CPU_BOOLEAN Clk_TS_NTP_ToTS(CLK_TS_SEC *p_ts_sec,
                            CLK_TS_SEC ts_ntp_sec)
{
  RTOS_ASSERT_DBG((p_ts_sec != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);

  *p_ts_sec = CLK_TS_SEC_NONE;                                  // Init to ts none for err (see Note #3).

  if (ts_ntp_sec < CLK_NTP_EPOCH_OFFSET_SEC) {                  // Chk for ovf.
    LOG_ERR(("NTP TS to TS: overflow."));
    return (DEF_FAIL);
  }

  *p_ts_sec = ts_ntp_sec - CLK_NTP_EPOCH_OFFSET_SEC;            // -------------- CONV NTP TS TO CLK TS ---------------

  return (DEF_OK);
}
#endif

/****************************************************************************************************//**
 *                                           Clk_TS_NTP_ToDateTime()
 *
 * @brief    Converts NTP timestamp to date/time structure.
 *
 * @param    ts_ntp_sec      Timestamp to convert (in seconds,          UTC+00).
 *
 * @param    tz_sec          Time zone offset     (in seconds, +|- from UTC).
 *
 * @param    p_date_time     Pointer to the variable that will receive the date/time structure.
 *
 * @return   DEF_OK, if date/time structure is successfully returned.
 *           DEF_FAIL, otherwise.
 *
 * @note     (1) Timestamp ('ts_ntp_sec') MUST be set for UTC+00 and should NOT include the time
 *               zone offset ('tz_sec') since Clk_TS_NTP_ToDateTime() includes the time zone
 *               offset in its date/time calculation. The time zone offset should NOT be
 *               applied before or after calling Clk_TS_NTP_ToDateTime().
 *               @n
 *               Time zone field of the date/time structure ('p_date_time->TZ_sec') is set to
 *               the value of the time zone argument ('tz_sec').
 *               @n
 *               Time zone is based on Coordinated Universal Time (UTC) and uses the following values :
 *                   - (a) Between  +|- 12 hours (+|- 43200 seconds)
 *                   - (b) Multiples of a 15 minute interval
 *******************************************************************************************************/

#if (CLK_CFG_NTP_EN == DEF_ENABLED)
CPU_BOOLEAN Clk_TS_NTP_ToDateTime(CLK_TS_SEC    ts_ntp_sec,
                                  CLK_TZ_SEC    tz_sec,
                                  CLK_DATE_TIME *p_date_time)
{
  CPU_BOOLEAN valid;

  //                                                               ------------- CONV NTP TS TO DATE/TIME -------------
  valid = Clk_TS_ToDateTimeHandler(ts_ntp_sec,
                                   tz_sec,
                                   p_date_time,
                                   CLK_NTP_EPOCH_YR_START,
                                   CLK_NTP_EPOCH_YR_END);
  if (valid != DEF_OK) {
    return (DEF_FAIL);
  }

  return (DEF_OK);
}
#endif

/****************************************************************************************************//**
 *                                           Clk_DateTimeToTS_NTP()
 *
 * @brief    Converts a date/time structure to an NTP timestamp.
 *
 * @param    p_ts_ntp_sec    Pointer to the variable that will receive the NTP timestamp :
 *                               - In seconds UTC+00, if NO error(s);
 *                               - CLK_TS_SEC_NONE, otherwise.
 *
 * @param    p_date_time     Pointer to the variable that contains the date/time structure to convert.
 *
 * @return   DEF_OK, if the timestamp successfully returned.
 *           DEF_FAIL, otherwise.
 *
 * @note     (1) Date/time structure ('p_date_time') MUST be representable in the NTP timestamp, so
 *               the date to convert MUST be :
 *               - (a) >= CLK_NTP_EPOCH_YR_START
 *               - (b) <  CLK_NTP_EPOCH_YR_END
 *
 * @note     (2) Date/time ('p_date_time') should be set to local time with correct time zone
 *               offset ('p_date_time->TZ_sec'). Clk_DateTimeToTS_NTP() removes the time zone
 *               offset from the date/time to calculate and return a Clock timestamp at UTC+00.
 *               @n
 *               Time zone is based on Coordinated Universal Time (UTC) and uses the following values :
 *               - (a) Between  +|- 12 hours (+|- 43200 seconds)
 *               - (b) Multiples of a 15 minute interval
 *
 * @note     (3) In case of errors, pointers to variables that return values MUST be initialized
 *               before all other validation or function handling.
 *******************************************************************************************************/

#if (CLK_CFG_NTP_EN == DEF_ENABLED)
CPU_BOOLEAN Clk_DateTimeToTS_NTP(CLK_TS_SEC    *p_ts_ntp_sec,
                                 CLK_DATE_TIME *p_date_time)
{
  CPU_BOOLEAN valid;

  RTOS_ASSERT_DBG((p_ts_ntp_sec != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);

  *p_ts_ntp_sec = CLK_TS_SEC_NONE;                              // Init to ts none for err (see Note #3).

  //                                                               ---------------- VALIDATE DATE/TIME ----------------
  valid = Clk_IsNTP_DateTimeValid(p_date_time);
  if (valid != DEF_OK) {
    return  (DEF_FAIL);
  }

  //                                                               ------------- CONV DATE/TIME TO NTP TS -------------
  valid = Clk_DateTimeToTS_Handler(p_ts_ntp_sec,
                                   p_date_time,
                                   CLK_NTP_EPOCH_YR_START);
  if (valid != DEF_OK) {
    return  (DEF_FAIL);
  }

  return (DEF_OK);
}
#endif

/****************************************************************************************************//**
 *                                           Clk_NTP_DateTimeMake()
 *
 * @brief    Builds a valid NTP epoch date/time structure.
 *
 * @param    p_date_time     Pointer to the variable that will receive the date/time structure.
 *
 * @param    yr              Year    value [CLK_NTP_EPOCH_YR_START to CLK_NTP_EPOCH_YR_END) (see Note #1).
 *
 * @param    month           Month   value [         CLK_MONTH_JAN to        CLK_MONTH_DEC].
 *
 * @param    day             Day     value [                     1 to                   31].
 *
 * @param    hr              Hours   value [                     0 to                   23].
 *
 * @param    min             Minutes value [                     0 to                   59].
 *
 * @param    sec             Seconds value [                     0 to                   60] (see Note #3).
 *
 * @param    tz_sec          Time zone offset (in seconds, +|- from UTC) [-43200 to 43200].
 *
 * @return   DEF_OK, if date/time structure is successfully returned.
 *           DEF_FAIL, otherwise.
 *
 * @note     (1) Date/time structure ('p_date_time') MUST be representable in the NTP timestamp,
 *               so the date to convert MUST be :
 *               - (a) >= CLK_NTP_EPOCH_YR_START
 *               - (b) <  CLK_NTP_EPOCH_YR_END
 *
 * @note     (2) Day of week ('p_date_time->DayOfWk') and Day of year ('p_date_time->DayOfYr')
 *               are internally calculated and set in the date/time structure if the date is valid.
 *
 * @note     (3) The Seconds value of 60 is valid to be compatible with the leap second adjustment and
 *               the atomic clock time structure.
 *
 * @note     (4) Time zone is based ('tz_sec') on Coordinated Universal Time (UTC)and uses the following
 *               values :
 *               - (a) Between  +|- 12 hours (+|- 43200 seconds)
 *               - (b) Multiples of a 15 minute interval
 *******************************************************************************************************/

#if (CLK_CFG_NTP_EN == DEF_ENABLED)
CPU_BOOLEAN Clk_NTP_DateTimeMake(CLK_DATE_TIME *p_date_time,
                                 CLK_YR        yr,
                                 CLK_MONTH     month,
                                 CLK_DAY       day,
                                 CLK_HR        hr,
                                 CLK_MIN       min,
                                 CLK_SEC       sec,
                                 CLK_TZ_SEC    tz_sec)
{
  CPU_BOOLEAN valid;

  //                                                               ---------- VALIDATE & CONV NTP DATE/TIME -----------
  valid = Clk_DateTimeMakeHandler(p_date_time,
                                  yr,
                                  month,
                                  day,
                                  hr,
                                  min,
                                  sec,
                                  tz_sec,
                                  CLK_NTP_EPOCH_YR_START,
                                  CLK_NTP_EPOCH_YR_END);
  if (valid != DEF_YES) {
    return  (DEF_FAIL);
  }

  return (DEF_OK);
}
#endif

/****************************************************************************************************//**
 *                                           Clk_IsNTP_DateTimeValid()
 *
 * @brief    Determines if the date/time structure is valid in the NTP epoch.
 *
 * @param    p_date_time     Pointer to the variable that contains the date/time structure to validate.
 *
 * @return   DEF_YES, if the date/time structure is valid.
 *           DEF_NO, otherwise.
 *
 * @note     (1) Date/time structure ('p_date_time') MUST be representable in NTP epoch,so the
 *               date to validate MUST be :
 *               - (a) >= CLK_NTP_EPOCH_YR_START
 *               - (b) <  CLK_NTP_EPOCH_YR_END
 *
 * @note     (2) Time zone is based ('p_date_time->TZ_sec') on Coordinated Universal Time (UTC)
 *               and uses the following values :
 *               - (a) Between  +|- 12 hours (+|- 43200 seconds)
 *               - (b) Multiples of a 15 minute interval
 *******************************************************************************************************/

#if (CLK_CFG_NTP_EN == DEF_ENABLED)
CPU_BOOLEAN Clk_IsNTP_DateTimeValid(CLK_DATE_TIME *p_date_time)
{
  CPU_BOOLEAN valid;

  //                                                               -------------- VALIDATE NTP DATE/TIME --------------
  valid = Clk_IsDateTimeValidHandler(p_date_time,
                                     CLK_NTP_EPOCH_YR_START,
                                     CLK_NTP_EPOCH_YR_END);
  if (valid != DEF_YES) {
    return  (DEF_NO);
  }

  return (DEF_YES);
}
#endif

/****************************************************************************************************//**
 *                                               Clk_GetTS_Unix()
 *
 * @brief    Gets current Clock timestamp as a Unix timestamp.
 *
 * @param    p_ts_unix_sec   Pointer to the variable that will receive the Unix timestamp :
 *                               - In seconds UTC+00,  if NO error(s);
 *                               - CLK_TS_SEC_NONE,    otherwise.
 *
 * @return   DEF_OK, if the timestamp is successfully returned.
 *           DEF_FAIL, otherwise.
 *
 * @note     (1) Unix timestamp is converted from the internal Clock timestamp which should be set
 *               for UTC+00, so the Unix timestamp is returned at UTC+00.
 *               Unix timestamp does NOT include the internal Clock time zone, so any local time
 *               zone offset MUST be applied after calling Clk_GetTS_Unix().
 *
 * @note     (2) Unix timestamp will eventually overflow, so it is not possible to get Unix timestamp
 *               for years on or after CLK_UNIX_EPOCH_YR_END.
 *
 * @note     (3) In case of errors, pointers to variables that return values MUST be initialized
 *               before all other validation or function handling.
 *******************************************************************************************************/

#if (CLK_CFG_UNIX_EN == DEF_ENABLED)
CPU_BOOLEAN Clk_GetTS_Unix(CLK_TS_SEC *p_ts_unix_sec)
{
  CLK_TS_SEC  ts_sec;
  CPU_BOOLEAN valid;

  //                                                               -------------------- GET CLK TS --------------------
  ts_sec = Clk_GetTS();

  //                                                               -------------- CONV CLK TS TO UNIX TS --------------
  valid = Clk_TS_ToTS_Unix(ts_sec, p_ts_unix_sec);
  if (valid != DEF_OK) {
    return  (DEF_FAIL);
  }

  return (DEF_OK);
}
#endif

/****************************************************************************************************//**
 *                                               Clk_SetTS_Unix()
 *
 * @brief    Sets the Clock timestamp from a Unix timestamp.
 *
 * @param    ts_unix_sec     Current Unix timestamp to set (in seconds, UTC+00).
 *
 * @return   DEF_OK, if the timestamp is successfully set.
 *           DEF_FAIL, otherwise.
 *
 * @note     (1) Internal Clock timestamp should be set for UTC+00 and should NOT include any local
 *               time zone offset.
 *
 * @note     (2) Only years supported by Clock & Unix can be set, so the timestamp date MUST be :
 *               - (a) >= CLK_EPOCH_YR_START
 *               - (b) <  CLK_UNIX_EPOCH_YR_END
 *******************************************************************************************************/

#if (CLK_CFG_UNIX_EN == DEF_ENABLED)
CPU_BOOLEAN Clk_SetTS_Unix(CLK_TS_SEC ts_unix_sec)
{
  CLK_TS_SEC  ts_sec;
  CPU_BOOLEAN valid;

  //                                                               -------------- CONV UNIX TS TO CLK TS --------------
  valid = Clk_TS_UnixToTS(&ts_sec, ts_unix_sec);
  if (valid != DEF_OK) {
    return  (DEF_FAIL);
  }

  //                                                               -------------------- SET CLK TS --------------------
  valid = Clk_SetTS(ts_sec);

  return (valid);
}
#endif

/****************************************************************************************************//**
 *                                           Clk_TS_ToTS_Unix()
 *
 * @brief    Converts the Clock timestamp to a Unix timestamp.
 *
 * @param    ts_sec          Timestamp to convert (in seconds, UTC+00).
 *
 * @param    p_ts_unix_sec   Pointer to the variable that will receive the Unix timestamp :
 *                               - In seconds UTC+00, if NO error(s);
 *                               - CLK_TS_SEC_NONE, otherwise.
 *
 * @return   DEF_OK, if the timestamp successfully converted.
 *           DEF_FAIL, otherwise.
 *
 * @note     (1) Returned timestamp does NOT include any time zone offset, so any local time
 *               zone offset should be applied before or after calling Clk_TS_ToTS_Unix().
 *
 * @note     (2) Only years supported by the Clock & Unix can be converted, so the timestamp date must
 *               be :
 *               - (a) >= CLK_EPOCH_YR_START
 *               - (b) <  CLK_UNIX_EPOCH_YR_END
 *
 * @note     (3) In case of errors, pointers to variables that return values MUST be initialized
 *               before all other validation or function handling.
 *******************************************************************************************************/

#if (CLK_CFG_UNIX_EN == DEF_ENABLED)
CPU_BOOLEAN Clk_TS_ToTS_Unix(CLK_TS_SEC ts_sec,
                             CLK_TS_SEC *p_ts_unix_sec)
{
  CLK_TS_SEC ts_unix_sec;

  RTOS_ASSERT_DBG((p_ts_unix_sec != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);

  *p_ts_unix_sec = CLK_TS_SEC_NONE;                             // Init to ts none for err (see Note #3).

  //                                                               -------------- CONV CLK TS TO UNIX TS --------------
  ts_unix_sec = ts_sec + CLK_UNIX_EPOCH_OFFSET_SEC;
  if (ts_unix_sec < CLK_UNIX_EPOCH_OFFSET_SEC) {                // Chk for ovf.
    LOG_ERR(("TS to Unix TS: conversion has failed."));
    return (DEF_FAIL);
  }

  *p_ts_unix_sec = ts_unix_sec;

  return (DEF_OK);
}
#endif

/****************************************************************************************************//**
 *                                               Clk_TS_UnixToTS()
 *
 * @brief    Converts Unix timestamp to Clock timestamp.
 *
 * @param    p_ts_sec        Pointer to the variable that will receive the Clock timestamp :
 *                               - In seconds UTC+00, if NO error(s);
 *                               - CLK_TS_SEC_NONE, otherwise.
 *
 * @param    ts_unix_sec     Unix timestamp value to convert (in seconds, UTC+00).
 *
 * @return   DEF_OK, if the timestamp successfully converted.
 *           DEF_FAIL, otherwise.
 *
 * @note     (1) Returned timestamp does NOT include any time zone offset, so any local time
 *               zone offset should be applied before or after calling Clk_TS_UnixToTS().
 *
 * @note     (2) Only years supported by Clock & Unix can be converted, so the timestamp date must be:
 *               - (a) >= CLK_EPOCH_YR_START
 *               - (b) <  CLK_UNIX_EPOCH_YR_END
 *
 * @note     (3) In case of errors, pointers to variables that return values MUST be initialized
 *               before all other validation or function handling.
 *******************************************************************************************************/

#if (CLK_CFG_UNIX_EN == DEF_ENABLED)
CPU_BOOLEAN Clk_TS_UnixToTS(CLK_TS_SEC *p_ts_sec,
                            CLK_TS_SEC ts_unix_sec)
{
  RTOS_ASSERT_DBG((p_ts_sec != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);

  *p_ts_sec = CLK_TS_SEC_NONE;                                  // Init to ts none for err (see Note #3).

  if (ts_unix_sec < CLK_UNIX_EPOCH_OFFSET_SEC) {                // Chk for ovf.
    LOG_ERR(("Unix TS to TS: overflow."));
    return (DEF_FAIL);
  }

  //                                                               -------------- CONV UNIX TS TO CLK TS --------------
  *p_ts_sec = ts_unix_sec - CLK_UNIX_EPOCH_OFFSET_SEC;

  return (DEF_OK);
}
#endif

/****************************************************************************************************//**
 *                                           Clk_TS_UnixToDateTime()
 *
 * @brief    Converts the Unix timestamp to a date/time structure.
 *
 * @param    ts_unix_sec     Timestamp to convert (in seconds,          UTC+00).
 *
 * @param    tz_sec          Time zone offset     (in seconds, +|- from UTC).
 *
 * @param    p_date_time     Pointer to the variable that will receive the date/time structure.
 *
 * @return   DEF_OK, if the date/time structure successfully returned.
 *           DEF_FAIL, otherwise.
 *
 * @note     (1) Timestamp ('ts_unix_sec') MUST be set for UTC+00 & should NOT include the time
 *               zone offset ('tz_sec') since Clk_TS_UnixToDateTime() includes the time zone
 *               offset in its date/time calculation. The time zone offset should NOT be
 *               applied before or after calling Clk_TS_UnixToDateTime().
 *               @n
 *               Time zone field of the date/time structure ('p_date_time->TZ_sec') is set to
 *               the value of the time zone argument ('tz_sec').
 *               @n
 *               Time zone is based on Coordinated Universal Time (UTC) and uses the following values :
 *               - (a) Between  +|- 12 hours (+|- 43200 seconds)
 *               - (b) Multiples of a 15 minute interval
 *******************************************************************************************************/

#if (CLK_CFG_UNIX_EN == DEF_ENABLED)
CPU_BOOLEAN Clk_TS_UnixToDateTime(CLK_TS_SEC    ts_unix_sec,
                                  CLK_TZ_SEC    tz_sec,
                                  CLK_DATE_TIME *p_date_time)
{
  CPU_BOOLEAN valid;

  //                                                               ------------ CONV UNIX TS TO DATE/TIME -------------
  valid = Clk_TS_ToDateTimeHandler(ts_unix_sec,
                                   tz_sec,
                                   p_date_time,
                                   CLK_UNIX_EPOCH_YR_START,
                                   CLK_UNIX_EPOCH_YR_END);
  if (valid != DEF_OK) {
    return  (DEF_FAIL);
  }

  return (DEF_OK);
}
#endif

/****************************************************************************************************//**
 *                                           Clk_DateTimeToTS_Unix()
 *
 * @brief    Converts a date/time structure to a Unix timestamp.
 *
 * @param    p_ts_sec        Pointer to the variable that will receive the Unix timestamp :
 *                               - In seconds UTC+00,  if NO error(s);
 *                               - CLK_TS_SEC_NONE,    otherwise.
 *
 * @param    p_date_time     Pointer to the variable that contains date/time structure to convert.
 *
 * @return   DEF_OK, if the timestamp successfully returned.
 *           DEF_FAIL, otherwise.
 *
 * @note     (1) Date/time structure ('p_date_time') MUST be representable in Unix timestamp.
 *               Thus date to convert MUST be :
 *               - (a) >= CLK_UNIX_EPOCH_YR_START
 *               - (b) <  CLK_UNIX_EPOCH_YR_END
 *
 * @note     (2) Date/time ('p_date_time') should be set to local time with correct  time zone
 *               offset ('p_date_time->TZ_sec'). Clk_DateTimeToTS_Unix() removes the time zone
 *               offset from the date/time to calculate & return a Clock timestamp at UTC+00.
 *               @n
 *               Time zone is based on Coordinated Universal Time (UTC) and uses the following values :
 *               - (a) Between  +|- 12 hours (+|- 43200 seconds)
 *               - (b) Multiples of a 15 minute interval
 *
 * @note     (3) In case of errors, pointers to variables that return values MUST be initialized
 *               before all other validation or function handling.
 *******************************************************************************************************/

#if (CLK_CFG_UNIX_EN == DEF_ENABLED)
CPU_BOOLEAN Clk_DateTimeToTS_Unix(CLK_TS_SEC    *p_ts_unix_sec,
                                  CLK_DATE_TIME *p_date_time)
{
  CPU_BOOLEAN valid;

  RTOS_ASSERT_DBG((p_ts_unix_sec != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);

  *p_ts_unix_sec = CLK_TS_SEC_NONE;                             // Init to ts none for err (see Note #3).

  //                                                               ---------------- VALIDATE DATE/TIME ----------------
  valid = Clk_IsUnixDateTimeValid(p_date_time);
  if (valid != DEF_OK) {
    return  (DEF_FAIL);
  }

  //                                                               ------------ CONV DATE/TIME TO UNIX TS -------------
  valid = Clk_DateTimeToTS_Handler(p_ts_unix_sec,
                                   p_date_time,
                                   CLK_UNIX_EPOCH_YR_START);
  if (valid != DEF_OK) {
    return  (DEF_FAIL);
  }

  return (DEF_OK);
}
#endif

/****************************************************************************************************//**
 *                                           Clk_UnixDateTimeMake()
 *
 * @brief    Builds a valid Unix epoch date/time structure.
 *
 * @param    p_date_time     Pointer to the variable that will receive the date/time structure.
 *
 * @param    yr              Year    value [CLK_UNIX_EPOCH_YR_START to CLK_UNIX_EPOCH_YR_END) (see Note #1).
 *
 * @param    month           Month   value [          CLK_MONTH_JAN to         CLK_MONTH_DEC].
 *
 * @param    day             Day     value [                      1 to                    31].
 *
 * @param    hr              Hours   value [                      0 to                    23].
 *
 * @param    min             Minutes value [                      0 to                    59].
 *
 * @param    sec             Seconds value [                      0 to                    60] (see Note #3).
 *
 * @param    tz_sec          Time zone offset (in seconds, +|- from UTC) [-43200 to 43200].
 *
 * @return   DEF_OK, if the date/time structure successfully returned.
 *           DEF_FAIL, otherwise.
 *
 * @note     (1) Date/time structure ('p_date_time') MUST be representable in Unix timestamp.
 *               Thus date to convert MUST be :
 *               - (a) >= CLK_UNIX_EPOCH_YR_START
 *               - (b) <  CLK_UNIX_EPOCH_YR_END
 *
 * @note     (2) Day of week ('p_date_time->DayOfWk') and Day of year ('p_date_time->DayOfYr')
 *               are internally calculated and set in the date/time structure if date is valid.
 *
 * @note     (3) Seconds value of 60 is valid to be compatible with leap second adjustment and
 *               the atomic clock time structure.
 *
 * @note     (4) Time zone is based ('tz_sec') on Coordinated Universal Time (UTC) and has valid
 *               values :
 *               - (a) Between  +|- 12 hours (+|- 43200 seconds)
 *               - (b) Multiples of a 15 minute interval
 *******************************************************************************************************/

#if (CLK_CFG_UNIX_EN == DEF_ENABLED)
CPU_BOOLEAN Clk_UnixDateTimeMake(CLK_DATE_TIME *p_date_time,
                                 CLK_YR        yr,
                                 CLK_MONTH     month,
                                 CLK_DAY       day,
                                 CLK_HR        hr,
                                 CLK_MIN       min,
                                 CLK_SEC       sec,
                                 CLK_TZ_SEC    tz_sec)
{
  CPU_BOOLEAN valid;

  //                                                               ---------- VALIDATE & CONV UNIX DATE/TIME ----------
  valid = Clk_DateTimeMakeHandler(p_date_time,
                                  yr,
                                  month,
                                  day,
                                  hr,
                                  min,
                                  sec,
                                  tz_sec,
                                  CLK_UNIX_EPOCH_YR_START,
                                  CLK_UNIX_EPOCH_YR_END);
  if (valid != DEF_YES) {
    return  (DEF_FAIL);
  }

  return (DEF_OK);
}
#endif

/****************************************************************************************************//**
 *                                           Clk_IsUnixDateTimeValid()
 *
 * @brief    Determine if date/time structure is valid in Unix epoch.
 *
 * @param    p_date_time     Pointer to the variable that contains the date/time structure to validate.
 *
 * @return   DEF_YES, if date/time structure is valid.
 *           DEF_NO, otherwise.
 *
 * @note     (1) Date/time structure ('p_date_time') MUST be representable in Unix epoch, so the
 *               date to validate MUST be :
 *               - (a) >= CLK_UNIX_EPOCH_YR_START
 *               - (b) <  CLK_UNIX_EPOCH_YR_END
 *
 * @note     (2) Time zone is based ('p_date_time->TZ_sec') on Coordinated Universal Time (UTC)
 *               and uses the following values :
 *               - (a) Between  +|- 12 hours (+|- 43200 seconds)
 *               - (b) Multiples of a 15 minute interval
 *******************************************************************************************************/

#if (CLK_CFG_UNIX_EN == DEF_ENABLED)
CPU_BOOLEAN Clk_IsUnixDateTimeValid(CLK_DATE_TIME *p_date_time)
{
  CPU_BOOLEAN valid;

  //                                                               ------------- VALIDATE UNIX DATE/TIME --------------
  valid = Clk_IsDateTimeValidHandler(p_date_time,
                                     CLK_UNIX_EPOCH_YR_START,
                                     CLK_UNIX_EPOCH_YR_END);
  if (valid != DEF_YES) {
    return  (DEF_FAIL);
  }

  return (DEF_YES);
}
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                           LOCAL FUNCTIONS
 ********************************************************************************************************
 *******************************************************************************************************/

/****************************************************************************************************//**
 *                                               Clk_IsLeapYr()
 *
 * @brief    Determines if year is a leap year.
 *
 * @param    yr  Year value [1900 to  2135].
 *
 * @return   DEF_YES, if 'yr' is     a leap year.
 *           DEF_NO,  if 'yr' is NOT a leap year.
 *
 * @note     (1) Most years that are evenly divisible by 4 are leap years; ...
 *               - (a) except years that are evenly divisible by 100,        ...
 *               - (b) unless they  are also evenly divisible by 400.
 *******************************************************************************************************/
static CPU_BOOLEAN Clk_IsLeapYr(CLK_YR yr)
{
  CPU_BOOLEAN leap_yr;

  leap_yr = ( ((yr %   4u) == 0u)                               // Chk for leap yr (see Note #1).
              && (((yr % 100u) != 0u) || ((yr % 400u) == 0u))) ? DEF_YES : DEF_NO;

  return (leap_yr);
}

/****************************************************************************************************//**
 *                                               Clk_IsDateValid()
 *
 * @brief    Determines if the date values are valid.
 *
 * @param    yr          Year  value [yr_start to  yr_end].
 *
 * @param    month       Month value [       1 to      12] / [CLK_MONTH_JAN to CLK_MONTH_DEC].
 *
 * @param    day         Day   value [       1 to      31].
 *
 * @param    yr_start    Start year of the epoch.
 *
 * @param    yr_end      End year of the epoch.
 *
 * @return   DEF_YES, if date is valid.
 *           DEF_NO, otherwise.
 *******************************************************************************************************/
static CPU_BOOLEAN Clk_IsDateValid(CLK_YR    yr,
                                   CLK_MONTH month,
                                   CLK_DAY   day,
                                   CLK_YR    yr_start,
                                   CLK_YR    yr_end)
{
  CPU_BOOLEAN leap_yr;
  CPU_INT08U  leap_yr_ix;
  CLK_DAY     days_in_month;

  //                                                               ------------------- VALIDATE YR --------------------
  if ((yr < yr_start)
      || (yr >= yr_end)) {
    LOG_ERR(("Invalid year, must be > ", (u)yr_end, " & < ", (u)yr_start));
    return (DEF_NO);
  }

  //                                                               ------------------ VALIDATE MONTH ------------------
  if ((month < CLK_FIRST_MONTH_OF_YR)
      || (month > CLK_MONTH_PER_YR)) {
    LOG_ERR(("Invalid year, must be >= ", (u)CLK_FIRST_MONTH_OF_YR, " & < ", (u)CLK_MONTH_PER_YR));
    return (DEF_NO);
  }

  //                                                               ------------------- VALIDATE DAY -------------------
  leap_yr = Clk_IsLeapYr(yr);
  leap_yr_ix = (leap_yr == DEF_YES) ? 1u : 0u;
  days_in_month = Clk_DaysInMonth[leap_yr_ix][month - CLK_FIRST_MONTH_OF_YR];
  if ((day < CLK_FIRST_DAY_OF_MONTH)
      || (day > days_in_month)) {
    LOG_ERR(("Invalid day, must be > ", (u)CLK_FIRST_DAY_OF_MONTH, " & < ", (u)days_in_month));
    return (DEF_NO);
  }

  return (DEF_YES);
}

/****************************************************************************************************//**
 *                                           Clk_IsDayOfYrValid()
 *
 * @brief    Determines if the day of year is valid.
 *
 * @param    yr          Year        value [1900 to 2135].
 *
 * @param    day_of_yr   Day of year value [1    to   31].
 *
 * @return   DEF_YES, if day of year is valid.
 *           DEF_NO, otherwise.
 *******************************************************************************************************/
static CPU_BOOLEAN Clk_IsDayOfYrValid(CLK_YR  yr,
                                      CLK_DAY day_of_yr)
{
  CPU_BOOLEAN leap_yr;
  CPU_INT08U  leap_yr_ix;
  CLK_DAY     yr_days_max;

  //                                                               ---------------- VALIDATE DAY OF YR ----------------
  leap_yr = Clk_IsLeapYr(yr);
  leap_yr_ix = (leap_yr == DEF_YES) ? 1u : 0u;
  yr_days_max = Clk_DaysInYr[leap_yr_ix];
  if ((day_of_yr < CLK_FIRST_DAY_OF_YR)
      || (day_of_yr > yr_days_max)) {
    LOG_ERR(("Invalid day of year, must be >= ", (u)CLK_FIRST_DAY_OF_YR, " & < ", (u)yr_days_max));
    return (DEF_NO);
  }

  return (DEF_YES);
}

/****************************************************************************************************//**
 *                                           Clk_IsDayOfWkValid()
 *
 * @brief    Determines if the day of week is valid.
 *
 * @param    day_of_wk   Day of week value [1 to 7] / [CLK_DAY_OF_WK_SUN to CLK_DAY_OF_WK_SAT].
 *
 * @return   DEF_YES, if day of week is valid.
 *           DEF_NO, otherwise.
 *******************************************************************************************************/
static CPU_BOOLEAN Clk_IsDayOfWkValid(CLK_DAY day_of_wk)
{
  //                                                               ---------------- VALIDATE DAY OF WK ----------------
  if ((day_of_wk < CLK_FIRST_DAY_OF_WK)
      || (day_of_wk > DEF_TIME_NBR_DAY_PER_WK)) {
    LOG_ERR(("Invalid day of week, must be >= ", (u)CLK_FIRST_DAY_OF_WK, " & < ", (u)DEF_TIME_NBR_DAY_PER_WK));
    return (DEF_NO);
  }

  return (DEF_YES);
}

/****************************************************************************************************//**
 *                                               Clk_IsTimeValid()
 *
 * @brief    Determines if time values are valid.
 *
 * @param    hr      Hours   value [0 to 23].
 *
 * @param    min     Minutes value [0 to 59].
 *
 * @param    sec     Seconds value [0 to 60] (see Note #1).
 *
 * @return   DEF_YES, if time is valid.
 *           DEF_NO, otherwise.
 *
 * @note     (1) The Seconds value of 60 is valid to be compatible with leap second adjustment and
 *               the atomic clock time structure.
 *******************************************************************************************************/
static CPU_BOOLEAN Clk_IsTimeValid(CLK_HR    hr,
                                   CLK_MONTH min,
                                   CLK_DAY   sec)
{
  //                                                               ------------------ VALIDATE HOUR -------------------
  if (hr >= DEF_TIME_NBR_HR_PER_DAY) {
    LOG_ERR(("Invalid hour, must be < ", (u)DEF_TIME_NBR_HR_PER_DAY));
    return (DEF_NO);
  }

  //                                                               ------------------- VALIDATE MIN -------------------
  if (min >= DEF_TIME_NBR_MIN_PER_HR) {
    LOG_ERR(("Invalid minute, must be < ", (u)DEF_TIME_NBR_MIN_PER_HR));
    return (DEF_NO);
  }

  //                                                               ------------------- VALIDATE SEC -------------------
  if (sec > DEF_TIME_NBR_SEC_PER_MIN) {
    LOG_ERR(("Invalid second, must be =< ", (u)DEF_TIME_NBR_SEC_PER_MIN));
    return (DEF_NO);
  }

  return (DEF_YES);
}

/****************************************************************************************************//**
 *                                               Clk_IsTZValid()
 *
 * @brief    Determines if the time zone is valid.
 *
 * @param    tz_sec  Time zone offset (in seconds, +|- from UTC) [-43200 to 43200].
 *
 * @return   DEF_YES, if time zone is valid.
 *           DEF_NO, otherwise.
 *
 * @note     (1) Time zone is based ('tz_sec') on Coordinated Universal Time (UTC) and uses the following values :
 *               - (a) Between  +|- 12 hours (+|- 43200 seconds)
 *               - (b) Multiples of a 15 minute interval
 *
 * @note     (2) Absolute value of the time zone offset is stored into 'CLK_TS_SEC' data type to be
 *               compliant with unsigned integer verification/operations.
 *******************************************************************************************************/
static CPU_BOOLEAN Clk_IsTZValid(CLK_TZ_SEC tz_sec)
{
  CLK_TS_SEC tz_sec_abs;                                        // See Note #2.

  //                                                               -------------- VALIDATE TZ PRECISION ---------------
  tz_sec_abs = (CLK_TS_SEC)DEF_ABS(tz_sec);
  if ((tz_sec_abs % CLK_TZ_SEC_PRECISION) != 0u) {              // See Note #1b.
    LOG_ERR(("Invalid time zone, must be multiple of ", (u)CLK_TZ_SEC_PRECISION, " seconds."));
    return (DEF_NO);
  }

  //                                                               --------------- VALIDATE TZ MIN-MAX ----------------
  if (tz_sec_abs > CLK_TZ_SEC_MAX) {                            // See Note #1a.
    LOG_ERR(("Invalid time zone, must be > ", (d)CLK_TZ_SEC_MIN, , " & < ", (u)CLK_TZ_SEC_MAX));
    return (DEF_NO);
  }

  return (DEF_YES);
}

/****************************************************************************************************//**
 *                                       Clk_IsDateTimeValidHandler()
 *
 * @brief    Determines if date/time structure is valid.
 *
 * @param    p_date_time     Pointer to the variable that contains the date/time structure to validate.
 *
 * @param    yr_start        Start year of the epoch.
 *
 * @param    yr_end          End year of the epoch.
 *
 * @return   DEF_YES, date/time structure is valid.
 *           DEF_NO, otherwise.
 *******************************************************************************************************/
static CPU_BOOLEAN Clk_IsDateTimeValidHandler(CLK_DATE_TIME *p_date_time,
                                              CLK_YR        yr_start,
                                              CLK_YR        yr_end)
{
  CPU_BOOLEAN valid;

  RTOS_ASSERT_DBG((p_date_time != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_NO);

  //                                                               ------------------ VALIDATE DATE -------------------
  valid = Clk_IsDateValid(p_date_time->Yr, p_date_time->Month, p_date_time->Day, yr_start, yr_end);
  if (valid != DEF_YES) {
    return  (DEF_NO);
  }

  //                                                               ------------------ VALIDATE TIME -------------------
  valid = Clk_IsTimeValid(p_date_time->Hr, p_date_time->Min, p_date_time->Sec);
  if (valid != DEF_YES) {
    return  (DEF_NO);
  }

  //                                                               ---------------- VALIDATE DAY OF WK ----------------
  valid = Clk_IsDayOfWkValid(p_date_time->DayOfWk);
  if (valid != DEF_YES) {
    return  (DEF_NO);
  }

  //                                                               ---------------- VALIDATE DAY OF YR ----------------
  valid = Clk_IsDayOfYrValid(p_date_time->Yr, p_date_time->DayOfYr);
  if (valid != DEF_YES) {
    return  (DEF_NO);
  }

  //                                                               ------------------- VALIDATE TZ --------------------
  valid = Clk_IsTZValid(p_date_time->TZ_sec);
  if (valid != DEF_YES) {
    return  (DEF_NO);
  }

  return (DEF_YES);
}

/****************************************************************************************************//**
 *                                           Clk_GetDayOfYrHandler()
 *
 * @brief    Gets the day of year.
 *
 * @param    yr      Year value [1900 to 2135].
 *
 * @param    --     Argument checked in Clk_TS_ToDateTimeHandler(),
 *                                       Clk_DateTimeMakeHandler().
 *
 * @param    month   Month value [1 to 12] / [CLK_MONTH_JAN to CLK_MONTH_DEC].
 *
 * @param    -----  Argument checked in Clk_TS_ToDateTimeHandler(),
 *                                       Clk_DateTimeMakeHandler().
 *
 * @param    day     Day value [1 to 31].
 *
 * @param    ---    --   Argument checked in Clk_TS_ToDateTimeHandler(),
 *                                           Clk_DateTimeMakeHandler().
 *
 * @return   Day of year [1 to 366].
 *******************************************************************************************************/
static CLK_DAY Clk_GetDayOfYrHandler(CLK_YR    yr,
                                     CLK_MONTH month,
                                     CLK_DAY   day)
{
  CPU_BOOLEAN leap_yr;
  CPU_INT08U  leap_yr_ix;
  CLK_MONTH   month_ix;
  CLK_DAY     day_of_yr;
  CLK_DAY     days_month;
  CLK_DAY     days_in_month;
  CPU_SR_ALLOC();

  LOG_VRB(("Day of year of ", (u)day, ", ", (u)month, ", ", (u)yr, " = "));

  day_of_yr = day - CLK_FIRST_DAY_OF_MONTH;
  leap_yr = Clk_IsLeapYr(yr);
  leap_yr_ix = (leap_yr == DEF_YES) ? 1u : 0u;

  CPU_CRITICAL_ENTER();
  if ((Clk_CacheYr != yr)
      || (Clk_CacheMonth != month)) {                           // See if cached val could be used.
    CPU_CRITICAL_EXIT();
    days_month = 0u;
    for (month_ix = CLK_FIRST_MONTH_OF_YR; month_ix < month; month_ix++) {
      days_in_month = Clk_DaysInMonth[leap_yr_ix][month_ix - CLK_FIRST_MONTH_OF_YR];
      days_month += days_in_month;
    }
  } else {
    days_month = Clk_CacheMonthDays;
    CPU_CRITICAL_EXIT();
  }

  day_of_yr += days_month;
  day_of_yr += CLK_FIRST_DAY_OF_YR;
  LOG_VRB(("Day of year = ", (u)day_of_yr));

  return (day_of_yr);
}

/****************************************************************************************************//**
 *                                           Clk_GetDayOfWkHandler()
 *
 * @brief    Gets the day of week.
 *
 * @param    yr      Year value [1900 to 2135].
 *
 * @param    --     Argument checked in Clk_GetDayOfWk(),
 *                                       Clk_TS_ToDateTimeHandler(),
 *                                       Clk_DateTimeMakeHandler().
 *
 * @param    month   Month value [1 to 12] / [CLK_MONTH_JAN to CLK_MONTH_DEC].
 *
 * @param    -----  Argument checked in Clk_GetDayOfWk(),
 *                                       Clk_TS_ToDateTimeHandler(),
 *                                       Clk_DateTimeMakeHandler().
 *
 * @param    day     Day value [1 to 31].
 *
 * @param    ---    --   Argument checked in Clk_GetDayOfWk(),
 *                                           Clk_TS_ToDateTimeHandler(),
 *                                           Clk_DateTimeMakeHandler().
 *
 * @return   Day of week [1 to 7] / [CLK_DAY_OF_WK_SUN to CLK_DAY_OF_WK_SAT].
 *
 * @note     (1) Finds the day of week at the earliest supported year (NTP) and its first day of week :
 *           - (a) CLK_NTP_EPOCH_YR_START
 *           - (b) CLK_NTP_EPOCH_DAY_OF_WK
 *******************************************************************************************************/
static CLK_DAY Clk_GetDayOfWkHandler(CLK_YR    yr,
                                     CLK_MONTH month,
                                     CLK_DAY   day)
{
  CPU_BOOLEAN  leap_yr;
  CPU_INT08U   leap_yr_ix;
  CLK_YR       yr_ix;
  CLK_MONTH    month_ix;
  CLK_DAY      day_of_wk;
  CLK_DAY      days_in_month;
  CLK_DAY      days_month;
  CLK_NBR_DAYS days_yr;
  CLK_NBR_DAYS days;
  CPU_SR_ALLOC();

  LOG_VRB(("Day of week of ", (u)day, ", ", (u)month, ", ", (u)yr));

  days = day - CLK_FIRST_DAY_OF_MONTH;
  leap_yr = Clk_IsLeapYr(yr);
  leap_yr_ix = (leap_yr == DEF_YES) ? 1u : 0u;

  CPU_CRITICAL_ENTER();
  if ((Clk_CacheYr != yr)
      || (Clk_CacheMonth != month)) {                           // See if cached val could be used.
    CPU_CRITICAL_EXIT();

    days_month = 0u;
    for (month_ix = CLK_FIRST_MONTH_OF_YR; month_ix < month; month_ix++) {
      days_in_month = Clk_DaysInMonth[leap_yr_ix][month_ix - CLK_FIRST_MONTH_OF_YR];
      days_month += days_in_month;
    }

    CPU_CRITICAL_ENTER();
    Clk_CacheMonth = month;
    Clk_CacheMonthDays = days_month;
  } else {
    days_month = Clk_CacheMonthDays;
  }

  if (Clk_CacheYr != yr) {
    CPU_CRITICAL_EXIT();

    days_yr = 0u;
    for (yr_ix = CLK_NTP_EPOCH_YR_START; yr_ix < yr; yr_ix++) {
      leap_yr = Clk_IsLeapYr(yr_ix);
      leap_yr_ix = (leap_yr == DEF_YES) ? 1u : 0u;
      days_yr += Clk_DaysInYr[leap_yr_ix];
    }

    CPU_CRITICAL_ENTER();
    Clk_CacheYr = yr;
    Clk_CacheYrDays = days_yr;
    CPU_CRITICAL_EXIT();
  } else {
    days_yr = Clk_CacheYrDays;
    CPU_CRITICAL_EXIT();
  }

  days += days_month;
  days += days_yr;
  days += CLK_NTP_EPOCH_DAY_OF_WK;                              // See Note #1b.
  days -= CLK_FIRST_DAY_OF_WK;
  day_of_wk = days % DEF_TIME_NBR_DAY_PER_WK;
  day_of_wk += CLK_FIRST_DAY_OF_WK;
  LOG_VRB(("Day of week = ", (u)day_of_wk));

  return (day_of_wk);
}

/****************************************************************************************************//**
 *                                           Clk_SetTZ_Handler()
 *
 * @brief    Sets the Clock time zone offset.
 *
 * @param    tz_sec  Time zone offset (in seconds, +|- from UTC).
 *
 * @param    ------  Argument checked by Clk_SetTZ(),
 *                   Clk_SetDateTime().
 *
 * @note     (1) 'Clk_TZ_sec' MUST ALWAYS be accessed exclusively in critical sections.
 *******************************************************************************************************/
static void Clk_SetTZ_Handler(CLK_TZ_SEC tz_sec)
{
  CPU_SR_ALLOC();

  CPU_CRITICAL_ENTER();
  Clk_Ptr->Clk_TZ_sec = tz_sec;
  CPU_CRITICAL_EXIT();
}

/****************************************************************************************************//**
 *                                       Clk_TS_ToDateTimeHandler()
 *
 * @brief    Converts any type of timestamp to date/time structure.
 *
 * @param    ts_sec          Timestamp to convert (in seconds,          UTC+00).
 *
 * @param    tz_sec          Time zone offset     (in seconds, +|- from UTC).
 *
 * @param    p_date_time     Pointer to the variable that contains the date/time structure to validate.
 *
 * @param    yr_start        Start year of the epoch.
 *
 * @param    yr_end          End year of the epoch.
 *
 * @return   DEF_OK, if date/time structure is valid.
 *           DEF_FAIL, otherwise.
 *
 * @note     (1) Timestamp ('ts_sec') MUST be set for UTC+00 & should NOT include the time
 *               zone offset ('tz_sec') since Clk_TS_ToDateTimeHandler() includes the time
 *               zone offset in its date/time calculation. The time zone offset should
 *               NOT be applied before or after calling Clk_TS_ToDateTimeHandler().
 *               @n
 *               Time zone is based on Coordinated Universal Time (UTC) and uses the following values :
 *               - (a) Between  +|- 12 hours (+|- 43200 seconds)
 *               - (b) Multiples of a 15 minute interval
 *               @n
 *               Timestamp MUST be adjusted by adding timezone offset ('tz_sec') :
 *               - (c) For negative timezone offset, the amount is subtracted
 *               - (d) For positive timezone offset, the amount is added
 *               @n
 *               Time zone field of the date/time structure ('p_date_time->TZ_sec') is set
 *               to the value of time zone argument ('tz_sec').
 *
 * @note     (2) Absolute value of the time zone offset is stored into 'CLK_TS_SEC' data type to be
 *               compliant with unsigned integer verification/operations.
 *******************************************************************************************************/
static CPU_BOOLEAN Clk_TS_ToDateTimeHandler(CLK_TS_SEC    ts_sec,
                                            CLK_TZ_SEC    tz_sec,
                                            CLK_DATE_TIME *p_date_time,
                                            CLK_YR        yr_start,
                                            CLK_YR        yr_end)
{
  CLK_TS_SEC   ts_sec_rem;
  CLK_TS_SEC   tz_sec_abs;                                      // See Note #2.
  CLK_TS_SEC   sec_to_remove;
  CLK_NBR_DAYS days_in_yr;
  CLK_DAY      days_in_month;
  CPU_INT08U   leap_yr_ix;
  CPU_BOOLEAN  leap_yr;
  CPU_BOOLEAN  valid;

  RTOS_ASSERT_DBG((p_date_time != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);

  //                                                               ------------ ADJ INIT TS FOR TZ OFFSET -------------
  ts_sec_rem = ts_sec;
  tz_sec_abs = (CLK_TS_SEC)DEF_ABS(tz_sec);
  if (tz_sec < 0) {
    if (ts_sec_rem < tz_sec_abs) {                              // Chk for ovf when tz is neg.
      LOG_ERR(("TS_ToDateTime: Timestamp is too small to substract time zone offset."));
      return (DEF_FAIL);
    }
    ts_sec_rem -= tz_sec_abs;                                   // See Note #1c.
  } else {
    ts_sec_rem += tz_sec_abs;                                   // See Note #1d.
    if (ts_sec_rem < tz_sec_abs) {                              // Chk for ovf when tz is pos.
      LOG_ERR(("TS_ToDateTime: Timestamp is too big to add time zone offset."));
      return (DEF_FAIL);
    }
  }

  //                                                               ---------------------- GET YR ----------------------
  p_date_time->Yr = yr_start;
  leap_yr = Clk_IsLeapYr(p_date_time->Yr);
  leap_yr_ix = (leap_yr == DEF_YES) ? 1u : 0u;
  days_in_yr = Clk_DaysInYr[leap_yr_ix];
  sec_to_remove = days_in_yr * DEF_TIME_NBR_SEC_PER_DAY;
  while ((ts_sec_rem >= sec_to_remove)
         && (p_date_time->Yr < yr_end)) {
    ts_sec_rem -= sec_to_remove;
    p_date_time->Yr++;
    leap_yr = Clk_IsLeapYr(p_date_time->Yr);
    leap_yr_ix = (leap_yr == DEF_YES) ? 1u : 0u;
    days_in_yr = Clk_DaysInYr[leap_yr_ix];
    sec_to_remove = days_in_yr * DEF_TIME_NBR_SEC_PER_DAY;
  }

  if (p_date_time->Yr >= yr_end) {
    LOG_ERR(("TS_ToDateTime: Year conversion has failed."));
    return (DEF_FAIL);
  }

  //                                                               -------------------- GET MONTH ---------------------
  p_date_time->Month = CLK_FIRST_MONTH_OF_YR;
  days_in_month = Clk_DaysInMonth[leap_yr_ix][p_date_time->Month - CLK_FIRST_MONTH_OF_YR];
  sec_to_remove = days_in_month * DEF_TIME_NBR_SEC_PER_DAY;
  while ((ts_sec_rem >= sec_to_remove)
         && (p_date_time->Month < CLK_MONTH_PER_YR)) {
    ts_sec_rem -= sec_to_remove;
    p_date_time->Month++;
    days_in_month = Clk_DaysInMonth[leap_yr_ix][p_date_time->Month - CLK_FIRST_MONTH_OF_YR];
    sec_to_remove = days_in_month * DEF_TIME_NBR_SEC_PER_DAY;
  }

  if (p_date_time->Month > CLK_MONTH_PER_YR) {
    LOG_ERR(("TS_ToDateTime: Month conversion has failed."));
    return (DEF_FAIL);
  }

  //                                                               --------------------- GET DAY ----------------------
  sec_to_remove = DEF_TIME_NBR_SEC_PER_DAY;
  p_date_time->Day = (CLK_DAY)(ts_sec_rem / sec_to_remove);
  p_date_time->Day += CLK_FIRST_DAY_OF_MONTH;
  ts_sec_rem = ts_sec_rem % sec_to_remove;

  if (p_date_time->Day > days_in_month) {
    LOG_ERR(("TS_ToDateTime: Day conversion has failed."));
    return (DEF_FAIL);
  }

  //                                                               ------------------ GET DAY OF WK -------------------
  p_date_time->DayOfWk = Clk_GetDayOfWkHandler(p_date_time->Yr,
                                               p_date_time->Month,
                                               p_date_time->Day);
  valid = Clk_IsDayOfWkValid(p_date_time->DayOfWk);

  if ((p_date_time->Day > days_in_month)
      || (valid == DEF_NO)) {
    LOG_ERR(("TS_ToDateTime: Day conversion has failed."));
    return (DEF_FAIL);
  }

  p_date_time->DayOfYr = Clk_GetDayOfYrHandler(p_date_time->Yr,
                                               p_date_time->Month,
                                               p_date_time->Day);
  valid = Clk_IsDayOfYrValid(p_date_time->Yr,
                             p_date_time->DayOfYr);
  if (valid != DEF_OK) {
    LOG_ERR(("TS_ToDateTime: Day of year conversion has failed."));
    return (DEF_FAIL);
  }

  //                                                               --------------------- GET HR -----------------------
  sec_to_remove = DEF_TIME_NBR_SEC_PER_HR;
  p_date_time->Hr = (CLK_HR)(ts_sec_rem / sec_to_remove);
  ts_sec_rem = ts_sec_rem % sec_to_remove;

  //                                                               --------------------- GET MIN ----------------------
  sec_to_remove = DEF_TIME_NBR_SEC_PER_MIN;
  p_date_time->Min = (CLK_MIN)(ts_sec_rem / sec_to_remove);
  ts_sec_rem = ts_sec_rem % sec_to_remove;

  //                                                               --------------------- GET SEC ----------------------
  p_date_time->Sec = (CLK_SEC) ts_sec_rem;

  //                                                               --------------------- GET TZ -----------------------
  p_date_time->TZ_sec = tz_sec;

  //                                                               ------------------ VALIDATE TIME -------------------
  valid = Clk_IsTimeValid(p_date_time->Hr, p_date_time->Min, p_date_time->Sec);
  if (valid != DEF_OK) {
    LOG_ERR(("TS_ToDateTime: Time conversion has failed."));
    return  (DEF_FAIL);
  }

  LOG_VRB(("Date converted:\r\n    Year      = ", (u)p_date_time->Yr,
           "\r\n    Month     = ", (u)p_date_time->Month,
           "\r\n    Day       = ", (u)p_date_time->Day));
  LOG_VRB(("Time converted:\r\n    Hour      = ", (u)p_date_time->Hr,
           "\r\n    Minutes   = ", (u)p_date_time->Min,
           "\r\n    Seconds   = ", (u)p_date_time->Sec,
           "\r\n    Time zone = ", (d)p_date_time->TZ_sec));

  return (DEF_OK);
}

/****************************************************************************************************//**
 *                                       Clk_DateTimeToTS_Handler()
 *
 * @brief          Converts the date/time structure to any type of timestamp.
 *
 * @param          p_ts_sec        Pointer to the variable that will receive the timestamp :
 *                                 In seconds UTC+00, if NO error(s);
 *                                 CLK_TS_SEC_NONE, otherwise.
 *                                 Argument checked in Clk_DateTimeToTS(),
 *                                 Clk_DateTimeToTS_NTP(),
 *                                 Clk_DateTimeToTS_Unix().
 *
 * @param          p_date_time     Pointer to the variable that contains the date/time structure to validate.
 *                                 Argument checked in Clk_DateTimeToTS(),
 *                                 Clk_DateTimeToTS_NTP(),
 *                                 Clk_DateTimeToTS_Unix().
 *
 * @param          yr_start        Start year of the epoch.
 *
 * @return        DEF_OK, if the timestamp is valid.
 *               DEF_FAIL, otherwise.
 *
 * @note     (1) Date/time ('p_date_time') should be set to local time with correct time zone
 *               offset ('p_date_time->TZ_sec'). Clk_DateTimeToTS_Handler() removes the time
 *               zone offset from the date/time to calculate & return a Clock timestamp at
 *               UTC+00.
 *               - (a) Time zone is based on Coordinated Universal Time (UTC) and uses the following values :
 *                   - (1) Between  +|- 12 hours (+|- 43200 seconds)
 *                   - (2) Multiples of a 15 minute interval
 *               - (b) Timestamp MUST be adjusted by subtracting timezone offset ('p_date_time->TZ_sec') :
 *                   - (1) For negative timezone offset, the amount is added
 *                   - (2) For positive timezone offset, the amount is subtracted
 *
 * @note     (2) In case of errors, pointers to variables that return values MUST be initialized
 *               before all other validation or function handling.
 *               - (a) However, some pointers may already be initialized by calling functions. These
 *                     pointers do NOT need to be re-initialized but are shown for completeness.
 *
 * @note     (3) Absolute value of the time zone offset is stored into 'CLK_TS_SEC' data type to be
 *                   compliant with unsigned integer verification/operations.
 *******************************************************************************************************/
static CPU_BOOLEAN Clk_DateTimeToTS_Handler(CLK_TS_SEC    *p_ts_sec,
                                            CLK_DATE_TIME *p_date_time,
                                            CLK_YR        yr_start)
{
  CPU_BOOLEAN  leap_yr;
  CPU_INT08U   leap_yr_ix;
  CLK_YR       yr_ix;
  CLK_MONTH    month_ix;
  CLK_NBR_DAYS nbr_days;
  CLK_TS_SEC   ts_sec;
  CLK_TS_SEC   tz_sec_abs;                                      // See Note #3.

#if 0                                                           // See Note #2a.
  *p_ts_sec = CLK_TS_SEC_NONE;                                  // Init to ts none for err (see Note #2).
#endif

  LOG_VRB(("Date converted:\r\n     Year      = ", (u)p_date_time->Yr,
           "\r\n     Month     = ", (u)p_date_time->Month,
           "\r\n     Day       = ", (u)p_date_time->Day));
  LOG_VRB(("Time converted:\r\n     Hour      = ", (u)p_date_time->Hr,
           "\r\n     Minutes   = ", (u)p_date_time->Min,
           "\r\n     Seconds   = ", (u)p_date_time->Sec,
           "\r\n     Time zone = ", (d)p_date_time->TZ_sec));

  //                                                               ------------- CONV DATE/TIME TO CLK TS -------------
  nbr_days = p_date_time->Day - CLK_FIRST_DAY_OF_MONTH;
  leap_yr = Clk_IsLeapYr(p_date_time->Yr);
  leap_yr_ix = (leap_yr == DEF_YES) ? 1u : 0u;
  for (month_ix = CLK_FIRST_MONTH_OF_YR; month_ix < p_date_time->Month; month_ix++) {
    nbr_days += Clk_DaysInMonth[leap_yr_ix][month_ix - CLK_FIRST_MONTH_OF_YR];
  }

  for (yr_ix = yr_start; yr_ix < p_date_time->Yr; yr_ix++) {
    leap_yr = Clk_IsLeapYr(yr_ix);
    leap_yr_ix = (leap_yr == DEF_YES) ? 1u : 0u;
    nbr_days += Clk_DaysInYr[leap_yr_ix];
  }

  ts_sec = nbr_days         * DEF_TIME_NBR_SEC_PER_DAY;
  ts_sec += p_date_time->Hr  * DEF_TIME_NBR_SEC_PER_HR;
  ts_sec += p_date_time->Min * DEF_TIME_NBR_SEC_PER_MIN;
  ts_sec += p_date_time->Sec;

  //                                                               ------------ ADJ FINAL TS FOR TZ OFFSET ------------
  tz_sec_abs = (CLK_TS_SEC)DEF_ABS(p_date_time->TZ_sec);
  if (p_date_time->TZ_sec < 0) {
    ts_sec += tz_sec_abs;                                       // See Note #1c1.
    if (ts_sec < tz_sec_abs) {                                  // Chk for ovf when tz is neg.
      LOG_ERR(("DateTimeToTS: Timestamp is too big to add time zone offset."));
      return (DEF_FAIL);
    }
  } else {
    if (ts_sec < tz_sec_abs) {                                  // Chk for ovf when tz is pos.
      LOG_ERR(("DateTimeToTS: Timestamp is too small to substract time zone offset."));
      return (DEF_FAIL);
    }
    ts_sec -= tz_sec_abs;                                       // See Note #1c2.
  }

  *p_ts_sec = ts_sec;

  return (DEF_OK);
}

/****************************************************************************************************//**
 *                                           Clk_DateTimeMakeHandler()
 *
 * @brief    Builds a valid epoch date/time structure.
 *
 * @param    p_date_time     Pointer to the variable that will receive the date/time structure.
 *
 * @param    yr              Year    value [yr_start      to yr_end).
 *
 * @param    month           Month   value [CLK_MONTH_JAN to CLK_MONTH_DEC].
 *
 * @param    day             Day     value [            1 to            31].
 *
 * @param    hr              Hours   value [            0 to            23].
 *
 * @param    min             Minutes value [            0 to            59].
 *
 * @param    sec             Seconds value [            0 to            60] (see Note #2).
 *
 * @param    tz_sec          Time zone offset (in seconds, +|- from UTC) [-43200 to 43200].
 *
 * @param    yr_start        Start year of the epoch.
 *
 * @param    yr_end          End year of the epoch.
 *
 * @return   DEF_OK, if date/time structure is valid.
 *           DEF_FAIL, otherwise.
 *
 * @note     (1) Day of week ('p_date_time->DayOfWk') and Day of year ('p_date_time->DayOfYr')
 *               are internally calculated and set in the date/time structure if date is valid.
 *
 * @note     (2) The Seconds value of 60 is valid to be compatible with leap second adjustment and
 *               the atomic clock time structure.
 *
 * @note     (3) Time zone is based ('tz_sec') on Coordinated Universal Time (UTC) and has valid
 *               values :
 *               - (a) Between  +|- 12 hours (+|- 43200 seconds)
 *               - (b) Multiples of 15 minute intervals
 *******************************************************************************************************/
static CPU_BOOLEAN Clk_DateTimeMakeHandler(CLK_DATE_TIME *p_date_time,
                                           CLK_YR        yr,
                                           CLK_MONTH     month,
                                           CLK_DAY       day,
                                           CLK_HR        hr,
                                           CLK_MIN       min,
                                           CLK_SEC       sec,
                                           CLK_TZ_SEC    tz_sec,
                                           CLK_YR        yr_start,
                                           CLK_YR        yr_end)
{
  CPU_BOOLEAN valid;

  RTOS_ASSERT_DBG((p_date_time != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);

  //                                                               ------------------ VALIDATE DATE -------------------
  valid = Clk_IsDateValid(yr, month, day, yr_start, yr_end);
  if (valid != DEF_YES) {
    return  (DEF_FAIL);
  }

  p_date_time->Yr = yr;
  p_date_time->Month = month;
  p_date_time->Day = day;
  p_date_time->DayOfWk = Clk_GetDayOfWkHandler(yr, month, day);
  p_date_time->DayOfYr = Clk_GetDayOfYrHandler(yr, month, day);

  //                                                               ---------------- VALIDATE DAY OF WK ----------------
  valid = Clk_IsDayOfWkValid(p_date_time->DayOfWk);
  if (valid != DEF_YES) {
    return  (DEF_FAIL);
  }

  //                                                               ---------------- VALIDATE DAY OF YR ----------------
  valid = Clk_IsDayOfYrValid(yr, p_date_time->DayOfYr);
  if (valid != DEF_YES) {
    return  (DEF_FAIL);
  }

  //                                                               ------------------ VALIDATE TIME -------------------
  valid = Clk_IsTimeValid(hr, min, sec);
  if (valid != DEF_YES) {
    return  (DEF_FAIL);
  }

  p_date_time->Hr = hr;
  p_date_time->Min = min;
  p_date_time->Sec = sec;

  //                                                               ------------------- VALIDATE TZ --------------------
  valid = Clk_IsTZValid(tz_sec);
  if (valid != DEF_YES) {
    return  (DEF_FAIL);
  }

  p_date_time->TZ_sec = tz_sec;

  return (DEF_OK);
}

/****************************************************************************************************//**
 *                                               Clk_TaskHandler()
 *
 * @brief    Handles clock signal : waits for a signal and updates the timestamp.
 *
 * @param    p_arg   Pointer to argument (unused).
 *
 * @note
 *******************************************************************************************************/

#if ((CLK_CFG_EXT_EN == DEF_DISABLED) \
  && (CLK_CFG_SIGNAL_EN == DEF_DISABLED))
static void Clk_TaskHandler(void *p_arg)
{
  CPU_SR_ALLOC();

  PP_UNUSED_PARAM(p_arg);

  while (DEF_ON) {
    KAL_DlyTick(KAL_TickRateGet(),
                KAL_OPT_DLY_NONE);

    //                                                             -------------------- UPDATE TS ---------------------
    CPU_CRITICAL_ENTER();
    if (Clk_Ptr->Clk_TS_UTC_sec < CLK_TS_SEC_MAX) {
      Clk_Ptr->Clk_TS_UTC_sec++;
    }
    CPU_CRITICAL_EXIT();
  }
}
#endif
