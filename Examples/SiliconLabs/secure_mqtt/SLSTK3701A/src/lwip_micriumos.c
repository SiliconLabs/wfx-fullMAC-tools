/***************************************************************************//**
 * @file
 * @brief LwIP task and related functions
 *******************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
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


#include <bsp_os.h>
#include "bsp.h"
#include "ecode.h"
#include "dmadrv.h"
#include "em_msc.h"

#include <cpu/include/cpu.h>
#include <kernel/include/os.h>
#include <kernel/include/os_trace.h>
#include <common/include/common.h>
#include <common/include/lib_def.h>
#include <common/include/rtos_utils.h>
#include <common/include/toolchains.h>

#include "sl_wfx.h"
#include "string.h"
#include "sleep.h"

#include "demo_config.h"

// LwIP includes.
#include "lwip/dns.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/ip_addr.h"
#include "lwip/apps/httpd.h"
#include "lwip/apps/mqtt.h"
#include "lwip/netifapi.h"
#include "lwip/altcp_tls.h"
#ifdef LWIP_IPERF_SERVER
#include "lwip/ip_addr.h"
#include "lwip/apps/lwiperf.h"
#endif
#include "trng.h"
#include "console.h"
#include "dhcp_client.h"
#include "ethernetif.h"
#include "sl_wfx_host.h"
#include "sl_wfx_task.h"
#include "sl_wfx_host.h"
#include "sl_wfx_host_events.h"

static int netif_config(void);
static int lwip_app_mqtt_connection(void);

#define LWIP_TASK_PRIO                        23u
#define LWIP_TASK_STK_SIZE                   800u

#define LWIP_APP_TX_TICK_PERIOD             1000u
#define LWIP_APP_ECHO_ENABLED                  1u
#define LWIP_APP_MQTT_KEEPALIVE_INTERVAL      10u // Seconds

/// LwIP station network interface structure.
struct netif sta_netif;
/// LwIP AP network interface structure.
struct netif ap_netif;

/// Enable or disable DHCP client for station.
static int use_dhcp_client = USE_DHCP_CLIENT_DEFAULT;

/// Station IP address octet 0.
uint8_t sta_ip_addr0 = STA_IP_ADDR0_DEFAULT;
/// Station IP address octet 1.
uint8_t sta_ip_addr1 = STA_IP_ADDR1_DEFAULT;
/// Station IP address octet 2.
uint8_t sta_ip_addr2 = STA_IP_ADDR2_DEFAULT;
/// Station IP address octet 3.
uint8_t sta_ip_addr3 = STA_IP_ADDR3_DEFAULT;

/// Station net mask octet 0.
uint8_t sta_netmask_addr0 = STA_NETMASK_ADDR0_DEFAULT;
/// Station net mask octet 1.
uint8_t sta_netmask_addr1 = STA_NETMASK_ADDR1_DEFAULT;
/// Station net mask octet 2.
uint8_t sta_netmask_addr2 = STA_NETMASK_ADDR2_DEFAULT;
/// Station net mask octet 3.
uint8_t sta_netmask_addr3 = STA_NETMASK_ADDR3_DEFAULT;

/// Station gateway IP octet 0.
uint8_t sta_gw_addr0 = STA_GW_ADDR0_DEFAULT;
/// Station gateway IP octet 1.
uint8_t sta_gw_addr1 = STA_GW_ADDR1_DEFAULT;
/// Station gateway IP octet 2.
uint8_t sta_gw_addr2 = STA_GW_ADDR2_DEFAULT;
/// Station gateway IP octet 3.
uint8_t sta_gw_addr3 = STA_GW_ADDR3_DEFAULT;

/// AP IP address octet 0.
uint8_t ap_ip_addr0 = AP_IP_ADDR0_DEFAULT;
/// AP IP address octet 1.
uint8_t ap_ip_addr1 = AP_IP_ADDR1_DEFAULT;
/// AP IP address octet 2.
uint8_t ap_ip_addr2 = AP_IP_ADDR2_DEFAULT;
/// AP IP address octet 3.
uint8_t ap_ip_addr3 = AP_IP_ADDR3_DEFAULT;

/// AP net mask octet 0.
uint8_t ap_netmask_addr0 = AP_NETMASK_ADDR0_DEFAULT;
/// AP net mask octet 1.
uint8_t ap_netmask_addr1 = AP_NETMASK_ADDR1_DEFAULT;
/// AP net mask octet 2.
uint8_t ap_netmask_addr2 = AP_NETMASK_ADDR2_DEFAULT;
/// AP net mask octet 3.
uint8_t ap_netmask_addr3 = AP_NETMASK_ADDR3_DEFAULT;

/// AP gateway IP octet 0.
uint8_t ap_gw_addr0 = AP_GW_ADDR0_DEFAULT;
/// AP gateway IP octet 1.
uint8_t ap_gw_addr1 = AP_GW_ADDR1_DEFAULT;
/// AP gateway IP octet 2.
uint8_t ap_gw_addr2 = AP_GW_ADDR2_DEFAULT;
/// AP gateway IP octet 3.
uint8_t ap_gw_addr3 = AP_GW_ADDR3_DEFAULT;


/// LwIP task stack
static CPU_STK lwip_task_stk[LWIP_TASK_STK_SIZE];
/// LwIP task TCB
static OS_TCB lwip_task_tcb;
/// Memory to store an event to display in the web page
char event_log[50];

/// MQTT
static ip_addr_t mqtt_broker_ip;
static uint16_t mqtt_broker_port;
static char mqtt_username[256];
static char mqtt_password[256];
static char mqtt_publish_topic[128];
static char mqtt_subscribe_topic[128];
static bool mqtt_is_session_encrypted = true;
static bool mqtt_is_session_protected = false;
static mqtt_client_t *mqtt_client = NULL;
static char mqtt_client_id[32];
static struct mqtt_connect_client_info_t mqtt_client_info;

#ifdef EFM32GG11B820F2048GM64
static const char button_json_object[] = "{\"name\":\"PB%u\"}";
static console_config_t console_config = {
  .usart_instance = USART0,
  .dma_peripheral_signal = dmadrvPeripheralSignal_USART0_RXDATAV,
  .echo = 1
};
#else
static const char button_json_object[] = "{\"name\":\"BTN%u\"}";
static console_config_t console_config = {
  .usart_instance = USART4,
  .dma_peripheral_signal = dmadrvPeripheralSignal_USART4_RXDATAV,
  .echo = 1
};
#endif

static const char led_json_object[] = "{\"name\":\"LED%u\",\"state\":\"%s\"}";

/// TLS
#define CA_CERTIFICATE_ADDR       (FLASH_BASE + FLASH_SIZE - 3*FLASH_PAGE_SIZE)
#define DEVICE_CERTIFICATE_ADDR   (CA_CERTIFICATE_ADDR + FLASH_PAGE_SIZE)
#define DEVICE_KEY_ADDR           (DEVICE_CERTIFICATE_ADDR + FLASH_PAGE_SIZE)

static const char *ca_certificate = (char *)CA_CERTIFICATE_ADDR;
static const char *device_certificate = (char *)DEVICE_CERTIFICATE_ADDR;
static const char *device_key = (char *)DEVICE_KEY_ADDR;

static char input_buf[2048]; // Buffer with sufficient size to contain 2048bits certificates/keys

#ifdef LWIP_IPERF_SERVER
/***************************************************************************//**
 * @brief Function to handle iperf results report
 ******************************************************************************/
