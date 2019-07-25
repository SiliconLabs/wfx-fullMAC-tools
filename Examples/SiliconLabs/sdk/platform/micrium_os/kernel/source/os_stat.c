/***************************************************************************//**
 * @file
 * @brief Kernel - Statistics Module
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
const CPU_CHAR *os_stat__c = "$Id: $";
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                           GLOBAL FUNCTIONS
 ********************************************************************************************************
 *******************************************************************************************************/

#if (OS_CFG_STAT_TASK_EN == DEF_ENABLED)
/****************************************************************************************************//**
 *                                               OSStatReset()
 *
 * @brief    Called by your application to reset the statistics.
 *
 * @param    p_err   Pointer to the variable that will receive one of the following error code(s)
 *                   from this function:
 *                       - RTOS_ERR_NONE
 *******************************************************************************************************/
void OSStatReset(RTOS_ERR *p_err)
{
#if (OS_CFG_DBG_EN == DEF_ENABLED)
  OS_TCB *p_tcb;
#if (OS_MSG_EN == DEF_ENABLED)
  OS_MSG_Q *p_msg_q;
#endif
#if (OS_CFG_Q_EN == DEF_ENABLED)
  OS_Q *p_q;
#endif
#endif
  CPU_SR_ALLOC();

  OS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err,; );

  CPU_CRITICAL_ENTER();
#if (OS_CFG_STAT_TASK_EN == DEF_ENABLED)
  OSStatTaskCPUUsageMax = 0u;
#if (OS_CFG_TS_EN == DEF_ENABLED)
  OSStatTaskTimeMax = 0u;
#endif
#endif

#if (OS_CFG_TASK_TICK_EN == DEF_ENABLED)
#if (OS_CFG_TS_EN == DEF_ENABLED)
  OSTickTaskTimeMax = 0u;
#endif
#endif

#if (OS_CFG_TMR_EN == DEF_ENABLED)
#if (OS_CFG_TS_EN == DEF_ENABLED)
  OSTmrTaskTimeMax = 0u;
#endif
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
  OSIntDisTimeMax = 0u;                                         // Reset the maximum interrupt disable time
#endif

#if (OS_CFG_SCHED_LOCK_TIME_MEAS_EN == DEF_ENABLED)
  OSSchedLockTimeMax = 0u;                                      // Reset the maximum scheduler lock time
#endif

#if ((OS_MSG_EN == DEF_ENABLED) && (OS_CFG_DBG_EN == DEF_ENABLED))
  OSMsgPool.NbrUsedMax = 0u;
#endif
  CPU_CRITICAL_EXIT();

#if (OS_CFG_DBG_EN == DEF_ENABLED)
  CPU_CRITICAL_ENTER();
  p_tcb = OSTaskDbgListPtr;
  CPU_CRITICAL_EXIT();
  while (p_tcb != DEF_NULL) {                                   // Reset per-Task statistics
    CPU_CRITICAL_ENTER();

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    p_tcb->IntDisTimeMax = 0u;
#endif

#if (OS_CFG_SCHED_LOCK_TIME_MEAS_EN == DEF_ENABLED)
    p_tcb->SchedLockTimeMax = 0u;
#endif

#if (OS_CFG_TASK_PROFILE_EN == DEF_ENABLED)
#if (OS_CFG_TASK_Q_EN == DEF_ENABLED)
    p_tcb->MsgQPendTimeMax = 0u;
#endif
    p_tcb->SemPendTimeMax = 0u;
    p_tcb->CtxSwCtr = 0u;
    p_tcb->CPUUsage = 0u;
    p_tcb->CPUUsageMax = 0u;
    p_tcb->CyclesTotal = 0u;
    p_tcb->CyclesTotalPrev = 0u;
#if (OS_CFG_TS_EN == DEF_ENABLED)
    p_tcb->CyclesStart = OS_TS_GET();
#endif
#endif

#if (OS_CFG_TASK_Q_EN == DEF_ENABLED)
    p_msg_q = &p_tcb->MsgQ;
    p_msg_q->NbrEntriesMax = 0u;
#endif
    p_tcb = p_tcb->DbgNextPtr;
    CPU_CRITICAL_EXIT();
  }
#endif

#if (OS_CFG_Q_EN == DEF_ENABLED) && (OS_CFG_DBG_EN == DEF_ENABLED)
  CPU_CRITICAL_ENTER();
  p_q = OSQDbgListPtr;
  CPU_CRITICAL_EXIT();
  while (p_q != DEF_NULL) {                                     // Reset message queues statistics
    CPU_CRITICAL_ENTER();
    p_msg_q = &p_q->MsgQ;
    p_msg_q->NbrEntriesMax = 0u;
    p_q = p_q->DbgNextPtr;
    CPU_CRITICAL_EXIT();
  }
