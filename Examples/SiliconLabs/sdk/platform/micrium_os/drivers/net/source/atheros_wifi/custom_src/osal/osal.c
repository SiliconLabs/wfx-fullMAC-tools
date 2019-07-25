//                                                                 Copyright (c) Qualcomm Atheros, Inc.
//                                                                 All rights reserved.
//                                                                 Redistribution and use in source and binary forms, with or without modification, are permitted (subject to
//                                                                 the limitations in the disclaimer below) provided that the following conditions are met:
//
//                                                                 · Redistributions of source code must retain the above copyright notice, this list of conditions and the
//                                                                   following disclaimer.
//                                                                 · Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
//                                                                   following disclaimer in the documentation and/or other materials provided with the distribution.
//                                                                 · Neither the name of nor the names of its contributors may be used to endorse or promote products derived
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

#include <osal.h>
#include <common_api.h>

/******************************************************************************/
/* OS Layer Wrapper function implementation */
/******************************************************************************/
/****************
* OS Task API's
****************/

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
QOSAL_VOID qosal_start()
{
}

/* Description:
 * This function is used to get the OS return/error code
 *
 * Params: OS error code
 *
 * Returns: QOSAL_OK if Success, QOSAL_ERROR if Fail
 */
QOSAL_STATUS qosal_get_error_code(QOSAL_UINT32 os_err)
{
  QOSAL_STATUS osal_ret;

  switch (os_err) {
    case RTOS_ERR_NONE:
      osal_ret = QOSAL_OK;
      break;

    default:
      osal_ret = QOSAL_ERROR;
      break;
  }
  return osal_ret;
}

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
  QOSAL_BOOL auto_start
)
{
  QOSAL_STATUS osal_ret = QOSAL_OK;

  /* not implemented for uCOS, See qca.c */
#if 0
  RTOS_ERR err_rtos;

  QCA_CtxPtr->DriverTaskHandle = KAL_TaskAlloc(task_name,
                                               DEF_NULL,
                                               stack_size,
                                               DEF_NULL,
                                               &err_rtos);

  if (err_rtos == RTOS_ERR_NONE) {
    KAL_TaskCreate(QCA_CtxPtr->DriverTaskHandle,
                   (qosal_task_fct) Task,
                   param,
                   task_priority,
                   DEF_NULL,
                   &err_rtos);
  }
  task_handle = &QCA_CtxPtr->DriverTaskHandle;
  osal_ret = qosal_get_error_code(RTOS_ERR_CODE_GET(err_rtos));
#endif

  return osal_ret;
}

/*
 * Description: This function is used to get the current priority of the task
 *
 * Params: Task handle
 *         Task priority
 *
 * Returns: QOSAL_OK if Success, QOSAL_ERROR if Fail
 */
QOSAL_STATUS qosal_task_get_priority(qosal_task_handle task_handle,
                                     QOSAL_UINT32 *priority_ptr)
{
  QOSAL_STATUS osal_ret = QOSAL_OK;

#if 0
  RTOS_ERR err_rtos;
  osal_ret = qosal_get_error_code(RTOS_ERR_CODE_GET(err_rtos));
#endif

  return osal_ret;
}

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
                                     QOSAL_VOID *priority_ptr)
{
  QOSAL_STATUS osal_ret = QOSAL_OK;

#if 0
  RTOS_ERR err_rtos;
  osal_ret = qosal_get_error_code(RTOS_ERR_CODE_GET(err_rtos));
#endif

  return osal_ret;
}

/*
 * Description: This function is used to get the current task ID
 *
 * Params: None
 *
 * Returns: Task handle
 */
qosal_task_handle qosal_task_get_handle(QOSAL_VOID)
{
  qosal_task_handle handle = DEF_NULL;

#if 0
  RTOS_ERR err_rtos;
  osal_ret = qosal_get_error_code(RTOS_ERR_CODE_GET(err_rtos));
#endif

  return handle;
}

/*
 * Description: This function is used to destroy the task
 *
 * Params: task_handle
 *
 * Returns: QOSAL_OK if Success, QOSAL_ERROR if Fail
 */
QOSAL_STATUS qosal_task_destroy(qosal_task_handle task_handle)
{
  QOSAL_STATUS osal_ret = QOSAL_OK;

#if 0
  RTOS_ERR err_rtos;
  osal_ret = qosal_get_error_code(RTOS_ERR_CODE_GET(err_rtos));
#endif

  return osal_ret;
}

/*
 * Description: This function is used to suspend the active task
 *
 * Params: Task handle
 *
 * Returns: None
 */
