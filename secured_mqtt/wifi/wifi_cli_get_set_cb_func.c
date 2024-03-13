/**************************************************************************//**
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
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
#include "wifi_cli_get_set_cb_func.h"
#include "dhcp_client.h"
#include "dhcp_server.h"
#include "ethernetif.h"
#include "app_wifi_events.h"
#include "wifi_cli_params.h"
#include "wifi_cli_lwip.h"
#include "ports/includes.h"
#include "utils/common.h"
#include "crypto/crypto.h"
#include "sl_wfx_sae.h"

/* Store the information if the station's security mode is switched to WPA2 in WPA2/WPA3 transition mode */
bool secur_mode_fallback;

/***************************************************************************//**
 * @brief
 *    This function check the input string if "ON/On/on", state is set to 1;
 *    otherwise if "OFF/Off/off", state is set to 0
 *
 * @param[in]
 *        + input_str : "ON/On/on" or "OFF/Off/off"
 *
 * @param[out]
 *        + state : 1 if "on/ON/On"
 *                  0 if "off/OFF/Off"
 * @return
 *    0 if successful
 *    -1 if failed
 ******************************************************************************/
static int get_on_off_state(char *in_str, uint8_t *out_state)
{
  int ret = -1;
  if (in_str != NULL) {
      convert_to_lower_case_string(in_str);
      if (strcmp(in_str, "on") == 0) {
          *out_state = 0;
          ret = 0;
      } else if (strcmp(in_str, "off") == 0) {
          *out_state = 1;
          ret = 0;
      }
  }
  return ret;
}

/***************************************************************************//**
 * @brief
 *    This common function invokes the registered set function
 *
 * @param[in]
 *
 * @param[out] None
 *
 * @return  None
 ******************************************************************************/
static void set_wifi_param_common(sl_cli_command_arg_t *args)
{
  char *val_ptr = NULL;
  char *param_name = NULL;
  int param_idx;
  uint8_t argc;
  char *security_wpa3_pmksa_config_ex = "Examples: wifi set station.security WPA3\r\n"
                                        "          wifi set station.security WPA3 -pmksa\r\n";

  /* Get input argument string */
  val_ptr = sl_cli_get_argument_string(args, 0);

  /* Search the global param index with param name */
  param_name = sl_cli_get_command_string(args, 2);

  /* Number of arguments only excluding commands */
  argc = sl_cli_get_argument_count(args);

  /* Process the set command for station.security modes WPA3 and WPA2/WPA3 */
  if (!strcmp(param_name, "station.security"))
  {
      if (!strcmp(val_ptr, "WPA3") || !strcmp(val_ptr, "WPA2/WPA3"))
      {
          printf("NOTICE: If you want to select the \"station.security\" as \"WPA3\" or \"WPA2/WPA3\", "
          "please refer to the README.md file stored in "
          "\"wfx-fullMAC-tools/wpa_supplicant-2.7/wpa3_sae_resources\"\r\n");
          /* Check for PMKSA caching option */
          if (argc > 1)
          {
              if (!strcmp(sl_cli_get_argument_string(args, 1), "-pmksa"))
              {
                  /* Enable PMKSA caching feature */
                  wlan_security_wpa3_pmksa = true;
                  printf("Applying PMKSA caching feature\r\n");
              }
              else
              {
                  /* Unknown option! */
                  printf("Invalid input!\r\n%s\r\n", security_wpa3_pmksa_config_ex);
              }
          }
          /* The pmksa option is not specified */
          else
          {
              /* Clear the PMK cache */
              sl_wfx_sae_invalidate_pmksa();
              /* Disable PMKSA caching feature */
              wlan_security_wpa3_pmksa = false;
          }
      }
      /* A security mode where neither WPA3 nor WPA2/WPA3 is selected */
      else
      {
          /* Disable PMKSA caching feature */
          wlan_security_wpa3_pmksa = false;
      }
  }

  param_idx = param_search(param_name);
  if (param_idx < 0) {
      LOG_DEBUG("The %s parameter hasn't been registered", param_name);
      printf("Failed to set %s parameter\r\n", param_name);
      return;
  }

  /* Invoke the registered set callback function */
  wifi_params[param_idx].set_func(wifi_params[param_idx].name,
                                  wifi_params[param_idx].address,
                                  wifi_params[param_idx].size,
                                  val_ptr);
}