#endif

  RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
}

/****************************************************************************************************//**
 *                                           OSStatTaskCPUUsageInit()
 *
 * @brief    Establishes CPU usage by first determining how high a 32-bit counter would count to in
 *           1/10 of a second if no other tasks were ready to execute during that time. CPU usage is
 *           determined by a low priority task which keeps track of this 32-bit counter every second,
 *           but this time, with other tasks running. CPU usage is determined by:
 *           @verbatim
 *                                           OS_Stat_IdleCtr
 *               CPU Usage (%) = 100 * (1 - ------------------)
 *                                           OS_Stat_IdleCtrMax
 *           @endverbatim
 * @param    p_err   Pointer to the variable that will receive one of the following error code(s)
 *                   from this function:
 *                       - RTOS_ERR_NONE
 *                       - RTOS_ERR_WOULD_OVF
 *                       - RTOS_ERR_OS
 *                       - RTOS_ERR_INVALID_ARG
 *                       - RTOS_ERR_NOT_READY
 *                       - RTOS_ERR_INVALID_STATE
 *                       - RTOS_ERR_OS_SCHED_LOCKED
 *******************************************************************************************************/
void OSStatTaskCPUUsageInit(RTOS_ERR *p_err)
{
  OS_TICK dly;
  CPU_SR_ALLOC();

  OS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err,; );

  //                                                               Make sure kernel is running.
  OS_ASSERT_DBG_ERR_SET((OSRunning == OS_STATE_OS_RUNNING), *p_err, RTOS_ERR_NOT_READY,; );

#if ((OS_CFG_TMR_EN == DEF_ENABLED) && (OS_CFG_TASK_SUSPEND_EN == DEF_ENABLED))
  OSTaskSuspend(&OSTmrTaskTCB, p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }
#endif

  OSTimeDly(2u,                                                 // Synchronize with clock tick
            (OS_OPT)OS_OPT_TIME_DLY,
            p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }
  CPU_CRITICAL_ENTER();
  OSStatTaskCtr = 0u;                                           // Clear idle counter
  CPU_CRITICAL_EXIT();

  dly = 0u;
  if (OSCfg_TickRate_Hz > OSCfg_StatTaskRate_Hz) {
    dly = (OS_TICK)(OSCfg_TickRate_Hz / OSCfg_StatTaskRate_Hz);
  }
  if (dly == 0u) {
    dly = (OSCfg_TickRate_Hz / 10u);
  }

  OSTimeDly(dly,                                                // Determine MAX. idle counter value
            OS_OPT_TIME_DLY,
            p_err);

#if ((OS_CFG_TMR_EN == DEF_ENABLED) && (OS_CFG_TASK_SUSPEND_EN == DEF_ENABLED))
  OSTaskResume(&OSTmrTaskTCB, p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }
#endif

  CPU_CRITICAL_ENTER();
#if (OS_CFG_TS_EN == DEF_ENABLED)
  OSStatTaskTimeMax = 0u;
#endif

  OSStatTaskCtrMax = OSStatTaskCtr;                             // Store maximum idle counter count
  OSStatTaskRdy = DEF_TRUE;
  CPU_CRITICAL_EXIT();
}

/********************************************************************************************************
 ********************************************************************************************************
 *                                           INTERNAL FUNCTIONS
 ********************************************************************************************************
 *******************************************************************************************************/

/****************************************************************************************************//**
 *                                               OS_StatTask()
 *
 * @brief    This task is internal to the Kernel and is used to compute some statistics about the
 *           multitasking environment. Specifically, OS_StatTask() computes the CPU usage.
 *           CPU usage is determined by:
 *
 *                                               OSStatTaskCtr
 *               OSStatTaskCPUUsage = 100 * (1 - ------------------)     (units are in %)
 *                                               OSStatTaskCtrMax
 *
 * @param    p_arg   Argument passed to the task when the task is created (unused).
 *
 * @note     (1) This task runs at a priority level higher than the idle task.
 *
 * @note     (2) You can disable this task by setting the configuration #define OS_CFG_STAT_TASK_EN
 *               to 0.
 *
 * @note     (3) You MUST have at least a delay of 2/10 seconds to allow for the system to establish
 *               the maximum value for the idle counter.
 *
 * @note     (4) This function is INTERNAL to the Kernel and your application MUST NOT call it.
 *******************************************************************************************************/
