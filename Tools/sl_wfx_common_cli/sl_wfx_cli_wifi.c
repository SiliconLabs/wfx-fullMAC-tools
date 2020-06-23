/**************************************************************************//**
 * Copyright 2019, Silicon Laboratories Inc.
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

#include <stdio.h>
#include <stdint.h>
#include "sl_wfx.h"
#include "sl_wfx_cli_generic.h"
#include "sl_wfx_cli_common.h"
#include "sl_wfx_host_events.h"

// X.x.x: Major version of the WiFi CLI
#define SL_WFX_CLI_WIFI_VERSION_MAJOR      2
// x.X.x: Minor version of the WiFi CLI
#define SL_WFX_CLI_WIFI_VERSION_MINOR      0
// x.x.X: Revision of the WiFi CLI
#define SL_WFX_CLI_WIFI_VERSION_REVISION   4

// Provides the version of the WiFi CLI
#define SL_WFX_CLI_WIFI_VERSION   SL_WFX_CLI_GEN_MODULE_VERSION(SL_WFX_CLI_WIFI_VERSION_MAJOR, \
                                                                SL_WFX_CLI_WIFI_VERSION_MINOR, \
                                                                SL_WFX_CLI_WIFI_VERSION_REVISION)

// Provides the version of the WiFi CLI as string
#define SL_WFX_CLI_WIFI_VERSION_STRING     SL_WFX_CLI_GEN_MODULE_VERSION_STRING(SL_WFX_CLI_WIFI_VERSION_MAJOR, \
                                                                                SL_WFX_CLI_WIFI_VERSION_MINOR, \
                                                                                SL_WFX_CLI_WIFI_VERSION_REVISION)

extern const uint32_t sl_wfx_firmware_size;

static sl_wfx_context_t *wifi_ctx = NULL;
static uint8_t wifi_clients[SL_WFX_CLI_WIFI_NB_MAX_CLIENTS][6] = {0};
static uint8_t wifi_nb_clients_connected = 0;

static int reboot_cmd_cb (int argc,
                          char **argv,
                          char *output_buf,
                          uint32_t output_buf_len)
{
  char success_msg[] = "WF(M)200 initialized successful";
  char failure_msg[] = "Failed to initialized WF(M)200: ";
  char *status_msg;
  char *error_msg;
  sl_status_t status;
  int res = -1;

  (void)argc;
  (void)argv;

  // Force the bus deinitialization to ensure a re-initialization from scratch.
  // This is especially useful for the SDIO
  sl_wfx_host_deinit_bus();

  // Initialize the Wi-Fi chip
  status = sl_wfx_init( wifi_ctx );
  switch(status) {
    case SL_STATUS_OK:
      status_msg = success_msg;
      error_msg = "";
      res = 0;
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

  // Format the output message
  snprintf(output_buf,
           output_buf_len,
           "%s%s\r\n",
           status_msg,
           error_msg);

  return res;
}

static const sl_wfx_cli_generic_command_t reboot_cmd =
{
  "wifi-reboot",
  "wifi-reboot              : Reboot the Wi-Fi chip\r\n",
  reboot_cmd_cb,
  0
};

static int network_up_cmd_cb (int argc,
                              char **argv,
                              char *output_buf,
                              uint32_t output_buf_len)
{
  char *msg = NULL;
  char *wlan_ssid = NULL;
  char *wlan_passkey = NULL;
  sl_status_t status;
  sl_wfx_security_mode_t *wlan_security;
  int res = -1;

  (void)argc;
  (void)argv;

  // Retrieve all the parameters required by the function
  wlan_ssid = (char *)sl_wfx_cli_param_get_addr("wlan.ssid");
  wlan_passkey = (char *)sl_wfx_cli_param_get_addr("wlan.passkey");
  wlan_security = (sl_wfx_security_mode_t *)sl_wfx_cli_param_get_addr("wlan.security");

  if ((wlan_ssid != NULL)
      && (wlan_passkey != NULL)
      && (wlan_security != NULL)) {
    // Configure the wait event
    sl_wfx_cli_generic_config_wait(SL_WFX_EVENT_CONNECT);

    // Connect to a Wi-Fi access point
    status = sl_wfx_send_join_command((uint8_t*) wlan_ssid,
                                      strlen(wlan_ssid),
                                      NULL,
                                      0,
                                      *wlan_security,
                                      1,
                                      0,
                                      (uint8_t*) wlan_passkey,
                                      strlen(wlan_passkey),
                                      NULL,
                                      0);

    if (status == SL_STATUS_OK) {
      // Wait for a confirmation
      res = sl_wfx_cli_generic_wait(SL_WFX_DEFAULT_REQUEST_TIMEOUT_MS);
      if (res == SL_WFX_CLI_ERROR_TIMEOUT) {
        msg = (char *)command_timeout_msg;
        res = -1;
      } else if (res == SL_WFX_CLI_ERROR_NONE) {
        // Success
        res = 0;
      } else {
        msg = (char *)command_error_msg;
        res = -1;
      }
    } else {
      msg = (char *)command_error_msg;
    }
  } else {
    // One of the parameter could not be retrieved
    msg = (char *)missing_parameter_msg;
  }

  if (msg != NULL) {
    // Format the output message
    strncpy(output_buf, msg, output_buf_len);
    if (output_buf_len > 0) {
      output_buf[output_buf_len - 1] = '\0';
    }
  }

  return res;
}

static sl_wfx_cli_generic_command_t network_up_cmd =
{
  "network-up",
  "network-up               : Connect to the Wi-Fi access point with the information stored in wlan parameters\r\n",
  network_up_cmd_cb,
  0
};

static sl_wfx_cli_generic_command_t nup_cmd =
{
  "nup",
  "nup                      : Alias of network-up\r\n",
  network_up_cmd_cb,
  0
};

static int network_down_cmd_cb (int argc,
                                char **argv,
                                char *output_buf,
                                uint32_t output_buf_len)
{
  char *msg = NULL;
  sl_status_t status;
  int res = -1;

  (void)argc;
  (void)argv;

  // Configure the wait event
  sl_wfx_cli_generic_config_wait(SL_WFX_EVENT_DISCONNECT);

  // Disconnect from a Wi-Fi access point
  status = sl_wfx_send_disconnect_command();

  if (status == SL_STATUS_OK) {
    // Wait for a confirmation
    res = sl_wfx_cli_generic_wait(SL_WFX_DEFAULT_REQUEST_TIMEOUT_MS);
    if (res == SL_WFX_CLI_ERROR_TIMEOUT) {
      msg = (char *)command_timeout_msg;
      res = -1;
    } else if (res == SL_WFX_CLI_ERROR_NONE) {
      // Success
      res = 0;
    } else {
      msg = (char *)command_error_msg;
      res = -1;
    }
  } else {
    msg = (char *)command_error_msg;
  }

  if (msg != NULL) {
    // Format the output message
    strncpy(output_buf, msg, output_buf_len);
    if (output_buf_len > 0) {
      output_buf[output_buf_len - 1] = '\0';
    }
  }

  return res;
}

static const sl_wfx_cli_generic_command_t network_down_cmd =
{
  "network-down",
  "network-down             : Disconnect from the Wi-Fi access point\r\n",
  network_down_cmd_cb,
  0
};

static const sl_wfx_cli_generic_command_t ndo_cmd =
{
  "ndo",
  "ndo                      : Alias of network-down\r\n",
  network_down_cmd_cb,
  0
};

static int scan_cmd_cb (int argc,
                        char **argv,
                        char *output_buf,
                        uint32_t output_buf_len)
{
  char *msg = NULL;
  sl_status_t status;
  int res = -1;

  (void)argc;
  (void)argv;

  // Configure the wait event
  sl_wfx_cli_generic_config_wait(SL_WFX_EVENT_SCAN_COMPLETE);

  printf("!  # Ch RSSI MAC (BSSID)        Network (SSID)\r\n");

  // Start a scan
  status = sl_wfx_send_scan_command(WFM_SCAN_MODE_ACTIVE,
                                    NULL,
                                    0,
                                    NULL,
                                    0,
                                    NULL,
                                    0,
                                    NULL);

  if ((status == SL_STATUS_OK)
      || (status == SL_STATUS_WIFI_WARNING)) {
    // Wait for a confirmation
    res = sl_wfx_cli_generic_wait(SL_WFX_DEFAULT_REQUEST_TIMEOUT_MS);
    if (res == SL_WFX_CLI_ERROR_TIMEOUT) {
      msg = (char *)command_timeout_msg;
      res = -1;
    } else if (res == SL_WFX_CLI_ERROR_NONE) {
      // Success
      res = 0;
    } else {
      msg = (char *)command_error_msg;
      res = -1;
    }
  } else {
    msg = (char *)command_error_msg;
  }

  if (msg != NULL) {
    // Format the output message
    strncpy(output_buf, msg, output_buf_len);
    if (output_buf_len > 0) {
      output_buf[output_buf_len - 1] = '\0';
    }
  }

  return res;
}

static const sl_wfx_cli_generic_command_t scan_cmd =
{
  "scan",
  "scan                     : Perform a Wi-Fi scan\r\n",
  scan_cmd_cb,
  0
};

static int powermode_cmd_cb (int argc,
                             char **argv,
                             char *output_buf,
                             uint32_t output_buf_len)
{
  sl_status_t status;
  char *msg = NULL;
  int power_mode;
  int interval;
  int res = -1;
  char buf[5];

  if ((argc == 2) || (argc == 3)) {
    strncpy(&buf[0], argv[1], sizeof(buf));
    buf[sizeof(buf)-1] = '\0';
    power_mode = atoi(buf);

    if (power_mode == 0) {
      status = sl_wfx_set_power_mode(WFM_PM_MODE_ACTIVE, 0);
      if (status == SL_STATUS_OK) {
        strncpy(output_buf, "Power Mode disabled\r\n", output_buf_len);
        res = 0;
      } else {
        msg = (char *)command_error_msg;
      }
    } else if (argc == 3) {
      strncpy(&buf[0], argv[2], sizeof(buf));
      buf[sizeof(buf)-1] = '\0';
      interval = atoi(buf);

      status = sl_wfx_set_power_mode((sl_wfx_pm_mode_t)power_mode,
                                     (uint16_t)interval);
      if (status == SL_STATUS_OK) {
        snprintf(output_buf,
                 output_buf_len,
                 "Power mode %d, interval %u\r\n",
                 power_mode,
                 (uint16_t)interval);
        res = 0;
      } else {
        msg = (char *)command_error_msg;
      }
    } else {
      msg = (char *)invalid_command_msg;
    }
  } else {
    msg = (char *)invalid_command_msg;
  }

  if (msg != NULL) {
    // Format the output message
    strncpy(output_buf, msg, output_buf_len);
    if (output_buf_len > 0) {
      output_buf[output_buf_len - 1] = '\0';
    }
  }

  return res;
}

static const sl_wfx_cli_generic_command_t powermode_cmd =
{
  "wlan-pm",
  "wlan-pm                  : Enable/disable the Power Mode on the WLAN interface of the Wi-Fi chip\r\n"
  "                         NOTE: WLAN must be up\r\n"
  "                         Usage: wlan-pm <mode> [interval]\r\n"
  "                           mode: 0(awake), 1(wake-up on beacons), 2(wake-up on DTIMs)\r\n"
  "                           interval: number of beacons/DTIMs to skip while asleep\r\n",
  powermode_cmd_cb,
  -1
};

static int powersave_cmd_cb (int argc,
                             char **argv,
                             char *output_buf,
                             uint32_t output_buf_len)
{
  sl_status_t status;
  char buf[4];
  int state;
  int res = -1;

  (void)argc;

  strncpy(&buf[0], argv[1], sizeof(buf));
  buf[sizeof(buf)-1] = '\0';
  state = atoi(buf);

  if (state == 0) {
    // Disable the Power Save
    status = sl_wfx_disable_device_power_save();
    if (status == SL_STATUS_OK) {
      strncpy(output_buf, "Power Save disabled\r\n", output_buf_len);
      res = 0;
    }
  } else {
    // Enable the Power Save
    status = sl_wfx_enable_device_power_save();
    if (status == SL_STATUS_OK) {
      strncpy(output_buf, "Power Save enabled\r\n", output_buf_len);
      res = 0;
    }
  }

  if (res != 0) {
    // Format the output message
    strncpy(output_buf, (char *)command_error_msg, output_buf_len);
    if (output_buf_len > 0) {
      output_buf[output_buf_len - 1] = '\0';
    }
  }

  return res;
}

static const sl_wfx_cli_generic_command_t powersave_cmd =
{
  "wlan-ps",
  "wlan-ps                  : Enable/disable the Power Save on the WLAN interface of the Wi-Fi chip\r\n"
  "                         Usage: wlan-ps <state>\r\n"
  "                           state: 0(OFF), 1(ON)\r\n",
  powersave_cmd_cb,
  1
};

static int wlan_rssi_cmd_cb (int argc,
                             char **argv,
                             char *output_buf,
                             uint32_t output_buf_len)
{
  sl_status_t status;
  uint32_t rcpi;
  int res = -1;

  (void)argc;

  status = sl_wfx_get_signal_strength(&rcpi);
  if (status == SL_STATUS_OK) {
    snprintf(output_buf,
             output_buf_len,
             "RSSI value : %d dBm\r\n",
             (int16_t)(rcpi - 220)/2);
    res = 0;
  } // else let the generic CLI display the error message

  return res;
}

static const sl_wfx_cli_generic_command_t wlan_rssi_cmd =
{
  "wlan-rssi",
  "wlan-rssi                : Get the RSSI of the WLAN interface\r\n",
  wlan_rssi_cmd_cb,
  0
};

static int softap_up_cmd_cb (int argc,
                             char **argv,
                             char *output_buf,
                             uint32_t output_buf_len)
{
  char *msg = NULL;
  char *softap_ssid = NULL;
  char *softap_passkey = NULL;
  uint8_t  *softap_channel = NULL;
  sl_wfx_security_mode_t *softap_security = NULL;
  sl_status_t status;
  int res = -1;

  (void)argc;
  (void)argv;

  // Retrieve all the parameters required by the function
  softap_ssid = (char *)sl_wfx_cli_param_get_addr("softap.ssid");
  softap_passkey = (char *)sl_wfx_cli_param_get_addr("softap.passkey");
  softap_channel = (uint8_t *)sl_wfx_cli_param_get_addr("softap.channel");
  softap_security = (sl_wfx_security_mode_t *)sl_wfx_cli_param_get_addr("softap.security");

  if ((softap_ssid != NULL)
      && (softap_passkey != NULL)
      && (softap_security != NULL)
      && (softap_channel != NULL)) {
    // Configure the wait event
    sl_wfx_cli_generic_config_wait(SL_WFX_EVENT_START_AP);

    // Start the SoftAP interface
    status = sl_wfx_start_ap_command(*softap_channel,
                                     (uint8_t*)softap_ssid,
                                     strlen(softap_ssid),
                                     0,
                                     0,
                                     *softap_security,
                                     0,
                                     (uint8_t*)softap_passkey,
                                     strlen(softap_passkey),
                                     NULL,
                                     0,
                                     NULL,
                                     0);

    if (status == SL_STATUS_OK) {
      // Wait for a confirmation
      res = sl_wfx_cli_generic_wait(SL_WFX_DEFAULT_REQUEST_TIMEOUT_MS);
      if (res == SL_WFX_CLI_ERROR_TIMEOUT) {
        msg = (char *)command_timeout_msg;
        res = -1;
      } else if (res == SL_WFX_CLI_ERROR_NONE) {
        // Success
        res = 0;
      } else {
        msg = (char *)command_error_msg;
        res = -1;
      }
    } else {
      msg = (char *)command_error_msg;
    }
  } else {
    // One of the parameter could not be retrieved
    msg = (char *)missing_parameter_msg;
  }

  if (msg != NULL) {
    // Format the output message
    strncpy(output_buf, msg, output_buf_len);
    if (output_buf_len > 0) {
      output_buf[output_buf_len - 1] = '\0';
    }
  }

  return res;
}

static const sl_wfx_cli_generic_command_t softap_up_cmd =
{
  "softap-up",
  "softap-up                : Start the SoftAP interface using the information stored in softap parameters\r\n",
  softap_up_cmd_cb,
  0
};

static const sl_wfx_cli_generic_command_t sup_cmd =
{
  "sup",
  "sup                      : Alias of softap-up\r\n",
  softap_up_cmd_cb,
  0
};

static int softap_down_cmd_cb (int argc,
                               char **argv,
                               char *output_buf,
                               uint32_t output_buf_len)
{
  char *msg = NULL;
  sl_status_t status;
  int res = -1;

  (void)argc;
  (void)argv;

  // Configure the wait event
  sl_wfx_cli_generic_config_wait(SL_WFX_EVENT_STOP_AP);

  // Stop the SoftAP interface
  status = sl_wfx_stop_ap_command();
  if (status == SL_STATUS_OK) {
    // Wait for a confirmation
    res = sl_wfx_cli_generic_wait(SL_WFX_DEFAULT_REQUEST_TIMEOUT_MS);
    if (res == SL_WFX_CLI_ERROR_TIMEOUT) {
      msg = (char *)command_timeout_msg;
      res = -1;
    } else if (res == SL_WFX_CLI_ERROR_NONE) {
      // Success
      res = 0;
    } else {
      msg = (char *)command_error_msg;
      res = -1;
    }
  } else {
    msg = (char *)command_error_msg;
  }

  if (msg != NULL) {
    // Format the output message
    strncpy(output_buf, msg, output_buf_len);
    if (output_buf_len > 0) {
      output_buf[output_buf_len - 1] = '\0';
    }
  }

  // Clear the client list
  wifi_nb_clients_connected = 0;

  return res;
}

static const sl_wfx_cli_generic_command_t softap_down_cmd =
{
  "softap-down",
  "softap-down              : Stop the SoftAP interface\r\n",
  softap_down_cmd_cb,
  0
};

static const sl_wfx_cli_generic_command_t sdo_cmd =
{
  "sdo",
  "sdo                      : Alias of softap-down\r\n",
  softap_down_cmd_cb,
  0
};

static int softap_rssi_cmd_cb (int argc,
                               char **argv,
                               char *output_buf,
                               uint32_t output_buf_len)
{
  sl_status_t status;
  uint32_t rcpi;
  char *msg = NULL;
  char mac[18];
  int res = -1;
  sl_wfx_mac_address_t mac_address;

  (void)argc;

  strncpy(&mac[0], argv[1], sizeof(mac));
  mac[sizeof(mac)-1] = 0;

  // Should be "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx" but it
  // doesn't work with the nano LibC
  // This code might cause issues.
  res = sscanf(mac,
               "%02hx:%02hx:%02hx:%02hx:%02hx:%02hx",
               (short unsigned int *)&mac_address.octet[0],
               (short unsigned int *)&mac_address.octet[1],
               (short unsigned int *)&mac_address.octet[2],
               (short unsigned int *)&mac_address.octet[3],
               (short unsigned int *)&mac_address.octet[4],
               (short unsigned int *)&mac_address.octet[5]);

  if (res == sizeof(mac_address.octet)) {
    status = sl_wfx_get_ap_client_signal_strength(&mac_address , &rcpi);

    if (status == SL_STATUS_OK) {
      snprintf(output_buf,
               output_buf_len,
               "Client %s RSSI value : %d dBm\r\n",
               mac,
               (int16_t)(rcpi - 220)/2);
      res = 0;
    } else {
      msg = (char *)command_error_msg;
    }
  } else {
    msg = (char *)invalid_command_msg;
  }

  if (msg != NULL) {
    // Format the output message
    strncpy(output_buf, msg, output_buf_len);
    if (output_buf_len > 0) {
      output_buf[output_buf_len - 1] = '\0';
    }
  }

  return res;
}

static const sl_wfx_cli_generic_command_t softap_rssi_cmd =
{
  "softap-rssi",
  "softap-rssi              : Get the RSSI of a station connected to the SoftAP\r\n"
  "                         Usage: softap-rssi <sta_mac>\r\n",
  softap_rssi_cmd_cb,
  1
};

static int softap_client_list_cmd_cb (int argc,
                                      char **argv,
                                      char *output_buf,
                                      uint32_t output_buf_len)
{
  // Output can have a consequent size, directly output it
  for (uint8_t i=0; i<wifi_nb_clients_connected; i++) {
    printf("%02X:%02X:%02X:%02X:%02X:%02X\r\n",
           wifi_clients[i][0], wifi_clients[i][1],
           wifi_clients[i][2], wifi_clients[i][3],
           wifi_clients[i][4], wifi_clients[i][5]);
  }

  return 0;
}

static const sl_wfx_cli_generic_command_t softap_client_list_cmd =
{
  "softap-client-list",
  "softap-client-list       : Get the list of clients connected to the SoftAP\r\n",
  softap_client_list_cmd_cb,
  0
};

int sl_wfx_cli_wifi_init (sl_wfx_context_t *wfx_ctx)
{
  int res = -1;

  if (wfx_ctx != NULL) {
    // Initialize the resources needed by the commands
    wifi_ctx = wfx_ctx;

    // Add wifi commands to the CLI
    res = sl_wfx_cli_generic_register_cmd(&reboot_cmd);
    res = sl_wfx_cli_generic_register_cmd(&network_up_cmd);
    res = sl_wfx_cli_generic_register_cmd(&nup_cmd);
    res = sl_wfx_cli_generic_register_cmd(&network_down_cmd);
    res = sl_wfx_cli_generic_register_cmd(&ndo_cmd);
    res = sl_wfx_cli_generic_register_cmd(&scan_cmd);
    res = sl_wfx_cli_generic_register_cmd(&powermode_cmd);
    res = sl_wfx_cli_generic_register_cmd(&powersave_cmd);
    res = sl_wfx_cli_generic_register_cmd(&wlan_rssi_cmd);
    res = sl_wfx_cli_generic_register_cmd(&softap_up_cmd);
    res = sl_wfx_cli_generic_register_cmd(&sup_cmd);
    res = sl_wfx_cli_generic_register_cmd(&softap_down_cmd);
    res = sl_wfx_cli_generic_register_cmd(&sdo_cmd);
    res = sl_wfx_cli_generic_register_cmd(&softap_rssi_cmd);
    res = sl_wfx_cli_generic_register_cmd(&softap_client_list_cmd);

    if (res == 0) {
      // Register the WiFi CLI module
      res = sl_wfx_cli_generic_register_module("WiFi CLI",
                                               SL_WFX_CLI_WIFI_VERSION_STRING);
    }
  }

  return res;
}

int sl_wfx_cli_wifi_add_client (uint8_t *mac)
{
  int res = -1;

  if (wifi_nb_clients_connected < SL_WFX_CLI_WIFI_NB_MAX_CLIENTS) {
    memcpy(wifi_clients[wifi_nb_clients_connected], mac, sizeof(wifi_clients[0]));
    wifi_nb_clients_connected++;
    res = 0;
  }

  return res;
}

int sl_wfx_cli_wifi_remove_client (uint8_t *mac)
{
  int res = -1;
  uint8_t i;
  bool wifi_client_found = false;

  for (i=0; i<wifi_nb_clients_connected; i++) {
    if (memcmp(wifi_clients[i], mac, sizeof(wifi_clients[i])) == 0) {
      memset(wifi_clients[i], 0, sizeof(wifi_clients[i]));
      wifi_client_found = true;
      break;
    }
  }

  if (wifi_client_found) {
    // Other clients may exit after the one removed, shift them all
    for (uint8_t j=i+1; j<wifi_nb_clients_connected; i++, j++) {
      memcpy(wifi_clients[i], wifi_clients[j], sizeof(wifi_clients[i]));
    }

    wifi_nb_clients_connected--;
    res = 0;
  }

  return res;
}
