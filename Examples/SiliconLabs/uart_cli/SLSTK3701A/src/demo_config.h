/***************************************************************************//**
 * @file
 * @brief LwIP task and related functions header file
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
#ifndef DEMO_CONFIG_H
#define DEMO_CONFIG_H

#include "sl_wfx_cmd_api.h"
#include "sl_status.h"

#define USE_DHCP_CLIENT_DEFAULT    1 ///< If defined, DHCP requests are enabled, otherwise static address below is used
#define USE_DHCP_SERVER_DEFAULT    1 ///< If defined, DHCP server is enabled, otherwise client must have a static address

/************************** Station Static Default ****************************/
#define STA_IP_ADDR0_DEFAULT   (uint8_t) 192 ///< Static IP: IP address value 0
#define STA_IP_ADDR1_DEFAULT   (uint8_t) 168 ///< Static IP: IP address value 1
#define STA_IP_ADDR2_DEFAULT   (uint8_t) 0   ///< Static IP: IP address value 2
#define STA_IP_ADDR3_DEFAULT   (uint8_t) 1   ///< Static IP: IP address value 3

/*NETMASK*/
#define STA_NETMASK_ADDR0_DEFAULT   (uint8_t) 255 ///< Static IP: Netmask value 0
#define STA_NETMASK_ADDR1_DEFAULT   (uint8_t) 255 ///< Static IP: Netmask value 1
#define STA_NETMASK_ADDR2_DEFAULT   (uint8_t) 255 ///< Static IP: Netmask value 2
#define STA_NETMASK_ADDR3_DEFAULT   (uint8_t) 0   ///< Static IP: Netmask value 3

/*Gateway Address*/
#define STA_GW_ADDR0_DEFAULT   (uint8_t) 0        ///< Static IP: Gateway value 0
#define STA_GW_ADDR1_DEFAULT   (uint8_t) 0        ///< Static IP: Gateway value 1
#define STA_GW_ADDR2_DEFAULT   (uint8_t) 0        ///< Static IP: Gateway value 2
#define STA_GW_ADDR3_DEFAULT   (uint8_t) 0        ///< Static IP: Gateway value 3

/************************** Access Point Static Default ****************************/
#define AP_IP_ADDR0_DEFAULT   (uint8_t) 10 ///< Static IP: IP address value 0
#define AP_IP_ADDR1_DEFAULT   (uint8_t) 10 ///< Static IP: IP address value 1
#define AP_IP_ADDR2_DEFAULT   (uint8_t) 0   ///< Static IP: IP address value 2
#define AP_IP_ADDR3_DEFAULT   (uint8_t) 1   ///< Static IP: IP address value 3

/*NETMASK*/
#define AP_NETMASK_ADDR0_DEFAULT   (uint8_t) 255 ///< Static IP: Netmask value 0
#define AP_NETMASK_ADDR1_DEFAULT   (uint8_t) 255 ///< Static IP: Netmask value 1
#define AP_NETMASK_ADDR2_DEFAULT   (uint8_t) 255 ///< Static IP: Netmask value 2
#define AP_NETMASK_ADDR3_DEFAULT   (uint8_t) 0   ///< Static IP: Netmask value 3

/*Gateway Address*/
#define AP_GW_ADDR0_DEFAULT   (uint8_t) 0        ///< Static IP: Gateway value 0
#define AP_GW_ADDR1_DEFAULT   (uint8_t) 0        ///< Static IP: Gateway value 1
#define AP_GW_ADDR2_DEFAULT   (uint8_t) 0        ///< Static IP: Gateway value 2
#define AP_GW_ADDR3_DEFAULT   (uint8_t) 0        ///< Static IP: Gateway value 3
/***************************************************************************//**
 * Functions to initialize LwIP
 ******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************//**
 * Start LwIP task.
 *****************************************************************************/
sl_status_t lwip_start (void);

/**************************************************************************//**
 * Set station link status to up.
 *****************************************************************************/
sl_status_t lwip_set_sta_link_up(void);

/**************************************************************************//**
 * Set station link status to down.
 *****************************************************************************/
sl_status_t lwip_set_sta_link_down(void);

/**************************************************************************//**
 * Set AP link status to up.
 *****************************************************************************/
sl_status_t lwip_set_ap_link_up(void);

/**************************************************************************//**
 * Set AP link status to down.
 *****************************************************************************/
sl_status_t lwip_set_ap_link_down(void);

/**************************************************************************//**
 * Button handler
 *****************************************************************************/
void lwip_button_handler(uint32_t button_id);

/**************************************************************************//**
 * Register the parameters specific to the application into the CLI
 *****************************************************************************/
int lwip_param_register (void);
#ifdef __cplusplus
}
#endif

#define WLAN_SSID_DEFAULT       "AP_name"                         ///< wifi ssid for client mode
#define WLAN_PASSKEY_DEFAULT    "passkey"                         ///< wifi password for client mode
#define WLAN_SECURITY_DEFAULT   WFM_SECURITY_MODE_WPA2_PSK        ///< wifi security mode for client mode: WFM_SECURITY_MODE_OPEN/WFM_SECURITY_MODE_WEP/WFM_SECURITY_MODE_WPA2_WPA1_PSK
#define SOFTAP_SSID_DEFAULT     "silabs_softap"                   ///< wifi ssid for soft ap mode
#define SOFTAP_PASSKEY_DEFAULT  "changeme"                        ///< wifi password for soft ap mode
#define SOFTAP_SECURITY_DEFAULT WFM_SECURITY_MODE_WPA2_PSK        ///< wifi security for soft ap mode: WFM_SECURITY_MODE_OPEN/WFM_SECURITY_MODE_WEP/WFM_SECURITY_MODE_WPA2_WPA1_PSK
#define SOFTAP_CHANNEL_DEFAULT  6                                 ///< wifi channel for soft ap

extern char wlan_ssid[32+1];
extern char wlan_passkey[64+1];
extern char wlan_pmk[64+1];
extern sl_wfx_security_mode_t wlan_security;
extern char softap_ssid[32+1];
extern char softap_passkey[64+1];
extern char softap_pmk[64+1];
extern sl_wfx_security_mode_t softap_security;
extern uint8_t softap_channel;

extern uint8_t sta_ip_addr0;
extern uint8_t sta_ip_addr1;
extern uint8_t sta_ip_addr2;
extern uint8_t sta_ip_addr3;

extern uint8_t sta_netmask_addr0;
extern uint8_t sta_netmask_addr1;
extern uint8_t sta_netmask_addr2;
extern uint8_t sta_netmask_addr3;

extern uint8_t sta_gw_addr0;
extern uint8_t sta_gw_addr1;
extern uint8_t sta_gw_addr2;
extern uint8_t sta_gw_addr3;

extern uint8_t ap_ip_addr0;
extern uint8_t ap_ip_addr1;
extern uint8_t ap_ip_addr2;
extern uint8_t ap_ip_addr3;

extern uint8_t ap_netmask_addr0;
extern uint8_t ap_netmask_addr1;
extern uint8_t ap_netmask_addr2;
extern uint8_t ap_netmask_addr3;

extern uint8_t ap_gw_addr0;
extern uint8_t ap_gw_addr1;
extern uint8_t ap_gw_addr2;
extern uint8_t ap_gw_addr3;

extern struct netif ap_netif;
extern struct netif sta_netif;

extern int use_dhcp_client;
extern int use_dhcp_server;

extern sl_wfx_rx_stats_t rx_stats;

#endif
