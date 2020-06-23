/**************************************************************************//**
 * Copyright 2018, Silicon Laboratories Inc.
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

#include "cmsis_os.h"
#include "sl_wfx_host.h"
#include "sl_wfx_host_events.h"
#include "lwip_common.h"

struct evt_notif {
  void *ownership;
  sl_wfx_host_events_callback_t cb;
};

extern sl_wfx_context_t wifi;

EventGroupHandle_t wifi_events;

static struct evt_notif notifications[SL_WFX_EVENT_COUNT] = {0};
static uint32_t waited_flags = 0;
/*
 * The task that implements Wi-Fi events handling.
 */
static void wifi_events_task(void const * pvParameters);

void wifi_events_start(void)
{
  wifi_events = xEventGroupCreate();
  osThreadDef(eventsTask, wifi_events_task, osPriorityBelowNormal, 0, 128);
  osThreadCreate(osThread(eventsTask), NULL);
}

static void wifi_events_task(void const * pvParameters)
{
  EventBits_t flags;
  EventBits_t all_flags = SL_WFX_EVENT_ALL_FLAGS;
  
  for(;;)
  {
    // Wait for specific events
    flags = xEventGroupWaitBits(wifi_events,
                                all_flags,
                                pdTRUE,
                                pdFALSE,
                                5000/portTICK_PERIOD_MS);
    
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
