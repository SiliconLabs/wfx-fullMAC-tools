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
 * \file wf200.c
 * \brief WF200 Full MAC driver
 * \author Silicon Labs
 * \version 0.0.1
 * \date 24 septembre 2018
 *
 * wf200.c contains the APIs to communicate with the WF200 Full MAC API
 *
 */

#include "wf200.h"
#include "wf200_bus.h"
#include "wf200_host_api.h"
#include "wfx_fm_api.h"
#include "wfx_sm_api.h"
#include "firmware/wf200_registers.h"
#include "wf200_configuration.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Define the WEAK macro for GCC compatible compilers
#ifndef WEAK
#define WEAK __attribute__((weak))
#endif

#if (WF200_OPN_SIZE != API_OPN_SIZE)
#error Wi-Fi firmware OPN size has changed
#endif

/******************************************************
*                      Macros
******************************************************/

/******************************************************
*                    Constants
******************************************************/

/******************************************************
*                   Enumerations
******************************************************/

/******************************************************
*                 Type Definitions
******************************************************/

/******************************************************
*                    Structures
******************************************************/

/******************************************************
*                    Variables
******************************************************/

wf200_context_t* wf200_context;
uint8_t          encryption_keyset;
static uint16_t wf200_input_buffer_number;
/******************************************************
*               Static Function Declarations
******************************************************/

static sl_status_t wf200_send_request( uint16_t id, wf200_buffer_t* request, uint32_t request_length );
static sl_status_t poll_for_value( uint32_t address, uint32_t polled_value, uint32_t max_retries );
static sl_status_t wf200_init_chip( );
static sl_status_t wf200_download_run_bootloader();
static sl_status_t wf200_download_run_firmware( );

/******************************************************
*               Function Definitions
******************************************************/

/**
 * \brief Init wf200
 *
 * \note Actions performed by wf200_init(): Reset -> load firmware -> send PDS
 * \param context: wf200 context to maintain wf200 information
 * \return SL_SUCCESS if the initialization is successful, SL_ERROR otherwise
 */
sl_status_t wf200_init( wf200_context_t* context )
{
  sl_status_t     result;
  HiStartupInd_t* startup_info;

  memset( context, 0, sizeof( *context ) );

  wf200_context = context;
  wf200_context->used_buffer_number = 0;

  result = wf200_init_bus(  );
  ERROR_CHECK( result );
#ifdef DEBUG
  printf("--Bus initialized--\r\n");
#endif

  result = wf200_init_chip( );
  ERROR_CHECK( result );
#ifdef DEBUG
  printf("--Chip initialized--\r\n");
#endif

  result = wf200_download_run_bootloader();
  ERROR_CHECK( result );
#ifdef DEBUG
  printf("--Bootloader downloaded--\r\n");
#endif

  wf200_context->waited_event_id = HI_STARTUP_IND_ID;

  /* Downloading wf200 firmware contained in wfm_wf200.h */
  result = wf200_download_run_firmware( );
  ERROR_CHECK( result );
#ifdef DEBUG
  printf("--Firmware downloaded--\r\n");
#endif

  result = wf200_enable_irq(  );
  ERROR_CHECK( result );

  result = wf200_set_access_mode_message(  );
  ERROR_CHECK( result );
#ifdef DEBUG
  printf("--Message mode set--\r\n");
#endif

  /* Waiting for the startup indication from wf200, HI_STARTUP_IND_ID */
  result = wf200_host_wait_for_confirmation( WF200_DEFAULT_REQUEST_TIMEOUT, (void **)&startup_info );
  ERROR_CHECK( result );

  /* Storing mac addresses from wf200 in the context  */
  memcpy(&(context->mac_addr_0.octet), startup_info->Body.MacAddr0, sizeof(wf200_mac_address_t));
  memcpy(&(context->mac_addr_1.octet), startup_info->Body.MacAddr1, sizeof(wf200_mac_address_t));

  /* Sending to wf200 PDS configuration (Platform data set) contained in wf200_pds.c  */
  for ( uint8_t a = 0; a < wf200_config_count; a++ )
  {
    result = wf200_send_configuration( wf200_config_table[a].data, wf200_config_table[a].size );
  }
  ERROR_CHECK( result );
  wf200_input_buffer_number = startup_info->Body.NumInpChBufs;
#ifdef DEBUG
  printf("--PDS configured--\r\n");
#endif

error_handler:
  if ( result != SL_SUCCESS )
  {
    wf200_disable_irq( );
  }

  return result;
}

/**
 * \brief Deinit wf200
 *
 * \return SL_SUCCESS if the deinitialization is successful, SL_ERROR otherwise
 */
sl_status_t wf200_deinit( void )
{
  sl_status_t result;

  result = wf200_disable_irq( );
  ERROR_CHECK( result );

  result = wf200_shutdown( );
  ERROR_CHECK( result );

error_handler:
  return result;
}

/** \addtogroup FMAC_API
 *  @{
 */

/**
 * \brief Set the MAC addressed used by wf200
 *
 * \param sta_mac: MAC address used by the station interface
 * \param softap_mac: MAC address used by the softap interface
 * \return SL_SUCCESS if the request has been sent correctly, SL_ERROR otherwise
 */
sl_status_t wf200_set_mac_address( const wf200_mac_address_t* sta_mac, const wf200_mac_address_t* softap_mac )
{
  WfmHiSetMacAddressReqBody_t payload;

  memcpy( &payload.MacAddr0, &sta_mac->octet,    sizeof( payload.MacAddr0 ) );
  memcpy( &payload.MacAddr1, &softap_mac->octet, sizeof( payload.MacAddr1 ) );

  return wf200_send_command( WFM_HI_SET_MAC_ADDRESS_REQ_ID, &payload, sizeof( payload ), WF200_STA_INTERFACE );
}

/**
 * \brief As a station, send a command to join a Wi-Fi network
 *
 * \note The PDS file contains the WF200 settings
 * \param ssid: Name of the AP to connect to
 * \param ssid_length: Size of the SSID name
 * \param security_mode: Security mode of the AP
 *   \arg         WFM_SECURITY_MODE_OPEN
 *   \arg         WFM_SECURITY_MODE_WEP
 *   \arg         WFM_SECURITY_MODE_WPA2_WPA1_PSK
 *   \arg         WFM_SECURITY_MODE_WPA2_PSK
 * \param passkey: Passkey of the AP
 * \param passkey_length: Size of the passkey
 * \return SL_SUCCESS if the command has been sent correctly, SL_ERROR otherwise
 */