void OS_StatTask(void *p_arg)
{
#if (OS_CFG_DBG_EN == DEF_ENABLED)
#if (OS_CFG_TASK_PROFILE_EN == DEF_ENABLED)
  OS_CPU_USAGE usage;
  OS_CYCLES    cycles_total;
  OS_CYCLES    cycles_div;
  OS_CYCLES    cycles_mult;
  OS_CYCLES    cycles_max;
#endif
  OS_TCB *p_tcb;
#endif
  OS_TICK  ctr_max;
  OS_TICK  ctr_mult;
  OS_TICK  ctr_div;
  RTOS_ERR err;
  OS_TICK  dly;
#if (OS_CFG_TS_EN == DEF_ENABLED)
  CPU_TS ts_start;
  CPU_TS ts_end;
#endif
  CPU_SR_ALLOC();

  (void)p_arg;                                                  // Prevent compiler warning for not using 'p_arg'

  while (OSStatTaskRdy != DEF_TRUE) {
    OSTimeDly(2u * OSCfg_StatTaskRate_Hz,                       // Wait until statistic task is ready
              OS_OPT_TIME_DLY,
              &err);
  }
  OSStatReset(&err);                                            // Reset statistics

  dly = (OS_TICK)0;                                             // Compute statistic task sleep delay
  if (OSCfg_TickRate_Hz > OSCfg_StatTaskRate_Hz) {
    dly = (OSCfg_TickRate_Hz / OSCfg_StatTaskRate_Hz);
  }
  if (dly == 0u) {
    dly = (OSCfg_TickRate_Hz / 10u);
  }

  while (DEF_ON) {
#if (OS_CFG_TS_EN == DEF_ENABLED)
    ts_start = OS_TS_GET();
#endif
#ifdef  CPU_CFG_INT_DIS_MEAS_EN
    OSIntDisTimeMax = CPU_IntDisMeasMaxGet();
#endif

    CPU_CRITICAL_ENTER();                                       // ---------------- OVERALL CPU USAGE -----------------
    OSStatTaskCtrRun = OSStatTaskCtr;                           // Obtain the of the stat counter for the past .1 second
    OSStatTaskCtr = 0u;                                         // Reset the stat counter for the next .1 second
    CPU_CRITICAL_EXIT();

    if (OSStatTaskCtrMax > OSStatTaskCtrRun) {                  // Compute CPU Usage with best resolution
      if (OSStatTaskCtrMax < 400000u) {                         // 1 to       400,000
        ctr_mult = 10000u;
        ctr_div = 1u;
      } else if (OSStatTaskCtrMax < 4000000u) {                 // 400,000 to     4,000,000
        ctr_mult = 1000u;
        ctr_div = 10u;
      } else if (OSStatTaskCtrMax < 40000000u) {                // 4,000,000 to    40,000,000
        ctr_mult = 100u;
        ctr_div = 100u;
      } else if (OSStatTaskCtrMax < 400000000u) {               // 40,000,000 to   400,000,000
        ctr_mult = 10u;
        ctr_div = 1000u;
      } else {                                                  // 400,000,000 and up
        ctr_mult = 1u;
        ctr_div = 10000u;
      }
      ctr_max = OSStatTaskCtrMax / ctr_div;
      OSStatTaskCPUUsage = (OS_CPU_USAGE)((OS_TICK)10000u - ctr_mult * OSStatTaskCtrRun / ctr_max);
      if (OSStatTaskCPUUsageMax < OSStatTaskCPUUsage) {
        OSStatTaskCPUUsageMax = OSStatTaskCPUUsage;
      }
    } else {
      OSStatTaskCPUUsage = 0u;
    }

    OSStatTaskHook();                                           // Invoke user definable hook

#if (OS_CFG_DBG_EN == DEF_ENABLED)
#if (OS_CFG_TASK_PROFILE_EN == DEF_ENABLED)
    cycles_total = 0u;

    CPU_CRITICAL_ENTER();
    p_tcb = OSTaskDbgListPtr;
    CPU_CRITICAL_EXIT();
    while (p_tcb != DEF_NULL) {                                 // ---------------- TOTAL CYCLES COUNT ----------------
      CPU_CRITICAL_ENTER();
      p_tcb->CyclesTotalPrev = p_tcb->CyclesTotal;              // Save accumulated # cycles into a temp variable
      p_tcb->CyclesTotal = 0u;                                  // Reset total cycles for task for next run
      CPU_CRITICAL_EXIT();

      cycles_total += p_tcb->CyclesTotalPrev;                   // Perform sum of all task # cycles

      CPU_CRITICAL_ENTER();
      p_tcb = p_tcb->DbgNextPtr;
      CPU_CRITICAL_EXIT();
    }
#endif

#if (OS_CFG_TASK_PROFILE_EN == DEF_ENABLED)
    //                                                             ------------ INDIVIDUAL TASK CPU USAGE -------------
    if (cycles_total > 0u) {                                    // 'cycles_total' scaling ...
      if (cycles_total < 400000u) {                             // 1 to       400,000
        cycles_mult = 10000u;
        cycles_div = 1u;
      } else if (cycles_total < 4000000u) {                     // 400,000 to     4,000,000
        cycles_mult = 1000u;
        cycles_div = 10u;
      } else if (cycles_total < 40000000u) {                    // 4,000,000 to    40,000,000
        cycles_mult = 100u;
        cycles_div = 100u;
      } else if (cycles_total < 400000000u) {                   // 40,000,000 to   400,000,000
        cycles_mult = 10u;
        cycles_div = 1000u;
      } else {                                                  // 400,000,000 and up
        cycles_mult = 1u;
        cycles_div = 10000u;
      }
      cycles_max = cycles_total / cycles_div;
    } else {
      cycles_mult = 0u;
      cycles_max = 1u;
    }
#endif
    CPU_CRITICAL_ENTER();
    p_tcb = OSTaskDbgListPtr;
    CPU_CRITICAL_EXIT();
    while (p_tcb != DEF_NULL) {
#if (OS_CFG_TASK_PROFILE_EN == DEF_ENABLED)                     // Compute execution time of each task
      usage = (OS_CPU_USAGE)(cycles_mult * p_tcb->CyclesTotalPrev / cycles_max);
      if (usage > 10000u) {
        usage = 10000u;
      }
      p_tcb->CPUUsage = usage;
      if (p_tcb->CPUUsageMax < usage) {                         // Detect peak CPU usage
        p_tcb->CPUUsageMax = usage;
      }
#endif

#if (OS_CFG_STAT_TASK_STK_CHK_EN == DEF_ENABLED)
      OSTaskStkChk(p_tcb,                                       // Compute stack usage of active tasks only
                   &p_tcb->StkFree,
                   &p_tcb->StkUsed,
                   &err);
#endif

      CPU_CRITICAL_ENTER();
      p_tcb = p_tcb->DbgNextPtr;
      CPU_CRITICAL_EXIT();
    }
#endif

    if (OSStatResetFlag == DEF_TRUE) {                          // Check if need to reset statistics
      OSStatResetFlag = DEF_FALSE;
      OSStatReset(&err);
    }

#if (OS_CFG_TS_EN == DEF_ENABLED)
    ts_end = OS_TS_GET() - ts_start;                            // Measure execution time of statistic task
    if (OSStatTaskTimeMax < ts_end) {
      OSStatTaskTimeMax = ts_end;
    }
#endif

    OSTimeDly(dly,
              OS_OPT_TIME_DLY,
              &err);
  }
}

