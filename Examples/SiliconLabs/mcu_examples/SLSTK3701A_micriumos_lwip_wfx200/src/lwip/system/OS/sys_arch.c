/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#include <stdlib.h>
/* lwIP includes. */
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/stats.h"
#include "arch/sys_arch.h"
#include  <kernel/include/os.h>
#if !NO_SYS

CPU_STK sys_stack[LWIP_MAX_TASKS][LWIP_STACK_SIZE];
OS_TCB  sys_TCB[LWIP_MAX_TASKS];
static uint8_t sys_thread_no;


#if defined(LWIP_SOCKET_SET_ERRNO) && defined(LWIP_PROVIDE_ERRNO)
int errno;
#endif

/*-----------------------------------------------------------------------------------*/
//  Creates an empty mailbox.
err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
	RTOS_ERR err;
	mbox->is_valid = 0;
	OSQCreate(&(mbox->Q),"lwipQ",size,&err);
	LWIP_ASSERT("OSQCreate", (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) );
	OSSemCreate(&(mbox->Q_full),"lwipQsem",size,&err);
	LWIP_ASSERT("OSSemCreate", (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) );
	mbox->is_valid = 1;
#if SYS_STATS
     ++lwip_stats.sys.mbox.used;
#endif /* SYS_STATS */
	return ERR_OK;
}

/*-----------------------------------------------------------------------------------*/
/*
  Deallocates a mailbox. If there are messages still present in the
  mailbox when the mailbox is deallocated, it is an indication of a
  programming error in lwIP and the developer should be notified.
*/
void sys_mbox_free(sys_mbox_t *mbox)
{

	RTOS_ERR err;
	OSSemDel(&(mbox->Q_full),OS_OPT_DEL_ALWAYS,&err);
	LWIP_ASSERT("OSSemDel", (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) );
    OSQDel(&(mbox->Q),OS_OPT_DEL_ALWAYS,&err);
    LWIP_ASSERT("OSQDel", (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) );
    mbox->is_valid = 0;
#if SYS_STATS
     --lwip_stats.sys.mbox.used;
#endif /* SYS_STATS */
}

/*-----------------------------------------------------------------------------------*/
//   Posts the "msg" to the mailbox.
void sys_mbox_post(sys_mbox_t *mbox, void *data)
{
  RTOS_ERR err;
	/* Wait for an available slot in the queue. */
  OSSemPend(&(mbox->Q_full), 0, OS_OPT_PEND_BLOCKING, 0, &err);
  LWIP_ASSERT("OSSemPend", (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) );
  /* Posts the message to the queue. */
  OSQPost(&(mbox->Q), data, 0, OS_OPT_POST_FIFO, &err);
  LWIP_ASSERT("OSQPost", (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) );

}


/*-----------------------------------------------------------------------------------*/
//   Try to post the "msg" to the mailbox. Non-blocking (actually blocks for 1 tick)
err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
    err_t result=ERR_OK;
    RTOS_ERR err;

    OSSemPend(&(mbox->Q_full),0,OS_OPT_PEND_NON_BLOCKING,0,&err);
    if (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE)
    {
    	/* Posts the message to the queue. */
    	  OSQPost(&(mbox->Q), msg, 0, OS_OPT_POST_FIFO, &err);
    	  LWIP_ASSERT("OSQPost", (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) );
    } else {
#if SYS_STATS
      lwip_stats.sys.mbox.err++;
#endif /* SYS_STATS */
        return ERR_MEM;
    }

			

   return result;
}

/*-----------------------------------------------------------------------------------*/
/*
  Blocks the thread until a message arrives in the mailbox, but does
  not block the thread longer than "timeout" milliseconds (similar to
  the sys_arch_sem_wait() function). The "msg" argument is a result
  parameter that is set by the function (i.e., by doing "*msg =
  ptr"). The "msg" parameter maybe NULL to indicate that the message
  should be dropped.

  The return values are the same as for the sys_arch_sem_wait() function:
  Number of milliseconds spent waiting or SYS_ARCH_TIMEOUT if there was a
  timeout.

  Note that a function with a similar name, sys_mbox_fetch(), is
  implemented by lwIP.
*/
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
	int32_t os_timeout;
#if 0
		OS_RATE_HZ tick_rate;
		OS_TICK ticks;
#endif
		u32_t starttime;
	    RTOS_ERR err;
	    OS_MSG_SIZE size;
	    void *temp;
	    starttime = sys_now();
		/* Convert lwIP timeout (in milliseconds) to uOS timeout (in OS_TICKS) */
#if 0
		if(timeout) {
		    	tick_rate = OSCfg_TickRate_Hz;
		    	ticks = (tick_rate * ((OS_TICK)timeout + (OS_TICK)500u / tick_rate)) / (OS_TICK)1000u;
		    	os_timeout = ticks;
		        if(os_timeout < 1)
		            os_timeout = 1;
		} else {
		        os_timeout = 0;
		}
#else
		os_timeout = timeout;
