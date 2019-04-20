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

#include "sl_wfx.h"

/// Define the WEAK macro for GCC compatible compilers
#ifndef WEAK
#define WEAK __attribute__((weak))
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
sl_wfx_context_t *sl_wfx_context;
static uint8_t   encryption_keyset;
static uint16_t  sl_wfx_input_buffer_number;
/******************************************************
*               Static Function Declarations
******************************************************/

static sl_status_t sl_wfx_poll_for_value(uint32_t address, uint32_t polled_value, uint32_t max_retries);
static sl_status_t sl_wfx_init_chip();
static sl_status_t sl_wfx_download_run_bootloader();
static sl_status_t sl_wfx_download_run_firmware();
static sl_status_t sl_wfx_compare_keysets(uint8_t sl_wfx_keyset, char *firmware_keyset);

/******************************************************
*               Function Definitions
******************************************************/

/**************************************************************************//**
 * @addtogroup DRIVER_API
 * @{
 *****************************************************************************/

/**************************************************************************//**
 * @brief Init the Wi-Fi chip
 *
 * @param context maintain the Wi-Fi chip information
 * @returns Returns SL_SUCCESS if the initialization is successful,
 * SL_ERROR otherwise
 *
 * @note Actions performed by sl_wfx_init(): Reset -> load firmware -> send PDS
 *****************************************************************************/
sl_status_t sl_wfx_init(sl_wfx_context_t *context)
{
  sl_status_t           result;
  sl_wfx_startup_ind_t *startup_info;
  uint16_t              pds_size;
  const char           *pds_data;

  memset(context, 0, sizeof(*context) );

  sl_wfx_context = context;
  sl_wfx_context->used_buffer_number = 0;

  result = sl_wfx_init_bus(  );
  SL_WFX_ERROR_CHECK(result);
#ifdef DEBUG
  printf("--Bus initialized--\r\n");
#endif

  result = sl_wfx_init_chip( );
  SL_WFX_ERROR_CHECK(result);
#ifdef DEBUG
  printf("--Chip initialized--\r\n");
#endif

  result = sl_wfx_download_run_bootloader();
  SL_WFX_ERROR_CHECK(result);
#ifdef DEBUG
  printf("--Bootloader downloaded--\r\n");
#endif

  sl_wfx_context->waited_event_id = SL_WFX_STARTUP_IND_ID;

  /* Downloading Wi-Fi chip firmware */
  result = sl_wfx_download_run_firmware( );
  SL_WFX_ERROR_CHECK(result);
#ifdef DEBUG
  printf("--Firmware downloaded--\r\n");
#endif

  result = sl_wfx_enable_irq(  );
  SL_WFX_ERROR_CHECK(result);

  result = sl_wfx_set_access_mode_message(  );
  SL_WFX_ERROR_CHECK(result);
#ifdef DEBUG
  printf("--Message mode set--\r\n");
#endif

  /* Waiting for the startup indication from Wi-Fi chip, SL_WFX_STARTUP_IND_ID */
  result = sl_wfx_host_wait_for_confirmation(SL_WFX_DEFAULT_REQUEST_TIMEOUT_MS, (void **)&startup_info);
  SL_WFX_ERROR_CHECK(result);

  /* Storing mac addresses from Wi-Fi chip in the context  */
  memcpy(&(context->mac_addr_0.octet), startup_info->body.mac_addr[0], sizeof(sl_wfx_mac_address_t));
  memcpy(&(context->mac_addr_1.octet), startup_info->body.mac_addr[1], sizeof(sl_wfx_mac_address_t));

  /* Storing input buffer limit from Wi-Fi chip. -1 to ensure a FMAC command will not exceed the limit */
  sl_wfx_input_buffer_number = sl_wfx_htole16(startup_info->body.num_inp_ch_bufs - 1);

  /* Set the wake up pin of the host */
  sl_wfx_host_set_wake_up_pin(1);

  result = sl_wfx_host_get_pds_size(&pds_size);
  SL_WFX_ERROR_CHECK(result);
  /* Sending to Wi-Fi chip PDS configuration (Platform data set) */
  for (uint8_t a = 0; a < pds_size; a++) {
    result = sl_wfx_host_get_pds_data(&pds_data,a);
    SL_WFX_ERROR_CHECK(result);
    result = sl_wfx_send_configuration(pds_data, strlen(pds_data));
    SL_WFX_ERROR_CHECK(result);
  }

#ifdef DEBUG
  printf("--PDS configured--\r\n");
#endif

  error_handler:
  if (result != SL_SUCCESS) {
    sl_wfx_disable_irq( );
    sl_wfx_deinit_bus();
    sl_wfx_host_deinit();
  }

  return result;
}

/**************************************************************************//**
 * @brief Deinit the Wi-Fi chip
 *
 * @returns Returns SL_SUCCESS if the deinitialization is successful,
 * SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_deinit(void)
{
  sl_status_t result;

  result = sl_wfx_shutdown();
  SL_WFX_ERROR_CHECK(result);

  result = sl_wfx_disable_irq();
  SL_WFX_ERROR_CHECK(result);

  result = sl_wfx_deinit_bus();
  SL_WFX_ERROR_CHECK(result);

  error_handler:
  return result;
}

/**************************************************************************//**
 * @addtogroup FULL_MAC_DRIVER_API
 * @{
 *****************************************************************************/

