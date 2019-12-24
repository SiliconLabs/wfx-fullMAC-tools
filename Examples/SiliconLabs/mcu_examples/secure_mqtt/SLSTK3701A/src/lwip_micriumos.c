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


#include  <bsp_os.h>
#include  "bsp.h"

#include  <cpu/include/cpu.h>
#include  <kernel/include/os.h>
#include  <kernel/include/os_trace.h>
#include  <common/include/common.h>
#include  <common/include/lib_def.h>
#include  <common/include/rtos_utils.h>
#include  <common/include/toolchains.h>

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
#if LWIP_APP_TLS_ENABLED
#include "lwip/altcp_tls.h"
#endif
#ifdef LWIP_IPERF_SERVER
#include "lwip/ip_addr.h"
#include "lwip/apps/lwiperf.h"
#endif
#include "wifi_cli.h"
#include "dhcp_client.h"
#include "dhcp_server.h"
#include "ethernetif.h"
#include "wfx_host.h"
#include "wfx_task.h"
#include "dhcp_server.h"
#include "wfx_host.h"

static void netif_config(void);

#define LWIP_TASK_PRIO              23u
#define LWIP_TASK_STK_SIZE         800u

#if LWIP_APP_TLS_ENABLED
#define LWIP_APP_MQTT_PORT        8883u
#else
#define LWIP_APP_MQTT_PORT        1883u
#endif

#define LWIP_APP_TX_TICK_PERIOD   1000u
#define LWIP_APP_ECHO_ENABLED        1u

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
static ip_addr_t broker_ip;
static mqtt_client_t *mqtt_client = NULL;

#ifdef EFM32GG11B820F2048GM64
static const char device_name[] = "wgm160p";
static const char button_json_object[] = "{\"name\":\"PB%u\"}";
#else
static const char device_name[] = "efm32gg11";
static const char button_json_object[] = "{\"name\":\"BTN%u\"}";
#endif

static const char led_json_object[] = "{\"name\":\"LED%u\",\"state\":\"%s\"}";

static struct mqtt_connect_client_info_t mqtt_client_info = {
  device_name,
  NULL, NULL,
  10 /*Keep alive (seconds)*/,
  NULL, NULL, 0, 0,
#if LWIP_APP_TLS_ENABLED
  NULL
#endif
};

#if LWIP_APP_TLS_ENABLED
/// TLS
extern const char ca_certificate[];

#ifdef EFM32GG11B820F2048GM64
extern const char wgm160p_certificate[];
extern const char wgm160p_key[];

static const char* device_certificate = wgm160p_certificate;
static const char* device_key = wgm160p_key;
#else
extern const char efm32gg11_certificate[];
extern const char efm32gg11_key[];

static const char* device_certificate = efm32gg11_certificate;
static const char* device_key = efm32gg11_key;
#endif
#endif


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

static void mqtt_connection_cb (mqtt_client_t *client,
                                void *arg,
                                mqtt_connection_status_t status)
{
  (void)arg;

  if (status == MQTT_CONNECT_ACCEPTED) {
    printf("Connection success\r\n");
  } else {
    printf("Connection error: %d\r\n", status);
  }
}

static void mqtt_suscribe_cb (void *arg, err_t err)
{
  (void)arg;

  if (err == 0) {
    printf("Subscribe success\r\n");
  } else {
    printf("Subscribe error: %d\r\n", err);
  }
}

static void mqtt_publish_cb (void *arg, err_t err)
{
  (void)arg;

  if (err != 0) {
    printf("Publish error: %d\r\n", err);
  }
}

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
      // sscanf captures everything up to the null termination
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