sl_status_t wf200_send_join_command( const uint8_t* ssid,
                                    uint32_t       ssid_length,
                                    uint16_t       security_mode,
                                    const uint8_t* passkey,
                                    uint32_t       passkey_length )
{
  sl_status_t            result;
  WfmHiConnectCnf_t*     reply;
  wf200_buffer_t*        frame           = NULL;
  WfmHiConnectReqBody_t* connect_request = NULL;

  result = wf200_host_allocate_buffer( &frame, WF200_CONTROL_BUFFER, sizeof( WfmHiConnectReq_t ), WF200_WAIT_FOREVER );
  ERROR_CHECK( result );

  frame->msg_info                      = 0;
  connect_request                      = (WfmHiConnectReqBody_t*)&frame->data;
  connect_request->Channel             = 0;
  connect_request->SecurityMode        = security_mode;
  connect_request->PreventRoaming      = 1;
  connect_request->MgmtFrameProtection = 0;
  connect_request->PasswordLength      = passkey_length;
  connect_request->IeDataLength        = 0;
  connect_request->SsidDef.SsidLength  = ssid_length;
  memcpy( connect_request->SsidDef.Ssid, ssid, ssid_length );
  memset( connect_request->SsidDef.Ssid + ssid_length, 0, WFM_API_SSID_SIZE - ssid_length );
  memset( connect_request->BSSID, 0xFF, WFM_API_BSSID_SIZE );
  memcpy( connect_request->Password, passkey, passkey_length );

  result = wf200_send_request( WFM_HI_CONNECT_REQ_ID, frame, sizeof( WfmHiConnectReq_t ));
  ERROR_CHECK( result );

  result = wf200_host_wait_for_confirmation( WF200_JOIN_REQUEST_TIMEOUT, (void**) &reply );
  ERROR_CHECK( result );

  if ( reply->Body.Status != WFM_STATUS_SUCCESS )
  {
    result = SL_ERROR;
  }

error_handler:
  if ( frame != NULL )
  {
    wf200_host_free_buffer( frame, WF200_CONTROL_BUFFER );
  }
  return result;
}

/**
 * \brief As a station, send a disconnection request to the AP
 *
 * \return SL_SUCCESS if the command has been sent correctly, SL_ERROR otherwise
 */
sl_status_t wf200_send_disconnect_command(void)
{
  return wf200_send_command( WFM_HI_DISCONNECT_REQ_ID, NULL, 0, WF200_STA_INTERFACE);
}

/**
 * \brief Send a command to start the softap mode
 *
 * \param channel: channel used by the softap
 * \param ssid: SSID name used by the softap
 * \param ssid_length: SSID name length
 * \param security: Security level used by the AP
 *   \arg         WFM_SECURITY_MODE_OPEN
 *   \arg         WFM_SECURITY_MODE_WEP
 *   \arg         WFM_SECURITY_MODE_WPA2_WPA1_PSK
 *   \arg         WFM_SECURITY_MODE_WPA2_PSK
 * \param passkey: Passkey used by the softap if security level is different to WFM_SECURITY_MODE_OPEN
 * \param passkey_length: Passkey length, [8;64]
 * \return SL_SUCCESS if the request has been sent correctly, SL_ERROR otherwise
 */
sl_status_t wf200_start_ap_command( uint32_t            channel,
                                   uint8_t*            ssid,
                                   uint32_t            ssid_length,
                                   wfm_security_mode   security,
                                   const uint8_t*      passkey,
                                   uint8_t             passkey_length )
{
  WfmHiStartApReqBody_t payload =
  {
    .Channel               = channel,
    .SecurityMode          = security,
    .SsidDef.SsidLength    = ssid_length,
    .HiddenSsid            = 0,
    .ClientIsolation       = 0,
    .MgmtFrameProtection   = 0,
    .BeaconIeDataLength    = 0,
    .ProbeRespIeDataLength = 0,
    .PasswordLength        = 0,
  };

  memcpy( payload.SsidDef.Ssid, ssid, ssid_length );
  memset( payload.SsidDef.Ssid + ssid_length, 0, sizeof( payload.SsidDef.Ssid ) - ssid_length );

  if ( security != WFM_SECURITY_MODE_OPEN )
  {
    payload.PasswordLength = passkey_length;
    memcpy( payload.Password, passkey, passkey_length );
    memset( &payload.Password[passkey_length], 0, sizeof( payload.Password ) - passkey_length );
  }
  else
  {
    memset( &payload.Password, '\0', WFM_API_PASSWORD_SIZE );
  }

  return wf200_send_command( WFM_HI_START_AP_REQ_ID, &payload, sizeof( payload ), WF200_SOFTAP_INTERFACE);
}

/**
 * \brief Update AP settings
 *
 * \param beacon_iedata_length: length of Beacon IE data
 * \param proberesp_iedata_length: length of probe response IE data
 * \param beacon_iedata: Beacon IE data
 * \param proberesp_iedata: probe response IE data
 * \return SL_SUCCESS if the request has been sent correctly, SL_ERROR otherwise
 */
sl_status_t wf200_update_ap_command( uint16_t beacon_iedata_length, uint16_t proberesp_iedata_length, uint32_t* beacon_iedata, uint32_t* proberesp_iedata )
{
  WfmHiUpdateApReqBody_t payload =
  {
    .BeaconIeDataLength          = beacon_iedata_length,
    .ProbeRespIeDataLength       = proberesp_iedata_length,
  };

  memcpy((WfmHiUpdateApReqBody_t*)(&payload + 1), beacon_iedata, beacon_iedata_length);
  memcpy((WfmHiUpdateApReqBody_t*)(&payload + 1) + beacon_iedata_length, proberesp_iedata, proberesp_iedata_length);

  return wf200_send_command( WFM_HI_UPDATE_AP_REQ_ID, &payload, sizeof( payload ), WF200_SOFTAP_INTERFACE);
}