/**************************************************************************//**
 * Brief Set the MAC address used by Wi-Fi chip
 *
 * @param mac is the MAC address to be set on the specified the interface
 * @param interface is the interface to be configured. see sl_wfx_interface_t.
 *   @arg         SL_WFX_STA_INTERFACE
 *   @arg         SL_WFX_SOFTAP_INTERFACE
 * @returns Returns SL_SUCCESS if the request has been sent correctly,
 * SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_set_mac_address(const sl_wfx_mac_address_t *mac, sl_wfx_interface_t interface)
{
  sl_wfx_set_mac_address_req_body_t payload;
  memcpy(&payload.mac_addr, &mac->octet, sizeof(payload.mac_addr));
  return sl_wfx_send_command(SL_WFX_SET_MAC_ADDRESS_REQ_ID, &payload, sizeof(payload), interface, NULL);
}

/**************************************************************************//**
 * @brief As a station, send a command to join a Wi-Fi network
 *
 * @param ssid is the name of the AP to connect to
 * @param ssid_length is the length of the SSID name
 * @param bssid is the address of the AP to connect to (optional)
 * @param channel is the channel used by AP. Set at 0 if the channel is unknown
 * @param security_mode is the security used by the AP
 *   @arg         WFM_SECURITY_MODE_OPEN
 *   @arg         WFM_SECURITY_MODE_WEP
 *   @arg         WFM_SECURITY_MODE_WPA2_WPA1_PSK
 *   @arg         WFM_SECURITY_MODE_WPA2_PSK
 * @param prevent_roaming is equal to 1 to prevent automatic roaming between APs
 * @param management_frame_protection is equal to 1 to enable PMF mode
 * @param passkey is the passkey used by the AP. Can be the WPA hash key to
 * improve connection speed
 * @param passkey_length is the length of the passkey
 * @param ie_data are the Vendor-specific IEs to be added to the probe request
 * @param ie_data_length is the length of the IEs
 * @returns Returns SL_SUCCESS if the command has been sent correctly,
 * SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_send_join_command(const uint8_t  *ssid,
                                     uint32_t        ssid_length,
                                     const sl_wfx_mac_address_t *bssid,
                                     uint16_t        channel,
                                     sl_wfx_security_mode_t security_mode,
                                     uint8_t         prevent_roaming,
                                     uint16_t        management_frame_protection,
                                     const uint8_t  *passkey,
                                     uint16_t        passkey_length,
                                     const uint8_t  *ie_data,
                                     uint16_t        ie_data_length)
{
  sl_status_t               result           = SL_SUCCESS;
  sl_wfx_connect_cnf_t      *reply           = NULL;
  sl_wfx_generic_message_t  *frame           = NULL;
  sl_wfx_connect_req_body_t *connect_request = NULL;
  uint32_t request_length  = SL_WFX_ROUND_UP_EVEN(sizeof(sl_wfx_connect_req_t) + ie_data_length);

  result = sl_wfx_allocate_command_buffer(&frame, SL_WFX_CONNECT_REQ_ID, request_length, SL_WAIT_FOREVER);
  SL_WFX_ERROR_CHECK(result);

  memset(frame, 0, request_length);

  frame->header.info = SL_WFX_STA_INTERFACE;

  connect_request                        = (sl_wfx_connect_req_body_t *)&frame->body;
  connect_request->ssid_def.ssid_length  = sl_wfx_htole32(ssid_length);
  connect_request->channel               = sl_wfx_htole16(channel);
  connect_request->security_mode         = security_mode;
  connect_request->prevent_roaming       = prevent_roaming;
  connect_request->mgmt_frame_protection = sl_wfx_htole16(management_frame_protection);
  connect_request->password_length       = sl_wfx_htole16(passkey_length);
  connect_request->ie_data_length        = sl_wfx_htole16(ie_data_length);
  memcpy(connect_request->ssid_def.ssid, ssid, ssid_length);
  memcpy(connect_request->password, passkey, passkey_length);
  memcpy(connect_request + 1, ie_data, ie_data_length);
  if (bssid != NULL) {
    memcpy(connect_request->bssid, &bssid->octet, SL_WFX_BSSID_SIZE);
  } else {
    memset(connect_request->bssid, 0xFF, SL_WFX_BSSID_SIZE);
  }

  result = sl_wfx_send_request(SL_WFX_CONNECT_REQ_ID, frame, request_length);
  SL_WFX_ERROR_CHECK(result);

  result = sl_wfx_host_wait_for_confirmation(SL_WFX_DEFAULT_REQUEST_TIMEOUT_MS, (void **)&reply);
  SL_WFX_ERROR_CHECK(result);

  if (sl_wfx_htole32(reply->body.status) != WFM_STATUS_SUCCESS) {
    result = SL_ERROR;
  }

  error_handler:
  if (frame != NULL) {
    sl_wfx_host_free_buffer(frame, SL_WFX_CONTROL_BUFFER);
  }
  return result;
}

/**************************************************************************//**
 * @brief Connected as a station, send a disconnection request to the AP
 *
 * @returns SL_SUCCESS if the command has been sent correctly,
 * SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_send_disconnect_command(void)
{
  return sl_wfx_send_command(SL_WFX_DISCONNECT_REQ_ID, NULL, 0, SL_WFX_STA_INTERFACE, NULL);
}

/**************************************************************************//**
 * @brief Send a command to start the softap mode
 *
 * @param channel is the channel used by AP. Between 1 and 14
 * @param ssid is the SSID name used by the softap
 * @param ssid_length is the SSID name length
 * @param hidden_ssid is equal to 1 to hide the network
 * @param client_isolation is equal to 1 isolate clients from each other
 * @param security_mode is the security level used by the softap
 *   @arg         WFM_SECURITY_MODE_OPEN
 *   @arg         WFM_SECURITY_MODE_WEP
 *   @arg         WFM_SECURITY_MODE_WPA2_WPA1_PSK
 *   @arg         WFM_SECURITY_MODE_WPA2_PSK
 * @param management_frame_protection is equal to 1 to enable PMF mode
 * @param passkey is the passkey used by the softap. Only applicable in security
 * modes different from WFM_SECURITY_MODE_OPEN.
 * @param passkey_length is the length of the passkey
 * @param beacon_ie_data is the Vendor-specific IE data to be added to beacons
 * @param beacon_ie_data_length is the length of the beacon IEs
 * @param probe_response_ie_data is the Vendor-specific IE data to be added to
 * probe responses
 * @param probe_response_ie_data_length is the length of the probe response IEs
 * @returns Returns SL_SUCCESS if the command has been sent correctly,
 * SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_start_ap_command(uint16_t        channel,
                                    uint8_t        *ssid,
                                    uint32_t        ssid_length,
                                    uint8_t         hidden_ssid,
                                    uint8_t         client_isolation,
                                    sl_wfx_security_mode_t security_mode,
                                    uint8_t         management_frame_protection,
                                    const uint8_t  *passkey,
                                    uint16_t        passkey_length,
                                    const uint8_t  *beacon_ie_data,
                                    uint16_t        beacon_ie_data_length,
                                    const uint8_t  *probe_response_ie_data,
                                    uint16_t        probe_response_ie_data_length)
{
  sl_status_t                 result           = SL_SUCCESS;
  sl_wfx_start_ap_cnf_t      *reply            = NULL;
  sl_wfx_generic_message_t   *frame            = NULL;
  sl_wfx_start_ap_req_body_t *start_ap_request = NULL;
  uint32_t request_length = SL_WFX_ROUND_UP_EVEN(sizeof(sl_wfx_start_ap_req_t) + beacon_ie_data_length + probe_response_ie_data_length);

  result = sl_wfx_allocate_command_buffer(&frame, SL_WFX_START_AP_REQ_ID, request_length, SL_WAIT_FOREVER);
  SL_WFX_ERROR_CHECK(result);

  memset(frame, 0, request_length);

  frame->header.info = SL_WFX_SOFTAP_INTERFACE;

  start_ap_request                            = (sl_wfx_start_ap_req_body_t *)&frame->body;
  start_ap_request->ssid_def.ssid_length      = sl_wfx_htole32(ssid_length);
  start_ap_request->hidden_ssid               = hidden_ssid;
  start_ap_request->client_isolation          = client_isolation;
  start_ap_request->security_mode             = security_mode;
  start_ap_request->mgmt_frame_protection     = management_frame_protection;
  start_ap_request->channel                   = sl_wfx_htole16(channel);
  start_ap_request->password_length           = sl_wfx_htole16(passkey_length);
  start_ap_request->beacon_ie_data_length     = sl_wfx_htole16(beacon_ie_data_length);
  start_ap_request->probe_resp_ie_data_length = sl_wfx_htole16(probe_response_ie_data_length);
  memcpy(start_ap_request->ssid_def.ssid, ssid, ssid_length);
  memcpy(start_ap_request->password, passkey, passkey_length);
  memcpy(start_ap_request + 1, beacon_ie_data, beacon_ie_data_length);
  memcpy((uint8_t *)(start_ap_request + 1) + beacon_ie_data_length,
                    probe_response_ie_data,
                    probe_response_ie_data_length);

  result = sl_wfx_send_request(SL_WFX_START_AP_REQ_ID, frame, request_length);
  SL_WFX_ERROR_CHECK(result);

  result = sl_wfx_host_wait_for_confirmation(SL_WFX_DEFAULT_REQUEST_TIMEOUT_MS, (void **)&reply);
  SL_WFX_ERROR_CHECK(result);

  if (sl_wfx_htole32(reply->body.status) != WFM_STATUS_SUCCESS) {
    result = SL_ERROR;
  }

  error_handler:
  if (frame != NULL) {
    sl_wfx_host_free_buffer(frame, SL_WFX_CONTROL_BUFFER);
  }
  return result;
}

/**************************************************************************//**
 * @brief Update the softap settings
 *
 * @param beacon_ie_data_length is the length of the beacon IEs
 * @param probe_response_ie_data_length is the length of the probe response IEs
 * @param beacon_ie_data is the Vendor-specific IE data to be added to beacons
 * @param probe_response_ie_data is the Vendor-specific IE data to be added to
 * probe responses
 * @returns SL_SUCCESS if the command has been sent correctly,
 * SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_update_ap_command(uint16_t beacon_ie_data_length,
                                     uint16_t probe_response_ie_data_length,
                                     uint32_t *beacon_ie_data,
                                     uint32_t *probe_response_ie_data)
{
  sl_status_t                  result            = SL_SUCCESS;
  sl_wfx_update_ap_cnf_t      *reply             = NULL;
  sl_wfx_generic_message_t    *frame             = NULL;
  sl_wfx_update_ap_req_body_t *update_ap_request = NULL;
  uint32_t request_length = SL_WFX_ROUND_UP_EVEN(sizeof(sl_wfx_update_ap_req_t) + beacon_ie_data_length + probe_response_ie_data_length);

  result = sl_wfx_allocate_command_buffer(&frame, SL_WFX_UPDATE_AP_REQ_ID, request_length, SL_WAIT_FOREVER);
  SL_WFX_ERROR_CHECK(result);

  memset(frame, 0, request_length);

  frame->header.info = SL_WFX_SOFTAP_INTERFACE;

  update_ap_request                            = (sl_wfx_update_ap_req_body_t *)&frame->body;
  update_ap_request->beacon_ie_data_length     = sl_wfx_htole16(beacon_ie_data_length);
  update_ap_request->probe_resp_ie_data_length = sl_wfx_htole16(probe_response_ie_data_length);
  memcpy(update_ap_request + 1, beacon_ie_data, beacon_ie_data_length);
  memcpy((uint8_t *)(update_ap_request + 1) + beacon_ie_data_length, probe_response_ie_data, probe_response_ie_data_length);

  result = sl_wfx_send_request(SL_WFX_UPDATE_AP_REQ_ID, frame, request_length);
  SL_WFX_ERROR_CHECK(result);

  result = sl_wfx_host_wait_for_confirmation(SL_WFX_DEFAULT_REQUEST_TIMEOUT_MS, (void **)&reply);
  SL_WFX_ERROR_CHECK(result);

  if (sl_wfx_htole32(reply->body.status) != WFM_STATUS_SUCCESS) {
    result = SL_ERROR;
  }

  error_handler:
  if (frame != NULL) {
    sl_wfx_host_free_buffer(frame, SL_WFX_CONTROL_BUFFER);
  }
  return result;
}

/**************************************************************************//**
 * @brief Send a command to stop the softap mode
 *
 * @returns SL_SUCCESS if the command has been sent correctly,
 * SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_stop_ap_command(void)
{
  return sl_wfx_send_command(SL_WFX_STOP_AP_REQ_ID, NULL, 0, SL_WFX_SOFTAP_INTERFACE, NULL);
}

/**************************************************************************//**
 * @brief Send an Ethernet frame
 *
 * @param frame contains the Ethernet frame to be sent
 * @param data_length is the length of the Ethernet frame
 * @param interface is the interface used to send the ethernet frame.
 *   @arg         SL_WFX_STA_INTERFACE
 *   @arg         SL_WFX_SOFTAP_INTERFACE
 * @param priority is the priority level used to send the Ethernet frame.
 * @returns SL_SUCCESS if the request has been sent correctly,
 * SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_send_ethernet_frame(sl_wfx_send_frame_req_t *frame,
                                       uint32_t data_length,
                                       sl_wfx_interface_t interface,
                                       uint8_t priority)
{
  sl_status_t              result = SL_ERROR_OUT_OF_BUFFERS;
  uint32_t request_length = SL_WFX_ROUND_UP_EVEN(sizeof(sl_wfx_send_frame_req_t) + data_length);

  if (sl_wfx_context->used_buffer_number < sl_wfx_input_buffer_number)
  {
    sl_wfx_context->data_frame_id++;

    frame->header.length           = sl_wfx_htole16(data_length + sizeof(sl_wfx_send_frame_req_t));
    frame->header.id               = SL_WFX_SEND_FRAME_REQ_ID;
    frame->header.info             = interface;
    frame->body.frame_type         = WFM_FRAME_TYPE_DATA;
    frame->body.priority           = priority;
    frame->body.packet_id          = sl_wfx_htole16(sl_wfx_context->data_frame_id);
    frame->body.packet_data_length = sl_wfx_htole32(data_length);

    result = sl_wfx_send_request(SL_WFX_SEND_FRAME_REQ_ID, (sl_wfx_generic_message_t*) frame, request_length);

    if (result == SL_SUCCESS) {
      sl_wfx_context->used_buffer_number++;
    }
  }

  return result;
}

/**************************************************************************//**
 * @brief Send a scan command
 *
 * @param scan_mode is the mode used during the scan phase
 *   @arg        WFM_SCAN_MODE_PASSIVE
 *   @arg        WFM_SCAN_MODE_ACTIVE
 * @param channel_list is the list of channels to be scanned. Suitable values
 * goes from 1 to 13.
 * @param channel_list_count is the number of channels to be scanned.
 * If 0 the scan will be performed on every channel.
 * @param ssid_list is the list of SSID names to look for. The Wi-Fi chip will
 * only send scan results for the specified SSIDs. If Null, every scan result
 * will be passed on to the host.
 * @param ssid_list_count is the number of SSID specified (from 0 up to 2)
 * @param ie_data is the Vendor-specific IE data to be added to probe requests
 * @param ie_data_length is the length of the probe request IEs
 * @returns SL_SUCCESS if the command has been sent correctly,
 * SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_send_scan_command(uint16_t               scan_mode,
                                     const uint8_t         *channel_list,
                                     uint16_t               channel_list_count,
                                     const sl_wfx_ssid_def_t  *ssid_list,
                                     uint16_t               ssid_list_count,
                                     const uint8_t         *ie_data,
                                     uint16_t               ie_data_length)
{
  sl_status_t                   result                   = SL_SUCCESS;
  sl_wfx_generic_message_t     *frame                    = NULL;
  uint8_t                      *scan_params_copy_pointer = NULL;
  sl_wfx_start_scan_req_body_t *scan_request             = NULL;
  sl_wfx_start_scan_cnf_t      *reply                    = NULL;
  uint32_t scan_params_length   = channel_list_count + (ssid_list_count * sizeof(sl_wfx_ssid_def_t) ) + ie_data_length;
  uint32_t request_total_length = SL_WFX_ROUND_UP_EVEN(sizeof(sl_wfx_start_scan_req_t) + scan_params_length);
  uint32_t reply_status;

  result = sl_wfx_allocate_command_buffer(&frame, SL_WFX_START_SCAN_REQ_ID, request_total_length, SL_WAIT_FOREVER);
  SL_WFX_ERROR_CHECK(result);

  memset(frame, 0, request_total_length);

  scan_request = (sl_wfx_start_scan_req_body_t *)frame->body;

  //TODO: swap SSID length
  scan_request->scan_mode          = sl_wfx_htole16(scan_mode);
  scan_request->channel_list_count = sl_wfx_htole16(channel_list_count);
  scan_request->ssid_list_count    = sl_wfx_htole16(ssid_list_count);
  scan_request->ie_data_length     = sl_wfx_htole16(ie_data_length);

  scan_params_copy_pointer = (uint8_t *)scan_request + sizeof(sl_wfx_start_scan_req_body_t);

  // Write channel list
  if (channel_list_count > 0) {
    memcpy(scan_params_copy_pointer, channel_list, channel_list_count);
    scan_params_copy_pointer += channel_list_count;
  }

  // Write SSID list
  if (ssid_list_count > 0) {
    memcpy(scan_params_copy_pointer, ssid_list, ssid_list_count * sizeof(sl_wfx_ssid_def_t) );
    scan_params_copy_pointer += ssid_list_count * sizeof(sl_wfx_ssid_def_t);
  }

  // Write IE
  if (ie_data_length > 0) {
    memcpy(scan_params_copy_pointer, ie_data, ie_data_length);
  }

  result = sl_wfx_send_request(SL_WFX_START_SCAN_REQ_ID, frame, request_total_length);
  SL_WFX_ERROR_CHECK(result);

  result = sl_wfx_host_wait_for_confirmation(SL_WFX_DEFAULT_REQUEST_TIMEOUT_MS, (void **)&reply);
  SL_WFX_ERROR_CHECK(result);

  reply_status = sl_wfx_htole32(reply->body.status);

  if ((reply_status != WFM_STATUS_SUCCESS) && (reply_status != WFM_STATUS_WARNING)) {
    result = SL_ERROR;
  }

  error_handler:
  if (frame != NULL) {
    sl_wfx_host_free_buffer(frame, SL_WFX_CONTROL_BUFFER);
  }
  return result;
}

/**************************************************************************//**
 * @brief Stop an ongoing scan process
 *
 * @returns SL_SUCCESS if the command has been sent correctly,
 * SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_send_stop_scan_command(void)
{
  return sl_wfx_send_command(SL_WFX_STOP_SCAN_REQ_ID, NULL, 0, SL_WFX_STA_INTERFACE, NULL);
}

/**************************************************************************//**
 * @brief Join or create an IBSS network
 *
 * @param ssid is the name of the IBSS network
 * @param ssid_length is the length of the SSID name
 * @param channel is the channel used by the network
 * @param security_mode is the security mode used by the network
 *   @arg         WFM_SECURITY_MODE_OPEN
 *   @arg         WFM_SECURITY_MODE_WEP
 * @param passkey is the passkey used by the network
 * @param passkey_length is the length of the passkey
 * @returns SL_SUCCESS if the command has been sent correctly,
 * SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_join_ibss_command(const uint8_t  *ssid,
                                     uint32_t        ssid_length,
                                     uint32_t        channel,
                                     uint16_t        security_mode,
                                     const uint8_t  *passkey,
                                     uint16_t        passkey_length)
{
  sl_wfx_join_ibss_req_body_t payload;

  payload.ssid_def.ssid_length = sl_wfx_htole32(ssid_length);
  payload.channel              = sl_wfx_htole32(channel);
  payload.security_mode        = sl_wfx_htole16(security_mode);
  payload.password_length      = sl_wfx_htole16(passkey_length);
  memcpy(payload.ssid_def.ssid, ssid, ssid_length);
  memcpy(payload.password, passkey, passkey_length);

  return sl_wfx_send_command(SL_WFX_JOIN_IBSS_REQ_ID, &payload, sizeof(payload), SL_WFX_STA_INTERFACE, NULL);
}

/**************************************************************************//**
 * @brief Send a command to stop the IBSS mode
 *
 * @returns SL_SUCCESS if the command has been sent correctly,
 * SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_leave_ibss_command(void)
{
  return sl_wfx_send_command(SL_WFX_LEAVE_IBSS_REQ_ID, NULL, 0, SL_WFX_STA_INTERFACE, NULL);
}

/**************************************************************************//**
 * @brief Get the signal strength of the last packets received
 *
 * @param signal_strength: return the RCPI value averaged on the last packets
 * received. RCPI ranges from 0 - 220 with 220 corresponds to 0d_bm and each
 * increment represents an increase of 0.5 d_bm.
 * @returns SL_SUCCESS if the command has been sent correctly,
 * SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_get_signal_strength(uint32_t *signal_strength)
{
  sl_status_t result;
  sl_wfx_get_signal_strength_cnf_t *reply = NULL;

  result = sl_wfx_send_command(SL_WFX_GET_SIGNAL_STRENGTH_REQ_ID,
                               NULL,
                               0,
                               SL_WFX_STA_INTERFACE,
                               (sl_wfx_generic_confirmation_t **)&reply);

  if (result == SL_SUCCESS) {
    if (sl_wfx_htole32(reply->body.status) != WFM_STATUS_SUCCESS) {
      result = SL_ERROR;
    }
    *signal_strength = sl_wfx_htole32(reply->body.rcpi);
  }

  return result;
}

/**************************************************************************//**
 * @brief In AP mode, disconnect the specified client
 *
 * @param client is the mac address of the client to disconnect
 * @returns SL_SUCCESS if the command has been sent correctly,
 * SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_disconnect_ap_client_command(const sl_wfx_mac_address_t *client)
{
  sl_wfx_disconnect_ap_client_req_body_t payload;

  memcpy(payload.mac, &client->octet, sizeof(payload.mac) );

  return sl_wfx_send_command(SL_WFX_DISCONNECT_AP_CLIENT_REQ_ID, &payload, sizeof(payload), SL_WFX_SOFTAP_INTERFACE, NULL);
}

/**************************************************************************//**
 * @brief Set the power mode used as a station. The Wi-Fi chip will enable the
 * Wi-Fi power save feature.
 *
 * @param mode is the power mode to be used by the connection
 *   @arg         WFM_PM_MODE_ACTIVE
 *   @arg         WFM_PM_MODE_BEACON
 *   @arg         WFM_PM_MODE_DTIM
 * @param interval is the number of beacons/DTIMs to skip while sleeping
 * @returns SL_SUCCESS if the command has been sent correctly,
 * SL_ERROR otherwise
 *
 * @note the power mode has to be set once the connection with the AP is
 * established
 *****************************************************************************/