/****************************************************************************************************//**
 *                                               OS_StatTaskInit()
 *
 * @brief    This function is called by OSInit() to initialize the statistic task.
 *
 * @param    p_err   Pointer to the variable that will receive one of the following error code(s) from this function:
 *                       - RTOS_ERR_NONE
 *                       - RTOS_ERR_OS_ILLEGAL_RUN_TIME
 *
 * @note     (1) This function is INTERNAL to the Kernel and your application MUST NOT call it.
 *******************************************************************************************************/
void OS_StatTaskInit(RTOS_ERR *p_err)
{
  OS_ASSERT_DBG_ERR_SET((OSCfg_StatTaskStkBasePtr != DEF_NULL), *p_err, RTOS_ERR_INVALID_CFG,; );

  OS_ASSERT_DBG_ERR_SET((OSCfg_StatTaskStkSize >= OSCfg_StkSizeMin), *p_err, RTOS_ERR_INVALID_CFG,; );

  OS_ASSERT_DBG_ERR_SET((OSCfg_StatTaskPrio < (OS_CFG_PRIO_MAX - 1u)), *p_err, RTOS_ERR_INVALID_CFG,; );

  OSStatTaskCtr = 0u;
  OSStatTaskCtrRun = 0u;
  OSStatTaskCtrMax = 0u;
  OSStatTaskRdy = DEF_FALSE;                                    // Statistic task is not ready
  OSStatResetFlag = DEF_FALSE;

  //                                                               --------------- CREATE THE STAT TASK ---------------
  OSTaskCreate(&OSStatTaskTCB,
               (CPU_CHAR *)((void *)"Kernel's Stat Task"),
               OS_StatTask,
               DEF_NULL,
               OSCfg_StatTaskPrio,
               OSCfg_StatTaskStkBasePtr,
               OSCfg_StatTaskStkLimit,
               OSCfg_StatTaskStkSize,
               0u,
               0u,
               DEF_NULL,
               (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               p_err);
}
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                   DEPENDENCIES & AVAIL CHECK(S) END
 ********************************************************************************************************
 *******************************************************************************************************/

#endif // (defined(RTOS_MODULE_KERNEL_AVAIL))