/**
 * \brief Send a command to stop the softap mode
 *
 * \return SL_SUCCESS if the request has been sent correctly, SL_ERROR otherwise
 */
sl_status_t wf200_stop_ap_command( void )
{
  return wf200_send_command( WFM_HI_STOP_AP_REQ_ID, NULL, 0, WF200_SOFTAP_INTERFACE );
}

/**
 * \brief Send an Ethernet frame
 *
 * \param frame: Ethernet frame to be sent
 * \param data_length: Size of the frame
 * \param interface: Interface used to send the ethernet frame.
 *   \arg         WF200_STA_INTERFACE
 *   \arg         WF200_SOFTAP_INTERFACE
 * \return SL_SUCCESS if the command has been sent correctly, SL_ERROR otherwise
 */
sl_status_t wf200_send_ethernet_frame( wf200_frame_t* frame, uint32_t data_length, wf200_interface_t interface )
{
  HiMsgHdr_t* frame_header;
  sl_status_t           result = SL_ERROR_OUT_OF_BUFFERS;
  if(wf200_context->used_buffer_number <= wf200_input_buffer_number)
  {
    frame->FrameType        = WFM_FRAME_TYPE_DATA;
    frame->Priority         = WFM_PRIORITY_VO;
    frame->PacketId         = wf200_context->data_frame_id++;
    frame->PacketDataLength = data_length;
    frame->msg_id           = WFM_HI_SEND_FRAME_REQ_ID;

    frame_header = (HiMsgHdr_t*) frame;
    frame_header->s.t.MsgInfo = 0;
    frame_header->s.b.IntId = interface;

    // Note: Length must be multiple of 2 (16 bit word aligned)
    frame->msg_len = data_length + sizeof(wf200_frame_t);
    frame->msg_len = ( frame->msg_len + 1 ) & 0xFFFE;
    result = wf200_host_transmit_frame( (wf200_buffer_t*) frame );
    if(result == SL_SUCCESS){
        wf200_context->used_buffer_number++;
    }
  }
  return result;
}

/**
 * \brief send a scan command
 *
 * \param scan_mode: Mode used for scanning
 *   \arg        WFM_SCAN_MODE_PASSIVE
 *   \arg        WFM_SCAN_MODE_ACTIVE
 * \param channel_list: Channels to be scanned [1;13]
 * \param channel_list_count: Number of channels to be scanned
 * \param ssid_list: Specify SSID names to look for. WF200 will send information only the specified SSID. Null to request information for every AP found.
 * \param ssid_list_count: The number of SSID specified [0;2]
 * \param ie_data: NA
 * \param ie_data_length: NA
 * \return SL_SUCCESS if the command has been sent correctly, SL_ERROR otherwise
 */
sl_status_t wf200_send_scan_command( uint16_t               scan_mode,
                                    const uint8_t*         channel_list,
                                    uint16_t               channel_list_count,
                                    const WfmHiSsidDef_t*  ssid_list,
                                    uint16_t               ssid_list_count,
                                    const uint8_t*         ie_data,
                                    uint16_t               ie_data_length )
{
  sl_status_t              result;
  wf200_buffer_t*          frame;
  uint8_t*                 scan_params_copy_pointer;
  uint32_t                 scan_params_length   = channel_list_count + ( ssid_list_count * sizeof(WfmHiSsidDef_t) ) + ie_data_length;
  uint32_t                 request_total_length = sizeof(WfmHiStartScanReq_t) + scan_params_length;
  WfmHiStartScanReqBody_t* scan_request         = NULL;
  WfmHiStartScanCnf_t*     reply                = NULL;

  result = wf200_host_allocate_buffer( &frame, WF200_CONTROL_BUFFER, request_total_length, WF200_WAIT_FOREVER );
  ERROR_CHECK( result );

  frame->msg_info                      = 0;
  scan_request = (WfmHiStartScanReqBody_t*) frame->data;

  scan_request->ScanMode         = scan_mode;
  scan_request->ChannelListCount = channel_list_count;
  scan_request->SsidListCount    = ssid_list_count;
  scan_request->IeDataLength     = ie_data_length;

  scan_params_copy_pointer = (uint8_t*) scan_request + sizeof(WfmHiStartScanReqBody_t);

  // Write channel list
  if ( channel_list_count > 0 )
  {
    memcpy( scan_params_copy_pointer, channel_list, channel_list_count );
    scan_params_copy_pointer += channel_list_count;
  }

  // Write SSID list
  if ( ssid_list_count > 0 )
  {
    memcpy( scan_params_copy_pointer, ssid_list, ssid_list_count * sizeof(WfmHiSsidDef_t) );
    scan_params_copy_pointer += ssid_list_count * sizeof(WfmHiSsidDef_t);
  }

  // Write IE
  if ( ie_data_length > 0 )
  {
    memcpy( scan_params_copy_pointer, ie_data, ie_data_length );
  }

  result = wf200_send_request( WFM_HI_START_SCAN_REQ_ID, frame, request_total_length );
  ERROR_CHECK( result );

  result = wf200_host_wait_for_confirmation( WF200_DEFAULT_REQUEST_TIMEOUT, (void**) &reply );
  ERROR_CHECK( result );

  if ( reply->Body.Status != WFM_STATUS_SUCCESS )
  {
    result = SL_ERROR;
  }

error_handler:
  if ( frame != NULL )
  {
    wf200_host_free_buffer( frame, WF200_CONTROL_BUFFER );
  }
  return result;
}

/**
 * \brief Stop a scan process
 *
 * \return SL_SUCCESS if the request has been sent correctly, SL_ERROR otherwise
 */
sl_status_t wf200_send_stop_scan_command( void )
{
  return wf200_send_command( WFM_HI_STOP_SCAN_REQ_ID, NULL, 0, WF200_STA_INTERFACE);
}

/**
 * \brief Get the signal strength of the last packets received
 *
 * \param signal_strength: return the RCPI value averaged on the last packets received. RCPI ranges from 0 - 220 with 220 corresponds to 0dBm and each increment represents an increase of 0.5 dBm
 * \return SL_SUCCESS if the request has been sent correctly, SL_ERROR otherwise
 */
