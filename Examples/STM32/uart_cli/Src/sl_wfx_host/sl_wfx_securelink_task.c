/**************************************************************************//**
 * Copyright 2021, Silicon Laboratories Inc.
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

#include <stdio.h>
#include "cmsis_os.h"
#include "sl_wfx.h"
#include "sl_wfx_secure_link.h"

osThreadId    sl_wfx_secure_link_task_handle;
osSemaphoreId sl_wfx_secure_link_mutex;

static void sl_wfx_secure_link_task_entry(void const * pvParameters);

/***************************************************************************//**
 * Creates WFX securelink key renegotiation task.
 ******************************************************************************/
void sl_wfx_secure_link_start (void) {
  osThreadDef(secure_link_task, sl_wfx_secure_link_task_entry, osPriorityLow, 0, 128);
  sl_wfx_secure_link_task_handle = osThreadCreate(osThread(secure_link_task), NULL);
}

/**************************************************************************//**
 * The task that implements the Secure Link renegotiation with WFX.
 *****************************************************************************/
static void sl_wfx_secure_link_task_entry (void const * pvParameters) {
  sl_status_t result;
  
  /* Create a mutex used for making Secure Link renegotiations atomic */
  sl_wfx_secure_link_mutex = xSemaphoreCreateMutex();
  
  while(1) {
    /* Wait for a key renegotiation request */
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    
    result = sl_wfx_secure_link_renegotiate_session_key();
    if (result != SL_STATUS_OK) {
      printf("session key negotiation error %lu\n",result);
    }
  }
}
