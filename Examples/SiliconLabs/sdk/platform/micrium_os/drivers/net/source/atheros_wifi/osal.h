//                                                                 Copyright (c) Qualcomm Atheros, Inc.
//                                                                 All rights reserved.
//                                                                 Redistribution and use in source and binary forms, with or without modification, are permitted (subject to
//                                                                 the limitations in the disclaimer below) provided that the following conditions are met:
//
//                                                                 � Redistributions of source code must retain the above copyright notice, this list of conditions and the
//                                                                   following disclaimer.
//                                                                 � Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
//                                                                   following disclaimer in the documentation and/or other materials provided with the distribution.
//                                                                 � Neither the name of nor the names of its contributors may be used to endorse or promote products derived
//                                                                   from this software without specific prior written permission.
//
//                                                                 NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS
//                                                                 PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
//                                                                 BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//                                                                 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//                                                                 INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//                                                                 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
//                                                                 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//                                                                 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#ifndef _OSAL_H_
#define _OSAL_H_

#include <string.h>

/*
 * Global macros
 */

#include <a_types.h>
#include <athdefs.h>
#include <a_osapi.h>
#include  <common/source/kal/kal_priv.h>

/* QOSAL Error codes */
typedef enum {
  QOSAL_ERROR = -1,
  QOSAL_OK = 0,
  QOSAL_SUCCESS = 1,
  QOSAL_INVALID_TASK_ID = 2,
  QOSAL_INVALID_PARAMETER = 3,
  QOSAL_INVALID_POINTER = 4,
  QOSAL_ALREADY_EXISTS = 5,
  QOSAL_INVALID_EVENT = 6,
  QOSAL_EVENT_TIMEOUT = 7,
  QOSAL_INVALID_MUTEX = 8,
  QOSAL_TASK_ALREADY_LOCKED = 9,
  QOSAL_MUTEX_ALREADY_LOCKED = 10,
  QOSAL_OUT_OF_MEMORY = 11,
} QOSAL_STATUS;

/* Priority setting */

#define QOSAL_DRIVER_TASK_PRIORITY     QOSAL_TASK_PRIORITY_HIGHEST

#define QOSAL_TASK_PRIORITY_LOWEST              15
#define QOSAL_TASK_PRIORITY_ABOVE_LOWEST        14
#define QOSAL_TASK_PRIORITY_BELOW_LOWER         13
#define QOSAL_TASK_PRIORITY_LOWER               12
#define QOSAL_TASK_PRIORITY_ABOVE_LOWER         11
#define QOSAL_TASK_PRIORITY_MEDIUM              10
#define QOSAL_TASK_PRIORITY_ABOVE_MEDIUM        9
#define QOSAL_TASK_PRIORITY_BELOW_HIGH          8
#define QOSAL_TASK_PRIORITY_HIGH                7
#define QOSAL_TASK_PRIORITY_ABOVE_HIGH          5
#define QOSAL_TASK_PRIORITY_HIGHER              4
#define QOSAL_TASK_PRIORITY_HIGHEST             3

#define QOSAL_NULL_TASK_ID             &KAL_TaskHandleNull

#define QOSAL_EVENT_STRUCT              KAL_SEM_HANDLE
#define QOSAL_TICK_STRUCT               NULL
#define QOSAL_TICK_STRUCT_PTR           void*
#define QOSAL_TASK_ID                   KAL_TASK_HANDLE *
#define UNUSED_ARGUMENT(arg)            ((void)arg)
#define QOSAL_EVENT_AUTO_CLEAR          0x01

typedef KAL_SEM_HANDLE*              qosal_sem_handle;
typedef KAL_Q_HANDLE*              qosal_queue_handle;
typedef KAL_TASK_HANDLE*              qosal_task_handle;

typedef KAL_SEM_HANDLE*              qosal_event_handle;
typedef KAL_LOCK_HANDLE*             qosal_mutex_handle;//TBD
typedef KAL_SEM_HANDLE*              qosal_sema_handle; //TBD

typedef QOSAL_STATUS                 QOSAL_EVT_RET;

typedef void (*qosal_task_fct) (void * p_arg);
/*Global Structure*/
typedef struct time_struct{
  /* The number of seconds in the time.  */
  QOSAL_UINT32     SECONDS;
  /* The number of milliseconds in the time. */

  QOSAL_UINT32     MILLISECONDS;
} TIME_STRUCT;

/*
 * This universal OS Layer function is used to start the OS
 */
/* Description:
 * This function is used to start the RTOS
 *
 * Params: None
 *
 * Returns: None
 */
QOSAL_VOID qosal_start();

/*
 * Description: This function is used for creating an RTOS task
 *
 * Params: Task entry function
 *         Function arguments
 *         Task name
 *         Stack size
 *         Task priority
 *         Task handle
 *         Blocking Flag
 * Returns:  QOSAL_OK if Success, QOSAL_ERROR if Fail
 */
QOSAL_STATUS qosal_task_create
(
  QOSAL_VOID  Task(QOSAL_UINT32),
  char *task_name,
  int stack_size, QOSAL_VOID *param,
  unsigned long task_priority,
  qosal_task_handle *task_handle,
  QOSAL_BOOL blocked
);

