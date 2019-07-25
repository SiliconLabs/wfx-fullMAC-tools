/***************************************************************************//**
 * @file
 * @brief CPU - POSIX Emulation Port
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
 *
 * @note     (3) Requires a Single UNIX Specification, Version 3 compliant operating environment.
 *               On Linux _XOPEN_SOURCE must be defined to at least 600, generally by passing the
 *               -D_XOPEN_SOURCE=600 command line option to GCC.
 *******************************************************************************************************/

/********************************************************************************************************
 ********************************************************************************************************
 *                                               INCLUDE FILES
 ********************************************************************************************************
 *******************************************************************************************************/

#include  <pthread.h>
#include  <stdio.h>
#include  <stdarg.h>
#include  <stdlib.h>
#include  <string.h>
#include  <signal.h>
#include  <unistd.h>
#include  <stdlib.h>
#include  <sched.h>
#include  <errno.h>

#include  <common/source/rtos/rtos_utils_priv.h>
#include  <common/include/toolchains.h>
#include  <cpu/include/cpu.h>
#include  "armv7m_cpu_port.h"

#if  (_POSIX_C_SOURCE < 199309L)
# pragma message "_POSIX_C_SOURCE is less than 199309L. YMMV."
#endif

#if (CPU_CFG_TS_TMR_EN == DEF_ENABLED)
# include  <time.h>
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                               LOCAL DEFINES
 ********************************************************************************************************
 *******************************************************************************************************/

#define  RTOS_MODULE_CUR               RTOS_CFG_MODULE_CPU

#define  CPU_TMR_INT_TASK_PRIO         sched_get_priority_max(SCHED_RR) // Tmr interrupt task priority.
#define  CPU_IRQ_SIG                  (SIGURG)                          // IRQ trigger signal.

/********************************************************************************************************
 ********************************************************************************************************
 *                                           LOCAL DATA TYPES
 ********************************************************************************************************
 *******************************************************************************************************/

typedef struct cpu_interrupt_node CPU_INTERRUPT_NODE;

struct cpu_interrupt_node {
  CPU_INTERRUPT      *InterruptPtr;
  CPU_INTERRUPT_NODE *NextPtr;
};

/********************************************************************************************************
 ********************************************************************************************************
 *                                       LOCAL GLOBAL VARIABLES
 ********************************************************************************************************
 *******************************************************************************************************/

static pthread_mutex_t     CPU_InterruptQueueMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutexattr_t CPU_InterruptQueueMutexAttr;

static CPU_INTERRUPT_NODE *CPU_InterruptPendListHeadPtr;
static CPU_INTERRUPT_NODE *CPU_InterruptRunningListHeadPtr;

static sigset_t CPU_IRQ_SigMask;

/********************************************************************************************************
 ********************************************************************************************************
 *                                       LOCAL FUNCTION PROTOTYPES
 ********************************************************************************************************
 *******************************************************************************************************/

static void CPU_IRQ_Handler(int sig);

static void CPU_InterruptTriggerInternal(CPU_INTERRUPT *p_interrupt);

static void CPU_InterruptQueue(CPU_INTERRUPT *p_isr);

static void *CPU_TmrInterruptTask(void *p_arg);

static void CPU_ISR_Sched(void);

/********************************************************************************************************
 ********************************************************************************************************
 *                                           GLOBAL FUNCTIONS
 ********************************************************************************************************
 *******************************************************************************************************/

/****************************************************************************************************//**
 *                                               CPU_IntInit()
 *
 * @brief    This function initializes the critical section.
 *
 * @note    (1) CPU_IntInit() MUST be called prior to use any of the CPU_IntEn(), and CPU_IntDis()
 *               functions.
 *******************************************************************************************************/