static void lwip_iperf_results(void *arg, enum lwiperf_report_type report_type,
                               const ip_addr_t* local_addr, u16_t local_port, const ip_addr_t* remote_addr, u16_t remote_port,
                               u32_t bytes_transferred, u32_t ms_duration, u32_t bandwidth_kbitpsec)
{
  printf("\r\nIperf Server Report:\r\n");
  printf("Interval %d.%d sec\r\n", (int)(ms_duration / 1000), (int)(ms_duration % 1000));
  printf("Bytes transferred %d.%dMBytes\r\n", (int)(bytes_transferred / 1024 / 1024), (int)((((bytes_transferred / 1024) * 1000) / 1024) % 1000));
  printf("Bandwidth %d.%d Mbits/sec\r\n\r\n", (int)(bandwidth_kbitpsec / 1024), (int)(((bandwidth_kbitpsec * 1000) / 1024) % 1000));
}

#endif

/**************************************************************************//**
 * Manage a MQTT subscription result.
 *****************************************************************************/
static void mqtt_subscribe_cb (void *arg, err_t err)
{
  (void)arg;

  if (err == 0) {
    printf("Subscribe success\r\n");
  } else {
    printf("Subscribe error: %d\r\n", err);
  }
}

/**************************************************************************//**
 * Manage a MQTT publishing result.
 *****************************************************************************/
