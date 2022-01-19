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
#ifndef MQTT_CLI_LWIP_H_
#define MQTT_CLI_LWIP_H_

#include "sl_wfx_cmd_api.h"
#include "sl_status.h"

#include "mqtt_cli_params.h"

#include "lwip/stats.h"
#include "lwip/tcpip.h"
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/inet.h"
#include "lwip/inet_chksum.h"
#include "lwip/ip.h"

#include "lwip/apps/mqtt.h"
#include "lwip/altcp_tls.h"

#define MQTT_KEEPALIVE_INTERVAL      240u // Seconds
#define MQTT_QOS                     (1)
#define MQTT_RETAIN                  (0)

/// MQTT
extern char mqtt_broker_address[128];
extern ip_addr_t mqtt_broker_ip;
extern uint16_t mqtt_broker_port;
extern char mqtt_username[256];
extern char mqtt_password[256];
extern char mqtt_publish_topic[128];
extern char mqtt_subscribe_topic[128];
extern bool mqtt_is_session_encrypted;
extern bool mqtt_is_session_protected;
extern char mqtt_msg_publish[256];  //maybe refactor this one later

extern char mqtt_client_id[32];
extern struct mqtt_connect_client_info_t mqtt_client_info;
extern mqtt_client_t *mqtt_client;
extern bool mqtt_connected;
/// TSL
extern char ca_certificate[2048];
extern char device_certificate[2048];
extern char device_key[2048];

int mqtt_cli_lwip_initialization (void);
int mqtt_cli_lwip_connection (void);
int mqtt_cli_lwip_subscribe (char *subscribe_topic,
                             u8_t qos,
                             mqtt_request_cb_t subscribe_cb,
                             mqtt_incoming_publish_cb_t incoming_pub_cb,
                             mqtt_incoming_data_cb_t incoming_data_cb);

int mqtt_cli_lwip_publish (char *publish_topic,
                                  char *message,
                                  u8_t qos,
                                  u8_t retain,
                                  mqtt_request_cb_t publish_cb);


#endif
