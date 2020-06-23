/***************************************************************************//**
 * @file
 * @brief WFX FMAC driver event processing task
 *******************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
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

// Event Task Configurations
#define WIFI_EVENTS_TASK_PRIO              21u
#define WIFI_EVENTS_TASK_STK_SIZE         512u

struct evt_notif {
  void *ownership;
  sl_wfx_host_events_callback_t cb;
};

/// wifi event task stack
static CPU_STK wifi_events_task_stk[WIFI_EVENTS_TASK_STK_SIZE];
/// wifi event task TCB
static OS_TCB wifi_events_task_tcb;

static struct evt_notif notifications[SL_WFX_EVENT_COUNT] = {0};
static OS_FLAGS waited_flags = 0;

/// WiFi event flags
OS_FLAG_GRP wifi_events;

/***************************************************************************//**
 * WiFi events processing task.
 ******************************************************************************/
static void wifi_events_task(void *p_arg)
{
  OS_FLAGS all_flags = SL_WFX_EVENT_ALL_FLAGS;
  OS_FLAGS flags = 0;
  RTOS_ERR err;

  while (1)
  {
    // Wait for specific events
    flags = OSFlagPend(&wifi_events,
                       all_flags,
                       0,
                       OS_OPT_PEND_FLAG_SET_ANY | OS_OPT_PEND_BLOCKING | OS_OPT_PEND_FLAG_CONSUME,
                       0,
                       &err);

    // Realize generic actions related to the event nature
    if (flags & SL_WFX_EVENT_CONNECT) {
      lwip_set_sta_link_up();

      if (waited_flags & SL_WFX_EVENT_CONNECT) {
        notifications[0].cb(SL_WFX_EVENT_CONNECT);
      }
    }

    if (flags & SL_WFX_EVENT_DISCONNECT){
      lwip_set_sta_link_down();

      if (waited_flags & SL_WFX_EVENT_DISCONNECT) {
        notifications[1].cb(SL_WFX_EVENT_DISCONNECT);
      }
    }

    if (flags & SL_WFX_EVENT_START_AP){
      lwip_set_ap_link_up();

      if (waited_flags & SL_WFX_EVENT_START_AP) {
        notifications[2].cb(SL_WFX_EVENT_START_AP);
      }
    }

    if (flags & SL_WFX_EVENT_STOP_AP){
      lwip_set_ap_link_down();

      if (waited_flags & SL_WFX_EVENT_STOP_AP) {
        notifications[3].cb(SL_WFX_EVENT_STOP_AP);
      }
    }

    if (flags & SL_WFX_EVENT_SCAN_COMPLETE){
      if (waited_flags & SL_WFX_EVENT_SCAN_COMPLETE) {
        notifications[4].cb(SL_WFX_EVENT_SCAN_COMPLETE);
      }
    }
  }
}

/***************************************************************************//**
 * Creates WFX events processing task.
 ******************************************************************************/
void wifi_events_start()
{
  RTOS_ERR err;

  OSFlagCreate(&wifi_events, "wifi events", 0, &err);
  //   Check error code.
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

  OSTaskCreate(&wifi_events_task_tcb,
               "WiFi events task",
               wifi_events_task,
               DEF_NULL,
               WIFI_EVENTS_TASK_PRIO,
               &wifi_events_task_stk[0],
               (WIFI_EVENTS_TASK_STK_SIZE / 10u),
               WIFI_EVENTS_TASK_STK_SIZE,
               0u,
               0u,
               DEF_NULL,
               (OS_OPT_TASK_STK_CLR),
               &err);
  //   Check error code.
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
                               void *ownership)
{
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
int sl_wfx_host_events_unnotify (uint32_t event_flags, void *ownership)
{
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
uint32_t sl_wfx_host_events_waited_events (void)
{
  return waited_flags;
}