static void mqtt_publish_cb (void *arg, err_t err)
{
  (void)arg;

  if (err != 0) {
    printf("Publish error: %d\r\n", err);
  }
}

/**************************************************************************//**
 * Manage a MQTT incoming data.
 *****************************************************************************/
static void mqtt_incoming_publish_cb (void *arg,
                                      const char *topic,
                                      uint32_t tot_len)
{
  (void)arg;
  printf("Incoming data: topic %s, size %lu\r\n", topic, tot_len);
}

static void mqtt_incoming_data_cb (void *arg,
                                   const uint8_t *data,
                                   uint16_t len,
                                   uint8_t flags)
{
  char buf[32];
  char state[4];
  int res;
  unsigned int led_id;

  (void)arg;
  (void)flags;

  // Limit the input data size
  strncpy(buf, (char*)data, sizeof(buf));
  if (len < sizeof(buf)) {
    buf[len] = '\0';
  }

  // Extract the data information
  res = sscanf(buf, led_json_object, &led_id, state);
  if (res == 2) {
    if (led_id < BSP_NO_OF_LEDS) {
      // sscanf captures everything up to the nul termination
      if (strcmp(state, "On\"}") == 0) {
        BSP_LedSet(led_id);
      } else if (strcmp(state, "Off\"}") == 0) {
        BSP_LedClear(led_id);
      }
    } else {
      printf("Wrong led id\n");
    }
  } else {
    printf("Wrong JSON format\n");
  }
}

/**************************************************************************//**
 * Manage a MQTT connection change of state.
 *****************************************************************************/
static void mqtt_connection_cb (mqtt_client_t *client,
                                void *arg,
                                mqtt_connection_status_t status)
{
  RTOS_ERR err;

  (void)client;
  (void)arg;

  if (status == MQTT_CONNECT_ACCEPTED) {
    printf("Connection success\r\n");
  } else {
    printf("Disconnection(%d)\r\n", status);
  }

  OSTaskSemPost(&lwip_task_tcb, OS_OPT_POST_NONE, &err);
}

/**************************************************************************//**
 * Prompt the user to provide the Wi-Fi configuration.
 *****************************************************************************/
static void lwip_app_wifi_config_prompt (void)
{
  char buf[4];
  int res;
  bool error;

#ifdef SLEEP_ENABLED
  SLEEP_SleepBlockBegin(sleepEM2);
#endif

  do {
      printf("\nEnter the SSID of the AP you want to connect:\n");
      res = console_get_line(wlan_ssid, sizeof(wlan_ssid));
  } while (res < 0);

  do {
    error = false;
    printf("Select a security mode:\n1. Open\n2. WEP\n3. WPA1 or WPA2\n4. WPA2\nEnter 1,2,3 or 4:\n");
    res = console_get_line(buf, sizeof(buf));

    if (res <= 0) {
      // Invalid input
      error = true;
      continue;
    }

    if (!strncmp(buf, "1", 1)) {
      wlan_security = WFM_SECURITY_MODE_OPEN;
    } else if (!strncmp(buf, "2", 1)) {
      wlan_security = WFM_SECURITY_MODE_WEP;
    } else if (!strncmp(buf, "3", 1)) {
      wlan_security = WFM_SECURITY_MODE_WPA2_WPA1_PSK;
    } else if (!strncmp(buf, "4", 1)) {
      wlan_security = WFM_SECURITY_MODE_WPA2_PSK;
    } else {
      printf("Unknown value\n");
      error = true;
    }
  } while (error);

  if (wlan_security != WFM_SECURITY_MODE_OPEN) {
    do {
      printf("Enter the Passkey of the AP you want to connect (8-chars min):\n");
      res = console_get_line(wlan_passkey, sizeof(wlan_passkey));
    } while (res < 8);
  }

  printf("\n");

#ifdef SLEEP_ENABLED
  SLEEP_SleepBlockEnd(sleepEM2);
#endif
}

/**************************************************************************//**
 * Connect to a Wi-Fi AP and wait until an IP address is set.
 *****************************************************************************/
