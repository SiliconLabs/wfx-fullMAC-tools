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
 * Host specific implementation: STM32F4 + FreeRTOS
 *****************************************************************************/

#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "sl_wfx.h"
#include "sl_wfx_host.h"
#include "sl_wfx_host_pin.h"
#include "BRD802xx_pds.h"
#include <stdlib.h>
#include <string.h>

#if   defined( SL_WFX_ALPHA_KEY )
#include "wfm_wf200_A0.h"
#elif defined( SL_WFX_PROD_KEY )
#include "wfm_wf200_C0.h"
#else
#error Must define either SL_WFX_ALPHA_KEY/SL_WFX_PROD_KEY
#endif

void lwip_set_link_up (void);
void lwip_set_link_down (void);

sl_wfx_interface_status_t sl_wfx_status;
QueueHandle_t eventQueue;
uint8_t scan_count = 0; 

struct
{
    uint32_t sl_wfx_firmware_download_progress;
}host_context;

/* Initialization phase*/
sl_status_t sl_wfx_host_init(void)
{
  host_context.sl_wfx_firmware_download_progress = 0;
  eventQueue = xQueueCreate(1, sizeof( uint32_t ));
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
  osDelay(10);
  HAL_GPIO_WritePin(SL_WFX_RESET_PORT, SL_WFX_RESET_GPIO, GPIO_PIN_SET);
  osDelay(5);
  
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
  osDelay(2);
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_wait_for_confirmation(uint32_t timeout, void** event_payload_out)
{
  uint32_t posted_event;
  for(uint32_t i = 0; i < timeout; i++)
  {
    if(xQueueReceive(eventQueue, &(posted_event), 1))
    { 
            
      if (sl_wfx_context->waited_event_id == posted_event)
      {
#ifdef DEBUG
        printf("event %#08X \n\r", posted_event); 
#endif 
        if (event_payload_out != NULL)
        {
          *event_payload_out = sl_wfx_context->event_payload_buffer;
        }
        return SL_SUCCESS;
      }
    }
  }
  return SL_TIMEOUT;
}

sl_status_t sl_wfx_host_wait(uint32_t wait_time)
{
  osDelay(wait_time);
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_post_event(sl_wfx_frame_type_t frame_type, sl_wfx_generic_message_t *network_rx_buffer)
{

  switch(network_rx_buffer->header.id){
  /******** INDICATION ********/
  case SL_WFX_CONNECT_IND_ID:
    {
      sl_wfx_connect_ind_t* connect_indication = (sl_wfx_connect_ind_t*) network_rx_buffer;
      sl_wfx_connect_callback(connect_indication->body.mac, connect_indication->body.status);
      break;
    }
  case SL_WFX_DISCONNECT_IND_ID:
    {
      sl_wfx_disconnect_ind_t* disconnect_indication = (sl_wfx_disconnect_ind_t*) network_rx_buffer;
      sl_wfx_disconnect_callback(disconnect_indication->body.mac, disconnect_indication->body.reason);
      break;
    }
  case SL_WFX_START_AP_IND_ID:
    {
      sl_wfx_start_ap_ind_t* start_ap_indication = (sl_wfx_start_ap_ind_t*) network_rx_buffer;
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
      sl_wfx_received_ind_t* ethernet_frame = (sl_wfx_received_ind_t*) network_rx_buffer;
      if (ethernet_frame->body.frame_type == 0)
      {
        sl_wfx_host_received_frame_callback( ethernet_frame );
      }
      break;
    }
  case SL_WFX_SCAN_RESULT_IND_ID:
    {
      sl_wfx_scan_result_ind_t* scan_result = (sl_wfx_scan_result_ind_t*) network_rx_buffer;
      sl_wfx_scan_result_callback(&scan_result->body);
      break;
    }
  case SL_WFX_SCAN_COMPLETE_IND_ID:
    {
      sl_wfx_scan_complete_ind_t* scan_complete = (sl_wfx_scan_complete_ind_t*) network_rx_buffer;
      sl_wfx_scan_complete_callback(scan_complete->body.status);
      break;
    }
  case SL_WFX_AP_CLIENT_CONNECTED_IND_ID:
    {
      sl_wfx_ap_client_connected_ind_t* client_connected_indication = (sl_wfx_ap_client_connected_ind_t*) network_rx_buffer;
      sl_wfx_client_connected_callback(client_connected_indication->body.mac);
      break;
    }
  case SL_WFX_AP_CLIENT_REJECTED_IND_ID:
    {
      break;
    }
  case SL_WFX_AP_CLIENT_DISCONNECTED_IND_ID:
    {
      sl_wfx_ap_client_disconnected_ind_t* ap_client_disconnected_indication = (sl_wfx_ap_client_disconnected_ind_t*) network_rx_buffer;
      sl_wfx_ap_client_disconnected_callback(ap_client_disconnected_indication->body.reason, ap_client_disconnected_indication->body.mac);
      break;
    }
  case SL_WFX_EXCEPTION_IND_ID:
    {
      printf("Firmware Exception\r\n");
      break;
    }
  case SL_WFX_ERROR_IND_ID:
    {
      printf("Firmware Error\r\n");
      break;
    }
  /******** CONFIRMATION ********/
  case SL_WFX_SEND_FRAME_CNF_ID:
    {
      if (sl_wfx_context->used_buffer_number > 0)
      {
        sl_wfx_context->used_buffer_number--;
      }
      break;
    }
  default:
	{
	  //Confirmation are sent to the event queue
	}
  }    
  
  if (sl_wfx_context->waited_event_id == network_rx_buffer->header.id)
  {
    if(network_rx_buffer->header.length < 512){
      /* Post the event in the queue */
      memcpy(sl_wfx_context->event_payload_buffer, (void*) network_rx_buffer,network_rx_buffer->header.length);
      sl_wfx_context->posted_event_id = network_rx_buffer->header.id;
      xQueueOverwrite(eventQueue, ( void * ) &sl_wfx_context->posted_event_id);
    }
  }
  sl_wfx_host_free_buffer(network_rx_buffer, SL_WFX_RX_FRAME_BUFFER);
  return SL_SUCCESS;
}

/* Memory management */
sl_status_t sl_wfx_host_allocate_buffer(void** buffer, sl_wfx_buffer_type_t type, uint32_t buffer_size, uint32_t wait_duration)
{
  SL_WFX_UNUSED_PARAMETER(type);
  SL_WFX_UNUSED_PARAMETER(wait_duration);
  *buffer = pvPortMalloc(buffer_size);
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_free_buffer(void* buffer, sl_wfx_buffer_type_t type)
{
  SL_WFX_UNUSED_PARAMETER(type);
  vPortFree(buffer);
  return SL_SUCCESS;
}

/* Frame hook */
sl_status_t sl_wfx_host_transmit_frame(void* frame, uint32_t frame_len)
{
  return sl_wfx_data_write(frame, frame_len);
}

sl_status_t sl_wfx_host_setup_waited_event(uint32_t event_id)
{
  sl_wfx_context->waited_event_id = event_id;
  sl_wfx_context->posted_event_id = 0;
  return SL_SUCCESS;
}

/** Callback for individual AP discovered
 */
void sl_wfx_scan_result_callback(sl_wfx_scan_result_ind_body_t* scan_result)
{
  scan_count++;
  printf(
          "# %2d %2d  %03d %02X:%02X:%02X:%02X:%02X:%02X  %s",
          scan_count,
          scan_result->channel,
          ((int16_t)(scan_result->rcpi - 220)/2),
          scan_result->mac[0], scan_result->mac[1],
          scan_result->mac[2], scan_result->mac[3],
          scan_result->mac[4], scan_result->mac[5],
          scan_result->ssid_def.ssid);
  /*Report one AP information*/
  printf("\r\n");
}

/** Callback to indicate the scan completion
 */
void sl_wfx_scan_complete_callback(uint32_t status)
{
  scan_count = 0;
}

/** Callback triggered when a connection is established as station
 */
void sl_wfx_connect_callback(uint8_t* mac, uint32_t status)
{
  if(status == 0)
  {  
    lwip_set_link_up();
    sl_wfx_status |= SL_WFX_STA_INTERFACE_CONNECTED;
    printf("Connected\r\n");
  }else{
    printf("Connection attempt failed\r\n");
  }
}

/** Callback triggered when connection is disconnected
 */
void sl_wfx_disconnect_callback(uint8_t* mac, uint16_t reason)
{
  lwip_set_link_down();
  sl_wfx_status &= ~SL_WFX_STA_INTERFACE_CONNECTED;
  printf("Disconnected\r\n");
}

/** Callback triggered when a softap is started
 */
void sl_wfx_start_ap_callback(uint32_t status)
{
  if(status == 0)
  {  
    sl_wfx_status |= SL_WFX_AP_INTERFACE_UP;
    printf("AP started\r\n");
  }else{
    printf("AP start failed\r\n");
    lwip_set_link_down();
  }
}

/** Callback triggered when a softap is stopped
 */
void sl_wfx_stop_ap_callback(void)
{
  lwip_set_link_down();
  sl_wfx_status &= ~SL_WFX_AP_INTERFACE_UP;
  printf("AP stopped\r\n");
}

/** Callback triggered when a client connects
 */
void sl_wfx_client_connected_callback(uint8_t* mac)
{
  printf("Client connected\r\n");  
}

/** Callback triggered when a client disconnects
 */
void sl_wfx_ap_client_disconnected_callback(uint32_t status, uint8_t* mac)
{
  printf("Client disconnected\r\n");
}
