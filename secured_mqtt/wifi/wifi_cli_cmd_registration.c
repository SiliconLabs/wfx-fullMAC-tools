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
 *  RESET COMMAND
 *****************************************************************************/
static const sl_cli_command_info_t cli_cmd_reset_cpu = \
    SL_CLI_COMMAND(reset_host_cpu,
                   "Reset the host CPU",
                   "reset" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

/**************************************************************************//**
 *  GET COMMANDS STRUCTS
 *****************************************************************************/
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

static const sl_cli_command_info_t cli_cmd_get_station_mac = \
    SL_CLI_COMMAND(get_station_mac,
                   "Get station MAC address",
                   "station.mac" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

/**************************************************************************//**
 * CREATE GET GROUP COMMANDS
 *****************************************************************************/
static const sl_cli_command_entry_t cmd_get_grp_table[] = {
    {"wifi.state", &cli_cmd_get_wifi_state, false},
    {"station.ssid", &cli_cmd_get_station_ssid, false},
    {"station.passkey", &cli_cmd_get_station_passkey, false},
    {"station.security", &cli_cmd_get_station_security, false},
    {"station.dhcp_client_state", &cli_cmd_get_station_dhcp_client_state, false},
    {"station.netmask", &cli_cmd_get_station_netmask, false},
    {"station.gateway", &cli_cmd_get_station_gateway, false},
    {"station.ip", &cli_cmd_get_station_ip, false},
    {"station.mac", &cli_cmd_get_station_mac, false},
    {NULL, NULL, false}
};

static const sl_cli_command_info_t get_cmds = \
    SL_CLI_COMMAND_GROUP(cmd_get_grp_table, "<parameter_name> \t \t"
                                            "For example: \t"
                                            "wifi get station.ssid");

/**************************************************************************//**
 * SET COMAMNDS CONSTRUCT
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

/**************************************************************************//**
 * CREATE SET GROUP COMMANDS
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
    {NULL, NULL, false}
};


static const sl_cli_command_info_t set_cmds = \
    SL_CLI_COMMAND_GROUP(cmd_set_grp_table, "<parameter_name> <value>");

/**************************************************************************//**
 * WIFI INIT COMMAND
 *****************************************************************************/
static const sl_cli_command_info_t cli_cmd_wifi_init = \
    SL_CLI_COMMAND(wifi_init,
                   "Reboot the wifi chip",
                   "Reboot wifi chip" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });
/*** WIFI SAVE COMMAND *****/
static const sl_cli_command_info_t cli_cmd_wifi_save = \
    SL_CLI_COMMAND(wifi_save,
                   "Save the Station information to NVM memory",
                   "Save the Station information to NVM memory" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

/**************************************************************************//**
 * WIFI STATION NETWORK UP/DOWN COMMAND
 *****************************************************************************/
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
                   "Wifi Scan" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

/**************************************************************************//**
 * GROUP WIFI COMAMNDS
 *****************************************************************************/
static const sl_cli_command_entry_t wifi_cli_cmds_table[] = {
    {"get", &get_cmds, false},
    {"set", &set_cmds, false},
    {"init", &cli_cmd_wifi_init, false},
    {"connect", &cli_cmd_wifi_sta_connect, false},
    {"disconnect", &cli_cmd_wifi_sta_disconnect, false},
    {"scan", &cli_cmd_wifi_sta_scan, false},
    {"save", &cli_cmd_wifi_save, false},
    {NULL, NULL, false}
};

static const sl_cli_command_info_t wifi_cli_cmds = \
    SL_CLI_COMMAND_GROUP(wifi_cli_cmds_table, "Wifi CLI commands");

/**************************************************************************//**
 * CREATE WIFI CLI COMMAND TABLE
 *****************************************************************************/
static const sl_cli_command_entry_t wifi_table[] = {
    {"wifi", &wifi_cli_cmds, false},
    {NULL, NULL, false}
};


/**************************************************************************//**
* PING COMMAND
*****************************************************************************/
static const sl_cli_command_info_t cli_cmd_ping = \
    SL_CLI_COMMAND(ping_cmd_cb,
                   "Send ICMP ECHO_REQUEST to network hosts",
                   "[-n nb] <ip>",
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

/**************************************************************************//**
* CREATE CLI COMMANDS TABLE
*****************************************************************************/
static const sl_cli_command_entry_t cmds_table[] = {
    {"reset", &cli_cmd_reset_cpu, false},
    {"ping", &cli_cmd_ping, false},
    {NULL, NULL, false}
};

/**************************************************************************//**
* Create the wifi command group at the top level
*****************************************************************************/
static sl_cli_command_group_t wifi_group = {
  { NULL },
  false,
  wifi_table
};

/**************************************************************************//**
* Create the commands group at the top level
*****************************************************************************/
static sl_cli_command_group_t cmds_group = {
  { NULL },
  false,
  cmds_table
};

/***************************************************************************//**
* @brief
*    This function call APIs to add command groups to the CLI service
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

  return status;
}

/***************************************************************************//**
* @brief
*    This task registers CLI's commands
*
* @param[in]
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
