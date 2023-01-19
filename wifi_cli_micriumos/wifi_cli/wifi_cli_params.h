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
#ifndef WIFI_CLI_PARAMS_H
#define WIFI_CLI_PARAMS_H

#include <stdint.h>
#include "os.h"
#include "sl_wfx_cmd_api.h"
#include "lwip/ip_addr.h"
#include "nvm3_default.h"
#include "sl_wfx_constants.h"

#if defined(SL_CATALOG_POWER_MANAGER_PRESENT)
#error "Power Save must be enabled through the CLI not the Power manager component"
#endif

/*******************************************************************************
 *****************************   LOG DEBUG   ***********************************
 ******************************************************************************/
#define CR_LF "\r\n"
#define INT_ERR  "Internal Error: "
#define FILE_LINE "File: %s - line %d: "

#define PRINTF_LOG(format, args...) do { \
                                        printf(FILE_LINE INT_ERR format CR_LF, \
                                               __FILE__, __LINE__, ##args);    \
                                    } while (0)
#if DEBUG_EFM
#define LOG_DEBUG(format, args...) PRINTF_LOG(format, ##args)
#else
#define LOG_DEBUG(format, args...) (void)0
#endif

/*******************************************************************************
 ***********************   DEFAULT GLOBAL WIFI PARAMETERS   ********************
 ******************************************************************************/
#define SDIO_BUS  "sdio"
#define SPI_BUS   "spi"

#define BUF_LEN   128
#define SL_WFX_CLI_MAX_PARAMS   30
#define SL_WFX_CLI_MAX_CLIENTS  10

/* Parameter edit rights mask */
#define SL_WFX_CLI_PARAM_SET_RIGHT       (1<<0)
#define SL_WFX_CLI_PARAM_GET_RIGHT       (1<<1)

#define USE_DHCP_CLIENT_DEFAULT    1   ///< If defined, DHCP request is enabled,
                                       /// otherwise static address below is used

#define USE_DHCP_SERVER_DEFAULT    1   ///< If defined, DHCP server is enabled,
                                       ///otherwise static address below is used

/************************** Station Static Default ****************************/
#define STA_IP_ADDR0_DEFAULT   (uint8_t) 192 ///< Static IP: IP address value 0
#define STA_IP_ADDR1_DEFAULT   (uint8_t) 168 ///< Static IP: IP address value 1
#define STA_IP_ADDR2_DEFAULT   (uint8_t) 0   ///< Static IP: IP address value 2
#define STA_IP_ADDR3_DEFAULT   (uint8_t) 1   ///< Static IP: IP address value 3

/* NETMASK */
#define STA_NETMASK_ADDR0_DEFAULT   (uint8_t) 255 ///< Static IP: Netmask value 0
#define STA_NETMASK_ADDR1_DEFAULT   (uint8_t) 255 ///< Static IP: Netmask value 1
#define STA_NETMASK_ADDR2_DEFAULT   (uint8_t) 255 ///< Static IP: Netmask value 2
#define STA_NETMASK_ADDR3_DEFAULT   (uint8_t) 0   ///< Static IP: Netmask value 3

/* Gateway Address */
#define STA_GW_ADDR0_DEFAULT   (uint8_t) 0       ///< Static IP: Gateway value 0
#define STA_GW_ADDR1_DEFAULT   (uint8_t) 0       ///< Static IP: Gateway value 1
#define STA_GW_ADDR2_DEFAULT   (uint8_t) 0       ///< Static IP: Gateway value 2
#define STA_GW_ADDR3_DEFAULT   (uint8_t) 0       ///< Static IP: Gateway value 3

/*********************** Access Point Static Default **************************/
#define AP_IP_ADDR0_DEFAULT   (uint8_t) 10 ///< Static IP: IP address value 0
#define AP_IP_ADDR1_DEFAULT   (uint8_t) 10 ///< Static IP: IP address value 1
#define AP_IP_ADDR2_DEFAULT   (uint8_t) 0   ///< Static IP: IP address value 2
#define AP_IP_ADDR3_DEFAULT   (uint8_t) 1   ///< Static IP: IP address value 3

/* NETMASK */
#define AP_NETMASK_ADDR0_DEFAULT   (uint8_t) 255 ///< Static IP: Netmask value 0
#define AP_NETMASK_ADDR1_DEFAULT   (uint8_t) 255 ///< Static IP: Netmask value 1
#define AP_NETMASK_ADDR2_DEFAULT   (uint8_t) 255 ///< Static IP: Netmask value 2
#define AP_NETMASK_ADDR3_DEFAULT   (uint8_t) 0   ///< Static IP: Netmask value 3

