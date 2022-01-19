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

#ifndef MQTT_CLI_GET_SET_CB_FUNC_H_
#define MQTT_CLI_GET_SET_CB_FUNC_H_

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "sl_cli.h"
#include "sl_cli_instances.h"
#include "sl_cli_arguments.h"
#include "sl_cli_handles.h"

/**** "GET" COMMANDS PROTOTYPES FOR MQTT CLI ********/
void get_mqtt_string(sl_cli_command_arg_t *args);
void get_mqtt_uint16(sl_cli_command_arg_t *args);

/**** "SET" COMMANDS PROTOTYPES FOR MQTT CLI ********/
void set_mqtt_string(sl_cli_command_arg_t *args);
void set_mqtt_uint16(sl_cli_command_arg_t *args);
///***MQTT INIT COMMAND ****/
///
///** MQTT commands ***/
void mqtt_cli_connect(sl_cli_command_arg_t *args);
void mqtt_cli_publish(sl_cli_command_arg_t *args);
void mqtt_cli_save(sl_cli_command_arg_t *args);
void mqtt_cli_get_info(sl_cli_command_arg_t *args);

// MQTT certificate set handles
void mqtt_set_ca_certificate(sl_cli_command_arg_t *args);
void mqtt_set_device_certificate(sl_cli_command_arg_t *args);
void mqtt_set_device_private_key(sl_cli_command_arg_t *args);

#endif /* MQTT_CLI_GET_SET_CB_FUNC_H_ */