void CPU_IntInit(void)
{
  struct sigaction on_isr_trigger_sig_action;
  int              res;

  CPU_InterruptPendListHeadPtr = DEF_NULL;
  CPU_InterruptRunningListHeadPtr = DEF_NULL;

  sigemptyset(&CPU_IRQ_SigMask);
  sigaddset(&CPU_IRQ_SigMask, CPU_IRQ_SIG);
  //                                                               Initialize the Mutex for the Interrupt lists.
  pthread_mutexattr_init(&CPU_InterruptQueueMutexAttr);
  pthread_mutexattr_settype(&CPU_InterruptQueueMutexAttr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&CPU_InterruptQueueMutex, &CPU_InterruptQueueMutexAttr);

  //                                                               Register interrupt trigger signal handler.
  memset(&on_isr_trigger_sig_action, 0, sizeof(on_isr_trigger_sig_action));
  res = sigemptyset(&on_isr_trigger_sig_action.sa_mask);
  RTOS_ASSERT_CRITICAL(res == 0u, RTOS_ERR_FAIL,; );

  on_isr_trigger_sig_action.sa_flags = SA_NODEFER;
  on_isr_trigger_sig_action.sa_handler = CPU_IRQ_Handler;
  res = sigaction(CPU_IRQ_SIG, &on_isr_trigger_sig_action, NULL);
  RTOS_ASSERT_CRITICAL(res == 0u, RTOS_ERR_FAIL,; );
}

/****************************************************************************************************//**
 *                                               CPU_IntDis()
 *
 * @brief    This function disables interrupts for critical sections of code.
 *******************************************************************************************************/
void CPU_IntDis(void)
{
  pthread_sigmask(SIG_BLOCK, &CPU_IRQ_SigMask, DEF_NULL);
}

/****************************************************************************************************//**
 *                                               CPU_IntEn()
 *
 * @brief    This function enables interrupts after critical sections of code.
 *******************************************************************************************************/
void CPU_IntEn(void)
{
  pthread_sigmask(SIG_UNBLOCK, &CPU_IRQ_SigMask, DEF_NULL);
}

/****************************************************************************************************//**
 *                                               CPU_ISR_End()
 *
 * @brief    Ends an ISR.
 *
 * @note     (1) This function MUST be called at the end of an ISR.
 *
 * @note     (2) The signal MUST be blocked before locking the mutex to prevent a ISR signal to be handled
 *               by this thread and having the mutex in an unknown state. The Mutex MUST then be released
 *               before unblocking the signals.
 *******************************************************************************************************/
void CPU_ISR_End(void)
{
  CPU_INTERRUPT_NODE *p_interrupt_node;

  CPU_INT_DIS();                                                // See Note #2.
  pthread_mutex_lock(&CPU_InterruptQueueMutex);
  RTOS_ASSERT_CRITICAL(CPU_InterruptRunningListHeadPtr != DEF_NULL, RTOS_ERR_FAIL,; );

  //                                                               Add current ISR in free list.
  p_interrupt_node = CPU_InterruptRunningListHeadPtr;
  CPU_InterruptRunningListHeadPtr = CPU_InterruptRunningListHeadPtr->NextPtr;
  pthread_mutex_unlock(&CPU_InterruptQueueMutex);
  CPU_INT_EN();                                                 // See Note #2.

  free(p_interrupt_node);

  CPU_ISR_Sched();
}

/****************************************************************************************************//**
 *                                           CPU_TmrInterruptCreate()
 *
 * @brief    Simulated hardware timer instance creation.
 *
 * @param    p_tmr_interrupt     Pointer to a timer interrupt descriptor.
 *******************************************************************************************************/