sl_status_t sl_wfx_set_power_mode(sl_wfx_pm_mode_t mode, uint16_t interval)
{
  sl_wfx_set_pm_mode_req_body_t payload;

  payload.power_mode      = sl_wfx_htole16(mode);
  payload.listen_interval = sl_wfx_htole16(interval);

  return sl_wfx_send_command(SL_WFX_SET_PM_MODE_REQ_ID, &payload, sizeof(payload), SL_WFX_STA_INTERFACE, NULL);
}

/**************************************************************************//**
 * @brief Add a multicast address to the multicast filter whitelist
 *
 * @param mac_address is the MAC address to add
 * @param interface is the interface where to apply the change
 *   @arg         SL_WFX_STA_INTERFACE
 *   @arg         SL_WFX_SOFTAP_INTERFACE
 * @returns SL_SUCCESS if the setting is applied correctly, SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_add_multicast_address(const sl_wfx_mac_address_t *mac_address, sl_wfx_interface_t interface)
{
  sl_wfx_add_multicast_addr_req_body_t payload;

  memcpy(payload.mac, &mac_address->octet, sizeof(payload.mac) );

  return sl_wfx_send_command(SL_WFX_ADD_MULTICAST_ADDR_REQ_ID, &payload, sizeof(payload), interface, NULL);
}

/**************************************************************************//**
 * @brief Remove a multicast address from the multicast filter whitelist
 *
 * @param mac_address is the MAC address to remove
 * @param interface is the interface where to apply the change
 *   @arg         SL_WFX_STA_INTERFACE
 *   @arg         SL_WFX_SOFTAP_INTERFACE
 * @returns SL_SUCCESS if the setting is applied correctly, SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_remove_multicast_address(const sl_wfx_mac_address_t *mac_address, sl_wfx_interface_t interface)
{
  sl_wfx_remove_multicast_addr_req_body_t payload;

  memcpy(payload.mac, &mac_address->octet, sizeof(payload.mac) );

  return sl_wfx_send_command(SL_WFX_REMOVE_MULTICAST_ADDR_REQ_ID, &payload, sizeof(payload), interface, NULL);
}

/**************************************************************************//**
 * @brief Set the ARP offloading state
 *
 * @param arp_ip_addr is the pointer to ARP IP address list to offload
 * @param num_arp_ip_addr is the number of addresses in the list (0 - 2)
 * @returns SL_SUCCESS if the setting is applied correctly, SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_set_arp_ip_address(uint32_t *arp_ip_addr, uint8_t num_arp_ip_addr)
{
  sl_wfx_set_arp_ip_address_req_body_t payload;
  uint8_t i;

  memset(&payload, 0, sizeof(sl_wfx_set_arp_ip_address_req_body_t) );

  for (i = 0; i < num_arp_ip_addr; i++) {
    payload.arp_ip_addr[i] = sl_wfx_htole32(arp_ip_addr[i]);
  }

  return sl_wfx_send_command(SL_WFX_SET_ARP_IP_ADDRESS_REQ_ID, &payload, sizeof(payload), SL_WFX_STA_INTERFACE, NULL);
}

/**************************************************************************//**
 * @brief Set the NS offloading state
 *
 * @param ns_ip_addr is the pointer to NS IP address list to offload
 * @param num_ns_ip_addr is the number of addresses in the list (0 - 2)
 * @returns SL_SUCCESS if the setting is applied correctly, SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_set_ns_ip_address(uint8_t *ns_ip_addr, uint8_t num_ns_ip_addr)
{
  sl_wfx_set_ns_ip_address_req_body_t payload;
  uint8_t i;

  memset(&payload, 0, sizeof(sl_wfx_set_ns_ip_address_req_body_t));

  for (i = 0; i < num_ns_ip_addr; i++) {
    memcpy(&payload.ns_ip_addr[i], &ns_ip_addr[i * sizeof(sl_wfx_ns_ip_addr_t)], sizeof(sl_wfx_ns_ip_addr_t) );
  }

  return sl_wfx_send_command(SL_WFX_SET_NS_IP_ADDRESS_REQ_ID, &payload, sizeof(payload), SL_WFX_STA_INTERFACE, NULL);
}

/**************************************************************************//**
 * @brief Set the broadcast filter state
 *
 * @param filter is equal to 1 to enable broadcast filtering.
 * @returns SL_SUCCESS if the setting is applied correctly, SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_set_broadcast_filter(uint32_t filter)
{
  sl_wfx_set_broadcast_filter_req_body_t payload;

  payload.filter = sl_wfx_htole32(filter);

  return sl_wfx_send_command(SL_WFX_SET_BROADCAST_FILTER_REQ_ID, &payload, sizeof(payload), SL_WFX_STA_INTERFACE, NULL);
}

/**************************************************************************//**
 * @brief Set the unicast filter state
 *
 * @param filter is equal to 1 to enable unicast filtering.
 * @returns SL_SUCCESS if the setting is applied correctly, SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_set_unicast_filter(uint32_t filter)
{
  sl_wfx_set_unicast_filter_req_body_t payload;

  payload.filter = sl_wfx_htole32(filter);

  return sl_wfx_send_command(SL_WFX_SET_UNICAST_FILTER_REQ_ID, &payload, sizeof(payload), SL_WFX_SOFTAP_INTERFACE, NULL);
}

/**************************************************************************//**
 * @brief Add a mac address to the device whitelist
 *
 * @param mac_address is the MAC address to add.
 * Broadcast address allows all MAC addresses.
 * @returns SL_SUCCESS if the setting is applied correctly, SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_add_whitelist_address(const sl_wfx_mac_address_t *mac_address)
{
  sl_wfx_add_whitelist_addr_req_body_t payload;

  memcpy(payload.mac, &mac_address->octet, sizeof(payload.mac));

  return sl_wfx_send_command(SL_WFX_ADD_WHITELIST_ADDR_REQ_ID, &payload, sizeof(payload), SL_WFX_SOFTAP_INTERFACE, NULL);
}

/**************************************************************************//**
 * @brief Add a mac address to the device blacklist
 *
 * @param mac_address is the MAC address to add.
 * Broadcast address denies all MAC addresses.
 * @returns SL_SUCCESS if the setting is applied correctly, SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_add_blacklist_address(const sl_wfx_mac_address_t *mac_address)
{
  sl_wfx_add_blacklist_addr_req_body_t payload;

  memcpy(payload.mac, &mac_address->octet, sizeof(payload.mac));

  return sl_wfx_send_command(SL_WFX_ADD_BLACKLIST_ADDR_REQ_ID, &payload, sizeof(payload), SL_WFX_SOFTAP_INTERFACE, NULL);
}

/**************************************************************************//**
 * @brief Configure the maximum number of clients supported in softap mode
 *
 * @param max_clients is the maximum number of clients supported in softap.
 * Broadcast address denies all MAC addresses.
 * @returns SL_SUCCESS if the setting is applied correctly, SL_ERROR otherwise
 *
 * @note sl_wfx_set_max_ap_client() has to be called after
 * sl_wfx_start_ap_command(). If the softap is stopped or the Wi-Fi chip resets,
 * the command has to be issued again.
 *****************************************************************************/
