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
#ifndef WFX_TASK_H
#define WFX_TASK_H

#include <kernel/include/os.h>
#include "sl_wfx_constants.h"
extern OS_FLAG_GRP wf200_evts;
#define WF200_EVENT_FLAG_RX      1
#define WF200_EVENT_FLAG_TX      2
#define WF200_EVENT_WAKE         4
#define WF200_QUEUE_SIZE       10
typedef struct {
	sl_wfx_send_frame_req_t *frame;
	uint32_t data_length;
	sl_wfx_interface_t interface;
	uint8_t priority;
} wf200_frame_q_item;

extern wf200_frame_q_item tx_frame; //todo: make this a queue
extern OS_SEM txComplete;
bool isWFXReceiveProcessing (void);
extern OS_MUTEX   wf200_mutex;

#endif
