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

#include <string.h>
#include <stdint.h>

#include "mqtt_cli_params.h"

#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/apps/httpd.h"
#include "lwip/netifapi.h"
#include "lwip/ip_addr.h"

#include "dhcp_client.h"
#include "dhcp_server.h"
#include "ethernetif.h"
#include "sl_wfx_task.h"
#include "sl_wfx_host.h"

#include "lwip/apps/mqtt.h"
#include "lwip/altcp_tls.h"
#include "mqtt_cli_lwip.h"

#define WFX_CLI_LWIP_TASK_STK_SIZE  800u


static int sl_wfx_cli_register_mqtt_param(char *name, void *address,
                                      char *description,
                                      sl_wfx_mqtt_cli_param_custom_get_func_t get_func,
                                      sl_wfx_mqtt_cli_param_custom_set_func_t set_func,
                                      sl_wfx_mqtt_cli_param_type_t type,
                                      uint16_t size, uint8_t rights);

static int number_registered_params = 0;
///mqtt parameters struct array
mqtt_param_t mqtt_params[SL_WFX_MQTT_CLI_MAX_PARAMS] = {0};

/*** SOME HELPER FUNCTIONS UTILITY ***/
int mqtt_cli_param_search (char *name)
{
  int param_index = -1;
  int i;

  for (i=0; i<number_registered_params; i++) {
    if ((strncmp(name, mqtt_params[i].name, strlen(mqtt_params[i].name)) == 0)
        && (name[strlen(mqtt_params[i].name)] == '\0')) {
      param_index = i;
      break;
    }
  }

  return param_index;
}

void *mqtt_cli_get_param_addr(char *param_name) {
  int param_idx = -1;

  if (param_name == NULL)
    return NULL;

  param_idx = mqtt_cli_param_search(param_name);
  if (param_idx >= 0) {
      return (void*) mqtt_params[param_idx].address;
  }
  return NULL;
}

/** @brief: callback function to process string type parameter of
 * global struct wifi_params's entry
 * */
static int getStringParam(char *param_name,
                          void *param_addr,
                          uint32_t param_size,
                          char *out_buf,
                          uint32_t out_buf_len) {

    (void) param_name, (void) out_buf, (void) out_buf_len, (void) param_size;
    int ret = 0;
    if (out_buf == NULL) {
        printf("%s\r\n", (char*)param_addr);
    } else {
        /*< TODO: Later, to copy string to out_buf */
    }
    return ret;
}


static int setStringParam(char *param_name,
                          void *param_addr,
                          uint32_t param_size,
                          char *new_value) {
    (void) param_name;
    char *str = param_addr;
    int ret = 0;
    if (new_value == NULL) {
        printf("Input value is NULL\r\n");
       ret = -1;
    } else {
        int input_length = strlen(new_value);
        if (input_length == 0) {
            printf("Empty input string!\r\n");
            ret = -1;
        } else if (input_length < (int)(param_size - 1)) {
            strncpy(str, (char*)new_value, param_size);
            //Make sure the string is NULL terminated
            str[input_length] = '\0';
        } else {
            printf("Input string length is too large! (%d/%ld)\r\n",
                                                     input_length, param_size);
            ret = -1;
        }
    }

    return ret;
}


/** @brief: callback function to process string type parameter of
 * global struct wifi_params's entry
 * */
static int getIntegerParam(char *param_name,
                          void *param_addr,
                          uint32_t param_size,
                          char *out_buf,
                          uint32_t out_buf_len) {
    (void) param_name, (void) out_buf, (void) out_buf_len, (void) param_size;
    if (out_buf == NULL) {
        printf("%d\r\n", *(int*)(param_addr));
    } else {
        /*< TODO: Later, to copy string to out_buf */
    }
    return 0;
}


static int setIntegerParam(char *param_name,
                           void *param_addr,
                           uint32_t param_size,
                           char *new_value) {
    (void) param_name, (void) param_size;

    uint16_t* num = param_addr;
    *num = (uint16_t)new_value;

    return 0;
}

/**** INITIALIZE WIFI PARAMS ****/

/// Registering wifi params to the global wifi_params struct
static int sl_wfx_cli_register_mqtt_param(char *name, void *address,
                                      char *description,
                                      sl_wfx_mqtt_cli_param_custom_get_func_t get_func,
                                      sl_wfx_mqtt_cli_param_custom_set_func_t set_func,
                                      sl_wfx_mqtt_cli_param_type_t type,
                                      uint16_t size, uint8_t rights) {

  int ret = -1;
  mqtt_param_t param = {0};

  if (number_registered_params < SL_WFX_MQTT_CLI_MAX_PARAMS) {
    param.name = name;
    param.address = address;
    param.description = description;
    param.size = size;
    param.rights = rights;
    param.type = type;
    param.get_func = get_func;
    param.set_func = set_func;

    memcpy(&mqtt_params[number_registered_params], &param, sizeof(mqtt_param_t));
    number_registered_params++;
    ret = 0;
  }

  return ret;
}