sl_status_t sl_wfx_set_max_ap_client(uint32_t max_clients)
{
  sl_wfx_set_max_ap_client_count_req_body_t payload;

  payload.count = sl_wfx_htole32(max_clients);

  return sl_wfx_send_command(SL_WFX_SET_MAX_AP_CLIENT_COUNT_REQ_ID, &payload, sizeof(payload), SL_WFX_SOFTAP_INTERFACE, NULL);
}

/**************************************************************************//**
 * @brief Configure the maximum client idle time
 *
 * @param inactivity_timeout is the maximum client idle time in seconds.
 * @returns SL_SUCCESS if the setting is applied correctly, SL_ERROR otherwise
 *
 * @note sl_wfx_set_max_ap_client() has to be called after
 * sl_wfx_start_ap_command(). If the softap is stopped or the Wi-Fi chip resets,
 * the command has to be issued again.
 *****************************************************************************/
sl_status_t sl_wfx_set_max_ap_client_inactivity(uint32_t inactivity_timeout)
{
  sl_wfx_set_max_ap_client_inactivity_req_body_t payload;

  payload.inactivity_timeout = sl_wfx_htole32(inactivity_timeout);

  return sl_wfx_send_command(SL_WFX_SET_MAX_AP_CLIENT_INACTIVITY_REQ_ID, &payload, sizeof(payload), SL_WFX_SOFTAP_INTERFACE, NULL);
}

/**************************************************************************//**
 * @brief Configure scan parameters
 *
 * @param active_channel_time is the time spent on each channel during active
 * scan. Set to 0 for FW default, 1 - 550 TUs (1 TU = 1.024 ms).
 * @param passive_channel_time: Time spent on each channel during passive scan.
 * Set to 0 for FW default, 1 - 550 TUs (1 TU = 1.024 ms).
 * @param num_probe_requests is the number of probe requests to send.
 * Set to 0 for FW default, 1 - 2
 * @returns SL_SUCCESS if the setting is applied correctly, SL_ERROR otherwise
 *
 * @note Parameters set with sl_wfx_set_scan_parameters() will apply for every
 * future scan
 *****************************************************************************/
sl_status_t sl_wfx_set_scan_parameters(uint16_t active_channel_time,
                                       uint16_t passive_channel_time,
                                       uint16_t num_probe_requests)
{
  sl_wfx_set_scan_parameters_req_body_t payload;

  payload.active_channel_time   = sl_wfx_htole16(active_channel_time);
  payload.passive_channel_time  = sl_wfx_htole16(passive_channel_time);
  payload.num_of_probe_requests = sl_wfx_htole16(num_probe_requests);
  payload.reserved              = 0;

  return sl_wfx_send_command(SL_WFX_SET_SCAN_PARAMETERS_REQ_ID, &payload, sizeof(payload), SL_WFX_STA_INTERFACE, NULL);
}

