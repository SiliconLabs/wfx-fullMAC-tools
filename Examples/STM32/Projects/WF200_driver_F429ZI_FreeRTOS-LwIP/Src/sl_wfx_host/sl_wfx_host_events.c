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
#include "demo_config.h"

osThreadId wifi_events_task_handle;
extern EventGroupHandle_t sl_wfx_event_group;
extern sl_wfx_context_t wifi;

/*
 * The task that implements Wi-Fi events handling.
 */
static void wifi_events_task(void const * pvParameters);

void wifi_events_start(void)
{
  osThreadDef(wifi_events, wifi_events_task, osPriorityBelowNormal, 0, 128);
  wifi_events_task_handle = osThreadCreate(osThread(wifi_events), NULL);
}

static void wifi_events_task(void const * pvParameters)
{
  EventBits_t flags;
  
  for(;;)
  {
    flags = xEventGroupWaitBits(sl_wfx_event_group,
                                SL_WFX_CONNECT | SL_WFX_STOP_AP | SL_WFX_DISCONNECT | SL_WFX_START_AP,
                                pdTRUE,
                                pdFALSE,
                                5000/portTICK_PERIOD_MS);
    if (flags & SL_WFX_CONNECT) {
      lwip_set_sta_link_up();

#ifdef SLEEP_ENABLED
      if (!(wifi.state & SL_WFX_AP_INTERFACE_UP)) {
        // Enable the power save
        sl_wfx_set_power_mode(WFM_PM_MODE_PS, 0);
        sl_wfx_enable_device_power_save();
      }
#endif
    }
    if (flags & SL_WFX_DISCONNECT){
      lwip_set_sta_link_down();
    }
    if (flags & SL_WFX_START_AP){
      lwip_set_ap_link_up();

#ifdef SLEEP_ENABLED
      // Power save always disabled when SoftAP mode enabled
      sl_wfx_set_power_mode(WFM_PM_MODE_ACTIVE, 0);
      sl_wfx_disable_device_power_save();
#endif
    }
    if (flags & SL_WFX_STOP_AP){
      lwip_set_ap_link_down();

#ifdef SLEEP_ENABLED
      if (wifi.state & SL_WFX_STA_INTERFACE_CONNECTED) {
        // Enable the power save
        sl_wfx_set_power_mode(WFM_PM_MODE_PS, 0);
        sl_wfx_enable_device_power_save();
      }
#endif
    }
  }
}

