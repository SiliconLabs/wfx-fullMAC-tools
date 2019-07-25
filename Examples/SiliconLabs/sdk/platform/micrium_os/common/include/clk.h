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

/****************************************************************************************************//**
 * @defgroup COMMON_CLK Clock API
 * @ingroup  COMMON
 * @brief      Clock API
 *
 * @addtogroup COMMON_CLK
 * @{
 ********************************************************************************************************
 * @note       - (a) Clock module is based on Coordinated Universal Time (UTC) and supports the following
 *                   features: time zones, leap years and leap seconds.
 *             - (b) Clock module does NOT support Daylight (Savings) Time. If you want to handle Daylight
 *                   Time in your application, set time zone offset accordingly.
 *             - (c) Timestamp and Coordinated Universal Time (UTC) related links:
 *                 - (1) http://www.timeanddate.com/time/aboututc.html
 *                 - (2) http://www.allanstime.com/Publications/DWA/Science_Timekeeping/TheScienceOfTimekeeping.pdf
 *                 - (3) http://www.cl.cam.ac.uk/~mgk25/time/metrologia-leapsecond.pdf
 *
 * @note       - (a) Clock module implements a software-maintained clock/calendar when 'CLK_CFG_EXT_EN' is
 *                   disabled.
 *             - (b) Software-maintained clock/calendar is based on a periodic delay or timeout when
 *                       'CLK_CFG_SIGNAL_EN' is disabled.
 *                   Software-maintained clock/calendar is based on a periodic signal or timer when
 *                       'CLK_CFG_SIGNAL_EN' is enabled.
 *             - (c) When software-maintained clock is enabled, Clock module's OS-dependent files and
 *                   respective OS-application configuration MUST be included in the build.
 *
 * @note       - (a) Clock module initializes, gets and sets its timestamp via an External timestamp when
 *                   'CLK_CFG_EXT_EN' is enabled.
 *             - (b)     External timestamp can be maintained either in: hardware (e.g. via a hardware
 *                       clock chip) or from another application (e.g. SNTPc).
 *                       External timestamp is accessed by application/BSP functions defined by the
 *                       developer that MUST follow the functional requirements of the particular
 *                       hardware/application(s).
 *                       See also 'net.h  Clk_ExtTS_Init()',
 *                                   'net.h  Clk_ExtTS_Get()' and
 *                                   'net.h  Clk_ExtTS_Set()'.
 *******************************************************************************************************/

/********************************************************************************************************
 ********************************************************************************************************
 *                                               MODULE
 ********************************************************************************************************
 *******************************************************************************************************/

#ifndef  _CLK_H_
#define  _CLK_H_

/********************************************************************************************************
 ********************************************************************************************************
 *                                               INCLUDE FILES
 ********************************************************************************************************
 *******************************************************************************************************/

#include  <cpu/include/cpu.h>

#include  <common/include/lib_str.h>
#include  <common/include/lib_mem.h>
#include  <common/include/rtos_path.h>
#include  <common/include/rtos_types.h>

#include  <common_cfg.h>
#include  <rtos_cfg.h>

/********************************************************************************************************
 ********************************************************************************************************
 *                                               DEFINES
 ********************************************************************************************************
 *******************************************************************************************************/

/********************************************************************************************************
 *                                       CLK STR FORMAT DEFINES
 *******************************************************************************************************/

//                                                                 1         2         3
//                                                                 0123456789012345678901234567890
//                                                                     Str len of fmt "YYYY-MM-DD HH:MM:SS UTC+TZ" :
//                                                                             ...    "YYYY-MM-DD HH:MM:SS UTC+hh:mm"
//                                                                             ... or "YYYY-MM-DD HH:MM:SS UTC-hh:mm".
#define  CLK_STR_FMT_YYYY_MM_DD_HH_MM_SS_UTC_LEN          30u
#define  CLK_STR_FMT_YYYY_MM_DD_HH_MM_SS_LEN              20u   ///<     Str len of fmt "YYYY-MM-DD HH:MM:SS".
#define  CLK_STR_FMT_MM_DD_YY_HH_MM_SS_LEN                18u   ///<     Str len of fmt "MM-DD-YY HH:MM:SS".
#define  CLK_STR_FMT_YYYY_MM_DD_LEN                       11u   ///<     Str len of fmt "YYYY-MM-DD".
#define  CLK_STR_FMT_MM_DD_YY_LEN                          9u   ///<     Str len of fmt "MM-DD-YY".
#define  CLK_STR_FMT_DAY_MONTH_DD_YYYY_MAX_LEN            29u   ///< Max str len of fmt "Day Month DD, YYYY".
#define  CLK_STR_FMT_DAY_MONTH_DD_HH_MM_SS_YYYY_LEN       25u   ///<     Str len of fmt "Day Mon DD HH:MM:SS YYYY".
#define  CLK_STR_FMT_HH_MM_SS_LEN                          9u   ///<     Str len of fmt "HH:MM:SS".
#define  CLK_STR_FMT_HH_MM_SS_AM_PM_LEN                   15u   ///<     Str len of fmt "HH:MM:SS AM|PM".
#define  CLK_STR_FMT_HH_MM_AM_PM_LEN                       9u   ///<     Str len of fmt "HH:MM    AM|PM".

#define  CLK_STR_FMT_MAX_LEN                              30u   ///< Max str len of all clk str fmts.