/**************************************************************************//**
 * @brief Configure the roaming behavior in STA mode
 *
 * @param rcpi_threshold is the lower RCPI limit trigerring a roaming attempt
 * @param rcpi_hysteresis is the High RCPI limit that must be attain before a
 * roaming attempt can be triggered again
 * @param beacon_lost_count is the beacon loss limit for a roaming attempt
 * @param channel_list is the list of specific channels to scan
 * @param channel_list_count is the amount of specific channels to scan
 * @returns SL_SUCCESS if the setting is applied correctly, SL_ERROR otherwise
 *
 * @note Parameters set by sl_wfx_set_roam_parameters() take effect at the next
 * connection. Calling it while connected has no immediate effect.
 *****************************************************************************/
sl_status_t sl_wfx_set_roam_parameters(uint8_t rcpi_threshold,
                                       uint8_t rcpi_hysteresis,
                                       uint8_t beacon_lost_count,
                                       const uint8_t *channel_list,
                                       uint8_t channel_list_count)
{
  sl_status_t               result          = SL_SUCCESS;
  sl_wfx_generic_message_t *frame           = NULL;
  uint32_t                  request_length  = SL_WFX_ROUND_UP_EVEN(sizeof(sl_wfx_set_roam_parameters_req_t) + channel_list_count);
  sl_wfx_set_roam_parameters_req_body_t *request = NULL;
  sl_wfx_set_roam_parameters_cnf_t      *reply   = NULL;

  result = sl_wfx_allocate_command_buffer(&frame, SL_WFX_SET_ROAM_PARAMETERS_REQ_ID, request_length, SL_WAIT_FOREVER);
  SL_WFX_ERROR_CHECK(result);

  memset(frame, 0, request_length);

  request = (sl_wfx_set_roam_parameters_req_body_t *)frame->body;

  request->rcpi_threshold     = rcpi_threshold;
  request->rcpi_hysteresis    = rcpi_hysteresis;
  request->beacon_lost_count  = beacon_lost_count;
  request->channel_list_count = channel_list_count;
  memcpy(request + 1, channel_list, channel_list_count);

  result = sl_wfx_send_request(SL_WFX_SET_ROAM_PARAMETERS_REQ_ID, frame, request_length);
  SL_WFX_ERROR_CHECK(result);

  result = sl_wfx_host_wait_for_confirmation(SL_WFX_DEFAULT_REQUEST_TIMEOUT_MS, (void **)&reply);
  SL_WFX_ERROR_CHECK(result);

  if (sl_wfx_htole32(reply->body.status) != WFM_STATUS_SUCCESS) {
    result = SL_ERROR;
  }

  error_handler:
  if (frame != NULL) {
    sl_wfx_host_free_buffer(frame, SL_WFX_CONTROL_BUFFER);
  }
  return result;
}

/**************************************************************************//**
 * @brief Configure the roaming behavior in STA mode
 *
 * @param rate_set_bitmask is the list of rates that will be used in STA mode.
 * @returns SL_SUCCESS if the setting is applied correctly, SL_ERROR otherwise
 *
 * @note Parameters set by sl_wfx_set_roam_parameters() take effect at the next
 * connection. Calling it while connected has no immediate effect.
 *****************************************************************************/
sl_status_t sl_wfx_set_tx_rate_parameters(sl_wfx_rate_set_bitmask_t rate_set_bitmask)
{
  sl_wfx_set_tx_rate_parameters_req_body_t payload;

  payload.reserved = 0;
  memcpy(&payload.rate_set_bitmask, &rate_set_bitmask, sizeof(sl_wfx_rate_set_bitmask_t));

  return sl_wfx_send_command(SL_WFX_SET_TX_RATE_PARAMETERS_REQ_ID, &payload, sizeof(payload), SL_WFX_STA_INTERFACE, NULL);
}

/**************************************************************************//**
 * @brief Set maximum transmit power
 *
 * @param max_tx_power is the maximum transmit power in units of 0.1d_bm.
 * @param interface is the interface used to send the request.
 *   @arg         SL_WFX_STA_INTERFACE
 *   @arg         SL_WFX_SOFTAP_INTERFACE
 * @returns SL_SUCCESS if the setting is applied correctly, SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_set_max_tx_power(int32_t max_tx_power, sl_wfx_interface_t interface)
{
  sl_wfx_set_max_tx_power_req_body_t payload;

  payload.max_tx_power = sl_wfx_htole32(max_tx_power);

  return sl_wfx_send_command(SL_WFX_SET_MAX_TX_POWER_REQ_ID, &payload, sizeof(payload), interface, NULL);
}

/**************************************************************************//**
 * @brief Set maximum transmit power
 *
 * @param max_tx_power_rf_port_1 returns the maximum transmit power in units of
 * 0.1d_bm, on RF port 1.
 * @param max_tx_power_rf_port_2 returns the maximum transmit power in units of
 * 0.1d_bm, on RF port 2.
 * @param interface is the interface used to send the request.
 *   @arg         SL_WFX_STA_INTERFACE
 *   @arg         SL_WFX_SOFTAP_INTERFACE
 * @returns SL_SUCCESS if the setting is applied correctly, SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_get_max_tx_power(int32_t *max_tx_power_rf_port_1,
                                    int32_t *max_tx_power_rf_port_2,
                                    sl_wfx_interface_t interface)
{
  sl_status_t     result;
  sl_wfx_get_max_tx_power_cnf_t *reply = NULL;

  result = sl_wfx_send_command(SL_WFX_GET_MAX_TX_POWER_REQ_ID, NULL, 0, interface, (sl_wfx_generic_confirmation_t **)&reply);

  if (result == SL_SUCCESS) {
    if (sl_wfx_htole32(reply->body.status) != WFM_STATUS_SUCCESS) {
      result = SL_ERROR;
    }
    *max_tx_power_rf_port_1 = sl_wfx_htole32(reply->body.max_tx_power_rf_port1);
    *max_tx_power_rf_port_2 = sl_wfx_htole32(reply->body.max_tx_power_rf_port2);
  }

  return result;
}

/** @} end FULL_MAC_DRIVER_API */

/**************************************************************************//**
 * @addtogroup GENERAL_DRIVER_API
 * @{
 *****************************************************************************/

 /**************************************************************************//**
  * @brief Function to send PDS chunks
  *
  * @param pds_data: Data to be sent in the compressed PDS format
  * @param pds_data_length: Size of the data to be sent
  * @returns SL_SUCCESS if the configuration has been sent correctly,
  * SL_ERROR otherwise
  *
  * @note The PDS (Platform Data Set) file contains the WF200 settings
  *****************************************************************************/
sl_status_t sl_wfx_send_configuration(const char *pds_data, uint32_t pds_data_length)
{
  sl_status_t                      result         = SL_SUCCESS;
  sl_wfx_generic_message_t        *frame          = NULL;
  sl_wfx_configuration_cnf_t      *reply          = NULL;
  sl_wfx_configuration_req_body_t *config_request = NULL;
  uint32_t request_length = SL_WFX_ROUND_UP_EVEN(sizeof(sl_wfx_configuration_req_t) + pds_data_length);

  result = sl_wfx_allocate_command_buffer(&frame, SL_WFX_CONFIGURATION_REQ_ID, request_length, SL_WAIT_FOREVER);
  SL_WFX_ERROR_CHECK(result);

  memset(frame, 0, request_length);

  config_request = (sl_wfx_configuration_req_body_t *)&frame->body;
  config_request->length = sl_wfx_htole16(pds_data_length);

  memcpy(config_request->pds_data, pds_data, pds_data_length);

  result = sl_wfx_send_request(SL_WFX_CONFIGURATION_REQ_ID, frame, request_length);
  SL_WFX_ERROR_CHECK(result);

  result = sl_wfx_host_wait_for_confirmation(SL_WFX_DEFAULT_REQUEST_TIMEOUT_MS, (void **)&reply);
  SL_WFX_ERROR_CHECK(result);

  if (sl_wfx_htole32(reply->body.status) != WFM_STATUS_SUCCESS) {
    result = SL_ERROR;
  }

  error_handler:
  if (frame != NULL) {
    sl_wfx_host_free_buffer(frame, SL_WFX_CONTROL_BUFFER);
  }
  return result;
}

/**************************************************************************//**
 * @brief Send a request to read or write a GPIO
 *
 * @param gpio_label is the GPIO label to control (defined in the PDS)
 * @param gpio_mode defines how to read or set the GPIO
 * @param value returns the read value or the detailed error cause
 * @returns SL_SUCCESS if the request has been sent correctly,
 * SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_control_gpio(uint8_t gpio_label, uint8_t gpio_mode, uint32_t *value)
{
  sl_status_t result;
  sl_wfx_control_gpio_cnf_t *reply = NULL;
  sl_wfx_control_gpio_req_body_t payload;

  payload.gpio_label= gpio_label;
  payload.gpio_mode= gpio_mode;

  result = sl_wfx_send_command(SL_WFX_CONTROL_GPIO_REQ_ID, &payload, sizeof(payload), SL_WFX_STA_INTERFACE, (sl_wfx_generic_confirmation_t **)&reply);

  if (result == SL_SUCCESS) {
    if (sl_wfx_htole32(reply->body.status) != WFM_STATUS_SUCCESS) {
      result = SL_ERROR;
    }
    *value = sl_wfx_htole32(reply->body.value);
  }

  return result;
}

/**************************************************************************//**
 * @brief Prevent Rollback request
 *
 * @param magic_word: Used to prevent mistakenly sent request to burn the OTP
 * @returns SL_SUCCESS if the setting is applied correctly, SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_prevent_rollback(uint32_t magic_word)
{
  sl_wfx_prevent_rollback_req_body_t payload;

  payload.magic_word= sl_wfx_htole32(magic_word);

  return sl_wfx_send_command(SL_WFX_PREVENT_ROLLBACK_REQ_ID, &payload, sizeof(payload), SL_WFX_STA_INTERFACE, NULL);
}

/**************************************************************************//**
 * @brief Shutdown the Wi-Fi chip
 *
 * @returns SL_SUCCESS if the Wi-Fi chip has been shutdown correctly,
 * SL_ERROR otherwise
 *
 * @note Send the shutdown command, clear the WUP bit and the GPIO WUP to enable
 * the Wi-Fi chips to go to sleep
 *****************************************************************************/
