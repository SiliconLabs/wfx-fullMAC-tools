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

#include "sl_wfx_host.h"
#include "sl_wfx_host_cfg.h"
#include "sl_wfx_task.h"
#include "sl_wfx.h"

wfx_frame_q_item   wfxtask_tx_frame;
SemaphoreHandle_t  wfxtask_tx_complete;
SemaphoreHandle_t  wfxtask_mutex ;

SemaphoreHandle_t  s_xDriverSemaphore;
EventGroupHandle_t sl_wfx_event_group;
osThreadId busCommTaskHandle ;

SemaphoreHandle_t spi_irq;
// Flag to indicate receive frames is currently running.
static bool wfx_rx_in_process = false;

// wfx_fmac_driver context
sl_wfx_context_t wifi;

// Connection parameters
char wlan_ssid[32]                     = WLAN_SSID_DEFAULT;
char wlan_passkey[64]                  = WLAN_PASSKEY_DEFAULT;
sl_wfx_security_mode_t wlan_security   = WLAN_SECURITY_DEFAULT;
char softap_ssid[32]                   = SOFTAP_SSID_DEFAULT;
char softap_passkey[64]                = SOFTAP_PASSKEY_DEFAULT;
sl_wfx_security_mode_t softap_security = SOFTAP_SECURITY_DEFAULT;
uint8_t softap_channel                 = SOFTAP_CHANNEL_DEFAULT;

/***************************************************************************//**
 * Check receive frame status
 ******************************************************************************/
bool wfxtask_is_receive_processing(void)
{
  return wfx_rx_in_process;
}

/***************************************************************************//**
 * Receives frames from the WFX.
 ******************************************************************************/
static sl_status_t receive_frames()
{
  sl_status_t result;
  uint16_t control_register = 0;

  do {
    result = sl_wfx_receive_frame(&control_register);
    SL_WFX_ERROR_CHECK(result);
  } while ( (control_register & SL_WFX_CONT_NEXT_LEN_MASK) != 0 );

error_handler:
  return result;
}

/***************************************************************************//**
 * WFX bus communication task.
 ******************************************************************************/
static void wfx_bus_task(void const *p_arg)
{
  /* create a mutex used for making driver accesses atomic */
  s_xDriverSemaphore = xSemaphoreCreateMutex();

  /* create an event group to track Wi-Fi events */
  sl_wfx_event_group = xEventGroupCreate();

  for(;;)
  {
    /*Wait for an interrupt from WFX*/
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    /*Disable the interrupt while treating frames received to avoid
     *the case where the interrupt is set but there is no frame left to treat.*/
    sl_wfx_host_disable_platform_interrupt();

    /*Receive the frame(s) pending in WFX*/
    receive_frames();

    /*Re-enable the interrupt*/
    sl_wfx_host_enable_platform_interrupt();
  }
}

/***************************************************************************//**
 * Creates WFX bus communication task.
 ******************************************************************************/
void wfx_bus_start()
{
  osThreadDef(busCommTask, wfx_bus_task, osPriorityRealtime, 0, 512);
  busCommTaskHandle = osThreadCreate(osThread(busCommTask), NULL);
}