#define  CLK_STR_DIG_YR_LEN                                4u   ///<     Str len of       yr dig.
#define  CLK_STR_DIG_YR_TRUNC_LEN                          2u   ///<     Str len of trunc yr dig.
#define  CLK_STR_DIG_MONTH_LEN                             2u   ///<     Str len of mon      dig.
#define  CLK_STR_DIG_DAY_LEN                               2u   ///<     Str len of day      dig.
#define  CLK_STR_DIG_HR_LEN                                2u   ///<     Str len of hr       dig.
#define  CLK_STR_DIG_MIN_LEN                               2u   ///<     Str len of min      dig.
#define  CLK_STR_DIG_SEC_LEN                               2u   ///<     Str len of sec      dig.
#define  CLK_STR_DIG_TZ_HR_LEN                             2u   ///<     Str len of tz hr    dig.
#define  CLK_STR_DIG_TZ_MIN_LEN                            2u   ///<     Str len of tz min   dig.
#define  CLK_STR_DIG_TZ_MAX_LEN                            2u   ///< Max str len of tz       digs.
#define  CLK_STR_DAY_OF_WK_MAX_LEN                         9u   ///< Max str len of day of wk       str (e.g. Wednesday)
#define  CLK_STR_DAY_OF_WK_TRUNC_LEN                       3u   ///<     Str len of day of wk trunc str.
#define  CLK_STR_MONTH_MAX_LEN                             9u   ///< Max str len of month           str (e.g. September)
#define  CLK_STR_MONTH_TRUNC_LEN                           3u   ///<     Str len of month     trunc str.
#define  CLK_STR_AM_PM_LEN                                 2u   ///<     Str len of am-pm           str.

/********************************************************************************************************
 *                                               CLOCK DEFINES
 *
 * Note(s) : (1) Year 2038 problem (e.g. Unix Millennium bug, Y2K38 or Y2.038K) may cause some computer
 *               software to fail before or in the year 2038. The problem affects all software and
 *               systems that both store time as a signed 32-bit integer and interpret this number as
 *               the number of seconds since 00:00:00 UTC on 1970/01/1.
 *
 *               There is no straightforward and general fix for this problem. Changing timestamp
 *               datatype to an unsigned 32-bit integer have been chosen to avoid this problem. Thus
 *               timestamp will be accurate until the year 2106, but dates before 1970 are not possible.
 *******************************************************************************************************/

#define  CLK_FIRST_MONTH_OF_YR                             1u   ///< First month of a yr    [1 to  12].
#define  CLK_FIRST_DAY_OF_MONTH                            1u   ///< First day   of a month [1 to  31].
#define  CLK_FIRST_DAY_OF_YR                               1u   ///< First day   of a yr    [1 to 366].
#define  CLK_FIRST_DAY_OF_WK                               1u   ///< First day   of a wk    [1 to   7].

#define  CLK_MONTH_PER_YR                                 12u
#define  CLK_HR_PER_HALF_DAY                              12uL

#define  CLK_YR_NONE                                       0u

#define  CLK_MONTH_NONE                                    0u
#define  CLK_MONTH_JAN                                     1u
#define  CLK_MONTH_FEB                                     2u
#define  CLK_MONTH_MAR                                     3u
#define  CLK_MONTH_APR                                     4u
#define  CLK_MONTH_MAY                                     5u
#define  CLK_MONTH_JUN                                     6u
#define  CLK_MONTH_JUL                                     7u
#define  CLK_MONTH_AUG                                     8u
#define  CLK_MONTH_SEP                                     9u
#define  CLK_MONTH_OCT                                    10u
#define  CLK_MONTH_NOV                                    11u
#define  CLK_MONTH_DEC                                    12u

#define  CLK_DAY_NONE                                      0u
#define  CLK_DAY_OF_WK_NONE                                0u
#define  CLK_DAY_OF_WK_SUN                                 1u
#define  CLK_DAY_OF_WK_MON                                 2u
#define  CLK_DAY_OF_WK_TUE                                 3u
#define  CLK_DAY_OF_WK_WED                                 4u
#define  CLK_DAY_OF_WK_THU                                 5u
#define  CLK_DAY_OF_WK_FRI                                 6u
#define  CLK_DAY_OF_WK_SAT                                 7u

//                                                                 ------------------ CLK TS DEFINES ------------------
#define  CLK_TS_SEC_MIN                 DEF_INT_32U_MIN_VAL
#define  CLK_TS_SEC_MAX                 DEF_INT_32U_MAX_VAL
#define  CLK_TS_SEC_NONE                CLK_TS_SEC_MIN

//                                                                 ------------------ CLK TZ DEFINES ------------------
#define  CLK_TZ_MIN_PRECISION                             15uL
#define  CLK_TZ_SEC_PRECISION          (CLK_TZ_MIN_PRECISION * DEF_TIME_NBR_SEC_PER_MIN)
#define  CLK_TZ_SEC_MIN              (-(CLK_HR_PER_HALF_DAY  * DEF_TIME_NBR_SEC_PER_HR))
#define  CLK_TZ_SEC_MAX                (CLK_HR_PER_HALF_DAY  * DEF_TIME_NBR_SEC_PER_HR)

#define  CLK_TZ_SEC_FROM_UTC_GET(utc)  ((utc) * DEF_TIME_NBR_SEC_PER_HR)

//                                                                 ----------------- CLK TICK DEFINES -----------------
#define  CLK_TICK_NONE                                     0u

//                                                                 ---------------- CLK EPOCH DEFINES -----------------
#define  CLK_EPOCH_YR_START                             2000u   ///< Clk epoch starts = 2000-01-01 00:00:00 UTC.
#define  CLK_EPOCH_YR_END                               2136u   ///<           ends   = 2135-12-31 23:59:59 UTC.
#define  CLK_EPOCH_DAY_OF_WK                               7u   ///<                    2000-01-01 is Sat.