/*
 * Description: This function is used to get the current priority of the task
 *
 * Params: Task handle
 *         Task priority
 *
 * Returns: QOSAL_OK if Success, QOSAL_ERROR if Fail
 */
QOSAL_STATUS qosal_task_get_priority(qosal_task_handle task_handle,
                                     QOSAL_UINT32 *priority_ptr);

/*
 * Description: This function is used to set the current priority of the task
 *
 * Params: Task handle
 *         Old priority
 *         New priority
 * Returns: QOSAL_OK if Success, QOSAL_ERROR if Fail
 */
QOSAL_STATUS qosal_task_set_priority(qosal_task_handle task_handle,
                                     QOSAL_UINT32 new_priority,
                                     QOSAL_VOID *priority_ptr);

/*
 * Description: This function is used to get the current task ID
 *
 * Params: None
 *
 * Returns: Task handle
 */
qosal_task_handle qosal_task_get_handle(QOSAL_VOID);

/*
 * Description: This function is used to destroy the task
 *
 * Params: task_hanlde
 *
 * Returns: QOSAL_OK on Success, QOSAL_ERROR on fail
 */
QOSAL_STATUS qosal_task_destroy(qosal_task_handle task_handle);

/*
 * Description: This function is used to suspend the active task
 *
 * Params: Task handle
 *
 * Returns: None
 */
QOSAL_VOID qosal_task_suspend(qosal_task_handle *task_handle);

/*
 * Description: This function is used to unblock the task
 *
 * Params: Task handle
 *
 * Returns: None
 */
QOSAL_VOID qosal_task_resume(qosal_task_handle *task_handle);

/******************************************************************************
 *
 * Memory Management APIs
 *
 *****************************************************************************/
/*
 * Description: This function is used for initializing the memory blocks
 *
 * Params: None
 *
 * Returns: None
 */
QOSAL_VOID qosal_malloc_init(QOSAL_VOID);

/*
 * Description: This function is used to get the memory block size
 *
 * Params: Address, Size
 *
 * Returns: Size of memory block
 */
QOSAL_UINT32 qosal_get_size(QOSAL_VOID* addr);

/*
 * Description: This function is used for allocating the memory block of
 * requested size
 *
 * Params: Size
 *
 * Returns: Address of allocatated memory block
 */
QOSAL_VOID* qosal_malloc(QOSAL_UINT32 size);

/*
 * Description: This function is used for freeing of the memory block
 *
 * Params: Address
 *
 * Returns: None
 */
/*Clear a memory pool*/
QOSAL_STATUS  qosal_free(QOSAL_VOID* addr);

#define QOSAL_MALLOC(size)           qosal_malloc(size)
#define QOSAL_FREE(addr)             qosal_free(addr)

/******************************************************************************
 * Timer Ticks APIs
 *****************************************************************************/
/*
 * Description: This function is used for delay the OS task for specific time
 * in milli secs
 *
 * Params: msec
 *
 * Returns: None
 */
QOSAL_VOID qosal_msec_delay(QOSAL_ULONG mSec);

/*
 * Description: This function is used for delay the OS task for specific time
 * in micro secs
 *
 * Params: uSec
 *
 * Returns: None
 */
QOSAL_VOID qosal_usec_delay(QOSAL_UINT32 uSec);

/*
 * Description: This function is used to count timer ticks
 *
 * Params: count
 *
 * Returns: QOSAL_OK if Success, QOSAL_ERROR if Fail
 */

QOSAL_STATUS qosal_time_get_ticks(QOSAL_UINT32 *count);

/*
 * Description: This function is used to get time per sec
 *
 * Params: None
 *
 * Returns: Time ticks per sec
 */
QOSAL_ULONG qosal_time_get_ticks_per_sec(QOSAL_VOID);

/*
 * Description: This function is used nvalidating all the data cache entries.
 *
 * Params: None
 *
 * Returns: None
 */
QOSAL_VOID qosal_dcache_invalidate();

/*
 * Description: This function is used flushing the data cache.
 *
 * Params: None
 *
 * Returns: None
 */
QOSAL_VOID qosal_dcache_flush();

/*
 * Description: This function is used for time delay
 *
 * Params: Delay value
 *
 * Returns: None
 */
QOSAL_VOID qosal_time_delay_ticks(QOSAL_ULONG val);

/******************************************************************************
*
* Interrupt Control APIs
******************************************************************************/
/*
 * Description: This function is used for disabling the external MCU interrupts
 *
 * Params: None
 *
 * Returns: None
 */

#define  qosal_intr_disable()  do { CPU_SR_ALLOC(); CPU_CRITICAL_ENTER(); } while (0)

/*
 * Description: This function is used for enabling the external MCU interrupts
 *
 * Params: None
 *
 * Returns: None
 */

#define qosal_intr_enable() CPU_CRITICAL_EXIT()

//                                                                QOSAL_VOID qosal_intr_enable (QOSAL_VOID);

/*****************************************************************************
 *
 * Event Handling APIs
 ******************************************************************************/
