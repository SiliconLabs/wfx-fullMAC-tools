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
#include "dhcp_server.h"

/* Firmware include */
#include "sl_wfx_wf200_C0.h"

/* Platform Data Set (PDS) include */
#include "brd8022a_pds.h"
#include "brd8023a_pds.h"

extern int use_dhcp_client;
extern char event_log[];

sl_wfx_rx_stats_t rx_stats;
scan_result_list_t scan_list[SL_WFX_MAX_SCAN_RESULTS];
uint8_t scan_count = 0;
uint8_t scan_count_web = 0;
bool scan_verbose = true;

QueueHandle_t sl_wfx_confirmation_queue;
osSemaphoreId sl_wfx_wake_up_sem;
osSemaphoreId sl_wfx_driver_mutex;

struct{
  uint32_t sl_wfx_firmware_download_progress;
  uint8_t waited_event_id;
}host_context;

/**************************************************************************//**
 * Initialize the host resources
 *****************************************************************************/
sl_status_t sl_wfx_host_init (void) {
  host_context.sl_wfx_firmware_download_progress = 0;
  sl_wfx_driver_mutex = xSemaphoreCreateMutex();
  sl_wfx_wake_up_sem = xSemaphoreCreateBinary();
  sl_wfx_confirmation_queue = xQueueCreate(1, sizeof(uint8_t));
  
  return SL_STATUS_OK;
}

/**************************************************************************//**
 * Get a firmware chunk
 *****************************************************************************/
sl_status_t sl_wfx_host_get_firmware_data (const uint8_t** data, uint32_t data_size) {
  *data = &sl_wfx_firmware[host_context.sl_wfx_firmware_download_progress];
  host_context.sl_wfx_firmware_download_progress += data_size;
  
  return SL_STATUS_OK;
}

/**************************************************************************//**
 * Get the firmware size
 *****************************************************************************/
sl_status_t sl_wfx_host_get_firmware_size (uint32_t* firmware_size) {
  *firmware_size = sizeof(sl_wfx_firmware);
  
  return SL_STATUS_OK;
}

/**************************************************************************//**
 * Get a PDS chunk
 *****************************************************************************/
sl_status_t sl_wfx_host_get_pds_data (const char **pds_data, uint16_t index) {
  /* Manage dynamically the PDS in function of the chip connected */
  if (strncmp("WFM200", (char *)sl_wfx_context->wfx_opn, 6) == 0) {
    *pds_data = pds_table_brd8023a[index];
  } else {
    *pds_data = pds_table_brd8022a[index];
  }
  
  return SL_STATUS_OK;
}

/**************************************************************************//**
 * Get PDS size
 *****************************************************************************/
sl_status_t sl_wfx_host_get_pds_size(uint16_t *pds_size)
{
  /* Manage dynamically the PDS in function of the chip connected */
  if (strncmp("WFM200", (char *)sl_wfx_context->wfx_opn, 6) == 0) {
    *pds_size = SL_WFX_ARRAY_COUNT(pds_table_brd8023a);
  } else {
    *pds_size = SL_WFX_ARRAY_COUNT(pds_table_brd8022a);
  }
  
  return SL_STATUS_OK;
}

/**************************************************************************//**
 * Deinit the host resources
 *****************************************************************************/