void CPU_TmrInterruptCreate(CPU_TMR_INTERRUPT *p_tmr_interrupt)
{
  pthread_t          thread;
  pthread_attr_t     attr;
  struct sched_param param;
  int                res;

  res = pthread_attr_init(&attr);
  RTOS_ASSERT_CRITICAL(res == 0u, RTOS_ERR_FAIL,; );

  res = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
  RTOS_ASSERT_CRITICAL(res == 0u, RTOS_ERR_FAIL,; );

  param.sched_priority = CPU_TMR_INT_TASK_PRIO;
  res = pthread_attr_setschedpolicy(&attr, SCHED_RR);
  RTOS_ASSERT_CRITICAL(res == 0u, RTOS_ERR_FAIL,; );

  res = pthread_attr_setschedparam(&attr, &param);
  RTOS_ASSERT_CRITICAL(res == 0u, RTOS_ERR_FAIL,; );

  res = pthread_create(&thread, &attr, CPU_TmrInterruptTask, p_tmr_interrupt);
  RTOS_ASSERT_CRITICAL(res == 0u, RTOS_ERR_FAIL,; );
}

/****************************************************************************************************//**
 *                                           CPU_InterruptTrigger()
 *
 * @brief    Queue an interrupt and send the IRQ signal.
 *
 * @param    p_interrupt     Interrupt to be queued.
 *
 * @note     (1) The Interrupt signal should not be blocked before calling this function since they will
 *               be blocked internally, otherwise, the internal called will be nested and the signals will
 *               no longer be blocked at the end of this function.
 *******************************************************************************************************/
void CPU_InterruptTrigger(CPU_INTERRUPT *p_interrupt)
{
  CPU_INT_DIS();
  CPU_InterruptTriggerInternal(p_interrupt);                    // Signal are now blocked: rest of Trigger is internal.
  CPU_INT_EN();
}

/****************************************************************************************************//**
 *                                               CPU_Printf()
 *
 * @brief    This function is analog of printf.
 *
 * @param    p_str   Pointer to format string output.
 *
 * @return   Number of characters written.
 *******************************************************************************************************/

#ifdef  CPU_CFG_MSG_TRACE_EN
static int CPU_Printf(char const *p_str, ...)
{
  va_list argp;
  int     n;

  va_start(argp, p_str);
  n = vfprintf(stdout, p_str, argp);
  va_end(argp);

  return (n);
}
#endif

/****************************************************************************************************//**
 *                                           CPU_CntLeadZeros()
 *
 * @brief    Count the number of contiguous, most-significant, leading zero bits in a data value.
 *
 * @param    val     Data value to count leading zero bits.
 *
 * @return   Number of contiguous, most-significant, leading zero bits in 'val', if NO error(s).
 *
 *           0,                                                                  otherwise.
 *
 * @note     (1) Supports 8, 16 and 32-bits value sizes :
 *               - (a) For  8-bit values :
 *                     @verbatim
 *                               b07  b06  b05  b04  b03  b02  b01  b00    # Leading Zeros
 *                               ---  ---  ---  ---  ---  ---  ---  ---    ---------------
 *                               1    x    x    x    x    x    x    x            0
 *                               0    1    x    x    x    x    x    x            1
 *                               0    0    1    x    x    x    x    x            2
 *                               0    0    0    1    x    x    x    x            3
 *                               0    0    0    0    1    x    x    x            4
 *                               0    0    0    0    0    1    x    x            5
 *                               0    0    0    0    0    0    1    x            6
 *                               0    0    0    0    0    0    0    1            7
 *                               0    0    0    0    0    0    0    0            8
 *                     @verbatim
 *               - (b) For 16-bit values :
 *                     @verbatim
 *                       b15  b14  b13  ...  b04  b03  b02  b01  b00    # Leading Zeros
 *                       ---  ---  ---       ---  ---  ---  ---  ---    ---------------
 *                           1    x    x         x    x    x    x    x            0
 *                           0    1    x         x    x    x    x    x            1
 *                           0    0    1         x    x    x    x    x            2
 *                           :    :    :         :    :    :    :    :            :
 *                           :    :    :         :    :    :    :    :            :
 *                           0    0    0         1    x    x    x    x           11
 *                           0    0    0         0    1    x    x    x           12
 *                           0    0    0         0    0    1    x    x           13
 *                           0    0    0         0    0    0    1    x           14
 *                           0    0    0         0    0    0    0    1           15
 *                           0    0    0         0    0    0    0    0           16
 *                     @endverbatim
 *               - (c) For 32-bit values :
 *                       @verbatim
 *                       b31  b30  b29  ...  b04  b03  b02  b01  b00    # Leading Zeros
 *                       ---  ---  ---       ---  ---  ---  ---  ---    ---------------
 *                           1    x    x         x    x    x    x    x            0
 *                           0    1    x         x    x    x    x    x            1
 *                           0    0    1         x    x    x    x    x            2
 *                           :    :    :         :    :    :    :    :            :
 *                           :    :    :         :    :    :    :    :            :
 *                           0    0    0         1    x    x    x    x           27
 *                           0    0    0         0    1    x    x    x           28
 *                           0    0    0         0    0    1    x    x           29
 *                           0    0    0         0    0    0    1    x           30
 *                           0    0    0         0    0    0    0    1           31
 *                           0    0    0         0    0    0    0    0           32
 *                       @endverbatim
 *               See also 'CPU COUNT LEAD ZEROs LOOKUP TABLE  Note #1'.
 *               See also 'cpu_def.h  CPU WORD CONFIGURATION  Note #1'.
 *
 * @note     (2) MUST be implemented in cpu_a.asm if and only if CPU_CFG_LEAD_ZEROS_ASM_PRESENT
 *               is #define'd in 'cpu_cfg.h' or 'cpu.h'.
 *******************************************************************************************************/

