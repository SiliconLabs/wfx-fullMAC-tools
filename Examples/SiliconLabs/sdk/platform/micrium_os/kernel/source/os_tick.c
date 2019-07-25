/***************************************************************************//**
 * @file
 * @brief Kernel - Tick Management
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
const CPU_CHAR *os_tick__c = "$Id: $";
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                       LOCAL FUNCTION PROTOTYPES
 ********************************************************************************************************
 *******************************************************************************************************/

#if (OS_CFG_TASK_TICK_EN == DEF_ENABLED)

static CPU_TS OS_TickListUpdateDly(OS_TICK ticks);
static CPU_TS OS_TickListUpdateTimeout(OS_TICK ticks);

/********************************************************************************************************
 ********************************************************************************************************
 *                                           INTERNAL FUNCTIONS
 ********************************************************************************************************
 *******************************************************************************************************/

/****************************************************************************************************//**
 *                                               OS_TickTask()
 *
 * @brief    This task is internal to the Kernel and is triggered by the tick interrupt.
 *
 * @param    p_arg   Argument passed to the task when the task is created (unused).
 *
 * @note     (1) This function is INTERNAL to the Kernel and your application MUST NOT call it.
 *******************************************************************************************************/
void OS_TickTask(void *p_arg)
{
  RTOS_ERR err;
#if (OS_CFG_TS_EN == DEF_ENABLED)
  CPU_TS ts_delta;
  CPU_TS ts_delta_dly;
  CPU_TS ts_delta_timeout;
#endif
#if (OS_CFG_DYN_TICK_EN == DEF_ENABLED)
  OS_TICK tick_step_dly;
  OS_TICK tick_step_timeout;
#endif
  OS_TICK tick_step;
  CPU_SR_ALLOC();

  (void)&p_arg;                                                 // Prevent compiler warning

  while (DEF_ON) {
    (void)OSTaskSemPend(0u,
                        OS_OPT_PEND_BLOCKING,
                        DEF_NULL,
                        &err);                                  // Wait for signal from tick interrupt
    if (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) {
      CPU_CRITICAL_ENTER();

#if (OS_CFG_DYN_TICK_EN == DEF_ENABLED)
      tick_step = OSTickCtrPend;
      OSTickCtr += tick_step;
      OSTickCtrPend = 0;
#else
      tick_step = 1u;                                           // Always tick once when dynamic tick is disabled
      OSTickCtr++;                                              // Keep track of the number of ticks
#endif

      OS_TRACE_TICK_INCREMENT(OSTickCtr);

#if (OS_CFG_TS_EN == DEF_ENABLED)
      ts_delta_dly = OS_TickListUpdateDly(tick_step);
      ts_delta_timeout = OS_TickListUpdateTimeout(tick_step);
      ts_delta = ts_delta_dly + ts_delta_timeout;               // Compute total execution time of list updates
      if (OSTickTaskTimeMax < ts_delta) {
        OSTickTaskTimeMax = ts_delta;
      }
#else
      (void)OS_TickListUpdateDly(tick_step);
      (void)OS_TickListUpdateTimeout(tick_step);
#endif

#if (OS_CFG_DYN_TICK_EN == DEF_ENABLED)
      tick_step_dly = (OS_TICK)-1;
      tick_step_timeout = (OS_TICK)-1;
      if (OSTickListDly.TCB_Ptr != DEF_NULL) {
        tick_step_dly = OSTickListDly.TCB_Ptr->TickRemain;
      }
      if (OSTickListTimeout.TCB_Ptr != DEF_NULL) {
        tick_step_timeout = OSTickListTimeout.TCB_Ptr->TickRemain;
      }
      OSTickCtrStep = (tick_step_dly < tick_step_timeout) ? tick_step_dly : tick_step_timeout;
      BSP_OS_TickNextSet(OSTickCtrStep);
#endif
      CPU_CRITICAL_EXIT();
    }
  }
}

/****************************************************************************************************//**
 *                                               OS_TickTaskInit()
 *
 * @brief    This function is called by OSInit() to create the tick task.
 *
 * @param    p_err   Pointer to variable that will receive the return error code from this function :
 *                       - RTOS_ERR_NONE
 *                       - RTOS_ERR_OS_ILLEGAL_RUN_TIME
 *
 * @note     (1) This function is INTERNAL to the Kernel and your application MUST NOT call it.
 *******************************************************************************************************/