static void lwip_app_wifi_connection (void)
{
  RTOS_ERR err;

  printf("Waiting for the Wi-Fi connection...\r\n");
  // Connect to the Wi-Fi AP
  sl_wfx_send_join_command((uint8_t*) wlan_ssid,
                           strlen(wlan_ssid),
                           NULL,
                           0,
                           wlan_security,
                           1,
                           0,
                           (uint8_t*) wlan_passkey,
                           strlen(wlan_passkey),
                           NULL,
                           0);

  // Wait for the connection establishment
  OSFlagPend(&wifi_events,
             SL_WFX_EVENT_CONNECT,
             0,
             OS_OPT_PEND_FLAG_SET_ANY | OS_OPT_PEND_BLOCKING | OS_OPT_PEND_FLAG_CONSUME,
             0,
             &err);
  if (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) {
    // Wait for the IP address binding if necessary
    while (use_dhcp_client && dhcp_supplied_address(&sta_netif) == 0) {
      OSTimeDly(500, OS_OPT_TIME_DLY, &err);
    }
  } else {
    printf("Wait error %d\r\n", err.Code);
  }
}

/**************************************************************************//**
 * Manage a DNS response.
 *****************************************************************************/
static void dns_found_cb (const char *name,
                          const ip_addr_t *ipaddr,
                          void *callback_arg)
{
  RTOS_ERR err;

  (void)callback_arg;

  OSTaskQPost(&lwip_task_tcb,
              (void *)ipaddr,
              sizeof(ip_addr_t),
              OS_OPT_POST_NONE,
              &err);
}

/**************************************************************************//**
 * Convert a TLS certificate/key end of lines to suit MBEDTLS parsing.
 *****************************************************************************/