/*< Add all wifi param to the wifi_param struct array here! */
int mqtt_cli_params_init(void) {
  int ret = -1;
  ret = sl_wfx_cli_register_mqtt_param("broker.address",
                                       (void*)&mqtt_broker_address[0],
                                       "get set broker address",
                                       getStringParam,
                                       setStringParam,
                                       SL_WFX_MQTT_CLI_PARAM_TYPE_STRING,
                                       sizeof(mqtt_broker_address),
                                       SL_WFX_MQTT_CLI_PARAM_GET_RIGHT);

  ret |= sl_wfx_cli_register_mqtt_param("broker.port",
                                       (void*)&mqtt_broker_port,
                                       "get set broker port",
                                       getIntegerParam,
                                       setIntegerParam,
                                       SL_WFX_MQTT_CLI_PARAM_TYPE_UNSIGNED_INTEGER,
                                       sizeof(mqtt_broker_port),
                                       SL_WFX_MQTT_CLI_PARAM_GET_RIGHT);

  ret |= sl_wfx_cli_register_mqtt_param("broker.username",
                                      (void*)&mqtt_username[0],
                                      "get set mqtt username",
                                      getStringParam,
                                      setStringParam,
                                      SL_WFX_MQTT_CLI_PARAM_TYPE_STRING,
                                      sizeof(mqtt_username),
                                      SL_WFX_MQTT_CLI_PARAM_GET_RIGHT);

  ret |= sl_wfx_cli_register_mqtt_param("broker.password",
                                      (void*)&mqtt_password[0],
                                      "get set mqtt password",
                                      getStringParam,
                                      setStringParam,
                                      SL_WFX_MQTT_CLI_PARAM_TYPE_STRING,
                                      sizeof(mqtt_password),
                                      SL_WFX_MQTT_CLI_PARAM_GET_RIGHT);

  ret |= sl_wfx_cli_register_mqtt_param("tls.certification_authority",
                                      (void*)&ca_certificate[0],
                                      "set ca certificate",
                                      getStringParam,
                                      setStringParam,
                                      SL_WFX_MQTT_CLI_PARAM_TYPE_STRING,
                                      sizeof(ca_certificate),
                                      SL_WFX_MQTT_CLI_PARAM_GET_RIGHT);


  ret |= sl_wfx_cli_register_mqtt_param("tls.device_certificate",
                                      (void*)&device_certificate[0],
                                      "set device certificate",
                                      getStringParam,
                                      setStringParam,
                                      SL_WFX_MQTT_CLI_PARAM_TYPE_STRING,
                                      sizeof(device_certificate),
                                      SL_WFX_MQTT_CLI_PARAM_GET_RIGHT);

  ret |= sl_wfx_cli_register_mqtt_param("tls.device_private_key",
                                      (void*)&device_key[0],
                                      "set device private key",
                                      getStringParam,
                                      setStringParam,
                                      SL_WFX_MQTT_CLI_PARAM_TYPE_STRING,
                                      sizeof(device_key),
                                      SL_WFX_MQTT_CLI_PARAM_GET_RIGHT);

  ret |= sl_wfx_cli_register_mqtt_param("publish_topic.name",
                                      (void*)&mqtt_publish_topic[0],
                                      "get/set publish topic name",
                                      getStringParam,
                                      setStringParam,
                                      SL_WFX_MQTT_CLI_PARAM_TYPE_STRING,
                                      sizeof(mqtt_publish_topic),
                                      SL_WFX_MQTT_CLI_PARAM_GET_RIGHT);

  ret |= sl_wfx_cli_register_mqtt_param("subscribe_topic.name",
                                      (void*)&mqtt_subscribe_topic[0],
                                      "get/set subscribe topic name",
                                      getStringParam,
                                      setStringParam,
                                      SL_WFX_MQTT_CLI_PARAM_TYPE_STRING,
                                      sizeof(mqtt_subscribe_topic),
                                      SL_WFX_MQTT_CLI_PARAM_GET_RIGHT);

  ret |= sl_wfx_cli_register_mqtt_param("client_id",
                                      (void*)&mqtt_client_id[0],
                                      "get/set client ID",
                                      getStringParam,
                                      setStringParam,
                                      SL_WFX_MQTT_CLI_PARAM_TYPE_STRING,
                                      sizeof(mqtt_client_id),
                                      SL_WFX_MQTT_CLI_PARAM_GET_RIGHT);

  // Look for an old MQTT configuration
    nvm3_readData(nvm3_defaultHandle, NVM3_KEY_MQTT_BROKER, (void *)mqtt_broker_address, sizeof(mqtt_broker_address));
    nvm3_readData(nvm3_defaultHandle, NVM3_KEY_MQTT_PORT, (void *)&mqtt_broker_port, sizeof(mqtt_broker_port));
    nvm3_readData(nvm3_defaultHandle, NVM3_KEY_MQTT_CLIENT_ID, (void *)mqtt_client_id, sizeof(mqtt_client_id));
    nvm3_readData(nvm3_defaultHandle, NVM3_KEY_MQTT_PUBLISH_TOPIC, (void *)mqtt_publish_topic, sizeof(mqtt_publish_topic));
    nvm3_readData(nvm3_defaultHandle, NVM3_KEY_MQTT_SUBSCRIBE_TOPIC, (void *)mqtt_subscribe_topic, sizeof(mqtt_subscribe_topic));

    nvm3_readData(nvm3_defaultHandle, NVM3_KEY_MQTT_USERNAME, (void *)mqtt_username, sizeof(mqtt_username));
    nvm3_readData(nvm3_defaultHandle, NVM3_KEY_MQTT_PASSWORD, (void *)mqtt_password, sizeof(mqtt_password));
    nvm3_readData(nvm3_defaultHandle, NVM3_KEY_MQTT_CA_CERTIFICATE, (void *)ca_certificate, sizeof(ca_certificate));
    nvm3_readData(nvm3_defaultHandle, NVM3_KEY_MQTT_DEVICE_CERTIFICATE, (void *)device_certificate, sizeof(device_certificate));
    nvm3_readData(nvm3_defaultHandle, NVM3_KEY_MQTT_DEVICE_KEY, (void *)device_key, sizeof(device_key));


  return ret;
}