#ifdef  CPU_CFG_LEAD_ZEROS_ASM_PRESENT
CPU_DATA CPU_CntLeadZeros(CPU_DATA val)
{
}
#endif

/****************************************************************************************************//**
 *                                           CPU_CntTrailZeros()
 *
 * @brief    Count the number of contiguous, least-significant, trailing zero bits in a data value.
 *
 * @param    val     Data value to count trailing zero bits.
 *
 * @return   Number of contiguous, least-significant, trailing zero bits in 'val'.
 *
 * @note     (1) Supports the 8, 16, 32 and 64-bits data value sizes :
 *               - (a) For  8-bit values :
 *                       @verbatim
 *                               b07  b06  b05  b04  b03  b02  b01  b00    # Trailing Zeros
 *                               ---  ---  ---  ---  ---  ---  ---  ---    ----------------
 *                               x    x    x    x    x    x    x    1            0
 *                               x    x    x    x    x    x    1    0            1
 *                               x    x    x    x    x    1    0    0            2
 *                               x    x    x    x    1    0    0    0            3
 *                               x    x    x    1    0    0    0    0            4
 *                               x    x    1    0    0    0    0    0            5
 *                               x    1    0    0    0    0    0    0            6
 *                               1    0    0    0    0    0    0    0            7
 *                               0    0    0    0    0    0    0    0            8
 *                       @endverbatim
 *               - (b) For 16-bit values :
 *                       @verbatim
 *                       b15  b14  b13  b12  b11  ...  b02  b01  b00    # Trailing Zeros
 *                       ---  ---  ---  ---  ---       ---  ---  ---    ----------------
 *                           x    x    x    x    x         x    x    1            0
 *                           x    x    x    x    x         x    1    0            1
 *                           x    x    x    x    x         1    0    0            2
 *                           :    :    :    :    :         :    :    :            :
 *                           :    :    :    :    :         :    :    :            :
 *                           x    x    x    x    1         0    0    0           11
 *                           x    x    x    1    0         0    0    0           12
 *                           x    x    1    0    0         0    0    0           13
 *                           x    1    0    0    0         0    0    0           14
 *                           1    0    0    0    0         0    0    0           15
 *                           0    0    0    0    0         0    0    0           16
 *                       @endverbatim
 *               - (c) For 32-bit values :
 *                       @verbatim
 *                       b31  b30  b29  b28  b27  ...  b02  b01  b00    # Trailing Zeros
 *                       ---  ---  ---  ---  ---       ---  ---  ---    ----------------
 *                           x    x    x    x    x         x    x    1            0
 *                           x    x    x    x    x         x    1    0            1
 *                           x    x    x    x    x         1    0    0            2
 *                           :    :    :    :    :         :    :    :            :
 *                           :    :    :    :    :         :    :    :            :
 *                           x    x    x    x    1         0    0    0           27
 *                           x    x    x    1    0         0    0    0           28
 *                           x    x    1    0    0         0    0    0           29
 *                           x    1    0    0    0         0    0    0           30
 *                           1    0    0    0    0         0    0    0           31
 *                           0    0    0    0    0         0    0    0           32
 *                       @endverbatim
 *               - (d) For 64-bit values :
 *                       @verbatim
 *                       b63  b62  b61  b60  b59  ...  b02  b01  b00    # Trailing Zeros
 *                       ---  ---  ---  ---  ---       ---  ---  ---    ----------------
 *                           x    x    x    x    x         x    x    1            0
 *                           x    x    x    x    x         x    1    0            1
 *                           x    x    x    x    x         1    0    0            2
 *                           :    :    :    :    :         :    :    :            :
 *                           :    :    :    :    :         :    :    :            :
 *                           x    x    x    x    1         0    0    0           59
 *                           x    x    x    1    0         0    0    0           60
 *                           x    x    1    0    0         0    0    0           61
 *                           x    1    0    0    0         0    0    0           62
 *                           1    0    0    0    0         0    0    0           63
 *                           0    0    0    0    0         0    0    0           64
 *                       @endverbatim
 *                   See also 'cpu_def.h  CPU WORD CONFIGURATION  Note #1'.
 *
 * @note     (2) For non-zero values, the returned number of contiguous, least-significant, trailing
 *               zero bits is also equivalent to the bit position of the least-significant set bit.
 *
 * @note     (3) 'val' SHOULD be validated for non-'0' PRIOR to all other counting zero calculations :
 *               - (a) CPU_CntTrailZeros()'s final conditional statement calculates 'val's number of
 *                     trailing zeros based on its return data size, 'CPU_CFG_DATA_SIZE', & 'val's
 *                     calculated number of lead zeros ONLY if the initial 'val' is non-'0' :
 *                       @verbatim
 *                       if (val != 0u) {
 *                           nbr_trail_zeros = ((CPU_CFG_DATA_SIZE * DEF_OCTET_NBR_BITS) - 1u) - nbr_lead_zeros;
 *                       } else {
 *                           nbr_trail_zeros = nbr_lead_zeros;
 *                       }
 *                       @endverbatim
 *                     Therefore, initially validating all non-'0' values avoids having to conditionally
 *                     execute the final statement.
 *******************************************************************************************************/

