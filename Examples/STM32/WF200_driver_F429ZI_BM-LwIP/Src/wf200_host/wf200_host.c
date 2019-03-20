/*
 * EVALUATION AND USE OF THIS SOFTWARE IS SUBJECT TO THE TERMS AND
 * CONDITIONS OF THE CONTROLLING LICENSE AGREEMENT FOUND AT LICENSE.md
 * IN THIS SDK. IF YOU DO NOT AGREE TO THE LICENSE TERMS AND CONDITIONS,
 * PLEASE RETURN ALL SOURCE FILES TO SILICON LABORATORIES.
 * (c) Copyright 2018, Silicon Laboratories Inc.  All rights reserved.
 */

/*
 *  Host specific implementation 
 */

#include "lwip.h"
#include "wf200.h"
#include "stm32f4xx_hal.h"
#include "wf200_host.h"
#include "wf200_registers.h"
#include "wf200_host_pin.h"
#include <stdlib.h>
#include <string.h>

#if   defined( WF200_ALPHA_KEY )
#include "wfm_wf200_A0.h"
#elif defined( WF200_PROD_KEY )
#include "wfm_wf200_C0.h"
#else
#error Must define either WF200_ALPHA_KEY/WF200_PROD_KEY
#endif

extern wf200_status_t wf200_status;
uint8_t scan_count = 0; 
uint8_t wf200_interrupt_event = 0;
uint8_t button_push_event = 0;
uint8_t scan_ongoing = 0;
wf200_buffer_t* network_rx_buffer_gbl;

struct
{
    uint32_t wf200_firmware_download_progress;
} host_context;

sl_status_t wf200_host_init( void )
{
  host_context.wf200_firmware_download_progress = 0;
  return SL_SUCCESS;
}

sl_status_t wf200_host_get_firmware_data( const uint8_t** data, uint32_t data_size )
{
  *data = &wf200_firmware[host_context.wf200_firmware_download_progress];
  host_context.wf200_firmware_download_progress += data_size;
  return SL_SUCCESS;
}

sl_status_t wf200_host_get_firmware_size( uint32_t* firmware_size )
{
  printf("Firmware version : %s\r\n", FIRMWARE_VERSION);
  printf("Firmware size    : %d kB\r\n", wf200_firmware_size);
  *firmware_size = sizeof(wf200_firmware);
  return SL_SUCCESS;
}

sl_status_t wf200_host_deinit( void )
{
  return SL_SUCCESS;
}

/* GPIO interface */
sl_status_t wf200_host_reset_chip( void )
{
  // hold pin high to get chip out of reset
  HAL_GPIO_WritePin(WF200_RESET_PORT, WF200_RESET_GPIO, GPIO_PIN_RESET);
  HAL_Delay( 10 );
  HAL_GPIO_WritePin(WF200_RESET_PORT, WF200_RESET_GPIO, GPIO_PIN_SET);
  HAL_Delay( 30 );
  
  return SL_SUCCESS;
}

sl_status_t wf200_host_hold_in_reset( void )
{
  HAL_GPIO_WritePin(WF200_RESET_PORT, WF200_RESET_GPIO, GPIO_PIN_RESET);
  return SL_SUCCESS;
}

sl_status_t wf200_host_set_wake_up_pin( uint8_t state )
{
  if ( state > 0 )
  {
    HAL_GPIO_WritePin(WF200_WUP_PORT, WF200_WUP_GPIO, GPIO_PIN_SET);
  }
  else
  {
    HAL_GPIO_WritePin(WF200_WUP_PORT, WF200_WUP_GPIO, GPIO_PIN_RESET);
  }
  return SL_SUCCESS;
}

sl_status_t wf200_host_wait_for_wake_up( void )
{
  HAL_Delay(2);
  return SL_SUCCESS;
}

sl_status_t wf200_host_wait_for_confirmation( uint32_t timeout, void** event_payload_out )
{
  uint16_t control_register = 0;
  
  for(uint32_t i = 0; i < timeout; i++)
  {     
    do{
      wf200_receive_frame(&control_register);
    }while ( (control_register & WF200_CONT_NEXT_LEN_MASK) != 0 );
    if ( wf200_context->waited_event_id == wf200_context->posted_event_id )
    {
#ifdef DEBUG
      printf("event %#08X \n\r", wf200_context->posted_event_id); 
#endif 
      wf200_context->posted_event_id = 0;
      if ( event_payload_out != NULL )
      {
        *event_payload_out = wf200_context->event_payload_buffer;
      }
      return SL_SUCCESS;
    }else{
      wf200_host_wait(1);
    }
  }
  return SL_TIMEOUT;
}

