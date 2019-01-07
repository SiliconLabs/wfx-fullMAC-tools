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


//#define SOFT_AP_MODE   ///< If defined, default WiFi is soft AP mode (requires static IP)
#ifndef SOFT_AP_MODE
#define USE_DHCP       ///< If defined, DHCP is enabled, otherwise static address below is used
#endif
 
/*Static IP ADDRESS*/
#define IP_ADDR0   (uint8_t) 192 ///< Static IP: IP address value 0 
#define IP_ADDR1   (uint8_t) 168 ///< Static IP: IP address value 1
#define IP_ADDR2   (uint8_t) 0   ///< Static IP: IP address value 2
#define IP_ADDR3   (uint8_t) 1   ///< Static IP: IP address value 3
   
/*NETMASK*/
#define NETMASK_ADDR0   (uint8_t) 255 ///< Static IP: Netmask value 0 
#define NETMASK_ADDR1   (uint8_t) 255 ///< Static IP: Netmask value 1 
#define NETMASK_ADDR2   (uint8_t) 255 ///< Static IP: Netmask value 2 
#define NETMASK_ADDR3   (uint8_t) 0   ///< Static IP: Netmask value 3 

/*Gateway Address*/
#define GW_ADDR0   (uint8_t) 192      ///< Static IP: Gateway value 0 
#define GW_ADDR1   (uint8_t) 168      ///< Static IP: Gateway value 1 
#define GW_ADDR2   (uint8_t) 0        ///< Static IP: Gateway value 2 
#define GW_ADDR3   (uint8_t) 0        ///< Static IP: Gateway value 3 

/***************************************************************************//**
 * Functions to initialize LwIP
 ******************************************************************************/
void lwip_start (void);

#define LWIP_IPERF_SERVER ///< If defined, iperf server is enabled
#define LWIP_HTTP_SERVER  ///< If defined, http server is enabled

#define WLAN_SSID       "AP_name"                         ///< wifi ssid for client mode
#define WLAN_PASSKEY    "passkey"                         ///< wifi password for client mode
#define WLAN_SECURITY   WFM_SECURITY_MODE_WPA2_WPA1_PSK   ///< wifi security mode for client mode: WFM_SECURITY_MODE_OPEN/WFM_SECURITY_MODE_WEP/WFM_SECURITY_MODE_WPA2_WPA1_PSK
#define SOFTAP_SSID     "WF200_AP"                        ///< wifi ssid for soft ap mode
#define SOFTAP_PASSKEY  "12345678"                        ///< wifi password for soft ap mode
#define SOFTAP_SECURITY WFM_SECURITY_MODE_WPA2_WPA1_PSK   ///< wifi security for soft ap mode: WFM_SECURITY_MODE_OPEN/WFM_SECURITY_MODE_WEP/WFM_SECURITY_MODE_WPA2_WPA1_PSK
#define SOFTAP_CHANNEL  6                                 ///< wifi channel for soft ap

#endif