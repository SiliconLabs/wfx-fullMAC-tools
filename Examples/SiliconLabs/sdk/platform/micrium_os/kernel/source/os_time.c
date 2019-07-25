/***************************************************************************//**
 * @file
 * @brief Kernel - Time Management
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
 *                                       DEPENDENCIES & AVAIL CHECK(S)
 ********************************************************************************************************
 *******************************************************************************************************/

#include  <rtos_description.h>

#if (defined(RTOS_MODULE_KERNEL_AVAIL))

/********************************************************************************************************
 ********************************************************************************************************
 *                                               INCLUDE FILES
 ********************************************************************************************************
 *******************************************************************************************************/

#define  MICRIUM_SOURCE
#include "../include/os.h"
#include "os_priv.h"

#ifdef VSC_INCLUDE_SOURCE_FILE_NAMES
const CPU_CHAR *os_time__c = "$Id: $";
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                           GLOBAL FUNCTIONS
 ********************************************************************************************************
 *******************************************************************************************************/

/****************************************************************************************************//**
 *                                               OSTimeDly()
 *
 * @brief    Delays the execution of the currently running task until the specified number of system
 *           ticks expires. This directly equates to delaying the current task for some time to
 *           expire. No delay will result if the specified delay is 0. If the specified delay is
 *           greater than 0, this results in a context switch.
 *
 * @param    dly     Value in 'clock ticks' that the task for which will either delay, the target
 *                   matches the value of the tick counter (OSTickCtr). Note that setting this to 0
 *                   means that no delay will be applied to the task.
 *                   Depending on the option argument, the task will wake up when OSTickCtr reaches:
 *                       - OS_OPT_TIME_DLY         OSTickCtr + dly
 *                       - OS_OPT_TIME_TIMEOUT     OSTickCtr + dly
 *                       - OS_OPT_TIME_MATCH       dly
 *                       - OS_OPT_TIME_PERIODIC    OSTCBCurPtr.TickCtrPrev + dly
 *
 * @param    opt     Specifies whether 'dly' represents absolute or relative time; default option
 *                   is OS_OPT_TIME_DLY:
 *                       - OS_OPT_TIME_DLY         Specifies a relative time from the current value of
 *                                                 OSTickCtr.
 *                       - OS_OPT_TIME_TIMEOUT     Same as OS_OPT_TIME_DLY.
 *                       - OS_OPT_TIME_MATCH       Indicates that 'dly' specifies the absolute value
 *                                                 that OSTickCtr must reach before the task will be
 *                                                 resumed.
 *                       - OS_OPT_TIME_PERIODIC    Indicates that 'dly' specifies the periodic value
 *                                                 that OSTickCtr must reach before the task will be
 *                                                 resumed.
 *
 * @param    p_err   Pointer to the variable that will receive one of the following error code(s)
 *                   from this function:
 *                       - RTOS_ERR_NONE
 *                       - RTOS_ERR_INVALID_ARG
 *                       - RTOS_ERR_OS_SCHED_LOCKED
 *******************************************************************************************************/
void OSTimeDly(OS_TICK  dly,
               OS_OPT   opt,
               RTOS_ERR *p_err)
{
#if (OS_CFG_TASK_TICK_EN == DEF_ENABLED)
  CPU_SR_ALLOC();
#endif

  OS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err,; );

  //                                                               Not allowed to call from an ISR
  OS_ASSERT_DBG_ERR_SET((OSIntNestingCtr == 0u), *p_err, RTOS_ERR_ISR,; );

  //                                                               Make sure kernel is running.
  OS_ASSERT_DBG_ERR_SET((OSRunning == OS_STATE_OS_RUNNING), *p_err, RTOS_ERR_NOT_READY,; );

  if (OSSchedLockNestingCtr > 0u) {                             // Can't delay when the scheduler is locked
    RTOS_ERR_SET(*p_err, RTOS_ERR_OS_SCHED_LOCKED);
    return;
  }

  //                                                               Validate 'opt'
  OS_ASSERT_DBG_ERR_SET(((opt == OS_OPT_TIME_DLY)
                         || (opt == OS_OPT_TIME_TIMEOUT)
                         || (opt == OS_OPT_TIME_PERIODIC)
                         || (opt == OS_OPT_TIME_MATCH)), *p_err, RTOS_ERR_INVALID_ARG,; );

  //                                                               Make sure we didn't specify a 0 delay
  OS_ASSERT_DBG_ERR_SET(((opt == OS_OPT_TIME_MATCH)
                         || (dly != 0u)), *p_err, RTOS_ERR_INVALID_ARG,; );