//                                                                 -------------- NTP EPOCH DATE DEFINES --------------
#define  CLK_NTP_EPOCH_YR_START                         1900u   ///< NTP epoch starts = 1900-01-01 00:00:00 UTC.
#define  CLK_NTP_EPOCH_YR_END                           2036u   ///<           ends   = 2035-12-31 23:59:59 UTC.
#define  CLK_NTP_EPOCH_DAY_OF_WK                           2u   ///<                    1900-01-01 is Mon.
#define  CLK_NTP_EPOCH_OFFSET_YR_CNT           (CLK_EPOCH_YR_START - CLK_NTP_EPOCH_YR_START)

//                                                                 Only 24 leap yrs because 1900 is NOT a leap yr.
#define  CLK_NTP_EPOCH_OFFSET_LEAP_DAY_CNT    ((CLK_NTP_EPOCH_OFFSET_YR_CNT / 4u) - 1u)

//                                                                 100 yrs * 365 * 24 * 60 * 60 = 3153600000
//                                                                 +  24 leap days * 24 * 60 * 60 =    2073600
//                                                                 CLK_NTP_OFFSET_SEC               = 3155673600
#define  CLK_NTP_EPOCH_OFFSET_SEC             ((CLK_NTP_EPOCH_OFFSET_YR_CNT       * DEF_TIME_NBR_SEC_PER_YR) \
                                               + (CLK_NTP_EPOCH_OFFSET_LEAP_DAY_CNT * DEF_TIME_NBR_SEC_PER_DAY))

//                                                                 ------------- UNIX EPOCH DATE DEFINES --------------
//                                                                 See Note #1.
#define  CLK_UNIX_EPOCH_YR_START                        1970u   ///< Unix epoch starts = 1970-01-01 00:00:00 UTC.
#define  CLK_UNIX_EPOCH_YR_END                          2106u   ///<            ends   = 2105-12-31 23:59:59 UTC.
#define  CLK_UNIX_EPOCH_DAY_OF_WK                          5u   ///<                     1970-01-01 is Thu.
#define  CLK_UNIX_EPOCH_OFFSET_YR_CNT          (CLK_EPOCH_YR_START - CLK_UNIX_EPOCH_YR_START)
#define  CLK_UNIX_EPOCH_OFFSET_LEAP_DAY_CNT    (CLK_UNIX_EPOCH_OFFSET_YR_CNT / 4u)

//                                                                 30 yrs * 365 * 24 * 60 * 60 = 946080000
//                                                                 +  7 leap days * 24 * 60 * 60 =    604800
//                                                                 CLK_UNIX_OFFSET_SEC             = 946684800
#define  CLK_UNIX_EPOCH_OFFSET_SEC            ((CLK_UNIX_EPOCH_OFFSET_YR_CNT       * DEF_TIME_NBR_SEC_PER_YR) \
                                               + (CLK_UNIX_EPOCH_OFFSET_LEAP_DAY_CNT * DEF_TIME_NBR_SEC_PER_DAY))

/********************************************************************************************************
 ********************************************************************************************************
 *                                               DATA TYPES
 ********************************************************************************************************
 *******************************************************************************************************/

/********************************************************************************************************
 *                                       CLOCK FORMAT STRING DATA TYPE
 *******************************************************************************************************/

#if (CLK_CFG_STR_CONV_EN == DEF_ENABLED)
typedef enum clk_str_fmt {
  CLK_STR_FMT_YYYY_MM_DD_HH_MM_SS_UTC = 1u,
  CLK_STR_FMT_YYYY_MM_DD_HH_MM_SS = 2u,
  CLK_STR_FMT_MM_DD_YY_HH_MM_SS = 3u,
  CLK_STR_FMT_YYYY_MM_DD = 4u,
  CLK_STR_FMT_MM_DD_YY = 5u,
  CLK_STR_FMT_DAY_MONTH_DD_YYYY = 6u,
  CLK_STR_FMT_DAY_MONTH_DD_HH_MM_SS_YYYY = 7u,
  CLK_STR_FMT_HH_MM_SS = 8u,
  CLK_STR_FMT_HH_MM_SS_AM_PM = 9u,
  CLK_STR_FMT_HH_MM_AM_PM
} CLK_STR_FMT;
#endif

/********************************************************************************************************
 *                                           CLOCK DATE DATA TYPES
 *******************************************************************************************************/

typedef CPU_INT16U CLK_YR;
typedef CPU_INT08U CLK_MONTH;
typedef CPU_INT16U CLK_DAY;
typedef CPU_INT32U CLK_NBR_DAYS;

/********************************************************************************************************
 *                                           CLOCK TIME DATA TYPES
 *******************************************************************************************************/

typedef CPU_INT08U CLK_HR;
typedef CPU_INT08U CLK_MIN;
typedef CPU_INT08U CLK_SEC;

/********************************************************************************************************
 *                                       CLOCK TIMESTAMP DATA TYPE
 *******************************************************************************************************/

typedef CPU_INT32U CLK_TS_SEC;

/********************************************************************************************************
 *                                       CLOCK TIME ZONE DATA TYPE
 *******************************************************************************************************/

typedef CPU_INT32S CLK_TZ_SEC;