sl_status_t wf200_get_signal_strength( uint32_t* signal_strength )
{
  sl_status_t        result;
  wf200_buffer_t*    frame      = NULL;
  WfmHiGetSignalStrengthReq_t* get_signal_strength = NULL;
  WfmHiGetSignalStrengthCnf_t* reply      = NULL;

  result = wf200_host_allocate_buffer( &frame, WF200_CONTROL_BUFFER, sizeof( *get_signal_strength ), WF200_WAIT_FOREVER );
  ERROR_CHECK( result );

  get_signal_strength->s.b.IntId = WF200_STA_INTERFACE;

  result = wf200_send_request( WFM_HI_GET_SIGNAL_STRENGTH_REQ_ID, frame, sizeof( *get_signal_strength ) );
  ERROR_CHECK( result );

  result = wf200_host_wait_for_confirmation( WF200_DEFAULT_REQUEST_TIMEOUT, (void**) &reply );
  ERROR_CHECK( result );

  *signal_strength = reply->Body.Rcpi;

error_handler:
  if ( frame != NULL )
  {
    wf200_host_free_buffer( frame, WF200_CONTROL_BUFFER );
  }
  return result;
}

/**
 * \brief In AP mode, disconnect the specified client
 *
 * \param client: the mac address of the client to disconnect
 * \return SL_SUCCESS if the request has been sent correctly, SL_ERROR otherwise
 */
sl_status_t wf200_disconnect_ap_client_command( const wf200_mac_address_t* client )
{
  WfmHiDisconnectApClientReqBody_t payload;

  memcpy( payload.Mac, &client->octet, sizeof( payload.Mac ) );

  return wf200_send_command( WFM_HI_DISCONNECT_AP_CLIENT_REQ_ID, &payload, sizeof( payload ), WF200_SOFTAP_INTERFACE);
}

/**
 * \brief Set the power mode used as a station
 *
 * \note the power mode has to be set once the connection with the AP is established
 * \param mode: Power mode to be used by the connection
 *   \arg         WFM_PM_MODE_ACTIVE
 *   \arg         WFM_PM_MODE_PS
 *   \arg         WFM_PM_MODE_AUTO
 * \param interval: interval of sleep in seconds
 * \return SL_SUCCESS if the request has been sent correctly, SL_ERROR otherwise
 */
sl_status_t wf200_set_power_mode( wfm_pm_mode mode, uint16_t interval )
{
  WfmHiSetPmModeReqBody_t payload =
  {
    .PowerMode               = mode,
    .ListenInterval          = interval,
  };

  return wf200_send_command( WFM_HI_SET_PM_MODE_REQ_ID, &payload, sizeof( payload ), WF200_STA_INTERFACE );
}

/**
 * \brief Configure the maximum number of clients supported in softap mode
 * //TODO
 * \param mac_address:
 * \param interface: Interface used to send the ethernet frame. wf200_interface_t.
 *   \arg         WF200_STA_INTERFACE
 *   \arg         WF200_SOFTAP_INTERFACE
 * \return SL_SUCCESS if the setting is applied correctly, SL_ERROR otherwise
 */
sl_status_t wf200_add_multicast_address( const wf200_mac_address_t* mac_address, wf200_interface_t interface )
{
    sl_status_t                    result;
    wf200_buffer_t*                frame           = NULL;
    WfmHiAddMulticastAddrReq_t*    request         = NULL;
    WfmHiAddMulticastAddrCnf_t*    reply           = NULL;
    const uint32_t                 request_length  = sizeof( *request );
    uint16_t                       overhead        = 0;

    result = wf200_host_allocate_buffer( &frame, WF200_CONTROL_BUFFER, request_length + overhead, WF200_WAIT_FOREVER );
    ERROR_CHECK( result );

    request = (WfmHiAddMulticastAddrReq_t*)frame;
    memcpy( &request->Body.Mac, &mac_address->octet, WFM_API_MAC_SIZE );

    result = wf200_send_command( WFM_HI_ADD_MULTICAST_ADDR_REQ_ID, frame, request_length, interface );
    ERROR_CHECK( result );

    result = wf200_host_wait_for_confirmation( WF200_DEFAULT_REQUEST_TIMEOUT, (void**) &reply );
    ERROR_CHECK( result );

    // TODO: remove check for WFM_STATUS_WRONG_STATE (only included to make secure link testing easier)
    if ( reply->Body.Status != WFM_STATUS_SUCCESS && reply->Body.Status != WFM_STATUS_WRONG_STATE)
    {
        result = SL_ERROR;
    }

error_handler:
    if ( frame != NULL )
    {
        wf200_host_free_buffer( frame, WF200_CONTROL_BUFFER );
    }
    return result;
}

/**
 * \brief Configure the maximum number of clients supported in softap mode
 *
 * \note wf200_set_max_ap_client() has to be called after wf200_start_ap_command(). If the softap is stopped or wf200 resets, the command has to be issued again.
 * \param max_clients: Maximum number of clients supported in softap.
 * \return SL_SUCCESS if the setting is applied correctly, SL_ERROR otherwise
 */
sl_status_t wf200_set_max_ap_client( uint32_t max_clients )
{
    sl_status_t                    result;
    wf200_buffer_t*                frame           = NULL;
    WfmHiSetMaxApClientCountReq_t* ap_request      = NULL;
    WfmHiSetMaxApClientCountCnf_t* reply           = NULL;
    const uint32_t                 request_length  = sizeof( *ap_request );
    uint16_t                       overhead        = 0;

    result = wf200_host_allocate_buffer( &frame, WF200_CONTROL_BUFFER, request_length + overhead, WF200_WAIT_FOREVER );
    ERROR_CHECK( result );

    ap_request = (WfmHiSetMaxApClientCountReq_t*)frame;
    ap_request->Body.Count = max_clients;

    result = wf200_send_command( WFM_HI_SET_MAX_AP_CLIENT_COUNT_REQ_ID, frame, request_length, WF200_SOFTAP_INTERFACE );
    ERROR_CHECK( result );

    result = wf200_host_wait_for_confirmation( WF200_DEFAULT_REQUEST_TIMEOUT, (void**) &reply );
    ERROR_CHECK( result );

    if ( reply->Body.Status != WFM_STATUS_SUCCESS )
    {
        result = SL_ERROR;
    }

error_handler:
    if ( frame != NULL )
    {
        wf200_host_free_buffer( frame, WF200_CONTROL_BUFFER );
    }
    return result;
}
/** @}*/ //FMAC_API

