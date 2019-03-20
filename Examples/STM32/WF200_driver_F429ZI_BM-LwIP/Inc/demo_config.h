/***************************************************************************//**
 * @file lwip_freertos.h
 * @brief FreeRTOS LwIP implementation and tasks.
 * @version 1.0.0
 *******************************************************************************
 * @section License
 * <b>Copyright 2017 Silicon Labs, Inc. http://www.silabs.com</b>
 *******************************************************************************
 *
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 *
 ******************************************************************************/
#ifndef LWIP_FREERTOS_H
#define LWIP_FREERTOS_H

#include "wfm_cmd_api.h"

#define SOFT_AP_MODE_DEFAULT 0   ///< If set to 1, default WiFi is soft AP mode (requires static IP)
#define USE_DHCP_CLIENT_DEFAULT     1   ///< If defined, DHCP is enabled, otherwise static address below is used
 
/*Static IP ADDRESS*/
#define IP_ADDR0_DEFAULT   (uint8_t) 192 ///< Static IP: IP address value 0 
#define IP_ADDR1_DEFAULT   (uint8_t) 168 ///< Static IP: IP address value 1
#define IP_ADDR2_DEFAULT   (uint8_t) 0   ///< Static IP: IP address value 2
#define IP_ADDR3_DEFAULT   (uint8_t) 1   ///< Static IP: IP address value 3
   
/*NETMASK*/
#define NETMASK_ADDR0_DEFAULT   (uint8_t) 255 ///< Static IP: Netmask value 0 
#define NETMASK_ADDR1_DEFAULT   (uint8_t) 255 ///< Static IP: Netmask value 1 
#define NETMASK_ADDR2_DEFAULT   (uint8_t) 255 ///< Static IP: Netmask value 2 
#define NETMASK_ADDR3_DEFAULT   (uint8_t) 0   ///< Static IP: Netmask value 3 

/*Gateway Address*/
#define GW_ADDR0_DEFAULT   (uint8_t) 0        ///< Static IP: Gateway value 0 
#define GW_ADDR1_DEFAULT   (uint8_t) 0        ///< Static IP: Gateway value 1 
#define GW_ADDR2_DEFAULT   (uint8_t) 0        ///< Static IP: Gateway value 2 
#define GW_ADDR3_DEFAULT   (uint8_t) 0        ///< Static IP: Gateway value 3 

/***************************************************************************//**
 * Functions to initialize LwIP
 ******************************************************************************/
void lwip_start (void);

#define LWIP_IPERF_SERVER ///< If defined, iperf server is enabled
#define LWIP_HTTP_SERVER  ///< If defined, http server is enabled

#define WLAN_SSID_DEFAULT       "AP_name"                         ///< wifi ssid for client mode
#define WLAN_PASSKEY_DEFAULT    "passkey"                         ///< wifi password for client mode
#define WLAN_SECURITY_DEFAULT   WFM_SECURITY_MODE_WPA2_WPA1_PSK   ///< wifi security mode for client mode: WFM_SECURITY_MODE_OPEN/WFM_SECURITY_MODE_WEP/WFM_SECURITY_MODE_WPA2_WPA1_PSK
#define SOFTAP_SSID_DEFAULT     "WF200_AP"                        ///< wifi ssid for soft ap mode
#define SOFTAP_PASSKEY_DEFAULT  "12345678"                        ///< wifi password for soft ap mode
#define SOFTAP_SECURITY_DEFAULT WFM_SECURITY_MODE_WPA2_PSK        ///< wifi security for soft ap mode: WFM_SECURITY_MODE_OPEN/WFM_SECURITY_MODE_WEP/WFM_SECURITY_MODE_WPA2_WPA1_PSK
#define SOFTAP_CHANNEL_DEFAULT  6                                 ///< wifi channel for soft ap

extern char wlan_ssid[32];
extern char wlan_passkey[64];
extern WfmSecurityMode wlan_security;
extern char softap_ssid[32];
extern char softap_passkey[64];
extern WfmSecurityMode softap_security;
extern uint8_t softap_channel;

extern int use_dhcp_client;
extern int soft_ap_mode;
extern uint8_t ip_addr0;
extern uint8_t ip_addr1;
extern uint8_t ip_addr2;
extern uint8_t ip_addr3;

extern uint8_t netmask_addr0;
extern uint8_t netmask_addr1;
extern uint8_t netmask_addr2;
extern uint8_t netmask_addr3;

extern uint8_t gw_addr0;
extern uint8_t gw_addr1;
extern uint8_t gw_addr2;
extern uint8_t gw_addr3;

#endif