sl_status_t sl_wfx_shutdown(void)
{
  sl_status_t               result  = SL_SUCCESS;
  sl_wfx_generic_message_t *frame   = NULL;

  result = sl_wfx_allocate_command_buffer(&frame, SL_WFX_SHUT_DOWN_REQ_ID, sizeof(sl_wfx_shut_down_req_t), SL_WAIT_FOREVER);
  SL_WFX_ERROR_CHECK(result);

  result = sl_wfx_send_request(SL_WFX_SHUT_DOWN_REQ_ID, frame, sizeof(sl_wfx_shut_down_req_t) );
  SL_WFX_ERROR_CHECK(result);

  // Clear WUP bit in control register
  result = sl_wfx_set_wake_up_bit(0);
  SL_WFX_ERROR_CHECK(result);
  // Clear WUP pin
  result = sl_wfx_host_set_wake_up_pin(0);
  SL_WFX_ERROR_CHECK(result);

  error_handler:
  return result;
}

/** @} end GENERAL_DRIVER_API */

/**************************************************************************//**
 * @brief Send a command to WF200
 *
 * @param command_id is the ID of the command to be sent (cf. wfm_fm_api.h)
 * @param data is the pointer to the data to be sent by the command
 * @param data_size is the size of the data to be sent
 * @param interface is the interface affected by the command
 * @param response is a pointer to the response retrieved
 *   @arg         SL_WFX_STA_INTERFACE
 *   @arg         SL_WFX_SOFTAP_INTERFACE
 * @returns SL_SUCCESS if the command is sent correctly, SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_send_command(uint8_t command_id,
                                void *data,
                                uint32_t data_size,
                                sl_wfx_interface_t interface,
                                sl_wfx_generic_confirmation_t **response)
{
  sl_status_t                    result;
  sl_wfx_generic_confirmation_t *reply;
  sl_wfx_generic_message_t      *request = NULL;
  uint32_t                       request_length = SL_WFX_ROUND_UP_EVEN(sizeof(sl_wfx_header_t) + data_size);

  result = sl_wfx_allocate_command_buffer(&request, command_id, request_length, SL_WAIT_FOREVER);
  SL_WFX_ERROR_CHECK(result);

  memset(request, 0, request_length);

  request->header.info = interface;

  if (data != NULL) {
    memcpy(request->body, data, data_size);
  }

  result = sl_wfx_send_request(command_id, request, request_length);
  SL_WFX_ERROR_CHECK(result);

  result = sl_wfx_host_wait_for_confirmation(SL_WFX_DEFAULT_REQUEST_TIMEOUT_MS, (void **)&reply);
  SL_WFX_ERROR_CHECK(result);

  if (response != NULL) {
    *response = reply;
  } else {
    if (sl_wfx_htole32((uint32_t)reply->status) != WFM_STATUS_SUCCESS) {
      result = SL_ERROR;
    }
  }

  error_handler:
  if (request != NULL) {
    sl_wfx_host_free_buffer(request, SL_WFX_CONTROL_BUFFER);
  }
  return result;
}

/**************************************************************************//**
 * @brief Send a request to the Wi-Fi chip
 *
 * @param command_id is the ID of the command to be sent (cf. wfm_fm_api.h)
 * @param request is the pointer to the request to be sent
 * @param request_length is the size of the request to be sent
 * @returns SL_SUCCESS if the command is sent correctly, SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_send_request(uint8_t command_id, sl_wfx_generic_message_t *request, uint16_t request_length)
{
  sl_wfx_context->waited_event_id = command_id;
  sl_wfx_context->posted_event_id = 0;

  // Write the buffer header
  request->header.id     = command_id;
  request->header.length = sl_wfx_htole16(request_length);

  return sl_wfx_host_transmit_frame(request, request_length);
}

/**************************************************************************//**
 * @brief Receive available frame from the Wi-Fi chip
 *
 * @param ctrl_reg is the control register value of the last call of
 * sl_wfx_receive_frame(). If equal to 0, the driver will read the control
 * register.
 * @returns SL_SUCCESS if the frame has been received correctly,
 * SL_WIFI_NO_PACKET_TO_RECEIVE if no frame are pending inside the Wi-Fi chip
 * SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_receive_frame(uint16_t *ctrl_reg)
{
  sl_status_t               result;
  sl_wfx_generic_message_t *network_rx_buffer;
  sl_wfx_frame_type_t       frame_type;
  uint32_t                  read_length, frame_size;

  frame_size = (*ctrl_reg & SL_WFX_CONT_NEXT_LEN_MASK) * 2;
  /* if frame_size is equal to 0, read the control register to know the frame size */
  if (frame_size == 0) {
    /* Read the control register */
    result = sl_wfx_reg_read_16(SL_WFX_CONTROL_REG_ID, ctrl_reg);
    SL_WFX_ERROR_CHECK(result);
    frame_size = (*ctrl_reg & SL_WFX_CONT_NEXT_LEN_MASK) * 2;
    /* At this point, if frame_size is equal to zero, nothing to be read by the host */
    if (frame_size == 0) {
      result = SL_WIFI_NO_PACKET_TO_RECEIVE;
      SL_WFX_ERROR_CHECK(result);
    }
  }

  /* critical : '+SL_WFX_CTRL_REGISTER_SIZE' is to read the piggy-back value at the end of the control register. */
  read_length = frame_size + SL_WFX_CTRL_REGISTER_SIZE;

  result = sl_wfx_host_allocate_buffer((void **)&network_rx_buffer, SL_WFX_RX_FRAME_BUFFER, SL_WFX_ROUND_UP(read_length, SL_WFX_ROUND_UP_VALUE), 0);
  SL_WFX_ERROR_CHECK(result);

  memset(network_rx_buffer, 0, read_length);

  /* Read the frame from WF200 */
  result = sl_wfx_data_read(network_rx_buffer, read_length);
  SL_WFX_ERROR_CHECK(result);

  /* retrieve the frame type from the control register*/
  frame_type = ((*ctrl_reg & SL_WFX_CONT_FRAME_TYPE_INFO) >> SL_WFX_CONT_FRAME_TYPE_OFFSET);
  /* read the control register value in the piggy back and pass it to the host */
  *ctrl_reg = sl_wfx_unpack_16bit_little_endian(((uint8_t *)network_rx_buffer) + frame_size);

  network_rx_buffer->header.length = sl_wfx_htole16(network_rx_buffer->header.length);

  /* send the information to the host */
  result = sl_wfx_host_post_event(frame_type,
                                  network_rx_buffer);

  error_handler:
  return result;
}

/**************************************************************************//**
 * @brief Enable the Wi-Fi chip irq
 *
 * @return SL_SUCCESS if the irq is enabled correctly, SL_ERROR otherwise
 *
 * @note Enable the host irq and set the Wi-Fi chip register accordingly
 *****************************************************************************/
sl_status_t sl_wfx_enable_irq(void)
{
  uint32_t    value32;
  sl_status_t result;

  result = sl_wfx_host_enable_platform_interrupt();
  SL_WFX_ERROR_CHECK(result);

  result = sl_wfx_reg_read_32(SL_WFX_CONFIG_REG_ID, &value32);
  SL_WFX_ERROR_CHECK(result);

  value32 |= SL_WFX_CONF_IRQ_RDY_ENABLE;

  result = sl_wfx_reg_write_32(SL_WFX_CONFIG_REG_ID, value32);
  SL_WFX_ERROR_CHECK(result);

  error_handler:
  return result;
}

/**************************************************************************//**
 * @brief Disable the Wi-Fi chip irq
 *
 * @return SL_SUCCESS if the irq is disabled correctly, SL_ERROR otherwise
 *
 * @note Disable the host irq and set the Wi-Fi chip register accordingly
 *****************************************************************************/
sl_status_t sl_wfx_disable_irq(void)
{
  uint32_t    value32;
  sl_status_t result;

  result = sl_wfx_reg_read_32(SL_WFX_CONFIG_REG_ID, &value32);
  SL_WFX_ERROR_CHECK(result);

  value32 &= ~SL_WFX_CONF_IRQ_RDY_ENABLE;

  result = sl_wfx_reg_write_32(SL_WFX_CONFIG_REG_ID, value32);
  SL_WFX_ERROR_CHECK(result);

  result = sl_wfx_host_disable_platform_interrupt();
  SL_WFX_ERROR_CHECK(result);

  error_handler:
  return result;
}

/**************************************************************************//**
 * @brief Set access mode message
 *
 * @return SL_SUCCESS if the message mode is enabled correctly,
 * SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_set_access_mode_message(void)
{
  /* Configure device for MESSSAGE MODE */
  sl_status_t result;
  uint32_t    val32;

  result = sl_wfx_reg_read_32(SL_WFX_CONFIG_REG_ID, &val32);
  SL_WFX_ERROR_CHECK(result);

  result = sl_wfx_reg_write_32(SL_WFX_CONFIG_REG_ID, val32 & ~SL_WFX_CONFIG_ACCESS_MODE_BIT);
  SL_WFX_ERROR_CHECK(result);

  error_handler:
  return result;
}