static void lwip_app_convert_keys_eol (void)
{
  char *ptr_buf = input_buf;
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
 * Callback checking that a TLS certificate/key (in PEM format) is
 * completely received from the console.
 *****************************************************************************/
static bool lwip_app_is_complete_key_received_cb (char *buffer_pos)
{
  // Check for the end of a certificate or key
  return (strstr(buffer_pos, "-----END ") != NULL);
}

/**************************************************************************//**
 * Retrieve a TLS certificate/key from the UART using DMA.
 * @return -1: error
 *         0 <=: the key size
 *****************************************************************************/
static int lwip_app_get_keys (void)
{
  int res;

  res = console_get_lines(input_buf,
                          sizeof(input_buf),
                          lwip_app_is_complete_key_received_cb);

  if (res > 0) {
    // Convert the end of line characters if needed to suit the mbedtls parsing
    lwip_app_convert_keys_eol();
  }

  return res;
}

/**************************************************************************//**
 * Retrieve a TLS certificate/key and store it.
 *
 * @param flash_address Flash address where to store the certificate/key.
 *
 * @return 0 on success, 0> otherwise
 *****************************************************************************/
static int lwip_app_retrieve_and_store_key (uint32_t flash_address)
{
  uint32_t value;
  int res = -1;
  int len;
  uint8_t align;

  // Retrieve the key
  len = lwip_app_get_keys();

  // Erase the page where the key is going to be stored
  res = MSC_ErasePage((uint32_t *)flash_address);
  if (res == 0) {
    // Ensure the data is a multiple of 4 and write it in the flash
    align = len % 4;
    res = MSC_WriteWord((uint32_t *)flash_address, input_buf, len - align);
    if ((res == 0) && align) {
      // Write the remaining bytes
      memcpy(&value, &input_buf[len - align], align);
      res = MSC_WriteWord((uint32_t *)flash_address + (len-align)/4,
                          &value,
                          sizeof(value));
    }

    if (res != 0) {
      printf("Key: write error\r\n");
    }
  } else {
    printf("Key: erase error\r\n");
  }

  return res;
}

/**************************************************************************//**
 * Prompt the user to provide the MQTT configuration.
 *****************************************************************************/
static void lwip_app_mqtt_config_prompt (void)
{
  long long port;
  ip_addr_t *ipaddr;
  char *ptr_end;
  RTOS_ERR err;
  uint16_t len;
  int res;
  bool error;

#ifdef SLEEP_ENABLED
  SLEEP_SleepBlockBegin(sleepEM2);
#endif

  printf("\n");

  do {
    error = false;
    printf("Enter the MQTT broker address:\n");
    res = console_get_line(input_buf, sizeof(input_buf));
    if (res <= 0) {
      // Input invalid
      error = true;
      continue;
    }

    // Execute a DNS request
    res = dns_gethostbyname(input_buf, &mqtt_broker_ip, dns_found_cb, NULL);
    switch (res) {
      case ERR_OK:
        printf("\n");
        break;

      case ERR_INPROGRESS:
        // Wait for the DNS resolution
        ipaddr = (ip_addr_t*)OSTaskQPend(0,
                                         OS_OPT_PEND_BLOCKING,
                                         &len,
                                         NULL,
                                         &err);

        if (ipaddr != NULL) {
          printf("\nHostname %s resolved\n", input_buf);
          mqtt_broker_ip = *ipaddr;
        } else {
          printf("\nHostname %s resolution error\n", input_buf);
          error = true;
        }
        break;

      case ERR_ARG:
        printf("Wrong hostname\n");
        error = true;
        break;
    }
  } while (error);

  do {
    error = false;
    printf("Enter the MQTT port:\n");
    res = console_get_line(input_buf, sizeof(input_buf));
    if (res <= 0) {
      // Input invalid
      error = true;
      continue;
    }

    port = strtoll(input_buf, &ptr_end, 0);
    if (((ptr_end != NULL) && (*ptr_end != '\0'))
        || (port > USHRT_MAX)){
      error = true;
    } else {
      mqtt_broker_port = port;
    }
  } while (error);

  do {
    error = false;
    printf("Enter the MQTT client Id (%d-chars max):\n",
           sizeof(mqtt_client_id)-1);
    res = console_get_line(mqtt_client_id, sizeof(mqtt_client_id));
    if (res <= 0) {
      // Input invalid
      error = true;
      continue;
    }
  } while (error);

  do {
    error = false;
    printf("Enter the MQTT publish topic (%d-chars max):\n",
           sizeof(mqtt_publish_topic)-1);
    res = console_get_line(mqtt_publish_topic, sizeof(mqtt_publish_topic));
    if (res <= 0) {
      // Input invalid
      error = true;
      continue;
    }
  } while (error);

  do {
    error = false;
    printf("Enter the MQTT subscribe topic (%d-chars max):\n",
           sizeof(mqtt_subscribe_topic)-1);
    res = console_get_line(mqtt_subscribe_topic, sizeof(mqtt_subscribe_topic));
    if (res <= 0) {
      // Input invalid
      error = true;
      continue;
    }
  } while (error);

  printf("Do you need to configure a user/password ? [y/n]\n");
  console_get_line(input_buf, sizeof(input_buf));

  if ((input_buf[0] == 'y') || (input_buf[0] == 'Y')) {
    mqtt_is_session_protected = true;

    do {
      error = false;
      printf("Enter the MQTT session username (%d-chars max):\n",
             sizeof(mqtt_username)-1);

      res = console_get_line(input_buf, sizeof(input_buf));
      if ((res < 0)
          || (res >= sizeof(mqtt_username)-1)) {
        // Input invalid
        error = true;
      } else {
        strcpy(mqtt_username, input_buf);
      }
    } while (error);

    do {
      error = false;
      printf("Enter the MQTT session password (%d-chars max):\n",
             sizeof(mqtt_password)-1);

      res = console_get_line(input_buf, sizeof(input_buf));
      if ((res < 0)
          || (res >= sizeof(mqtt_password)-1)) {
        // Input invalid
        error = true;
      } else {
        strcpy(mqtt_password, input_buf);
      }
    } while (error);
  }

  printf("Is this an encrypted MQTT session ? [y/n]\n");
  console_get_line(input_buf, sizeof(input_buf));

  if ((input_buf[0] == 'y') || (input_buf[0] == 'Y')) {
    mqtt_is_session_encrypted = true;

    printf("\nPress <Enter> within 5 seconds to load the TLS keys:\n");
    res = console_get_line_tmo(input_buf, sizeof(input_buf), 5);

    if (res == 0 /*Empty input*/) {
      printf("Enter the root CA authenticating the server:\n");
      lwip_app_retrieve_and_store_key(CA_CERTIFICATE_ADDR);

      printf("\nEnter the device certificate:\n");
      lwip_app_retrieve_and_store_key(DEVICE_CERTIFICATE_ADDR);

      printf("\nEnter the device key:\n");
      lwip_app_retrieve_and_store_key(DEVICE_KEY_ADDR);
    }

  }

  printf("\n");

#ifdef SLEEP_ENABLED
  SLEEP_SleepBlockEnd(sleepEM2);
#endif
}

/**************************************************************************//**
 * Initialize the MQTT resources.
 *****************************************************************************/
static int lwip_app_mqtt_initialization (void)
{
  int ret = 0;

  // Create a new MQTT client
  mqtt_client = mqtt_client_new();
  if (mqtt_client == NULL) {
    // Error
    ret = -1;
  }

  // Add MQTT context information
  mqtt_client_info.client_id = mqtt_client_id;
  mqtt_client_info.keep_alive = LWIP_APP_MQTT_KEEPALIVE_INTERVAL;

  if (mqtt_is_session_protected) {
    mqtt_client_info.client_user = mqtt_username;

    // Update the password if defined by the user
    if (strlen(mqtt_password) > 0) {
      mqtt_client_info.client_pass = mqtt_password;
    }
  }

  if (mqtt_is_session_encrypted) {
#ifdef SLEEP_ENABLED
    // Ensure the TRNG peripheral is started before creating the context.
    // Indeed it is stopped when going to sleep (EM2).
    mbedtls_trng_context trng_ctx;
    mbedtls_trng_init(&trng_ctx);
#endif

    // Create a TLS context for the client
    mqtt_client_info.tls_config =
        altcp_tls_create_config_client_2wayauth((uint8_t *)ca_certificate,
                                                strlen(ca_certificate)+1,
                                                (uint8_t *)device_key,
                                                strlen(device_key)+1,
                                                NULL,
                                                0,
                                                (uint8_t *)device_certificate,
                                                strlen(device_certificate)+1);

    if (mqtt_client_info.tls_config == NULL) {
      // Error
      ret = -1;
    }
  }

  return ret;
}

/**************************************************************************//**
 * Connect to a MQTT broker.
 *****************************************************************************/
static int lwip_app_mqtt_connection (void)
{
  uint8_t *ptr_ip = (uint8_t *)&mqtt_broker_ip.addr;
  int ret = -1;
  RTOS_ERR err;

  if (mqtt_client != NULL) {
    printf("Connecting to MQTT broker (%u.%u.%u.%u)...\r\n",
           ptr_ip[0], ptr_ip[1], ptr_ip[2], ptr_ip[3]);

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
 * Publish a MQTT message.
 *****************************************************************************/
static int lwip_app_mqtt_publish (char *publish_topic,
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
 * Subscribe to MQTT topic.
 *****************************************************************************/
static int lwip_app_mqtt_subscribe (char *subscribe_topic,
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
 * Button handler
 *
 * @param button_id Button Id related to the event
 *****************************************************************************/
void lwip_button_handler (uint32_t button_id)
{
  RTOS_ERR err;

  if ((button_id == 0)
      || (button_id == 1)) {
    // Toggle the associated LED as a life indicator
    BSP_LedToggle(button_id);

    // Notify the thread to publish a message if the client is connected
    if ((mqtt_client != NULL)
        && (mqtt_client_is_connected(mqtt_client))) {
      OSTaskQPost(&lwip_task_tcb,
                  (void *)button_id,
                  sizeof(button_id),
                  OS_OPT_POST_NONE,
                  &err);
    }
  }
}

/***************************************************************************//**
 * Start LwIP task(s).
 *
 * @param p_arg Unused parameter.
 ******************************************************************************/
static void lwip_task(void *p_arg)
{
  ip_addr_t prim_dns_addr = IPADDR4_INIT_BYTES(8, 8, 8, 8);
  ip_addr_t sec_dns_addr = IPADDR4_INIT_BYTES(8, 8, 4, 4);
  void *evt_data = NULL;
  char message[128];
  char tmp[32];
  int res, len;
  OS_TICK tick, timeout = LWIP_APP_TX_TICK_PERIOD;
  RTOS_ERR err;

  // Create tcp_ip stack thread
  tcpip_init(NULL, NULL);

  if (use_dhcp_client) {
    /* Start DHCP Client */
    dhcpclient_start();
  }

  // Initialize the LwIP stack
  res = netif_config();
  if (res != 0) {
    // Error, no need to go further
    while(1);
  }

#ifdef LWIP_IPERF_SERVER
  lwiperf_start_tcp_server_default(lwip_iperf_results, 0);
#endif

  // Initialize the DNS client (connecting to Google DNS servers)
  dns_setserver(0, &prim_dns_addr);
  dns_setserver(0, &sec_dns_addr);
  dns_init();

  // Initialize the console
  res = console_init(&console_config);
  LWIP_ASSERT("Console: init error", res == 0);

  // Prompt the user to configure the Wi-Fi connection
  lwip_app_wifi_config_prompt();

  // Realize the Wi-Fi connection
  lwip_app_wifi_connection();

  // Prompt the user to configure the MQTT connection
  lwip_app_mqtt_config_prompt();

  // Initialize the MQTT client
  res = lwip_app_mqtt_initialization();
  LWIP_ASSERT("MQTT: init error", res == 0);

  // Infinite loop, publishing data periodically and on event
  for (;; ) {
    if (mqtt_client_is_connected(mqtt_client) == 0) {
      // MQTT client not connected, (re)connect to the broker

      res = lwip_app_mqtt_connection();
      if (res == 0) {
        // Connection success
        lwip_app_mqtt_subscribe(mqtt_subscribe_topic,
                                0 /*qos*/,
                                mqtt_subscribe_cb,
                                mqtt_incoming_publish_cb,
                                mqtt_incoming_data_cb);

        // Compute the next publishing period
        tick = OSTimeGet(&err);
        timeout = LWIP_APP_TX_TICK_PERIOD - (tick % LWIP_APP_TX_TICK_PERIOD);
      } else {
        // Wait a while before trying to reconnect
        timeout = 1000;
      }
    } else {
      // MQTT client connected, publish some data

      if (evt_data == NULL) {
        // Periodical TX, prepare a message containing the LED states

        //{"leds":[{"name":"LED0","state":"Off"},{"name":"LED1","state":"On"}]}
        len = snprintf(message, sizeof(message), "{\"leds\":[");
        for (int i=0; i<BSP_NO_OF_LEDS; i++) {
          len += snprintf(tmp,
                          sizeof(tmp),
                          led_json_object,
                          i,
                          BSP_LedGet(i) ? "On" : "Off");
          strcat(tmp, ","); len++;
          strcat(message, tmp);
        }
        strcpy(&message[len-1] /*Overwrite the last comma*/, "]}"); len++ /*-1 + 2*/;
      } else {
        // An event occurred, prepare a message containing the source event

        //{"name":"BTN1"}
        len = snprintf(message,
                       sizeof(message),
                       button_json_object,
                       (uint16_t)((uint32_t)evt_data));
      }

      // Publish the prepared message
      lwip_app_mqtt_publish(mqtt_publish_topic, message,
                            1 /*qos*/,
                            0 /*retain*/,
                            mqtt_publish_cb);

      // Compute the next publishing period
      tick = OSTimeGet(&err);
      timeout = LWIP_APP_TX_TICK_PERIOD - (tick % LWIP_APP_TX_TICK_PERIOD);
    }

    // Wait for next event or timeout
    evt_data = OSTaskQPend(timeout,
                           OS_OPT_PEND_BLOCKING,
                           (uint16_t *)&len,
                           NULL,
                           &err);
  }
}
/**************************************************************************//**
 * Set station link status to up.
 *****************************************************************************/
sl_status_t lwip_set_sta_link_up(void)
{
  netifapi_netif_set_up(&sta_netif);
  netifapi_netif_set_link_up(&sta_netif);
  if (use_dhcp_client) {
    dhcpclient_set_link_state(1);
  }
  return SL_STATUS_OK;
}
/**************************************************************************//**
 * Set station link status to down.
 *****************************************************************************/
sl_status_t lwip_set_sta_link_down(void)
{
  if (use_dhcp_client) {
    dhcpclient_set_link_state(0);
  }
  netifapi_netif_set_link_down(&sta_netif);
  netifapi_netif_set_down(&sta_netif);
  return SL_STATUS_OK;
}
/**************************************************************************//**
 * Set AP link status to up.
 *****************************************************************************/
sl_status_t lwip_set_ap_link_up(void)
{
  netifapi_netif_set_up(&ap_netif);
  netifapi_netif_set_link_up(&ap_netif);
  return SL_STATUS_OK;
}

/**************************************************************************//**
 * Enable DHCP client.
 *****************************************************************************/
void lwip_enable_dhcp_client(void)
{
  use_dhcp_client = 1;
}

/**************************************************************************//**
 * Disable DHCP client.
 *****************************************************************************/
void lwip_disable_dhcp_client(void)
{
  use_dhcp_client = 0;
}

/**************************************************************************//**
 * Set AP link status to down.
 *****************************************************************************/
sl_status_t lwip_set_ap_link_down(void)
{
  netifapi_netif_set_link_down(&ap_netif);
  netifapi_netif_set_down(&ap_netif);
  return SL_STATUS_OK;
}
/***************************************************************************//**
 * Initializes LwIP network interface.
 ******************************************************************************/
static int netif_config(void)
{
  sl_status_t status;
  ip_addr_t sta_ipaddr, ap_ipaddr;
  ip_addr_t sta_netmask, ap_netmask;
  ip_addr_t sta_gw, ap_gw;
  int ret = -1;

  if (use_dhcp_client) {
    ip_addr_set_zero_ip4(&sta_ipaddr);
    ip_addr_set_zero_ip4(&sta_netmask);
    ip_addr_set_zero_ip4(&sta_gw);
  } else {
    IP_ADDR4(&sta_ipaddr, sta_ip_addr0, sta_ip_addr1, sta_ip_addr2, sta_ip_addr3);
    IP_ADDR4(&sta_netmask, sta_netmask_addr0, sta_netmask_addr1, sta_netmask_addr2, sta_netmask_addr3);
    IP_ADDR4(&sta_gw, sta_gw_addr0, sta_gw_addr1, sta_gw_addr2, sta_gw_addr3);
  }
  /* Initialize the SoftAP information */
  IP_ADDR4(&ap_ipaddr, ap_ip_addr0, ap_ip_addr1, ap_ip_addr2, ap_ip_addr3);
  IP_ADDR4(&ap_netmask, ap_netmask_addr0, ap_netmask_addr1, ap_netmask_addr2, ap_netmask_addr3);
  IP_ADDR4(&ap_gw, ap_gw_addr0, ap_gw_addr1, ap_gw_addr2, ap_gw_addr3);

  /* Initialize the WF200 used by the two interfaces */
  status = sl_wfx_init(&wifi);
  printf("FMAC Driver version    %s\r\n", FMAC_DRIVER_VERSION_STRING);
  switch (status) {
    case SL_STATUS_OK:
      wifi.state = SL_WFX_STARTED;
      printf("WF200 Firmware version %d.%d.%d\r\n",
             wifi.firmware_major,
             wifi.firmware_minor,
             wifi.firmware_build);
      printf("WF200 initialization successful\r\n");
      ret = 0;
      break;
    case SL_STATUS_WIFI_INVALID_KEY:
      printf("Failed to init WF200: Firmware keyset invalid\r\n");
      break;
    case SL_STATUS_WIFI_FIRMWARE_DOWNLOAD_TIMEOUT:
      printf("Failed to init WF200: Firmware download timeout\r\n");
      break;
    case SL_STATUS_TIMEOUT:
      printf("Failed to init WF200: Poll for value timeout\r\n");
      break;
    case SL_STATUS_FAIL:
      printf("Failed to init WF200: Error\r\n");
      break;
    default:
      printf("Failed to init WF200: Unknown error\r\n");
  }
#ifdef SLEEP_ENABLED
#ifdef SL_WFX_USE_SDIO
  status = sl_wfx_host_switch_to_wirq();
#endif
#endif
  // Add station and softAP interfaces
  netif_add(&sta_netif, &sta_ipaddr, &sta_netmask, &sta_gw, NULL, &sta_ethernetif_init, &tcpip_input);
  netif_add(&ap_netif, &ap_ipaddr, &ap_netmask, &ap_gw, NULL, &ap_ethernetif_init, &tcpip_input);

  // Registers the default network interface.
  netif_set_default(&sta_netif);

  return ret;
}

/**************************************************************************//**
 * Start LwIP task.
 *****************************************************************************/
sl_status_t lwip_start(void)
{
  RTOS_ERR err;

  OSTaskCreate(&lwip_task_tcb,
               "LWIP Task",
               lwip_task,
               DEF_NULL,
               LWIP_TASK_PRIO,
               &lwip_task_stk[0],
               (LWIP_TASK_STK_SIZE / 10u),
               LWIP_TASK_STK_SIZE,
               0u,
               0u,
               DEF_NULL,
               (OS_OPT_TASK_STK_CLR),
               &err);
  /*   Check error code.                                  */
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
  return SL_STATUS_OK;
}