#ifdef  CPU_CFG_TRAIL_ZEROS_ASM_PRESENT
CPU_DATA CPU_CntTrailZeros(CPU_DATA val)
{
}
#endif

/****************************************************************************************************//**
 *                                               CPU_TS_TmrInit()
 *
 * @brief    Initialize & start CPU timestamp timer.
 *
 * @note     (1) CPU_TS_TmrInit() is an application/BSP function that MUST be defined by the developer
 *               if either of the following CPU features is enabled :
 *               - (a) CPU timestamps
 *               - (b) CPU interrupts disabled time measurements
 *               @n
 *               See 'cpu_cfg.h  CPU TIMESTAMP CONFIGURATION  Note #1'
 *               & 'cpu_cfg.h  CPU INTERRUPTS DISABLED TIME MEASUREMENT CONFIGURATION  Note #1a'.
 *
 * @note     (2) Timer count values MUST be returned via word-size-configurable 'CPU_TS_TMR'
 *               data type.
 *               - (a) If timer has more bits, truncate timer values' higher-order bits greater
 *                     than the configured 'CPU_TS_TMR' timestamp timer data type word size.
 *               - (b) Since the timer MUST NOT have less bits than the configured 'CPU_TS_TMR'
 *                     timestamp timer data type word size; 'CPU_CFG_TS_TMR_SIZE' MUST be
 *                     configured so that ALL bits in 'CPU_TS_TMR' data type are significant.
 *                     In other words, if timer size is not a binary-multiple of 8-bit octets
 *                     (e.g. 20-bits or even 24-bits), then the next lower, binary-multiple
 *                     octet word size SHOULD be configured (e.g. to 16-bits).  However, the
 *                     minimum supported word size for CPU timestamp timers is 8-bits.
 *                     See also 'cpu_cfg.h   CPU TIMESTAMP CONFIGURATION  Note #2'
 *                            & 'cpu_core.h  CPU TIMESTAMP DATA TYPES     Note #1'.
 *               - (c) Timer SHOULD be an 'up'  counter whose values increase with each time count.
 *               - (d) When applicable, timer period SHOULD be less than the typical measured time
 *                     but MUST be less than the maximum measured time; otherwise, timer resolution
 *                     inadequate to measure desired times.
 *                     See also 'CPU_TS_TmrRd()  Note #2'.
 *******************************************************************************************************/