/********************************************************************************************************
 *                                       CLOCK DATE/TIME DATA TYPE
 *
 * Note(s) : (1) Same date/time structure is used for all epoch. Thus Year value ('Yr') should be a value
 *               between the epoch start and end years.
 *
 *           (2) Seconds value of 60 is valid to be compatible with leap second adjustment and the atomic
 *               clock time stucture.
 *
 *           (3) Time zone is based on Coordinated Universal Time (UTC) & has valid values :
 *
 *               (a) Between  +|- 12 hours (+|- 43200 seconds)
 *               (b) Multiples of 15 minutes
 *******************************************************************************************************/

typedef struct clk_date_time {
  CLK_YR     Yr;                                                ///< Yr        [epoch start to end yr), (see Note #1).
  CLK_MONTH  Month;                                             ///< Month     [          1 to     12], (Jan to Dec).
  CLK_DAY    Day;                                               ///< Day       [          1 to     31].
  CLK_DAY    DayOfWk;                                           ///< Day of wk [          1 to      7], (Sun to Sat).
  CLK_DAY    DayOfYr;                                           ///< Day of yr [          1 to    366].
  CLK_HR     Hr;                                                ///< Hr        [          0 to     23].
  CLK_MIN    Min;                                               ///< Min       [          0 to     59].
  CLK_SEC    Sec;                                               ///< Sec       [          0 to     60], (see Note #2).
  CLK_TZ_SEC TZ_sec;                                            ///< TZ        [     -43200 to  43200], (see Note #3).
} CLK_DATE_TIME;

/********************************************************************************************************
 *                                           INIT CFG STRUCTURE
 *******************************************************************************************************/

typedef struct clk_init_cfg {
#if ((CLK_CFG_EXT_EN == DEF_DISABLED) \
  && (CLK_CFG_SIGNAL_EN == DEF_DISABLED))
  CPU_STK_SIZE StkSizeElements;                                 ///< Size of the stk, in CPU_STK elements.
  CPU_STK      *StkPtr;                                         ///< Ptr to base of the stk.
#endif
  MEM_SEG      *MemSegPtr;                                      ///< Ptr to clk mem seg.
} CLK_INIT_CFG;

/********************************************************************************************************
 ********************************************************************************************************
 *                                           GLOBAL VARIABLES
 ********************************************************************************************************
 *******************************************************************************************************/

#if (RTOS_CFG_EXTERNALIZE_OPTIONAL_CFG_EN == DEF_DISABLED)
extern const CLK_INIT_CFG Clk_InitCfgDflt;
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                           FUNCTION PROTOTYPES
 ********************************************************************************************************
 *******************************************************************************************************/

/********************************************************************************************************
 *                                       GENERIC CLK FUNCTION PROTOTYPES
 *******************************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#if ((CLK_CFG_EXT_EN == DEF_DISABLED)    \
  && (CLK_CFG_SIGNAL_EN == DEF_DISABLED) \
  && (RTOS_CFG_EXTERNALIZE_OPTIONAL_CFG_EN == DEF_DISABLED))
void Clk_ConfigureTaskStk(CPU_STK_SIZE stk_size_elements,
                          CPU_STK      *p_stk_base);
#endif

#if (RTOS_CFG_EXTERNALIZE_OPTIONAL_CFG_EN == DEF_DISABLED)
void Clk_ConfigureMemSeg(MEM_SEG *p_mem_seg);
#endif

void Clk_Init(RTOS_ERR *p_err);

#if ((CLK_CFG_EXT_EN == DEF_DISABLED) \
  && (CLK_CFG_SIGNAL_EN == DEF_DISABLED))
void Clk_TaskPrioSet(RTOS_TASK_PRIO prio,
                     RTOS_ERR       *p_err);
#endif

#if    ((CLK_CFG_SIGNAL_EN == DEF_ENABLED) \
  && (CLK_CFG_EXT_EN == DEF_DISABLED))
void Clk_SignalClk(RTOS_ERR *p_err);
#endif

CLK_TS_SEC Clk_GetTS(void);

CPU_BOOLEAN Clk_SetTS(CLK_TS_SEC ts_sec);

CLK_TZ_SEC Clk_GetTZ(void);

CPU_BOOLEAN Clk_SetTZ(CLK_TZ_SEC tz_sec);

CPU_BOOLEAN Clk_GetDateTime(CLK_DATE_TIME *p_date_time);

CPU_BOOLEAN Clk_SetDateTime(CLK_DATE_TIME *p_date_time);

CPU_BOOLEAN Clk_TS_ToDateTime(CLK_TS_SEC    ts_sec,
                              CLK_TZ_SEC    tz_sec,
                              CLK_DATE_TIME *p_date_time);

CPU_BOOLEAN Clk_DateTimeToTS(CLK_TS_SEC    *p_ts_sec,
                             CLK_DATE_TIME *p_date_time);

CPU_BOOLEAN Clk_DateTimeMake(CLK_DATE_TIME *p_date_time,
                             CLK_YR        yr,
                             CLK_MONTH     month,
                             CLK_DAY       day,
                             CLK_HR        hr,
                             CLK_MIN       min,
                             CLK_SEC       sec,
                             CLK_TZ_SEC    tz_sec);

CPU_BOOLEAN Clk_IsDateTimeValid(CLK_DATE_TIME *p_date_time);

CLK_DAY Clk_GetDayOfWk(CLK_YR    yr,
                       CLK_MONTH month,
                       CLK_DAY   day);

CLK_DAY Clk_GetDayOfYr(CLK_YR    yr,
                       CLK_MONTH month,
                       CLK_DAY   day);

/********************************************************************************************************
 *                                   STR CONV UTIL FUNCTION PROTOTYPES
 *******************************************************************************************************/

