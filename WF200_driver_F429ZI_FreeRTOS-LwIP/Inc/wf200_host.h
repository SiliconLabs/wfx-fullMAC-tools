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

#pragma once
#include <stdint.h>
#include "sl_status.h"

#define WF200_EVENT_RECEIVED  (1 << 0)

/* WF2OO host callbacks */
void wf200_connect_callback( uint8_t* mac, uint32_t status );
void wf200_disconnect_callback( uint8_t* mac, uint16_t reason );
void wf200_start_ap_callback( uint32_t status );
void wf200_stop_ap_callback( void );
void wf200_host_received_frame_callback( wf200_ethernet_frame_t* rx_buffer );
void wf200_scan_result_callback( wf200_scan_result_t* scan_result );
void wf200_scan_complete_callback( uint32_t status );
void wf200_generic_status_callback( HiGenericIndBody_t* frame );
void wf200_client_connected_callback( uint8_t* mac );
void wf200_ap_client_disconnected_callback( uint32_t status, uint8_t* mac );

struct wf200_host_context_s
{
    uint32_t wf200_firmware_download_progress;
};

struct sl_event
{
    uint32_t flags;
};

typedef struct sl_event sl_event_t;


sl_status_t wf200_host_setup_waited_event( uint32_t event_id );