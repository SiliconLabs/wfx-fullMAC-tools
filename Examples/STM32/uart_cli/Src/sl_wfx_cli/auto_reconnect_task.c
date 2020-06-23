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
#include "string.h"
#include "sl_wfx.h"

osThreadId AutoReconnectTaskHandle;

extern sl_wfx_context_t wifi;
extern char wlan_ssid[];
extern char wlan_passkey[];
extern sl_wfx_security_mode_t wlan_security;

/*
 * The task that implement the auto-reconnect mechanism.
 */
static void prvAutoReconnectTask(void const * pvParameters );

void vAutoReconnectStart( void )
{
  osThreadDef(AutoReconnectTask, prvAutoReconnectTask, osPriorityLow, 0, 128);
  AutoReconnectTaskHandle = osThreadCreate(osThread(AutoReconnectTask), NULL);
}

static void prvAutoReconnectTask(void const * pvParameters )
{

  for( ;; )
  {
    osDelay(5000);
    /*If WF200 is not in a connected state, try to connect to the access point*/
    if((wifi.state & SL_WFX_STA_INTERFACE_CONNECTED) != SL_WFX_STA_INTERFACE_CONNECTED)
    {
      sl_wfx_send_join_command((uint8_t*) wlan_ssid, strlen(wlan_ssid), NULL, 0, wlan_security, 1, 0, (uint8_t*) wlan_passkey, strlen(wlan_passkey), NULL, 0);
    }
  }
}
