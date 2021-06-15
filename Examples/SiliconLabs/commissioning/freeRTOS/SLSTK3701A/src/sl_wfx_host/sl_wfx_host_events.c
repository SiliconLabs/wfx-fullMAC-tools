/***************************************************************************//**
 * @file
 * @brief WFX FMAC driver main bus communication task
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "em_gpio.h"
#include "em_usart.h"
#include "em_cmu.h"
#include "em_ldma.h"
#include "em_bus.h"

#include "cmsis_os.h"

#include "demo_config.h"

#include "sl_wfx_task.h"
#include "sl_wfx_host.h"
#include "sl_wfx_host_events.h"

#define SL_WFX_EVENTS_NB_MAX                 10u

osThreadId wfx_events_task_handle ;
QueueHandle_t         wifi_event_queue;
/***************************************************************************//**
 * WFX events processing task.
 ******************************************************************************/
static void wfx_events_task(void const *p_arg)
{
  sl_wfx_generic_message_t *msg;
  BaseType_t ret;

  while(1) {
      ret = xQueueReceive(wifi_event_queue, (void *)&msg, portMAX_DELAY);

      if ((ret == pdTRUE) && (msg != NULL)) {
        switch (msg->header.id) {
          case SL_WFX_CONNECT_IND_ID:
          {
            lwip_set_sta_link_up();

  #ifdef SLEEP_ENABLED
            if (!(wifi_context.state & SL_WFX_AP_INTERFACE_UP)) {
              /* Enable the power save */
              sl_wfx_set_power_mode(WFM_PM_MODE_PS, WFM_PM_POLL_FAST_PS,1);
              sl_wfx_enable_device_power_save();
            }
  #endif
            break;
          }
          case SL_WFX_DISCONNECT_IND_ID:
          {
            lwip_set_sta_link_down();
            break;
          }
          case SL_WFX_START_AP_IND_ID:
          {
            lwip_set_ap_link_up();

  #ifdef SLEEP_ENABLED
            /* Power save always disabled when SoftAP mode enabled */
            sl_wfx_set_power_mode(WFM_PM_MODE_ACTIVE, WFM_PM_POLL_FAST_PS, 0);
            sl_wfx_disable_device_power_save();
  #endif
            break;
          }
          case SL_WFX_STOP_AP_IND_ID:
          {
            lwip_set_ap_link_down();

  #ifdef SLEEP_ENABLED
            if (wifi_context.state & SL_WFX_STA_INTERFACE_CONNECTED) {
              /* Enable the power save */
              sl_wfx_set_power_mode(WFM_PM_MODE_PS, WFM_PM_POLL_FAST_PS,1);
              sl_wfx_enable_device_power_save();
            }
  #endif
            break;
          }
          case SL_WFX_SCAN_COMPLETE_IND_ID:
          {
            xSemaphoreGive(wifi_scan_sem);
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
void wfx_events_task_start()
{
  osThreadDef(wfx_events, wfx_events_task, osPriorityBelowNormal, 0, 1024);
  wfx_events_task_handle = osThreadCreate(osThread(wfx_events), NULL);
  wifi_event_queue = xQueueCreate(SL_WFX_EVENTS_NB_MAX, sizeof(void *));
}