#if (CPU_CFG_TS_TMR_EN == DEF_ENABLED)
void CPU_TS_TmrInit(void)
{
  struct timespec res;

  res.tv_sec = 0;
  res.tv_nsec = 0;

  (void)clock_settime(CLOCK_MONOTONIC, &res);

  CPU_TS_TmrFreqSet(1000000000);
}
#endif

/****************************************************************************************************//**
 *                                               CPU_TS_TmrRd()
 *
 * @brief    Get current CPU timestamp timer count value.
 *
 * @return   Timestamp timer count (see Notes #2a & #2b).
 *
 * @note     (1) CPU_TS_TmrRd() is an application/BSP function that MUST be defined by the developer
 *               if either of the following CPU features is enabled :
 *               - (a) CPU timestamps
 *               - (b) CPU interrupts disabled time measurements
 *               See 'cpu_cfg.h  CPU TIMESTAMP CONFIGURATION  Note #1'
 *               & 'cpu_cfg.h  CPU INTERRUPTS DISABLED TIME MEASUREMENT CONFIGURATION  Note #1a'.
 *
 * @note     (2) Timer count values MUST be returned via word-size-configurable 'CPU_TS_TMR'
 *               data type.
 *               - (a) If timer has more bits, truncate timer values' higher-order bits greater
 *                     than the configured 'CPU_TS_TMR' timestamp timer data type word size.
 *               - (b) Since the timer MUST NOT have less bits than the configured 'CPU_TS_TMR'
 *                     timestamp timer data type word size; 'CPU_CFG_TS_TMR_SIZE' MUST be
 *                     configured so that ALL bits in 'CPU_TS_TMR' data type are significant.
 *                     In other words, if timer size is not a binary-multiple of 8-bit octets
 *                     (e.g. 20-bits or even 24-bits), then the next lower, binary-multiple
 *                     octet word size SHOULD be configured (e.g. to 16-bits).  However, the
 *                     minimum supported word size for CPU timestamp timers is 8-bits.
 *                     See also 'cpu_cfg.h   CPU TIMESTAMP CONFIGURATION  Note #2'
 *                            & 'cpu_core.h  CPU TIMESTAMP DATA TYPES     Note #1'.
 *               - (c) Timer SHOULD be an 'up'  counter whose values increase with each time count.
 *                   - (1) If timer is a 'down' counter whose values decrease with each time count,
 *                         then the returned timer value MUST be ones-complemented.
 *               - (d) When applicable, the amount of time measured by CPU timestamps is
 *                     calculated by either of the following equations :
 *                      @verbatim
 *                   - (1) Time measured  =  Number timer counts  *  Timer period
 *
 *                               where
 *
 *                                   Number timer counts     Number of timer counts measured
 *                                   Timer period            Timer's period in some units of
 *                                                               (fractional) seconds
 *                                   Time measured           Amount of time measured, in same
 *                                                               units of (fractional) seconds
 *                                                               as the Timer period
 *
 *                                               Number timer counts
 *                   - (2) Time measured  =  ---------------------
 *                                               Timer frequency
 *
 *                               where
 *
 *                                   Number timer counts     Number of timer counts measured
 *                                   Timer frequency         Timer's frequency in some units
 *                                                               of counts per second
 *                                   Time measured           Amount of time measured, in seconds
 *                     @endverbatim
 *               - (e) Timer period SHOULD be less than the typical measured time but MUST be less
 *                     than the maximum measured time; otherwise, timer resolution inadequate to
 *                     measure desired times.
 *******************************************************************************************************/

