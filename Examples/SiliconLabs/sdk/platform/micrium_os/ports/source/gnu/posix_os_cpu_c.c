/***************************************************************************//**
 * @file
 * @brief Kernel - POSIX Emulation Port
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
 *                   Core      : POSIX
 *                   Mode      :
 *                   Toolchain : GNU C Compiler
 *
 * @note     (2) This port is an emulation support port.
 *******************************************************************************************************/

#define   OS_CPU_GLOBALS
#define  _GNU_SOURCE

#ifdef VSC_INCLUDE_SOURCE_FILE_NAMES
const CPU_CHAR *os_cpu_c__c = "$Id: $";
#endif

/********************************************************************************************************
 *                                               INCLUDE FILES
 *******************************************************************************************************/

#include  <kernel/include/os.h>

#include  <common/include/rtos_path.h>
#include  <os_cfg.h>

#include  <stdio.h>
#include  <pthread.h>
#include  <stdint.h>
#include  <signal.h>
#include  <semaphore.h>
#include  <time.h>
#include  <string.h>
#include  <unistd.h>
#include  <stdlib.h>
#include  <sys/types.h>
#include  <sys/syscall.h>
#include  <sys/resource.h>
#include  <errno.h>

#ifdef __cplusplus
extern  "C" {
#endif

/********************************************************************************************************
 *                                               LOCAL DEFINES
 *******************************************************************************************************/

#define  THREAD_CREATE_PRIO       50u                           // Tasks underlying posix threads prio.

//                                                                 Err handling convenience macro.
#define  ERR_CHK(func)                                                                          \
  do {                                                                                          \
    int res = func;                                                                             \
    if (res != 0u) {                                                                            \
      printf("Error in call '%s' from %s(): %s\r\n", #func, __FUNCTION__, strerror(res));       \
      perror(" \\->'errno' indicates (might not be relevant if function doesn't use 'errno')"); \
      raise(SIGABRT);                                                                           \
    }                                                                                           \
  } while (0)

/********************************************************************************************************
 *                                           LOCAL DATA TYPES
 *******************************************************************************************************/

typedef struct os_tcb_ext_posix {
  pthread_t Thread;
  pid_t     ProcessId;
  sem_t     InitSem;
  sem_t     Sem;
} OS_TCB_EXT_POSIX;

/********************************************************************************************************
 *                                       LOCAL FUNCTION PROTOTYPES
 *******************************************************************************************************/

static void *OSTaskPosix(void *p_arg);

static void OSTaskTerminate(OS_TCB *p_tcb);

static void OSThreadCreate(pthread_t *p_thread,
                           void      *p_task,
                           void      *p_arg,
                           int       prio);

static void OSTimeTickHandler(void);

/********************************************************************************************************
 *                                           LOCAL VARIABLES
 *******************************************************************************************************/

//                                                                 Tick timer cfg.
static CPU_TMR_INTERRUPT OSTickTmrInterrupt = { .Interrupt.NamePtr = "Tick tmr interrupt",
                                                .Interrupt.Prio = 10u,
                                                .Interrupt.TraceEn = DEF_DISABLED,
                                                .Interrupt.ISR_Fnct = OSTimeTickHandler,
                                                .Interrupt.En = DEF_ENABLED,
                                                .OneShot = DEF_NO,
                                                .PeriodSec = 0u,
                                                .PeriodMuSec = 0u };

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

  sleep(1u);                                                    // Reduce CPU utilization.
}

/****************************************************************************************************//**
 *                                               OSInitHook()
 *
 * @brief    Called by OSInit() at the beginning of OSInit().
 *
 * @note     (1) When using hardware floating point, follow these steps during the reset handler:
 *               - (a) Set full access for CP10 and CP11 bits in CPACR register.
 *               - (b) Set bits ASPEN and LSPEN in FPCCR register.
 *******************************************************************************************************/
void OSInitHook(void)
{
  struct rlimit rtprio_limits;

  ERR_CHK(getrlimit(RLIMIT_RTPRIO, &rtprio_limits));
  if (rtprio_limits.rlim_cur != RLIM_INFINITY) {
    printf("Error: RTPRIO limit is too low. Set to 'unlimited' via 'ulimit -r' or /etc/security/limits.conf\r\n");
    exit(-1);
  }

  CPU_IntInit();                                                // Initialize critical section objects.
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
  if (OS_AppRedzoneHitHookPtr != DEF_NULL) {
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
  OS_TCB_EXT_POSIX *p_tcb_ext;
  int              ret;

#if OS_CFG_APP_HOOKS_EN > 0u
  if (OS_AppTaskCreateHookPtr != (OS_APP_HOOK_TCB)0) {
    (*OS_AppTaskCreateHookPtr)(p_tcb);
  }
#endif

  p_tcb_ext = malloc(sizeof(OS_TCB_EXT_POSIX));
  p_tcb->ExtPtr = p_tcb_ext;

  ERR_CHK(sem_init(&p_tcb_ext->InitSem, 0u, 0u));
  ERR_CHK(sem_init(&p_tcb_ext->Sem, 0u, 0u));

  OSThreadCreate(&p_tcb_ext->Thread, OSTaskPosix, p_tcb, THREAD_CREATE_PRIO);

  do {
    ret = sem_wait(&p_tcb_ext->InitSem);                        // Wait for init.
    if (ret != 0 && errno != EINTR) {
      raise(SIGABRT);
    }
  } while (ret != 0);
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
  OS_TCB_EXT_POSIX *p_tcb_ext = (OS_TCB_EXT_POSIX *)p_tcb->ExtPtr;
  pthread_t        self;
  CPU_BOOLEAN      same;

#if OS_CFG_APP_HOOKS_EN > 0u
  if (OS_AppTaskDelHookPtr != (OS_APP_HOOK_TCB)0) {
    (*OS_AppTaskDelHookPtr)(p_tcb);
  }
#endif

  self = pthread_self();
  same = (pthread_equal(self, p_tcb_ext->Thread) != 0u);
  if (same != DEF_YES) {
    ERR_CHK(pthread_cancel(p_tcb_ext->Thread));
  }

  OSTaskTerminate(p_tcb);
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
  return (p_stk_base);
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
#if (OS_CFG_TASK_STK_REDZONE_EN == DEF_ENABLED)
  CPU_BOOLEAN stk_status;
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

#if (OS_CFG_TASK_STK_REDZONE_EN == DEF_ENABLED)
  //                                                               Check if stack overflowed.
  stk_status = OSTaskStkRedzoneChk(DEF_NULL);
  if (stk_status != DEF_OK) {
    OSRedzoneHitHook(OSTCBCurPtr);
  }
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
  OS_TCB_EXT_POSIX *p_tcb_ext;
  sigset_t         sig_set;
  int              signo;

  OSTaskSwHook();

  p_tcb_ext = (OS_TCB_EXT_POSIX *)OSTCBCurPtr->ExtPtr;

  CPU_INT_DIS();

  ERR_CHK(sem_post(&p_tcb_ext->Sem));

  ERR_CHK(sigemptyset(&sig_set));
  ERR_CHK(sigaddset(&sig_set, SIGTERM));
  ERR_CHK(sigwait(&sig_set, &signo));
}

/****************************************************************************************************//**
 *                                                   OSCtxSw()
 *
 * @brief    This function is called when a task makes a higher priority task ready-to-run.
 *******************************************************************************************************/
void OSCtxSw(void)
{
  OS_TCB_EXT_POSIX *p_tcb_ext_old;
  OS_TCB_EXT_POSIX *p_tcb_ext_new;
  int              ret;
  CPU_BOOLEAN      detach = DEF_NO;

  OSTaskSwHook();

  p_tcb_ext_new = (OS_TCB_EXT_POSIX *)OSTCBHighRdyPtr->ExtPtr;
  p_tcb_ext_old = (OS_TCB_EXT_POSIX *)OSTCBCurPtr->ExtPtr;

  if (OSTCBCurPtr->TaskState == OS_TASK_STATE_DEL) {
    detach = DEF_YES;
  }

  OSTCBCurPtr = OSTCBHighRdyPtr;
  OSPrioCur = OSPrioHighRdy;

  ERR_CHK(sem_post(&p_tcb_ext_new->Sem));

  if (detach == DEF_NO) {
    do {
      ret = sem_wait(&p_tcb_ext_old->Sem);
      if (ret != 0 && errno != EINTR) {
        raise(SIGABRT);
      }
    } while (ret != 0);
  }
}

/****************************************************************************************************//**
 *                                               OSIntCtxSw()
 *
 * @brief    This function is called by OSIntExit() to perform a context switch from an ISR.
 *******************************************************************************************************/
void OSIntCtxSw(void)
{
  if (OSTCBCurPtr != OSTCBHighRdyPtr) {
    OSCtxSw();
  }
}

/****************************************************************************************************//**
 *                                           OS_CPU_SysTickInit()
 *
 * @brief    Initialize the system tick.
 *******************************************************************************************************/
void OS_CPU_SysTickInit(void)
{
#if (OS_CFG_TASK_TICK_EN == DEF_ENABLED)
  OSTickTmrInterrupt.PeriodMuSec = (1000000u / OSCfg_TickRate_Hz);
#else
  OSTickTmrInterrupt.PeriodMuSec = 0u;
#endif

  CPU_TmrInterruptCreate(&OSTickTmrInterrupt);
}

/********************************************************************************************************
 ********************************************************************************************************
 *                                               LOCAL FUNCTIONS
 ********************************************************************************************************
 *******************************************************************************************************/

/****************************************************************************************************//**
 *                                           OSTimeTickHandler()
 *
 * @brief    Handle Tick simulated interrupt.
 *******************************************************************************************************/
static void OSTimeTickHandler(void)
{
  OSIntEnter();
  OSTimeTick();
  CPU_ISR_End();
  OSIntExit();
}

/****************************************************************************************************//**
 *                                               OSTaskPosix()
 *
 * @brief    This function is a generic POSIX task wrapper for created tasks.
 *
 * @param    p_arg   Pointer to argument of the task's TCB.
 *
 * @note     (1) Priorities of these tasks are very important.
 *******************************************************************************************************/
static void *OSTaskPosix(void *p_arg)
{
  OS_TCB_EXT_POSIX *p_tcb_ext;
  OS_TCB           *p_tcb;
#if (OS_CFG_TASK_DEL_EN == DEF_ENABLED)
  RTOS_ERR err;
#endif

  p_tcb = (OS_TCB *)p_arg;
  p_tcb_ext = (OS_TCB_EXT_POSIX *)p_tcb->ExtPtr;

  p_tcb_ext->ProcessId = syscall(SYS_gettid);
  ERR_CHK(sem_post(&p_tcb_ext->InitSem));

#ifdef OS_CFG_MSG_TRACE_EN
  if (p_tcb->NamePtr != DEF_NULL) {
    printf("Task[%3.1d] '%-32s' running\n", p_tcb->Prio, p_tcb->NamePtr);
  }
#endif

  CPU_INT_DIS();
  {
    int ret = -1u;
    while (ret != 0u) {
      ret = sem_wait(&p_tcb_ext->Sem);                          // Wait until first CTX SW.
      if ((ret != 0) && (ret != -EINTR)) {
        ERR_CHK(ret);
      }
    }
  }
  CPU_INT_EN();

#if (OS_CFG_DBG_EN == DEF_ENABLED)
  ((void (*)(void *))p_tcb->TaskEntryAddr)(p_tcb->TaskEntryArg);
#endif

#if (OS_CFG_TASK_DEL_EN == DEF_ENABLED)
  OSTaskDel(p_tcb, &err);                                       // Thread may exit at OSCtxSw().
#endif

  return (0u);
}

/****************************************************************************************************//**
 *                                               OSTaskTerminate()
 *
 * @brief    This function handles task termination control signals.
 *
 * @param    p_task  Pointer to the task information structure of the task to clear its control
 *                   signals.
 *******************************************************************************************************/
static void OSTaskTerminate(OS_TCB *p_tcb)
{
#ifdef OS_CFG_MSG_TRACE_EN
  if (p_tcb->NamePtr != DEF_NULL) {
    printf("Task[%3.1d] '%-32s' deleted\n", p_tcb->Prio, p_tcb->NamePtr);
  }
#endif

  free(p_tcb->ExtPtr);
}

/****************************************************************************************************//**
 *                                               OSThreadCreate()
 *
 * @brief    Create new posix thread.
 *
 * @param    p_thread    Pointer to preallocated thread variable.
 *
 * @param    p_task      Pointer to associated function.
 *
 * @param    p_arg       Pointer to associated function's argument.
 *
 * @param    prio        Thread priority.
 *
 * @return   Thread's corresponding LWP pid.
 *******************************************************************************************************/
static void OSThreadCreate(pthread_t *p_thread,
                           void      *p_task,
                           void      *p_arg,
                           int       prio)
{
  pthread_attr_t     attr;
  struct sched_param param;

  if (prio < sched_get_priority_min(SCHED_RR)
      || prio > sched_get_priority_max(SCHED_RR)) {
#ifdef OS_CFG_MSG_TRACE_EN
    printf("ThreadCreate(): Invalid prio arg.\n");
#endif
    raise(SIGABRT);
  }

  ERR_CHK(pthread_attr_init(&attr));
  ERR_CHK(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED));
  param.__sched_priority = prio;
  ERR_CHK(pthread_attr_setschedpolicy(&attr, SCHED_RR));
  ERR_CHK(pthread_attr_setschedparam(&attr, &param));
  ERR_CHK(pthread_create(p_thread, &attr, p_task, p_arg));
}

#ifdef __cplusplus
}
#endif
