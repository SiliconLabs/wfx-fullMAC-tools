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
#include <stdint.h>
#include <string.h>

#include "ethernetif.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/apps/httpd.h"
#include "lwip/netifapi.h"
#include "dhcp_client.h"
#include "dhcp_server.h"
#include "lwip/dns.h"
#include "sl_wfx_task.h"
#include "sl_wfx_host.h"

#include "lwip/apps/mqtt.h"
#include "lwip/altcp_tls.h"

#include "mqtt_cli_lwip.h"
#include "mqtt_cli_app.h"

/// MQTT
char mqtt_broker_address[128];
ip_addr_t mqtt_broker_ip;
uint16_t mqtt_broker_port;
char mqtt_username[256];
char mqtt_password[256];
char mqtt_publish_topic[128];
char mqtt_subscribe_topic[128];
char mqtt_msg_publish[256];
char mqtt_client_id[32];
struct mqtt_connect_client_info_t mqtt_client_info;
mqtt_client_t *mqtt_client = NULL;
bool mqtt_connected = false;

///TLS
char ca_certificate[2048] = {0};
char device_certificate[2048] = {0};
char device_key[2048] = {0};



extern OS_TCB mqtt_cli_task_tcb;    //need to refactor this one

/*******************************************************************************
 *******************************   Prototypes   ********************************
 ******************************************************************************/
static int lwip_app_send_dns_request (char *domain_address, ip_addr_t *ipaddr);

static void dns_found_cb (const char *name,
                          const ip_addr_t *ipaddr,
                          void *callback_arg);

static int mqtt_cli_lwip_configuration (void);

static void lwip_app_convert_keys_eol (char *key_buf);
/**************************************************************************//**
 * Initialize the MQTT resources.
 *****************************************************************************/
int mqtt_cli_lwip_initialization (void)
{
  int ret = 0;

  ret = mqtt_cli_lwip_configuration();
  // Create a new MQTT client
  mqtt_client = mqtt_client_new();
  if (mqtt_client == NULL) {
    // Error
    ret = -1;
  }

  // Add MQTT context information
  mqtt_client_info.client_id = mqtt_client_id;
  mqtt_client_info.keep_alive = MQTT_KEEPALIVE_INTERVAL;
  if(strlen(mqtt_username) != 0)
    mqtt_client_info.client_user = mqtt_username;
  else
    mqtt_client_info.client_user = NULL;

  if(strlen(mqtt_password) != 0)
     mqtt_client_info.client_pass = mqtt_password;
   else
     mqtt_client_info.client_pass = NULL;

  return ret;
}

/**************************************************************************//**
 * Convert a TLS certificate/key end of lines to suit MBEDTLS parsing.
 *****************************************************************************/
static void lwip_app_convert_keys_eol (char *key_buf)
{
  char *ptr_buf = key_buf;
  char *ptr_eol;

  // Iterate through the key buffer to replace potential '\r'
  // characters by '\n'.
  while ((ptr_eol = strstr(ptr_buf, "\r")) != NULL) {
    if (*(ptr_eol+1) != '\n') {
      // Convert '\r' into '\n'
      *ptr_eol = '\n';
      ptr_buf = ptr_eol + 1;
    } else {
      // Assume the whole key is correctly formated
      break;
    }
  }
}

/**************************************************************************//**
 * configure mqtt information default
 *****************************************************************************/
static int mqtt_cli_lwip_configuration (void)
{
  int ret = 0;

  ///check configuration
  if ( (strlen(mqtt_broker_address) <= 0)
       || (mqtt_broker_port <= 0)
       || (strlen(mqtt_publish_topic) <= 0)
       || (strlen(mqtt_subscribe_topic) <= 0) ) {
          printf("Not enough information mqtt, please check configure again\r\n");
          return -1;
  }

  lwip_app_convert_keys_eol(ca_certificate);
  lwip_app_convert_keys_eol(device_key);
  lwip_app_convert_keys_eol(device_certificate);

  mqtt_client_info.tls_config =
     altcp_tls_create_config_client_2wayauth((uint8_t *)ca_certificate,
                                             strlen(ca_certificate)+1,
                                             (uint8_t *)device_key,
                                             strlen(device_key)+1,
                                             NULL,
                                             0,
                                             (uint8_t *)device_certificate,
                                             strlen(device_certificate)+1);
  if (mqtt_client_info.tls_config == NULL)
  {
    printf("MQTT configure failed, please check certificate files\r\n");
    return -1;
  }

  ///get broker ip
  lwip_app_send_dns_request(mqtt_broker_address, &mqtt_broker_ip);

  /// Display non-critical information
  printf("\nMQTT configuration:\n");
  printf("\tBroker domain: %s, port %u\n", mqtt_broker_address, mqtt_broker_port);
  printf("\tMQTT broker ip (%s)...\r\n", ipaddr_ntoa(&mqtt_broker_ip));
  printf("\tPublish topic: %s\n", mqtt_publish_topic);
  printf("\tSubscribe topic: %s\n", mqtt_subscribe_topic);

  return ret;

}