static void lwip_app_wifi_config_prompt (void)
{
  char buf[4];
  bool error;

#ifdef SLEEP_ENABLED
  SLEEP_SleepBlockBegin(sleepEM2);
#endif

  printf("\nEnter the SSID of the AP you want to connect:\n");
  wifi_cli_get_input(wlan_ssid, sizeof(wlan_ssid), LWIP_APP_ECHO_ENABLED);

  do {
    error = false;
    printf("Enter the Passkey of the AP you want to connect (8-chars min):\n");
    wifi_cli_get_input(wlan_passkey, sizeof(wlan_passkey), LWIP_APP_ECHO_ENABLED);

    if (strlen(wlan_passkey) < 8) {
      printf("Size error\n");
      error = true;
    }
  } while (error);

  do {
    error = false;
    printf("Select a security mode:\n1. Open\n2. WEP\n3. WPA1 or WPA2\n4. WPA2\nEnter 1,2,3 or 4:\n");
    wifi_cli_get_input(buf, 2 + 1 /* let the user tap enter */, LWIP_APP_ECHO_ENABLED);

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

  printf("\n");

#ifdef SLEEP_ENABLED
  SLEEP_SleepBlockEnd(sleepEM2);
#endif
}

static void lwip_app_wifi_connection (void)
{
  RTOS_ERR err;

  printf("Waiting for the Wi-Fi connection...\r\n");
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

  OSFlagPend(&sl_wfx_event_group,
             SL_WFX_CONNECT,
             0,
             OS_OPT_PEND_FLAG_SET_ANY | OS_OPT_PEND_BLOCKING | OS_OPT_PEND_FLAG_CONSUME,
             0,
             &err);
  if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
    printf("Wait error %d\r\n", err.Code);
  }

  // Wait for the IP address binding if necessary
  while (use_dhcp_client && dhcp_supplied_address(&sta_netif) == 0) {
    OSTimeDly(500, OS_OPT_TIME_DLY, &err);
  }
}

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

static void lwip_app_mqtt_config_prompt (void)
{
  ip_addr_t *ipaddr;
  char buf[64];
  RTOS_ERR err;
  uint16_t len;
  err_t res;
  bool error;

#ifdef SLEEP_ENABLED
  SLEEP_SleepBlockBegin(sleepEM2);
#endif

  printf("\n");

  do {
    error = false;
    printf("Enter the MQTT broker address:\n");
    wifi_cli_get_input(buf, sizeof(buf), LWIP_APP_ECHO_ENABLED);

    res = dns_gethostbyname(buf, &broker_ip, dns_found_cb, NULL);
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
          printf("\nHostname %s resolved\n", buf);
          broker_ip = *ipaddr;
        } else {
          printf("\nHostname %s resolution error\n", buf);
          error = true;
        }
        break;

      case ERR_ARG:
        printf("Wrong hostname\n");
        error = true;
        break;
    }
  } while (error);

#ifdef SLEEP_ENABLED
  SLEEP_SleepBlockEnd(sleepEM2);
#endif
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

    OSTaskQPost(&lwip_task_tcb,
                (void *)button_id,
                sizeof(button_id),
                OS_OPT_POST_NONE,
                &err);
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
  uint8_t *ptr_ip = (uint8_t *)&broker_ip.addr;
  char pub_event_topic[32];
  char pub_data_topic[32];
  char sub_topic[32];
  char string[128];
  char tmp[32];
  int res, len;
  uint32_t evt_button_id;
  OS_TICK tick, timeout = LWIP_APP_TX_TICK_PERIOD;
  RTOS_ERR err;

  // Create tcp_ip stack thread
  tcpip_init(NULL, NULL);

  if (use_dhcp_client) {
    /* Start DHCP Client */
    dhcpclient_start();
  }

  // Initialize the LwIP stack
  netif_config();

#ifdef LWIP_IPERF_SERVER
  lwiperf_start_tcp_server_default(lwip_iperf_results, 0);