#if (OS_CFG_TASK_TICK_EN == DEF_ENABLED)
  CPU_CRITICAL_ENTER();
  OS_TickListInsertDly(OSTCBCurPtr,
                       dly,
                       opt,
                       p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    CPU_CRITICAL_EXIT();
    return;
  }

  OS_TRACE_TASK_DLY(dly);
  OS_RdyListRemove(OSTCBCurPtr);                                // Remove current task from ready list
  CPU_CRITICAL_EXIT();
  OSSched();                                                    // Find next task to run!
#else
  (void)dly;                                                    // Prevent compiler warning for not using 'dly'
  (void)opt;                                                    // Prevent compiler warning for not using 'opt'
#endif
}

/****************************************************************************************************//**
 *                                               OSTimeDlyHMSM()
 *
 * @brief    Delay execution of the currently running task until some time expires. This call allows
 *           you to specify the delay time in HOURS, MINUTES, SECONDS, and MILLISECONDS instead of ticks.
 *
 * @param    hours       Specifies the number of hours that the task will be delayed (max. is 999 if the
 *                       tick rate is 1000 Hz or less otherwise, a higher value would overflow a 32-bit
 *                       unsigned counter). (max. 99 if 'opt' is OS_OPT_TIME_HMSM_STRICT)
 *
 * @param    minutes     Specifies the number of minutes. (max. 59 if 'opt' is OS_OPT_TIME_HMSM_STRICT)
 *
 * @param    seconds     Specifies the number of seconds. (max. 59 if 'opt' is OS_OPT_TIME_HMSM_STRICT)
 *
 * @param    milli       Specifies the number of milliseconds. (max. 999 if 'opt' is OS_OPT_TIME_HMSM_STRICT)
 *
 * @param    opt         Specifies time delay bit-field options logically OR'd; default options marked
 *                       with *** :
 *                           - OS_OPT_TIME_DLY         *** Specifies a relative time from the current
 *                                                   value of OSTickCtr.
 *                           - OS_OPT_TIME_TIMEOUT     Same as OS_OPT_TIME_DLY.
 *                           - OS_OPT_TIME_MATCH       Indicates that the delay specifies the
 *                                                     absolute value that OSTickCtr must reach
 *                                                     before the task will be resumed.
 *                           - OS_OPT_TIME_PERIODIC    Indicates that the delay specifies the
 *                                                     periodic value that OSTickCtr must reach
 *                                                     before the task will be resumed.
 *                           - OS_OPT_TIME_HMSM_STRICT    *** Strictly allows only hours        (0...99)
 *                                                                                 minutes      (0...59)
 *                                                                                 seconds      (0...59)
 *                                                                                 milliseconds (0...999)
 *                           - OS_OPT_TIME_HMSM_NON_STRICT    Allows any value of  hours        (0...999)
 *                                                                                 minutes      (0...9999)
 *                                                                                 seconds      (0...65535)
 *                                                                                 milliseconds (0...4294967295)
 *
 * @param    p_err       Pointer to the variable that will receive one of the following error code(s)
 *                       from this function:
 *                           - RTOS_ERR_NONE
 *                           - RTOS_ERR_INVALID_ARG
 *                           - RTOS_ERR_OS_SCHED_LOCKED
 *
 * @note     (1) The resolution of milliseconds depends on the tick rate. For example, you cannot
 *                   do a 10 mS delay if the ticker interrupts every 100 mS. In this case, the delay would
 *                   be set to 0. The actual delay is rounded to the nearest tick.
 *
 * @note     (2) Although this function allows you to delay a task for many hours, it is not recommended
 *                   to put a task to sleep for that long.
 *******************************************************************************************************/
