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
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "os.h"
#include "sl_cli.h"
#include "sl_cli_instances.h"
#include "sl_cli_arguments.h"
#include "sl_cli_handles.h"
#include "wifi_cli_cmd_registration.h"
#include "wifi_cli_get_set_cb_func.h"

/*******************************************************************************
 ***********  Wi-Fi CLI Commands Registration Task Configuration   *************
 ******************************************************************************/
/* Some defines for wifi_cli_command_registration_task */
#define WFX_CLI_CMD_REGISTRATION_TASK_PRIO        32u
#define WFX_CLI_CMD_REIGSTRATION_TASK_STK_SIZE    800u

/* CLI command registration task stack */
static CPU_STK wfx_cli_cmd_registraton_task_stk[WFX_CLI_CMD_REIGSTRATION_TASK_STK_SIZE];

/* CLI command registration TCB */
static OS_TCB wfx_cli_cmd_registration_task_tcb;
static void wfx_cli_cmd_registration_task(void *p_arg);

/**************************************************************************//**
 *  @brief: Construct the host CPU reset command
 *****************************************************************************/
static const sl_cli_command_info_t cli_cmd_reset_cpu = \
    SL_CLI_COMMAND(reset_host_cpu,
                   "Reset the host CPU",
                   "reset" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

/**************************************************************************//**
 *  @brief: Construct all get commands
 *****************************************************************************/
static const sl_cli_command_info_t cli_cmd_get_wifi_drv_version = \
    SL_CLI_COMMAND(get_wifi_drv_version,
                   "Get wifi driver version",
                   "wifi.drv_version" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_wifi_fw_version = \
    SL_CLI_COMMAND(get_wifi_fw_version,
                   "Get wifi firmware version",
                   "wifi.fw_version" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_wifi_bus = \
    SL_CLI_COMMAND(get_wifi_bus,
                   "Get wifi bus",
                   "wifi.bus" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_wifi_state = \
    SL_CLI_COMMAND(get_wifi_state,
                   "Get wifi state",
                   "wifi.state" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_station_ssid = \
    SL_CLI_COMMAND(get_station_ssid,
                   "Get station ssid",
                   "station.ssid" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_station_passkey = \
    SL_CLI_COMMAND(get_station_passkey,
                   "Get station passkey",
                   "station.passkey" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_station_security = \
    SL_CLI_COMMAND(get_station_security,
                   "Get station security",
                   "station.security" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_station_dhcp_client_state = \
    SL_CLI_COMMAND(get_station_dhcp_client_state,
                   "Get station dhcp client state",
                   "station.dhcp_client_state" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_station_netmask = \
    SL_CLI_COMMAND(get_station_netmask,
                   "Get station netmask (IPV4 format)",
                   "station.netmask" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_station_gateway = \
    SL_CLI_COMMAND(get_station_gateway,
                   "Get station gateway",
                   "station.gateway" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_station_ip = \
    SL_CLI_COMMAND(get_station_ip,
                   "Get station ip",
                   "station.ip" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_station_pmk = \
    SL_CLI_COMMAND(get_station_pmk,
                   "Get station pairwise master key",
                   "station.ip" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_station_mac = \
    SL_CLI_COMMAND(get_station_mac,
                   "Get station MAC address",
                   "station.mac" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_softap_ssid = \
    SL_CLI_COMMAND(get_softap_ssid,
                   "Get SoftAP SSID",
                   "softap.ssid" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_softap_passkey = \
   SL_CLI_COMMAND(get_softap_passkey,
                  "Get SoftAP passkey",
                  "softap.passkey" SL_CLI_UNIT_SEPARATOR,
                  {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_softap_security = \
   SL_CLI_COMMAND(get_softap_security,
                  "Get SoftAP security",
                  "softap.security" SL_CLI_UNIT_SEPARATOR,
                  {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_softap_channel = \
   SL_CLI_COMMAND(get_softap_channel,
                  "Get SoftAP channel (decimal)",
                  "softap.channel" SL_CLI_UNIT_SEPARATOR,
                  {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_softap_netmask = \
   SL_CLI_COMMAND(get_softap_netmask,
                  "Get SoftAP netmask (IPv4 format)",
                  "softap.netmask" SL_CLI_UNIT_SEPARATOR,
                  {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_softap_gateway = \
   SL_CLI_COMMAND(get_softap_gateway,
                  "Get SoftAP gateway (IPv4 format)",
                  "softap.gateway" SL_CLI_UNIT_SEPARATOR,
                  {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_softap_ip = \
   SL_CLI_COMMAND(get_softap_ip,
                  "Get SoftAP ip (IPv4 format)",
                  "softap.ip" SL_CLI_UNIT_SEPARATOR,
                  {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_softap_pmk = \
   SL_CLI_COMMAND(get_softap_pmk,
                  "Get SoftAP pairwise master key",
                  "softap.pmk" SL_CLI_UNIT_SEPARATOR,
                  {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_softap_mac = \
   SL_CLI_COMMAND(get_softap_mac,
                  "Get SoftAP MAC address (EUI-48 format)",
                  "softap.mac" SL_CLI_UNIT_SEPARATOR,
                  {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_softap_dhcp_server_state = \
   SL_CLI_COMMAND(get_softap_dhcp_server_state,
                  "Get SoftAP DHCP server state",
                  "softap.dhcp" SL_CLI_UNIT_SEPARATOR,
                  {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_softap_client_list = \
   SL_CLI_COMMAND(get_softap_client_list,
                  "Get SoftAP client list",
                  "softap.client_list" SL_CLI_UNIT_SEPARATOR,
                  {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

/**************************************************************************//**
 * @brief: Grouping all get commands
 *****************************************************************************/
static const sl_cli_command_entry_t cmd_get_grp_table[] = {
    {"wifi.drv_version", &cli_cmd_get_wifi_drv_version, false},
    {"wifi.fw_version", &cli_cmd_get_wifi_fw_version, false},
    {"wifi.bus", &cli_cmd_get_wifi_bus, false},
    {"wifi.state", &cli_cmd_get_wifi_state, false},
    {"station.ssid", &cli_cmd_get_station_ssid, false},
    {"station.passkey", &cli_cmd_get_station_passkey, false},
    {"station.security", &cli_cmd_get_station_security, false},
    {"station.dhcp_client_state", &cli_cmd_get_station_dhcp_client_state, false},
    {"station.netmask", &cli_cmd_get_station_netmask, false},
    {"station.gateway", &cli_cmd_get_station_gateway, false},
    {"station.ip", &cli_cmd_get_station_ip, false},
    {"station.pmk", &cli_cmd_get_station_pmk, false},
    {"station.mac", &cli_cmd_get_station_mac, false},
    {"softap.ssid", &cli_cmd_get_softap_ssid, false},
    {"softap.passkey", &cli_cmd_get_softap_passkey, false},
    {"softap.security", &cli_cmd_get_softap_security, false},
    {"softap.channel", &cli_cmd_get_softap_channel, false},
    {"softap.netmask", &cli_cmd_get_softap_netmask, false},
    {"softap.gateway", &cli_cmd_get_softap_gateway, false},
    {"softap.ip", &cli_cmd_get_softap_ip, false},
    {"softap.pmk", &cli_cmd_get_softap_pmk, false},
    {"softap.mac", &cli_cmd_get_softap_mac, false},
    {"softap.dhcp_server_state", &cli_cmd_get_softap_dhcp_server_state, false},
    {"softap.client_list", &cli_cmd_get_softap_client_list, false},
    {NULL, NULL, false}
};

static const sl_cli_command_info_t get_cmds = \
    SL_CLI_COMMAND_GROUP(cmd_get_grp_table, "<parameter_name> \t \t"
                                            "For example: \t"
                                            "wifi get station.ssid");

/**************************************************************************//**
 * @brief: Construct set commands
 *****************************************************************************/
static const sl_cli_command_info_t cli_cmd_set_station_ssid = \
    SL_CLI_COMMAND(set_station_ssid,
                   "Set station ssid",
                   "station.ssid" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_set_station_passkey = \
    SL_CLI_COMMAND(set_station_passkey,
                   "Set station passkey (Max 64-byte length)",
                   "station.passkey" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_set_station_security = \
    SL_CLI_COMMAND(set_station_security,
                   "Set station security mode with values "
                   "[OPEN, WEP, WPA1/WPA2, WPA2, WPA3]",
                   "station.security" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_set_station_dhcp_client_state = \
    SL_CLI_COMMAND(set_station_dhcp_client_state,
                   "Set station dhcp client state",
                   "ON/OFF" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_set_station_netmask = \
    SL_CLI_COMMAND(set_station_netmask,
                   "Set station netmask (IPV4 format)",
                   "station.netmask" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_set_station_gateway = \
    SL_CLI_COMMAND(set_station_gateway,
                   "Set station gateway",
                   "station.gateway" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_set_station_ip = \
    SL_CLI_COMMAND(set_station_ip,
                   "Set station ip",
                   "station.ip" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_set_station_mac = \
    SL_CLI_COMMAND(set_station_mac,
                   "Set station MAC address",
                   "station.mac" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_set_softap_ssid = \
    SL_CLI_COMMAND(set_softap_ssid,
                   "Set SoftAP SSID",
                   "softap.ssid" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_set_softap_passkey = \
   SL_CLI_COMMAND(set_softap_passkey,
                  "Set SoftAP passkey",
                  "softap.passkey" SL_CLI_UNIT_SEPARATOR,
                  {SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_set_softap_security = \
   SL_CLI_COMMAND(set_softap_security,
                  "Set SoftAP security",
                  "softap.security" SL_CLI_UNIT_SEPARATOR,
                  {SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_set_softap_channel = \
   SL_CLI_COMMAND(set_softap_channel,
                  "Set SoftAP channel",
                  "softap.channel" SL_CLI_UNIT_SEPARATOR,
                  {SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_set_softap_netmask = \
   SL_CLI_COMMAND(set_softap_netmask,
                  "Set SoftAP netmask (IPv4 format)",
                  "softap.netmask" SL_CLI_UNIT_SEPARATOR,
                  {SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_set_softap_gateway = \
   SL_CLI_COMMAND(set_softap_gateway,
                  "Set SoftAP gateway (IPv4 format)",
                  "softap.gateway" SL_CLI_UNIT_SEPARATOR,
                  {SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_set_softap_ip = \
   SL_CLI_COMMAND(set_softap_ip,
                  "Set SoftAP ip (IPv4 format)",
                  "softap.ip" SL_CLI_UNIT_SEPARATOR,
                  {SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_set_softap_mac = \
   SL_CLI_COMMAND(set_softap_mac,
                  "Set SoftAP MAC address (EUI-48 format)",
                  "softap.mac" SL_CLI_UNIT_SEPARATOR,
                  {SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_set_softap_dhcp_server_state = \
   SL_CLI_COMMAND(set_softap_dhcp_server_state,
                  "Set SoftAP DHCP server state",
                  "softap.dhcp_client_state" SL_CLI_UNIT_SEPARATOR,
                  {SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_set_mac_key = \
   SL_CLI_COMMAND(set_mac_key,
                  "Secure Link MAC key (32 bytes hex array format)",
                  "wifi.mac_key" SL_CLI_UNIT_SEPARATOR,
                  {SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

/**************************************************************************//**
 * @brief: Grouping all set commands
 *****************************************************************************/
static const sl_cli_command_entry_t cmd_set_grp_table[] = {
    {"station.ssid", &cli_cmd_set_station_ssid, false},
    {"station.passkey", &cli_cmd_set_station_passkey, false},
    {"station.security", &cli_cmd_set_station_security, false},
    {"station.dhcp_client_state", &cli_cmd_set_station_dhcp_client_state, false},
    {"station.netmask", &cli_cmd_set_station_netmask, false},
    {"station.gateway", &cli_cmd_set_station_gateway, false},
    {"station.ip", &cli_cmd_set_station_ip, false},
    {"station.mac", &cli_cmd_set_station_mac, false},
    {"softap.ssid", &cli_cmd_set_softap_ssid, false},
    {"softap.passkey", &cli_cmd_set_softap_passkey, false},
    {"softap.security", &cli_cmd_set_softap_security, false},
    {"softap.channel", &cli_cmd_set_softap_channel, false},
    {"softap.netmask", &cli_cmd_set_softap_netmask, false},
    {"softap.gateway", &cli_cmd_set_softap_gateway, false},
    {"softap.ip", &cli_cmd_set_softap_ip, false},
    {"softap.mac", &cli_cmd_set_softap_mac, false},
    {"softap.dhcp_server_state", &cli_cmd_set_softap_dhcp_server_state, false},
    {"wifi.mac_key", &cli_cmd_set_mac_key, false},
    {NULL, NULL, false}
};


static const sl_cli_command_info_t set_cmds = \
    SL_CLI_COMMAND_GROUP(cmd_set_grp_table, "<parameter_name> <value>");

/**************************************************************************//**
 * @brief: Construct the wifi reboot (init) command
 *****************************************************************************/
static const sl_cli_command_info_t cli_cmd_wifi_init = \
    SL_CLI_COMMAND(wifi_init,
                   "Reboot the wifi chip",
                   "Reboot the Wi-Fi chip" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

/**************************************************************************//**
 * @brief: Construct the wifi station's setting parameters save command
 ******************************************************************************/
static const sl_cli_command_info_t cli_cmd_wifi_save = \
    SL_CLI_COMMAND(wifi_save,
                   "Save the Station information to NVM memory",
                   "Save the Station information to NVM memory"
                   SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

/**************************************************************************//**
 * @brief:
 *  Construct Wi-Fi station scan, connect, disconnect network & get station RSSI
 ******************************************************************************/
static const sl_cli_command_info_t cli_cmd_wifi_sta_connect = \
    SL_CLI_COMMAND(wifi_station_connect,
                   "Connect to the Wi-Fi access point with the information"
                   " stored in wlan parameters",
                   "Set up ssid, security, passkey first" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_wifi_sta_disconnect = \
    SL_CLI_COMMAND(wifi_station_disconnect,
                   "Disconnect from the Wi-Fi access point",
                   "Network down" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_wifi_sta_scan = \
    SL_CLI_COMMAND(wifi_station_scan,
                   "Perform a Wi-Fi scan",
                   "Wi-Fi Scan" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_wifi_sta_rssi = \
    SL_CLI_COMMAND(wifi_station_rssi,
                   "Get the RSSI of the WLAN interface",
                   "Station RSSI" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

/**************************************************************************//**
 * @brief: Construct start/stop softAP & get softAP RSSI commands
 ******************************************************************************/
static const sl_cli_command_info_t cli_cmd_wifi_start_softap = \
    SL_CLI_COMMAND(wifi_start_softap,
                   "Start the SoftAP interface using the information"
                   " stored in softap parameters",
                   "Start SOFTAP" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_wifi_stop_softap = \
    SL_CLI_COMMAND(wifi_stop_softap,
                   "Stop the SoftAP interface",
                   "Stop SOFTAP" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_wifi_softap_rssi = \
    SL_CLI_COMMAND(wifi_softap_rssi,
                   "Get the RSSI of a station connected to the SoftAP",
                   "wifi softap_rssi <client MAC>"
                   "(MAC format: 00:00:00:00:00:00)" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

/**************************************************************************//**
 * @brief: Construct wifi powermode, powersave commands
 ******************************************************************************/
static const sl_cli_command_info_t cli_cmd_wifi_power_mode = \
    SL_CLI_COMMAND(wifi_station_power_mode,
                   "Set the Power Mode on the WLAN interface "
                   "of the Wi-Fi chip",
                   "Usage: [ACTIVE] | [BEACONS | DTIM] [UAPSD | FAST_PS] "
                   "<number of beacons/DTIMs>"
                   SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_STRING, SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_wifi_station_power_save = \
    SL_CLI_COMMAND(wifi_station_power_save,
                   "Enable/disable the Power Save on the WLAN interface "
                   "of the Wi-Fi chip ",
                   "wifi powersave <state>" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

/**************************************************************************//**
 * @brief: Construct the wifi RF test command
 ******************************************************************************/
static const sl_cli_command_info_t cli_cmd_wifi_test_agent = \
    SL_CLI_COMMAND(wifi_test_agent,
                   "Send a command to the RF Test Agent",
                   "Usage: wifi test <cmd> [cmd_args]" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

/**************************************************************************//**
 * @brief: Construct the wifi secure_link-related commands
 ******************************************************************************/
static const sl_cli_command_info_t cli_cmd_wifi_slk_rekey = \
    SL_CLI_COMMAND(wifi_slk_rekey,
                   "Renegotiate Secure Link session key",
                   "Renegotiate Secure Link session key" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_wifi_slk_add = \
    SL_CLI_COMMAND(wifi_slk_add,
                   "Enable the encryption of API General messages "
                   "with the specified \"msg_id\"",
                   "slk_add <msg_id>" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_wifi_slk_remove = \
    SL_CLI_COMMAND(wifi_slk_remove,
                   "Disable the encryption of API General messages "
                   "with the specified \"msg_id\"",
                   "slk_remove <msg_id>" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_wifi_slk_bitmap = \
    SL_CLI_COMMAND(wifi_slk_bitmap,
                   "Display the current state of the Secure Link Encryption Bitmap",
                   "slk_bitmap <msg_id>" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_wifi_wlan_rate_algo = \
    SL_CLI_COMMAND(wifi_wlan_rate_algo,
                   "Configure the rate algorithm to use"
                   "rate-algo <state>",
                   "state: 0(AARF), 1(minstrel)" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });
/**************************************************************************//**
 * @brief: Grouping all wifi commands
 ******************************************************************************/
static const sl_cli_command_entry_t wifi_cli_cmds_table[] = {
    {"get", &get_cmds, false},
    {"set", &set_cmds, false},
    {"init", &cli_cmd_wifi_init, false},
    {"connect", &cli_cmd_wifi_sta_connect, false},
    {"disconnect", &cli_cmd_wifi_sta_disconnect, false},
    {"scan", &cli_cmd_wifi_sta_scan, false},
    {"station_rssi", &cli_cmd_wifi_sta_rssi, false},
    {"start_softap", &cli_cmd_wifi_start_softap, false},
    {"stop_softap", &cli_cmd_wifi_stop_softap, false},
    {"softap_rssi", &cli_cmd_wifi_softap_rssi, false},
    {"powermode", &cli_cmd_wifi_power_mode, false},
    {"powersave", &cli_cmd_wifi_station_power_save, false},
    {"test", &cli_cmd_wifi_test_agent, false},
    {"slk_renegotiate", &cli_cmd_wifi_slk_rekey, false},
    {"slk_add", &cli_cmd_wifi_slk_add, false},
    {"slk_remove", &cli_cmd_wifi_slk_remove, false},
    {"slk_bitmap", &cli_cmd_wifi_slk_bitmap, false},
    {"save", &cli_cmd_wifi_save, false},
    {"rate-algo", &cli_cmd_wifi_wlan_rate_algo, false},
    {NULL, NULL, false}
};

static const sl_cli_command_info_t wifi_cli_cmds = \
    SL_CLI_COMMAND_GROUP(wifi_cli_cmds_table, "Wifi CLI commands");

/**************************************************************************//**
 * @brief: Create the wifi_table
 ******************************************************************************/
static const sl_cli_command_entry_t wifi_table[] = {
    {"wifi", &wifi_cli_cmds, false},
    {NULL, NULL, false}
};

/**************************************************************************//**
* @brief: Construct the lwip statistic command
******************************************************************************/
static const sl_cli_command_info_t cli_cmd_lwip_ip_stats = \
    SL_CLI_COMMAND(lwip_ip_stats,
                   "Display the LwIP stack statistics",
                   "lwip-stats",
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

/**************************************************************************//**
* @brief: Create the lwip_table
******************************************************************************/
static const sl_cli_command_entry_t lwip_cli_cmds_table[] = {
    {"stats", &cli_cmd_lwip_ip_stats, false},
    {NULL, NULL, false}
};

static const sl_cli_command_info_t lwip_cli_cmds = \
    SL_CLI_COMMAND_GROUP(lwip_cli_cmds_table, "lwip CLI commands");

static const sl_cli_command_entry_t lwip_table[] = {
    {"lwip", &lwip_cli_cmds, false},
    {NULL, NULL, false}
};

/**************************************************************************//**
* @brief: Construct iperf-related commands
*****************************************************************************/
static const sl_cli_command_info_t cli_cmd_iperf = \
    SL_CLI_COMMAND(iperf,
                   "Start a TCP iPerf test as a client or a server",
                   "iperf <-c ip [-t dur] [-p port] [-k] | -s>",
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_iperf_server_stop = \
    SL_CLI_COMMAND(iperf_server_stop,
                   "Stop the running iPerf server",
                   "iperf_stop_server",
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_iperf_client_stop = \
    SL_CLI_COMMAND(iperf_client_stop,
                   "Stop the running iPerf client",
                   "iperf_stop_client",
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

/**************************************************************************//**
* @brief: Construct the ping command
*****************************************************************************/
static const sl_cli_command_info_t cli_cmd_ping = \
    SL_CLI_COMMAND(ping_cmd_cb,
                   "Send ICMP ECHO_REQUEST to network hosts",
                   "[-n nb] <ip>",
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

/**************************************************************************//**
* @brief: Create the cmds_table containing reset, ping & iperf-related commands
*****************************************************************************/
static const sl_cli_command_entry_t cmds_table[] = {
    {"reset", &cli_cmd_reset_cpu, false},
    {"ping", &cli_cmd_ping, false},
    {"iperf", &cli_cmd_iperf, false},
    {"iperf_server_stop", &cli_cmd_iperf_server_stop, false},
    {"iperf_client_stop", &cli_cmd_iperf_client_stop, false},
    {NULL, NULL, false}
};

/**************************************************************************//**
* @brief: Create the wifi_group as the top level from the wifi_table
*****************************************************************************/
static sl_cli_command_group_t wifi_group = {
  { NULL },
  false,
  wifi_table
};

/**************************************************************************//**
* @brief: Create the cmds_group as the top level from the cmds_table
*****************************************************************************/
static sl_cli_command_group_t cmds_group = {
  { NULL },
  false,
  cmds_table
};

/**************************************************************************//**
* @brief: Create the lwip_group as the top level from the lwip_table
*****************************************************************************/
static sl_cli_command_group_t lwip_group = {
  { NULL },
  false,
  lwip_table
};

/***************************************************************************//**
* @brief
*    This function calls APIs to add command groups to the CLI service
*
* @param[in]
*
* @param[out] None
*
* @return
*       true if success
*       false if failed
******************************************************************************/
static bool register_cli_commands(void)
{
  bool status;

  /* Add the wifi_group commands */
  status = sl_cli_command_add_command_group(sl_cli_inst_handle, &wifi_group);
  EFM_ASSERT(status);

  /* Add the cmds_group */
  status |= sl_cli_command_add_command_group(sl_cli_inst_handle, &cmds_group);
  EFM_ASSERT(status);

  /* Add the lwip_group commands */
  status |= sl_cli_command_add_command_group(sl_cli_inst_handle, &lwip_group);
  EFM_ASSERT(status);

  return status;
}

/***************************************************************************//**
* @brief
*    This task registers CLI's commands
*
* @param[in] p_arg: The input argument of this task
*
* @param[out] None
*
* @return  None
******************************************************************************/
static void wfx_cli_cmd_registration_task(void *p_arg)
{
  RTOS_ERR err;
  PP_UNUSED_PARAM(p_arg);

  /* Register CLI commands */
  if(false == register_cli_commands()) {
      LOG_DEBUG("Failed to register CLI commands \r\n");
      return;
  }

  /* Delete this task */
  OSTaskDel(NULL, &err);
}

/***************************************************************************//**
 * @brief
 *    This function starts a task for registering CLI's commands
 *
 * @param[in]
 *
 * @param[out] None
 *
 * @return  None
 ******************************************************************************/
void wifi_cli_commands_init(void)
{
  RTOS_ERR err;

  /* Create the command registration task. */
  OSTaskCreate(&wfx_cli_cmd_registration_task_tcb,
               "Start wifi CLI commands registration task",
               wfx_cli_cmd_registration_task,
               DEF_NULL,
               WFX_CLI_CMD_REGISTRATION_TASK_PRIO,
               &wfx_cli_cmd_registraton_task_stk[0],
               (WFX_CLI_CMD_REIGSTRATION_TASK_STK_SIZE / 10u),
               WFX_CLI_CMD_REIGSTRATION_TASK_STK_SIZE,
               0u,
               0u,
               DEF_NULL,
               (OS_OPT_TASK_STK_CLR),
               &err);

  /* Check err code */
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
}