/***************************************************************************//**
 * @brief
 *    This common function invokes the registered get function
 *
 * @param[in]
 *
 * @param[out] None
 *
 * @return None
 ******************************************************************************/
static void get_wifi_param_common(sl_cli_command_arg_t *args)
{
  /* Search the global param index with param name */
  char *param_name = sl_cli_get_command_string(args, 2);
  int param_idx = param_search(param_name);
  if (param_idx < 0) {
      LOG_DEBUG("The %s parameter hasn't been registered", param_name);
      printf("Failed to get the %s parameter\r\n", param_name);
      return;
  }

  /* Invoke the registered get callback function */
  wifi_params[param_idx].get_func(wifi_params[param_idx].name,
                                  wifi_params[param_idx].address,
                                  wifi_params[param_idx].size,
                                  NULL,
                                  0);
}

/***************************************************************************//**
 * @brief
 *    This function retrieves DHCP server/client states based on the input type
 *
 * @param[in]
 *        + args: argument with sl_cli_command_arg_t type of CLI module
 *        + dhcp_type: DHCP_SERVER or DHCP_CLIENT
 *
 * @param[out] None
 *
 * @return None
 ******************************************************************************/
static void get_dhcp_state(sl_cli_command_arg_t *args, dhcp_type_et dhcp_type)
{
  char *cmd = sl_cli_get_command_string(args, 2);
  int param_idx = param_search(cmd);
  if (param_idx >= 0) {
      printf("DHCP %s state: %s\r\n",
             dhcp_type == DHCP_SERVER ? "server" : "client",
             *(uint8_t*)wifi_params[param_idx].address == 0 ? "OFF" : "ON");
  }
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get wifi state.
 *****************************************************************************/
void get_wifi_state(sl_cli_command_arg_t *args)
{
  (void)args;
  char tmp_buf[BUF_LEN] = {0}; /*!< Make sure BUF_LEN is large enough */

  if (!(wifi.state & SL_WFX_STARTED)) {
      printf("WiFi chip not started\r\n");
      return;
  }
  snprintf(tmp_buf,
           BUF_LEN,
           "WiFi chip started:\r\n\t"
           "STA: %sconnected\r\n\t"
           "AP:  %sstarted\r\n\t"
           "PS:  %sactive",
           wifi.state & SL_WFX_STA_INTERFACE_CONNECTED ? "" : "not ",
           wifi.state & SL_WFX_AP_INTERFACE_UP ? "" : "not ",
           wifi.state & SL_WFX_POWER_SAVE_ACTIVE ? "" : "in");

  if (wifi.state & SL_WFX_POWER_SAVE_ACTIVE) {
      strcat(tmp_buf, wifi.state & SL_WFX_SLEEPING ? " (sleeping)" : " (awake)");
  }
  printf("%s\r\n", tmp_buf);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get station SSID.
 *****************************************************************************/
void get_station_ssid(sl_cli_command_arg_t *args)
{
  /* Call the common function to get parameter*/
  get_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: set station SSID.
 *****************************************************************************/
void set_station_ssid(sl_cli_command_arg_t *args)
{
  /* Call the common function to set parameter */
  set_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get station Passkey.
 *****************************************************************************/
void get_station_passkey(sl_cli_command_arg_t *args)
{
  /* Call the common function to get parameter*/
  get_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: set station Passkey.
 *****************************************************************************/
void set_station_passkey(sl_cli_command_arg_t *args)
{
  /* Call the common function to set parameter */
  set_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get station security.
 *****************************************************************************/
void get_station_security(sl_cli_command_arg_t *args)
{
  /* Call the common function to get parameter*/
  get_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: set station security.
 *****************************************************************************/
void set_station_security(sl_cli_command_arg_t *args)
{
  /* Call the common function to set parameter */
  set_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get station DHCP client state.
 *****************************************************************************/
void get_station_dhcp_client_state(sl_cli_command_arg_t *args)
{
  get_dhcp_state(args, DHCP_CLIENT);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: set station DHCP client state.
 *****************************************************************************/
void set_station_dhcp_client_state(sl_cli_command_arg_t *args)
{
  int param_idx;
  char *cmd = NULL;
  uint8_t new_state;
  int station_netif_idx;
  char *state_ptr = NULL;
  struct netif *station_netif = NULL;
  ip_addr_t sta_ipaddr, sta_netmask, sta_gw;

  /* Get input string */
  state_ptr = sl_cli_get_argument_string(args, 0);

  if (get_on_off_state(state_ptr, &new_state) < 0) {
      printf("Invalid argument.\n"
             "Please input OFF for Disable; ON for Enable\r\n");
      return;
  }

  /* Find and point to the station network interface struct */
  station_netif_idx = param_search("station.ip");
  if (station_netif_idx < 0) {
      LOG_DEBUG("The sta_netif parameter hasn't been registered");
      goto error;
  }

  /* Pointer to sta_netif struct */
  station_netif = wifi_params[station_netif_idx].address;

  /** To limit undefined behaviors and ease the development
  *   only accept a DHCP state change while the WLAN interface is down.
  */
  if (wifi.state & SL_WFX_STA_INTERFACE_CONNECTED) {
      printf("Operation denied: stop WLAN first\r\n");
      return;
  }

  /* Retrieve use_dhcp_client param */
  cmd = sl_cli_get_command_string(args, 2);
  param_idx = param_search(cmd);

  if (param_idx < 0) {
      LOG_DEBUG("The use_dhcp_client parameter hasn't been registered");
      goto error;
  }

  /* Check if the new_state differs from the old state */
  if (*(uint8_t*)wifi_params[param_idx].address == new_state) {
      printf("Station client DHCP state is already set to %s\r\n",
             new_state == 0 ? "OFF" : "ON");
      return;
  }

  /* Update the DHCP client state (use_dhcp_client) */
  *(uint8_t*)wifi_params[param_idx].address = new_state;

  if (new_state == 0) {
      /* Disable the DHCP requests*/
      dhcpclient_set_link_state(0);

      /* Restore default static addresses*/
      IP_ADDR4(&sta_ipaddr,
               sta_ip_addr0,
               sta_ip_addr1,
               sta_ip_addr2,
               sta_ip_addr3);

      IP_ADDR4(&sta_netmask,
               sta_netmask_addr0,
               sta_netmask_addr1,
               sta_netmask_addr2,
               sta_netmask_addr3);

      IP_ADDR4(&sta_gw,
               sta_gw_addr0,
               sta_gw_addr1,
               sta_gw_addr2,
               sta_gw_addr3);

      netif_set_addr(station_netif, &sta_ipaddr, &sta_netmask, &sta_gw);
  } else {
      /* Clear current address*/
      netif_set_addr(station_netif, NULL, NULL, NULL);
      /* Let the connect event enable the DHCP requests*/
  }
  return; /* success */

error:
  printf("Command error\r\n");
  return; /* Failed */
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get station netmask.
 *****************************************************************************/
void get_station_netmask(sl_cli_command_arg_t *args)
{

  if (!(wifi.state & SL_WFX_STA_INTERFACE_CONNECTED)) {
      printf("Station is not connected to an AP\r\n");
      return;
  }
  /* Call the common function to get parameter */
  get_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: set station netmask.
 *****************************************************************************/
void set_station_netmask(sl_cli_command_arg_t *args)
{
  /* Call the common function to set parameter */
  set_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get station gateway.
 *****************************************************************************/
void get_station_gateway(sl_cli_command_arg_t *args)
{

  if (!(wifi.state & SL_WFX_STA_INTERFACE_CONNECTED)) {
      printf("Station is not connected to an AP\r\n");
      return;
  }
  /* Call the common function to get parameter */
  get_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: set station gateway.
 *****************************************************************************/
void set_station_gateway(sl_cli_command_arg_t *args)
{
  /* Call the common function to set parameter */
  set_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get station IP address.
 *****************************************************************************/
void get_station_ip(sl_cli_command_arg_t *args)
{
  if (!(wifi.state & SL_WFX_STA_INTERFACE_CONNECTED)) {
      printf("Station is not connected to an AP\r\n");
      return;
  }
  /* Call the common function to get parameter */
  get_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: set station IP address.
 *****************************************************************************/
void set_station_ip(sl_cli_command_arg_t *args)
{
  /* Call the common function to set parameter */
  set_wifi_param_common(args);
}
/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get station mac address (EUI-48 format).
 *****************************************************************************/
void get_station_mac(sl_cli_command_arg_t *args)
{
  char *cmd = NULL;
  int param_idx = -1;

  if (!(wifi.state & SL_WFX_STA_INTERFACE_CONNECTED)) {
     printf("Station is not connected to an AP\r\n");
     return;
  }

  cmd = sl_cli_get_command_string(args, 2);
  param_idx = param_search(cmd);
  if (param_idx < 0) {
      LOG_DEBUG("The wifi parameter hasn't been registered");
      printf("Command error\r\n");
      return; /* failed */
  }

  uint8_t *station_mac_ptr = (uint8_t *)wifi_params[param_idx].address;
  /* Convert to octet */
  printf("%02X:%02X:%02X:%02X:%02X:%02X\r\n", station_mac_ptr[0], \
                                               station_mac_ptr[1], \
                                               station_mac_ptr[2], \
                                               station_mac_ptr[3], \
                                               station_mac_ptr[4], \
                                               station_mac_ptr[5]);
 }

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: set station mac address (EUI-48 format).
 *****************************************************************************/
void set_station_mac(sl_cli_command_arg_t *args)
{
  /* Call the common function to set parameter */
  set_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: Reset the host CPU.
 *****************************************************************************/
void reset_host_cpu(sl_cli_command_arg_t *args)
{
  (void)args;

  printf("The host CPU reset\r\n");
  __DSB();
  SCB->AIRCR = (uint32_t)( (0x5FAUL << SCB_AIRCR_VECTKEY_Pos)
                            |(SCB->AIRCR & SCB_AIRCR_PRIGROUP_Msk)
                            | SCB_AIRCR_SYSRESETREQ_Msk);
  // Ensure completion of memory access
  __DSB();

  // Wait until reset
  for (;;) {
    __NOP();
  }
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: Reboot the Wi-Fi chip.
 *****************************************************************************/
void wifi_init(sl_cli_command_arg_t *args)
{
  (void)args;

  char *status_msg;
  char *error_msg;
  sl_status_t status;
  char success_msg[] = "WF(M)200 initialized successful";
  char failure_msg[] = "Failed to initialized WF(M)200: ";

  /* Force the bus de-initialization to ensure a re-initialization from scratch.
     This is especially useful for the SDIO */
  sl_wfx_host_deinit_bus();

  /* Initialize the Wi-Fi chip */
  status = sl_wfx_init(&wifi);
  switch (status) {
    case SL_STATUS_OK:
      status_msg = success_msg;
      error_msg = "";
      break;

    case SL_STATUS_WIFI_INVALID_KEY:
      status_msg = failure_msg;
      error_msg = "Firmware keyset invalid";
      break;

    case SL_STATUS_WIFI_FIRMWARE_DOWNLOAD_TIMEOUT:
      status_msg = failure_msg;
      error_msg = "Firmware download timeout";
      break;

    case SL_STATUS_TIMEOUT:
      status_msg = failure_msg;
      error_msg = "Poll for value timeout";
      break;

    case SL_STATUS_FAIL:
      status_msg = failure_msg;
      error_msg = "Error";
      break;

    default :
      status_msg = failure_msg;
      error_msg = "Unknown error";
  }

  /* Display the output message */
  printf("%s%s\r\n", status_msg, error_msg);
}

/**************************************************************************//**
 * @brief:
 * Wi-Fi CLI's callback: Connect to the Wi-Fi access point with
 *                       the information stored in station (wlan) parameters.
 *****************************************************************************/
void wifi_station_connect(sl_cli_command_arg_t *args)
{
  (void)args;
  bool ap_found = false;
  sl_wfx_status_t status;
  RTOS_ERR_CODE err_code;
  sl_wfx_ssid_def_t ap_ssid = {0};
  uint8_t wlan_bssid[SL_WFX_BSSID_SIZE]; /*!< Save AP's MAC */
  uint8_t retry_cnt;
  uint16_t channel;
  char *p_wlan_ssid = NULL;
  char *p_wlan_passkey = NULL;
  sl_wfx_security_mode_bitmask_t security_mode;
  /* The security mode option selected by users */
  sl_wfx_security_mode_t *p_wlan_secur_mode = NULL;

  secur_mode_fallback = false;

  /* Check whether station is already connected */
  if (wifi.state & SL_WFX_STA_INTERFACE_CONNECTED) {
      printf("Station is already connected\r\n");
      return;
  }

  /* Step 1: Retrieve settings parameters */
  p_wlan_ssid = (char *)wifi_cli_get_param_addr("station.ssid");
  p_wlan_passkey = (char *) wifi_cli_get_param_addr("station.passkey");
  p_wlan_secur_mode = (sl_wfx_security_mode_t *)wifi_cli_get_param_addr("station.security");

  /* Step 2: Scan to seach for an AP with given settings */
  if ((p_wlan_ssid == NULL)
      || (p_wlan_passkey == NULL)
      || (p_wlan_secur_mode == NULL)) {

      LOG_DEBUG("Station's ssid, passkey or security mode not found\r\n");
      goto error;
  }

  retry_cnt = 0;
  channel = 0;

  ap_ssid.ssid_length = strlen(p_wlan_ssid);
  strncpy((char *)ap_ssid.ssid, p_wlan_ssid, ap_ssid.ssid_length);

  /* Scan with retry */
  do {
      /* Reset scan list & count */
      scan_count_web = 0;
      scan_verbose = false;
      memset(scan_list,
             0,
             sizeof(scan_result_list_t) * SL_WFX_MAX_SCAN_RESULTS);

      /* Send scan command to WF200 */
      status = sl_wfx_send_scan_command(WFM_SCAN_MODE_ACTIVE,
                                        NULL,
                                        0,
                                        &ap_ssid,
                                        1,
                                        NULL,
                                        0,
                                        NULL);

      if ((status == SL_STATUS_OK) || (status == SL_STATUS_WIFI_WARNING)) {
          /* Block CLI to wait for scan_complete indication */
          err_code = wifi_cli_wait(&g_cli_sem,
                                   SL_WFX_SCAN_COMPLETE_IND_ID,
                                   SL_WFX_DEFAULT_REQUEST_TIMEOUT_MS);

          if (err_code == RTOS_ERR_TIMEOUT) {
              printf("Command timeout! Retry %d time(s)\r\n", retry_cnt + 1);

          } else if (err_code == RTOS_ERR_NONE) {
              /* Retrieve AP information from the scan_list */
              for (uint8_t i = 0; i < scan_count_web; i++) {
                  if (strcmp((char *)scan_list[i].ssid_def.ssid,
                             p_wlan_ssid) == 0) {
                      /* If matched, obtain MAC address */
                      memcpy(&wlan_bssid, scan_list[i].mac,
                             SL_WFX_BSSID_SIZE);
                      channel = scan_list[i].channel;
                      security_mode = scan_list[i].security_mode;
                      ap_found = true;
                  }
              }

          } else {
              printf("Command error! Retry %d time(s)\r\n", retry_cnt + 1);
          }
      }

  } while((ap_found == false) && (retry_cnt++ < 3));

  scan_verbose = true;

  if (ap_found == false) {
      printf("Access point's name: \"%s\" not found\r\n", p_wlan_ssid);
      return;
  }

  /* If WPA2/WPA3 transition mode is selected, check whether the AP supports up to WPA3 security mode */
  if ((*p_wlan_secur_mode == WFM_SECURITY_MODE_WPA3_SAE_WPA2_PSK) 
      && (security_mode.wpa3 == 0) && (security_mode.wpa2 == 1)) {
      secur_mode_fallback = true;
      printf("The Access Point (AP) does not support WPA3 security mode\n");
  }

  /* WPA3 security mode configuration */
  if ((*p_wlan_secur_mode == WFM_SECURITY_MODE_WPA3_SAE) ||
      ((*p_wlan_secur_mode == WFM_SECURITY_MODE_WPA3_SAE_WPA2_PSK) && !secur_mode_fallback)) {
      status = sl_wfx_sae_prepare(&wifi.mac_addr_0,
                                  (sl_wfx_mac_address_t*) wlan_bssid,
                                  (uint8_t*) p_wlan_ssid,
                                  strlen(p_wlan_ssid),
                                  (uint8_t*)p_wlan_passkey,
                                  strlen(p_wlan_passkey),
                                  security_mode.h2e);
      printf("wlan preparing SAE...\r\n");
      if (status != SL_STATUS_OK) {
          printf("wlan: Could not prepare SAE\r\n");
      }
  }
  
  /* Step 3: Configure scan parameters & Connect to the found AP */
  sl_wfx_set_scan_parameters(0, 0, 1);

  /* Connect to a Wi-Fi access point */
  status = sl_wfx_send_join_command((uint8_t *)p_wlan_ssid,
                                    strlen(p_wlan_ssid),
                                    (sl_wfx_mac_address_t*)wlan_bssid,
                                    channel,
                                    secur_mode_fallback ? 
                                    WFM_SECURITY_MODE_WPA2_PSK : 
                                    *p_wlan_secur_mode,
                                    1,
                                    0,
                                    (uint8_t *)p_wlan_passkey,
                                    strlen(p_wlan_passkey),
                                    NULL,
                                    0);
  if (status == SL_STATUS_OK) {
      /* Block to wait for a connected confirmation */
      err_code = wifi_cli_wait(&g_cli_sem,
                               SL_WFX_CONNECT_IND_ID,
                               SL_WFX_DEFAULT_REQUEST_TIMEOUT_MS);

      if (err_code == RTOS_ERR_TIMEOUT) {
          LOG_DEBUG("wifi_cli_wait() timeout\r\n");
          goto error;
      } else if (err_code != RTOS_ERR_NONE) {
          LOG_DEBUG("wifi_cli_wait() failed: err_code = %d\r\n", err_code);
          goto error;
      }
      return;
  } else {
      LOG_DEBUG("Failed to send join command\r\n");
      /* go to error */
  }

error:
  printf("Command error\r\n");
  return; /* Failed */
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: Disconnect from the Wi-Fi access point.
 *****************************************************************************/
void wifi_station_disconnect(sl_cli_command_arg_t *args)
{
  (void)args;

  RTOS_ERR_CODE err_code;
  sl_wfx_status_t status;
  
  /* Check if station is not connected */
  if (!(wifi.state & SL_WFX_STA_INTERFACE_CONNECTED)) {
      printf("Station is not connected to an AP\r\n");
      return;
  }

  /* Disconnect from a Wi-Fi access point */
  status = sl_wfx_send_disconnect_command();
  if (status == SL_STATUS_OK) {
      /* Block to wait for a confirmation */
      err_code = wifi_cli_wait(&g_cli_sem,
                               SL_WFX_DISCONNECT_IND_ID,
                               SL_WFX_DEFAULT_REQUEST_TIMEOUT_MS);

      if (err_code == RTOS_ERR_TIMEOUT) {
          LOG_DEBUG("wifi_cli_wait() timeout\r\n");
          goto error;
      } else if (err_code != RTOS_ERR_NONE) {
          LOG_DEBUG("wifi_cli_wait() failed: err_code = %d\r\n", err_code);
          goto error;
      }
      return;
  }

error:
  printf("Command error\r\n");
  return; /* Failed */
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: Perform a Wi-Fi scan.
 *****************************************************************************/
void wifi_station_scan(sl_cli_command_arg_t *args)
{
  (void)args;
  RTOS_ERR_CODE err_code;

  printf("!  # Ch RSSI MAC (BSSID)        Network (SSID) \n");
  /* Start a scan*/
  sl_wfx_send_scan_command(WFM_SCAN_MODE_ACTIVE,
                           NULL,
                           0,
                           NULL,
                           0,
                           NULL,
                           0,
                           NULL);

  /* Block to wait indication messages */
  err_code = wifi_cli_wait(&g_cli_sem,
                           SL_WFX_SCAN_COMPLETE_IND_ID,
                           SL_WFX_DEFAULT_REQUEST_TIMEOUT_MS);

  if (err_code == RTOS_ERR_TIMEOUT) {
      LOG_DEBUG("wifi_cli_wait() timeout\r\n");
      goto error;
  } else if (err_code != RTOS_ERR_NONE) {
      LOG_DEBUG("wifi_cli_wait() failed: err_code = %d\r\n", err_code);
      goto error;
  }
  return;

error:
  printf("Command error\r\n");
  return; /* Failed */
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: Send ICMP ECHO_REQUEST to network hosts.
 *****************************************************************************/
void ping_cmd_cb(sl_cli_command_arg_t *args)
{
  int argc;
  int req_number;
  char *ip_str = NULL;
  char *err_msg = "Command error\r\n";
  char *invalid_arg = "Invalid argument";
  char *help_text = "Examples: ping 192.168.0.1\r\n"
                       "         ping -n 100 192.168.0.1";

  argc = sl_cli_get_argument_count(args);
  switch (argc) {
    case 1:
      /* No request msg number: ping ip_addr */ 
      ip_str = sl_cli_get_argument_string(args, 0);
      if (ping_cmd(PING_DEFAULT_REQ_NB, ip_str) != SL_STATUS_OK) {
          printf("%s", err_msg);
      }
      break;

    case 3:
      /* Full options: ping -n nb ip_addr */ 
      ip_str = sl_cli_get_argument_string(args, 2);
      if (strcmp(sl_cli_get_argument_string(args, 0), "-n")) {
        goto invalid_arg_err;
      }

      req_number = atoi(sl_cli_get_argument_string(args, 1));
      if (req_number <= 0) {
        goto  invalid_arg_err;
      }

      if (ping_cmd((uint32_t)req_number, ip_str) != SL_STATUS_OK) {
          printf("%s", err_msg);
      }
      break;

    default:
      goto invalid_arg_err;
      break;
  }
  return;

invalid_arg_err:
    printf("%s!\r\n%s\r\n", invalid_arg, help_text);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: Save wifi connection settings parameters
 *****************************************************************************/
void wifi_save(sl_cli_command_arg_t *args)
{
  (void)args;
  if (strlen(wlan_ssid) > 0)
  {
    nvm3_writeData(nvm3_defaultHandle,
                   NVM3_KEY_AP_SSID,
                   (void *)wlan_ssid,
                   sizeof(wlan_ssid));
  }

  if (strlen(wlan_passkey) > 0) {
    nvm3_writeData(nvm3_defaultHandle,
                   NVM3_KEY_AP_PASSKEY,
                   (void *)wlan_passkey,
                   sizeof(wlan_passkey));
  }

  nvm3_writeData(nvm3_defaultHandle,
                NVM3_KEY_AP_SECURITY_MODE,
                (void *)&wlan_security,
                sizeof(wlan_security));

  nvm3_writeData(nvm3_defaultHandle,
                NVM3_KEY_AP_SECURITY_WPA3_PMKSA,
                (void *)&wlan_security_wpa3_pmksa,
                sizeof(wlan_security_wpa3_pmksa));                 
}