/** \addtogroup COMMON_API
 *  @{
 */

/**
 * \brief Function to send PDS chunks
 *
 * \note The PDS file contains the WF200 settings (refer to wf200_pds.h)
 * \param pds_data: Data to be sent in the compressed PDS format
 * \param pds_data_length: Size of the data to be sent
 * \return SL_SUCCESS if the configuration has been sent correctly, SL_ERROR otherwise
 */
sl_status_t wf200_send_configuration( const char* pds_data, uint32_t pds_data_length )
{
  sl_status_t               result;
  HiConfigurationCnf_t*     reply;
  wf200_buffer_t*           frame          = NULL;
  HiConfigurationReqBody_t* config_request = NULL;
  uint32_t                  request_length = sizeof(HiConfigurationReq_t) + pds_data_length - API_PDS_DATA_SIZE; // '-4' to exclude size of 'Body.DpdData.SddData' field, which is already included in sdd_data_length

  result = wf200_host_allocate_buffer( &frame, WF200_CONTROL_BUFFER, request_length, WF200_WAIT_FOREVER );
  ERROR_CHECK( result );

  config_request         = (HiConfigurationReqBody_t*)&frame->data;
  config_request->Length = pds_data_length;

  memcpy( config_request->PdsData, pds_data, pds_data_length );

  result = wf200_send_request( HI_CONFIGURATION_REQ_ID, frame, request_length );
  ERROR_CHECK( result );

  result = wf200_host_wait_for_confirmation( WF200_DEFAULT_REQUEST_TIMEOUT, (void**)&reply );
  ERROR_CHECK( result );

  if(reply->Body.Status != WFM_STATUS_SUCCESS)
  {
    result = SL_ERROR;
  }

error_handler:
  if ( frame != NULL )
  {
    wf200_host_free_buffer( frame, WF200_CONTROL_BUFFER );
  }
  return result;
}

/**
 * \brief Shutdown wf200
 *
 * \note Send the shutdown command and clear GPIO WUP to enable wf200 to go to sleep
 *
 * \return SL_SUCCESS if the wf200 has been shutdown correctly, SL_ERROR otherwise
 */
sl_status_t wf200_shutdown( void )
{
    sl_status_t      status  = SL_SUCCESS;

    status = wf200_send_command( HI_SHUT_DOWN_REQ_ID, NULL, 0, WF200_STA_INTERFACE );
    ERROR_CHECK( status );

    // Clear WUP bit in control register
    status = wf200_set_wake_up_bit( 0 );

    error_handler:
    return status;
}
/** @}*/ //COMMON_API

/**
 * \brief Send a command to WF200
 *
 * \param command_id: ID of the command to be sent (cf. wfm_fm_api.h)
 * \param data: Pointer to the data to be sent by the command
 * \param data_size: Size of the data to be sent
 * \param interface: Interface used to send the ethernet frame. wf200_interface_t.
 *   \arg         WF200_STA_INTERFACE
 *   \arg         WF200_SOFTAP_INTERFACE
 * \return SL_SUCCESS if the command is sent correctly, SL_ERROR otherwise
 */
sl_status_t wf200_send_command( uint32_t command_id, void* data, uint32_t data_size, wf200_interface_t interface )
{
    sl_status_t           result;
    HiMsgHdr_t*           reply;
    HiMsgHdr_t*           request_header;
    wf200_buffer_t*       request = NULL;
    uint32_t              request_length = sizeof(HiMsgHdr_t) + data_size;

    result = wf200_host_allocate_buffer( &request, WF200_CONTROL_BUFFER, request_length, WF200_WAIT_FOREVER );
    ERROR_CHECK( result );

    request_header = (HiMsgHdr_t*) request;
    request_header->s.t.MsgInfo      = 0;
    request_header->s.b.IntId = interface;

    if ( data != NULL )
    {
        memcpy( request->data, data, data_size );
    }

    result = wf200_send_request( command_id, request, request_length );
    ERROR_CHECK( result );

    result = wf200_host_wait_for_confirmation( WF200_DEFAULT_REQUEST_TIMEOUT, (void**)&reply );
    ERROR_CHECK( result );

error_handler:
    if ( request != NULL )
    {
        wf200_host_free_buffer( request, WF200_CONTROL_BUFFER );
    }
    return result;
}

/**
 * \brief Send a request to wf200
 *
 * \param command_id: ID of the command to be sent (cf. wfm_fm_api.h)
 * \param request: Pointer to the request to be sent
 * \param request_length: Size of the request to be sent
 * \return SL_SUCCESS if the request is sent correctly, SL_ERROR otherwise
 */
static sl_status_t wf200_send_request( uint16_t command_id, wf200_buffer_t* request, uint32_t request_length )
{
    wf200_context->waited_event_id = command_id;
    wf200_context->posted_event_id = 0;

    request->msg_id   = command_id;
    request->msg_len  = request_length;
    request->msg_len  = ( request->msg_len + 1 ) & 0xFFFE; // Note: Length must be multiple of 2 (16 bit word aligned)

    return wf200_host_transmit_frame( request );
}

/**
 * \brief Receive available frame from wf200
 *
 * \note The host is responsible for handling the frame
 *
 * \return SL_SUCCESS if the request has been sent correctly, SL_ERROR otherwise
 */
