/*
* Copyright 2018, Silicon Laboratories Inc.  All rights reserved.
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
*/

/*
 *  FreeRTOS WF200 task
 */

/* FreeRTOS includes. */
#include "SEGGER_SYSVIEW_FreeRTOS.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "cmsis_os.h"
#include "wf200.h"
#include "wf200_host.h"
#include "lwip_freertos.h"
#include <stdio.h>

osThreadId busCommTaskHandle;
extern osSemaphoreId s_xDriverSemaphore;
extern QueueHandle_t rxFrameQueue;
extern wf200_context_t* wf200_context;
uint8_t scan_count = 0; 
wf200_context_t wifi;

/* Default parameters */
char wlan_ssid[32]                 = WLAN_SSID;
char wlan_passkey[64]              = WLAN_PASSKEY;
wfm_security_mode wlan_security    = WLAN_SECURITY;
char softap_ssid[32]               = SOFTAP_SSID;
char softap_passkey[64]            = SOFTAP_PASSKEY;
wfm_security_mode softap_security  = SOFTAP_SECURITY;
uint8_t softap_channel             = SOFTAP_CHANNEL;
/* Default parameters */

/*
 * The task that implements the command console processing.
 */
static void prvBusCommTask(void const * pvParameters );

void vBusCommStart( void )
{
  osThreadDef(busCommTask, prvBusCommTask, osPriorityRealtime, 0, 128);
  busCommTaskHandle = osThreadCreate(osThread(busCommTask), NULL);
}
wf200_buffer_t *network_rx_buffer_gbl;

static void receive_frames ()
{
  //bool release_rx_buffer_at_end = true;
  network_rx_buffer_gbl = NULL;
  while ((wf200_receive_frame() == SL_WIFI_FRAME_RECEIVED) && (network_rx_buffer_gbl != NULL))
  {
    
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

          // the network stack will release the buffer when done with it
          //release_rx_buffer_at_end = false;
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
        //TODO
        break;
      }
    case WFM_HI_AP_CLIENT_DISCONNECTED_IND_ID:
      {
        WfmHiApClientDisconnectedInd_t* ap_client_disconnected_indication = (WfmHiApClientDisconnectedInd_t*)network_rx_buffer_gbl;
        wf200_ap_client_disconnected_callback(ap_client_disconnected_indication->Body.Reason, ap_client_disconnected_indication->Body.Mac);
        break;
      }
    case WFM_HI_JOIN_IBSS_IND_ID:
      {
        //TODO
        break;
      }
    case WFM_HI_LEAVE_IBSS_IND_ID:
      {
        //TODO
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
        WfmHiSendFrameCnf_t* reply = (WfmHiSendFrameCnf_t*)network_rx_buffer_gbl;
        if ( reply->Body.Status == WFM_STATUS_SUCCESS )
        {
          wf200_context->used_buffer_number--;
        }
        break;
      }
    }    

    ///if ( release_rx_buffer_at_end == true )
    //{
      wf200_host_free_buffer( network_rx_buffer_gbl, WF200_RX_FRAME_BUFFER );
      network_rx_buffer_gbl = NULL;
    //}
   
  }
}

static void prvBusCommTask(void const * pvParameters )
{
  for( ;; )
  {
    /*Wait for an interrupt from WF200*/
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
    /*Receive the frame(s) pending in WF200*/
    if( s_xDriverSemaphore != NULL )
    {
        /* See if we can obtain the semaphore.  If the semaphore is not
        available wait to see if it becomes free. */
        if( xSemaphoreTake( s_xDriverSemaphore, ( osWaitForever ) ) == pdTRUE )
        {
          receive_frames();
          xSemaphoreGive( s_xDriverSemaphore );
        }
        else
        {
          //unable to receive
        }
    }
    else
    {
       receive_frames();
    }
  }
}

/** Callback for individual AP discovered
 */
void wf200_scan_result_callback( wf200_scan_result_t* scan_result )
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
          scan_result->ssid);
  /*Report one AP information*/
  printf("\r\n");
}

/** Callback to indicate the scan completion
 */
void wf200_scan_complete_callback( uint32_t status )
{
  scan_count = 0;
}

/** Callback triggered when a connection is established as station
 */
void wf200_connect_callback( uint8_t* mac, uint32_t status )
{
  if(status == 0)
  {  
    printf("Connected\r\n");
  }else{
    printf("Connection attempt failed\r\n");
  }
}

/** Callback triggered when signal strength confirmation is received
 */
void wf200_disconnect_callback( uint8_t* mac, uint16_t reason )
{
  printf("Disconnected\r\n");
}

/** Callback triggered when a softap is started
 */
void wf200_start_ap_callback( uint32_t status )
{
  if(status == 0)
  {  
    printf("AP started\r\n");
  }else{
    printf("AP start failed\r\n");
  }
}

/** Callback triggered when a softap is stopped
 */
void wf200_stop_ap_callback( void )
{
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
