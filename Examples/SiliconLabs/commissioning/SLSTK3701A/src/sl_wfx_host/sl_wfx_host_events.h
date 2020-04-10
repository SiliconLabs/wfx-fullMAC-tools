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
#ifndef SL_WFX_EVENTS_TASK_H
#define SL_WFX_EVENTS_TASK_H

/* Wi-Fi events*/
#define SL_WFX_EVENT_CONNECT          ( 1 << 1 )
#define SL_WFX_EVENT_DISCONNECT       ( 1 << 2 )
#define SL_WFX_EVENT_START_AP         ( 1 << 3 )
#define SL_WFX_EVENT_STOP_AP          ( 1 << 4 )
#define SL_WFX_EVENT_SCAN_COMPLETE    ( 1 << 5 )

extern OS_FLAG_GRP wifi_events;

#ifdef __cplusplus
extern "C" {
#endif
/***************************************************************************//**
 * Creates WFX events processing task.
 ******************************************************************************/
void wifi_start(void);

#ifdef __cplusplus
}
#endif

#endif