#if (CPU_CFG_TS_TMR_EN == DEF_ENABLED)
CPU_TS_TMR CPU_TS_TmrRd(void)
{
  struct timespec res;
  CPU_TS_TMR      ts;

  (void)clock_gettime(CLOCK_MONOTONIC, &res);

  ts = (CPU_TS_TMR)(res.tv_sec * 1000000000u + res.tv_nsec);

  return (ts);
}
#endif

#ifdef __cplusplus
}
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                               LOCAL FUNCTIONS
 ********************************************************************************************************
 *******************************************************************************************************/

/****************************************************************************************************//**
 *                                               CPU_IRQ_Handler()
 *
 * @brief    CPU_IRQ_SIG signal handler.
 *
 * @param    sig     Signal that triggered the handler (unused).
 *******************************************************************************************************/
static void CPU_IRQ_Handler(int sig)
{
  PP_UNUSED_PARAM(sig);

  CPU_ISR_Sched();
}

/****************************************************************************************************//**
 *                                       CPU_InterruptTriggerInternal()
 *
 * @brief    Queue an interrupt and send the IRQ signal.
 *
 * @param    p_interrupt     Interrupt to be queued.
 *
 * @note     (1) The Interrupt signal must be blocked before calling this function.
 *******************************************************************************************************/
void CPU_InterruptTriggerInternal(CPU_INTERRUPT *p_interrupt)
{
  if (p_interrupt->En) {
    CPU_InterruptQueue(p_interrupt);

    kill(getpid(), CPU_IRQ_SIG);

    if (p_interrupt->TraceEn == DEF_ENABLED) {
      struct timespec ts;
      clock_gettime(CLOCK_MONOTONIC, &ts);
      printf("@ %lu:%06lu", ts.tv_sec, ts.tv_nsec / 1000u);
      printf("  %s interrupt fired.\r\n", p_interrupt->NamePtr);
    }
  }
}

/****************************************************************************************************//**
 *                                           CPU_InterruptQueue()
 *
 * @brief    Queue an interrupt.
 *
 * @param    p_interrupt     Pointer to the interrupt to be queued.
 *
 * @note     (1) The signals for this thread are already blocked during this function call.
 *
 * @note     (2) Since the signal are already blocked, it is safe to lock and release the mutex.
 *******************************************************************************************************/
static void CPU_InterruptQueue(CPU_INTERRUPT *p_interrupt)
{
  CPU_INTERRUPT_NODE *p_cur_interrupt_node;
  CPU_INTERRUPT_NODE *p_prev_interrupt_node;
  CPU_INTERRUPT_NODE *p_interrupt_node;

  p_interrupt_node = (CPU_INTERRUPT_NODE *)malloc(sizeof(CPU_INTERRUPT_NODE));
  p_interrupt_node->InterruptPtr = p_interrupt;

  pthread_mutex_lock(&CPU_InterruptQueueMutex);                 // See Note #2.
  if ((CPU_InterruptPendListHeadPtr == DEF_NULL)                // Add Interrupt to the Head of the list.
      || (CPU_InterruptPendListHeadPtr->InterruptPtr->Prio < p_interrupt->Prio)) {
    p_interrupt_node->NextPtr = CPU_InterruptPendListHeadPtr;
    CPU_InterruptPendListHeadPtr = p_interrupt_node;
  } else {                                                      // Insert Interrupt in priority order.
    p_cur_interrupt_node = CPU_InterruptPendListHeadPtr;
    while ((p_cur_interrupt_node != DEF_NULL)
           && (p_cur_interrupt_node->InterruptPtr->Prio >= p_interrupt->Prio)) {
      p_prev_interrupt_node = p_cur_interrupt_node;
      p_cur_interrupt_node = p_cur_interrupt_node->NextPtr;
    }
    p_prev_interrupt_node->NextPtr = p_interrupt_node;
    p_interrupt_node->NextPtr = p_cur_interrupt_node;
  }
  pthread_mutex_unlock(&CPU_InterruptQueueMutex);               // See Note #2.
}