/*
 * Description: This function is used for waiting for an event
 *
 * Params: Event pointer, Bits to Wait for, Ticks to Wait for
 *
 * Returns: QOSAL_OK if Success, QOSAL_ERROR if Fail
 */
QOSAL_UINT32  qosal_wait_for_event(qosal_event_handle event_ptr,
                                   QOSAL_UINT32 bitsToWaitFor,
                                   QOSAL_UINT32 all,
                                   QOSAL_UINT32 var1,
                                   QOSAL_UINT32 ticksToWait);
/*
 * Description: This function is used for set an event
 *
 * Params: Event pointer, Bits to Set
 *
 * Returns: QOSAL_OK if Success, QOSAL_ERROR if Fail
 */
QOSAL_STATUS qosal_set_event(qosal_event_handle event_ptr, QOSAL_UINT32 bitsToSet);

/*
 * Description: This function is used for creating an event
 *
 * Params: Event pointer
 *
 * Returns: QOSAL_OK if Success, QOSAL_ERROR if Fail
 */
QOSAL_EVT_RET qosal_create_event(qosal_event_handle event_ptr);

/*
 * Description: This function is used for setting auto clearing of event bits in event group
 *
 * Params: Event pointer, flags
 *
 * Returns: QOSAL_OK if Success, QOSAL_ERROR if Fail
 */
QOSAL_STATUS qosal_set_event_auto_clear(qosal_event_handle event_ptr, QOSAL_UINT32 flags);

/*
 * Description: This function is used for clearing the event
 *
 * Params: Event pointer, BitsToClear
 *
 * Returns: QOSAL_OK if Success, QOSAL_ERROR if Fail
 */
QOSAL_STATUS qosal_clear_event(qosal_event_handle event_ptr, QOSAL_UINT32 bitsToClear);

/*
 * Description:  This function is used for deleting an event
 *
 * Params: Event pointer
 *
 * Returns: QOSAL_OK if Success, QOSAL_ERROR if Fail
 */
QOSAL_STATUS qosal_delete_event(qosal_event_handle event_ptr);

/*****************************************************************************
*
* Task Synchronization APIs (Mutex)
*
*****************************************************************************/
/*
 * Description: This function is used for initialization of the mutex
 *
 * Params: Mutex pointer, Attr_ptr
 *
 * Returns: QOSAL_OK if Success, QOSAL_ERROR if Fail
 */

QOSAL_STATUS qosal_mutex_init(qosal_mutex_handle mutex_ptr);

/*
 * Description: This function is used for aquiring the mutex lock
 *
 * Params: Mutex pointer, tick_count
 *
 * Returns: QOSAL_OK if Success, QOSAL_ERROR if Fail
 */
QOSAL_STATUS qosal_mutex_acquire(qosal_mutex_handle mutex_lock, QOSAL_ULONG tick_count);

/*
 * Description: This function is used for releasing the mutex lock
 *
 * Params: Mutex pointer
 *
 * Returns: QOSAL_OK if Success, QOSAL_ERROR if Fail
 */
QOSAL_STATUS qosal_mutex_release(qosal_mutex_handle mutex_ptr);

/*
 * Description: This function is used for deleting the mutex
 *
 * Params: Mutex pointer
 *
 * Returns: QOSAL_OK if Success, QOSAL_ERROR if Fail
 */
QOSAL_STATUS qosal_mutex_destroy(qosal_mutex_handle mutex_ptr);

/*****************************************************************************
*
* Time delay APIs
*
*****************************************************************************/
/*
 * Description: This function is used to delay for given time ms
 *
 * Params: msec
 *
 * Returns: None
 */
QOSAL_VOID qosal_time_delay(QOSAL_UINT32 msec);

/*
 * Description: This function is used to get the elapsed time from tick time
 *
 * Params: ptr
 *
 * Returns: None
 */
//                                                                QOSAL_VOID qosal_time_get_elapsed(TIME_STRUCT* time);

/********************************************************************
*               Task Synchronization APIs (Semaphores)
********************************************************************/
//                                                                TBD

QOSAL_STATUS qosal_sema_init(qosal_sema_handle *sem_ptr,
                             QOSAL_UINT32 maxCount);
QOSAL_STATUS qosal_sema_get(qosal_sema_handle *sem_lock, QOSAL_ULONG tick_count);

QOSAL_STATUS qosal_sema_put(qosal_sema_handle *sem_lock);

QOSAL_STATUS qosal_sema_destroy(qosal_sema_handle *sem_lock);

#if 0 //TBD

/*********************************************************************
*                   Kernel Log APIs
*********************************************************************/

QOSAL_UINT32 qosal_klog_create(QOSAL_UINT32 size, QOSAL_UINT32 flags);
QOSAL_VOID qosal_klog_control(QOSAL_UINT32 size, QOSAL_BOOL flags);
QOSAL_BOOL qosal_klog_dispaly(QOSAL_VOID);
QOSAL_UINT32 qosal_klog_create_at(QOSAL_UINT32 max_size, QOSAL_UINT32 flags, QOSAL_VOID* ptr);
/*****************************************************************************/
#endif

#endif          //_OSAL_H_