#endif
		temp = OSQPend(&(mbox->Q), os_timeout, OS_OPT_PEND_BLOCKING ,&size, 0, &err);

		if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE)
		{
		 	return SYS_ARCH_TIMEOUT;
		}
		/* Tells tasks waiting because of a full buffer that the buffer is not full
		 * anymore. */
		OSSemPost(&(mbox->Q_full),OS_OPT_POST_1,&err);

		if(msg) {
		    *msg = temp;
		}


		return (sys_now() - starttime);
  
}



/*-----------------------------------------------------------------------------------*/
/*
  Similar to sys_arch_mbox_fetch, but if message is not ready immediately, we'll
  return with SYS_MBOX_EMPTY.  On success, 0 is returned.
*/
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
	RTOS_ERR err;
OS_MSG_SIZE size;
void *temp;
	temp = OSQPend(&(mbox->Q), 0, OS_OPT_PEND_NON_BLOCKING ,&size, 0, &err);



			    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE)
			    {
			    	return SYS_MBOX_EMPTY;
			    }
			        /* Tells tasks waiting because of a full buffer that the buffer is not full
			         * anymore. */
			        OSSemPost(&(mbox->Q_full),OS_OPT_POST_1,&err);

			        if(msg) {
			            *msg = temp;
			        }


			        return 0;
}
/*----------------------------------------------------------------------------------*/
int sys_mbox_valid(sys_mbox_t *mbox)          
{      

    return mbox->is_valid;
}                                             
/*-----------------------------------------------------------------------------------*/                                              
void sys_mbox_set_invalid(sys_mbox_t *mbox)   
{                                             
  mbox->is_valid = 0;
}                                             

/*-----------------------------------------------------------------------------------*/
//  Creates a new semaphore. The "count" argument specifies
//  the initial state of the semaphore.
err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
  RTOS_ERR err;
  OSSemCreate (&(sem->sem), "lwip sem", count, &err);

  if(RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE)
  {
	  sem->is_valid = 0;
#if SYS_STATS
      ++lwip_stats.sys.sem.err;
#endif /* SYS_STATS */	
		return ERR_MEM;
  }
  sem->is_valid = 1;
#if SYS_STATS
	++lwip_stats.sys.sem.used;
 	if (lwip_stats.sys.sem.max < lwip_stats.sys.sem.used) {
		lwip_stats.sys.sem.max = lwip_stats.sys.sem.used;
	}
#endif /* SYS_STATS */
		
	return ERR_OK;
}

/*-----------------------------------------------------------------------------------*/
/*
  Blocks the thread while waiting for the semaphore to be
  signaled. If the "timeout" argument is non-zero, the thread should
  only be blocked for the specified time (measured in
  milliseconds).

  If the timeout argument is non-zero, the return value is the number of
  milliseconds spent waiting for the semaphore to be signaled. If the
  semaphore wasn't signaled within the specified time, the return value is
  SYS_ARCH_TIMEOUT. If the thread didn't have to wait for the semaphore
  (i.e., it was already signaled), the function may return zero.

  Notice that lwIP implements a function with a similar name,
  sys_sem_wait(), that uses the sys_arch_sem_wait() function.
*/

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
	int32_t os_timeout;
#if 0
	OS_RATE_HZ tick_rate;
	OS_TICK ticks;
#endif
	u32_t starttime;
    RTOS_ERR err;
    starttime = sys_now();
#if 0
	    /* Convert lwIP timeout (in milliseconds) to uOS timeout (in OS_TICKS) */
	    if(timeout) {
	    	tick_rate = OSCfg_TickRate_Hz;
	    	ticks = (tick_rate * ((OS_TICK)timeout + (OS_TICK)500u / tick_rate)) / (OS_TICK)1000u;
	    	os_timeout = ticks;
	        if(os_timeout < 1)
	            os_timeout = 1;
	    } else {
	        os_timeout = 0;
	    }
#else
	    os_timeout = timeout;
#endif
	OSSemPend (&(sem->sem), os_timeout,OS_OPT_PEND_BLOCKING, 0,&err);
	if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE)
	{
		return SYS_ARCH_TIMEOUT;
	}

    return (sys_now() - starttime);

}

/*-----------------------------------------------------------------------------------*/
// Signals a semaphore
void sys_sem_signal(sys_sem_t *sem)
{
	RTOS_ERR err;
	OSSemPost (&(sem->sem),OS_OPT_POST_1,&err);
}

/*-----------------------------------------------------------------------------------*/
// Deallocates a semaphore
void sys_sem_free(sys_sem_t *sem)
{
 RTOS_ERR err;
#if SYS_STATS
  --lwip_stats.sys.sem.used;
#endif /* SYS_STATS */
  
  OSSemDel (&(sem->sem),OS_OPT_DEL_ALWAYS, &err);
}
/*-----------------------------------------------------------------------------------*/
int sys_sem_valid(sys_sem_t *sem)                                               
{
  return sem->is_valid;
}

/*-----------------------------------------------------------------------------------*/                                                                                                                                                                
void sys_sem_set_invalid(sys_sem_t *sem)                                        
{                                                                               
  sem->is_valid = 0;
} 

/*-----------------------------------------------------------------------------------*/ 
OS_MUTEX   lwip_sys_mutex;