sl_status_t wf200_host_post_event(wf200_frame_type_t frame_type, uint32_t event_id, void* event_payload, uint32_t event_payload_length )
{
  network_rx_buffer_gbl = (wf200_buffer_t*)event_payload; 
  
  switch(network_rx_buffer_gbl->msg_id ){
    /******** INDICATION ********/
  case WFM_HI_CONNECT_IND_ID:
    {
      WfmHiConnectInd_t* connect_indication = (WfmHiConnectInd_t*) network_rx_buffer_gbl;
      wf200_connect_callback(connect_indication->Body.Mac, connect_indication->Body.Status);
      break;
    }
  case WFM_HI_DISCONNECT_IND_ID:
    {
      WfmHiDisconnectInd_t* disconnect_indication = (WfmHiDisconnectInd_t*) network_rx_buffer_gbl;
      wf200_disconnect_callback(disconnect_indication->Body.Mac, disconnect_indication->Body.Reason);
      break;
    }
  case WFM_HI_START_AP_IND_ID:
    {
      WfmHiStartApInd_t* start_ap_indication = (WfmHiStartApInd_t*) network_rx_buffer_gbl;
      wf200_start_ap_callback(start_ap_indication->Body.Status);
      break;
    }
  case WFM_HI_STOP_AP_IND_ID:
    {
      wf200_stop_ap_callback();
      break;
    }
  case WFM_HI_RECEIVED_IND_ID:
    {
      wf200_ethernet_frame_t* ethernet_frame = (wf200_ethernet_frame_t*) network_rx_buffer_gbl;
      if ( ethernet_frame->frame_type == 0 )
      {
        wf200_host_received_frame_callback( ethernet_frame );
      }
      break;
    }
  case WFM_HI_SCAN_RESULT_IND_ID:
    {
      WfmHiScanResultInd_t* scan_result = (WfmHiScanResultInd_t*)network_rx_buffer_gbl;
      wf200_scan_result_callback((wf200_scan_result_t*)&scan_result->Body);
      break;
    }
  case WFM_HI_SCAN_COMPLETE_IND_ID:
    {
      WfmHiScanCompleteInd_t* scan_complete = (WfmHiScanCompleteInd_t*)network_rx_buffer_gbl;
      wf200_scan_complete_callback(scan_complete->Body.Status);
      break;
    }
  case WFM_HI_AP_CLIENT_CONNECTED_IND_ID:
    {
      WfmHiApClientConnectedInd_t* client_connected_indication = (WfmHiApClientConnectedInd_t*)network_rx_buffer_gbl;
      wf200_client_connected_callback(client_connected_indication->Body.Mac);
      break;
    }
  case WFM_HI_AP_CLIENT_REJECTED_IND_ID:
    {
      break;
    }
  case WFM_HI_AP_CLIENT_DISCONNECTED_IND_ID:
    {
      WfmHiApClientDisconnectedInd_t* ap_client_disconnected_indication = (WfmHiApClientDisconnectedInd_t*)network_rx_buffer_gbl;
      wf200_ap_client_disconnected_callback(ap_client_disconnected_indication->Body.Reason, ap_client_disconnected_indication->Body.Mac);
      break;
    }
  case HI_GENERIC_IND_ID:
    {
      HiGenericInd_t* generic_status = (HiGenericInd_t*)network_rx_buffer_gbl;
      wf200_generic_status_callback(&generic_status->Body);
      break;
    }
    /******** CONFIRMATION ********/
  case WFM_HI_SEND_FRAME_CNF_ID:
    {
      if ( wf200_context->used_buffer_number > 0 )
      {
        wf200_context->used_buffer_number--;
      }
      break;
    }
  }
  
  if ( wf200_context->waited_event_id == event_id )
  {
    memcpy( wf200_context->event_payload_buffer, event_payload, event_payload_length );
    wf200_context->posted_event_id = event_id;
  }
  wf200_host_free_buffer( network_rx_buffer_gbl, WF200_RX_FRAME_BUFFER );
  network_rx_buffer_gbl = NULL;
  return SL_SUCCESS;
}

sl_status_t wf200_host_wait( uint32_t wait_time )
{
  HAL_Delay(wait_time);
  return SL_SUCCESS;
}