void OS_TickTaskInit(RTOS_ERR *p_err)
{
  OS_ASSERT_DBG_ERR_SET((OSCfg_TickTaskStkBasePtr != DEF_NULL), *p_err, RTOS_ERR_INVALID_CFG,; );

  OS_ASSERT_DBG_ERR_SET((OSCfg_TickTaskStkSize >= OSCfg_StkSizeMin), *p_err, RTOS_ERR_INVALID_CFG,; );

  OS_ASSERT_DBG_ERR_SET((OSCfg_TickTaskPrio < (OS_CFG_PRIO_MAX - 1u)), *p_err, RTOS_ERR_INVALID_CFG,; );

  OSTickCtr = 0u;                                               // Clear the tick counter

#if (OS_CFG_DYN_TICK_EN == DEF_ENABLED)
  OSTickCtrStep = (OS_TICK)-1;
  OSTickCtrPend = 0u;
#endif

  OSTickListDly.TCB_Ptr = DEF_NULL;
  OSTickListTimeout.TCB_Ptr = DEF_NULL;

#if (OS_CFG_DBG_EN == DEF_ENABLED)
  OSTickListDly.NbrEntries = 0u;
  OSTickListDly.NbrUpdated = 0u;

  OSTickListTimeout.NbrEntries = 0u;
  OSTickListTimeout.NbrUpdated = 0u;
#endif

  //                                                               --------------- CREATE THE TICK TASK ---------------
  OSTaskCreate(&OSTickTaskTCB,
               (CPU_CHAR *)((void *)"Kernel's Tick Task"),
               OS_TickTask,
               DEF_NULL,
               OSCfg_TickTaskPrio,
               OSCfg_TickTaskStkBasePtr,
               OSCfg_TickTaskStkLimit,
               OSCfg_TickTaskStkSize,
               0u,
               0u,
               DEF_NULL,
               (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_NO_TLS),
               p_err);
}

/****************************************************************************************************//**
 *                                           OS_TickListInsert()
 *
 * @brief    This function is called to insert a task in a tick list.
 *
 * @param    p_list  Pointer to the desired list.
 *
 * @param    p_tcb   Pointer to the TCB to insert in the list.
 *
 * @param    time    Amount of time remaining (in ticks) for the task to become ready.
 *
 * @note     (1) This function is INTERNAL to the Kernel and your application MUST NOT call it.
 *******************************************************************************************************/