sl_status_t wf200_receive_frame( void )
{
  sl_status_t result;
  uint16_t ctrl_reg = 0;

  result = wf200_reg_read_16( WF200_CONTROL_REG_ID, &ctrl_reg );
  ERROR_CHECK( result );
  if ( ( ctrl_reg & WF200_CONT_NEXT_LEN_MASK ) != 0 )
  {
    wf200_buffer_t* network_rx_buffer;
    uint32_t read_len = ( ctrl_reg & WF200_CONT_NEXT_LEN_MASK ) * 2 + 2; // critical : '+2' is for the piggy-back crlr register read at the end. without it the FW throws an exception
    result = wf200_host_allocate_buffer( &network_rx_buffer, WF200_RX_FRAME_BUFFER, ROUND_UP( read_len, 64 ), 0 );
    ERROR_CHECK( result );

    memset( network_rx_buffer, 0, read_len );

    result = wf200_data_read( network_rx_buffer, read_len );
    ERROR_CHECK( result );

    network_rx_buffer->msg_len = UNPACK_16BIT_LITTLE_ENDIAN(&network_rx_buffer->msg_len);
    network_rx_buffer->msg_id  = UNPACK_16BIT_LITTLE_ENDIAN(&network_rx_buffer->msg_id)  & 0xFF;

    ctrl_reg = UNPACK_16BIT_LITTLE_ENDIAN( ((uint8_t*)network_rx_buffer) + read_len - 2 );

    result = wf200_host_post_event(network_rx_buffer->msg_id, network_rx_buffer, read_len - 2);
    result = SL_WIFI_FRAME_RECEIVED;
  }
  else
  {
    result = SL_WIFI_NO_PACKET_TO_RECEIVE;
  }

error_handler:
  return result;
}

/**
 * \brief Enable wf200 irq
 *
 * \note Enable the host irq and set wf200 register accordingly
 *
 * \return SL_SUCCESS if the irq is enabled correctly, SL_ERROR otherwise
 */
sl_status_t wf200_enable_irq( void )
{
    uint32_t value32;
    sl_status_t result;

    result = wf200_host_enable_platform_interrupt();
    ERROR_CHECK(result);

    result = wf200_reg_read_32( WF200_CONFIG_REG_ID, &value32);
    ERROR_CHECK(result);

    value32 |= WF200_CONF_IRQ_RDY_ENABLE;

    result = wf200_reg_write_32( WF200_CONFIG_REG_ID, value32);
    ERROR_CHECK(result);

 error_handler:
    return result;
}

/**
 * \brief Disable wf200 irq
 *
 * \note Disable the host irq and set wf200 register accordingly
 *
 * \return SL_SUCCESS if the irq is disabled correctly, SL_ERROR otherwise
 */
sl_status_t wf200_disable_irq( void )
{
    uint32_t value32;
    sl_status_t result;

    result = wf200_reg_read_32( WF200_CONFIG_REG_ID, &value32);
    ERROR_CHECK(result);

    value32 &= ~WF200_CONF_IRQ_RDY_ENABLE;

    result = wf200_reg_write_32( WF200_CONFIG_REG_ID, value32);
    ERROR_CHECK(result);

    result = wf200_host_disable_platform_interrupt();
    ERROR_CHECK(result);

 error_handler:
    return result;
}

/**
 * \brief Set access mode message
 *
 * \return SL_SUCCESS if the message mode is enabled correctly, SL_ERROR otherwise
 */
sl_status_t wf200_set_access_mode_message( void )
{
    /* Configure device for MESSSAGE MODE */
    sl_status_t result;
    uint32_t    val32;

    result = wf200_reg_read_32( WF200_CONFIG_REG_ID, &val32 );
    ERROR_CHECK( result );

    result = wf200_reg_write_32( WF200_CONFIG_REG_ID, val32 & ~WF200_CONFIG_ACCESS_MODE_BIT );
    ERROR_CHECK( result );

error_handler:
    return result;
}

/**
 * \brief Set wf200 wake up bit
 *
 * \note Depending on the state parameter, set or reset the wake up bit
 * \param state: 0 to reset the wake up bit, set the wake up bit otherwise
 * \return SL_SUCCESS if the bit has been set correctly, SL_ERROR otherwise
 */
sl_status_t wf200_set_wake_up_bit( uint8_t state)
{
    sl_status_t status;
    uint16_t control_register_value;

    // Reading doesnt work, test later
    status = wf200_reg_read_16( WF200_CONTROL_REG_ID, &control_register_value );
    ERROR_CHECK( status );

    if ( state > 0 )
    {
        control_register_value |= WF200_CONT_WUP_BIT;
    }
    else
    {
        control_register_value &= ~WF200_CONT_WUP_BIT;
    }

    status = wf200_reg_write_16( WF200_CONTROL_REG_ID, control_register_value );

    error_handler: return status;
}

/******************************************************
 *                 Static Functions
 ******************************************************/

 /**
  * \brief Init wf200 chip
  *
  * \return SL_SUCCESS if the initialization is successful, SL_ERROR otherwise
  */
static sl_status_t wf200_init_chip( void )
{
    sl_status_t status;
    uint32_t    value32;
    uint16_t    value16;

    status = wf200_reg_read_32( WF200_CONFIG_REG_ID, &value32);
    ERROR_CHECK(status);

    /* General purpose registers setting */
    status = wf200_reg_write_32( WF200_TSET_GEN_R_W_REG_ID, 0x07208775);
    ERROR_CHECK(status);
    status = wf200_reg_write_32( WF200_TSET_GEN_R_W_REG_ID, 0x082ec020);
    ERROR_CHECK(status);
    status = wf200_reg_write_32( WF200_TSET_GEN_R_W_REG_ID, 0x093c3c3c);
    ERROR_CHECK(status);
    status = wf200_reg_write_32( WF200_TSET_GEN_R_W_REG_ID, 0x0b322c44);
    ERROR_CHECK(status);
    status = wf200_reg_write_32( WF200_TSET_GEN_R_W_REG_ID, 0x0ca06497);
    ERROR_CHECK(status);

    /* set wake-up bit */
    status = wf200_reg_read_16( WF200_CONTROL_REG_ID, &value16);
    ERROR_CHECK(status);
    value16 |= WF200_CONT_WUP_BIT;
    status = wf200_reg_write_16( WF200_CONTROL_REG_ID, value16);
    ERROR_CHECK(status);

    /* .. and wait for wake-up */
    for ( uint32_t i = 0; i < 200; ++i)
    {
        status = wf200_reg_read_16( WF200_CONTROL_REG_ID, &value16 );
        ERROR_CHECK( status );

        if ( ( value16 & WF200_CONT_RDY_BIT ) == WF200_CONT_RDY_BIT )
        {
          break;
        }else{
          wf200_host_wait( 1 );
        }
    }

    if ( ( value16 & WF200_CONT_RDY_BIT ) != WF200_CONT_RDY_BIT )
    {
        status = SL_TIMEOUT;
        ERROR_CHECK( status );
    }

    /* check for access mode bit */
    status = wf200_reg_read_32( WF200_CONFIG_REG_ID, &value32 );
    ERROR_CHECK( status );
    if ( ( value32 & WF200_CONFIG_ACCESS_MODE_BIT ) == 0 )
    {
        status = SL_ERROR;
        ERROR_CHECK( status );
    }

error_handler:
    return status;
}