/* Memory management */
sl_status_t wf200_host_allocate_buffer(wf200_buffer_t** buffer, wf200_buffer_type_t type, uint32_t buffer_size, uint32_t wait_duration)
{
  UNUSED_PARAMETER( type );
  UNUSED_PARAMETER( wait_duration );
  *buffer = malloc( buffer_size );
  return SL_SUCCESS;
}

sl_status_t wf200_host_free_buffer( wf200_buffer_t* buffer, wf200_buffer_type_t type )
{
  UNUSED_PARAMETER( type );
  free( buffer );
  return SL_SUCCESS;
}

/* Frame hook */
sl_status_t wf200_host_transmit_frame( wf200_buffer_t* frame )
{
    return wf200_data_write( frame, frame->msg_len );
}

sl_status_t wf200_host_setup_waited_event( uint32_t event_id )
{
  wf200_context->waited_event_id = event_id;
  wf200_context->posted_event_id = 0;
  return SL_SUCCESS;
}

void WF200_Process(void)
{
  uint16_t control_register = 0;
  if(wf200_interrupt_event == 1)
  {
    wf200_interrupt_event = 0;
    do
    {
      wf200_receive_frame(&control_register);
    }while ( (control_register & WF200_CONT_NEXT_LEN_MASK) != 0 ); 
  }
  if(button_push_event == 1)
  {
    if(scan_ongoing == 0){
      printf("!  # Ch RSSI MAC (BSSID)        Network (SSID)\r\n");
      wf200_send_scan_command(WFM_SCAN_MODE_ACTIVE, NULL, 0, NULL, 0, NULL, 0);
      scan_ongoing = 1;
    }
    button_push_event = 0;
  }
}

/*Callback for individual AP discovered*/
void wf200_scan_result_callback( wf200_scan_result_t* scan_result )
{
  scan_count++;
  printf("# %2d %2d  %03d %02X:%02X:%02X:%02X:%02X:%02X  %s\r\n",
         scan_count,
         scan_result->channel,
         ((int16_t)(scan_result->rcpi - 220)/2),
         scan_result->mac[0], scan_result->mac[1],
         scan_result->mac[2], scan_result->mac[3],
         scan_result->mac[4], scan_result->mac[5],
         scan_result->ssid);
}

/*Callback to indicate the scan completion*/
void wf200_scan_complete_callback( uint32_t status )
{
  printf("Scan completed\r\n");
  scan_ongoing = 0;
  scan_count = 0;
}

/*Callback to indicate successful connection to an AP*/
void wf200_connect_callback( uint8_t* mac, uint32_t status )
{
  if(status == 0)
  {  
    wf200_status |= WF200_STA_INTERFACE_CONNECTED;
    printf("Connected\r\n");
    MX_LWIP_Connect();
  }else{
    printf("Connection attempt failed\r\n");
  }
}

void wf200_disconnect_callback( uint8_t* mac, uint16_t reason )
{
  printf("Disconnected\r\n");
  wf200_status &= ~WF200_STA_INTERFACE_CONNECTED;
  MX_LWIP_Disconnect ();
}

void wf200_received_frame_callback( wf200_ethernet_frame_t* rx_buffer )
{
  MX_LWIP_Frame_Received (rx_buffer);
  wf200_host_free_buffer( (wf200_buffer_t*)rx_buffer, WF200_RX_FRAME_BUFFER );
}

/** Callback triggered when a softap is started
 */
void wf200_start_ap_callback( uint32_t status )
{
  if(status == 0)
  {  
    wf200_status |= WF200_AP_INTERFACE_UP;
    printf("AP started\r\n");
  }else{
    printf("AP start failed\r\n");
  }
}

/** Callback triggered when a softap is stopped
 */
void wf200_stop_ap_callback( void )
{
  wf200_status &= ~WF200_AP_INTERFACE_UP;
  printf("AP stopped\r\n");
}

/** Callback triggered when a client connects
 */
void wf200_client_connected_callback( uint8_t* mac )
{
  printf("Client connected\r\n");
}

/** Callback triggered when a client disconnects
 */
void wf200_ap_client_disconnected_callback(  uint32_t status, uint8_t* mac )
{
  printf("Client disconnected\r\n");
}

/** Callback triggered when a generic status is received
 */
void wf200_generic_status_callback( HiGenericIndBody_t* frame )
{
  printf("Generic status received\r\n");
}