void OS_TickListInsert(OS_TICK_LIST *p_list,
                       OS_TCB       *p_tcb,
                       OS_TICK      time)
{
  OS_TCB  *p_tcb1;
  OS_TCB  *p_tcb2;
  OS_TICK remain;
#if (OS_CFG_DYN_TICK_EN == DEF_ENABLED)
  OS_TICK tick_step = (OS_TICK)-1;
#endif

  if (p_list->TCB_Ptr == DEF_NULL) {                            // Is the list empty?
    p_tcb->TickRemain = time;                                   // Yes, Store time in TCB
    p_tcb->TickNextPtr = DEF_NULL;
    p_tcb->TickPrevPtr = DEF_NULL;
    p_tcb->TickListPtr = p_list;                                // Link to this list
    p_list->TCB_Ptr = p_tcb;                                    // Point to TCB of task to place in the list
#if (OS_CFG_DBG_EN == DEF_ENABLED)
    p_list->NbrEntries = 1u;                                    // List contains 1 entry
#endif
#if (OS_CFG_DYN_TICK_EN == DEF_ENABLED)
    tick_step = time;
#endif
  } else {
    p_tcb1 = p_list->TCB_Ptr;
    p_tcb2 = p_list->TCB_Ptr;                                   // No,  Insert somewhere in the list in delta order
    remain = time;
    while (p_tcb2 != DEF_NULL) {
      if (remain <= p_tcb2->TickRemain) {
        if (p_tcb2->TickPrevPtr == DEF_NULL) {                  // Insert before the first entry in the list?
          p_tcb->TickRemain = remain;                           // Yes, Store remaining time
          p_tcb->TickPrevPtr = DEF_NULL;
          p_tcb->TickNextPtr = p_tcb2;
          p_tcb->TickListPtr = p_list;                          // Link TCB to this list
          p_tcb2->TickRemain -= remain;                         // Reduce time of next entry in the list
          p_tcb2->TickPrevPtr = p_tcb;
          p_list->TCB_Ptr = p_tcb;                              // Add TCB to the list
#if (OS_CFG_DBG_EN == DEF_ENABLED)
          p_list->NbrEntries++;                                 // List contains an extra entry
#endif
#if (OS_CFG_DYN_TICK_EN == DEF_ENABLED)
          tick_step = remain;
#endif
        } else {                                                // No,  Insert somewhere further in the list
          p_tcb1 = p_tcb2->TickPrevPtr;
          p_tcb->TickRemain = remain;                           // Store remaining time
          p_tcb->TickPrevPtr = p_tcb1;
          p_tcb->TickNextPtr = p_tcb2;
          p_tcb->TickListPtr = p_list;                          // TCB points to this list
          p_tcb2->TickRemain -= remain;                         // Reduce time of next entry in the list
          p_tcb2->TickPrevPtr = p_tcb;
          p_tcb1->TickNextPtr = p_tcb;
#if (OS_CFG_DBG_EN == DEF_ENABLED)
          p_list->NbrEntries++;                                 // List contains an extra entry
#endif
        }

#if (OS_CFG_DYN_TICK_EN == DEF_ENABLED)
        if (tick_step < OSTickCtrStep) {
          OSTickCtrStep = tick_step;
          BSP_OS_TickNextSet(tick_step);
        }
#endif

        return;
      } else {
        remain -= p_tcb2->TickRemain;                           // Point to the next TCB in the list
        p_tcb1 = p_tcb2;
        p_tcb2 = p_tcb2->TickNextPtr;
      }
    }
    p_tcb->TickRemain = remain;
    p_tcb->TickPrevPtr = p_tcb1;
    p_tcb->TickNextPtr = DEF_NULL;
    p_tcb->TickListPtr = p_list;                                // Link the list to the TCB
    p_tcb1->TickNextPtr = p_tcb;
#if (OS_CFG_DBG_EN == DEF_ENABLED)
    p_list->NbrEntries++;                                       // List contains an extra entry
#endif
  }

#if (OS_CFG_DYN_TICK_EN == DEF_ENABLED)
  if (tick_step < OSTickCtrStep) {
    OSTickCtrStep = tick_step;
    BSP_OS_TickNextSet(tick_step);
  }
#endif
}

/****************************************************************************************************//**
 *                                           OS_TickListInsertDly()
 *
 * @brief    This function is called to place a task in a list of task waiting for either time to
 *           expire.
 *
 * @param    p_tcb   Pointer to the TCB of the task to add to the tick list.
 *
 * @param    time    Represents either the 'match' value of OSTickCtr or a relative time from
 *                   the current value of OSTickCtr as specified by the 'opt' argument..
 *                       - OS_OPT_TIME_DLY         OSTickCtr + time
 *                       - OS_OPT_TIME_TIMEOUT     OSTickCtr + time
 *                       - OS_OPT_TIME_MATCH       time
 *                       - OS_OPT_TIME_PERIODIC    OSTCBCurPtr.TickCtrPrev + time
 *
 * @param    opt     Option specifying how to calculate time. The valid values are:
 *                       - OS_OPT_TIME_DLY         Specifies a relative time from the current value of
 *                                                 OSTickCtr.
 *                       - OS_OPT_TIME_TIMEOUT     Same as OS_OPT_TIME_DLY.
 *                       - OS_OPT_TIME_MATCH       Indicates that 'time' specifies the absolute value
 *                                                 that OSTickCtr must reach before the task will be
 *                                                 resumed.
 *                       - OS_OPT_TIME_PERIODIC    Indicates that 'time' specifies the periodic value
 *                                                 that OSTickCtr must reach before the task will be
 *                                                 resumed.
 *
 * @param    p_err   Pointer to variable that will receive the return error code from this function :
 *                       - RTOS_ERR_NONE
 *                       - RTOS_ERR_INVALID_ARG
 *
 * @note     (1) This function is INTERNAL to the Kernel and your application MUST NOT call it.
 *
 * @note     (2) This function is assumed to be called with interrupts disabled.
 *******************************************************************************************************/
