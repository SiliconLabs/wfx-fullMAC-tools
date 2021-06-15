/***************************************************************************//**
 * @file
 * @brief WFX FMAC driver event processing task
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
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
#include "demo_config.h"

#include "em_gpio.h"
#include "em_usart.h"
#include "em_cmu.h"
#include "em_ldma.h"
#include "em_bus.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/include/os.h>

#include "sl_wfx_host_events.h"
#include "sl_wfx_host.h"
#include "sl_wfx_task.h"
#include "sl_wfx.h"
#include "sl_wfx_sae.h"

// Event Task Configurations
#define WFX_EVENTS_TASK_PRIO              21u
#define WFX_EVENTS_TASK_STK_SIZE        1024u
#define WFX_EVENTS_NB_MAX                 10u

struct evt_notif {
  void *ownership;
  sl_wfx_host_events_callback_t cb;
};

/* wifi event task stack*/
static CPU_STK wfx_events_task_stk[WFX_EVENTS_TASK_STK_SIZE];
/* wifi event task TCB*/
static OS_TCB wfx_events_task_tcb;

static struct evt_notif notifications[SL_WFX_EVENT_COUNT] = {0};
static OS_FLAGS waited_flags = 0;

/* WiFi event queue*/
OS_Q wifi_events;

/***************************************************************************//**
 * WiFi events processing task.
 ******************************************************************************/
static void wfx_events_task(void *p_arg)
{
	RTOS_ERR err;
	OS_MSG_SIZE msg_size;
	sl_wfx_generic_message_t *msg;

	(void)p_arg;

	while (1) {
	  msg = (sl_wfx_generic_message_t *)OSQPend(&wifi_events,
												0,
												OS_OPT_PEND_BLOCKING,
												&msg_size,
												NULL,
												&err);
	  if (msg != NULL) {
		  switch (msg->header.id) {
			case SL_WFX_CONNECT_IND_ID:
			{
			  lwip_set_sta_link_up();
			  if (waited_flags & SL_WFX_EVENT_CONNECT) {
					 notifications[0].cb(SL_WFX_EVENT_CONNECT);
			   }

#ifdef SLEEP_ENABLED
			  if (!(wifi_context.state & SL_WFX_AP_INTERFACE_UP)) {
				/* Enable the power save*/
				sl_wfx_set_power_mode(WFM_PM_MODE_PS, 1);
				sl_wfx_enable_device_power_save();
			  }
#endif
			  break;
			}
			case SL_WFX_DISCONNECT_IND_ID:
			{
			  lwip_set_sta_link_down();
			  if (waited_flags & SL_WFX_EVENT_DISCONNECT) {
					 notifications[1].cb(SL_WFX_EVENT_DISCONNECT);
			   }
			  break;
			}
			case SL_WFX_START_AP_IND_ID:
			{
			  lwip_set_ap_link_up();
			  if (waited_flags & SL_WFX_EVENT_START_AP) {
					 notifications[2].cb(SL_WFX_EVENT_START_AP);
			   }
#ifdef SLEEP_ENABLED
			  /* Power save always disabled when SoftAP mode enabled*/
			  sl_wfx_set_power_mode(WFM_PM_MODE_ACTIVE, 0);
			  sl_wfx_disable_device_power_save();
#endif
			  break;
			}
			case SL_WFX_STOP_AP_IND_ID:
			{
			  lwip_set_ap_link_down();
			  if (waited_flags & SL_WFX_EVENT_STOP_AP) {
					  notifications[3].cb(SL_WFX_EVENT_STOP_AP);
				}

#ifdef SLEEP_ENABLED
			  if (wifi_context.state & SL_WFX_STA_INTERFACE_CONNECTED) {
				/* Enable the power save*/
				sl_wfx_set_power_mode(WFM_PM_MODE_PS, 1);
				sl_wfx_enable_device_power_save();
			  }
#endif
			  break;
			}
			case SL_WFX_SCAN_COMPLETE_IND_ID:
			{
				if (waited_flags & SL_WFX_EVENT_SCAN_COMPLETE) {
					notifications[4].cb(SL_WFX_EVENT_SCAN_COMPLETE);
				}
			  break;
			}
			case SL_WFX_EXT_AUTH_IND_ID:
			{
			  sl_wfx_sae_exchange((sl_wfx_ext_auth_ind_t *)msg);
			  if (waited_flags & SL_WFX_EVENT_EXTERNAL_AUTHENTICATION) {
					 notifications[5].cb(SL_WFX_EVENT_EXTERNAL_AUTHENTICATION);
				   }
			  break;
			}
		  }

		  sl_wfx_host_free_buffer(msg, SL_WFX_RX_FRAME_BUFFER);
		}
	}
}

/***************************************************************************//**
 * Creates WFX events processing task.
 ******************************************************************************/
void wfx_events_start () {
  RTOS_ERR err;

  OSQCreate(&wifi_events, "wifi events", WFX_EVENTS_NB_MAX, &err);
  /* Check error code.*/
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

  OSTaskCreate(&wfx_events_task_tcb,
               "WFX events task",
               wfx_events_task,
               DEF_NULL,
               WFX_EVENTS_TASK_PRIO,
               &wfx_events_task_stk[0],
               (WFX_EVENTS_TASK_STK_SIZE / 10u),
               WFX_EVENTS_TASK_STK_SIZE,
               0u,
               0u,
               DEF_NULL,
               (OS_OPT_TASK_STK_CLR),
               &err);
  /* Check error code.*/
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
}

/***************************************************************************//**
 * Register a notification for one or more specific events.
 * event_flags: event flags to notify
 * cb: notification callback
 * ownership: notification ownership (protection against unnotify)
 *            can be set to NULL to bypass the protection
 *
 * return 0 on success, -1 otherwise (a notification is already registered)
 ******************************************************************************/
int sl_wfx_host_events_notify (uint32_t event_flags,
                               sl_wfx_host_events_callback_t cb,
                               void *ownership) {
  int res;

  if ((waited_flags & event_flags) == 0) {
    for (int i=0; i < SL_WFX_EVENT_COUNT; i++) {
      if ((event_flags & (1 << i)) != 0) {
        notifications[i].ownership = ownership;
        notifications[i].cb = cb;
        waited_flags |= (1 << i);
      }
    }
    res = 0;
  } else {
    // One or more flags already have a callback registered
    res = -1;
  }

  return res;
}

/***************************************************************************//**
 * Register a notification for one or more specific events.
 * event_flags: event flags to notify
 * ownership: check ownership before unregistering the notification
 *
 * return 0 on success, -1 otherwise (ownership issue)
 ******************************************************************************/
int sl_wfx_host_events_unnotify (uint32_t event_flags, void *ownership) {
  int res = -1;

  for (int i=0; i < SL_WFX_EVENT_COUNT; i++) {
    if (((event_flags & (1 << i)) != 0)
        && (notifications[i].ownership == ownership)) {
      notifications[i].ownership = NULL;
      notifications[i].cb = NULL;
      waited_flags &= ~(1 << i);
     }
  }

  if ((waited_flags & event_flags) == 0) {
    res = 0;
  }

  return res;
}

/***************************************************************************//**
 * Return the event flags associated to a notification.
 ******************************************************************************/
uint32_t sl_wfx_host_events_waited_events (void) {
  return waited_flags;
}