#endif

  // Initialize the DNS client (connecting to Google DNS servers)
  dns_setserver(0, &prim_dns_addr);
  dns_setserver(0, &sec_dns_addr);
  dns_init();

  // Prompt the user to configure the Wi-Fi connection
  lwip_app_wifi_config_prompt();

  // Realize the Wi-Fi connection
  lwip_app_wifi_connection();

  // Prompt the user to configure the MQTT connection
  lwip_app_mqtt_config_prompt();

  printf("Connecting to MQTT broker (%u.%u.%u.%u)...\r\n",
         ptr_ip[0], ptr_ip[1], ptr_ip[2], ptr_ip[3]);

  // Create a new MQTT client
  mqtt_client = mqtt_client_new();
  LWIP_ASSERT("MQTT: allocation error", mqtt_client);

#if LWIP_APP_TLS_ENABLED
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
#endif

  // Connect to the MQTT broker
  res = mqtt_client_connect(mqtt_client,
                            &broker_ip,
                            LWIP_APP_MQTT_PORT,
                            mqtt_connection_cb,
                            NULL,
                            &mqtt_client_info);
  LWIP_ASSERT("MQTT: connection error\r\n", res == 0);

  // Subscribe to a topic
  snprintf(sub_topic,
           sizeof(sub_topic),
           "%s/leds/set",
           mqtt_client_info.client_id);

  mqtt_set_inpub_callback(mqtt_client,
                          mqtt_incoming_publish_cb,
                          mqtt_incoming_data_cb,
                          NULL);

  mqtt_sub_unsub(mqtt_client, sub_topic, 0 /*qos*/, mqtt_suscribe_cb, NULL, 1);

  // Configure the publish topics
  snprintf(pub_data_topic,
           sizeof(pub_data_topic),
           "%s/leds/state",
           mqtt_client_info.client_id);

  snprintf(pub_event_topic,
           sizeof(pub_event_topic),
           "%s/button/event",
           mqtt_client_info.client_id);

  // Infinite loop, publishing data periodically and on event
  for (;; ) {
    evt_button_id = (uint32_t)OSTaskQPend(timeout,
                                          OS_OPT_PEND_BLOCKING,
                                          (uint16_t *)&len,
                                          NULL,
                                          &err);

    if (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) {
      // An event occurred, publish the source event

      //{"name":"BTN1"}
      len = snprintf(string,
                     sizeof(string),
                     button_json_object,
                     (uint16_t)evt_button_id);

      mqtt_publish(mqtt_client,
                   pub_event_topic,
                   (void *)string,
                   len,
                   1 /*qos*/,
                   0,
                   mqtt_publish_cb,
                   NULL);

    } else if (RTOS_ERR_CODE_GET(err) == RTOS_ERR_TIMEOUT) {
      // Periodical TX, publish the LED states

      //{"leds":[{"name":"LED0","state":"Off"},{"name":"LED1","state":"On"}]}
      len = snprintf(string, sizeof(string), "{\"leds\":[");
      for (int i=0; i<BSP_NO_OF_LEDS; i++) {
        len += snprintf(tmp,
                 sizeof(tmp),
                 led_json_object,
                 i,
                 BSP_LedGet(i) ? "On" : "Off");
        strcat(tmp, ","); len++;
        strcat(string, tmp);
      }
      strcpy(&string[len-1] /*Overwrite the last comma*/, "]}"); len++ /*-1 + 2*/;

      mqtt_publish(mqtt_client,
                   pub_data_topic,
                   (void *)string,
                   len,
                   1 /*qos*/,
                   0,
                   mqtt_publish_cb,
                   NULL);

      // Compute the next publishing period
      tick = OSTimeGet(&err);
      timeout = LWIP_APP_TX_TICK_PERIOD - (tick % LWIP_APP_TX_TICK_PERIOD);
    }
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
  dhcpserver_start();
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
  dhcpserver_stop();
  netifapi_netif_set_link_down(&ap_netif);
  netifapi_netif_set_down(&ap_netif);
  return SL_STATUS_OK;
}
/***************************************************************************//**
 * Initializes LwIP network interface.
 ******************************************************************************/
static void netif_config(void)
{
  sl_status_t status;
  ip_addr_t sta_ipaddr, ap_ipaddr;
  ip_addr_t sta_netmask, ap_netmask;
  ip_addr_t sta_gw, ap_gw;
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
