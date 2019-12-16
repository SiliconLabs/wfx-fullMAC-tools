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

// LwIP includes.
#include "ethernetif.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/ip_addr.h"
#include "lwip/apps/httpd.h"
#include "lwip/apps/mqtt.h"
#include "lwip/netifapi.h"
#include "demo_config.h"
#include "wifi_cli.h"
#include "dhcp_client.h"
#include "dhcp_server.h"
#include "ethernetif.h"
#include "wfx_host.h"
#ifdef LWIP_IPERF_SERVER
#include "lwip/ip_addr.h"
#include "lwip/apps/lwiperf.h"
#endif
#include "wfx_task.h"
#include "dhcp_server.h"
#include "wfx_host.h"

static void netif_config(void);

#define LWIP_APP_TX_TICK_PERIOD   1000u

#define LWIP_TASK_PRIO              23u
#define LWIP_TASK_STK_SIZE         800u

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
static struct mqtt_connect_client_info_t mqtt_client_info = {
  "efm32gg11",
  NULL, NULL,
  10 /*Keep alive (seconds)*/,
  NULL, NULL, 0, 0,
};


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
  (void)arg;

  for (int i=0; i<len; i++) {
    printf("%02X", data[i]);
  }

  if (flags & MQTT_DATA_FLAG_LAST) {
    printf("\r\n");
  }
}

static void lwip_example_config_prompt (void)
{
  char buf[16];
  uint8_t *ptr_ip = (uint8_t *)&broker_ip.addr;
  int res;
  bool error = false;

#ifdef SLEEP_ENABLED
  SLEEP_SleepBlockBegin(sleepEM2);
#endif

  printf("\nEnter the SSID of the AP you want to connect:\n");
  wifi_cli_get_input(wlan_ssid, sizeof(wlan_ssid), 1);

  do {
    error = false;
    printf("Enter the Passkey of the AP you want to connect (8-chars min):\n");
    wifi_cli_get_input(wlan_passkey, sizeof(wlan_passkey), 1);

    if (strlen(wlan_passkey) < 8) {
      printf("Size error\n");
      error = true;
    }
  } while (error);

  do {
    error = false;
    printf("Select a security mode:\n1. Open\n2. WEP\n3. WPA1 or WPA2\n4. WPA2\nEnter 1,2,3 or 4:\n");
    wifi_cli_get_input(buf, 2 + 1 /* let the user tap enter */, 1);

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

  do {
    error = false;
    printf("Enter the IP address of your MQTT broker:\n");
    wifi_cli_get_input(buf, sizeof(buf), 1);
    // TODO manage DNS resolution
    res = sscanf(buf,
                 "%hu.%hu.%hu.%hu",
                 (short unsigned int *)&ptr_ip[0],
                 (short unsigned int *)&ptr_ip[1],
                 (short unsigned int *)&ptr_ip[2],
                 (short unsigned int *)&ptr_ip[3]);
    if (res != 4) {
      printf("Wrong IP format\n");
      error = true;
    }
  } while (error);

  printf("\n");

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
  uint8_t *ptr_ip = (uint8_t *)&broker_ip.addr;
  char pub_event_topic[32];
  char pub_data_topic[32];
  char sub_topic[32];
  char string[32];
  char event[8];
  int res, len;
  uint32_t cnt = 1;
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

  // Prompt the user to configure the example
  lwip_example_config_prompt();

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

  printf("Connecting to MQTT broker (%u.%u.%u.%u)...\r\n",
         ptr_ip[0], ptr_ip[1], ptr_ip[2], ptr_ip[3]);

  // Create a new MQTT client
  mqtt_client = mqtt_client_new();
  LWIP_ASSERT("MQTT: allocation error", mqtt_client);

  // Connect to the MQTT broker
  res = mqtt_client_connect(mqtt_client,
                            &broker_ip,
                            1883,
                            mqtt_connection_cb,
                            NULL,
                            &mqtt_client_info);
  LWIP_ASSERT("MQTT: connection error\r\n", res == 0);

  // Subscribe to a topic
  snprintf(sub_topic,
           sizeof(sub_topic),
           "/downstream/%s/#",
           mqtt_client_info.client_id);
  sub_topic[sizeof(sub_topic)-1] = '\0';

  mqtt_set_inpub_callback(mqtt_client,
                          mqtt_incoming_publish_cb,
                          mqtt_incoming_data_cb,
                          NULL);

  mqtt_sub_unsub(mqtt_client, sub_topic, 0, mqtt_suscribe_cb, NULL, 1);

  snprintf(pub_data_topic,
           sizeof(pub_data_topic),
           "/upstream/%s/data",
           mqtt_client_info.client_id);
  pub_data_topic[sizeof(pub_data_topic)-1] = '\0';

  snprintf(pub_event_topic,
           sizeof(pub_event_topic),
           "/upstream/%s/event",
           mqtt_client_info.client_id);
  pub_event_topic[sizeof(pub_event_topic)-1] = '\0';

  // Infinite loop, publishing data periodically and on event
  for (;; ) {
    evt_button_id = (uint32_t)OSTaskQPend(timeout,
                                          OS_OPT_PEND_BLOCKING,
                                          (uint16_t *)&len,
                                          NULL,
                                          &err);

    if (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) {
      len = snprintf(event, sizeof(event), "button%lu", evt_button_id);

      mqtt_publish(mqtt_client,
                   pub_event_topic,
                   (void *)event,
                   len,
                   0 /*qos*/,
                   0,
                   mqtt_publish_cb,
                   NULL);

    } else if (RTOS_ERR_CODE_GET(err) == RTOS_ERR_TIMEOUT) {
      len = snprintf(string, sizeof(string), "Hello %lu!", cnt);

      mqtt_publish(mqtt_client,
                   pub_data_topic,
                   (void *)string,
                   len,
                   0 /*qos*/,
                   0,
                   mqtt_publish_cb,
                   NULL);
      cnt++;

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