void OSTimeDlyHMSM(CPU_INT16U hours,
                   CPU_INT16U minutes,
                   CPU_INT16U seconds,
                   CPU_INT32U milli,
                   OS_OPT     opt,
                   RTOS_ERR   *p_err)
{
  OS_OPT opt_time;
#if (OS_CFG_TASK_TICK_EN == DEF_ENABLED)
  OS_RATE_HZ tick_rate;
  OS_TICK    ticks;
  CPU_SR_ALLOC();
#endif

  OS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err,; );

  //                                                               Not allowed to call from an ISR
  OS_ASSERT_DBG_ERR_SET((OSIntNestingCtr == 0u), *p_err, RTOS_ERR_ISR,; );

  //                                                               Make sure kernel is running.
  OS_ASSERT_DBG_ERR_SET((OSRunning == OS_STATE_OS_RUNNING), *p_err, RTOS_ERR_NOT_READY,; );

  if (OSSchedLockNestingCtr > 0u) {                             // Can't delay when the scheduler is locked
    RTOS_ERR_SET(*p_err, RTOS_ERR_OS_SCHED_LOCKED);
    return;
  }

  opt_time = opt & OS_OPT_TIME_MASK;                            // Retrieve time options only.

  //                                                               Validate 'opt'
  OS_ASSERT_DBG_ERR_SET(((opt_time == OS_OPT_TIME_DLY)
                         || (opt_time == OS_OPT_TIME_TIMEOUT)
                         || (opt_time == OS_OPT_TIME_PERIODIC)
                         || (opt_time == OS_OPT_TIME_MATCH)), *p_err, RTOS_ERR_INVALID_ARG,; );

  //                                                               Make sure we didn't specify a 0 delay
  OS_ASSERT_DBG_ERR_SET(((opt_time == OS_OPT_TIME_MATCH)
                         || ((milli != 0u)
                             || (seconds != 0u)
                             || (minutes != 0u)
                             || (hours != 0u))), *p_err, RTOS_ERR_INVALID_ARG,; );

  OS_ASSERT_DBG_ERR_SET((DEF_BIT_IS_SET_ANY(opt, ~OS_OPT_TIME_OPTS_MASK) != DEF_YES), *p_err, RTOS_ERR_INVALID_ARG,; );

  if (DEF_BIT_IS_SET(opt, OS_OPT_TIME_HMSM_NON_STRICT) != DEF_YES) {
    OS_ASSERT_DBG_ERR_SET((milli <= 999u), *p_err, RTOS_ERR_INVALID_ARG,; );
    OS_ASSERT_DBG_ERR_SET((seconds <= 59u), *p_err, RTOS_ERR_INVALID_ARG,; );
    OS_ASSERT_DBG_ERR_SET((minutes <= 59u), *p_err, RTOS_ERR_INVALID_ARG,; );
    OS_ASSERT_DBG_ERR_SET((hours <= 99u), *p_err, RTOS_ERR_INVALID_ARG,; );
  } else {
    OS_ASSERT_DBG_ERR_SET((minutes <= 9999u), *p_err, RTOS_ERR_INVALID_ARG,; );
    OS_ASSERT_DBG_ERR_SET((hours <= 999u), *p_err, RTOS_ERR_INVALID_ARG,; );
  }

  //                                                               Compute the total number of clock ticks required..
  //                                                               .. (rounded to the nearest tick)
#if (OS_CFG_TASK_TICK_EN == DEF_ENABLED)
  tick_rate = OSCfg_TickRate_Hz;
  ticks = ((OS_TICK)hours * (OS_TICK)3600u + (OS_TICK)minutes * (OS_TICK)60u + (OS_TICK)seconds) * tick_rate
          + (tick_rate * ((OS_TICK)milli + (OS_TICK)500u / tick_rate)) / (OS_TICK)1000u;

  if (ticks > 0u) {
    CPU_CRITICAL_ENTER();
    OS_TickListInsertDly(OSTCBCurPtr,
                         ticks,
                         opt_time,
                         p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
      CPU_CRITICAL_EXIT();
      return;
    }
    OS_TRACE_TASK_DLY(ticks);
    OS_RdyListRemove(OSTCBCurPtr);                              // Remove current task from ready list
    CPU_CRITICAL_EXIT();
    OSSched();                                                  // Find next task to run!
    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
  } else {
    RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_ARG);
  }
