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
#include "sl_wfx.h"

/* WFX host callbacks */
void sl_wfx_connect_callback( uint8_t* mac, uint32_t status );
void sl_wfx_disconnect_callback( uint8_t* mac, uint16_t reason );
void sl_wfx_start_ap_callback( uint32_t status );
void sl_wfx_stop_ap_callback( void );
void sl_wfx_host_received_frame_callback( sl_wfx_received_ind_t* rx_buffer );
void sl_wfx_scan_result_callback( sl_wfx_scan_result_ind_body_t* scan_result );
void sl_wfx_scan_complete_callback( uint32_t status );
void sl_wfx_generic_status_callback( sl_wfx_generic_ind_t* frame );
void sl_wfx_client_connected_callback( uint8_t* mac );
void sl_wfx_ap_client_disconnected_callback( uint32_t status, uint8_t* mac );

sl_status_t sl_wfx_host_setup_waited_event( uint32_t event_id );