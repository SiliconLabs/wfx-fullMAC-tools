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

#ifndef SL_WFX_HOST_H
#define SL_WFX_HOST_H

#include <stdint.h>
#include "sl_wfx.h"
#include "cmsis_os.h"

#define SL_WFX_EVENT_MAX_SIZE   512
#define SL_WFX_EVENT_LIST_SIZE  1
#define SL_WFX_MAX_STATIONS     8
#define SL_WFX_MAX_SCAN_RESULTS 50

/* Wi-Fi events*/
#define SL_WFX_INTERRUPT	 ( 1 << 0 )
#define SL_WFX_CONNECT	         ( 1 << 1 )
#define SL_WFX_DISCONNECT	 ( 1 << 2 )
#define SL_WFX_START_AP	         ( 1 << 3 )
#define SL_WFX_STOP_AP	         ( 1 << 4 )
#define SL_WFX_SCAN_COMPLETE     ( 1 << 5 )

extern EventGroupHandle_t sl_wfx_event_group;

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
void sl_wfx_ap_client_rejected_callback(  uint32_t status, uint8_t* mac );

typedef struct __attribute__((__packed__)) scan_result_list_s {
  sl_wfx_ssid_def_t ssid_def;
  uint8_t  mac[SL_WFX_MAC_ADDR_SIZE];
  uint16_t channel;
  sl_wfx_security_mode_bitmask_t security_mode;
  uint16_t rcpi;
} scan_result_list_t;

#endif /* SL_WFX_HOST_H */