/**
 * \brief Download wf200 bootloader
 *
 * \return SL_SUCCESS if the bootloader is downloaded correctly, SL_ERROR otherwise
 */
static sl_status_t wf200_download_run_bootloader( void )
{
    sl_status_t status;
    uint32_t    value32;

    status = wf200_apb_read_32( ADDR_DWL_CTRL_AREA_NCP_STATUS, &value32 );
    ERROR_CHECK( status );

    /* release CPU from reset and enable clock */
    status = wf200_reg_read_32( WF200_CONFIG_REG_ID, &value32 );
    ERROR_CHECK( status );
    value32 &= ~( WF200_CONFIG_CPU_RESET_BIT | WF200_CONFIG_CPU_CLK_DIS_BIT );
    status = wf200_reg_write_32( WF200_CONFIG_REG_ID, value32 );
    ERROR_CHECK( status );

    /* Testing SRAM access */
    status = wf200_apb_write_32( ADDR_DOWNLOAD_FIFO_BASE, 0x23abc88e );
    ERROR_CHECK( status );

    /* Check if the write command is successful */
    status = wf200_apb_read_32( ADDR_DOWNLOAD_FIFO_BASE, &value32 );
    ERROR_CHECK( status );
    if (value32 != 0x23abc88e)
    {
        status = SL_ERROR;
        ERROR_CHECK( status );
    }

error_handler:
    return status;
}

/**
 * \brief Download wf200 firmware
 *
 * \return SL_SUCCESS if the firmware is downloaded correctly, SL_ERROR otherwise
 */
static sl_status_t wf200_download_run_firmware( void )
{
    sl_status_t    status;
    uint32_t       i;
    uint32_t       value32;
    uint32_t       image_length;
    uint32_t       block;
    uint32_t       num_blocks;
    uint32_t       put = 0;
    uint32_t       get = 0;
    const uint8_t* buffer;

    status = wf200_host_init( );
    ERROR_CHECK( status );

    status = wf200_apb_write_32( ADDR_DWL_CTRL_AREA_HOST_STATUS, HOST_STATE_NOT_READY );
    ERROR_CHECK( status );
    status = wf200_apb_write_32( ADDR_DWL_CTRL_AREA_PUT, 0 );
    ERROR_CHECK( status );
    status = wf200_apb_write_32( ADDR_DWL_CTRL_AREA_GET, 0 );
    ERROR_CHECK( status );
    status = wf200_apb_write_32( ADDR_DWL_CTRL_AREA_HOST_STATUS, HOST_STATE_READY );
    ERROR_CHECK( status );

    // wait for INFO_READ state
    status = poll_for_value( ADDR_DWL_CTRL_AREA_NCP_STATUS, NCP_STATE_INFO_READY, 100 );
    ERROR_CHECK( status );

    // read info
    status = wf200_apb_read_32( 0x0900C080, &value32 );
    ERROR_CHECK( status );

    // TODO check key version here?
    status = wf200_apb_read_32( WFX_PTE_INFO + 12, &value32 );
    ERROR_CHECK(status);
    encryption_keyset = (value32 >> 8);

    // report that info is read
    status = wf200_apb_write_32( ADDR_DWL_CTRL_AREA_HOST_STATUS, HOST_STATE_HOST_INFO_READ );
    ERROR_CHECK( status );

    // wait for READY state
    status = poll_for_value( ADDR_DWL_CTRL_AREA_NCP_STATUS, NCP_STATE_READY, 100 );
    ERROR_CHECK( status );

    // SB misc initialization. Work around for chips < A2.
    status = wf200_apb_write_32( ADDR_DOWNLOAD_FIFO_BASE, 0xFFFFFFFF );
    ERROR_CHECK( status );

    // write image length
    wf200_host_get_firmware_size( &image_length );
    status = wf200_apb_write_32( ADDR_DWL_CTRL_AREA_IMAGE_SIZE, image_length - FW_HASH_SIZE - FW_SIGNATURE_SIZE );
    ERROR_CHECK( status );

    // write image signature, which is the first FW_SIGNATURE_SIZE of given image
    status = wf200_host_get_firmware_data( &buffer, FW_SIGNATURE_SIZE );
    ERROR_CHECK( status );
    status = wf200_apb_write( ADDR_DWL_CTRL_AREA_SIGNATURE, buffer, FW_SIGNATURE_SIZE );
    ERROR_CHECK( status );

    // write image hash, which is the next  FW_HASH_SIZE of given image
    status = wf200_host_get_firmware_data( &buffer, FW_HASH_SIZE );
    ERROR_CHECK( status );
    status = wf200_apb_write( ADDR_DWL_CTRL_AREA_FW_HASH, buffer, FW_HASH_SIZE );
    ERROR_CHECK( status );

    // write version, this is a pre-defined value (?)
    status = wf200_apb_write_32( ADDR_DWL_CTRL_AREA_FW_VERSION, FW_VERSION_VALUE );
    ERROR_CHECK( status );

    // notify NCP that upload is starting
    status = wf200_apb_write_32( ADDR_DWL_CTRL_AREA_HOST_STATUS, HOST_STATE_UPLOAD_PENDING );
    ERROR_CHECK( status );

    // skip signature and hash from image length
    image_length -= ( FW_HASH_SIZE + FW_SIGNATURE_SIZE );

    /* Calculate number of download blocks */
    num_blocks = (image_length - 1) / DOWNLOAD_BLOCK_SIZE + 1;

    /* Firmware downloading loop */
    for ( block = 0; block < num_blocks; block++ )
    {
        /* check the download status in NCP */
        status = wf200_apb_read_32( ADDR_DWL_CTRL_AREA_NCP_STATUS, &value32 );
        ERROR_CHECK( status );

        if ( value32 != NCP_STATE_DOWNLOAD_PENDING )
        {
            status = SL_ERROR;
            ERROR_CHECK( status );
        }

        /* loop until put - get <= 24K */
        for ( i = 0; i < 100; i++ )
        {
            get = 0;
            status = wf200_apb_read_32( ADDR_DWL_CTRL_AREA_GET, &get );
            ERROR_CHECK( status );

            if ( ( put - get ) <= ( DOWNLOAD_FIFO_SIZE - DOWNLOAD_BLOCK_SIZE ) )
            {
                break;
            }
        }

        if ( ( put - get ) > ( DOWNLOAD_FIFO_SIZE - DOWNLOAD_BLOCK_SIZE ) )
        {
            status = SL_WIFI_FIRMWARE_DOWNLOAD_TIMEOUT;
            ERROR_CHECK( status );
        }

        /* calculate the block size */
        uint32_t block_size = image_length - put;
        if ( block_size > DOWNLOAD_BLOCK_SIZE )
        {
            block_size = DOWNLOAD_BLOCK_SIZE;
        }

        /* send the block to SRAM */
        status = wf200_host_get_firmware_data( &buffer, block_size );
        ERROR_CHECK( status );
        uint32_t block_address = ADDR_DOWNLOAD_FIFO_BASE + ( put % DOWNLOAD_FIFO_SIZE );
        status = wf200_apb_write( block_address, buffer, block_size );
        ERROR_CHECK( status );
#ifdef DEBUG
        printf("Firmware page: %d/%d \n\r", put, image_length);
#endif
        /* update the put register */
        put += block_size;

        status = wf200_apb_write_32( ADDR_DWL_CTRL_AREA_PUT, put );
        ERROR_CHECK( status );

    } /* End of firmware download loop */

    // notify NCP that upload ended
    status = wf200_apb_write_32( ADDR_DWL_CTRL_AREA_HOST_STATUS, HOST_STATE_UPLOAD_COMPLETE );
    ERROR_CHECK( status );

    // wait for authentication result
    status = poll_for_value( ADDR_DWL_CTRL_AREA_NCP_STATUS, NCP_STATE_AUTH_OK, 100 );
    ERROR_CHECK( status );

    // notify NCP that we are happy to run firmware
    status = wf200_apb_write_32( ADDR_DWL_CTRL_AREA_HOST_STATUS, HOST_STATE_OK_TO_JUMP );
    ERROR_CHECK( status );

error_handler:
    wf200_host_deinit( );

    return status;
}