QOSAL_VOID qosal_task_suspend(qosal_task_handle *task_handle)
{
#if 0
  RTOS_ERR err_rtos;
  osal_ret = qosal_get_error_code(RTOS_ERR_CODE_GET(err_rtos));
#endif
  return;
}

/*
 * Description: This function is used to unblock the task
 *
 * Params: Task handle
 *
 * Returns: None
 */
QOSAL_VOID qosal_task_resume(qosal_task_handle *task_handle)
{
#if 0
  RTOS_ERR err_rtos;
  osal_ret = qosal_get_error_code(RTOS_ERR_CODE_GET(err_rtos));
#endif
  return;
}

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
QOSAL_VOID qosal_malloc_init(QOSAL_VOID)
{
}

/*
 * Description: This function is used to get the memory block size
 *
 * Params: Address, Size
 *
 * Returns: Size of memory block
 */
QOSAL_UINT32 qosal_get_size(QOSAL_VOID* addr)
{
  QOSAL_UINT32 size = 0;

  return size;
}

/*
 * Description: This function is used for allocating the memory block of
 * requested size
 *
 * Params: Size
 *
 * Returns: Address of allocatated memory block
 */

A_NETBUF_QUEUE_T custom_alloc_queue;

QOSAL_VOID* qosal_malloc(QOSAL_UINT32 size)
{
  QOSAL_VOID  *addr = NULL;
  QOSAL_UINT8 *p_buf;
  A_NETBUF    *p_a_netbuf;

  p_a_netbuf = (A_NETBUF *)A_NETBUF_ALLOC(size);
  if (p_a_netbuf == NULL) {
    return DEF_NULL;
  }

  p_buf = A_NETBUF_DATA(p_a_netbuf);

  A_NETBUF_ENQUEUE(&custom_alloc_queue, p_a_netbuf);
  /* --------------- FIND SOCKET CONTEXT ---------------- */
  p_a_netbuf->pool_id = A_TX_NET_POOL;
  addr = (QOSAL_VOID*)p_buf;

  return addr;
}

/*
 * Description: This function is used for freeing of the memory block
 *
 * Params: Address
 *
 * Returns: QOSAL_OK if Success, QOSAL_ERROR if Fail
 */
/*Clear a memory pool*/

QOSAL_STATUS  qosal_free(QOSAL_VOID* addr)
{
  QOSAL_STATUS osal_ret = QOSAL_OK;
  A_NETBUF* a_netbuf_ptr = NULL;

  /*find buffer in zero copy queue*/
  //                                                              a_netbuf_ptr = A_NETBUF_DEQUEUE_ADV(&custom_alloc_queue, addr);

  if (a_netbuf_ptr != NULL) {
    //                                                             QCA_BufBlkFree(a_netbuf_ptr->buf_pool, a_netbuf_ptr);
    //                                                              Driver_ReportRxBuffStatus(QCA_CtxPtr, TRUE);
  }

  return osal_ret;
}

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
QOSAL_VOID qosal_msec_delay(QOSAL_ULONG mSec)
{
  KAL_Dly(mSec);
}

/*
 * Description: This function is used for delay the OS task for specific time
 * in micro secs
 *
 * Params: uSec
 *
 * Returns: None
 */
QOSAL_VOID qosal_usec_delay(QOSAL_UINT32 uSec)
{
  if (uSec < 1000) {
    KAL_Dly(1);
  } else {
    KAL_Dly(uSec / 1000);
  }
  return;
}

/*
 * Description: This function is used to get absolute time in ticks
 *
 * Params: count
 *
 * Returns: QOSAL_OK if Success, QOSAL_ERROR if Fail
 */
QOSAL_STATUS qosal_time_get_ticks(QOSAL_UINT32 *count)
{
  QOSAL_STATUS osal_ret = QOSAL_OK;
  RTOS_ERR     err_rtos;

  *count = (QOSAL_UINT32)KAL_TickGet(&err_rtos);

  osal_ret = qosal_get_error_code(RTOS_ERR_CODE_GET(err_rtos));

  return osal_ret;
}

/*
 * Description: This function is used to get time per sec
 *
 * Params: None
 *
 * Returns: Time ticks per sec
 */
QOSAL_ULONG qosal_time_get_ticks_per_sec(QOSAL_VOID)
{
  return (QOSAL_ULONG)KAL_TickRateGet();
}

/*
 * Description: This function is used flushing the data cache.
 *
 * Params: None
 *
 * Returns: None
 */
QOSAL_VOID qosal_dcache_flush(QOSAL_VOID)
{
}

