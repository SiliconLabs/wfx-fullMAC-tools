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
#include "sl_wfx.h"
#include "lwip_freertos.h"
   
osThreadId busCommTaskHandle;
osSemaphoreId s_xDriverSemaphore;

/* Default parameters */
sl_wfx_context_t wifi;
char wlan_ssid[32]                        = WLAN_SSID_DEFAULT;
char wlan_passkey[64]                     = WLAN_PASSKEY_DEFAULT;
sl_wfx_security_mode_t wlan_security      = WLAN_SECURITY_DEFAULT;
char softap_ssid[32]                      = SOFTAP_SSID_DEFAULT;
char softap_passkey[64]                   = SOFTAP_PASSKEY_DEFAULT;
sl_wfx_security_mode_t softap_security    = SOFTAP_SECURITY_DEFAULT;
uint8_t softap_channel                    = SOFTAP_CHANNEL_DEFAULT;
/* Default parameters */

/*
 * The task that implements the bus communication with WF200.
 */
static void prvBusCommTask(void const * pvParameters);

void vBusCommStart(void)
{
  osThreadDef(busCommTask, prvBusCommTask, osPriorityRealtime, 0, 512);
  busCommTaskHandle = osThreadCreate(osThread(busCommTask), NULL);
}

static sl_status_t receive_frames(void)
{
  sl_status_t result;
  uint16_t control_register = 0;
  
  do
  {
    result = sl_wfx_receive_frame(&control_register);
    SL_WFX_ERROR_CHECK( result );
  }while ((control_register & SL_WFX_CONT_NEXT_LEN_MASK) != 0);
  
error_handler:
  return result;
}

static void prvBusCommTask(void const * pvParameters)
{
  /* create a semaphore used for making driver accesses atomic */
  s_xDriverSemaphore = xSemaphoreCreateMutex();
    
  for(;;)
  {
    /*Wait for an interrupt from WF200*/
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    /*Receive the frame(s) pending in WF200*/

    /* See if we can obtain the semaphore.  If the semaphore is not
    available wait to see if it becomes free. */
    if(xSemaphoreTake(s_xDriverSemaphore, 500) == pdTRUE)
    {
      receive_frames();
      xSemaphoreGive(s_xDriverSemaphore);
    }
    else
    {
      printf("Wi-Fi RX sem timeout\r\n");
    }
  }
}

