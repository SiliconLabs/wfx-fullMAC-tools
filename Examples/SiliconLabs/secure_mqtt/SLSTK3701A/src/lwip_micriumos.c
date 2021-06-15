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
#include "nvm3.h"
#include "nvm3_hal_flash.h"

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

typedef struct {
  char *label;
  sl_wfx_security_mode_t mode;
} wifi_security_mode_t;

static const wifi_security_mode_t wlan_security_modes[] = {
  {"OPEN",          WFM_SECURITY_MODE_OPEN},
  {"WEP",           WFM_SECURITY_MODE_WEP},
  {"WPA1/WPA2/PSK", WFM_SECURITY_MODE_WPA2_WPA1_PSK},
  {"WPA2/PSK",      WFM_SECURITY_MODE_WPA2_PSK}
};


/// LwIP task stack
static CPU_STK lwip_task_stk[LWIP_TASK_STK_SIZE];
/// LwIP task TCB
static OS_TCB lwip_task_tcb;
/// Memory to store an event to display in the web page
char event_log[50];

/// MQTT
static char mqtt_broker_address[128];
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

static const char led_json_object[] = "{\"name\":\"LED%d\",\"state\":\"%3s\"}";

/// TLS
static char ca_certificate[2048] = {0};
static char device_certificate[2048] = {0};
static char device_key[2048] = {0};

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
      if (strcmp(state, "On\"") == 0) {
        BSP_LedSet(led_id);
      } else if (strcmp(state, "Off") == 0) {
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
  int res, val;
  bool error;
  bool reconfigure = true;

#ifdef SLEEP_ENABLED
  SLEEP_SleepBlockBegin(sleepEM2);
#endif

  // Look for an old Wi-Fi configuration
  res = nvm3_enumObjects(&nvm3_handle,
                         NULL, 0,
                         NVM3_KEY_AP_SSID, NVM3_KEY_AP_PASSKEY);
  if (res == NVM3_NB_MANDATORY_KEYS_WIFI) {
    // A configuration is stored, retrieve it
    nvm3_readData(&nvm3_handle, NVM3_KEY_AP_SSID, (void *)wlan_ssid, sizeof(wlan_ssid));
    nvm3_readData(&nvm3_handle, NVM3_KEY_AP_SECURITY_MODE, (void *)&wlan_security, sizeof(wlan_security));
    nvm3_readData(&nvm3_handle, NVM3_KEY_AP_PASSKEY, (void *)wlan_passkey, sizeof(wlan_passkey));

    // Display non critical information
    printf("\nWi-Fi configuration stored:\n");
    printf("\tSSID: %s\n", wlan_ssid);

    for (uint8_t i=0; i<(sizeof(wlan_security_modes)/sizeof(wifi_security_mode_t)); i++) {
      if (wlan_security_modes[i].mode == wlan_security) {
        printf("\tMode: %s\n", wlan_security_modes[i].label);
        break;
      }
    }

    printf("\nDo you want to change it ? [y/n]\n");
    console_get_line(buf, sizeof(buf));

    if ((buf[0] != 'y') && (buf[0] != 'Y')) {
      reconfigure = false;
    }
  }

  if (reconfigure) {
    // User needs or asked for a new configuration
    do {
      printf("\nEnter the SSID of the AP you want to connect:\n");
      res = console_get_line(wlan_ssid, sizeof(wlan_ssid));
    } while (res < 0);

    do {
      error = false;
      printf("Security modes:\n");
      for (uint8_t i=0; i<(sizeof(wlan_security_modes)/sizeof(wifi_security_mode_t)); i++) {
        printf("%d. %s\n", i+1, wlan_security_modes[i].label);
      }

      printf("Enter the number of the mode to select:\n");
      res = console_get_line(buf, sizeof(buf));

      if (res <= 0) {
        // Invalid input
        error = true;
        continue;
      }

      buf[2] = '\0';
      val = atoi(buf);
      switch (val) {
        case 1:
        case 2:
        case 3:
        case 4:
          wlan_security = wlan_security_modes[val-1].mode;
          break;

        default:
          printf("Unknown value\n");
          error = true;
          break;
      }
    } while (error);

    if (wlan_security != WFM_SECURITY_MODE_OPEN) {
      do {
        printf("Enter the Passkey of the AP you want to connect (8-chars min):\n");
        res = console_get_line(wlan_passkey, sizeof(wlan_passkey));
      } while (res < 8);
    } else {
      // Stop storing an old password
      memset(wlan_passkey, '\0', sizeof(wlan_passkey));
    }

    // Update the NVM fields with the new configuration
    nvm3_writeData(&nvm3_handle, NVM3_KEY_AP_SSID, (void *)wlan_ssid, sizeof(wlan_ssid));
    nvm3_writeData(&nvm3_handle, NVM3_KEY_AP_SECURITY_MODE, (void *)&wlan_security, sizeof(wlan_security));
    nvm3_writeData(&nvm3_handle, NVM3_KEY_AP_PASSKEY, (void *)wlan_passkey, sizeof(wlan_passkey));
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
static int lwip_app_get_keys (char *key_buf, uint16_t key_buf_size)
{
  int res;

  res = console_get_lines(key_buf,
                          key_buf_size,
                          lwip_app_is_complete_key_received_cb);

  if (res > 0) {
    // Convert the end of line characters if needed to suit the mbedtls parsing
    lwip_app_convert_keys_eol(key_buf);
  }

  return res;
}

/**************************************************************************//**
 * Prompt the user to provide the MQTT configuration.
 *****************************************************************************/
static void lwip_app_mqtt_config_prompt (void)
{
  long long port;
  char *ptr_end;
  char buf[256] = {0};
  Ecode_t ecode;
  int res;
  bool error;
  bool reconfigure = true;

#ifdef SLEEP_ENABLED
  SLEEP_SleepBlockBegin(sleepEM2);
#endif

  // Look for an old MQTT configuration
  res = nvm3_enumObjects(&nvm3_handle,
                         NULL, 0,
                         NVM3_KEY_MQTT_BROKER, NVM3_KEY_MQTT_PASSWORD);
  if (res >= NVM3_NB_MANDATORY_KEYS_MQTT) {
    // A configuration is troed, retrieve it
    nvm3_readData(&nvm3_handle, NVM3_KEY_MQTT_BROKER, (void *)mqtt_broker_address, sizeof(mqtt_broker_address));
    nvm3_readData(&nvm3_handle, NVM3_KEY_MQTT_PORT, (void *)&mqtt_broker_port, sizeof(mqtt_broker_port));
    nvm3_readData(&nvm3_handle, NVM3_KEY_MQTT_CLIENT_ID, (void *)mqtt_client_id, sizeof(mqtt_client_id));
    nvm3_readData(&nvm3_handle, NVM3_KEY_MQTT_PUBLISH_TOPIC, (void *)mqtt_publish_topic, sizeof(mqtt_publish_topic));
    nvm3_readData(&nvm3_handle, NVM3_KEY_MQTT_SUBSCRIBE_TOPIC, (void *)mqtt_subscribe_topic, sizeof(mqtt_subscribe_topic));

    // Display non-critical information
    printf("\nMQTT configuration stored:\n");
    printf("\tBroker: %s, port %u\n", mqtt_broker_address, mqtt_broker_port);
    printf("\tClient Id: %s\n", mqtt_client_id);
    printf("\tPublish topic: %s\n", mqtt_publish_topic);
    printf("\tSubscribe topic: %s\n", mqtt_subscribe_topic);

    if (res <= NVM3_NB_TOTAL_KEYS_MQTT) {
      ecode = nvm3_readData(&nvm3_handle, NVM3_KEY_MQTT_USERNAME, (void *)mqtt_username, sizeof(mqtt_username));
      if (ecode == ECODE_NVM3_OK) {
        nvm3_readData(&nvm3_handle, NVM3_KEY_MQTT_PASSWORD, (void *)mqtt_password, sizeof(mqtt_password));
        mqtt_is_session_protected = true;
      }

      ecode = nvm3_readData(&nvm3_handle, NVM3_KEY_MQTT_CA_CERTIFICATE, (void *)ca_certificate, sizeof(ca_certificate));
      if (ecode == ECODE_NVM3_OK) {
        nvm3_readData(&nvm3_handle, NVM3_KEY_MQTT_DEVICE_CERTIFICATE, (void *)device_certificate, sizeof(device_certificate));
        nvm3_readData(&nvm3_handle, NVM3_KEY_MQTT_DEVICE_KEY, (void *)device_key, sizeof(device_key));
        mqtt_is_session_encrypted = true;
      }

      printf("\tSession: %sencrypted, %sprotected by a credentials\n",
             mqtt_is_session_encrypted ? "" : "not ",
             mqtt_is_session_protected ? "" : "not ");
    }

    printf("\nDo you want to change it ? [y/n]\n");
    console_get_line(buf, sizeof(buf));

    if ((buf[0] != 'y') && (buf[0] != 'Y')) {
      reconfigure = false;
      // Broker address not stored directly, execute the DNS request to retrieve it
      lwip_app_send_dns_request(mqtt_broker_address, &mqtt_broker_ip);
    }
  }

  if (reconfigure) {
    // User needs or asked for a new configuration
    do {
      error = false;
      printf("\nEnter the MQTT broker address:\n");
      res = console_get_line(mqtt_broker_address, sizeof(mqtt_broker_address));
      if (res <= 0) {
        // Input invalid
        error = true;
        continue;
      }

      // Ensure the address validity
      res = lwip_app_send_dns_request(mqtt_broker_address, &mqtt_broker_ip);
      if (res != 0) {
        error = true;
      }
    } while (error);

    do {
      error = false;
      printf("Enter the MQTT port:\n");
      res = console_get_line(buf, sizeof(buf));
      if (res <= 0) {
        // Input invalid
        error = true;
        continue;
      }

      port = strtoll(buf, &ptr_end, 0);
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
    console_get_line(buf, sizeof(buf));

    if ((buf[0] == 'y') || (buf[0] == 'Y')) {
      mqtt_is_session_protected = true;

      do {
        error = false;
        printf("Enter the MQTT session username (%d-chars max):\n",
               sizeof(mqtt_username)-1);

        res = console_get_line(buf, sizeof(buf));
        if ((res < 0)
            || (res >= sizeof(mqtt_username)-1)) {
          // Input invalid
          error = true;
        } else {
          strcpy(mqtt_username, buf);
        }
      } while (error);

      do {
        error = false;
        printf("Enter the MQTT session password (%d-chars max):\n",
               sizeof(mqtt_password)-1);

        res = console_get_line(buf, sizeof(buf));
        if ((res < 0)
            || (res >= sizeof(mqtt_password)-1)) {
          // Input invalid
          error = true;
        } else {
          strcpy(mqtt_password, buf);
        }
      } while (error);
    } else {
      // Stop storing an old username/password
      memset(mqtt_username, '\0', sizeof(mqtt_username));
      memset(mqtt_password, '\0', sizeof(mqtt_password));
    }

    printf("Is this an encrypted MQTT session ? [y/n]\n");
    console_get_line(buf, sizeof(buf));

    if ((buf[0] == 'y') || (buf[0] == 'Y')) {
      mqtt_is_session_encrypted = true;

      printf("Enter the root CA authenticating the server:\n");
      lwip_app_get_keys(ca_certificate, sizeof(ca_certificate));

      printf("\nEnter the device certificate:\n");
      lwip_app_get_keys(device_certificate, sizeof(device_certificate));

      printf("\nEnter the device key:\n");
      lwip_app_get_keys(device_key, sizeof(device_key));
    } else {
      // Stop storing an old certificates/keys
      memset(ca_certificate, '\0', sizeof(ca_certificate));
      memset(device_certificate, '\0', sizeof(device_certificate));
      memset(device_key, '\0', sizeof(device_key));
    }

    // Update the NVM fields with the new configuration
    nvm3_writeData(&nvm3_handle, NVM3_KEY_MQTT_BROKER, (void *)mqtt_broker_address, sizeof(mqtt_broker_address));
    nvm3_writeData(&nvm3_handle, NVM3_KEY_MQTT_PORT, (void *)&mqtt_broker_port, sizeof(mqtt_broker_port));
    nvm3_writeData(&nvm3_handle, NVM3_KEY_MQTT_CLIENT_ID, (void *)mqtt_client_id, sizeof(mqtt_client_id));
    nvm3_writeData(&nvm3_handle, NVM3_KEY_MQTT_PUBLISH_TOPIC, (void *)mqtt_publish_topic, sizeof(mqtt_publish_topic));
    nvm3_writeData(&nvm3_handle, NVM3_KEY_MQTT_SUBSCRIBE_TOPIC, (void *)mqtt_subscribe_topic, sizeof(mqtt_subscribe_topic));
    nvm3_writeData(&nvm3_handle, NVM3_KEY_MQTT_USERNAME, (void *)mqtt_username, sizeof(mqtt_username));
    nvm3_writeData(&nvm3_handle, NVM3_KEY_MQTT_PASSWORD, (void *)mqtt_password, sizeof(mqtt_password));
    nvm3_writeData(&nvm3_handle, NVM3_KEY_MQTT_CA_CERTIFICATE, (void *)ca_certificate, sizeof(ca_certificate));
    nvm3_writeData(&nvm3_handle, NVM3_KEY_MQTT_DEVICE_CERTIFICATE, (void *)device_certificate, sizeof(device_certificate));
    nvm3_writeData(&nvm3_handle, NVM3_KEY_MQTT_DEVICE_KEY, (void *)device_key, sizeof(device_key));
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
  // Use Google servers as primary and secondary DNS server
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
    // Start DHCP Client
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