#if (CLK_CFG_STR_CONV_EN == DEF_ENABLED)
CPU_BOOLEAN Clk_DateTimeToStr(CLK_DATE_TIME *p_date_time,
                              CLK_STR_FMT   fmt,
                              CPU_CHAR      *p_str,
                              CPU_SIZE_T    str_len);
#endif

/********************************************************************************************************
 *                                   NTP-FORMAT CLK FUNCTION PROTOTYPES
 *******************************************************************************************************/

#if (CLK_CFG_NTP_EN == DEF_ENABLED)
CPU_BOOLEAN Clk_GetTS_NTP(CLK_TS_SEC *p_ts_ntp_sec);

CPU_BOOLEAN Clk_SetTS_NTP(CLK_TS_SEC ts_ntp_sec);

CPU_BOOLEAN Clk_TS_ToTS_NTP(CLK_TS_SEC ts_sec,
                            CLK_TS_SEC *p_ts_ntp_sec);

CPU_BOOLEAN Clk_TS_NTP_ToTS(CLK_TS_SEC *p_ts_sec,
                            CLK_TS_SEC ts_ntp_sec);

CPU_BOOLEAN Clk_TS_NTP_ToDateTime(CLK_TS_SEC    ts_ntp_sec,
                                  CLK_TZ_SEC    tz_sec,
                                  CLK_DATE_TIME *p_date_time);

CPU_BOOLEAN Clk_DateTimeToTS_NTP(CLK_TS_SEC    *p_ts_ntp_sec,
                                 CLK_DATE_TIME *p_date_time);

CPU_BOOLEAN Clk_NTP_DateTimeMake(CLK_DATE_TIME *p_date_time,
                                 CLK_YR        yr,
                                 CLK_MONTH     month,
                                 CLK_DAY       day,
                                 CLK_HR        hr,
                                 CLK_MIN       min,
                                 CLK_SEC       sec,
                                 CLK_TZ_SEC    tz_sec);

CPU_BOOLEAN Clk_IsNTP_DateTimeValid(CLK_DATE_TIME *p_date_time);
#endif

/********************************************************************************************************
 *                                   UNIX-FORMAT CLK FUNCTION PROTOTYPES
 *******************************************************************************************************/

#if (CLK_CFG_UNIX_EN == DEF_ENABLED)
CPU_BOOLEAN Clk_GetTS_Unix(CLK_TS_SEC *p_ts_unix_sec);

CPU_BOOLEAN Clk_SetTS_Unix(CLK_TS_SEC ts_unix_sec);

CPU_BOOLEAN Clk_TS_ToTS_Unix(CLK_TS_SEC ts_sec,
                             CLK_TS_SEC *p_ts_unix_sec);

CPU_BOOLEAN Clk_TS_UnixToTS(CLK_TS_SEC *p_ts_sec,
                            CLK_TS_SEC ts_unix_sec);

CPU_BOOLEAN Clk_TS_UnixToDateTime(CLK_TS_SEC    ts_unix_sec,
                                  CLK_TZ_SEC    tz_sec,
                                  CLK_DATE_TIME *p_date_time);

CPU_BOOLEAN Clk_DateTimeToTS_Unix(CLK_TS_SEC    *p_ts_unix_sec,
                                  CLK_DATE_TIME *p_date_time);

CPU_BOOLEAN Clk_UnixDateTimeMake(CLK_DATE_TIME *p_date_time,
                                 CLK_YR        yr,
                                 CLK_MONTH     month,
                                 CLK_DAY       day,
                                 CLK_HR        hr,
                                 CLK_MIN       min,
                                 CLK_SEC       sec,
                                 CLK_TZ_SEC    tz_sec);

CPU_BOOLEAN Clk_IsUnixDateTimeValid(CLK_DATE_TIME *p_date_time);
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                           FUNCTION PROTOTYPES
 *                                       DEFINED IN PRODUCT'S BSP
 ********************************************************************************************************
 *******************************************************************************************************/

/****************************************************************************************************//**
 *                                               Clk_ExtTS_Init()
 *
 * @brief    Initialize & start External timestamp timer.
 *
 * @note     (1) CLK_ExtTS_Init() is an application/BSP function that MUST be defined by the developer
 *               if External timestamp is enabled.
 *
 *               See 'common_cfg.h  CLK CONFIGURATION  Note #1'.
 *
 * @note     (2) External timestamp SHOULD be an 'up' counter whose values increase at each
 *               second. It's possible to use a 'down' counter, but a conversion MUST be applied
 *               when setting and getting timestamp.
 *               External timestamp COULD come from another application (e.g. by SNTPc).
 *
 * @note     (3) This function is an INTERNAL Clock module function & MUST be implemented by
 *               application/BSP function(s) [see Note #1] but MUST NOT be called by application
 *               function(s).
 *******************************************************************************************************/

#if (CLK_CFG_EXT_EN == DEF_ENABLED)
void Clk_ExtTS_Init(void);
#endif