/*
 * Description: This function is used nvalidating all the data cache entries.
 *
 * Params: None
 *
 * Returns: None
 */
QOSAL_VOID qosal_dcache_invalidate(QOSAL_VOID)
{
}

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

/*
 * Description: This function is used for enabling the external MCU interrupts
 *
 * Params: None
 *
 * Returns: None
 */

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
QOSAL_UINT32 qosal_wait_for_event(qosal_event_handle event_ptr,
                                  QOSAL_UINT32 bitsToWaitFor,
                                  QOSAL_UINT32 all,
                                  QOSAL_UINT32 var1,
                                  QOSAL_UINT32 ticksToWait)
{
  QOSAL_UINT32 os_ret;
  RTOS_ERR     err_rtos;

  KAL_SemPend(*event_ptr,
              KAL_OPT_PEND_NONE,
              ticksToWait,
              &err_rtos);

  os_ret = qosal_get_error_code(RTOS_ERR_CODE_GET(err_rtos));

  return os_ret;
}

/*
 * Description: This function is used for set an event
 *
 * Params: Event pointer, Bits to Set
 *
 * Returns: QOSAL_OK if Success, QOSAL_ERROR if Fail
 */
QOSAL_STATUS qosal_set_event(qosal_event_handle event_ptr, QOSAL_UINT32 bitsToSet)
{
  QOSAL_STATUS osal_ret = QOSAL_OK;
  RTOS_ERR err_rtos;

  KAL_SemPost(*event_ptr,
              KAL_OPT_PEND_NONE,
              &err_rtos);

  osal_ret = qosal_get_error_code(RTOS_ERR_CODE_GET(err_rtos));

  return osal_ret;
}

/*
 * Description: This function is used for creating an event
 *
 * Params: Event pointer
 *
 * Returns: QOSAL_OK if Success, QOSAL_ERROR if Fail
 */
QOSAL_EVT_RET
qosal_create_event(qosal_event_handle event_ptr)
{
  QOSAL_STATUS osal_ret = QOSAL_OK;
  RTOS_ERR     err_rtos;

  *event_ptr = KAL_SemCreate("qosal-event",
                             DEF_NULL,
                             &err_rtos);

  osal_ret = qosal_get_error_code(RTOS_ERR_CODE_GET(err_rtos));

  return osal_ret;
}

/*
 * Description: This function is used for setting auto clearing of event bits in event group
 *
 * Params: Event pointer, flags
 *
 * Returns: QOSAL_OK if Success, QOSAL_ERROR if Fail
 */
QOSAL_STATUS qosal_set_event_auto_clear(qosal_event_handle event_ptr, QOSAL_UINT32 flags)
{
  QOSAL_STATUS osal_ret = QOSAL_OK;

  return osal_ret;
}

/*
 * Description: This function is used for clearing the event
 *
 * Params: Event pointer, BitsToClear
 *
 * Returns: QOSAL_OK if Success, QOSAL_ERROR if Fail
 */
QOSAL_STATUS qosal_clear_event(qosal_event_handle event_ptr, QOSAL_UINT32 bitsToClear)
{
  QOSAL_STATUS osal_ret = QOSAL_OK;
  //                                                               RTOS_ERR     err_rtos;

  /*KAL_SemSet(*event_ptr,
              0,
             &err_rtos);
     osal_ret = qosal_get_error_code(RTOS_ERR_CODE_GET(err_rtos));
   */
  return osal_ret;
}

/*
 * Description:  This function is used for deleting an event
 *
 * Params: Event pointer
 *
 * Returns: QOSAL_OK if Success, QOSAL_ERROR if Fail
 */
QOSAL_STATUS qosal_delete_event(qosal_event_handle event_ptr)
{
  QOSAL_STATUS osal_ret = QOSAL_OK;

  KAL_SemDel(*event_ptr);

  return osal_ret;
}

/*****************************************************************************
*
* Task Synchronization APIs (Mutex)
*
*****************************************************************************/
/*
 * Description: This function is used for initialization of the mutex
 *
 * Params: Mutex pointer
 *
 * Returns: QOSAL_OK if Success, QOSAL_ERROR if Fail
 */
QOSAL_STATUS qosal_mutex_init(qosal_mutex_handle mutex_ptr)
{
  QOSAL_STATUS osal_ret = QOSAL_OK;
  RTOS_ERR     err_rtos;

  *mutex_ptr = KAL_LockCreate("QOSAL Lock",
                              DEF_NULL,
                              &err_rtos);

  osal_ret = qosal_get_error_code(RTOS_ERR_CODE_GET(err_rtos));

  return osal_ret;
}