/**************************************************************************//**
 * @brief Set the Wi-Fi chip wake up bit
 *
 * @param state is the state of the wake up bit to configure
 * @return SL_SUCCESS if the bit has been set correctly, SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_set_wake_up_bit(uint8_t state)
{
  sl_status_t status;
  uint16_t    control_register_value;

  // Reading doesnt work, test later
  status = sl_wfx_reg_read_16(SL_WFX_CONTROL_REG_ID, &control_register_value);
  SL_WFX_ERROR_CHECK(status);

  if (state > 0) {
    control_register_value |= SL_WFX_CONT_WUP_BIT;
  } else {
    control_register_value &= ~SL_WFX_CONT_WUP_BIT;
  }

  status = sl_wfx_reg_write_16(SL_WFX_CONTROL_REG_ID, control_register_value);

  error_handler: return status;
}

/******************************************************
*                 Static Functions
******************************************************/

/**************************************************************************//**
 * @brief Init the Wi-Fi chip
 *
 * @return SL_SUCCESS if the initialization is successful, SL_ERROR otherwise
 *****************************************************************************/
static sl_status_t sl_wfx_init_chip(void)
{
  sl_status_t status;
  uint32_t    value32;
  uint16_t    value16;

  status = sl_wfx_reg_read_32(SL_WFX_CONFIG_REG_ID, &value32);
  SL_WFX_ERROR_CHECK(status);

  /* General purpose registers setting */
  status = sl_wfx_reg_write_32(SL_WFX_TSET_GEN_R_W_REG_ID, 0x07208775);
  SL_WFX_ERROR_CHECK(status);
  status = sl_wfx_reg_write_32(SL_WFX_TSET_GEN_R_W_REG_ID, 0x082ec020);
  SL_WFX_ERROR_CHECK(status);
  status = sl_wfx_reg_write_32(SL_WFX_TSET_GEN_R_W_REG_ID, 0x093c3c3c);
  SL_WFX_ERROR_CHECK(status);
  status = sl_wfx_reg_write_32(SL_WFX_TSET_GEN_R_W_REG_ID, 0x0b322c44);
  SL_WFX_ERROR_CHECK(status);
  status = sl_wfx_reg_write_32(SL_WFX_TSET_GEN_R_W_REG_ID, 0x0ca06497);
  SL_WFX_ERROR_CHECK(status);

  /* set wake-up bit */
  status = sl_wfx_reg_read_16(SL_WFX_CONTROL_REG_ID, &value16);
  SL_WFX_ERROR_CHECK(status);
  value16 |= SL_WFX_CONT_WUP_BIT;
  status = sl_wfx_reg_write_16(SL_WFX_CONTROL_REG_ID, value16);
  SL_WFX_ERROR_CHECK(status);

  /* .. and wait for wake-up */
  for (uint32_t i = 0; i < 200; ++i) {
    status = sl_wfx_reg_read_16(SL_WFX_CONTROL_REG_ID, &value16);
    SL_WFX_ERROR_CHECK(status);

    if ((value16 & SL_WFX_CONT_RDY_BIT) == SL_WFX_CONT_RDY_BIT) {
      break;
    } else {
      sl_wfx_host_wait(1);
    }
  }

  if ((value16 & SL_WFX_CONT_RDY_BIT) != SL_WFX_CONT_RDY_BIT) {
    status = SL_TIMEOUT;
    SL_WFX_ERROR_CHECK(status);
  }

  /* check for access mode bit */
  status = sl_wfx_reg_read_32(SL_WFX_CONFIG_REG_ID, &value32);
  SL_WFX_ERROR_CHECK(status);
  if ((value32 & SL_WFX_CONFIG_ACCESS_MODE_BIT) == 0) {
    status = SL_ERROR;
    SL_WFX_ERROR_CHECK(status);
  }

  error_handler:
  return status;
}

/**************************************************************************//**
 * @brief run the Wi-Fi chip bootloader
 *
 * @return SL_SUCCESS if the bootloader runs correctly, SL_ERROR otherwise
 *****************************************************************************/
static sl_status_t sl_wfx_download_run_bootloader(void)
{
  sl_status_t status;
  uint32_t    value32;

  status = sl_wfx_apb_read_32(ADDR_DWL_CTRL_AREA_NCP_STATUS, &value32);
  SL_WFX_ERROR_CHECK(status);

  /* release CPU from reset and enable clock */
  status = sl_wfx_reg_read_32(SL_WFX_CONFIG_REG_ID, &value32);
  SL_WFX_ERROR_CHECK(status);
  value32 &= ~(SL_WFX_CONFIG_CPU_RESET_BIT | SL_WFX_CONFIG_CPU_CLK_DIS_BIT);
  status = sl_wfx_reg_write_32(SL_WFX_CONFIG_REG_ID, value32);
  SL_WFX_ERROR_CHECK(status);

  /* Testing SRAM access */
  status = sl_wfx_apb_write_32(ADDR_DOWNLOAD_FIFO_BASE, 0x23abc88e);
  SL_WFX_ERROR_CHECK(status);

  /* Check if the write command is successful */
  status = sl_wfx_apb_read_32(ADDR_DOWNLOAD_FIFO_BASE, &value32);
  SL_WFX_ERROR_CHECK(status);
  if (value32 != 0x23abc88e) {
    status = SL_ERROR;
    SL_WFX_ERROR_CHECK(status);
  }

  error_handler:
  return status;
}

/**************************************************************************//**
 * @brief Download the Wi-Fi chip firmware
 *
 * @return SL_SUCCESS if the firmware is downloaded correctly,
 * SL_WIFI_INVALID_KEY if the firmware keyset does not match the chip one,
 * SL_WIFI_FIRMWARE_DOWNLOAD_TIMEOUT or SL_TIMEOUT if the process times out,
 * SL_ERROR otherwise
 *****************************************************************************/