/**************************************************************************//**
 * Manage a MQTT connection change of state.
 *****************************************************************************/
static void mqtt_connection_cb (mqtt_client_t *client,
                                void *arg,
                                mqtt_connection_status_t status)
{
  RTOS_ERR err;

  PP_UNUSED_PARAM(client);
  PP_UNUSED_PARAM(arg);

  if (status == MQTT_CONNECT_ACCEPTED) {
      mqtt_connected = true;
      printf("Connection success\r\n");
  } else {
      mqtt_connected = false;
      printf("Disconnection(%d)\r\n", status);
  }

  OSTaskSemPost(&mqtt_cli_task_tcb, OS_OPT_POST_NONE, &err);
}

/**************************************************************************//**
 * Connect to a MQTT broker.
 *****************************************************************************/
int mqtt_cli_lwip_connection (void)
{
  int ret = -1;
  RTOS_ERR err;

  if (mqtt_client != NULL) {
    printf("Connecting to MQTT broker (%s)...\r\n", ipaddr_ntoa(&mqtt_broker_ip));

    OSTaskSemSet(NULL, 0, &err);

    LOCK_TCPIP_CORE();
    // Connect to the MQTT broker
    ret = mqtt_client_connect(mqtt_client,
                              &mqtt_broker_ip,
                              mqtt_broker_port,
                              mqtt_connection_cb,
                              NULL,
                              &mqtt_client_info);
    UNLOCK_TCPIP_CORE();

    if (ret == 0) {
      // Wait for the connection result
      OSTaskSemPend(0, OS_OPT_PEND_BLOCKING, NULL, &err);
    }
  }

  return ret;
}

/**************************************************************************//**
 * Subscribe to MQTT topic.
 *****************************************************************************/
int mqtt_cli_lwip_subscribe (char *subscribe_topic,
                                  u8_t qos,
                                  mqtt_request_cb_t subscribe_cb,
                                  mqtt_incoming_publish_cb_t incoming_pub_cb,
                                  mqtt_incoming_data_cb_t incoming_data_cb)
{
  err_t err = -1;

  if ((subscribe_topic != NULL)
      && (strlen(subscribe_topic) > 0)) {

    LOCK_TCPIP_CORE();
    mqtt_sub_unsub(mqtt_client,
                   subscribe_topic,
                   qos,
                   subscribe_cb,
                   NULL,
                   1);
    UNLOCK_TCPIP_CORE();

    mqtt_set_inpub_callback(mqtt_client,
                            incoming_pub_cb,
                            incoming_data_cb,
                            NULL);
  }

  return err;
}

/**************************************************************************//**
 * Publish a MQTT message.
 *****************************************************************************/
int mqtt_cli_lwip_publish (char *publish_topic,
                                  char *message,
                                  u8_t qos,
                                  u8_t retain,
                                  mqtt_request_cb_t publish_cb)
{
  uint16_t len = 0;
  err_t err;

  if (message != NULL) {
    len = strlen(message);
  }

  LOCK_TCPIP_CORE();
  err = mqtt_publish(mqtt_client,
                     publish_topic,
                     (void *)message,
                     len,
                     qos,
                     retain,
                     publish_cb,
                     NULL);
  UNLOCK_TCPIP_CORE();

  return err;
}
/**************************************************************************//**
 * Manage a DNS response.
 *****************************************************************************/
static void dns_found_cb (const char *name,
                          const ip_addr_t *ipaddr,
                          void *callback_arg)
{
  RTOS_ERR err;
  PP_UNUSED_PARAM(name);
  PP_UNUSED_PARAM(callback_arg);

  OSTaskQPost(&mqtt_cli_task_tcb,
              (void *)ipaddr,
              sizeof(ip_addr_t),
              OS_OPT_POST_NONE,
              &err);
}

/**************************************************************************//**
 * Send a DNS request.
 * @return -1: error
 *          0: success
 *****************************************************************************/
static int lwip_app_send_dns_request (char *domain_address, ip_addr_t *ipaddr)
{
  RTOS_ERR err;
  int res, ret = -1;
  ip_addr_t *addr;
  uint16_t len;

  // Execute a DNS request
  res = dns_gethostbyname(domain_address, ipaddr, dns_found_cb, NULL);
  switch (res) {
    case ERR_OK:
      // Resolution success (already in IP format)
      ret = 0;
      break;

    case ERR_INPROGRESS:
      // Wait for the DNS resolution
      addr = (ip_addr_t *)OSTaskQPend(0,
                                      OS_OPT_PEND_BLOCKING,
                                      &len,
                                      NULL,
                                      &err);

      if (addr != NULL) {
        // Resolution success
        *ipaddr = *addr;
        ret = 0;
      }
      break;

    case ERR_ARG:
      // Error, wrong hostname
      break;
  }

  if (ret == 0) {
    printf("DNS result: %s -> %s\n", domain_address, ipaddr_ntoa(ipaddr));
  } else {
    printf("DNS request error (%s)\n", domain_address);
  }

  return ret;
}