/*
 * Description: This function is used for aquiring the mutex lock
 *
 * Params: Mutex pointer, tick_count
 *
 * Returns: QOSAL_OK if Success, QOSAL_ERROR if Fail
 */
QOSAL_STATUS qosal_mutex_acquire(qosal_mutex_handle mutex_lock, QOSAL_ULONG tick_count)
{
  QOSAL_STATUS osal_ret;
  RTOS_ERR     err_rtos;
  QOSAL_UINT32 timeout_ms;
  QOSAL_UINT32 tick_ms;

  tick_ms = (1000u / KAL_TickRateGet());
  if (tick_ms != 0) {
    timeout_ms = tick_ms * tick_count;
  } else {
    timeout_ms = 1;
  }
  KAL_LockAcquire(*mutex_lock,
                  KAL_OPT_PEND_NONE,
                  timeout_ms,
                  &err_rtos);

  osal_ret = qosal_get_error_code(RTOS_ERR_CODE_GET(err_rtos));

  return osal_ret;
}

/*
 * Description: This function is used for releasing the mutex lock
 *
 * Params: Mutex pointer
 *
 * Returns: QOSAL_OK if Success, QOSAL_ERROR if Fail
 */
QOSAL_STATUS qosal_mutex_release(qosal_mutex_handle mutex_ptr)
{
  QOSAL_STATUS osal_ret;
  RTOS_ERR err_rtos;

  KAL_LockRelease(*mutex_ptr,
                  &err_rtos);

  osal_ret = qosal_get_error_code(RTOS_ERR_CODE_GET(err_rtos));

  return osal_ret;
}

/*
 * Description: This function is used for deleting the mutex
 *
 * Params: Mutex pointer
 *
 * Returns: QOSAL_OK if Success, QOSAL_ERROR if Fail
 */
QOSAL_STATUS qosal_mutex_destroy(qosal_mutex_handle mutex_ptr)
{
  QOSAL_STATUS osal_ret = QOSAL_OK;

  KAL_LockDel(*mutex_ptr);

  return osal_ret;
}

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
QOSAL_VOID qosal_time_delay(QOSAL_UINT32 msec)
{
  KAL_Dly(msec);
}

/*
 * Description: This function is used to get the elapsed time from tick time
 *
 * Params: Ptr to Time struct
 *
 * Returns: None
 */
QOSAL_VOID qosal_time_get_elapsed(TIME_STRUCT* time)
{
  return;
}

/*********************************************************************
*                 Kernel Log APIs
*********************************************************************/
/*
 * Description: This function is used to Creates the kernel logs
 *
 * Params: size    : size of the log,
           flags  :   1 (When the log is full, oldest entries are overwritten.)
   -                     0 (When the log is full, no more entries are written; default.)
 *
 * Returns: QOSAL_OK for success, QOSAL_ERROR for failure
 */
QOSAL_UINT32 qosal_klog_create(QOSAL_UINT32 size, QOSAL_UINT32 flags)
{
  QOSAL_UINT32 os_ret = QOSAL_OK;

  return os_ret;
}

/*
 * Description: Controls logging in kernel log
 *
 * Params:   bit_mask    :    Which bits of the kernel log control variable to modify.
           set_bits     :   TRUE ((Bits set in bit_mask are set in the control variable)
   -                           FALSE (Bits set in bit_mask are cleared in the control variable)
 *
 * Returns: None
 */
QOSAL_VOID qosal_klog_control(QOSAL_UINT32 bit_mask, QOSAL_BOOL set_bits)
{
}

/*
 * Description: Displays the oldest entry in kernel log and delete this entry
 *
 * Params: None
 *
 * Returns: QOSAL_OK for success, QOSAL_ERROR for failure
 */
QOSAL_BOOL qosal_klog_dispaly(QOSAL_VOID)
{
  QOSAL_BOOL  val = 0;

  return val;
}
/*
 * Description: This function is used to Creates the kernel logs
 *
 * Params: size    :   size of the log,
          flags   :   1 (When the log is full, oldest entries are overwritten.)
   -                     0 (When the log is full, no more entries are written; default.)
          ptr     :   Where in memory is the log to start
 *
 * Returns: QOSAL_OK for success, QOSAL_ERROR for failure
 */
QOSAL_UINT32 qosal_klog_create_at(QOSAL_UINT32 max_size, QOSAL_UINT32 flags, QOSAL_VOID* ptr)
{
  QOSAL_UINT32 os_ret = QOSAL_OK;

  return os_ret;
}

/*******************************************************************************/
