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
#include "mqtt_cli_app.h"
#include "mqtt_cli_params.h"
#include "mqtt_cli_lwip.h"
#include "mqtt_cli_get_set_cb_func.h"
#include "dhcp_client.h"
#include "dhcp_server.h"
#include <ethernetif.h>
#include "app_certificate.h"

static app_certificate_t *app_certificate;
/// Maximum size of a certificate
#define APP_CERTIFICATE_MAX_SIZE 2048
/******************** "GET" COMMANDS FOR MQTT CLI *****************************/

void get_mqtt_string(sl_cli_command_arg_t *args) {
  PP_UNUSED_PARAM(args);
  char *cmd = sl_cli_get_command_string(args, 2);
  int param_index = mqtt_cli_param_search(cmd);
  mqtt_params[param_index].get_func(NULL,
                                    mqtt_params[param_index].address,
                                    mqtt_params[param_index].size,
                                    NULL,
                                    0);
}

void get_mqtt_uint16(sl_cli_command_arg_t *args) {
  PP_UNUSED_PARAM(args);
  char *cmd = sl_cli_get_command_string(args, 2);
  int param_index = mqtt_cli_param_search(cmd);
  mqtt_params[param_index].get_func(NULL,
                                    mqtt_params[param_index].address,
                                    mqtt_params[param_index].size,
                                    NULL,
                                    0);
}
/******************** "SET" COMMANDS FOR MQTT CLI *****************************/
void set_mqtt_string(sl_cli_command_arg_t *args) {
  PP_UNUSED_PARAM(args);
  char *val_ptr = sl_cli_get_argument_string(args, 0);
  char *cmd = sl_cli_get_command_string(args, 2);

  int param_idx = mqtt_cli_param_search(cmd);
  if (param_idx >= 0) {
      mqtt_params[param_idx].set_func(NULL,
                                      mqtt_params[param_idx].address,
                                      mqtt_params[param_idx].size,
                                      val_ptr);
  }
}

void set_mqtt_uint16(sl_cli_command_arg_t *args) {
  PP_UNUSED_PARAM(args);

  uint16_t val_ptr = sl_cli_get_argument_uint16(args, 0);
  char *cmd = sl_cli_get_command_string(args, 2);
  int param_idx = mqtt_cli_param_search(cmd);
  if (param_idx >= 0) {
      mqtt_params[param_idx].set_func(NULL,
                                      mqtt_params[param_idx].address,
                                      mqtt_params[param_idx].size,
                                      (char*) val_ptr);
  }
}

void mqtt_cli_connect(sl_cli_command_arg_t *args) {
  PP_UNUSED_PARAM(args);
  RTOS_ERR err;
  if(mqtt_connected == true) {
    printf("MQTT already connected\r\n");
    return;
  }
  OSFlagPost(&mqtt_cli_events, SL_WFX_EVENT_MQTT_CONNECT, OS_OPT_POST_FLAG_SET, &err);
}

void mqtt_cli_publish(sl_cli_command_arg_t *args){
  PP_UNUSED_PARAM(args);
  RTOS_ERR rt_err;
  char *val_ptr = sl_cli_get_argument_string(args, 0);

  memset(mqtt_msg_publish, 0x00, sizeof(mqtt_msg_publish));
  strncpy(mqtt_msg_publish, val_ptr, sizeof(mqtt_msg_publish));
  OSFlagPost(&mqtt_cli_events, SL_WFX_EVENT_MQTT_PUBLISH, OS_OPT_POST_FLAG_SET, &rt_err);
}

