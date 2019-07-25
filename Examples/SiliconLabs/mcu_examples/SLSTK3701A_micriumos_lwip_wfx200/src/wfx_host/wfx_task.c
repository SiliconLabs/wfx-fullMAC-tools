/**************************************************************************//**
 * Copyright 2019, Silicon Laboratories Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/
#include "sl_wfx.h"
#include "sl_wfx_registers.h"
#include "lwip_micriumos.h"
#include "wfx_pin_config.h"

#include "em_gpio.h"
#include "em_usart.h"
#include "em_cmu.h"
#include "em_ldma.h"
#include "em_bus.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/include/os.h>
#include <common/include/rtos_utils.h>
#include <common/include/rtos_err.h>
#include <common/include/rtos_err.h>

#include "wfx_task.h"

// Bus Task Configurations
#define WF200_BUS_TASK_PRIO              15u
#define WF200_BUS_TASK_STK_SIZE         512u
#define WF200_EVENT_TIMEOUT_MS          (20)

//Task Data Structures
static CPU_STK WF200BusTaskStk[WF200_BUS_TASK_STK_SIZE];
OS_TCB WF200BusTaskTCB;

OS_FLAG_GRP wf200_evts;
wf200_frame_q_item tx_frame;
OS_SEM txComplete;
OS_MUTEX   wf200_mutex;
static bool wf200_rx_in_process = false;

/* Default parameters */
sl_wfx_context_t wifi;
char wlan_ssid[32]                     = WLAN_SSID_DEFAULT;
char wlan_passkey[64]                  = WLAN_PASSKEY_DEFAULT;
sl_wfx_security_mode_t wlan_security   = WLAN_SECURITY_DEFAULT;
char softap_ssid[32]                   = SOFTAP_SSID_DEFAULT;
char softap_passkey[64]                = SOFTAP_PASSKEY_DEFAULT;
sl_wfx_security_mode_t softap_security = SOFTAP_SECURITY_DEFAULT;
uint8_t softap_channel                 = SOFTAP_CHANNEL_DEFAULT;


bool isWFXReceiveProcessing (void)
{
	return wf200_rx_in_process;

}

static sl_status_t receive_frames ()
{
  sl_status_t result;
  uint16_t control_register = 0;
  wf200_rx_in_process = true;
  do
  {
    result = sl_wfx_receive_frame(&control_register);

    SL_WFX_ERROR_CHECK( result );
  }while ( (control_register & SL_WFX_CONT_NEXT_LEN_MASK) != 0 );
  wf200_rx_in_process = false;
error_handler:
  return result;
}


/*
 * The task that implements the bus communication with WF200.
 */
static void WF200BusTask (void *p_arg)
{
  RTOS_ERR err;
  sl_status_t result;
  OS_FLAGS  flags=0;
  OSMutexCreate(&wf200_mutex,"wf200 bus mutex",&err);
  OSFlagCreate(&wf200_evts,"wf200 events",0,&err);
  OSSemCreate(&txComplete,"wf200 tx comp",0,&err);
  for( ;; )
  {
#ifdef SLEEP_ENABLED
#ifdef SL_WFX_USE_SPI
	if (GPIO_PinInGet(BSP_EXP_SPI_WIRQPORT,  BSP_EXP_SPI_WIRQPIN)) //wf200 messages pending
#else
    if (GPIO_PinInGet(BSP_EXP_WIRQPORT,  BSP_EXP_WIRQPIN))
#endif
    {
    	OSFlagPost(&wf200_evts, WF200_EVENT_FLAG_RX,OS_OPT_POST_FLAG_SET,&err);
    }
#endif
    /*Wait for an interrupt from WF200*/
	flags = OSFlagPend(&wf200_evts,0xF,WF200_EVENT_TIMEOUT_MS,
			           OS_OPT_PEND_FLAG_SET_ANY | OS_OPT_PEND_BLOCKING | OS_OPT_PEND_FLAG_CONSUME,
					   0, &err);
	if (RTOS_ERR_CODE_GET(err) == RTOS_ERR_TIMEOUT)
	{

	}
    /*Receive the frame(s) pending in WF200*/
    /* See if we can obtain the mutex.  If is not
    available wait to see if it becomes free. */
	if (flags & WF200_EVENT_FLAG_RX)
	{

      OSMutexPend (&wf200_mutex,0,OS_OPT_PEND_BLOCKING,0,&err);

      if (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE)
      {
         result = receive_frames();
         if (result != SL_SUCCESS)
         {
        	 if (result != 1039)
        	 {
    	        printf ("receive_frames() error %d\n",result);
        	 }
         }
         if ((sl_wfx_context->state & SL_WFX_POWER_SAVE_ACTIVE) &&
               !sl_wfx_context->used_buffers && !(flags & WF200_EVENT_FLAG_TX))

         {
           sl_wfx_context->state |= SL_WFX_SLEEPING;
           result = sl_wfx_host_set_wake_up_pin(0);
         }

         OSMutexPost(&wf200_mutex,OS_OPT_POST_NONE,&err);
         if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE)
         {
    	     printf ("ERROR: wf200_mutex. unable to post.\n");
         }
      }
      else
      {
         //unable to receive
      	  printf ("ERROR: wf200_mutex. unable to receive data.\n");
      }

      //reenable interrupt (req for sdio)
#ifdef SL_WFX_USE_SDIO
      sl_wfx_host_enable_platform_interrupt();
#endif
	}
	if (flags & WF200_EVENT_FLAG_TX)
	{
		OSMutexPend (&wf200_mutex,0,OS_OPT_PEND_BLOCKING,0,&err);
		sl_wfx_send_ethernet_frame(tx_frame.frame, tx_frame.data_length, tx_frame.interface,tx_frame.priority);
		OSMutexPost(&wf200_mutex,OS_OPT_POST_NONE,&err);
		OSSemPost(&txComplete,OS_OPT_POST_ALL,&err);
	}


  }
}

/***************************************************************************//**
 * @brief Creates WF200 bus communication task.
 ******************************************************************************/
void WF200BusCommStart()
{
    RTOS_ERR err;

    OSTaskCreate(&WF200BusTaskTCB,
                 "WF200 bus Task",
                  WF200BusTask,
                  DEF_NULL,
				  WF200_BUS_TASK_PRIO,
                 &WF200BusTaskStk[0],
                 (WF200_BUS_TASK_STK_SIZE / 10u),
                  WF200_BUS_TASK_STK_SIZE,
                  0u,
                  0u,
                  DEF_NULL,
                 (OS_OPT_TASK_STK_CLR),
                 &err);
                                                                /*   Check error code.                                  */
    APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
}