/* Gateway Address */
#define AP_GW_ADDR0_DEFAULT   (uint8_t) 0        ///< Static IP: Gateway value 0
#define AP_GW_ADDR1_DEFAULT   (uint8_t) 0        ///< Static IP: Gateway value 1
#define AP_GW_ADDR2_DEFAULT   (uint8_t) 0        ///< Static IP: Gateway value 2
#define AP_GW_ADDR3_DEFAULT   (uint8_t) 0        ///< Static IP: Gateway value 3

#ifndef NVM3_KEY_AP_SSID
#define NVM3_KEY_AP_SSID 1
#endif
#ifndef NVM3_KEY_AP_SECURITY_MODE
#define NVM3_KEY_AP_SECURITY_MODE 2
#endif
#ifndef NVM3_KEY_AP_PASSKEY
#define NVM3_KEY_AP_PASSKEY 3
#endif
#define IPERF_SERVER                    ///< If defined, iperf server is enabled
#define HTTP_SERVER                     ///< If defined, http server is enabled

#define WLAN_SSID_DEFAULT       "AP_name"         ///< wifi ssid for client mode
#define WLAN_PASSKEY_DEFAULT    "passkey"     ///< wifi password for client mode

/** wifi security mode for client mode:
 *  WFM_SECURITY_MODE_OPEN/WFM_SECURITY_MODE_WEP/WFM_SECURITY_MODE_WPA2_WPA1_PSK
 * */
#define WLAN_SECURITY_DEFAULT   WFM_SECURITY_MODE_WPA2_PSK
#define SOFTAP_SSID_DEFAULT     "silabs_softap"  ///< wifi ssid for soft ap mode
#define SOFTAP_PASSKEY_DEFAULT  "changeme"   ///< wifi password for soft ap mode

/** wifi security for soft ap mode:
 *  WFM_SECURITY_MODE_OPEN/WFM_SECURITY_MODE_WEP/WFM_SECURITY_MODE_WPA2_WPA1_PSK
 * */
#define SOFTAP_SECURITY_DEFAULT WFM_SECURITY_MODE_WPA2_PSK
#define SOFTAP_CHANNEL_DEFAULT  6                  ///< wifi channel for soft ap

extern char wlan_ssid[32 + 1];
extern char wlan_passkey[64 + 1];
extern char wlan_pmk[64 + 3];
extern sl_wfx_security_mode_t wlan_security;

extern char softap_ssid[32 + 1];
extern char softap_passkey[64 + 1];
extern char softap_pmk[64 + 3];
extern uint8_t softap_channel;
extern sl_wfx_security_mode_t softap_security;

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

extern uint8_t use_dhcp_client;
extern uint8_t use_dhcp_server;
extern uint8_t wifi_clients[SL_WFX_CLI_MAX_CLIENTS][6];
extern sl_wfx_rx_stats_t rx_stats;

extern int number_connected_clients;
extern char string_list[4096];

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************//**
 * @brief: secure link bitmap change function pointers.
 *****************************************************************************/
typedef void (*secure_link_bitmap_change_fn_t)(uint8_t *bitmap, uint8_t request_id);

/**************************************************************************//**
 * @brief: DHCP types.
 *****************************************************************************/
typedef enum {
  DHCP_SERVER,
  DHCP_CLIENT
} dhcp_type_et;

/**************************************************************************//**
 * @brief: tx rates type is used for converting a uint32_t number to a bitmask
 *****************************************************************************/
typedef union {
    sl_wfx_rate_set_bitmask_t bit_mask;
    uint32_t rate;
} tx_rates_u;

/**************************************************************************//**
 * @brief: get/set function pointers.
 *****************************************************************************/
typedef int (*sl_wfx_cli_param_custom_get_func_t)(char *param_name,
                                                  void *param_addr,
                                                  uint32_t param_size,
                                                  char *output_buf,
                                                  uint32_t output_buf_len);

typedef int (*sl_wfx_cli_param_custom_set_func_t)(char *param_name,
                                                  void *param_addr,
                                                  uint32_t param_size,
                                                  char *new_value);

/**************************************************************************//**
 * @brief: wifi get/set parameters types
 *****************************************************************************/
typedef enum sl_wfx_cli_param_type_s {
  SL_WFX_CLI_PARAM_TYPE_INTEGER = 0,
  SL_WFX_CLI_PARAM_TYPE_UNSIGNED_INTEGER,
  SL_WFX_CLI_PARAM_TYPE_HEXADECIMAL,
  SL_WFX_CLI_PARAM_TYPE_ARRAY_INTEGER,
  SL_WFX_CLI_PARAM_TYPE_ARRAY_UNSIGNED_INTEGER,
  SL_WFX_CLI_PARAM_TYPE_ARRAY_HEXADECIMAL,
  SL_WFX_CLI_PARAM_TYPE_ARRAY_STRING,
  SL_WFX_CLI_PARAM_TYPE_STRING,
  SL_WFX_CLI_PARAM_TYPE_CUSTOM
} sl_wfx_cli_param_type_t;