/****************************************************************************************************//**
 *                                               Clk_ExtTS_Get()
 *
 * @brief    Get Clock module's timestamp from converted External timestamp.
 *
 * @return   Current Clock timestamp (in seconds, UTC+00).
 *
 * @note     (1) Clk_ExtTS_Get() is an application/BSP function that MUST be defined by the developer
 *               if External timestamp is enabled.
 *
 *               See 'common_cfg.h  CLK CONFIGURATION  Note #1'
 *
 * @note     (2) Clock timestamp values MUST be returned via 'CLK_TS_SEC' data type.
 *              - (a) If the External timestamp has more bits than the 'CLK_TS_SEC' data type,
 *                   Clk_ExtTS_Get() MUST truncate the External timestamp's higher order bits
 *                   greater than the 'CLK_TS_SEC' data type.
 *           - (b) If the External timestamp has less bits than the 'CLK_TS_SEC' data type,
 *                   Clk_ExtTS_Get() MUST pad the Clock timestamp's higher order bits with 0 bits.
 *
 * @note     (3) External timestamp values MUST be returned from the reference of the Clock
 *               epoch start date/time.
 *               @n
 *               External timestamp SHOULD start on midnight of January 1st of its epoch start
 *               year. Otherwise, the equations to convert between External timestamp & Clock
 *               timestamp MUST also include the External timestamp's epoch Day-of-Year, Hour,
 *               Minute, & Second (see Note #4).
 *               @n
 *               Returned Clock timestamp MUST be representable in Clock epoch. Thus:
 *               YR_START >= Date of external timestamp < CLK_EPOCH_YR_END.
 *               @n
 *               If the External timestamp includes an (optional) external time zone,
 *               Clk_ExtTS_Get() MUST subtract the external time zone offset from the converted
 *               External timestamp.
 *
 * @note     (4) The Clock timestamp is calculated by one of the following equations :
 *           - (a) When External epoch start year is less than Clock epoch start year
 *                   ('CLK_EPOCH_YR_START') :
 *                   @verbatim
 *                   Clock TS = External TS
 *                           - [(((Clock start year - External start year) * 365) + leap day count)
 *                               * seconds per day]
 *                           - External TZ
 *                   @endverbatim
 *                   Examples with a 32-bit External timestamp :
 *               - (1)   Valid equivalent date to convert is after  Clock epoch start year :
 *                       @verbatim
 *                       2010 Oct 8, 11:11:11 UTC-05:00
 *                       External TS (in seconds)                                 = 1286536271
 *                       External start year                                      =       1970
 *                       Clock    start year                                      =       2000
 *                       Leap day count between External & Clock epoch start year =          7
 *                       External TZ (in seconds)                                 =     -18000
 *                       Clock    TS (in seconds)                                 =  339869471
 *                       2010 Oct 8, 16:11:11 UTC
 *                       @endverbatim
 *                       This example successfully converts an External timestamp into a representable
 *                       Clock timestamp without underflowing.
 *                       @n
 *               - (2) Invalid equivalent date to convert is before Clock epoch start year :
 *                       @verbatim
 *                       1984 Oct 8, 11:11:11 UTC-05:00
 *                       External TS (in seconds)                                 =  466081871
 *                       External start year                                      =       1970
 *                       Clock    start year                                      =       2000
 *                       Leap day count between External & Clock epoch start year =          7
 *                       External TZ (in seconds)                                 =     -18000
 *                       Clock    TS (in seconds)                                 = -480584929
 *                       @endverbatim
 *                       This example underflows to a negative Clock timestamp since the equivalent
 *                       date to convert is incorrectly less than the Clock epoch start year
 *                       ('CLK_EPOCH_YR_START').
 *           - (b) When External epoch start year is greater than Clock epoch start year
 *                   ('CLK_EPOCH_YR_START') :
 *                   @verbatim
 *                   Clock TS = External TS
 *                           + [(((External start year - Clock start year) * 365) + leap day count)
 *                               * seconds per day]
 *                           - External TZ
 *                   @endverbatim
 *                   Examples with a 32-bit External timestamp :
 *               - (1)   Valid equivalent date to convert is before Clock epoch end year :
 *                       @verbatim
 *                       2010 Oct 8, 11:11:11 UTC-05:00
 *                       External TS (in seconds)                                 =   24232271
 *                       External start year                                      =       2010
 *                       Clock    end   year                                      =       2136
 *                       Leap day count between External & Clock epoch start year =          3
 *                       External TZ (in seconds)                                 =     -18000
 *                       Clock    TS (in seconds)                                 =  339869471
 *                       2010 Oct 8, 16:11:11 UTC-05:00
 *                       @endverbatim
 *                       This example successfully converts an External timestamp into a representable
 *                       Clock timestamp without overflowing.
 *
 *               - (2) Invalid equivalent date to convert is after  Clock epoch end year :
 *                       @verbatim
 *                       2140 Oct 8, 11:11:11 UTC-05:00
 *                       External TS (in seconds)                                 = 4126677071
 *                       External start year                                      =       2010
 *                       Clock    end   year                                      =       2136
 *                       Leap day count between External & Clock epoch start year =          3
 *                       External TZ (in seconds)                                 =     -18000
 *                       Clock    TS (in seconds)                                 = 4442314271
 *                       @endverbatim
 *                       This example overflows the Clock timestamp (32-bit) 'CLK_TS_SEC' data type
 *                       with an equivalent date incorrectly greater than or equal to the Clock epoch
 *                       end year ('CLK_EPOCH_YR_END').
 *           - (c) Where
 *               - (1) Clock    TS             Converted Clock timestamp (in seconds, from UTC+00)
 *               - (2) External TS             External timestamp to convert (in seconds)
 *               - (3) Clock    start year     Clock epoch start year ('CLK_EPOCH_YR_START')
 *               - (4) Clock    end   year     Clock epoch end   year ('CLK_EPOCH_YR_END')
 *               - (5) External start year     External timestamp epoch start year
 *               - (6) Leap day count          Number of leap days between Clock epoch start year &
 *                                                   External epoch start year
 *               - (7) Seconds per day         Number of seconds per day (86400)
 *               - (8) External TZ             Time zone offset applied to External TS (in seconds,
 *                                                   from UTC+00)
 *
 * @note     (5) This function is an INTERNAL Clock module function & MUST be implemented by
 *               application/BSP function(s) [see Note #1] but MUST NOT be called by application
 *               function(s).
 *******************************************************************************************************/

