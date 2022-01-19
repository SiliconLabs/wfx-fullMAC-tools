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
/**< This file is intended for implementing CLI commands */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "os.h"

#include "sl_cli.h"
#include "sl_cli_instances.h"
#include "sl_cli_arguments.h"
#include "sl_cli_handles.h"

#include "mqtt_cli_get_set_cb_func.h"
#include "mqtt_cli_params.h"
#include "mqtt_cli_app.h"
/*< Some defines for wifi_cli_command_registration_task */
#define WFX_CLI_CMD_REGISTRATION_TASK_PRIO        32u
#define WFX_CLI_CMD_REIGSTRATION_TASK_STK_SIZE    800u

/*< CLI command registration task stack*/
static CPU_STK wfx_cli_cmd_registraton_task_stk[WFX_CLI_CMD_REIGSTRATION_TASK_STK_SIZE];

/*< CLI command registration TCB */
static OS_TCB wfx_cli_cmd_registration_task_tcb;
static void wfx_cli_cmd_registration_task(void *p_arg);
static int register_cli_commands(void);

/**** GET COMMANDS STRUCTS ****/
static const sl_cli_command_info_t cli_cmd_get_broker_address = \
    SL_CLI_COMMAND(get_mqtt_string,
                   "get the broker address",
                   "broker.address" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_broker_port = \
    SL_CLI_COMMAND(get_mqtt_uint16,
                   "get the broker port",
                   "broker.port" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_client_id = \
    SL_CLI_COMMAND(get_mqtt_string,
                   "get the client ID",
                   "client_id" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_broker_username = \
    SL_CLI_COMMAND(get_mqtt_string,
                   "get the broker username",
                   "broker.username" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_broker_password = \
    SL_CLI_COMMAND(get_mqtt_string,
                   "get the broker password",
                   "broker.password" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_tls_device_certificate = \
    SL_CLI_COMMAND(get_mqtt_string,
                   "get the tls device certificate",
                   "tls.device_certificate" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_tls_certification_authority = \
    SL_CLI_COMMAND(get_mqtt_string,
                   "get the tls certification authority",
                   "tls.certification_authority" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_tls_device_private_key = \
    SL_CLI_COMMAND(get_mqtt_string,
                   "get the tls device private_key",
                   "tls.device_private_key" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_pulish_topic_name = \
    SL_CLI_COMMAND(get_mqtt_string,
                   "get the pulish topic name",
                   "publish_topic.name" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_get_subscribe_topic_name = \
    SL_CLI_COMMAND(get_mqtt_string,
                   "get the subscribe topic name",
                   "subscribe_topic.name" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd_mqtt_get_mqtt = \
    SL_CLI_COMMAND(mqtt_cli_get_info,
                   "Get the information of mqtt",
                   "Set up information mqtt first" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

/*** CREATE GET GROUP COMMANDS ***/
static const sl_cli_command_entry_t cmd_get_grp_table[] = {
    {"broker.address", &cli_cmd_get_broker_address, false},
    {"broker.port",    &cli_cmd_get_broker_port, false},
    {"broker.username", &cli_cmd_get_broker_username, false},
    {"broker.password", &cli_cmd_get_broker_password, false},
    {"tls.certification_authority", &cli_cmd_get_tls_certification_authority, false},
    {"tls.device_certificate", &cli_cmd_get_tls_device_certificate, false},
    {"tls.device_private_key", &cli_cmd_get_tls_device_private_key, false},
    {"publish_topic.name", &cli_cmd_get_pulish_topic_name, false},
    {"subscribe_topic.name", &cli_cmd_get_subscribe_topic_name, false},
    {"client_id", &cli_cmd_get_client_id, false},
    {"mqtt", &cmd_mqtt_get_mqtt, false},
    {NULL, NULL, false}
};

static const sl_cli_command_info_t get_cmds = \
    SL_CLI_COMMAND_GROUP(cmd_get_grp_table, "<parameter_name> \t \t"
                                            "For example: \t"
                                            "mqtt get broker.address");

/************ SET COMAMNDS CONSTRUCT *******************/
static const sl_cli_command_info_t cli_cmd_set_broker_address = \
    SL_CLI_COMMAND(set_mqtt_string,
                   "set the broker address",
                   "broker.address" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_set_broker_port = \
    SL_CLI_COMMAND(set_mqtt_uint16,
                   "set the broker port",
                   "broker.port" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_UINT16, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_set_broker_username = \
    SL_CLI_COMMAND(set_mqtt_string,
                   "set the broker username",
                   "broker.username" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_set_broker_password = \
    SL_CLI_COMMAND(set_mqtt_string,
                   "set the broker password",
                   "broker.password" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_set_cert_auth = \
    SL_CLI_COMMAND(mqtt_set_ca_certificate,
                   "set the TSL CA",
                   "tls.certification_authority" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_set_device_cert = \
    SL_CLI_COMMAND(mqtt_set_device_certificate,
                   "set the TLS device certificate",
                   "tls.device_certificate" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_set_device_private_key = \
    SL_CLI_COMMAND(mqtt_set_device_private_key,
                   "set the TLS device private key",
                   "tls.device_private_key" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });


static const sl_cli_command_info_t cli_cmd_set_publish_topic_name = \
    SL_CLI_COMMAND(set_mqtt_string,
                   "set the publish topic name",
                   "publish_topic.name" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_set_subscribe_topic_name = \
    SL_CLI_COMMAND(set_mqtt_string,
                   "set the subscribe topic name",
                   "subscribe_topic.name" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_set_client_id = \
    SL_CLI_COMMAND(set_mqtt_string,
                   "set the client ID",
                   "client_id" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

/** CREATE SET GROUP COMMANDS **/
static const sl_cli_command_entry_t cmd_set_grp_table[] = {
    {"broker.address", &cli_cmd_set_broker_address, false},
    {"broker.port", &cli_cmd_set_broker_port, false},
    {"broker.username", &cli_cmd_set_broker_username, false},
    {"broker.password", &cli_cmd_set_broker_password, false},
    {"tls.certification_authority", &cli_cmd_set_cert_auth, false},
    {"tls.device_certificate", &cli_cmd_set_device_cert, false},
    {"tls.device_private_key", &cli_cmd_set_device_private_key, false},
    {"publish_topic.name", &cli_cmd_set_publish_topic_name, false},
    {"subscribe_topic.name", &cli_cmd_set_subscribe_topic_name, false},
    {"client_id", &cli_cmd_set_client_id, false},
    {NULL, NULL, false}
};


static const sl_cli_command_info_t set_cmds = \
    SL_CLI_COMMAND_GROUP(cmd_set_grp_table, "<parameter_name> <value>");


/*** WIFI STATION NETWORK UP/DOWN COMMAND *****/
static const sl_cli_command_info_t cmd_mqtt_connect = \
    SL_CLI_COMMAND(mqtt_cli_connect,
                   "Connect to the broker",
                   "Set up information mqtt first" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd_mqtt_publish = \
    SL_CLI_COMMAND(mqtt_cli_publish,
                   "publish a message",
                   "Set up information mqtt first" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd_mqtt_save = \
    SL_CLI_COMMAND(mqtt_cli_save,
                   "save the parameters in NVM",
                   "Set up information mqtt first" SL_CLI_UNIT_SEPARATOR,
                   {SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

/** GROUP MQTT COMAMNDS **/
static const sl_cli_command_entry_t mqtt_cli_cmds_table[] = {
    {"get", &get_cmds, false},
    {"set", &set_cmds, false},
    {"connect", &cmd_mqtt_connect, false},
    {"save", &cmd_mqtt_save, false},
    {"publish", &cmd_mqtt_publish, false},
    {NULL, NULL, false}
};

static const sl_cli_command_info_t mqtt_cli_cmds = \
    SL_CLI_COMMAND_GROUP(mqtt_cli_cmds_table, "mqtt cli commands");

/*** CREATE MQTT CLI COMMANDS TABLE **/
static const sl_cli_command_entry_t mqtt_table[] = {
    {"mqtt", &mqtt_cli_cmds, false},
    {NULL, NULL, false}
};

/** Create the wifi command group at the top level ***/
static sl_cli_command_group_t mqtt_group = {
  { NULL },
  false,
  mqtt_table
};

static int register_cli_commands(void) {
  bool status;
  // And finally call the function to install the wifi_group commands.
  status = sl_cli_command_add_command_group(sl_cli_inst_handle, &mqtt_group);
  EFM_ASSERT(status);

  return status;
}

static void wfx_cli_cmd_registration_task(void *p_arg) {
  RTOS_ERR err;
  PP_UNUSED_PARAM(p_arg);

  // Register CLI commands
  if(false == register_cli_commands()) {
      MQTT_DBG("Failed to register MQTT CLI commands \r\n");
      return;
  }

  // Initialize Global mqtt params.
  if(mqtt_cli_params_init()) {
      MQTT_DBG("\n Failed to init mqtt params \n");
      return;
  }

  MQTT_DBG("\n Success to register MQTT CLI commands \n");

  // Delete this task
  OSTaskDel(NULL, &err);
}

void mqtt_cli_cmd_registration_init(void) {
  RTOS_ERR err;

  OSTaskCreate(&wfx_cli_cmd_registration_task_tcb,   // Create the cmd registration task.
               "Start APP Task",
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

    APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
}



