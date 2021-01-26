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
#ifndef SL_WFX_HOST_EVENTS_H
#define SL_WFX_HOST_EVENTS_H

#include <kernel/include/os.h>

extern OS_Q wifi_events;

/* Wi-Fi events*/
#define SL_WFX_EVENT_CONNECT                      ( 1 << 0 )
#define SL_WFX_EVENT_DISCONNECT                   ( 1 << 1 )
#define SL_WFX_EVENT_START_AP                     ( 1 << 2 )
#define SL_WFX_EVENT_STOP_AP                      ( 1 << 3 )
#define SL_WFX_EVENT_SCAN_COMPLETE                ( 1 << 4 )
#define SL_WFX_EVENT_EXTERNAL_AUTHENTICATION      ( 1 << 5 )

#define SL_WFX_EVENT_ALL_FLAGS          ( SL_WFX_EVENT_CONNECT \
                                        | SL_WFX_EVENT_STOP_AP \
                                        | SL_WFX_EVENT_DISCONNECT \
                                        | SL_WFX_EVENT_START_AP \
                                        | SL_WFX_EVENT_SCAN_COMPLETE\
										| SL_WFX_EVENT_EXTERNAL_AUTHENTICATION )

#define SL_WFX_EVENT_COUNT              6

typedef void (*sl_wfx_host_events_callback_t)(uint32_t event_flag);

#ifdef __cplusplus
extern "C" {
#endif
/***************************************************************************//**
 * Creates WIFI events processing task.
 ******************************************************************************/
void wfx_events_start(void);

/***************************************************************************//**
 * Register a notification for one or more specific events.
 * event_flags: event flags to notify
 * cb: notification callback
 * ownership: notification ownership (protection against unnotify)
 *            can be set to NULL to bypass the protection
 *
 * return 0 on success, -1 otherwise (a notification is already registered)
 ******************************************************************************/
int sl_wfx_host_events_notify(uint32_t event_flags,
                              sl_wfx_host_events_callback_t cb,
                              void *ownership);

/***************************************************************************//**
 * Register a notification for one or more specific events.
 * event_flags: event flags to notify
 * ownership: check ownership before unregistering the notification
 *
 * return 0 on success, -1 otherwise (ownership issue)
 ******************************************************************************/
int sl_wfx_host_events_unnotify(uint32_t event_flags, void *ownership);

/***************************************************************************//**
 * Return the event flags associated to a notification.
 ******************************************************************************/
uint32_t sl_wfx_host_events_waited_events(void);

#ifdef __cplusplus
}
#endif

#endif