/**
 * \brief Poll a value from wf200
 *
 * \param address: Address of the value to be polled
 * \param polled_value: waiting for the value to be equal to polled_value
 * \param max_retries: Number of polling to be done before returning SL_TIMEOUT
 * \return SL_SUCCESS if the value is received correctly, SL_ERROR otherwise
 */
static sl_status_t poll_for_value( uint32_t address, uint32_t polled_value, uint32_t max_retries )
{
    uint32_t    value;
    sl_status_t status = SL_SUCCESS;

    for ( ; max_retries > 0; max_retries-- )
    {
        status = wf200_apb_read_32( address, &value );
        ERROR_CHECK( status );
        if ( value == polled_value )
        {
          break;
        }
        else
        {
          wf200_host_wait( 1 );
        }
    }
    if ( value != polled_value )
    {
        status = SL_TIMEOUT;
    }

error_handler:
    return status;
}

/**
 * \brief Configure the antenna setting of wf200
 *
 * \note  May require a re-init of the wf200 to change antenna again. Seems to be required to change antenna more than once.
 * \param config: antenna configuration to be used.
 *   \arg         WF200_ANTENNA_1_ONLY
 *   \arg         WF200_ANTENNA_2_ONLY
 *   \arg         WF200_ANTENNA_TX1_RX2
 *   \arg         WF200_ANTENNA_TX2_RX1
 *   \arg         WF200_ANTENNA_DIVERSITY
 * \return SL_SUCCESS if the setting is applied correctly, SL_ERROR otherwise
 */
sl_status_t wf200_set_antenna_config( wf200_antenna_config_t config )
{
    sl_status_t   result;
    char pds[32] = { 0 };
    char* current = pds;

    current += sprintf(current, "{%c:{",   PDS_ANTENNA_SEL_KEY);
    current += sprintf(current, "%c:%X,",  PDS_KEY_A, (unsigned int) config);
    if(config == WF200_ANTENNA_DIVERSITY)
    {
        // Set diversity mode internal, wf200 will control antenna allocation
        current += sprintf(current, "%c:%X}}", PDS_KEY_B, 1);
    }
    else
    {
        current += sprintf(current, "%c:%X}}", PDS_KEY_B, 0);
    }

    result = wf200_send_configuration((const char*)pds, strlen(pds));
    ERROR_CHECK(result);

error_handler:
    return result;
}

/**
 * \brief Retrieve the hardware version and type
 *
 * \param revision: pointer to retrieve the revision version
 * \param type: pointer to retrieve the type
 * \return SL_SUCCESS if the values are retrieved correctly, SL_ERROR otherwise
 */
sl_status_t wf200_get_hardware_revision_and_type( uint8_t* revision, uint8_t* type )
{
    uint32_t config_reg = 0;
    sl_status_t status = SL_SUCCESS;
    status = wf200_reg_read_32( WF200_CONFIG_REG_ID, &config_reg );

    *type = ( config_reg >> WF200_CONFIG_TYPE_OFFSET ) & WF200_CONFIG_TYPE_MASK;
    *revision = ( config_reg >> WF200_CONFIG_REVISION_OFFSET ) & WF200_CONFIG_REVISION_MASK;
    return status;
}

/**
 * \brief Get wf200 opn
 *
 * \param opn: ?? TODO
 * \return SL_SUCCESS if the value is retrieved correctly, SL_ERROR otherwise
 */
sl_status_t wf200_get_opn( uint8_t** opn )
{
    sl_status_t status = SL_ERROR;

    if ( wf200_context != NULL )
    {
        *opn = (uint8_t *) &( wf200_context->ineo_opn );
        status = SL_SUCCESS;
    }

    return status;
}