void OS_TickListInsertDly(OS_TCB   *p_tcb,
                          OS_TICK  time,
                          OS_OPT   opt,
                          RTOS_ERR *p_err)
{
  OS_TICK remain;
  OS_TICK tick_ctr;

#if (OS_CFG_DYN_TICK_EN == DEF_ENABLED)
  tick_ctr = BSP_OS_TickGet();
#else
  tick_ctr = OSTickCtr;
#endif

  if (opt == OS_OPT_TIME_MATCH) {                                                     // MATCH to absolute OSTickCtr value mode
    remain = time - tick_ctr;
    if ((remain > OS_TICK_TH_RDY)                                                     // If delay already occurred, ...
        || (remain == 0u)) {
      p_tcb->TickRemain = 0u;
      RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_ARG);                                     // ... do NOT delay.
      return;
    }
  } else if (opt == OS_OPT_TIME_PERIODIC) {                                           // PERIODIC mode.
    if ((tick_ctr - p_tcb->TickCtrPrev) > time) {
      remain = time;                                                                  // ... first time we load .TickCtrPrev
      p_tcb->TickCtrPrev = tick_ctr + time;
    } else {
      remain = time - (tick_ctr - p_tcb->TickCtrPrev);
      if ((remain > OS_TICK_TH_RDY)                                                   // If delay time has already passed, ...
          || (remain == 0u)) {
        p_tcb->TickCtrPrev += time + time * ((tick_ctr - p_tcb->TickCtrPrev) / time); // Try to recover the period
        p_tcb->TickRemain = 0u;
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_ARG);                                   // ... do NOT delay.
        return;
      }
      p_tcb->TickCtrPrev += time;
    }
  } else {                                                      // RELATIVE time delay mode
    remain = time;
  }

  RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

  p_tcb->TaskState = OS_TASK_STATE_DLY;
  OS_TickListInsert(&OSTickListDly, p_tcb, remain + (tick_ctr - OSTickCtr));
}

/****************************************************************************************************//**
 *                                           OS_TickListRemove()
 *
 * @brief    This function is called to remove a task from the tick list.
 *
 * @param    p_tcb   Pointer to the TCB to remove.
 *
 * @note     (1) This function is INTERNAL to the Kernel and your application MUST NOT call it.
 *
 * @note     (2) This function is assumed to be called with interrupts disabled.
 *******************************************************************************************************/
void OS_TickListRemove(OS_TCB *p_tcb)
{
  OS_TICK_LIST *p_list;
  OS_TCB       *p_tcb1;
  OS_TCB       *p_tcb2;

  p_list = p_tcb->TickListPtr;
  p_tcb1 = p_tcb->TickPrevPtr;
  p_tcb2 = p_tcb->TickNextPtr;
  if (p_tcb1 == DEF_NULL) {
    if (p_tcb2 == DEF_NULL) {                                   // Remove ONLY entry in the list?
      p_list->TCB_Ptr = DEF_NULL;
#if (OS_CFG_DBG_EN == DEF_ENABLED)
      p_list->NbrEntries = 0u;
#endif
      p_tcb->TickRemain = 0u;
      p_tcb->TickListPtr = DEF_NULL;
    } else {
      p_tcb2->TickPrevPtr = DEF_NULL;
      p_tcb2->TickRemain += p_tcb->TickRemain;                  // Add back the ticks to the delta
      p_list->TCB_Ptr = p_tcb2;
#if (OS_CFG_DBG_EN == DEF_ENABLED)
      p_list->NbrEntries--;
#endif
      p_tcb->TickNextPtr = DEF_NULL;
      p_tcb->TickRemain = 0u;
      p_tcb->TickListPtr = DEF_NULL;
    }
  } else {
    p_tcb1->TickNextPtr = p_tcb2;
    if (p_tcb2 != DEF_NULL) {
      p_tcb2->TickPrevPtr = p_tcb1;
      p_tcb2->TickRemain += p_tcb->TickRemain;                  // Add back the ticks to the delta list
    }
    p_tcb->TickPrevPtr = DEF_NULL;
#if (OS_CFG_DBG_EN == DEF_ENABLED)
    p_list->NbrEntries--;
#endif
    p_tcb->TickNextPtr = DEF_NULL;
    p_tcb->TickRemain = 0u;
    p_tcb->TickListPtr = DEF_NULL;
  }
}

/********************************************************************************************************
 ********************************************************************************************************
 *                                           LOCAL FUNCTIONS
 ********************************************************************************************************
 *******************************************************************************************************/

/****************************************************************************************************//**
 *                                           OS_TickListUpdateDly()
 *
 * @brief    This function updates the delta list which contains tasks that have been delayed.
 *
 * @param    ticks   Number of OS Ticks to process.
 *
 * @return   Number of timestamp ticks elapsed in update, or 0 is timestamp is disabled.
 *
 * @note     (1) This function is INTERNAL to the Kernel and your application MUST NOT call it.
 *******************************************************************************************************/
