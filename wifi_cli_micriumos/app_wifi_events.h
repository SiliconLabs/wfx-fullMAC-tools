/**************************************************************************//**
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
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
#ifndef APP_WIFI_EVENTS_H
#define APP_WIFI_EVENTS_H

#include "sl_wfx_constants.h"
#include <kernel/include/os.h>
#include <common/include/rtos_utils.h>
#include <common/include/rtos_err.h>
#include <common/include/rtos_err.h>

/* Wi-Fi context */
extern sl_wfx_context_t   wifi;
/* Wi-Fi event message queue */
extern OS_Q               wifi_events;

extern scan_result_list_t scan_list[];
extern uint8_t scan_count_web;
extern bool scan_verbose;

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * Initialize the WFX and create a task processing Wi-Fi events.
 ******************************************************************************/
void app_wifi_events_start(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_WIFI_EVENTS_H */
