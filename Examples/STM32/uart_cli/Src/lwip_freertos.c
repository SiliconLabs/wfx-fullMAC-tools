/**************************************************************************//**
 * Copyright 2018, Silicon Laboratories Inc.
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

/* Includes */
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "string.h" 

#include "lwip_common.h"
/* LwIP includes. */
#include "ethernetif.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/netifapi.h"

#include "sl_wfx.h"
#include "sl_wfx_host.h"
#include "sl_wfx_host_pin.h"
#include "sl_wfx_cli_generic.h"
#include "sl_wfx_cli_common.h"
#include "dhcp_server.h"
#include "dhcp_client.h"

extern sl_wfx_rx_stats_t rx_stats;
extern sl_wfx_context_t wifi;
extern scan_result_list_t scan_list[];
extern uint8_t scan_count_web; 
char event_log[50];
/* station and softAP network interface structures */
struct netif sta_netif, ap_netif;
char string_list[4096];
uint8_t ap_channel;
struct eth_addr ap_mac;
  
int use_dhcp_client = USE_DHCP_CLIENT_DEFAULT;
int use_dhcp_server = USE_DHCP_SERVER_DEFAULT;

/* Station IP address */
uint8_t sta_ip_addr0 = STA_IP_ADDR0_DEFAULT;
uint8_t sta_ip_addr1 = STA_IP_ADDR1_DEFAULT;
uint8_t sta_ip_addr2 = STA_IP_ADDR2_DEFAULT;
uint8_t sta_ip_addr3 = STA_IP_ADDR3_DEFAULT;
uint8_t sta_netmask_addr0 = STA_NETMASK_ADDR0_DEFAULT;
uint8_t sta_netmask_addr1 = STA_NETMASK_ADDR1_DEFAULT;
uint8_t sta_netmask_addr2 = STA_NETMASK_ADDR2_DEFAULT;
uint8_t sta_netmask_addr3 = STA_NETMASK_ADDR3_DEFAULT;
uint8_t sta_gw_addr0 = STA_GW_ADDR0_DEFAULT;
uint8_t sta_gw_addr1 = STA_GW_ADDR1_DEFAULT;
uint8_t sta_gw_addr2 = STA_GW_ADDR2_DEFAULT;
uint8_t sta_gw_addr3 = STA_GW_ADDR3_DEFAULT;

/* SoftAP IP address */
uint8_t ap_ip_addr0 = AP_IP_ADDR0_DEFAULT;
uint8_t ap_ip_addr1 = AP_IP_ADDR1_DEFAULT;
uint8_t ap_ip_addr2 = AP_IP_ADDR2_DEFAULT;
uint8_t ap_ip_addr3 = AP_IP_ADDR3_DEFAULT;
uint8_t ap_netmask_addr0 = AP_NETMASK_ADDR0_DEFAULT;
uint8_t ap_netmask_addr1 = AP_NETMASK_ADDR1_DEFAULT;
uint8_t ap_netmask_addr2 = AP_NETMASK_ADDR2_DEFAULT;
uint8_t ap_netmask_addr3 = AP_NETMASK_ADDR3_DEFAULT;
uint8_t ap_gw_addr0 = AP_GW_ADDR0_DEFAULT;
uint8_t ap_gw_addr1 = AP_GW_ADDR1_DEFAULT;
uint8_t ap_gw_addr2 = AP_GW_ADDR2_DEFAULT;
uint8_t ap_gw_addr3 = AP_GW_ADDR3_DEFAULT;

static void StartThread(void const * argument);
static int Netif_Config(void);

/***************************************************************************//**
 * @brief
 *    Start LwIP task(s)
 *
 * @param[in] 
 *    not used
 *
 * @return
 *    none
 ******************************************************************************/
static void StartThread(void const * argument)
{
  int res;
  
  /* Create tcp_ip stack thread */
  tcpip_init(NULL, NULL);
  
  /* Start DHCP Client */
  dhcpclient_start();
  
  /* Initialize the LwIP stack */
  res = Netif_Config();
  if (res == 0) {  
    /* Register all application parameters to make them available in get/set commands */
    res = lwip_param_register();
    if (res != 0) {
      printf("Param register error (%d)\r\n", res);
    }
    
    /* Register the get/set commands in the shell */
    res = sl_wfx_cli_param_init();
    if (res != 0) {
      printf("Param CLI init error (%d)\r\n", res);
    }
    
    /* Register Wi-Fi commands in the shell */
    res = sl_wfx_cli_wifi_init(&wifi);
    if (res != 0) {
      printf("WiFi CLI init error (%d)\r\n", res);
    }
    
    /* Register IP commands in the shell */
    res = sl_wfx_cli_ip_init();
    if (res != 0) {
      printf("IP CLI init error (%d)\r\n", res);
    }
    
    /* Register the RF Test Agent commands in the shell */
    res = sl_wfx_cli_rf_test_agent_init(&wifi, &rx_stats);
    if (res != 0) {
      printf("RF Test Agent CLI init error (%d)\r\n", res);
    }
    
#ifdef SL_WFX_USE_SECURE_LINK
    /* Register the Secure Link commands in the shell */
    res = sl_wfx_cli_secure_link_init();
    if (res != 0) {
      printf("Secure Link CLI init error (%d)\r\n", res);
    }
#endif
  }
  
  for( ;; )
  {
    /* Delete the Init Thread */ 
    osThreadTerminate(NULL);
  }
}