/**************************************************************************//**
 * @brief: Main struct type is used for managing all get/set
 *         wifi parameters.
 *****************************************************************************/
typedef struct param_s {
  char *name;
  void *address;
  char *description;
  sl_wfx_cli_param_custom_get_func_t get_func;
  sl_wfx_cli_param_custom_set_func_t set_func;
  sl_wfx_cli_param_type_t type;
  uint8_t size;
  uint8_t rights;
} param_t;

/**************************************************************************//**
 * @brief: Wi-Fi CLI's states type
 *****************************************************************************/
typedef enum cli_state_s {
  POSTED_CLI, /*!< Another task has done the job, CLI doesn't need to wait */
  FREE_CLI,   /*!< CLI is free & ready for wait */
  PENDING_CLI /*!< CLI is being blocked to wait */
} cli_state_e;

/**************************************************************************//**
 * @brief: Wi-Fi CLI's event-based semaphore type
 *****************************************************************************/
typedef struct sem_type_s {
  sl_wfx_indications_ids_t event_type;
  OS_SEM cli_sem;           /*!< Binary semaphore */
  cli_state_e cli_state;    /*!< Wi-Fi CLI's current states */
} sem_type_t;

/**************************************************************************//**
 * @brief: Wi-Fi CLI's semaphore instance used for blocking to wait for events
 *****************************************************************************/
extern sem_type_t g_cli_sem;

/**************************************************************************//**
 * @brief: Wi-Fi CLI's the global wifi param instance used for managing all
 *         get/set parameters.
 * @note:  Maximum number of elements is "SL_WFX_CLI_MAX_PARAMS"
 *****************************************************************************/
extern param_t wifi_params[SL_WFX_CLI_MAX_PARAMS];

/***************************************************************************//**
 * @brief convert a given hex string to uint32_t.
 ******************************************************************************/
uint32_t convert_rate_string_to_uint32_t(char *rates_str);

/***************************************************************************//**
 * @brief
 *    This function converts a given string to lower-case string.
 *
 * @param[in]
 *        + str: The input is a NULL-terminated string.
 *
 * @param[out]
 *        + out_buf:  The output buffer contains separated word
 *
 * @return
 *        None
 * @note: The input string will be changed
 ******************************************************************************/
void convert_to_lower_case_string(char *str);

/***************************************************************************//**
 * @brief Search get/set parameters by its name in the wifi_params struct array
 *        and return the parameter's index
 * @param[in]
 *        + name: The parameter name needs to be searched
 * @param[out]
 *        None
 * @return
 *        + The index of parameter in the wifi_params arrays, if success
 *        + -1 if failed
 ******************************************************************************/
int param_search (char *name);

/**************************************************************************//**
 * @brief: Convert MAC string to "sl_wfx_mac_address_t" format
 *****************************************************************************/
int convert_str_to_mac_addr(char *mac_str, sl_wfx_mac_address_t *mac_addr);

/**************************************************************************//**
 * @brief: Get the wifi parameter's address by its name
 *****************************************************************************/
void *wifi_cli_get_param_addr(char *param_name);

/**************************************************************************//**
 * @brief: Convert Message Authentication Code (MAC) key string
 *         to an MAC key array.
 *****************************************************************************/
int convert_mac_key_string_to_array(char *mackey_str,
                                    uint8_t *mackey_arr,
                                    uint8_t arrLen);

/**************************************************************************//**
 * @brief: Wi-Fi CLI's event-based semaphore initialization
 *****************************************************************************/
void wifi_cli_sem_init(sem_type_t *p_cli_sem, RTOS_ERR *err);

/**************************************************************************//**
 * @brief: Block Wi-Fi CLI wait for desired events with timeout in ms
 *****************************************************************************/
RTOS_ERR_CODE wifi_cli_wait(sem_type_t *p_cli_sem,
                            sl_wfx_indications_ids_t sem_event_type,
                            uint32_t timeoutms);

/**************************************************************************//**
 * @brief: Release (post) Wi-Fi CLI's semaphore
 *****************************************************************************/
int wifi_cli_resume(sem_type_t *p_cli_sem,
                    sl_wfx_indications_ids_t sem_event_type);

/**************************************************************************//**
 * @brief: Registering Wi-Fi's get/set parameters to the wifi_param array
 *****************************************************************************/
int register_wifi_params(void);

/**************************************************************************//**
 * @brief: Create Wi-Fi CLI's get/set parameter initialization task
 *****************************************************************************/
void wifi_cli_params_init(void);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_CLI_PARAMS_H_ */
