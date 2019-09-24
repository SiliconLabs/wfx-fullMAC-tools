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

/**************************************************************************//**
 * Host specific implementation: STM32F4 + Bare Metal
 *****************************************************************************/

#include "lwip.h"
#include "stm32f4xx_hal.h"
#include "sl_wfx.h"
#include "sl_wfx_host.h"
#include "sl_wfx_host_pin.h"

#include <stdlib.h>
#include <string.h>

/* WF200 Firmware include */
#include "wfm_wf200_C0.h"

/* WF200 Platform Data Set (PDS) include */
#if defined(SL_WFX_USE_RF1_OUTPUT)
#include "PDS/BRD8022A_Rev_A06.h"
#elif defined(SL_WFX_USE_RF2_OUTPUT)
#include "PDS/BRD8022A_Rev_A06_RF2.h"
#else
#warning No RF output selected, defaulting to RF1
#include "PDS/BRD8022A_Rev_A06.h"
#endif

sl_wfx_mac_address_t connected_stations_tab[SL_WFX_MAX_STATIONS];
sl_wfx_scan_result_ind_body_t scan_list[SL_WFX_MAX_SCAN_RESULTS];
uint8_t connected_stations = 0; 
uint8_t scan_count_web = 0; 
uint8_t scan_count = 0; 
uint8_t wf200_interrupt_event = 0;
uint8_t button_push_event = 0;
uint8_t scan_ongoing = 0;

extern void lwip_set_sta_link_up();
extern void lwip_set_sta_link_down();
extern void lwip_set_ap_link_up();
extern void lwip_set_ap_link_down();

struct
{
  uint32_t sl_wfx_firmware_download_progress;
  uint8_t waited_event_id;
  uint8_t posted_event_id;
}host_context;

