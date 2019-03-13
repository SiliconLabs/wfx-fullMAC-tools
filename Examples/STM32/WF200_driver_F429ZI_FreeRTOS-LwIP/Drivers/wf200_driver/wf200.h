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

/**
 * \file wf200.h
 * \brief contains the definitions of the FMAC driver functions.
 *
 */

#ifndef __WF200_H
#define __WF200_H

#include "wf200_bus.h"
#include "wf200_pds.h"
#include "wf200_constants.h"
#include "wf200_configuration.h"
#include "wfm_api.h"
#include "wf200_host_api.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

sl_status_t wf200_init( wf200_context_t* context );

sl_status_t wf200_deinit( void );

sl_status_t wf200_enable_irq( void );

sl_status_t wf200_disable_irq( void );

sl_status_t wf200_shutdown( void );

sl_status_t wf200_receive_frame( uint16_t* ctrl_reg  );

sl_status_t wf200_send_configuration( const char* pds_data, uint32_t pds_data_length );

wf200_status_t wf200_get_status( void );

sl_status_t wf200_get_opn(uint8_t** opn);

/*
 * Send Ethernet frame
 */
sl_status_t wf200_send_ethernet_frame( wf200_frame_t* frame, uint32_t data_length, wf200_interface_t interface );

/*
 * Send generic WF200 command
 */
sl_status_t wf200_send_command( uint32_t command_id, void* data, uint32_t data_size, wf200_interface_t interface );


/*
 * Synchronous WF200 commands
 */
sl_status_t wf200_set_access_mode_message( void );

sl_status_t wf200_set_mac_address( const wf200_mac_address_t* mac, wf200_interface_t interface );

sl_status_t wf200_set_power_mode( WfmPmMode mode, uint16_t interval );

sl_status_t wf200_set_wake_up_bit( uint8_t state );

sl_status_t wf200_get_signal_strength( uint32_t* rcpi );

sl_status_t wf200_set_max_ap_client( uint32_t max_clients );

sl_status_t wf200_add_multicast_address( const wf200_mac_address_t* mac_address, wf200_interface_t interface );

/*
 * Asynchronous WF200 commands
 */
sl_status_t wf200_send_join_command( const uint8_t* ssid, uint32_t ssid_length, uint16_t security_mode, const uint8_t* passkey, uint32_t passkey_length );

sl_status_t wf200_send_disconnect_command( void );

sl_status_t wf200_send_scan_command( uint16_t scan_mode, const uint8_t* channel_list, uint16_t channel_list_count, const WfmHiSsidDef_t* ssid_list, uint16_t ssid_list_count, const uint8_t* ie_data, uint16_t ie_data_length );

sl_status_t wf200_start_ap_command( uint32_t channel, uint8_t* ssid, uint32_t ssid_length, WfmSecurityMode security, const uint8_t* passkey, uint8_t passkey_length );

sl_status_t wf200_update_ap_command( uint16_t beacon_iedata_length, uint16_t proberesp_iedata_length, uint32_t* beacon_iedata, uint32_t* proberesp_iedata );

sl_status_t wf200_stop_ap_command( void );

sl_status_t wf200_disconnect_ap_client_command( const wf200_mac_address_t* client );

/*
 * WF200 test mode
 */

sl_status_t wf200_set_antenna_config( wf200_antenna_config_t config );


#ifdef __cplusplus
} /*extern "C" */
#endif

#endif // __WF200_H
