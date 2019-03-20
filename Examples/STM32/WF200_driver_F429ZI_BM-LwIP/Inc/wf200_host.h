#pragma once
#include <stdint.h>
#include "sl_status.h"

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

sl_status_t wf200_host_setup_waited_event( uint32_t event_id );