#if (CLK_CFG_EXT_EN == DEF_ENABLED)
CLK_TS_SEC Clk_ExtTS_Get(void);
#endif

/****************************************************************************************************//**
 *                                           Clk_ExtTS_Set()
 *
 * @brief          Set External timestamp.
 *
 * @param          ts_sec      Timestamp value to set (in seconds, UTC+00).
 *
 * @return        DEF_OK,     if External timestamp succesfully set.
 *               DEF_FAIL,   otherwise.
 *
 * @note     (1) CLK_ExtTS_Set() is an application/BSP function that MUST be defined by the developer
 *                   if External timestamp is enabled.
 *                       See 'common_cfg.h  CLK CONFIGURATION  Note #1'.
 *                   - (a) If External timestamp is provided by another application, it's possible that the
 *                       External timestamp may NOT be set (e.g. by SNTPc) in which case CLK_ExtTS_Set()
 *                       MUST ALWAYS return 'DEF_FAIL'.
 *
 * @note     (2) External timestamp values are converted from Clock timestamp's 'CLK_TS_SEC' data
 *                   type.
 *                   - (a) If the External timestamp has more bits than the 'CLK_TS_SEC' data type,
 *                       Clk_ExtTS_Set() MUST pad the External timestamp's higher order bits with 0
 *                       bits.
 *                   - (b) If the External timestamp has less bits than the 'CLK_TS_SEC' data type,
 *                       Clk_ExtTS_Set() MUST truncate the Clock timestamp's higher order bits greater
 *                       than the External timestamp.
 *
 * @note     (3) External timestamp values MUST be converted from the reference of the Clock epoch
 *                   start date/time.
 *                   If the External timestamp includes an (optional) external time zone,
 *                   Clk_ExtTS_Set() MUST add the external time zone offset to the converted External
 *                   timestamp.
 *                   @n
 *                   External timestamp SHOULD start on midnight of January 1st of its epoch start
 *                   year. Otherwise, the equations to convert between External timestamp & Clock
 *                   timestamp MUST also include the External timestamp's epoch Day-of-Year, Hour,
 *                   Minute, & Second (see Note #4).
 *                   @n
 *                   Converted External timestamp MUST be representable in External epoch. Thus
 *                   equivalent date of the External timestamp MUST be between :
 *                       - (a) External epoch start year
 *                       - (b) External epoch end   year
 *
 * @note     (4) The External timestamp is calculated by one of the following equations :
 *                   - (a) When External epoch start year is less than Clock epoch start year
 *                       ('CLK_EPOCH_YR_START') :
 *                       @verbatim
 *                       External TS = Clock TS
 *                                   + [(((Clock start year - External start year) * 365) + leap day count)
 *                                       * seconds per day]
 *                                   + External TZ
 *                       @endverbatim
 *                       Examples with a 32-bit External timestamp :
 *                       - (1)   Valid equivalent date to convert is before External epoch end year :
 *                           @verbatim
 *                               2010 Oct 8, 16:11:11 UTC
 *                           Clock    TS (in seconds)                                 =  339869471
 *                           External start year                                      =       1970
 *                           External end   year                                      =       2106
 *                           Leap day count between External & Clock epoch start year =          7
 *                           External TZ (in seconds)                                 =     -18000
 *                           External TS (in seconds)                                 = 1286536271
 *                               2010 Oct 8, 11:11:11 UTC-05:00
 *                           @endverbatim
 *                           This example successfully converts an External timestamp into a representable
 *                           Clock timestamp without overflowing.
 *                       - (2) Invalid equivalent date to convert is after  External epoch end year :
 *                           @verbatim
 *                               2120 Oct 8, 11:11:11 UTC
 *                           Clock    TS (in seconds)                                 = 3811144271
 *                           External start year                                      =       1970
 *                           External end   year                                      =       2106
 *                           Leap day count between External & Clock epoch start year =          7
 *                           External TZ (in seconds)                                 =     -18000
 *                           External TS (in seconds)                                 = 4757811071
 *                           @endverbatim
 *                           This example overflows the External (32-bit) timestamp with an equivalent
 *                           date incorrectly greater than or equal to the External epoch end year.
 *                   - (b) When External epoch start year is greater than Clock epoch start year
 *                       ('CLK_EPOCH_YR_START') :
 *                       @verbatim
 *                       External TS = Clock TS
 *                                   - [(((External start year - Clock start year) * 365) + leap day count)
 *                                       * seconds per day]
 *                                   + External TZ
 *                       @endverbatim
 *                       Examples with a 32-bit External timestamp :
 *                       - (1)   Valid equivalent date to convert is after  External epoch start year :
 *                           @verbatim
 *                               2010 Oct 8, 16:11:11 UTC
 *                           Clock    TS (in seconds)                                 =  339869471
 *                           External start year                                      =       2010
 *                           Leap day count between External & Clock epoch start year =          3
 *                           External TZ (in seconds)                                 =     -18000
 *                           External TS (in seconds)                                 =   24232271
 *                               2010 Oct 8, 11:11:11 UTC-05:00
 *                           @endverbatim
 *                           This example successfully converts an External timestamp into a representable
 *                           Clock timestamp without underflowing.
 *                       - (2) Invalid equivalent date to convert is before External epoch start year :
 *                           @verbatim
 *                               2005 Oct 8, 11:11:11 UTC
 *                           Clock    TS (in seconds)                                 =  182085071
 *                           External start year                                      =       2010
 *                           Leap day count between External & Clock epoch start year =          3
 *                           External TZ (in seconds)                                 =     -18000
 *                           External TS (in seconds)                                 = -133552129
 *                           @endverbatim
 *                           This example underflows to a negative External timestamp since the equivalent
 *                           date to convert is incorrectly less than the External epoch start year.
 *                   - (c) where
 *                       - (1) Clock    TS             Clock timestamp (in seconds, from UTC+00)
 *                       - (2) External TS             Converted External timestamp (in seconds)
 *                       - (3) Clock    start year     Clock epoch start year ('CLK_EPOCH_YR_START')
 *                       - (4) External start year     External timestamp epoch start year
 *                       - (5) External end   year     External timestamp epoch end   year
 *                       - (6) Leap day count          Number of leap days between Clock epoch start year &
 *                                                   External epoch start year
 *                       - (7) Seconds per day         Number of seconds per day (86400)
 *                       - (8) External TZ             Time zone offset applied to External TS (in seconds,
 *                                                   from UTC+00)
 *
 * @note     (5) This function is an INTERNAL Clock module function & MUST be implemented by
 *                   application/BSP function(s) [see Note #1] but MUST NOT be called by application
 *                   function(s).
 *******************************************************************************************************/

