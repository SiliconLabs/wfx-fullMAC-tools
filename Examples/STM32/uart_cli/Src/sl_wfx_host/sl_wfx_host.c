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
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "stm32f4xx_hal.h"

#include "sl_wfx.h"
#include "sl_wfx_host.h"
#include "sl_wfx_host_pin.h"
#include "sl_wfx_host_events.h"
#include "sl_wfx_cli_common.h"
#include "lwip_common.h"

/* Firmware include */
#include "sl_wfx_wf200_C0.h"

/* Platform Data Set (PDS) include */
#include "brd8022a_pds.h"
#include "brd8023a_pds.h"

extern osSemaphoreId s_xDriverSemaphore;

sl_wfx_rx_stats_t rx_stats;

scan_result_list_t scan_list[SL_WFX_MAX_SCAN_RESULTS];
uint8_t scan_count = 0; 
uint8_t scan_count_web = 0; 
QueueHandle_t eventQueue;
osSemaphoreId wfx_wakeup_sem;

struct
{
    uint32_t sl_wfx_firmware_download_progress;
    uint8_t waited_event_id;
}host_context;

/* Initialization phase*/
sl_status_t sl_wfx_host_init(void)
{
  host_context.sl_wfx_firmware_download_progress = 0;
  eventQueue = xQueueCreate(SL_WFX_EVENT_LIST_SIZE, sizeof(uint8_t));
  wfx_wakeup_sem = xSemaphoreCreateBinary();
  return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_get_firmware_data(const uint8_t** data, uint32_t data_size)
{
  *data = &sl_wfx_firmware[host_context.sl_wfx_firmware_download_progress];
  host_context.sl_wfx_firmware_download_progress += data_size;
  return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_get_firmware_size(uint32_t* firmware_size)
{
  *firmware_size = sizeof(sl_wfx_firmware);
  return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_get_pds_data(const char **pds_data, uint16_t index)
{
  // Manage dynamically the PDS in function of the chip connected
  if (strncmp("WFM200", (char *)sl_wfx_context->wfx_opn, 6) == 0) {
    *pds_data = pds_table_brd8023a[index];
  } else {
    *pds_data = pds_table_brd8022a[index];
  }
  return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_get_pds_size(uint16_t *pds_size)
{
  // Manage dynamically the PDS in function of the chip connected
  if (strncmp("WFM200", (char *)sl_wfx_context->wfx_opn, 6) == 0) {
    *pds_size = SL_WFX_ARRAY_COUNT(pds_table_brd8023a);
  } else {
    *pds_size = SL_WFX_ARRAY_COUNT(pds_table_brd8022a);
  }
  return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_deinit(void)
{
  return SL_STATUS_OK;
}

/* GPIO interface */
sl_status_t sl_wfx_host_reset_chip(void)
{
  HAL_GPIO_WritePin(SL_WFX_RESET_PORT, SL_WFX_RESET_GPIO, GPIO_PIN_RESET);
  HAL_Delay(10);
  HAL_GPIO_WritePin(SL_WFX_RESET_PORT, SL_WFX_RESET_GPIO, GPIO_PIN_SET);
  HAL_Delay(10);
  
  return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_hold_in_reset(void)
{
  HAL_GPIO_WritePin(SL_WFX_RESET_PORT, SL_WFX_RESET_GPIO, GPIO_PIN_RESET);
  return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_set_wake_up_pin(uint8_t state)
{
  if (state > 0)
  {
    HAL_GPIO_WritePin(SL_WFX_WUP_PORT, SL_WFX_WUP_GPIO, GPIO_PIN_SET);
  }
  else
  {
    HAL_GPIO_WritePin(SL_WFX_WUP_PORT, SL_WFX_WUP_GPIO, GPIO_PIN_RESET);
  }
  return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_wait_for_wake_up(void)
{
  xSemaphoreTake(wfx_wakeup_sem, 0);
  xSemaphoreTake(wfx_wakeup_sem, 3/portTICK_PERIOD_MS);

  return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_sleep_grant(sl_wfx_host_bus_transfer_type_t type,
                                    sl_wfx_register_address_t address,
                                    uint32_t length)
{
  return SL_STATUS_WIFI_SLEEP_GRANTED;
}

sl_status_t sl_wfx_host_setup_waited_event(uint8_t event_id)
{
  host_context.waited_event_id = event_id;
  return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_wait_for_confirmation(uint8_t confirmation_id,
                                              uint32_t timeout_ms,
                                              void **event_payload_out)
{
  uint8_t posted_event_id;
  for(uint32_t i = 0; i < timeout_ms; i++)
  {
    /* Wait for an event posted by the function sl_wfx_host_post_event() */
    if(xQueueReceive(eventQueue, &posted_event_id, 1) == pdTRUE)
    {
      /* Once a message is received, check if it is the expected ID */
      if (confirmation_id == posted_event_id)
      {
        /* Pass the confirmation reply and return*/
        if ( event_payload_out != NULL )
        {
          *event_payload_out = sl_wfx_context->event_payload_buffer;
        }
        return SL_STATUS_OK;
      }
    }
  }
  /* The wait for the confirmation timed out, return */
  return SL_STATUS_TIMEOUT;
}

sl_status_t sl_wfx_host_wait(uint32_t wait_time)
{
  osDelay(wait_time);
  return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_post_event(sl_wfx_generic_message_t *event_payload)
{

  switch(event_payload->header.id){
    /******** INDICATION ********/
  case SL_WFX_CONNECT_IND_ID:
    {
      sl_wfx_connect_ind_t* connect_indication = (sl_wfx_connect_ind_t*) event_payload;
      sl_wfx_connect_callback(connect_indication->body.mac, connect_indication->body.status);
      break;
    }
  case SL_WFX_DISCONNECT_IND_ID:
    {
      sl_wfx_disconnect_ind_t* disconnect_indication = (sl_wfx_disconnect_ind_t*) event_payload;
      sl_wfx_disconnect_callback(disconnect_indication->body.mac, disconnect_indication->body.reason);
      break;
    }
  case SL_WFX_START_AP_IND_ID:
    {
      sl_wfx_start_ap_ind_t* start_ap_indication = (sl_wfx_start_ap_ind_t*) event_payload;
      sl_wfx_start_ap_callback(start_ap_indication->body.status);
      break;
    }
  case SL_WFX_STOP_AP_IND_ID:
    {
      sl_wfx_stop_ap_callback();
      break;
    }
  case SL_WFX_RECEIVED_IND_ID:
    {
      sl_wfx_received_ind_t* ethernet_frame = (sl_wfx_received_ind_t*) event_payload;
      if ( ethernet_frame->body.frame_type == 0 )
      {
        sl_wfx_host_received_frame_callback( ethernet_frame );
      }
      break;
    }
  case SL_WFX_SCAN_RESULT_IND_ID:
    {
      sl_wfx_scan_result_ind_t* scan_result = (sl_wfx_scan_result_ind_t*) event_payload;
      sl_wfx_scan_result_callback(&scan_result->body);
      break;
    }
  case SL_WFX_SCAN_COMPLETE_IND_ID:
    {
      sl_wfx_scan_complete_ind_t* scan_complete = (sl_wfx_scan_complete_ind_t*) event_payload;
      sl_wfx_scan_complete_callback(scan_complete->body.status);
      break;
    }
  case SL_WFX_AP_CLIENT_CONNECTED_IND_ID:
    {
      sl_wfx_ap_client_connected_ind_t* client_connected_indication = (sl_wfx_ap_client_connected_ind_t*) event_payload;
      sl_wfx_client_connected_callback(client_connected_indication->body.mac);
      break;
    }
  case SL_WFX_AP_CLIENT_REJECTED_IND_ID:
    {
      sl_wfx_ap_client_rejected_ind_t* ap_client_rejected_indication = (sl_wfx_ap_client_rejected_ind_t*) event_payload;
      sl_wfx_ap_client_rejected_callback(ap_client_rejected_indication->body.reason, ap_client_rejected_indication->body.mac);
      break;
    }
  case SL_WFX_AP_CLIENT_DISCONNECTED_IND_ID:
    {
      sl_wfx_ap_client_disconnected_ind_t* ap_client_disconnected_indication = (sl_wfx_ap_client_disconnected_ind_t*) event_payload;
      sl_wfx_ap_client_disconnected_callback(ap_client_disconnected_indication->body.reason, ap_client_disconnected_indication->body.mac);
      break;
    }
#ifdef SL_WFX_USE_SECURE_LINK
    case SL_WFX_SECURELINK_EXCHANGE_PUB_KEYS_IND_ID:
    {
      if(host_context.waited_event_id != SL_WFX_SECURELINK_EXCHANGE_PUB_KEYS_IND_ID) {
        memcpy((void*)&sl_wfx_context->secure_link_exchange_ind,(void*)event_payload, event_payload->header.length);
      }
      break;
    }
#endif
  case SL_WFX_GENERIC_IND_ID:
    {
      sl_wfx_generic_ind_t* generic_status = (sl_wfx_generic_ind_t*)event_payload;
      sl_wfx_generic_status_callback(generic_status);
      break;
    }
  case SL_WFX_EXCEPTION_IND_ID:
    {
      sl_wfx_exception_ind_t *firmware_exception = (sl_wfx_exception_ind_t*)event_payload;
      uint8_t *exception_tmp = (uint8_t *) firmware_exception;
      printf("firmware exception %lu\r\n", firmware_exception->body.reason);
      for (uint16_t i = 0; i < firmware_exception->header.length; i += 16) {
        printf("hif: %.8x:", i);
        for (uint8_t j = 0; (j < 16) && ((i + j) < firmware_exception->header.length); j ++) {
            printf(" %.2x", *exception_tmp);
            exception_tmp++;
        }
        printf("\r\n");
      }
      break;
    }
  case SL_WFX_ERROR_IND_ID:
    {
      sl_wfx_error_ind_t *firmware_error = (sl_wfx_error_ind_t*)event_payload;
      uint8_t *error_tmp = (uint8_t *) firmware_error;
      printf("firmware error %lu\r\n", firmware_error->body.type);
      for (uint16_t i = 0; i < firmware_error->header.length; i += 16) {
        printf("hif: %.8x:", i);
        for (uint8_t j = 0; (j < 16) && ((i + j) < firmware_error->header.length); j ++) {
            printf(" %.2x", *error_tmp);
            error_tmp++;
        }
        printf("\r\n");
      }
      break;
    }
  }

  if(host_context.waited_event_id == event_payload->header.id)
  {
    if(event_payload->header.length < SL_WFX_EVENT_MAX_SIZE)
    {
      /* Post the event in the queue */
      memcpy( sl_wfx_context->event_payload_buffer,
             (void*) event_payload,
             event_payload->header.length );
      xQueueOverwrite(eventQueue, (void *) &event_payload->header.id);
    }
  }

  return SL_STATUS_OK;
}

/* Memory management */
sl_status_t sl_wfx_host_allocate_buffer(void** buffer, sl_wfx_buffer_type_t type, uint32_t buffer_size)
{
  *buffer = pvPortMalloc( buffer_size );
  return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_free_buffer(void* buffer, sl_wfx_buffer_type_t type)
{
  vPortFree( buffer );
  return SL_STATUS_OK;
}

/* Frame hook */
sl_status_t sl_wfx_host_transmit_frame(void* frame, uint32_t frame_len)
{
  return sl_wfx_data_write( frame, frame_len );
}

#if SL_WFX_DEBUG_MASK
void sl_wfx_host_log(const char *string, ...)
{
  va_list valist;

  va_start(valist, string);
  vprintf(string, valist);
  va_end(valist);
}
#endif

/* Driver mutex handling */
sl_status_t sl_wfx_host_lock(void)
{
  sl_status_t status;
  
  if(xSemaphoreTake(s_xDriverSemaphore, 500) == pdTRUE)
  {
    status = SL_STATUS_OK;
  }else{
    printf("Wi-Fi driver mutex timeout\r\n");
    status = SL_STATUS_NO_MORE_RESOURCE;
  }
  
  return status;
}

sl_status_t sl_wfx_host_unlock(void)
{
  xSemaphoreGive(s_xDriverSemaphore);
  
  return SL_STATUS_OK;
}

/** Callback for individual AP discovered
 */
void sl_wfx_scan_result_callback(sl_wfx_scan_result_ind_body_t* scan_result)
{
  scan_count++;
  printf("# %2d %2d  %03d %02X:%02X:%02X:%02X:%02X:%02X  %s\r\n",
          scan_count,
          scan_result->channel,
          ((int16_t)(scan_result->rcpi - 220)/2),
          scan_result->mac[0], scan_result->mac[1],
          scan_result->mac[2], scan_result->mac[3],
          scan_result->mac[4], scan_result->mac[5],
          scan_result->ssid_def.ssid);
  if (scan_count <= SL_WFX_MAX_SCAN_RESULTS)
  {
    scan_list[scan_count - 1].ssid_def = scan_result->ssid_def;
    scan_list[scan_count - 1].channel = scan_result->channel;
    scan_list[scan_count - 1].security_mode = scan_result->security_mode;
    scan_list[scan_count - 1].rcpi = scan_result->rcpi;
    memcpy(scan_list[scan_count - 1].mac, scan_result->mac, 6);
  }
}

/** Callback to indicate the scan completion
 */
void sl_wfx_scan_complete_callback(uint32_t status)
{
  scan_count_web = scan_count;
  scan_count = 0;
  xEventGroupSetBits(wifi_events, SL_WFX_EVENT_SCAN_COMPLETE);
}

/** Callback triggered when a connection is established as station
 */
void sl_wfx_connect_callback(uint8_t* mac, uint32_t status)
{  
  switch(status){
  case WFM_STATUS_SUCCESS:
    {
      printf("Connected\r\n");
      sl_wfx_context->state |= SL_WFX_STA_INTERFACE_CONNECTED;
      xEventGroupSetBits(wifi_events, SL_WFX_EVENT_CONNECT);
      break;
    }
  case WFM_STATUS_NO_MATCHING_AP:
    {
      printf("Connection failed, access point not found\r\n");
      break;
    }
  case WFM_STATUS_CONNECTION_ABORTED:
    {
      printf("Connection aborted\r\n");
      break;
    }
  case WFM_STATUS_CONNECTION_TIMEOUT:
    {
      printf("Connection timeout\r\n");
      break;
    }
  case WFM_STATUS_CONNECTION_REJECTED_BY_AP:
    {
      printf("Connection rejected by the access point\r\n");
      break;
    }
  case WFM_STATUS_CONNECTION_AUTH_FAILURE:
    {
      printf("Connection authentication failure\r\n");
      break;
    }
  default:
    {
      printf("Connection attempt error\r\n");
    }
  }
}

/** Callback triggered when signal strength confirmation is received
 */
void sl_wfx_disconnect_callback(uint8_t* mac, uint16_t reason)
{
  printf("Disconnected %d\r\n", reason);
  sl_wfx_context->state &= ~SL_WFX_STA_INTERFACE_CONNECTED;
  xEventGroupSetBits(wifi_events, SL_WFX_EVENT_DISCONNECT);
}

/** Callback triggered when a softap is started
 */
void sl_wfx_start_ap_callback(uint32_t status)
{
  if (status == 0) {
    printf("AP started\r\n");
    printf("Join the AP with SSID: %s\r\n", softap_ssid);
    sl_wfx_context->state |= SL_WFX_AP_INTERFACE_UP;
    xEventGroupSetBits(wifi_events, SL_WFX_EVENT_START_AP);
  } else {
    printf("AP start failed\r\n");
  }
}

/** Callback triggered when a softap is stopped
 */
void sl_wfx_stop_ap_callback(void)
{
  printf("SoftAP stopped\r\n");
  sl_wfx_context->state &= ~SL_WFX_AP_INTERFACE_UP;
  xEventGroupSetBits(wifi_events, SL_WFX_EVENT_STOP_AP);
}

/** Callback triggered when a client connects
 */
void sl_wfx_client_connected_callback(uint8_t* mac)
{
//  printf("Client connected, MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
//         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  sl_wfx_cli_wifi_add_client(mac);
}

/** Callback triggered when a client is rejected
 */
void sl_wfx_ap_client_rejected_callback(uint32_t status, uint8_t* mac)
{
//  printf("Client rejected, reason: %d, MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
//         status, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

/** Callback triggered when a client disconnects
 */
void sl_wfx_ap_client_disconnected_callback(uint32_t status, uint8_t* mac)
{
//  printf("Client disconnected, reason: %d, MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
//         status, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  sl_wfx_cli_wifi_remove_client(mac);
}

void sl_wfx_generic_status_callback(sl_wfx_generic_ind_t* frame)
{
  rx_stats = frame->body.indication_data.rx_stats;
}