static sl_status_t sl_wfx_download_run_firmware(void)
{
  sl_status_t    status;
  uint32_t       i;
  uint32_t       value32;
  uint32_t       image_length;
  uint32_t       block;
  uint32_t       num_blocks;
  uint32_t       put = 0;
  uint32_t       get = 0;
  const uint8_t *buffer;

  status = sl_wfx_host_init();
  SL_WFX_ERROR_CHECK(status);

  status = sl_wfx_apb_write_32(ADDR_DWL_CTRL_AREA_HOST_STATUS, HOST_STATE_NOT_READY);
  SL_WFX_ERROR_CHECK(status);
  status = sl_wfx_apb_write_32(ADDR_DWL_CTRL_AREA_PUT, 0);
  SL_WFX_ERROR_CHECK(status);
  status = sl_wfx_apb_write_32(ADDR_DWL_CTRL_AREA_GET, 0);
  SL_WFX_ERROR_CHECK(status);
  status = sl_wfx_apb_write_32(ADDR_DWL_CTRL_AREA_HOST_STATUS, HOST_STATE_READY);
  SL_WFX_ERROR_CHECK(status);

  // wait for INFO_READ state
  status = sl_wfx_poll_for_value(ADDR_DWL_CTRL_AREA_NCP_STATUS, NCP_STATE_INFO_READY, 100);
  SL_WFX_ERROR_CHECK(status);

  // read info
  status = sl_wfx_apb_read_32(0x0900C080, &value32);
  SL_WFX_ERROR_CHECK(status);

  // retrieve WF200 keyset
  status = sl_wfx_apb_read_32(WFX_PTE_INFO + 12, &value32);
  SL_WFX_ERROR_CHECK(status);
  encryption_keyset = (value32 >> 8);

  // report that info is read
  status = sl_wfx_apb_write_32(ADDR_DWL_CTRL_AREA_HOST_STATUS, HOST_STATE_HOST_INFO_READ);
  SL_WFX_ERROR_CHECK(status);

  // wait for READY state
  status = sl_wfx_poll_for_value(ADDR_DWL_CTRL_AREA_NCP_STATUS, NCP_STATE_READY, 100);
  SL_WFX_ERROR_CHECK(status);

  // SB misc initialization. Work around for chips < A2.
  status = sl_wfx_apb_write_32(ADDR_DOWNLOAD_FIFO_BASE, 0xFFFFFFFF);
  SL_WFX_ERROR_CHECK(status);

  // write image length
  sl_wfx_host_get_firmware_size(&image_length);
  status = sl_wfx_apb_write_32(ADDR_DWL_CTRL_AREA_IMAGE_SIZE, image_length - FW_HASH_SIZE - FW_SIGNATURE_SIZE - FW_KEYSET_SIZE);
  SL_WFX_ERROR_CHECK(status);

  // get firmware keyset, which is the first FW_KEYSET_SIZE of given image
  status = sl_wfx_host_get_firmware_data(&buffer, FW_KEYSET_SIZE);
  SL_WFX_ERROR_CHECK(status);

  // check if the firmware keyset corresponds to the chip keyset
  status = sl_wfx_compare_keysets(encryption_keyset, (char *)buffer);
  SL_WFX_ERROR_CHECK(status);

  // write image signature, which is the next FW_SIGNATURE_SIZE of given image
  status = sl_wfx_host_get_firmware_data(&buffer, FW_SIGNATURE_SIZE);
  SL_WFX_ERROR_CHECK(status);
  status = sl_wfx_apb_write(ADDR_DWL_CTRL_AREA_SIGNATURE, buffer, FW_SIGNATURE_SIZE);
  SL_WFX_ERROR_CHECK(status);

  // write image hash, which is the next  FW_HASH_SIZE of given image
  status = sl_wfx_host_get_firmware_data(&buffer, FW_HASH_SIZE);
  SL_WFX_ERROR_CHECK(status);
  status = sl_wfx_apb_write(ADDR_DWL_CTRL_AREA_FW_HASH, buffer, FW_HASH_SIZE);
  SL_WFX_ERROR_CHECK(status);

  // write version, this is a pre-defined value (?)
  status = sl_wfx_apb_write_32(ADDR_DWL_CTRL_AREA_FW_VERSION, FW_VERSION_VALUE);
  SL_WFX_ERROR_CHECK(status);

  // notify NCP that upload is starting
  status = sl_wfx_apb_write_32(ADDR_DWL_CTRL_AREA_HOST_STATUS, HOST_STATE_UPLOAD_PENDING);
  SL_WFX_ERROR_CHECK(status);

  // skip signature and hash from image length
  image_length -= (FW_HASH_SIZE + FW_SIGNATURE_SIZE + FW_KEYSET_SIZE);

  /* Calculate number of download blocks */
  num_blocks = (image_length - 1) / DOWNLOAD_BLOCK_SIZE + 1;

  /* Firmware downloading loop */
  for ( block = 0; block < num_blocks; block++ ) {
    /* check the download status in NCP */
    status = sl_wfx_apb_read_32(ADDR_DWL_CTRL_AREA_NCP_STATUS, &value32);
    SL_WFX_ERROR_CHECK(status);

    if (value32 != NCP_STATE_DOWNLOAD_PENDING) {
      status = SL_ERROR;
      SL_WFX_ERROR_CHECK(status);
    }

    /* loop until put - get <= 24K */
    for ( i = 0; i < 100; i++ ) {
      get = 0;
      status = sl_wfx_apb_read_32(ADDR_DWL_CTRL_AREA_GET, &get);
      SL_WFX_ERROR_CHECK(status);

      if ((put - get) <= (DOWNLOAD_FIFO_SIZE - DOWNLOAD_BLOCK_SIZE)) {
        break;
      }
    }

    if ((put - get) > (DOWNLOAD_FIFO_SIZE - DOWNLOAD_BLOCK_SIZE)) {
      status = SL_WIFI_FIRMWARE_DOWNLOAD_TIMEOUT;
      SL_WFX_ERROR_CHECK(status);
    }

    /* calculate the block size */
    uint32_t block_size = image_length - put;
    if (block_size > DOWNLOAD_BLOCK_SIZE) {
      block_size = DOWNLOAD_BLOCK_SIZE;
    }

    /* send the block to SRAM */
    status = sl_wfx_host_get_firmware_data(&buffer, block_size);
    SL_WFX_ERROR_CHECK(status);
    uint32_t block_address = ADDR_DOWNLOAD_FIFO_BASE + (put % DOWNLOAD_FIFO_SIZE);
    status = sl_wfx_apb_write(block_address, buffer, block_size);
    SL_WFX_ERROR_CHECK(status);

    /* update the put register */
    put += block_size;

    status = sl_wfx_apb_write_32(ADDR_DWL_CTRL_AREA_PUT, put);
    SL_WFX_ERROR_CHECK(status);
  }   /* End of firmware download loop */

  // notify NCP that upload ended
  status = sl_wfx_apb_write_32(ADDR_DWL_CTRL_AREA_HOST_STATUS, HOST_STATE_UPLOAD_COMPLETE);
  SL_WFX_ERROR_CHECK(status);

  // wait for authentication result
  status = sl_wfx_poll_for_value(ADDR_DWL_CTRL_AREA_NCP_STATUS, NCP_STATE_AUTH_OK, 100);
  SL_WFX_ERROR_CHECK(status);

  // notify NCP that we are happy to run firmware
  status = sl_wfx_apb_write_32(ADDR_DWL_CTRL_AREA_HOST_STATUS, HOST_STATE_OK_TO_JUMP);
  SL_WFX_ERROR_CHECK(status);

  error_handler:
  sl_wfx_host_deinit();

  return status;
}

/**************************************************************************//**
 * @brief Poll a value from the Wi-Fi chip
 *
 * @param address is the address of the value to be polled
 * @param polled_value waiting for the value to be equal to polled_value
 * @param max_retries is the number of polling to be done before returning
 * SL_TIMEOUT
 * @return SL_SUCCESS if the value is received correctly,
 * SL_TIMEOUT if the value is not found in time,
 * SL_ERROR if not able to poll the value from the Wi-Fi chip
 *****************************************************************************/
static sl_status_t sl_wfx_poll_for_value(uint32_t address, uint32_t polled_value, uint32_t max_retries)
{
  uint32_t    value;
  sl_status_t status = SL_SUCCESS;

  for (; max_retries > 0; max_retries--) {
    status = sl_wfx_apb_read_32(address, &value);
    SL_WFX_ERROR_CHECK(status);
    if (value == polled_value) {
      break;
    } else {
      sl_wfx_host_wait(1);
    }
  }
  if (value != polled_value) {
    status = SL_TIMEOUT;
  }

  error_handler:
  return status;
}

/**************************************************************************//**
 * @brief Compare the chip keyset and the firmware one to check compatibility
 *
 * @param chip_keyset is the value retrieved from the Wi-Fi chip
 * @param firmware_keyset is the 8 first bytes of the firmware
 * @return SL_SUCCESS if the firmware is compatible with the WF200,
 * SL_WIFI_INVALID_KEY otherwise
 *****************************************************************************/
static sl_status_t sl_wfx_compare_keysets(uint8_t chip_keyset, char *firmware_keyset)
{
  sl_status_t result;
  char        keyset_string[3];
  uint8_t     keyset_value;

  keyset_string[0] = *(firmware_keyset + 6);
  keyset_string[1] = *(firmware_keyset + 7);
  keyset_string[2] = '\0';
  keyset_value     = (uint8_t)strtoul(keyset_string, NULL, 16);

  if (keyset_value == chip_keyset) {
    result = SL_SUCCESS;
  } else {
    result = SL_WIFI_INVALID_KEY;
  }
  return result;
}

/**************************************************************************//**
 * @brief Configure the antenna setting (done through the PDS)
 *
 * @param config is the antenna configuration to be used.
 *   @arg         SL_WFX_ANTENNA_1_ONLY
 *   @arg         SL_WFX_ANTENNA_2_ONLY
 *   @arg         SL_WFX_ANTENNA_TX1_RX2
 *   @arg         SL_WFX_ANTENNA_TX2_RX1
 *   @arg         SL_WFX_ANTENNA_DIVERSITY
 * @return SL_SUCCESS if the setting is applied correctly, SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_set_antenna_config(sl_wfx_antenna_config_t config)
{
  sl_status_t result;
  char        pds[32] = { 0 };
  char       *current = pds;

  current += sprintf(current, "{%c:{", PDS_ANTENNA_SEL_KEY);
  current += sprintf(current, "%c:%X,", PDS_KEY_A, (unsigned int) config);
  if (config == SL_WFX_ANTENNA_DIVERSITY) {
    // Set diversity mode internal, the Wi-Fi chip will control antenna allocation
    current += sprintf(current, "%c:%X}}", PDS_KEY_B, 1);
  } else {
    current += sprintf(current, "%c:%X}}", PDS_KEY_B, 0);
  }

  result = sl_wfx_send_configuration((const char *)pds, strlen(pds));
  SL_WFX_ERROR_CHECK(result);

  error_handler:
  return result;
}

/**************************************************************************//**
 * @brief Retrieve the hardware version and type
 *
 * @param revision is the pointer to retrieve the revision version
 * @param type is the pointer to retrieve the type
 * @return SL_SUCCESS if the values are retrieved correctly, SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_get_hardware_revision_and_type(uint8_t *revision, uint8_t *type)
{
  uint32_t config_reg = 0;
  sl_status_t status  = SL_SUCCESS;

  status = sl_wfx_reg_read_32(SL_WFX_CONFIG_REG_ID, &config_reg);

  *type = (config_reg >> SL_WFX_CONFIG_TYPE_OFFSET) & SL_WFX_CONFIG_TYPE_MASK;
  *revision = (config_reg >> SL_WFX_CONFIG_REVISION_OFFSET) & SL_WFX_CONFIG_REVISION_MASK;
  return status;
}

/**************************************************************************//**
 * @brief Get the part opn
 *
 * @param opn
 * @return SL_SUCCESS if the values are retrieved correctly, SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_get_opn(uint8_t **opn)
{
  sl_status_t status = SL_ERROR;

  if (sl_wfx_context != NULL) {
    *opn = (uint8_t *) &(sl_wfx_context->ineo_opn);
    status = SL_SUCCESS;
  }

  return status;
}

/**************************************************************************//**
 * @brief Allocate a buffer for the Wi-Fi driver
 *
 * @param buffer
 * @param command_id is the ID of the command to check if encryption is required
 * @param buffer_size is the size of the buffer to allocate
 * @param wait_duration_ms is the duration before returning SL_TIMEOUT
 * @return SL_SUCCESS if the values are retrieved correctly,
 * SL_TIMEOUT if the buffer is not allocated in time, SL_ERROR otherwise
 *****************************************************************************/
sl_status_t sl_wfx_allocate_command_buffer(sl_wfx_generic_message_t **buffer, uint32_t command_id, uint32_t buffer_size, uint32_t wait_duration_ms)
{
  SL_WFX_UNUSED_PARAMETER(command_id);
  return sl_wfx_host_allocate_buffer((void **)buffer,
                                     SL_WFX_CONTROL_BUFFER,
                                     buffer_size + SL_WFX_SECURE_LINK_HEADER_SIZE,
                                     wait_duration_ms);
}

/** @} end DRIVER_API */