sl_status_t sl_wfx_host_deinit (void) {
  //Commented for now, waiting for driver fix
  //vQueueDelete(wifi_event_queue);
  //vSemaphoreDelete(sl_wfx_driver_mutex);
  //vSemaphoreDelete(sl_wfx_wake_up_sem);

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * Reset the Wi-Fi device
 *****************************************************************************/
sl_status_t sl_wfx_host_reset_chip(void) {
  HAL_GPIO_WritePin(SL_WFX_RESET_PORT, SL_WFX_RESET_GPIO, GPIO_PIN_RESET);
  HAL_Delay(10);
  HAL_GPIO_WritePin(SL_WFX_RESET_PORT, SL_WFX_RESET_GPIO, GPIO_PIN_SET);
  HAL_Delay(10);

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * Hold the reset pin in reset mode
 *****************************************************************************/
sl_status_t sl_wfx_host_hold_in_reset (void) {
  HAL_GPIO_WritePin(SL_WFX_RESET_PORT, SL_WFX_RESET_GPIO, GPIO_PIN_RESET);
  
  return SL_STATUS_OK;
}

/**************************************************************************//**
 * Set or reset the wake-up pin 
 *****************************************************************************/
sl_status_t sl_wfx_host_set_wake_up_pin (uint8_t state) {
  if (state > 0) {
    HAL_GPIO_WritePin(SL_WFX_WUP_PORT, SL_WFX_WUP_GPIO, GPIO_PIN_SET);
  } else {
    HAL_GPIO_WritePin(SL_WFX_WUP_PORT, SL_WFX_WUP_GPIO, GPIO_PIN_RESET);
  }
  
  return SL_STATUS_OK;
}

/**************************************************************************//**
 * Wait for a wake-up
 *****************************************************************************/
sl_status_t sl_wfx_host_wait_for_wake_up (void) {
  xSemaphoreTake(sl_wfx_wake_up_sem, 0);
  xSemaphoreTake(sl_wfx_wake_up_sem, 3/portTICK_PERIOD_MS);

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * Grant the Wi-Fi device to go to sleep mode
 *****************************************************************************/
sl_status_t sl_wfx_host_sleep_grant (sl_wfx_host_bus_transfer_type_t type,
                                     sl_wfx_register_address_t address,
                                     uint32_t length) {
  /* To be implemented depending on the application */ 
  return SL_STATUS_WIFI_SLEEP_GRANTED;
}

/**************************************************************************//**
 * Set up a waited event
 *****************************************************************************/
sl_status_t sl_wfx_host_setup_waited_event (uint8_t event_id) {
  host_context.waited_event_id = event_id;
  
  return SL_STATUS_OK;
}

/**************************************************************************//**
 * Wait for a confirmation
 *****************************************************************************/
sl_status_t sl_wfx_host_wait_for_confirmation (uint8_t confirmation_id,
                                               uint32_t timeout_ms,
                                               void **event_payload_out) {
  uint8_t posted_event_id;
  
  for (uint32_t i = 0; i < timeout_ms; i++) {
    /* Wait for an event posted by the function sl_wfx_host_post_event() */
    if (xQueueReceive(sl_wfx_confirmation_queue, &posted_event_id, 1) == pdTRUE) {
      /* Once a message is received, check if it is the expected ID */
      if (confirmation_id == posted_event_id) {
        /* Pass the confirmation reply and return*/
        if (event_payload_out != NULL) {
          *event_payload_out = sl_wfx_context->event_payload_buffer;
        }
        return SL_STATUS_OK;
      }
    }
  }
  
  /* The wait for the confirmation timed out, return */
  return SL_STATUS_TIMEOUT;
}

/**************************************************************************//**
 * Wait function
 *****************************************************************************/
sl_status_t sl_wfx_host_wait (uint32_t wait_time) {
  osDelay(wait_time);
  
  return SL_STATUS_OK;
}

/**************************************************************************//**
 * Post an event comming from the Wi-Fi device
 *****************************************************************************/
sl_status_t sl_wfx_host_post_event (sl_wfx_generic_message_t *event_payload) {
  
  switch (event_payload->header.id) {
    /******** INDICATION ********/
  case SL_WFX_CONNECT_IND_ID:
    {
      sl_wfx_connect_callback((sl_wfx_connect_ind_t*) event_payload);
      break;
    }
  case SL_WFX_DISCONNECT_IND_ID:
    {
      sl_wfx_disconnect_callback((sl_wfx_disconnect_ind_t*) event_payload);
      break;
    }
  case SL_WFX_START_AP_IND_ID:
    {
      sl_wfx_start_ap_callback((sl_wfx_start_ap_ind_t*) event_payload);
      break;
    }
  case SL_WFX_STOP_AP_IND_ID:
    {
      sl_wfx_stop_ap_callback((sl_wfx_stop_ap_ind_t*) event_payload);
      break;
    }
  case SL_WFX_RECEIVED_IND_ID:
    {
      sl_wfx_received_ind_t* ethernet_frame = (sl_wfx_received_ind_t*) event_payload;
      if ( ethernet_frame->body.frame_type == 0 )
      {
        sl_wfx_host_received_frame_callback(ethernet_frame);
      }
      break;
    }
  case SL_WFX_SCAN_RESULT_IND_ID:
    {
      sl_wfx_scan_result_callback((sl_wfx_scan_result_ind_t*) event_payload);
      break;
    }
  case SL_WFX_SCAN_COMPLETE_IND_ID:
    {
      sl_wfx_scan_complete_callback((sl_wfx_scan_complete_ind_t*) event_payload);
      break;
    }
  case SL_WFX_AP_CLIENT_CONNECTED_IND_ID:
    {
      sl_wfx_ap_client_connected_callback((sl_wfx_ap_client_connected_ind_t*) event_payload);
      break;
    }
  case SL_WFX_AP_CLIENT_REJECTED_IND_ID:
    {
      sl_wfx_ap_client_rejected_callback((sl_wfx_ap_client_rejected_ind_t*) event_payload);
      break;
    }
  case SL_WFX_AP_CLIENT_DISCONNECTED_IND_ID:
    {
      sl_wfx_ap_client_disconnected_callback((sl_wfx_ap_client_disconnected_ind_t*) event_payload);
      break;
    }
  case SL_WFX_EXT_AUTH_IND_ID:
    {
      sl_wfx_ext_auth_callback((sl_wfx_ext_auth_ind_t*) event_payload);
      break;
    }
  case SL_WFX_GENERIC_IND_ID:
    {
      sl_wfx_generic_status_callback((sl_wfx_generic_ind_t *) event_payload);
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

  if (host_context.waited_event_id == event_payload->header.id) {
    if (event_payload->header.length < SL_WFX_EVENT_MAX_SIZE) {
      /* Post the event in the queue */
      memcpy(sl_wfx_context->event_payload_buffer,
             (void*) event_payload,
             event_payload->header.length);
      xQueueOverwrite(sl_wfx_confirmation_queue, (void *) &event_payload->header.id);
    }
  }

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * Allocate a buffer
 *****************************************************************************/
sl_status_t sl_wfx_host_allocate_buffer (void** buffer,
                                         sl_wfx_buffer_type_t type,
                                         uint32_t buffer_size) {
  *buffer = pvPortMalloc( buffer_size);
  
  return SL_STATUS_OK;
}

/**************************************************************************//**
 * Free a buffer
 *****************************************************************************/
sl_status_t sl_wfx_host_free_buffer (void* buffer, sl_wfx_buffer_type_t type) {
  vPortFree(buffer);
  
  return SL_STATUS_OK;
}

/**************************************************************************//**
 * Host transmit frame
 *****************************************************************************/
sl_status_t sl_wfx_host_transmit_frame (void* frame, uint32_t frame_len) {
  return sl_wfx_data_write(frame, frame_len);
}

/**************************************************************************//**
 * Output driver logs
 *****************************************************************************/
#if SL_WFX_DEBUG_MASK
void sl_wfx_host_log (const char *string, ...) {
  va_list valist;

  va_start(valist, string);
  vprintf(string, valist);
  va_end(valist);
}
#endif

/**************************************************************************//**
 * Lock the driver mutex
 *****************************************************************************/
sl_status_t sl_wfx_host_lock (void) {
  sl_status_t status;

  if (xSemaphoreTake(sl_wfx_driver_mutex, 500) == pdTRUE) {
    status = SL_STATUS_OK;
  } else {
    printf("Wi-Fi driver mutex timeout\r\n");
    status = SL_STATUS_TIMEOUT;
  }

  return status;
}

/**************************************************************************//**
 * Unlock the driver mutex
 *****************************************************************************/
sl_status_t sl_wfx_host_unlock (void) {
  xSemaphoreGive(sl_wfx_driver_mutex);

  return SL_STATUS_OK;
}


/**************************************************************************//**
 * Callback for individual scan result
 *****************************************************************************/
void sl_wfx_scan_result_callback (sl_wfx_scan_result_ind_t *scan_result) {
  scan_count++;

  if (scan_verbose) {
    /*Report one AP information*/
    printf(
        "# %2d %2d %02X %03d %02X:%02X:%02X:%02X:%02X:%02X  %s\r\n",
        scan_count,
        scan_result->body.channel,
        *(uint8_t *)&scan_result->body.security_mode,
        ((int16_t)(scan_result->body.rcpi - 220) / 2),
        scan_result->body.mac[0], scan_result->body.mac[1],
        scan_result->body.mac[2], scan_result->body.mac[3],
        scan_result->body.mac[4], scan_result->body.mac[5],
        scan_result->body.ssid_def.ssid);
  }

  if (scan_count <= SL_WFX_MAX_SCAN_RESULTS) {
    scan_list[scan_count - 1].ssid_def = scan_result->body.ssid_def;
    scan_list[scan_count - 1].channel = scan_result->body.channel;
    scan_list[scan_count - 1].security_mode = scan_result->body.security_mode;
    scan_list[scan_count - 1].rcpi = scan_result->body.rcpi;
    memcpy(scan_list[scan_count - 1].mac, scan_result->body.mac, 6);
  }
}

/**************************************************************************//**
 * Callback for scan complete
 *****************************************************************************/
void sl_wfx_scan_complete_callback (sl_wfx_scan_complete_ind_t *scan_complete) {
  void * buffer;
  sl_status_t status;

  scan_count_web = scan_count;
  scan_count = 0;

  status = sl_wfx_host_allocate_buffer(&buffer,
                                       SL_WFX_RX_FRAME_BUFFER,
                                       scan_complete->header.length);
  if (status == SL_STATUS_OK) {
    memcpy(buffer, (void *)scan_complete, scan_complete->header.length);
    xQueueSend(wifi_event_queue, &buffer, 0);
  }
}

/**************************************************************************//**
 * Callback when station connects
 *****************************************************************************/
void sl_wfx_connect_callback (sl_wfx_connect_ind_t *connect) {
  void *buffer;
  sl_status_t status;

  switch(connect->body.status){
  case WFM_STATUS_SUCCESS:
    {
      printf("Connected\r\n");
      sl_wfx_context->state |= SL_WFX_STA_INTERFACE_CONNECTED;

      status = sl_wfx_host_allocate_buffer(&buffer,
                                           SL_WFX_RX_FRAME_BUFFER,
                                           connect->header.length);
      if (status == SL_STATUS_OK) {
        memcpy(buffer, (void *)connect, connect->header.length);
        xQueueSend(wifi_event_queue, &buffer, 0);
      }
      break;
    }
  case WFM_STATUS_NO_MATCHING_AP:
    {
      strcpy(event_log, "Connection failed, access point not found");
      printf(event_log);
      printf("\r\n");
      break;
    }
  case WFM_STATUS_CONNECTION_ABORTED:
    {
      strcpy(event_log, "Connection aborted");
      printf(event_log);
      printf("\r\n");
      break;
    }
  case WFM_STATUS_CONNECTION_TIMEOUT:
    {
      strcpy(event_log, "Connection timeout");
      printf(event_log);
      printf("\r\n");
      break;
    }
  case WFM_STATUS_CONNECTION_REJECTED_BY_AP:
    {
      strcpy(event_log, "Connection rejected by the access point");
      printf(event_log);
      printf("\r\n");
      break;
    }
  case WFM_STATUS_CONNECTION_AUTH_FAILURE:
    {
      strcpy(event_log, "Connection authentication failure");
      printf(event_log);
      printf("\r\n");
      break;
    }
  default:
    {
      strcpy(event_log, "Connection attempt error");
      printf(event_log);
      printf("\r\n");
    }
  }
}

/**************************************************************************//**
 * Callback for station disconnect
 *****************************************************************************/
void sl_wfx_disconnect_callback (sl_wfx_disconnect_ind_t *disconnect) {
  void *buffer;
  sl_status_t status;

  printf("Disconnected %d\r\n", disconnect->body.reason);
  sl_wfx_context->state &= ~SL_WFX_STA_INTERFACE_CONNECTED;

  status = sl_wfx_host_allocate_buffer(&buffer,
                                       SL_WFX_RX_FRAME_BUFFER,
                                       disconnect->header.length);
  if (status == SL_STATUS_OK) {
    memcpy(buffer, (void *)disconnect, disconnect->header.length);
    xQueueSend(wifi_event_queue, &buffer, 0);
  }
}

/**************************************************************************//**
 * Callback for AP started
 *****************************************************************************/
void sl_wfx_start_ap_callback (sl_wfx_start_ap_ind_t *start_ap) {
  void *buffer;
  sl_status_t status;

  if (start_ap->body.status == 0) {
    printf("AP started\r\n");
    printf("Join the AP with SSID: %s\r\n", softap_ssid);
    sl_wfx_context->state |= SL_WFX_AP_INTERFACE_UP;

    status = sl_wfx_host_allocate_buffer(&buffer,
                                         SL_WFX_RX_FRAME_BUFFER,
                                         start_ap->header.length);
    if (status == SL_STATUS_OK) {
      memcpy(buffer, (void *)start_ap, start_ap->header.length);
      xQueueSend(wifi_event_queue, &buffer, 0);
    }
  } else {
    printf("AP start failed\r\n");
    strcpy(event_log, "AP start failed");
  }
}

/**************************************************************************//**
 * Callback for AP stopped
 *****************************************************************************/
void sl_wfx_stop_ap_callback(sl_wfx_stop_ap_ind_t *stop_ap) {
  void *buffer;
  sl_status_t status;

  printf("SoftAP stopped\r\n");
  dhcpserver_clear_stored_mac();
  sl_wfx_context->state &= ~SL_WFX_AP_INTERFACE_UP;

  status = sl_wfx_host_allocate_buffer(&buffer,
                                       SL_WFX_RX_FRAME_BUFFER,
                                       stop_ap->length);
  if (status == SL_STATUS_OK) {
    memcpy(buffer, (void *)stop_ap, stop_ap->length);
    xQueueSend(wifi_event_queue, &buffer, 0);
  }
}

/**************************************************************************//**
 * Callback for client connect to AP
 *****************************************************************************/
void sl_wfx_ap_client_connected_callback (sl_wfx_ap_client_connected_ind_t *ap_client_connected) {
  printf("Client connected, MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
         ap_client_connected->body.mac[0],
         ap_client_connected->body.mac[1],
         ap_client_connected->body.mac[2],
         ap_client_connected->body.mac[3],
         ap_client_connected->body.mac[4],
         ap_client_connected->body.mac[5]);
  printf("Open a web browser and go to http://%d.%d.%d.%d\r\n",
         ap_ip_addr0, ap_ip_addr1, ap_ip_addr2, ap_ip_addr3);
}

/**************************************************************************//**
 * Callback for client rejected from AP
 *****************************************************************************/
void sl_wfx_ap_client_rejected_callback (sl_wfx_ap_client_rejected_ind_t *ap_client_rejected) {
  struct eth_addr mac_addr;
  
  memcpy(&mac_addr, ap_client_rejected->body.mac, SL_WFX_BSSID_SIZE);
  dhcpserver_remove_mac(&mac_addr);
  printf("Client rejected, reason: %d, MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
         ap_client_rejected->body.reason,
         ap_client_rejected->body.mac[0],
         ap_client_rejected->body.mac[1],
         ap_client_rejected->body.mac[2],
         ap_client_rejected->body.mac[3],
         ap_client_rejected->body.mac[4],
         ap_client_rejected->body.mac[5]);
}

/**************************************************************************//**
 * Callback for AP client disconnect
 *****************************************************************************/
void sl_wfx_ap_client_disconnected_callback (sl_wfx_ap_client_disconnected_ind_t *ap_client_disconnected) {
  struct eth_addr mac_addr;
  
  memcpy(&mac_addr, ap_client_disconnected->body.mac, SL_WFX_BSSID_SIZE);
  dhcpserver_remove_mac(&mac_addr);
  printf("Client disconnected, reason: %d, MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
         ap_client_disconnected->body.reason,
         ap_client_disconnected->body.mac[0],
         ap_client_disconnected->body.mac[1],
         ap_client_disconnected->body.mac[2],
         ap_client_disconnected->body.mac[3],
         ap_client_disconnected->body.mac[4],
         ap_client_disconnected->body.mac[5]);
}

/**************************************************************************//**
 * Callback for generic status received
 *****************************************************************************/
void sl_wfx_generic_status_callback(sl_wfx_generic_ind_t* frame)
{
  rx_stats = frame->body.indication_data.rx_stats;
}

/**************************************************************************//**
 * Callback for External Authentication
 *****************************************************************************/
void sl_wfx_ext_auth_callback (sl_wfx_ext_auth_ind_t *ext_auth_indication) {
  void *buffer;
  sl_status_t status;

  status = sl_wfx_host_allocate_buffer(&buffer,
                                       SL_WFX_RX_FRAME_BUFFER,
                                       ext_auth_indication->header.length);
  if (status == SL_STATUS_OK) {
    memcpy(buffer,
           (void *)ext_auth_indication,
           ext_auth_indication->header.length);
    xQueueSend(wifi_event_queue, &buffer, 0);
  }
}