/****************************************************************************************************//**
 *                                               CPU_ISR_Sched()
 *
 * @brief    Schedules the highest priority pending interrupt.
 *
 * @note     (1) The signal MUST be blocked before locking the mutex to prevent a ISR signal to be handled
 *               by this thread and having the mutex in an unknown state. The Mutex MUST then be released
 *               before unblocking the signals.
 *******************************************************************************************************/
static void CPU_ISR_Sched(void)
{
  CPU_INTERRUPT_NODE *p_isr_node;

  CPU_INT_DIS();                                                // See Note #1.
  pthread_mutex_lock(&CPU_InterruptQueueMutex);
  p_isr_node = CPU_InterruptPendListHeadPtr;
  if ((p_isr_node != DEF_NULL)                                  // Add next ISR in free list, then handle its ISR_fnct.
      && ((CPU_InterruptRunningListHeadPtr == DEF_NULL)
          || (p_isr_node->InterruptPtr->Prio > CPU_InterruptRunningListHeadPtr->InterruptPtr->Prio))) {
    CPU_InterruptPendListHeadPtr = CPU_InterruptPendListHeadPtr->NextPtr;
    p_isr_node->NextPtr = CPU_InterruptRunningListHeadPtr;
    CPU_InterruptRunningListHeadPtr = p_isr_node;
    pthread_mutex_unlock(&CPU_InterruptQueueMutex);
    CPU_INT_EN();                                               // See Note #1.
    p_isr_node->InterruptPtr->ISR_Fnct();
  } else {
    pthread_mutex_unlock(&CPU_InterruptQueueMutex);
    CPU_INT_EN();                                               // See Note #1.
  }
}

/****************************************************************************************************//**
 *                                           CPU_TmrInterruptTask()
 *
 * @brief    Hardware timer interrupt simulation function.
 *
 * @param    p_arg   Pointer to a timer interrupt descriptor.
 *
 * @note     (1) Signals are already disabled in this task to prevent an ISR to be handled within this
 *               thread.
 *
 * @note     (2) Because signals are always disbaled, every Interrupt should be called with the Internal
 *               function to prevent signal to be unblocked unintentionally (nested scenario).
 *******************************************************************************************************/
static void *CPU_TmrInterruptTask(void *p_arg)
{
  struct timespec tspec, tspec_rem;
  int res;
  CPU_TMR_INTERRUPT *p_tmr_int;
  CPU_BOOLEAN one_shot;

  CPU_INT_DIS();

  p_tmr_int = (CPU_TMR_INTERRUPT *)p_arg;

  tspec.tv_nsec = p_tmr_int->PeriodMuSec * 1000u;
  tspec.tv_sec = p_tmr_int->PeriodSec;

  one_shot = p_tmr_int->OneShot;

  do {
    tspec_rem = tspec;
    do {
      res = clock_nanosleep(CLOCK_MONOTONIC, 0u, &tspec_rem, &tspec_rem);
    } while (res == EINTR);
    RTOS_ASSERT_CRITICAL(res == 0u, RTOS_ERR_FAIL,; );

    CPU_InterruptTriggerInternal(&p_tmr_int->Interrupt);        // See Note #2.
  } while (one_shot != DEF_YES);

  pthread_exit(DEF_NULL);
}