#else
  (void)hours;                                                  // Prevent compiler warning for not using 'hours'
  (void)minutes;                                                // Prevent compiler warning for not using 'minutes'
  (void)seconds;                                                // Prevent compiler warning for not using 'seconds'
  (void)milli;                                                  // Prevent compiler warning for not using 'milli'
  (void)opt;                                                    // Prevent compiler warning for not using 'opt'
#endif
}

/****************************************************************************************************//**
 *                                               OSTimeDlyResume()
 *
 * @brief    Resumes a task that has been delayed through a call to either OSTimeDly() or
 *           OSTimeDlyHMSM(). Note that you cannot call this function to resume a task that is waiting
 *           for an event with timeout.
 *
 * @param    p_tcb   Pointer to the TCB of the task to resume.
 *
 * @param    p_err   Pointer to the variable that will receive one of the following error code(s)
 *                   from this function:
 *                       - RTOS_ERR_NONE
 *                       - RTOS_ERR_OS_TASK_SUSPENDED
 *                       - RTOS_ERR_INVALID_STATE
 *******************************************************************************************************/
void OSTimeDlyResume(OS_TCB   *p_tcb,
                     RTOS_ERR *p_err)
{
  CPU_SR_ALLOC();

  OS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err,; );

  //                                                               Not allowed to call from an ISR
  OS_ASSERT_DBG_ERR_SET((OSIntNestingCtr == 0u), *p_err, RTOS_ERR_ISR,; );

  //                                                               User must supply a valid OS_TCB
  OS_ASSERT_DBG_ERR_SET((p_tcb != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR,; );

  //                                                               Make sure kernel is running.
  OS_ASSERT_DBG_ERR_SET((OSRunning == OS_STATE_OS_RUNNING), *p_err, RTOS_ERR_NOT_READY,; );

  CPU_CRITICAL_ENTER();
  switch (p_tcb->TaskState) {
    case OS_TASK_STATE_RDY:                                     // Cannot Abort delay if task is ready
    case OS_TASK_STATE_PEND:
    case OS_TASK_STATE_PEND_TIMEOUT:
    case OS_TASK_STATE_SUSPENDED:
    case OS_TASK_STATE_PEND_SUSPENDED:
    case OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED:
      CPU_CRITICAL_EXIT();
      RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);
      break;

    case OS_TASK_STATE_DLY:
      p_tcb->TaskState = OS_TASK_STATE_RDY;
#if (OS_CFG_TASK_TICK_EN == DEF_ENABLED)
      OS_TickListRemove(p_tcb);                                 // Remove task from tick list
      OS_RdyListInsert(p_tcb);                                  // Add to ready list
#endif
      CPU_CRITICAL_EXIT();
      RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
      break;

    case OS_TASK_STATE_DLY_SUSPENDED:
      p_tcb->TaskState = OS_TASK_STATE_SUSPENDED;
#if (OS_CFG_TASK_TICK_EN == DEF_ENABLED)
      OS_TickListRemove(p_tcb);                                 // Remove task from tick list
#endif
      CPU_CRITICAL_EXIT();
      RTOS_ERR_SET(*p_err, RTOS_ERR_OS_TASK_SUSPENDED);
      break;

    case OS_TASK_STATE_DEL:
#if (OS_CFG_TASK_DEL_EN == DEF_ENABLED)
      CPU_CRITICAL_EXIT();
      RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);
      break;
#endif

    default:
      CPU_CRITICAL_EXIT();
      RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_OS,; );
  }

  OSSched();
}

/****************************************************************************************************//**
 *                                               OSTimeGet()
 *
 * @brief    Used by your application to obtain the current value of the counter to keep track of
 *           the number of clock ticks.
 *
 * @param    p_err   Pointer to the variable that will receive one of the following error code(s)
 *                   from this function:
 *                       - RTOS_ERR_NONE
 *
 * @return   The current value of OSTickCtr.
 *******************************************************************************************************/
OS_TICK OSTimeGet(RTOS_ERR *p_err)
{
  OS_TICK ticks;
#if (OS_CFG_TASK_TICK_EN == DEF_ENABLED)
  CPU_SR_ALLOC();
#endif

  OS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, 0u);

