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
#ifndef WIFI_CLI_GET_SET_CB_FUNC_H
#define WIFI_CLI_GET_SET_CB_FUNC_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "sl_cli.h"
#include "sl_cli_instances.h"
#include "sl_cli_arguments.h"
#include "sl_cli_handles.h"
#include "sl_wfx_rf_test_agent.h"
#include "sl_wfx_host.h"
#include "sl_wfx_constants.h"
#include "lwip/netif.h"
#include "wifi_cli_params.h"
#include "app_wifi_events.h"
#include "sl_wfx_secure_link.h"

/* Global wifi context */
extern sl_wfx_context_t   wifi;

#ifdef __cplusplus
extern "C" {
#endif
/***************************************************************************//**
 * @brief Reset the host CPU
 ******************************************************************************/
void reset_host_cpu(sl_cli_command_arg_t *args);

/***************************************************************************//**
 * @brief Reboot the Wi-Fi transceiver chip
 ******************************************************************************/
void wifi_init(sl_cli_command_arg_t *args);

/***************************************************************************//**
 * @brief Save Wi-Fi parameters
 ******************************************************************************/
void wifi_save(sl_cli_command_arg_t *args);

/*******************************************************************************
 *****************  WI-FI CLI's GET COMMAND PROTOTYPES   ***********************
 ******************************************************************************/
void get_wifi_drv_version(sl_cli_command_arg_t *args);
void get_wifi_fw_version(sl_cli_command_arg_t *args);
void get_wifi_bus(sl_cli_command_arg_t *args);
void get_wifi_state(sl_cli_command_arg_t *args);
void get_station_ssid(sl_cli_command_arg_t *args);
void get_station_passkey(sl_cli_command_arg_t *args);
void get_station_security(sl_cli_command_arg_t *args);
void get_station_dhcp_client_state(sl_cli_command_arg_t *args);
void get_station_netmask(sl_cli_command_arg_t *args);
void get_station_gateway(sl_cli_command_arg_t *args);
void get_station_ip(sl_cli_command_arg_t *args);
void get_station_pmk(sl_cli_command_arg_t *args);
void get_station_mac(sl_cli_command_arg_t *args);

void get_softap_ssid(sl_cli_command_arg_t *args);
void get_softap_passkey(sl_cli_command_arg_t *args);
void get_softap_security(sl_cli_command_arg_t *args);
void get_softap_channel(sl_cli_command_arg_t *args);
void get_softap_netmask(sl_cli_command_arg_t *args);
void get_softap_gateway(sl_cli_command_arg_t *args);
void get_softap_ip(sl_cli_command_arg_t *args);
void get_softap_pmk(sl_cli_command_arg_t *args);
void get_softap_mac(sl_cli_command_arg_t *args);
void get_softap_dhcp_server_state(sl_cli_command_arg_t *args);
void get_softap_client_list(sl_cli_command_arg_t *args);

/*******************************************************************************
 ******************   WI-FI CLI's SET COMMAND PROTOTYPES   *********************
 ******************************************************************************/
void set_station_ssid(sl_cli_command_arg_t *args);
void set_station_passkey(sl_cli_command_arg_t *args);
void set_station_security(sl_cli_command_arg_t *args);
void set_station_dhcp_client_state(sl_cli_command_arg_t *args);
void set_station_netmask(sl_cli_command_arg_t *args);
void set_station_gateway(sl_cli_command_arg_t *args);
void set_station_ip(sl_cli_command_arg_t *args);
void set_station_mac(sl_cli_command_arg_t *args);

void set_softap_ssid(sl_cli_command_arg_t *args);
void set_softap_passkey(sl_cli_command_arg_t *args);
void set_softap_security(sl_cli_command_arg_t *args);
void set_softap_channel(sl_cli_command_arg_t *args);
void set_softap_netmask(sl_cli_command_arg_t *args);
void set_softap_gateway(sl_cli_command_arg_t *args);
void set_softap_ip(sl_cli_command_arg_t *args);
void set_softap_mac(sl_cli_command_arg_t *args);
void set_softap_dhcp_server_state(sl_cli_command_arg_t *args);
void set_mac_key(sl_cli_command_arg_t *args);

/*******************************************************************************
 **************   WI-FI CLI's STATION COMMAND PROTOTYPES   *********************
 ******************************************************************************/
void wifi_station_connect(sl_cli_command_arg_t *args);
void wifi_station_disconnect(sl_cli_command_arg_t *args);
void wifi_station_disconnect(sl_cli_command_arg_t *args);
void wifi_station_scan(sl_cli_command_arg_t *args);
void wifi_station_rssi(sl_cli_command_arg_t *args);

void wifi_station_power_mode(sl_cli_command_arg_t *args);
void wifi_station_power_save(sl_cli_command_arg_t *args);

/*******************************************************************************
 **************   WI-FI CLI's SOFTAP COMMAND PROTOTYPES   ********************
 ******************************************************************************/
void wifi_start_softap(sl_cli_command_arg_t *args);
void wifi_stop_softap(sl_cli_command_arg_t *args);
void wifi_softap_rssi(sl_cli_command_arg_t *args);

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: Display the IP stack statistics.
 *****************************************************************************/
void lwip_ip_stats(sl_cli_command_arg_t *args);

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: Send ICMP ECHO_REQUEST to network hosts.
 *****************************************************************************/
void ping_cmd_cb(sl_cli_command_arg_t *args);

/*******************************************************************************
 **************   WI-FI CLI's IPERF COMMANDS PROTOTYPES   **********************
 ******************************************************************************/
void iperf(sl_cli_command_arg_t *args);
void iperf_server_stop(sl_cli_command_arg_t *args);
void iperf_client_stop(sl_cli_command_arg_t *args);

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: Send a command to the RF Test Agent.
 *****************************************************************************/
void wifi_test_agent(sl_cli_command_arg_t *args);

/*******************************************************************************
 *************   WI-FI CLI's SECURE-LINK COMMAND PROTOTYPES   ******************
 ******************************************************************************/
void wifi_slk_rekey(sl_cli_command_arg_t *args);
void wifi_slk_add(sl_cli_command_arg_t *args);
void wifi_slk_remove(sl_cli_command_arg_t *args);
void wifi_slk_bitmap(sl_cli_command_arg_t *args);


/*******************************************************************************
 *************   WI-FI CLI's WLAN RATE ALGORITHM COMMANDS PROTOTYPES   ********
 ******************************************************************************/
void wifi_wlan_rate_algo(sl_cli_command_arg_t *args);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_CLI_GET_SET_CB_FUNC_H */