// Initialize sys arch
void sys_init(void)
{
  RTOS_ERR err;
  sys_thread_no = 0;
  OSMutexCreate (&lwip_sys_mutex,"lwip_sys_mutex",&err);
  LWIP_ASSERT("OSMutexCreate", (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) );
}
/*-----------------------------------------------------------------------------------*/
                                      /* Mutexes*/
/*-----------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------*/
#if LWIP_COMPAT_MUTEX == 0
/* Create a new mutex*/
err_t sys_mutex_new(sys_mutex_t *mutex) {
  RTOS_ERR err;
  
  OSMutexCreate (&(mutex->mutex), "lwip mutex",&err);

  if(RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE)
  {
	mutex->is_valid = 0;
#if SYS_STATS
    ++lwip_stats.sys.mutex.err;
#endif /* SYS_STATS */	
    return ERR_MEM;
  }
  mutex->is_valid = 1;
#if SYS_STATS
  ++lwip_stats.sys.mutex.used;
  if (lwip_stats.sys.mutex.max < lwip_stats.sys.mutex.used) {
    lwip_stats.sys.mutex.max = lwip_stats.sys.mutex.used;
  }
#endif /* SYS_STATS */
  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
/* Deallocate a mutex*/
void sys_mutex_free(sys_mutex_t *mutex)
{
	RTOS_ERR err;
#if SYS_STATS
      --lwip_stats.sys.mutex.used;
#endif /* SYS_STATS */

      OSMutexDel(&(mutex->mutex),OS_OPT_DEL_ALWAYS,&err);
      mutex->is_valid = 0;
}

int sys_mutex_valid (sys_mutex_t *mutex)
{
	return mutex->is_valid;
}

void sys_mutex_set_invalid	(sys_mutex_t *mutex)
{
	mutex->is_valid = 0;
}


/*-----------------------------------------------------------------------------------*/
/* Lock a mutex*/
void sys_mutex_lock(sys_mutex_t *mutex)
{
	RTOS_ERR err;
	OSMutexPend (&(mutex->mutex), 0,OS_OPT_PEND_BLOCKING,NULL,&err);
}

/*-----------------------------------------------------------------------------------*/
/* Unlock a mutex*/
void sys_mutex_unlock(sys_mutex_t *mutex)
{
	RTOS_ERR err;
	OSMutexPost (&(mutex->mutex),
			       OS_OPT_POST_NONE,
	               &err);
}
#endif /*LWIP_COMPAT_MUTEX*/
/*-----------------------------------------------------------------------------------*/
// TODO
/*-----------------------------------------------------------------------------------*/
/*
  Starts a new thread with priority "prio" that will begin its execution in the
  function "thread()". The "arg" argument will be passed as an argument to the
  thread() function. The id of the new thread is returned. Both the id and
  the priority are system dependent.
*/
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread , void *arg, int stacksize, int prio)
{
  RTOS_ERR err;
  LWIP_ASSERT("sys_thread_new: Max Sys. Tasks reached.", sys_thread_no < LWIP_MAX_TASKS);
  ++sys_thread_no; /* next task created will be one lower to this one */

  OSTaskCreate (&(sys_TCB[sys_thread_no-1]),
                      (CPU_CHAR*)name,
                      thread,
                      arg,
                      prio,
					  &sys_stack[sys_thread_no - 1][0],
					  (LWIP_STACK_SIZE / 10u),
					  LWIP_STACK_SIZE,
					  LWIP_Q_SIZE,
                      0,
					  DEF_NULL,
					  (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_NO_TLS),
                      &err);

  LWIP_ASSERT("Failed to create task.", RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE );

  return sys_thread_no;
}

/*
  This optional function does a "fast" critical region protection and returns
  the previous protection level. This function is only called during very short
  critical regions. An embedded system which supports ISR-based drivers might
  want to implement this function by disabling interrupts. Task-based systems
  might want to implement this by using a mutex or disabling tasking. This
  function should support recursive calls from the same task or interrupt. In
  other words, sys_arch_protect() could be called while already protected. In
  that case the return value indicates that it is already protected.

  sys_arch_protect() is only required if your port is supporting an operating
  system.
*/
sys_prot_t sys_arch_protect(void)
{
  RTOS_ERR err;
  OSMutexPend (&lwip_sys_mutex,
	                   0,
					   OS_OPT_PEND_BLOCKING,
	                   NULL,
	                   &err);
  //LWIP_ASSERT("OSMutexPend", (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) );
  return (sys_prot_t)1;
}


/*
  This optional function does a "fast" set of critical region protection to the
  value specified by pval. See the documentation for sys_arch_protect() for
  more information. This function is only required if your port is supporting
  an operating system.
*/
void sys_arch_unprotect(sys_prot_t pval)
{
  ( void ) pval;
  RTOS_ERR err;
  OSMutexPost (&lwip_sys_mutex,
		       OS_OPT_POST_NONE,
               &err);
  //LWIP_ASSERT("OSMutexPost", (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) );
}

/* returns system time in ms*/
u32_t sys_now(void)
{
	RTOS_ERR err;
	return OSTimeGet(&err);
}

#endif /* !NO_SYS */