static CPU_TS OS_TickListUpdateDly(OS_TICK ticks)
{
  OS_TCB       *p_tcb;
  OS_TICK_LIST *p_list;
#if (OS_CFG_TS_EN == DEF_ENABLED)
  CPU_TS ts_start;
  CPU_TS ts_delta_dly;
#endif
#if (OS_CFG_DBG_EN == DEF_ENABLED)
  OS_OBJ_QTY nbr_updated;
#endif

  //                                                               ========= UPDATE TASKS WAITING FOR DELAY =========
#if (OS_CFG_TS_EN == DEF_ENABLED)
  ts_start = OS_TS_GET();
#endif
#if (OS_CFG_DBG_EN == DEF_ENABLED)
  nbr_updated = (OS_OBJ_QTY)0u;
#endif
  p_list = &OSTickListDly;
  p_tcb = p_list->TCB_Ptr;
  if (p_tcb != DEF_NULL) {
    if (p_tcb->TickRemain <= ticks) {
      ticks = ticks - p_tcb->TickRemain;
      p_tcb->TickRemain = 0u;
    } else {
      p_tcb->TickRemain -= ticks;
    }

    while (p_tcb->TickRemain == 0u) {
#if (OS_CFG_DBG_EN == DEF_ENABLED)
      nbr_updated++;                                            // Keep track of the number of TCBs updated
#endif
      if (p_tcb->TaskState == OS_TASK_STATE_DLY) {
        p_tcb->TaskState = OS_TASK_STATE_RDY;
        OS_RdyListInsert(p_tcb);                                // Insert the task in the ready list
      } else if (p_tcb->TaskState == OS_TASK_STATE_DLY_SUSPENDED) {
        p_tcb->TaskState = OS_TASK_STATE_SUSPENDED;
      }

      p_list->TCB_Ptr = p_tcb->TickNextPtr;
      p_tcb = p_list->TCB_Ptr;                                  // Get 'p_tcb' again for loop
      if (p_tcb == DEF_NULL) {
#if (OS_CFG_DBG_EN == DEF_ENABLED)
        p_list->NbrEntries = 0u;
#endif
        break;
      } else {
#if (OS_CFG_DBG_EN == DEF_ENABLED)
        p_list->NbrEntries--;
#endif
        p_tcb->TickPrevPtr = DEF_NULL;
      }

      if (p_tcb->TickRemain <= ticks) {
        ticks = ticks - p_tcb->TickRemain;
        p_tcb->TickRemain = 0u;
      } else {
        p_tcb->TickRemain -= ticks;
      }
    }
  }
#if (OS_CFG_DBG_EN == DEF_ENABLED)
  p_list->NbrUpdated = nbr_updated;
#endif
#if (OS_CFG_TS_EN == DEF_ENABLED)
  ts_delta_dly = OS_TS_GET() - ts_start;                        // Measure execution time of the update
#endif

#if (OS_CFG_TS_EN == DEF_ENABLED)
  return (ts_delta_dly);
#else
  return (0u);
#endif
}

/****************************************************************************************************//**
 *                                       OS_TickListUpdateTimeout()
 *
 * @brief    This function updales the delta list which contains tasks that are pending with a timeout.
 *
 * @param    ticks   Number of OS Ticks to process.
 *
 * @return   Number of timestamp ticks elapsed in update, or 0 is timestamp is disabled.
 *
 * @note     (1) This function is INTERNAL to the Kernel and your application MUST NOT call it.
 *******************************************************************************************************/