/***************************************************************************//**
 * @brief
 *    Sets station link status to up in LwIP
 *
 * @param[in] 
 *    none
 *
 * @return
 *    none
 ******************************************************************************/
void lwip_set_sta_link_up (void)
{
  netifapi_netif_set_up(&sta_netif);
  netifapi_netif_set_link_up(&sta_netif);
  if (use_dhcp_client) {
    dhcpclient_set_link_state(1);
  }
}

/***************************************************************************//**
 * @brief
 *    Sets station link status to down in LwIP
 *
 * @param[in] 
 *    none
 *
 * @return
 *    none
******************************************************************************/
void lwip_set_sta_link_down (void)
{
  if (use_dhcp_client) {
    dhcpclient_set_link_state(0);
  }
  netifapi_netif_set_link_down(&sta_netif);
  netifapi_netif_set_down(&sta_netif);
}

/***************************************************************************//**
 * @brief
 *    Sets softAP link status to up in LwIP
 *
 * @param[in] 
 *    none
 *
 * @return
 *    none
 ******************************************************************************/
void lwip_set_ap_link_up (void)
{
  netifapi_netif_set_up(&ap_netif);
  netifapi_netif_set_link_up(&ap_netif);
  if (use_dhcp_server) {
    dhcpserver_start();
  }
}

/***************************************************************************//**
 * @brief
 *    Sets softAP link status to down in LwIP
 *
 * @param[in] 
 *    none
 *
 * @return
 *    none
 ******************************************************************************/
void lwip_set_ap_link_down (void)
{
  if (use_dhcp_server) {
    dhcpserver_stop();
  }
  netifapi_netif_set_link_down(&ap_netif);
  netifapi_netif_set_down(&ap_netif);
}

/***************************************************************************//**
 * @brief
 *    Initializes LwIP network interface
 *
 * @param[in] 
 *    none
 *
 * @return
*    0: initialization success, -1: an error occurred
 ******************************************************************************/
static int Netif_Config(void)
{
  sl_status_t status;
  ip_addr_t sta_ipaddr, ap_ipaddr;
  ip_addr_t sta_netmask, ap_netmask;
  ip_addr_t sta_gw, ap_gw;
  int res = -1;
  
  /* Initialize the Station information */
  if (use_dhcp_client)
  {
    ip_addr_set_zero_ip4(&sta_ipaddr);
    ip_addr_set_zero_ip4(&sta_netmask);
    ip_addr_set_zero_ip4(&sta_gw);
  }
  else
  {
    IP_ADDR4(&sta_ipaddr,sta_ip_addr0,sta_ip_addr1,sta_ip_addr2,sta_ip_addr3);
    IP_ADDR4(&sta_netmask,sta_netmask_addr0,sta_netmask_addr1,sta_netmask_addr2,sta_netmask_addr3);
    IP_ADDR4(&sta_gw,sta_gw_addr0,sta_gw_addr1,sta_gw_addr2,sta_gw_addr3);
  }
  
  /* Initialize the SoftAP information */
  IP_ADDR4(&ap_ipaddr,ap_ip_addr0,ap_ip_addr1,ap_ip_addr2,ap_ip_addr3);
  IP_ADDR4(&ap_netmask,ap_netmask_addr0,ap_netmask_addr1,ap_netmask_addr2,ap_netmask_addr3);
  IP_ADDR4(&ap_gw,ap_gw_addr0,ap_gw_addr1,ap_gw_addr2,ap_gw_addr3);
  
  /* Initialize the WF200 used by the two interfaces */
  status = sl_wfx_init(&wifi);
  switch(status) {
  case SL_STATUS_OK:
    wifi.state = SL_WFX_STARTED;
    res = 0;
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
  default :
    printf("Failed to init WF200: Unknown error\r\n");
  }
  
  if (res == 0) {
    /* Add station and softAP interfaces */
    netif_add(&sta_netif, &sta_ipaddr, &sta_netmask, &sta_gw, NULL, &sta_ethernetif_init, &tcpip_input);
    netif_add(&ap_netif, &ap_ipaddr, &ap_netmask, &ap_gw, NULL, &ap_ethernetif_init, &tcpip_input);
    
    /* Registers the default network interface */
    netif_set_default(&sta_netif);
  }

  return res;
}

/***************************************************************************//**
 * @brief
 *    Main function to call to start LwIP 
 *
 * @param[in] 
 *    none
 *
 * @return
 *    none
 ******************************************************************************/
void lwip_start (void)
{
  osThreadDef(Start, StartThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE * 5);
  osThreadCreate (osThread(Start), NULL);
}