#if (OS_CFG_TASK_TICK_EN == DEF_ENABLED)
  CPU_CRITICAL_ENTER();
#if (OS_CFG_DYN_TICK_EN == DEF_ENABLED)
  if (OSRunning == OS_STATE_OS_RUNNING) {
    ticks = BSP_OS_TickGet();
  } else {
    ticks = OSTickCtr;
  }
#else
  ticks = OSTickCtr;
#endif
  CPU_CRITICAL_EXIT();
#else
  ticks = 0u;
#endif

  RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
  return (ticks);
}

/****************************************************************************************************//**
 *                                               OSTimeSet()
 *
 * @brief    Sets the counter that keeps track of the number of clock ticks.
 *
 * @param    ticks   The desired tick value.
 *
 * @param    p_err   Pointer to the variable that will receive one of the following error code(s)
 *                   from this function:
 *                       - RTOS_ERR_NONE
 *
 * @note     (1) This function is deprecated and will be removed in an upcoming release.
 *******************************************************************************************************/
void OSTimeSet(OS_TICK  ticks,
               RTOS_ERR *p_err)
{
#if (OS_CFG_TASK_TICK_EN == DEF_ENABLED)
  CPU_SR_ALLOC();
#endif

  OS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err,; );

#if (OS_CFG_TASK_TICK_EN == DEF_ENABLED)
  CPU_CRITICAL_ENTER();
  OSTickCtr = ticks;
  CPU_CRITICAL_EXIT();
#else
  (void)ticks;                                                  // Prevent compiler warning for not using 'ticks'
#endif

  RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
}

/****************************************************************************************************//**
 *                                               OSTimeTick()
 *
 * @brief    Signal the Kernel an occurrence of a 'system tick' (also known as a 'clock tick').
 *           This function should be called by the tick ISR.
 *******************************************************************************************************/
void OSTimeTick(void)
{
#if (OS_CFG_TASK_TICK_EN == DEF_ENABLED)
  RTOS_ERR err;
#endif

  if (OSRunning != OS_STATE_OS_RUNNING) {
    return;
  }

  OSTimeTickHook();                                             // Call user definable hook

#if (OS_CFG_TASK_TICK_EN == DEF_ENABLED)

  (void)OSTaskSemPost(&OSTickTaskTCB,                           // Signal tick task
                      OS_OPT_POST_NONE,
                      &err);

  (void)err;

#if (OS_CFG_SCHED_ROUND_ROBIN_EN == DEF_ENABLED)
  OS_SchedRoundRobin(&OSRdyList[OSPrioCur]);
#endif

#if (OS_CFG_TMR_EN == DEF_ENABLED)
  OSTmrUpdateCtr--;
  if (OSTmrUpdateCtr == 0u) {
    OSTmrUpdateCtr = OSTmrUpdateCnt;
    (void)OSTaskSemPost(&OSTmrTaskTCB,                          // Signal timer task
                        OS_OPT_POST_NONE,
                        &err);
    (void)err;
  }
#endif
#endif
}

/****************************************************************************************************//**
 *                                               OSTimeDynTick()
 *
 * @brief    Signal the Kernel an occurrence of a 'system tick' (also known as a 'clock tick').
 *           This function should be called by the dynamic tick ISR.
 *
 * @param    ticks   Number of OS Ticks to process.
 *******************************************************************************************************/

#if (OS_CFG_TASK_TICK_EN == DEF_ENABLED)
#if (OS_CFG_DYN_TICK_EN == DEF_ENABLED)
void OSTimeDynTick(OS_TICK ticks)
{
  RTOS_ERR err;
  CPU_SR_ALLOC();

  if (OSRunning != OS_STATE_OS_RUNNING) {
    return;
  }

  OSTimeTickHook();

  CPU_CRITICAL_ENTER();
  OSTickCtrPend += ticks;
  CPU_CRITICAL_EXIT();

  (void)OSTaskSemPost(&OSTickTaskTCB,                           // Signal tick task
                      OS_OPT_POST_NONE,
                      &err);
}
#endif
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                   DEPENDENCIES & AVAIL CHECK(S) END
 ********************************************************************************************************
 *******************************************************************************************************/

#endif // (defined(RTOS_MODULE_KERNEL_AVAIL))