static CPU_TS OS_TickListUpdateTimeout(OS_TICK ticks)
{
  OS_TCB       *p_tcb;
  OS_TICK_LIST *p_list;
#if (OS_CFG_TS_EN == DEF_ENABLED)
  CPU_TS ts_start;
  CPU_TS ts_delta_timeout;
#endif
#if (OS_CFG_DBG_EN == DEF_ENABLED)
  OS_OBJ_QTY nbr_updated;
#endif
#if (OS_CFG_MUTEX_EN == DEF_ENABLED)
  OS_TCB  *p_tcb_owner;
  OS_PRIO prio_new;
#endif

  //                                                               ======= UPDATE TASKS WAITING WITH TIMEOUT ========
#if (OS_CFG_TS_EN == DEF_ENABLED)
  ts_start = OS_TS_GET();
#endif
#if (OS_CFG_DBG_EN == DEF_ENABLED)
  nbr_updated = 0u;
#endif
  p_list = &OSTickListTimeout;
  p_tcb = p_list->TCB_Ptr;
  if (p_tcb != DEF_NULL) {
    if (p_tcb->TickRemain <= ticks) {
      ticks = ticks - p_tcb->TickRemain;
      p_tcb->TickRemain = 0u;
    } else {
      p_tcb->TickRemain -= ticks;
    }

    while (p_tcb->TickRemain == 0u) {
#if (OS_CFG_DBG_EN == DEF_ENABLED)
      nbr_updated++;
#endif

#if (OS_CFG_MUTEX_EN == DEF_ENABLED)
      p_tcb_owner = DEF_NULL;
      if (p_tcb->PendOn == OS_TASK_PEND_ON_MUTEX) {
        p_tcb_owner = ((OS_MUTEX *)p_tcb->PendObjPtr)->OwnerTCBPtr;
      }
#endif

#if (OS_MSG_EN == DEF_ENABLED)
      p_tcb->MsgPtr = DEF_NULL;
      p_tcb->MsgSize = 0u;
#endif
#if (OS_CFG_TS_EN == DEF_ENABLED)
      p_tcb->TS = OS_TS_GET();
#endif
      OS_PendListRemove(p_tcb);                                 // Remove task from pend list
      if (p_tcb->TaskState == OS_TASK_STATE_PEND_TIMEOUT) {
        OS_RdyListInsert(p_tcb);                                // Insert the task in the ready list
        p_tcb->TaskState = OS_TASK_STATE_RDY;
      } else if (p_tcb->TaskState == OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED) {
        p_tcb->TaskState = OS_TASK_STATE_SUSPENDED;
      }
      p_tcb->PendStatus = OS_STATUS_PEND_TIMEOUT;               // Indicate pend timed out
      p_tcb->PendOn = OS_TASK_PEND_ON_NOTHING;                  // Indicate no longer pending

#if (OS_CFG_MUTEX_EN == DEF_ENABLED)
      if (p_tcb_owner != DEF_NULL) {
        if ((p_tcb_owner->Prio != p_tcb_owner->BasePrio)
            && (p_tcb_owner->Prio == p_tcb->Prio)) {            // Has the owner inherited a priority?
          prio_new = OS_MutexGrpPrioFindHighest(p_tcb_owner);
          prio_new = prio_new > p_tcb_owner->BasePrio ? p_tcb_owner->BasePrio : prio_new;
          if (prio_new != p_tcb_owner->Prio) {
            OS_TaskChangePrio(p_tcb_owner, prio_new);
            OS_TRACE_MUTEX_TASK_PRIO_DISINHERIT(p_tcb_owner, p_tcb_owner->Prio);
          }
        }
      }
#endif

      p_list->TCB_Ptr = p_tcb->TickNextPtr;
      p_tcb = p_list->TCB_Ptr;                                  // Get 'p_tcb' again for loop
      if (p_tcb == DEF_NULL) {
#if (OS_CFG_DBG_EN == DEF_ENABLED)
        p_list->NbrEntries = 0u;
#endif
        break;
      } else {
#if (OS_CFG_DBG_EN == DEF_ENABLED)
        p_list->NbrEntries--;
#endif
        p_tcb->TickPrevPtr = DEF_NULL;
      }
      if (p_tcb->TickRemain <= ticks) {
        ticks = ticks - p_tcb->TickRemain;
        p_tcb->TickRemain = 0u;
      } else {
        p_tcb->TickRemain -= ticks;
      }
    }
  }
#if (OS_CFG_DBG_EN == DEF_ENABLED)
  p_list->NbrUpdated = nbr_updated;
#endif
#if (OS_CFG_TS_EN == DEF_ENABLED)
  ts_delta_timeout = OS_TS_GET() - ts_start;                    // Measure execution time of the update
#endif

#if (OS_CFG_TS_EN == DEF_ENABLED)
  return (ts_delta_timeout);
#else
  return (0u);
#endif
}
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                   DEPENDENCIES & AVAIL CHECK(S) END
 ********************************************************************************************************
 *******************************************************************************************************/

#endif // (defined(RTOS_MODULE_KERNEL_AVAIL))