void mqtt_cli_save(sl_cli_command_arg_t *args) {
  PP_UNUSED_PARAM(args);

  if (strlen(mqtt_broker_address) > 0)
    nvm3_writeData(nvm3_defaultHandle,
                   NVM3_KEY_MQTT_BROKER,
                   (void *)mqtt_broker_address,
                   sizeof(mqtt_broker_address));

  if (mqtt_broker_port != 0)
    nvm3_writeData(nvm3_defaultHandle,
                   NVM3_KEY_MQTT_PORT,
                   (void *)&mqtt_broker_port,
                   sizeof(mqtt_broker_port));

  if (strlen(mqtt_publish_topic) > 0)
    nvm3_writeData(nvm3_defaultHandle,
                   NVM3_KEY_MQTT_PUBLISH_TOPIC,
                   (void *)mqtt_publish_topic,
                   sizeof(mqtt_publish_topic));

  if (strlen(mqtt_subscribe_topic) > 0)
    nvm3_writeData(nvm3_defaultHandle,
                   NVM3_KEY_MQTT_SUBSCRIBE_TOPIC,
                   (void *)mqtt_subscribe_topic,
                   sizeof(mqtt_subscribe_topic));

  if (strlen(mqtt_username) > 0)
    nvm3_writeData(nvm3_defaultHandle,
                   NVM3_KEY_MQTT_USERNAME,
                   (void *)mqtt_username,
                   sizeof(mqtt_username));

  if (strlen(mqtt_password) > 0)
   nvm3_writeData(nvm3_defaultHandle,
                  NVM3_KEY_MQTT_PASSWORD,
                  (void *)mqtt_password,
                  sizeof(mqtt_password));

  if (strlen(ca_certificate) > 0)
    nvm3_writeData(nvm3_defaultHandle,
                   NVM3_KEY_MQTT_CA_CERTIFICATE,
                   (void *)ca_certificate,
                   sizeof(ca_certificate));

  if (strlen(device_certificate) > 0)
    nvm3_writeData(nvm3_defaultHandle,
                   NVM3_KEY_MQTT_DEVICE_CERTIFICATE,
                   (void *)device_certificate,
                   sizeof(device_certificate));

  if (strlen(device_key) > 0)
    nvm3_writeData(nvm3_defaultHandle,
                   NVM3_KEY_MQTT_DEVICE_KEY,
                   (void *)device_key,
                   sizeof(device_key));

  if (strlen(mqtt_client_id) > 0)
    nvm3_writeData(nvm3_defaultHandle,
                   NVM3_KEY_MQTT_CLIENT_ID,
                   (void *)mqtt_client_id,
                   sizeof(mqtt_client_id));
}

void mqtt_cli_get_info(sl_cli_command_arg_t *args){
  (void) args;
  printf("\r\n");
  printf("Broker address : %s \r\n", mqtt_broker_address);
  printf("Broker port    : %d \r\n", mqtt_broker_port);
  printf("Username       : %s \r\n", mqtt_username);
  printf("Password       : %s \r\n", mqtt_password);
  printf("Publish topic  : %s \r\n", mqtt_publish_topic);
  printf("Subscribe topic: %s \r\n", mqtt_subscribe_topic);
}

static void app_handle_certificate_line(char *arg_str, void *cer)
{
  if (!arg_str) {
    return;
  }

  if ((strlen(arg_str) == 1) && (arg_str[0] == '.')) {
    sl_cli_redirect_command(sl_cli_default_handle, NULL, NULL, NULL);

    /* copy data to input certificate. Then free the app_certificate */
    memcpy(cer, app_certificate_data(app_certificate), app_certificate->data_length);
    app_certificate_free(app_certificate);
  } else {
    app_certificate_append_string(app_certificate, arg_str);
  }
}

void mqtt_set_ca_certificate(sl_cli_command_arg_t *args)
{
  (void)args;
  app_certificate = app_certificate_init(APP_CERTIFICATE_MAX_SIZE);
  if (!app_certificate) {
    printf("[Failed: unable to store certificate]\r\n");
    return;
  }

  sl_cli_redirect_command(sl_cli_default_handle, app_handle_certificate_line, "Certificate> ", (void*) &ca_certificate[0]);

  printf("[Enter CA certificate line by line, followed by . (dot) on a separate line]\r\n");
}

void mqtt_set_device_certificate(sl_cli_command_arg_t *args)
{
  (void)args;
  app_certificate = app_certificate_init(APP_CERTIFICATE_MAX_SIZE);
  if (!app_certificate) {
    printf("[Failed: unable to store certificate]\r\n");
    return;
  }

  sl_cli_redirect_command(sl_cli_default_handle, app_handle_certificate_line, "Certificate> ", (void*) &device_certificate[0]);

  printf("[Enter device certificate line by line, followed by . (dot) on a separate line]\r\n");
}

void mqtt_set_device_private_key(sl_cli_command_arg_t *args)
{
  (void)args;
  app_certificate = app_certificate_init(APP_CERTIFICATE_MAX_SIZE);
  if (!app_certificate) {
    printf("[Failed: unable to store certificate]\r\n");
    return;
  }

  sl_cli_redirect_command(sl_cli_default_handle, app_handle_certificate_line, "Certificate> ", (void*) &device_key[0]);

  printf("[Enter device private key line by line, followed by . (dot) on a separate line]\r\n");
}
