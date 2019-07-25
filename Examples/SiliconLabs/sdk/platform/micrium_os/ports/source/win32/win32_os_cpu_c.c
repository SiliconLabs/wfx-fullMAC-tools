/***************************************************************************//**
 * @file
 * @brief Kernel - Microsoft Win32 Emulation Port
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
 * @note     (1) This port targets the following:
 *                   Core      : Win32
 *                   Mode      :
 *                   Toolchain : Visual Studio
 *
 * @note     (2) This port is an emulation support port.
 *******************************************************************************************************/

#define   OS_CPU_GLOBALS

#ifdef VSC_INCLUDE_SOURCE_FILE_NAMES
const CPU_CHAR *os_cpu_c__c = "$Id: $";
#endif

/********************************************************************************************************
 *                                               INCLUDE FILES
 *******************************************************************************************************/

#include  <kernel/include/os.h>

#define  _WIN32_WINNT  0x0600
#define   WIN32_LEAN_AND_MEAN

#include  <windows.h>
#include  <mmsystem.h>
#include  <stdio.h>

#ifdef __cplusplus
extern  "C" {
#endif

/********************************************************************************************************
 *                                               LOCAL DEFINES
 *******************************************************************************************************/

#define  THREAD_EXIT_CODE                           0u          // Thread exit code returned on task termination.

#ifdef  _MSC_VER
#define  MS_VC_EXCEPTION                   0x406D1388u          // Visual Studio C Exception to change thread name.
#endif

#define  WIN_MM_MIN_RES                             1u          // Minimum timer resolution.

/********************************************************************************************************
 *                                           LOCAL DATA TYPES
 *******************************************************************************************************/

typedef enum os_task_state {
  STATE_NONE = 0,
  STATE_CREATED,
  STATE_RUNNING,
  STATE_SUSPENDED,
  STATE_INTERRUPTED,
  STATE_TERMINATING,
  STATE_TERMINATED
} OS_TASK_STATE;

typedef struct os_task {
  //                                                               Links all created tasks.
  struct os_task         *NextPtr;
  struct os_task         *PrevPtr;

  OS_TCB                 *OSTCBPtr;
  CPU_CHAR               *OSTaskName;
  //                                                               ---------------- INTERNAL INFORMATION ----------------
  void                   *TaskArgPtr;
  OS_OPT                 TaskOpt;
  OS_TASK_PTR            TaskPtr;
  volatile OS_TASK_STATE TaskState;
  DWORD                  ThreadID;
  HANDLE                 ThreadHandle;
  HANDLE                 InitSignalPtr;                         // Task created         signal.
  HANDLE                 SignalPtr;                             // Task synchronization signal.
} OS_TASK;

#ifdef _MSC_VER
#pragma pack(push, 8)
typedef struct threadname_info {
  DWORD  dwType;                                                // Must be 0x1000.
  LPCSTR szName;                                                // Pointer to name (in user addr space).
  DWORD  dwThreadID;                                            // Thread ID (-1 = caller thread).
  DWORD  dwFlags;                                               // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)
#endif

#if (OS_CFG_TIMER_METHOD_WIN32 == WIN32_MM_TMR)
#ifdef _MSC_VER
#pragma  comment (lib, "winmm.lib")
#endif
#endif

/********************************************************************************************************
 *                                           LOCAL VARIABLES
 *******************************************************************************************************/

static OS_TASK *OSTaskListPtr;
static HANDLE  OSTerminate_SignalPtr;

static HANDLE OSTick_Thread;
static DWORD  OSTick_ThreadId;
#if (OS_CFG_TIMER_METHOD_WIN32 == WIN32_MM_TMR)
static HANDLE   OSTick_SignalPtr;
static TIMECAPS OSTick_TimerCap;
static MMRESULT OSTick_TimerId;
#endif

/********************************************************************************************************
 *                                       LOCAL FUNCTION PROTOTYPES
 *******************************************************************************************************/

static DWORD WINAPI OSTickW32(LPVOID p_arg);
static DWORD WINAPI OSTaskW32(LPVOID p_arg);

static OS_TASK *OSTaskGet(OS_TCB *p_tcb);
static void OSTaskTerminate(OS_TASK *p_task);

static BOOL WINAPI OSCtrlBreakHandler(DWORD ctrl);

static void OSSetThreadName(DWORD    thread_id,
                            CPU_CHAR *p_name);

#ifdef OS_CFG_MSG_TRACE_EN
static int OS_Printf(char *p_str, ...);
#endif

/****************************************************************************************************//**
 *                                               OSIdleTaskHook()
 *
 * @brief    Allows you to stop the CPU (ex.: to conserve power). This function is called by the
 *           idle task.
 *
 * @note     (1) This function is deprecated and will be removed in an upcoming release.
 *******************************************************************************************************/
void OSIdleTaskHook(void)
{
#if OS_CFG_APP_HOOKS_EN > 0u
  if (OS_AppIdleTaskHookPtr != (OS_APP_HOOK_VOID)0) {
    (*OS_AppIdleTaskHookPtr)();
  }
#endif

  Sleep(1u);                                                    // Reduce CPU utilization.
}

/****************************************************************************************************//**
 *                                               OSInitHook()
 *
 * @brief    Called by OSInit() at the beginning of OSInit().
 *******************************************************************************************************/
void OSInitHook(void)
{
  HANDLE hProc;

#ifdef OS_CFG_MSG_TRACE_EN
#if (OS_CFG_TIMER_METHOD_WIN32 == WIN32_SLEEP)
  if (OSCfg_TickRate_Hz > 100u) {
    OS_Printf("OS_CFG_TIMER_METHOD_WIN32 Warning: Sleep timer method cannot maintain time accuracy with the current setting of OSCfg_TickRate_Hz (%du). Consider using Multimedia timer method.\n\n",
              OSCfg_TickRate_Hz);
  }
#endif
#endif

  OSTaskListPtr = NULL;
  OSTerminate_SignalPtr = NULL;
  OSTick_Thread = NULL;
#if (OS_CFG_TIMER_METHOD_WIN32 == WIN32_MM_TMR)
  OSTick_SignalPtr = NULL;
#endif

  // CPU_IntInit();                                                // Initialize Critical Section objects.

  hProc = GetCurrentProcess();
  SetPriorityClass(hProc, HIGH_PRIORITY_CLASS);
  SetProcessAffinityMask(hProc, 1);

  OSSetThreadName(GetCurrentThreadId(), "main()");

  //                                                               Manual reset enabled to broadcast terminate signal.
  OSTerminate_SignalPtr = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (OSTerminate_SignalPtr == NULL) {
#ifdef OS_CFG_MSG_TRACE_EN
    OS_Printf("Error: CreateEvent [OSTerminate] failed.\n");
#endif
    return;
  }
  SetConsoleCtrlHandler((PHANDLER_ROUTINE)OSCtrlBreakHandler, TRUE);

  OSTick_Thread = CreateThread(NULL, 0, OSTickW32, 0, CREATE_SUSPENDED, &OSTick_ThreadId);
  if (OSTick_Thread == NULL) {
#ifdef OS_CFG_MSG_TRACE_EN
    OS_Printf("Error: CreateThread [OSTickW32] failed.\n");
#endif
    CloseHandle(OSTerminate_SignalPtr);
    OSTerminate_SignalPtr = NULL;
    return;
  }

#ifdef OS_CFG_MSG_TRACE_EN
  OS_Printf("Thread    '%-32s' Created, Thread ID %5.0d\n",
            "OSTickW32",
            OSTick_ThreadId);
#endif

  SetThreadPriority(OSTick_Thread, THREAD_PRIORITY_HIGHEST);

#if (OS_CFG_TIMER_METHOD_WIN32 == WIN32_MM_TMR)
  if (timeGetDevCaps(&OSTick_TimerCap, sizeof(OSTick_TimerCap)) != TIMERR_NOERROR) {
#ifdef OS_CFG_MSG_TRACE_EN
    OS_Printf("Error: Cannot retrieve Timer capabilities.\n");
#endif
    CloseHandle(OSTick_Thread);
    CloseHandle(OSTerminate_SignalPtr);

    OSTick_Thread = NULL;
    OSTerminate_SignalPtr = NULL;
    return;
  }

  if (OSTick_TimerCap.wPeriodMin < WIN_MM_MIN_RES) {
    OSTick_TimerCap.wPeriodMin = WIN_MM_MIN_RES;
  }

  if (timeBeginPeriod(OSTick_TimerCap.wPeriodMin) != TIMERR_NOERROR) {
#ifdef OS_CFG_MSG_TRACE_EN
    OS_Printf("Error: Cannot set Timer minimum resolution.\n");
#endif
    CloseHandle(OSTick_Thread);
    CloseHandle(OSTerminate_SignalPtr);

    OSTick_Thread = NULL;
    OSTerminate_SignalPtr = NULL;
    return;
  }

  OSTick_SignalPtr = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (OSTick_SignalPtr == NULL) {
#ifdef OS_CFG_MSG_TRACE_EN
    OS_Printf("Error: CreateEvent [OSTick] failed.\n");
#endif
    timeEndPeriod(OSTick_TimerCap.wPeriodMin);
    CloseHandle(OSTick_Thread);
    CloseHandle(OSTerminate_SignalPtr);

    OSTick_Thread = NULL;
    OSTerminate_SignalPtr = NULL;
    return;
  }

#ifdef _MSC_VER
#pragma warning (disable : 4055)
#endif
  OSTick_TimerId = timeSetEvent((UINT)(1000u / OSCfg_TickRate_Hz),
                                (UINT) OSTick_TimerCap.wPeriodMin,
                                (LPTIMECALLBACK) OSTick_SignalPtr,
                                (DWORD_PTR) NULL,
                                (UINT)(TIME_PERIODIC | TIME_CALLBACK_EVENT_SET));
#ifdef _MSC_VER
#pragma warning (default : 4055)
#endif

  if (OSTick_TimerId == 0u) {
#ifdef OS_CFG_MSG_TRACE_EN
    OS_Printf("Error: Cannot start Timer.\n");
#endif
    CloseHandle(OSTick_SignalPtr);
    timeEndPeriod(OSTick_TimerCap.wPeriodMin);
    CloseHandle(OSTick_Thread);
    CloseHandle(OSTerminate_SignalPtr);

    OSTick_SignalPtr = NULL;
    OSTick_Thread = NULL;
    OSTerminate_SignalPtr = NULL;
    return;
  }
#endif
}

/****************************************************************************************************//**
 *                                           OSRedzoneHitHook()
 *
 * @brief    Called when a task's stack has overflowed.
 *
 * @param    p_tcb   Pointer to the TCB of the offending task. NULL if ISR.
 *******************************************************************************************************/

#if (OS_CFG_TASK_STK_REDZONE_EN == DEF_ENABLED)
void OSRedzoneHitHook(OS_TCB *p_tcb)
{
#if OS_CFG_APP_HOOKS_EN > 0u
  if (OS_AppRedzoneHitHookPtr != (OS_APP_HOOK_TCB)0) {
    (*OS_AppRedzoneHitHookPtr)(p_tcb);
  } else {
    CPU_SW_EXCEPTION(; );
  }
#else
  (void)p_tcb;                                                  // Prevent compiler warning
  CPU_SW_EXCEPTION(; );
#endif
}
#endif

/****************************************************************************************************//**
 *                                               OSStatTaskHook()
 *
 * @brief    This function is called every second by the Kernel's statistics task.  This allows your
 *           application to add functionality to the statistics task.
 *******************************************************************************************************/
void OSStatTaskHook(void)
{
#if OS_CFG_APP_HOOKS_EN > 0u
  if (OS_AppStatTaskHookPtr != (OS_APP_HOOK_VOID)0) {
    (*OS_AppStatTaskHookPtr)();
  }
#endif
}

/****************************************************************************************************//**
 *                                           OSTaskCreateHook()
 *
 * @brief    Called when a task is created.
 *
 * @param    p_tcb   Pointer to the TCB of the task being created.
 *******************************************************************************************************/
void OSTaskCreateHook(OS_TCB *p_tcb)
{
  OS_TASK *p_task;
  CPU_SR_ALLOC();

#if OS_CFG_APP_HOOKS_EN > 0u
  if (OS_AppTaskCreateHookPtr != (OS_APP_HOOK_TCB)0) {
    (*OS_AppTaskCreateHookPtr)(p_tcb);
  }
#endif

  p_task = OSTaskGet(p_tcb);
#if OS_CFG_DBG_EN > 0u
  p_task->OSTaskName = p_tcb->NamePtr;
#else
  p_task->OSTaskName = "";
#endif
  //                                                               See Note #2.
  p_task->SignalPtr = CreateEvent(NULL, FALSE, FALSE, NULL);
  if (p_task->SignalPtr == NULL) {
#ifdef OS_CFG_MSG_TRACE_EN
    OS_Printf("Task[%3.1d] '%s' cannot allocate signal event.\n",
              p_tcb->Prio,
              p_task->OSTaskName);
#endif
    return;
  }
  //                                                               See Note #2.
  p_task->InitSignalPtr = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (p_task->InitSignalPtr == NULL) {
    CloseHandle(p_task->SignalPtr);
    p_task->SignalPtr = NULL;

#ifdef OS_CFG_MSG_TRACE_EN
    OS_Printf("Task[%3.1d] '%s' cannot allocate initialization complete signal event.\n",
              p_tcb->Prio,
              p_task->OSTaskName);
#endif
    return;
  }

  p_task->ThreadHandle = CreateThread(NULL, 0, OSTaskW32, p_tcb, CREATE_SUSPENDED, &p_task->ThreadID);
  if (p_task->ThreadHandle == NULL) {
    CloseHandle(p_task->InitSignalPtr);
    CloseHandle(p_task->SignalPtr);

    p_task->InitSignalPtr = NULL;
    p_task->SignalPtr = NULL;
#ifdef OS_CFG_MSG_TRACE_EN
    OS_Printf("Task[%3.1d] '%s' failed to be created.\n",
              p_tcb->Prio,
              p_task->OSTaskName);
#endif
    return;
  }

#ifdef OS_CFG_MSG_TRACE_EN
  OS_Printf("Task[%3.1d] '%-32s' Created, Thread ID %5.0d\n",
            p_tcb->Prio,
            p_task->OSTaskName,
            p_task->ThreadID);
#endif

  p_task->TaskState = STATE_CREATED;
  p_task->OSTCBPtr = p_tcb;

  CPU_CRITICAL_ENTER();
  p_task->PrevPtr = (OS_TASK *)0;
  if (OSTaskListPtr == (OS_TASK *)0) {
    p_task->NextPtr = (OS_TASK *)0;
  } else {
    p_task->NextPtr = OSTaskListPtr;
    OSTaskListPtr->PrevPtr = p_task;
  }
  OSTaskListPtr = p_task;
  CPU_CRITICAL_EXIT();
}

/****************************************************************************************************//**
 *                                               OSTaskDelHook()
 *
 * @brief    Called when a task is deleted.
 *
 * @param    p_tcb   Pointer to the TCB of the task being deleted.
 *******************************************************************************************************/
void OSTaskDelHook(OS_TCB *p_tcb)
{
  OS_TASK *p_task;

#if OS_CFG_APP_HOOKS_EN > 0u
  if (OS_AppTaskDelHookPtr != (OS_APP_HOOK_TCB)0) {
    (*OS_AppTaskDelHookPtr)(p_tcb);
  }
#endif

  p_task = OSTaskGet(p_tcb);

  if (p_task == (OS_TASK *)0) {
    return;
  }

  switch (p_task->TaskState) {
    case STATE_RUNNING:
      if (GetCurrentThreadId() == p_task->ThreadID) {
        p_task->TaskState = STATE_TERMINATING;
      } else {
        TerminateThread(p_task->ThreadHandle, THREAD_EXIT_CODE);
        CloseHandle(p_task->ThreadHandle);

        OSTaskTerminate(p_task);
      }
      break;

    case STATE_CREATED:
    case STATE_SUSPENDED:
    case STATE_INTERRUPTED:
      TerminateThread(p_task->ThreadHandle, THREAD_EXIT_CODE);
      CloseHandle(p_task->ThreadHandle);

      OSTaskTerminate(p_task);
      break;

    default:
      break;
  }
}

/****************************************************************************************************//**
 *                                           OSTaskReturnHook()
 *
 * @brief    Called if a task accidentally returns. In other words, a task should either be an infinite
 *           loop or delete itself when done.
 *
 * @param    p_tcb   Pointer to the TCB of the task that is returning.
 *******************************************************************************************************/
void OSTaskReturnHook(OS_TCB *p_tcb)
{
#if OS_CFG_APP_HOOKS_EN > 0u
  if (OS_AppTaskReturnHookPtr != (OS_APP_HOOK_TCB)0) {
    (*OS_AppTaskReturnHookPtr)(p_tcb);
  }
#else
  (void)p_tcb;                                                  // Prevent compiler warning
#endif
}

/****************************************************************************************************//**
 *                                               OSTaskStkInit()
 *
 * @brief    Initializes the stack frame of the task being created as if it had been already
 *           switched-out. This function is called by OSTaskCreate() and is highly processor specific.
 *
 * @param    p_task          Pointer to the task entry point address.
 *
 * @param    p_arg           Pointer to a user-supplied data area that will be passed to the task
 *                           when the task first executes.
 *
 * @param    p_stk_base      Pointer to the base address of the stack.
 *
 * @param    p_stk_limit     Pointer to the element to set as the 'watermark' limit of the stack.
 *
 * @param    stk_size        Size of the stack (measured as number of CPU_STK elements).
 *
 * @param    opt             Options used to alter the behavior of OSTaskStkInit().
 *                           See OS.H for OS_TASK_OPT_xxx.
 *
 * @return   Always returns the location of the new top-of-stack once the processor registers have
 *           been placed on the stack in the proper order.
 *******************************************************************************************************/
CPU_STK *OSTaskStkInit(OS_TASK_PTR  p_task,
                       void         *p_arg,
                       CPU_STK      *p_stk_base,
                       CPU_STK      *p_stk_limit,
                       CPU_STK_SIZE stk_size,
                       OS_OPT       opt)
{
  OS_TASK *p_task_info;

  (void)p_stk_limit;                                            // Prevent compiler warning
  (void)stk_size;

  //                                                               Create task info struct into task's stack.
  p_task_info = (OS_TASK *)(&p_stk_base[stk_size] - sizeof(OS_TASK));

  p_task_info->NextPtr = NULL;
  p_task_info->PrevPtr = NULL;
  p_task_info->OSTCBPtr = NULL;
  p_task_info->OSTaskName = NULL;

  p_task_info->TaskArgPtr = p_arg;
  p_task_info->TaskOpt = opt;
  p_task_info->TaskPtr = p_task;
  p_task_info->TaskState = STATE_NONE;

  p_task_info->ThreadID = 0u;
  p_task_info->ThreadHandle = NULL;
  p_task_info->InitSignalPtr = NULL;
  p_task_info->SignalPtr = NULL;

  return ((CPU_STK *)p_task_info);
}

/****************************************************************************************************//**
 *                                               OSTaskSwHook()
 *
 * @brief    Allows you to perform other operations during a context switch. This function is called
 *           when a task switch is performed.
 *
 * @note     (1) Interrupts are disabled during this call.
 *
 * @note     (2) It is assumed that the global pointer 'OSTCBHighRdyPtr' points to the TCB of the task
 *               that will be 'switched in' (i.e. the highest priority task) and, 'OSTCBCurPtr' points
 *               to the task being switched out (i.e. the preempted task).
 *******************************************************************************************************/
void OSTaskSwHook(void)
{
#if OS_CFG_TASK_PROFILE_EN > 0u
  CPU_TS ts;
#endif
#ifdef  CPU_CFG_INT_DIS_MEAS_EN
  CPU_TS int_dis_time;
#endif

#if OS_CFG_APP_HOOKS_EN > 0u
  if (OS_AppTaskSwHookPtr != (OS_APP_HOOK_VOID)0) {
    (*OS_AppTaskSwHookPtr)();
  }
#endif

#if OS_CFG_TASK_PROFILE_EN > 0u
  ts = OS_TS_GET();
  if (OSTCBCurPtr != OSTCBHighRdyPtr) {
    OSTCBCurPtr->CyclesDelta = ts - OSTCBCurPtr->CyclesStart;
    OSTCBCurPtr->CyclesTotal += (OS_CYCLES)OSTCBCurPtr->CyclesDelta;
  }

  OSTCBHighRdyPtr->CyclesStart = ts;
#endif

#ifdef  CPU_CFG_INT_DIS_MEAS_EN
  int_dis_time = CPU_IntDisMeasMaxCurReset();                   // Keep track of per-task interrupt disable time
  if (OSTCBCurPtr->IntDisTimeMax < int_dis_time) {
    OSTCBCurPtr->IntDisTimeMax = int_dis_time;
  }
#endif

#if OS_CFG_SCHED_LOCK_TIME_MEAS_EN > 0u
  //                                                               Keep track of per-task scheduler lock time
  if (OSTCBCurPtr->SchedLockTimeMax < (CPU_TS)OSSchedLockTimeMaxCur) {
    OSTCBCurPtr->SchedLockTimeMax = (CPU_TS)OSSchedLockTimeMaxCur;
  }
  OSSchedLockTimeMaxCur = (CPU_TS)0;                            // Reset the per-task value
#endif
}

/****************************************************************************************************//**
 *                                               OSTimeTickHook()
 *
 * @brief    Called upon every elapsed tick.
 *
 * @note     (1) This function is assumed to be called from the Tick ISR.
 *
 * @note     (2) This function is deprecated and will be removed in an upcoming release.
 *******************************************************************************************************/
void OSTimeTickHook(void)
{
#if OS_CFG_APP_HOOKS_EN > 0u
  if (OS_AppTimeTickHookPtr != (OS_APP_HOOK_VOID)0) {
    (*OS_AppTimeTickHookPtr)();
  }
#endif
}

/****************************************************************************************************//**
 *                                               OSStartHighRdy()
 *
 * @brief    This function is called by OSStart() to start the highest priority task that was created
 *           by your application before calling OSStart().
 *******************************************************************************************************/
void OSStartHighRdy(void)
{
  OS_TASK  *p_task;
  OS_TCB   *p_tcb;
  RTOS_ERR err;
  CPU_SR_ALLOC();

  OSTaskSwHook();

  p_task = OSTaskGet(OSTCBHighRdyPtr);
  ResumeThread(p_task->ThreadHandle);
  //                                                               Wait while task is created and ready to run.
  SignalObjectAndWait(p_task->SignalPtr, p_task->InitSignalPtr, INFINITE, FALSE);
  ResumeThread(OSTick_Thread);                                  // Start OSTick Thread.
  WaitForSingleObject(OSTick_Thread, INFINITE);                 // Wait until OSTick Thread has terminated.

#if (OS_CFG_TIMER_METHOD_WIN32 == WIN32_MM_TMR)
  timeKillEvent(OSTick_TimerId);
  timeEndPeriod(OSTick_TimerCap.wPeriodMin);
  CloseHandle(OSTick_SignalPtr);
#endif

  CloseHandle(OSTick_Thread);
  CloseHandle(OSTerminate_SignalPtr);

#ifdef OS_CFG_MSG_TRACE_EN
  OS_Printf("\nDeleting all tasks...\n");
#endif
  //                                                               Delete all created tasks/threads.
  OSSchedLock(&err);

  CPU_CRITICAL_ENTER();
  p_task = OSTaskListPtr;
  while (p_task != NULL) {
    p_tcb = p_task->OSTCBPtr;
    p_task = p_task->NextPtr;

    if (p_tcb == &OSIdleTaskTCB) {
      OSTaskDelHook(p_tcb);
    } else {
      OSTaskDel(p_tcb, &err);
    }

    Sleep(1);                                                   // Allow thread to be deleted.
  }
  CPU_CRITICAL_EXIT();

#if 0                                                           // Prevent scheduler from running when exiting app.
  OSSchedUnlock(&err);
#endif

  // CPU_IntEnd();                                                 // Delete Critical Section objects.
}

/****************************************************************************************************//**
 *                                                   OSCtxSw()
 *
 * @brief    This function is called when a task makes a higher priority task ready-to-run.
 *******************************************************************************************************/
void OSCtxSw(void)
{
  OS_TASK *p_task_cur;
  OS_TASK *p_task_new;
#ifdef OS_CFG_MSG_TRACE_EN
  OS_TCB *p_tcb_cur;
  OS_TCB *p_tcb_new;
#endif
  CPU_SR_ALLOC();

#if (CPU_CFG_CRITICAL_METHOD == CPU_CRITICAL_METHOD_STATUS_LOCAL)
  cpu_sr = 0;
#endif

#ifdef OS_CFG_MSG_TRACE_EN
  p_tcb_cur = OSTCBCurPtr;
  p_tcb_new = OSTCBHighRdyPtr;
#endif

  p_task_cur = OSTaskGet(OSTCBCurPtr);

  OSTaskSwHook();

  OSTCBCurPtr = OSTCBHighRdyPtr;
  OSPrioCur = OSPrioHighRdy;

  if (p_task_cur->TaskState == STATE_RUNNING) {
    p_task_cur->TaskState = STATE_SUSPENDED;
  }
  p_task_new = OSTaskGet(OSTCBHighRdyPtr);
  switch (p_task_new->TaskState) {
    case STATE_CREATED:                                         // TaskState updated to STATE_RUNNING once thread runs.
      ResumeThread(p_task_new->ThreadHandle);
      //                                                           Wait while task is created and ready to run.
      SignalObjectAndWait(p_task_new->SignalPtr, p_task_new->InitSignalPtr, INFINITE, FALSE);
      break;

    case STATE_SUSPENDED:
      p_task_new->TaskState = STATE_RUNNING;
      SetEvent(p_task_new->SignalPtr);
      break;

    case STATE_INTERRUPTED:
      p_task_new->TaskState = STATE_RUNNING;
      ResumeThread(p_task_new->ThreadHandle);
      break;

#ifdef OS_CFG_MSG_TRACE_EN
    case STATE_NONE:
      OS_Printf("[OSCtxSw] Error: Invalid state STATE_NONE\nCur    Task[%3.1d] Thread ID %5.0d: '%s'\nNew    Task[%3.1d] Thread ID %5.0d: '%s'\n\n",
                p_tcb_cur->Prio,
                p_task_cur->ThreadID,
                p_task_cur->OSTaskName,
                p_tcb_new->Prio,
                p_task_new->ThreadID,
                p_task_new->OSTaskName);
      return;

    case STATE_RUNNING:
      OS_Printf("[OSCtxSw] Error: Invalid state STATE_RUNNING\nCur    Task[%3.1d] Thread ID %5.0d: '%s'\nNew    Task[%3.1d] Thread ID %5.0d: '%s'\n\n",
                p_tcb_cur->Prio,
                p_task_cur->ThreadID,
                p_task_cur->OSTaskName,
                p_tcb_new->Prio,
                p_task_new->ThreadID,
                p_task_new->OSTaskName);
      return;

    case STATE_TERMINATING:
      OS_Printf("[OSCtxSw] Error: Invalid state STATE_TERMINATING\nCur    Task[%3.1d] Thread ID %5.0d: '%s'\nNew    Task[%3.1d] Thread ID %5.0d: '%s'\n\n",
                p_tcb_cur->Prio,
                p_task_cur->ThreadID,
                p_task_cur->OSTaskName,
                p_tcb_new->Prio,
                p_task_new->ThreadID,
                p_task_new->OSTaskName);
      return;

    case STATE_TERMINATED:
      OS_Printf("[OSCtxSw] Error: Invalid state STATE_TERMINATED\nCur    Task[%3.1d] Thread ID %5.0d: '%s'\nNew    Task[%3.1d] Thread ID %5.0d: '%s'\n\n",
                p_tcb_cur->Prio,
                p_task_cur->ThreadID,
                p_task_cur->OSTaskName,
                p_tcb_new->Prio,
                p_task_new->ThreadID,
                p_task_new->OSTaskName);
      return;

#endif
    default:
      return;
  }

  if (p_task_cur->TaskState == STATE_TERMINATING) {
    OSTaskTerminate(p_task_cur);

    CPU_CRITICAL_EXIT();

    ExitThread(THREAD_EXIT_CODE);                               // ExitThread() never returns.
    return;
  }
  CPU_CRITICAL_EXIT();
  WaitForSingleObject(p_task_cur->SignalPtr, INFINITE);
  CPU_CRITICAL_ENTER();
}

/****************************************************************************************************//**
 *                                               OSIntCtxSw()
 *
 * @brief    This function is called by OSIntExit() to perform a context switch from an ISR.
 *******************************************************************************************************/
void OSIntCtxSw(void)
{
  OSTaskSwHook();

  OSTCBCurPtr = OSTCBHighRdyPtr;
  OSPrioCur = OSPrioHighRdy;
}

/****************************************************************************************************//**
 *                                           OSIntCurTaskSuspend()
 *
 * @brief    This function suspends current task for context switch.
 *
 * @return   DEF_TRUE,   current task     suspended successfully.
 *           DEF_FALSE   current task NOT suspended.
 *
 * @note     (1) Current task MUST be suspended before OSIntEnter().
 *
 * @note     (2) Suspending current task before OSIntEnter() and resuming it after OSIntExit() prevents
 *               task-level code to run concurrently with ISR-level code
 *******************************************************************************************************/
CPU_BOOLEAN OSIntCurTaskSuspend(void)
{
  OS_TCB      *p_tcb;
  OS_TASK     *p_task;
  CPU_BOOLEAN ret;

  p_tcb = OSTCBCurPtr;
  p_task = OSTaskGet(p_tcb);
  switch (p_task->TaskState) {
    case STATE_RUNNING:
      SuspendThread(p_task->ThreadHandle);
      SwitchToThread();

      p_task->TaskState = STATE_INTERRUPTED;

      ret = DEF_TRUE;
      break;

    case STATE_TERMINATING:                                     // Task terminated (run-to-completion or deleted itself).
      TerminateThread(p_task->ThreadHandle, THREAD_EXIT_CODE);
      CloseHandle(p_task->ThreadHandle);

      OSTaskTerminate(p_task);

      ret = DEF_TRUE;
      break;

#ifdef OS_CFG_MSG_TRACE_EN
    case STATE_NONE:
      OS_Printf("[OSIntCtxSw Suspend] Error: Invalid state STATE_NONE\nCur    Task[%3.1d] '%s' Thread ID %5.0d\n",
                p_tcb->Prio,
                p_task->OSTaskName,
                p_task->ThreadID);

      ret = DEF_FALSE;
      break;

    case STATE_CREATED:
      OS_Printf("[OSIntCtxSw Suspend] Error: Invalid state STATE_CREATED\nCur    Task[%3.1d] '%s' Thread ID %5.0d\n",
                p_tcb->Prio,
                p_task->OSTaskName,
                p_task->ThreadID);

      ret = DEF_FALSE;
      break;

    case STATE_INTERRUPTED:
      OS_Printf("[OSIntCtxSw Suspend] Error: Invalid state STATE_INTERRUPTED\nCur    Task[%3.1d] '%s' Thread ID %5.0d\n",
                p_tcb->Prio,
                p_task->OSTaskName,
                p_task->ThreadID);

      ret = DEF_FALSE;
      break;

    case STATE_SUSPENDED:
      OS_Printf("[OSIntCtxSw Suspend] Error: Invalid state STATE_SUSPENDED\nCur    Task[%3.1d] '%s' Thread ID %5.0d\n",
                p_tcb->Prio,
                p_task->OSTaskName,
                p_task->ThreadID);

      ret = DEF_FALSE;
      break;

    case STATE_TERMINATED:
      OS_Printf("[OSIntCtxSw Suspend] Error: Invalid state STATE_TERMINATED\nCur    Task[%3.1d] '%s' Thread ID %5.0d\n",
                p_tcb->Prio,
                p_task->OSTaskName,
                p_task->ThreadID);

      ret = DEF_FALSE;
      break;

#endif
    default:
      ret = DEF_FALSE;
      break;
  }

  return (ret);
}

/****************************************************************************************************//**
 *                                           OSIntCurTaskResume()
 *
 * @brief    This function resumes current task for context switch.
 *
 * @return   DEF_TRUE,   current task     resumed successfully.
 *           DEF_FALSE   current task NOT resumed.
 *
 * @note     (1) Current task MUST be resumed after OSIntExit().
 *
 * @note     (2) Suspending current task before OSIntEnter() and resuming it after OSIntExit() prevents
 *               task-level code to run concurrently with ISR-level code
 *******************************************************************************************************/
CPU_BOOLEAN OSIntCurTaskResume(void)
{
  OS_TCB      *p_tcb;
  OS_TASK     *p_task;
  CPU_BOOLEAN ret;

  p_tcb = OSTCBHighRdyPtr;
  p_task = OSTaskGet(p_tcb);
  switch (p_task->TaskState) {
    case STATE_CREATED:
      ResumeThread(p_task->ThreadHandle);
      //                                                           Wait while task is created and ready to run.
      SignalObjectAndWait(p_task->SignalPtr, p_task->InitSignalPtr, INFINITE, FALSE);
      ret = DEF_TRUE;
      break;

    case STATE_INTERRUPTED:
      p_task->TaskState = STATE_RUNNING;
      ResumeThread(p_task->ThreadHandle);
      ret = DEF_TRUE;
      break;

    case STATE_SUSPENDED:
      p_task->TaskState = STATE_RUNNING;
      SetEvent(p_task->SignalPtr);
      ret = DEF_TRUE;
      break;

#ifdef OS_CFG_MSG_TRACE_EN
    case STATE_NONE:
      OS_Printf("[OSIntCtxSw Resume] Error: Invalid state STATE_NONE\nNew    Task[%3.1d] '%s' Thread ID %5.0d\n",
                p_tcb->Prio,
                p_task->OSTaskName,
                p_task->ThreadID);
      ret = DEF_FALSE;
      break;

    case STATE_RUNNING:
      OS_Printf("[OSIntCtxSw Resume] Error: Invalid state STATE_RUNNING\nNew    Task[%3.1d] '%s' Thread ID %5.0d\n",
                p_tcb->Prio,
                p_task->OSTaskName,
                p_task->ThreadID);
      ret = DEF_FALSE;
      break;

    case STATE_TERMINATING:
      OS_Printf("[OSIntCtxSw Resume] Error: Invalid state STATE_TERMINATING\nNew    Task[%3.1d] '%s' Thread ID %5.0d\n",
                p_tcb->Prio,
                p_task->OSTaskName,
                p_task->ThreadID);
      ret = DEF_FALSE;
      break;

    case STATE_TERMINATED:
      OS_Printf("[OSIntCtxSw Resume] Error: Invalid state STATE_TERMINATED\nNew    Task[%3.1d] '%s' Thread ID %5.0d\n",
                p_tcb->Prio,
                p_task->OSTaskName,
                p_task->ThreadID);
      ret = DEF_FALSE;
      break;

#endif
    default:
      ret = DEF_FALSE;
      break;
  }

  return (ret);
}

/********************************************************************************************************
 ********************************************************************************************************
 *                                               LOCAL FUNCTIONS
 ********************************************************************************************************
 *******************************************************************************************************/

/****************************************************************************************************//**
 *                                               OSTickW32()
 *
 * @brief    This function is the Win32 task that generates the tick interrupts for the Kernel.
 *
 * @param    p_arg   Pointer to argument of the task.
 *
 * @note     (1) Priorities of these tasks are very important.
 *******************************************************************************************************/
static DWORD WINAPI OSTickW32(LPVOID p_arg)
{
  CPU_BOOLEAN terminate;
  CPU_BOOLEAN suspended;
#if (OS_CFG_TIMER_METHOD_WIN32 == WIN32_MM_TMR)
  HANDLE wait_signal[2];
#endif
  CPU_SR_ALLOC();

#if (OS_CFG_TIMER_METHOD_WIN32 == WIN32_MM_TMR)
  wait_signal[0] = OSTerminate_SignalPtr;
  wait_signal[1] = OSTick_SignalPtr;
#endif

  (void)p_arg;                                                  // Prevent compiler warning

  terminate = DEF_FALSE;
  while (!terminate) {
#if   (OS_CFG_TIMER_METHOD_WIN32 == WIN32_MM_TMR)
    switch (WaitForMultipleObjects(2, wait_signal, FALSE, INFINITE)) {
      case WAIT_OBJECT_0 + 1u:
        ResetEvent(OSTick_SignalPtr);
#elif (OS_CFG_TIMER_METHOD_WIN32 == WIN32_SLEEP)
    switch (WaitForSingleObject(OSTerminate_SignalPtr, 1000u / OSCfg_TickRate_Hz)) {
      case WAIT_TIMEOUT:
#endif
        CPU_CRITICAL_ENTER();

        suspended = OSIntCurTaskSuspend();
        if (suspended == DEF_TRUE) {
          OSIntEnter();
          OSTimeTick();
          OSIntExit();
          OSIntCurTaskResume();
        }

        CPU_CRITICAL_EXIT();
        break;

      case WAIT_OBJECT_0 + 0u:
        terminate = DEF_TRUE;
        break;

      default:
#ifdef OS_CFG_MSG_TRACE_EN
        OS_Printf("Thread    '%-32s' Error: Invalid signal.\n", "OSTickW32");
#endif
        terminate = DEF_TRUE;
        break;
    }
  }

#ifdef OS_CFG_MSG_TRACE_EN
  OS_Printf("Thread    '%-32s' Terminated.\n", "OSTickW32");
#endif

  return (0u);
}

/****************************************************************************************************//**
 *                                               OSTaskW32()
 *
 * @brief    This function is a generic Win32 task wrapper for the created tasks.
 *
 * @param    p_arg   Pointer to argument of the task.
 *
 * @note     (1) Priorities of these tasks are very important.
 *******************************************************************************************************/
static DWORD WINAPI OSTaskW32(LPVOID p_arg)
{
  OS_TASK  *p_task;
  OS_TCB   *p_tcb;
  RTOS_ERR err;

  p_tcb = (OS_TCB *)p_arg;
  p_task = OSTaskGet(p_tcb);

  p_task->TaskState = STATE_SUSPENDED;
  WaitForSingleObject(p_task->SignalPtr, INFINITE);

  OSSetThreadName(p_task->ThreadID, p_task->OSTaskName);

#ifdef OS_CFG_MSG_TRACE_EN
  OS_Printf("Task[%3.1d] '%-32s' Running\n",
            p_tcb->Prio,
            p_task->OSTaskName);
#endif

  p_task->TaskState = STATE_RUNNING;
  SetEvent(p_task->InitSignalPtr);                              // Indicate task has initialized successfully.

  p_task->TaskPtr(p_task->TaskArgPtr);

  OSTaskDel(p_tcb, &err);                                       // Thread may exit at OSCtxSw().

  return (0u);
}

/****************************************************************************************************//**
 *                                               OSTaskGet()
 *
 * @brief    This function retrieve the task information structure associated with a task control block.
 *
 * @param    p_tcb   Pointer to the task control block to retrieve the task information structure.
 *
 * @return   Task information.
 *******************************************************************************************************/
static OS_TASK *OSTaskGet(OS_TCB *p_tcb)
{
  OS_TASK *p_task;

  p_task = (OS_TASK *)p_tcb->StkPtr;                            // Ptr to task info struct is stored into TCB's .StkPtr.
  if (p_task != NULL) {
    return (p_task);
  }

  p_task = OSTaskListPtr;                                       // Task info struct not in TCB's .StkPtr.
  while (p_task != NULL) {                                      // Search all tasks.
    if (p_task->OSTCBPtr == p_tcb) {
      return (p_task);
    }
    p_task = p_task->NextPtr;
  }

  return (NULL);
}

/****************************************************************************************************//**
 *                                               OSTaskTerminate()
 *
 * @brief    This function handles task termination control signals.
 *
 * @param    p_task  Pointer to the task information structure of the task to clear its control
 *                   signals.
 *******************************************************************************************************/
static void OSTaskTerminate(OS_TASK *p_task)
{
#ifdef OS_CFG_MSG_TRACE_EN
  OS_TCB *p_tcb;
#endif
  OS_TASK *p_task_next;
  OS_TASK *p_task_prev;
  CPU_SR_ALLOC();

#ifdef OS_CFG_MSG_TRACE_EN
  p_tcb = p_task->OSTCBPtr;
  if (p_tcb->Prio != OS_PRIO_INIT) {
    OS_Printf("Task[%3.1d] '%-32s' Deleted\n",
              p_tcb->Prio,
              p_task->OSTaskName);
  } else {
    OS_Printf("Task      '%-32s' Deleted\n",
              p_task->OSTaskName);
  }
#endif
  CloseHandle(p_task->InitSignalPtr);
  CloseHandle(p_task->SignalPtr);

  p_task->OSTCBPtr = NULL;
  p_task->OSTaskName = NULL;
  p_task->TaskArgPtr = NULL;
  p_task->TaskOpt = OS_OPT_NONE;
  p_task->TaskPtr = NULL;
  p_task->TaskState = STATE_TERMINATED;
  p_task->ThreadID = 0u;
  p_task->ThreadHandle = NULL;
  p_task->InitSignalPtr = NULL;
  p_task->SignalPtr = NULL;

  CPU_CRITICAL_ENTER();
  p_task_prev = p_task->PrevPtr;
  p_task_next = p_task->NextPtr;

  if (p_task_prev == (OS_TASK *)0) {
    OSTaskListPtr = p_task_next;
    if (p_task_next != (OS_TASK *)0) {
      p_task_next->PrevPtr = (OS_TASK *)0;
    }
    p_task->NextPtr = (OS_TASK *)0;
  } else if (p_task_next == (OS_TASK *)0) {
    p_task_prev->NextPtr = (OS_TASK *)0;
    p_task->PrevPtr = (OS_TASK *)0;
  } else {
    p_task_prev->NextPtr = p_task_next;
    p_task_next->PrevPtr = p_task_prev;
    p_task->NextPtr = (OS_TASK *)0;
    p_task->PrevPtr = (OS_TASK *)0;
  }
  CPU_CRITICAL_EXIT();
}

/****************************************************************************************************//**
 *                                           OSCtrlBreakHandler()
 *
 * @brief    This function handles control signals sent to the console window.
 *
 * @param    ctrl    Control signal type.
 *
 * @return   TRUE,   control signal was     handled.
 *           FALSE   control signal was NOT handled.
 *******************************************************************************************************/
static BOOL WINAPI OSCtrlBreakHandler(DWORD ctrl)
{
  BOOL ret;

  ret = FALSE;

  switch (ctrl) {
    case CTRL_C_EVENT:                                          // CTRL-C pressed.
    case CTRL_BREAK_EVENT:                                      // CTRL-BREAK pressed.
    case CTRL_CLOSE_EVENT:                                      // Console window is closing.
    case CTRL_LOGOFF_EVENT:                                     // Logoff has started.
    case CTRL_SHUTDOWN_EVENT:                                   // System shutdown in process.
#ifdef OS_CFG_MSG_TRACE_EN
      OS_Printf("\nTerminating Scheduler...\n");
#endif
      SetEvent(OSTerminate_SignalPtr);

      if (ctrl == CTRL_CLOSE_EVENT) {
        Sleep(500);                                             // Give a chance to OSTickW32 to terminate.
      } else {
        ret = TRUE;
      }
      break;

    default:
      break;
  }

  return (ret);
}

/****************************************************************************************************//**
 *                                               OS_Printf()
 *
 * @brief    This function is analog of printf.
 *
 * @param    p_str   Pointer to format string output.
 *
 * @return   Number of characters written.
 *******************************************************************************************************/

#ifdef OS_CFG_MSG_TRACE_EN
static int OS_Printf(char *p_str, ...)
{
  va_list param;
  int     ret;

  va_start(param, p_str);
#ifdef _MSC_VER
  ret = vprintf_s(p_str, param);
#else
  ret = vprintf(p_str, param);
#endif
  va_end(param);

  return (ret);
}
#endif

/****************************************************************************************************//**
 *                                               OSDebuggerBreak()
 *
 * @brief    This function throws a breakpoint exception when a debugger is present.
 *******************************************************************************************************/
void OSDebuggerBreak(void)
{
#ifdef _MSC_VER
  __try {
    DebugBreak();
  }
  __except (GetExceptionCode() == EXCEPTION_BREAKPOINT
            ? EXCEPTION_EXECUTE_HANDLER
            : EXCEPTION_CONTINUE_SEARCH) {
    return;
  }
#else
#ifdef _DEBUG
  DebugBreak();
#endif
#endif
}

/****************************************************************************************************//**
 *                                               OSSetThreadName()
 *
 * @brief    This function sets thread names.
 *
 * @param    thread_id   Thread ID.
 *
 * @param    p_name      Pointer to name of the thread string.
 *******************************************************************************************************/
static void OSSetThreadName(DWORD thread_id, CPU_CHAR *p_name)
{
#ifdef _MSC_VER
  THREADNAME_INFO info;

  info.dwType = (DWORD)0x1000u;
  info.szName = (LPCSTR)p_name;
  info.dwThreadID = (DWORD)thread_id;
  info.dwFlags = (DWORD)0u;

  __try {
    RaiseException(MS_VC_EXCEPTION, 0u, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR *)&info);
  }
  __except (EXCEPTION_EXECUTE_HANDLER) {
  }
#endif
}

#ifdef __cplusplus
}
#endif