/* Initialization phase*/
sl_status_t sl_wfx_host_init(void)
{
  host_context.sl_wfx_firmware_download_progress = 0;
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_get_firmware_data(const uint8_t** data, uint32_t data_size)
{
  *data = &sl_wfx_firmware[host_context.sl_wfx_firmware_download_progress];
  host_context.sl_wfx_firmware_download_progress += data_size;
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_get_firmware_size(uint32_t* firmware_size)
{
  *firmware_size = sizeof(sl_wfx_firmware);
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_get_pds_data(const char **pds_data, uint16_t index)
{
  *pds_data = wf200_pds[index];
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_get_pds_size(uint16_t *pds_size)
{
  *pds_size = SL_WFX_ARRAY_COUNT(wf200_pds);
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_deinit(void)
{
  return SL_SUCCESS;
}

/* GPIO interface */
sl_status_t sl_wfx_host_reset_chip(void)
{
  // hold pin high to get chip out of reset
  HAL_GPIO_WritePin(SL_WFX_RESET_PORT, SL_WFX_RESET_GPIO, GPIO_PIN_RESET);
  HAL_Delay(10);
  HAL_GPIO_WritePin(SL_WFX_RESET_PORT, SL_WFX_RESET_GPIO, GPIO_PIN_SET);
  HAL_Delay(10);
  
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_hold_in_reset(void)
{
  HAL_GPIO_WritePin(SL_WFX_RESET_PORT, SL_WFX_RESET_GPIO, GPIO_PIN_RESET);
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_set_wake_up_pin(uint8_t state)
{
  if ( state > 0 )
  {
    HAL_GPIO_WritePin(SL_WFX_WUP_PORT, SL_WFX_WUP_GPIO, GPIO_PIN_SET);
  }
  else
  {
    HAL_GPIO_WritePin(SL_WFX_WUP_PORT, SL_WFX_WUP_GPIO, GPIO_PIN_RESET);
  }
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_wait_for_wake_up(void)
{
  HAL_Delay(2);
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_sleep_grant(sl_wfx_host_bus_tranfer_type_t type,
                                    sl_wfx_register_address_t address,
                                    uint32_t length)
{
  return SL_WIFI_SLEEP_GRANTED;
}

sl_status_t sl_wfx_host_setup_waited_event(uint8_t event_id)
{
  host_context.waited_event_id = event_id;
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_wait_for_confirmation(uint8_t confirmation_id,
                                              uint32_t timeout_ms,
                                              void **event_payload_out)
{
  uint16_t control_register = 0;
  
  for(uint32_t i = 0; i < timeout_ms; i++)
  {     
    do{
      sl_wfx_receive_frame(&control_register);
    }while ( (control_register & SL_WFX_CONT_NEXT_LEN_MASK) != 0 );   
    if (confirmation_id == host_context.posted_event_id)
    {
      host_context.posted_event_id = 0;
      if (event_payload_out != NULL)
      {
        *event_payload_out = sl_wfx_context->event_payload_buffer;
      }
      return SL_SUCCESS;
    }else{
      sl_wfx_host_wait(1);
    }
  }
  return SL_TIMEOUT;
}

sl_status_t sl_wfx_host_wait(uint32_t wait_time)
{
  HAL_Delay(wait_time);
  return SL_SUCCESS;
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
  case SL_WFX_GENERIC_IND_ID:
    {
      break;
    }
  case SL_WFX_EXCEPTION_IND_ID:
    {
      printf("Firmware Exception\r\n");
      sl_wfx_exception_ind_t *firmware_exception = (sl_wfx_exception_ind_t*)event_payload;
      printf ("Exception data = ");
      for(int i = 0; i < SL_WFX_EXCEPTION_DATA_SIZE; i++)
      {
        printf ("%X ", firmware_exception->body.data[i]);
      }
      printf("\r\n");
      break;
    }
  case SL_WFX_ERROR_IND_ID:
    {
      printf("Firmware Error\r\n");
      sl_wfx_error_ind_t *firmware_error = (sl_wfx_error_ind_t*)event_payload;
      printf ("Error type = %lu\r\n",firmware_error->body.type);
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
      host_context.posted_event_id = event_payload->header.id;
    }
    
  }
  return SL_SUCCESS;
}

/* Memory management */
sl_status_t sl_wfx_host_allocate_buffer(void** buffer, sl_wfx_buffer_type_t type, uint32_t buffer_size)
{
  SL_WFX_UNUSED_PARAMETER(type);
  *buffer = malloc(buffer_size);
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_free_buffer(void* buffer, sl_wfx_buffer_type_t type)
{
  SL_WFX_UNUSED_PARAMETER(type);
  free(buffer);
  return SL_SUCCESS;
}

/* Frame hook */
sl_status_t sl_wfx_host_transmit_frame(void* frame, uint32_t frame_len)
{
#ifdef SL_WFX_DEBUG
  printf("TX> ");
  printf("%02X%02X %02X%02X ", ((char*)frame)[0], ((char*)frame)[1], ((char*)frame)[2], ((char*)frame)[3]);
  for(uint32_t i = 4; i < frame_len; i++)
  {
    printf("%02X ", ((char*)frame)[i]);
  }
  printf("\r\n");
#endif
  return sl_wfx_data_write( frame, frame_len );
}

void sl_wfx_process(void)
{
  uint16_t control_register = 0;
  if(wf200_interrupt_event == 1)
  {
    wf200_interrupt_event = 0;
    do
    {
      sl_wfx_receive_frame(&control_register);
    }while ( (control_register & SL_WFX_CONT_NEXT_LEN_MASK) != 0 ); 
  }
  if(button_push_event == 1)
  {
    if(scan_ongoing == 0){
      printf("!  # Ch RSSI MAC (BSSID)        Network (SSID)\r\n");
      sl_wfx_send_scan_command(WFM_SCAN_MODE_ACTIVE, NULL, 0, NULL, 0, NULL, 0);
      scan_ongoing = 1;
    }
    button_push_event = 0;
  }
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
    memcpy(&scan_list[scan_count - 1], scan_result, sizeof(sl_wfx_scan_result_ind_body_t));
  }
}

/** Callback to indicate the scan completion
 */
void sl_wfx_scan_complete_callback(uint32_t status)
{
  scan_ongoing = 0;
  scan_count_web = scan_count;
  scan_count = 0;
}

/** Callback triggered when a connection is established as station
 */
void sl_wfx_connect_callback(uint8_t* mac, uint32_t status)
{
  if(status == 0)
  {  
    sl_wfx_context->state |= SL_WFX_STA_INTERFACE_CONNECTED;
    lwip_set_sta_link_up();
    printf("Connected\r\n");
  }else{
    printf("Connection attempt failed\r\n");
  }
}

/** Callback triggered when connection is disconnected
 */
void sl_wfx_disconnect_callback(uint8_t* mac, uint16_t reason)
{
  lwip_set_sta_link_down();
  sl_wfx_context->state &= ~SL_WFX_STA_INTERFACE_CONNECTED;
  printf("Disconnected\r\n");
}

/** Callback triggered when a softap is started
 */
void sl_wfx_start_ap_callback(uint32_t status)
{
  if(status == 0)
  {
    lwip_set_ap_link_up();
    sl_wfx_context->state |= SL_WFX_AP_INTERFACE_UP;
    printf("AP started\r\n");
  }else{
    printf("AP start failed\r\n");
  }
}

/** Callback triggered when a softap is stopped
 */
void sl_wfx_stop_ap_callback(void)
{
  sl_wfx_context->state &= ~SL_WFX_AP_INTERFACE_UP;
  lwip_set_ap_link_down();
  printf("AP stopped\r\n");
}

/** Callback triggered when a client connects
 */
void sl_wfx_client_connected_callback(uint8_t* mac)
{
  strncpy((char *) &connected_stations_tab[connected_stations], (char *) mac, 6);
  connected_stations++;
  printf("Client connected, MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

/** Callback triggered when a client disconnects
 */
void sl_wfx_ap_client_rejected_callback(uint32_t status, uint8_t* mac)
{
  for(uint8_t i = 0; i < connected_stations; i++)
  {
    if(strncmp((char *) &connected_stations_tab[i], (char *) mac, 6) == 0)
    {
      strncpy((char *) &connected_stations_tab[i],
              (char *) &connected_stations_tab[i + 1],
              (SL_WFX_MAX_STATIONS - 1 - i) * 6);
      memset(&connected_stations_tab[SL_WFX_MAX_STATIONS - 1], 0, 6);
      connected_stations--;
    }
  }
  printf("Client rejected, reason: %d, MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
         status, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

/** Callback triggered when a client disconnects
 */
void sl_wfx_ap_client_disconnected_callback(uint32_t status, uint8_t* mac)
{
  for(uint8_t i = 0; i < connected_stations; i++)
  {
    if(strncmp((char *) &connected_stations_tab[i], (char *) mac, 6) == 0)
    {
      strncpy((char *) &connected_stations_tab[i],
              (char *) &connected_stations_tab[i + 1],
              (SL_WFX_MAX_STATIONS - 1 - i) * 6);
      memset(&connected_stations_tab[SL_WFX_MAX_STATIONS - 1], 0, 6);
      connected_stations--;
    }
  }
  printf("Client disconnected, reason: %d, MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
         status, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}