#if (CLK_CFG_EXT_EN == DEF_ENABLED)
CPU_BOOLEAN Clk_ExtTS_Set(CLK_TS_SEC ts_sec);
#endif

#ifdef __cplusplus
}
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                           CONFIGURATION ERRORS
 ********************************************************************************************************
 *******************************************************************************************************/

#ifndef  CLK_CFG_STR_CONV_EN
#error  "CLK_CFG_STR_CONV_EN not #define'd in 'common_cfg.h'. [MUST be DEF_DISABLED || DEF_ENABLED]"
#elif  ((CLK_CFG_STR_CONV_EN != DEF_ENABLED) \
  && (CLK_CFG_STR_CONV_EN != DEF_DISABLED))
#error  "CLK_CFG_STR_CONV_EN illegally #define'd in 'common_cfg.h'. [MUST be DEF_DISABLED || DEF_ENABLED]"
#endif

#ifndef  CLK_CFG_NTP_EN
#error  "CLK_CFG_NTP_EN not #define'd in 'common_cfg.h'. [MUST be DEF_DISABLED || DEF_ENABLED]"
#elif  ((CLK_CFG_NTP_EN != DEF_ENABLED) \
  && (CLK_CFG_NTP_EN != DEF_DISABLED))
#error  "CLK_CFG_NTP_EN illegally #define'd in 'common_cfg.h'. [MUST be DEF_DISABLED || DEF_ENABLED]"
#endif

#ifndef  CLK_CFG_UNIX_EN
#error  "CLK_CFG_UNIX_EN not #define'd in 'common_cfg.h'. [MUST be DEF_DISABLED || DEF_ENABLED]"
#elif  ((CLK_CFG_UNIX_EN != DEF_ENABLED) \
  && (CLK_CFG_UNIX_EN != DEF_DISABLED))
#error  "CLK_CFG_UNIX_EN illegally #define'd in 'common_cfg.h'. [MUST be DEF_DISABLED || DEF_ENABLED]"
#endif

#ifndef  CLK_CFG_EXT_EN
#error  "CLK_CFG_EXT_EN not #define'd in 'common_cfg.h'. [MUST be DEF_DISABLED || DEF_ENABLED]"

#elif  ((CLK_CFG_EXT_EN != DEF_ENABLED) \
  && (CLK_CFG_EXT_EN != DEF_DISABLED))
#error  "CLK_CFG_EXT_EN illegally #define'd in 'common_cfg.h'. [MUST be DEF_DISABLED || DEF_ENABLED]"

#elif   (CLK_CFG_EXT_EN != DEF_ENABLED)

#ifndef  CLK_CFG_SIGNAL_EN
#error  "CLK_CFG_SIGNAL_EN not #define'd in 'common_cfg.h'. [MUST be DEF_DISABLED || DEF_ENABLED]"

#elif  ((CLK_CFG_SIGNAL_EN != DEF_ENABLED) \
  && (CLK_CFG_SIGNAL_EN != DEF_DISABLED))
#error  "CLK_CFG_SIGNAL_EN illegally #define'd in 'common_cfg.h'. [MUST be DEF_DISABLED || DEF_ENABLED]"

#elif   (CLK_CFG_SIGNAL_EN == DEF_ENABLED)

#ifndef  CLK_CFG_SIGNAL_FREQ_HZ
#error  "CLK_CFG_SIGNAL_FREQ_HZ not #define'd in 'common_cfg.h'. [MUST be > 0]"
#elif   (CLK_CFG_SIGNAL_FREQ_HZ < 1)
#error  "CLK_CFG_SIGNAL_FREQ_HZ illegally #define'd in 'common_cfg.h'. [MUST be > 0]"
#endif

#endif

#endif

/****************************************************************************************************//**
 ********************************************************************************************************
 * @}                                          MODULE END
 ********************************************************************************************************
 *******************************************************************************************************/

#endif // End of clk module include.
