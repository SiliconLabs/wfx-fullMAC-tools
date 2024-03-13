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
#include <string.h>
#include <stdint.h>
#include "wifi_cli_params.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/apps/httpd.h"
#include "lwip/netifapi.h"
#include "lwip/ip_addr.h"
#include "dhcp_client.h"
#include "dhcp_server.h"
#include "ethernetif.h"
#include "sl_wfx_task.h"
#include "sl_wfx_host.h"

/* CLI's parameters registration configuration */
#define WFX_CLI_PARAMS_TASK_PRIO        32u
#define WFX_CLI_PARAMS_TASK_STK_SIZE    800u

/* CLI's parameters registration task stack */
static CPU_STK wfx_cli_params_task_stk[WFX_CLI_PARAMS_TASK_STK_SIZE];
/* CLI's parameters registration task TCB */
static OS_TCB wfx_cli_params_task_tcb;
/* CLI's parameters registration task */
static void wfx_cli_params_task(void *p_arg);

/* global wifi context */
extern sl_wfx_context_t   wifi;

/* User-defined sem type is used to force CLI to wait */
sem_type_t g_cli_sem;
/* rx_stats */
sl_wfx_rx_stats_t rx_stats;

/* LwIP station network interface structure. */
struct netif sta_netif;
/* LwIP AP network interface structure. */
struct netif ap_netif;

/* Enable or disable DHCP client for station. */
uint8_t use_dhcp_client = USE_DHCP_CLIENT_DEFAULT;
/* Enable or disable DHCP server for SoftAP */
uint8_t use_dhcp_server = USE_DHCP_SERVER_DEFAULT;

/* Station IP address octet 0. */
uint8_t sta_ip_addr0 = STA_IP_ADDR0_DEFAULT;
/* Station IP address octet 1. */
uint8_t sta_ip_addr1 = STA_IP_ADDR1_DEFAULT;
/* Station IP address octet 2. */
uint8_t sta_ip_addr2 = STA_IP_ADDR2_DEFAULT;
/* Station IP address octet 3. */
uint8_t sta_ip_addr3 = STA_IP_ADDR3_DEFAULT;

/* Station net mask octet 0. */
uint8_t sta_netmask_addr0 = STA_NETMASK_ADDR0_DEFAULT;
/* Station net mask octet 1. */
uint8_t sta_netmask_addr1 = STA_NETMASK_ADDR1_DEFAULT;
/* Station net mask octet 2. */
uint8_t sta_netmask_addr2 = STA_NETMASK_ADDR2_DEFAULT;
/* Station net mask octet 3. */
uint8_t sta_netmask_addr3 = STA_NETMASK_ADDR3_DEFAULT;

/* Station gateway IP octet 0. */
uint8_t sta_gw_addr0 = STA_GW_ADDR0_DEFAULT;
/* Station gateway IP octet 1. */
uint8_t sta_gw_addr1 = STA_GW_ADDR1_DEFAULT;
/* Station gateway IP octet 2. */
uint8_t sta_gw_addr2 = STA_GW_ADDR2_DEFAULT;
/* Station gateway IP octet 3. */
uint8_t sta_gw_addr3 = STA_GW_ADDR3_DEFAULT;

/* AP IP address octet 0. */
uint8_t ap_ip_addr0 = AP_IP_ADDR0_DEFAULT;
/* AP IP address octet 1. */
uint8_t ap_ip_addr1 = AP_IP_ADDR1_DEFAULT;
/* AP IP address octet 2. */
uint8_t ap_ip_addr2 = AP_IP_ADDR2_DEFAULT;
/* AP IP address octet 3. */
uint8_t ap_ip_addr3 = AP_IP_ADDR3_DEFAULT;

/* AP net mask octet 0. */
uint8_t ap_netmask_addr0 = AP_NETMASK_ADDR0_DEFAULT;
/* AP net mask octet 1. */
uint8_t ap_netmask_addr1 = AP_NETMASK_ADDR1_DEFAULT;
/* AP net mask octet 2. */
uint8_t ap_netmask_addr2 = AP_NETMASK_ADDR2_DEFAULT;
/* AP net mask octet 3. */
uint8_t ap_netmask_addr3 = AP_NETMASK_ADDR3_DEFAULT;

/* AP gateway IP octet 0. */
uint8_t ap_gw_addr0 = AP_GW_ADDR0_DEFAULT;
/* AP gateway IP octet 1. */
uint8_t ap_gw_addr1 = AP_GW_ADDR1_DEFAULT;
/* AP gateway IP octet 2. */
uint8_t ap_gw_addr2 = AP_GW_ADDR2_DEFAULT;
/* AP gateway IP octet 3 */
uint8_t ap_gw_addr3 = AP_GW_ADDR3_DEFAULT;

/* Wi-Fi station connection parameters */
char wlan_ssid[32 + 1]                      = WLAN_SSID_DEFAULT;
char wlan_passkey[64 + 1]                   = WLAN_PASSKEY_DEFAULT;
sl_wfx_security_mode_t wlan_security        = WLAN_SECURITY_DEFAULT;
bool wlan_security_wpa3_pmksa               = false;
char wlan_pmk[64 + 3]                       = "0";

/* Wi-Fi SoftAP connection parameters */
char softap_ssid[32 + 1]                    = SOFTAP_SSID_DEFAULT;
char softap_passkey[64 + 1]                 = SOFTAP_PASSKEY_DEFAULT;
sl_wfx_security_mode_t softap_security      = SOFTAP_SECURITY_DEFAULT;
char softap_pmk[64 + 3]                     = "0";
uint8_t softap_channel                      = SOFTAP_CHANNEL_DEFAULT;
sl_wfx_mac_address_t client_mac_address     = { { 0, 0, 0, 0, 0, 0 } };

/* Memory to store WiFi scan results from web server */
char string_list[4096];
/* Memory to store an event to display in the web page */
char event_log[50];
/* Number of registered parameters */
static int number_registered_params = 0;
/* Wifi parameters struct array */
param_t wifi_params[SL_WFX_CLI_MAX_PARAMS] = {0};
/* Number of connected client */
int number_connected_clients = 0;
/* Wifi client list */
uint8_t wifi_clients[SL_WFX_CLI_MAX_CLIENTS][6] = {0};

/* Host-to-Wi-Fi bus */
#ifdef SL_CATALOG_WFX_BUS_SDIO_PRESENT
  static char bus_name[] = SDIO_BUS;
#else
#ifdef SL_CATALOG_WFX_BUS_SPI_PRESENT
  static char bus_name[] = SPI_BUS;
#endif
#endif

/**************************************************************************//**
 * @brief: Security Modes
 * @note:  Refer to "sl_wfx_security_mode_t" enum type
 *         in the "sl_wfx_cmd_api.h" header file
 *****************************************************************************/
static const char *security_modes[] = {
  "OPEN",
  "WEP",
  "WPA1/WPA2",
  NULL,       /*!< Trick to directly link the security mode to its "name" */
  "WPA2",
  NULL,
  "WPA3",
  "WPA2/WPA3"
};

/***************************************************************************//**
 * @brief convert a given hex string to uint32_t.
 ******************************************************************************/
uint32_t convert_rate_string_to_uint32_t(char *rates_str) 
{
  char *p_end = NULL;
  return (uint32_t)strtol(rates_str, &p_end, 0);
}

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
void convert_to_lower_case_string(char *str) {
    char *p_ch = str;
    for ( ; *p_ch; *p_ch = tolower(*p_ch), p_ch++);
}

/***************************************************************************//**
 * @brief
 *    This function converts the MAC string format to MAC address of
 *    "sl_wfx_mac_address_t" format
 *
 * @param[in]
 *    + mac_str: The input string format of MAC address.
 *               E.g: XX:XX:XX:XX:XX:XX
 *
 * @param[out]
 *    + mac_addr: The output converted MAC address
 *
 * @return
 *    0 if success
 *    -1 if error
 ******************************************************************************/
int convert_str_to_mac_addr(char *mac_str, sl_wfx_mac_address_t *mac_addr)
{
  int ret;
  uint8_t *mac_addr_ptr = NULL;
  char char_buf18[18] = {0};      /*!< maximum mac address size is 18 bytes */

  mac_addr_ptr = (uint8_t *)(&(mac_addr->octet[0]));

  /* Copy mac_str to char_buf18 with 18 bytes length */
  strncpy(char_buf18, mac_str, sizeof(char_buf18));
  ret = sscanf(char_buf18,
               "%02hx:%02hx:%02hx:%02hx:%02hx:%02hx",
               (unsigned short int *) &mac_addr_ptr[0],
               (unsigned short int *) &mac_addr_ptr[1],
               (unsigned short int *) &mac_addr_ptr[2],
               (unsigned short int *) &mac_addr_ptr[3],
               (unsigned short int *) &mac_addr_ptr[4],
               (unsigned short int *) &mac_addr_ptr[5]);
  if (ret == 6) {
      return 0; /* Success */
  }
  return -1; /* Error */
}


/***************************************************************************//**
 * @brief
 *        Searches "get/set" parameters by its name and returns the index in
 *        the global wifi_params struct array if found.
 *
 * @param[in]
 *        + name: The parameter name needs to be searched
 *
 * @param[out]
 *        None
 * @return
 *        The index of parameter in the wifi_params arrays, if success.
 *        -1 if not found
 ******************************************************************************/
int param_search (char *name)
{
  int param_index = -1;
  int i;

  for (i = 0; i < number_registered_params; i++) {

      /* Loop through & search the matching name with NULL-terminated string */
      if ((strncmp(name, wifi_params[i].name, strlen(wifi_params[i].name)) == 0)
          && (name[strlen(wifi_params[i].name)] == '\0')) {
        param_index = i;
        break;
      }
  }
  return param_index;
}

/***************************************************************************//**
 * @brief
 *    Searches & returns "get/set" parameters' address by its name if found
 *
 * @param[in]
 *    + name: The parameter name needs to be searched.
 *
 * @param[out] None
 *
 * @return
 *    Parameter's address if found.
 *    NULL if not found.
 ******************************************************************************/
void *wifi_cli_get_param_addr(char *param_name)
{
  int param_idx;
  if (param_name == NULL) {
      return NULL;
  }

  param_idx = param_search(param_name);
  if (param_idx >= 0) {
      return (void*)wifi_params[param_idx].address;
  }
  return NULL;
}

/***************************************************************************//**
 * @brief
 *    Initializes CLI's event-based binary semaphore.
 *
 * @param[in]
 *    + p_cli_sem:  Pointer to CLI's semaphore
 *
 * @param[out] None
 *
 * @return  None
 ******************************************************************************/
void wifi_cli_sem_init(sem_type_t *p_cli_sem, RTOS_ERR *err)
{
  if (p_cli_sem == NULL) {
      err->Code = RTOS_ERR_FAIL;
      return;
  }

  /* Initially, set CLI's state to free */
  p_cli_sem->cli_state = FREE_CLI;

  /* CLI's binary semaphore initialization */
  OSSemCreate(&(p_cli_sem->cli_sem),
              "cli_sem",
              0,    /*!< Initial counter is 0 */
              err);
}

/***************************************************************************//**
 * @brief
 *    This function forces CLI to wait for an event with given timeout in ms
 *
 * @param[in]
 *    + p_cli_sem: Pointer to initialized CLI's binary semaphore
 *    + sem_evet_type: The waiting event
 *    + timeoutms: timeout in milliseconds
 *
 * @param[out] None
 *
 * @return
 *    RTOS_ERR_CODE
 *
 * @note: This function is called by CLI task
 ******************************************************************************/
RTOS_ERR_CODE wifi_cli_wait(sem_type_t *p_cli_sem,
                            sl_wfx_indications_ids_t sem_event_type,
                            uint32_t timeoutms)
{
  RTOS_ERR err;
  OS_TICK tmo_ticks; /*!< Timeout in OS ticks */

  err.Code = RTOS_ERR_NONE;
  /* Convert timeout in ms to OS ticks */
  tmo_ticks = (OS_TICK)(((uint64_t)timeoutms * OSCfg_TickRate_Hz) / 1000);

  if ((p_cli_sem == NULL) || (p_cli_sem->cli_state == PENDING_CLI)) {
      LOG_DEBUG("CLI's semaphore has not been initialized "
             "or being used by other\r\n");
      return RTOS_ERR_FAIL;
  }
  /* If CLI's state is free, forces CLI to wait & set state to PENDING_CLI */
  if (p_cli_sem->cli_state == FREE_CLI) {

      p_cli_sem->cli_state = PENDING_CLI;
      p_cli_sem->event_type = sem_event_type; /*!< specific waiting event */

      /* Block to wait for a confirmation with timeout */
      OSSemPend(&(p_cli_sem->cli_sem),
                tmo_ticks,
                OS_OPT_PEND_BLOCKING,
                NULL,
                &err);
  }
  /* After waiting or timeout or already posted by another task */
  p_cli_sem->cli_state = FREE_CLI;
  return err.Code;
}

/***************************************************************************//**
 * @brief
 *    This function releases CLI's event-based binary semaphore
 *
 * @param[in]
 *    + p_cli_sem: Pointer to initialized CLI's binary semaphore
 *    + sem_evet_type: The waiting event
 *
 * @param[out] None
 *
 * @return
 *    0 if success
 *    -1 if failed
 ******************************************************************************/
int wifi_cli_resume(sem_type_t *p_cli_sem,
                    sl_wfx_indications_ids_t sem_event_type)
{
  RTOS_ERR err;

  if (p_cli_sem == NULL) {
      LOG_DEBUG("CLI's binary semaphore has not been initialized!\r\n");
      return -1;
  }

  if ((p_cli_sem->cli_state == PENDING_CLI) &&
      (p_cli_sem->event_type == sem_event_type)) {

      /* Release CLI's semaphore */
      OSSemPost(&(p_cli_sem->cli_sem),
                OS_OPT_POST_ALL,
                &err);

      if (err.Code != RTOS_ERR_NONE) {
          LOG_DEBUG("Failed to release CLI's semaphore\r\n");
          return -1;
      }

  } else if (p_cli_sem->cli_state == FREE_CLI) {
      /**
       * Another task has finished this job before CLI task.
       * @note CLI does not need to block to wait for this event anymore.
       * */
      p_cli_sem->cli_state = POSTED_CLI;

  } else {
      LOG_DEBUG("Cannot post/release CLI's semaphore without using tasks\r\n");
      return -1;
  }
  return 0;
}

/***************************************************************************//**
 * @brief
 *    This callback function gets (displays) the string type parameter in the
 *    global parameter structure "wifi_params".
 *
 * @param[in]
 *    + param_name: The name of parameter
 *    + param_addr: The address of parameter
 *    + param_size: The size of parameter
 *
 * @param[out]
 *    + out_buf: The output buffer
 *    + out_buf_len: the output buffer size
 *
 * @return
 *    0 if sucess
 *    -1 if failed
 ******************************************************************************/
static int get_string_param(char *param_name,
                            void *param_addr,
                            uint32_t param_size,
                            char *out_buf,
                            uint32_t out_buf_len)
{
    (void)param_name, (void)out_buf, (void)out_buf_len, (void)param_size;

    int ret = 0;
    if (out_buf == NULL) {
        printf("%s\r\n", (char *)param_addr);
    } else {
        /* Later, if need to copy string to out_buf */
        LOG_DEBUG("Copy string to the output buffer hasn't been implemented");
    }
    return ret;
}

/***************************************************************************//**
 * @brief
 *    This callback function gets (displays) the string type parameter in the
 *    global parameter structure "wifi_params".
 *
 * @param[in]
 *    + param_name: The name of parameter
 *    + param_addr: The address of parameter
 *    + param_size: The size of parameter
 *    + new_value:  The value needs to be set
 *
 * @param[out] None
 *
 * @return
 *    0 if sucess
 *    -1 if failed
 ******************************************************************************/
static int set_string_param(char *param_name,
                            void *param_addr,
                            uint32_t param_size,
                            char *new_value)
{
  (void)param_name;

  int input_len;
  char *old_str = param_addr;
  char *err_msg = "The input string length exceeds "
                  "the maximum internal buffer length";

  input_len = strlen(new_value);
  if (input_len < (int)(param_size - 1)) {
      /* Update new value */
      strncpy(old_str, new_value, param_size);
      old_str[input_len] = '\0';    /*!< the string is NULL-terminated */
      return 0; /* Success */
  }

  LOG_DEBUG("%s: %d/%ld\r\n", err_msg, input_len, param_size);
  printf("Failed to set %s parameter\r\n", param_name);
  return -1;
}

/***************************************************************************//**
 * @brief
 *    This function gets (display) the current station/AP security mode
 *
 * @param[in]
 *    + param_name: The name of parameter
 *    + param_addr: The address of parameter
 *    + param_size: The size of parameter
 *
 * @param[out]
 *    + out_buf: The output buffer
 *    + out_buf_len: the output buffer size
 *
 * @return
 *    0 if sucess
 *    -1 if failed
 ******************************************************************************/
static int get_security_mode(char *param_name,
                             void *param_addr,
                             uint32_t param_size,
                             char *out_buf,
                             uint32_t out_buf_len)
{
  (void)param_name, (void)out_buf, (void)out_buf_len;

  int security_idx;

  /** Get security value in "sl_wfx_security_mode_t" enum type.
   * @note  The size of enum type is not standardized but
   *        depends on the specific compilers.
   * */
  switch (param_size) {
    case 4:
      security_idx = *(uint32_t *)(param_addr); /*!< enum size is 4 bytes */
      break;

    case 2:
      security_idx = *(uint16_t *)(param_addr); /*!< enum size is 2 bytes */
      break;

    case 1:
      security_idx = *(uint8_t *)(param_addr); /*!< enum size is 1 byte */
      break;

    default:
      security_idx = -1;  /*!< Don't support this enum size */
      break;
  }

  if (security_idx < 0) {
      LOG_DEBUG("Unknown size of sl_wfx_security_mode_t "
                "enum type! Only support 1, 2, 4 byte-size\r\n");
      printf("Failed to get security mode\r\n");
      return -1;
  }
  printf("%s\r\n", security_modes[security_idx]);
  /* Check if the PMKSA caching is in use in WPA3 or WPA2/WPA3 transition mode */
  if ((strcmp(security_modes[security_idx], "WPA3") == 0) || 
  (strcmp(security_modes[security_idx], "WPA2/WPA3") == 0)) {
    if (wlan_security_wpa3_pmksa) {
      printf("(PMKSA caching enabled)\r\n");
    }
    else {
      printf("(PMKSA caching disabled)\r\n");
    }
  }

  return 0;
}

/***************************************************************************//**
 * @brief
 *    This function sets station/AP security modes by updating to the global
 *    wifi_param struct array
 *
 * @param[in]
 *    + param_name: The name of parameter
 *    + param_addr: The address of parameter
 *    + param_size: The size of parameter
 *    + new_value:  The value needs to be set
 *
 * @param[out] None
 *
 * @return
 *    0 if sucess
 *    -1 if failed
 ******************************************************************************/
static int set_security_mode(char *param_name,
                             void *param_addr,
                             uint32_t param_size,
                             char *new_value)
{
  (void)param_name, (void)param_size;

  uint8_t i;
  bool is_matched;
  uint32_t security_mode_nb;
  sl_wfx_security_mode_t *mode_ptr = NULL;

  /* The size of security_modes array */
  security_mode_nb =  sizeof(security_modes) / sizeof(char*);
  /* Pointer to the global security mode parameter */
  mode_ptr = (sl_wfx_security_mode_t *)param_addr;

  for (i = 0; i < security_mode_nb; i++) {
      /* Loop through & compare the input security mode vs supported ones */
      is_matched = strlen(security_modes[i]) && /*!< Skips the NULL entry */
                  (!strcmp(new_value, security_modes[i]));

      /* If the security mode is found, update the new security mode then exit */
      if (is_matched == true) {
         *mode_ptr = (sl_wfx_security_mode_t) i;
          return 0;
      }
  }
  printf("The input security mode (%s) is not supported\r\n", new_value);
  return -1;
}

/***************************************************************************//**
 * @brief
 *    This function gets (displays) network interfaces such as
 *    ip, gateway, netmask address
 *
 * @param[in]
 *    + param_name: The name of parameter
 *    + param_addr: The address of parameter
 *    + param_size: The size of parameter
 *
 * @param[out]
 *    + out_buf: The output buffer
 *    + out_buf_len: the output buffer size
 *
 * @return
 *    0 if sucess
 *    -1 if failed
 ******************************************************************************/
static int get_network_interface(char *param_name,
                                 void *param_addr,
                                 uint32_t param_size,
                                 char *out_buf,
                                 uint32_t out_buf_len)
{
  (void)param_size, (void)out_buf, (void)out_buf_len;

  uint8_t *ip_ptr = NULL;
  uint8_t *netmask_ip_ptr = NULL;
  uint8_t *gateway_ip_ptr = NULL;
  struct netif *netif_ptr = NULL;

  /* Pointer to sta_netif or ap_netif struct */
  netif_ptr = (struct netif *)param_addr;

  if (strstr(param_name, "netmask") != NULL) {
      /* Pointer to the netmask address */
      netmask_ip_ptr = (uint8_t *)&(netif_ptr->netmask.addr);

      /*< Get the network interface's netmask address */
      printf("%d.%d.%d.%d\r\n", netmask_ip_ptr[0], \
                                netmask_ip_ptr[1], \
                                netmask_ip_ptr[2], \
                                netmask_ip_ptr[3]);

  } else if (strstr(param_name, "gateway") != NULL) {
      /* Pointer to network interface's gateway address */
      gateway_ip_ptr = (uint8_t *)&(netif_ptr->gw.addr);

      /* Get the network interface's netmask address */
      printf("%d.%d.%d.%d\r\n", gateway_ip_ptr[0], \
                                gateway_ip_ptr[1], \
                                gateway_ip_ptr[2], \
                                gateway_ip_ptr[3]);

  } else if (strstr(param_name, "ip") != NULL) {
      /* Pointer to network interface's ip address */
      ip_ptr = (uint8_t *)&(netif_ptr->ip_addr.addr);

      /* Get the network interface's ip address */
      printf("%d.%d.%d.%d\r\n", ip_ptr[0], ip_ptr[1], ip_ptr[2], ip_ptr[3]);
  }
  return 0;
}

/***************************************************************************//**
 * @brief
 *    This function sets network interfaces such as ip, gateway, netmask address
 *
 * @param[in]
 *    + param_name: The name of parameter
 *    + param_addr: The address of parameter
 *    + param_size: The size of parameter
 *    + new_value:  The value needs to be set
 *
 * @param[out] None
 *
 * @return
 *    0 if sucess
 *    -1 if failed
 ******************************************************************************/
static int set_network_interface(char *param_name,
                                 void *param_addr,
                                 uint32_t param_size,
                                 char *new_value)
{
  (void)param_size;
  ip_addr_t newIP;
  struct netif *netif_ptr = NULL;

  /* Pointer to sta_netif or ap_netif struct */
  netif_ptr = (struct netif *)param_addr;

  if (ipaddr_aton(new_value, &newIP) == 0) {
      printf("Failed to convert %s (%s) to ipv4 format\r\n",
             param_name, new_value);
      return -1;
  }

  if (strstr(param_name, "netmask") != NULL) {
      /* Update the interface's netmask address */
      netif_set_netmask(netif_ptr, &newIP);

  } else if (strstr(param_name, "gateway") != NULL) {
      /* Set new gateway to network interface */
      netif_set_gw(netif_ptr, &newIP);

  } else if (strstr(param_name, "ip") != NULL) {
      /* Set new station IP address to network interface */
      netif_set_ipaddr(netif_ptr, &newIP);

  } else {
      LOG_DEBUG("Can't set %s", param_name);
      return -1; /* Failed to set netif */
  }
  return 0; /* Success */
}

/***************************************************************************//**
 * @brief
 *    This function sets MAC address
 *
 * @param[in]
 *    + param_name: The name of parameter
 *    + param_addr: The address of parameter
 *    + param_size: The size of parameter
 *    + new_value:  The value needs to be set
 *
 * @param[out] None
 *
 * @return
 *    0 if sucess
 *    -1 if failed
 ******************************************************************************/
static int set_mac_addr(char *param_name,
                        void *param_addr,
                        uint32_t param_size,
                        char *new_value)
{

  (void)param_size, (void)param_addr, (void)param_size;

  int ret;
  int param_idx;
  netif_init_fn netif_init;
  sl_wfx_interface_t interface;
  sl_wfx_mac_address_t new_mac;
  struct netif *netif_ptr = NULL;

  if (strcmp(param_name, "station.mac") == 0) {
      /* Station: search for the "sta_netif" parameter */
      param_idx = param_search("station.ip");
      if (param_idx < 0) {
          LOG_DEBUG("The sta_netif parameter not registered yet\r\n");
          goto error;
      }

      interface = SL_WFX_STA_INTERFACE;
      netif_init = sta_ethernetif_init;

  } else if (strcmp(param_name, "softap.mac") == 0) {
      /* SoftAP: search for the "ap_netif" parameter */
      param_idx = param_search("softap.ip");
      if (param_idx < 0) {
          LOG_DEBUG("The ap_netif parameter not registered yet\r\n");
          goto error;
      }

      interface = SL_WFX_SOFTAP_INTERFACE;
      netif_init = ap_ethernetif_init;

  } else {
      LOG_DEBUG("Does not support set \"%s\"\r\n", param_name);
      goto error;
  }

  /* Pointer to sta_netif or ap_netif */
  netif_ptr = (struct netif *)wifi_params[param_idx].address;

  /* Convert mac_str to MAC address */
  if (convert_str_to_mac_addr(new_value, &new_mac)) {
      printf("Failed to convert the input string (%s) "
             "to MAC address \r\n", new_value);
      return -1;
  }

  /* Apply the new MAC address */
  ret = sl_wfx_set_mac_address(&new_mac, interface);
  if (ret == 0) {
      /** Update to the global param "wifi.mac_addr_0.octet[]"
       *  @note This has been done in FMAC driver
       * */
      /*< Update the LwIP stack state */
      netif_init(netif_ptr);
      return 0;
  }

error:
  printf("Failed to set %s\r\n", param_name);
  return -1;
}

/***************************************************************************//**
 * @brief
 *    This function converts the Message Authentication Code
 *    (MAC) key string to a 32-byte array.
 *
 * @param[in]
 *    + mackey_str: The input string of mac key in the square brackets form
 *    + arrSize:  The output buffer size
 *
 * @param[out]
 *    + mackey_arr: The output array contains converted MAC key
 *
 * @return  
 *          0 if success
 *          -1 if failed
 *
 * @note    mackey_str must be expressed in the 32-byte array form of square
 *          brackets.
 *          E.g.: [0x42, 0x44,.., 0xB3].
 ******************************************************************************/
int convert_mac_key_string_to_array(char *mackey_str,
                                    uint8_t *mackey_arr,
                                    uint8_t arrSize)
{
  uint8_t i;
  char *p_end = NULL;
  char *p_str = mackey_str;
  int strLen = strlen(mackey_str);
  char *err_msg = "Format or array size error\r\n";

  if ((mackey_str[0] != '[') && (mackey_str[strLen - 1] != ']')) {
      /* mackey_str is not having opening & closing square brackets [...] */
      goto error;
  }

  p_str++;    /*!< Skips the opening square bracket '[' */

  for (i = 0; i < arrSize; i++) {
      /* Convert a string to long integer, the base is determined by string. */
      mackey_arr[i] = (uint8_t)strtol(p_str, &p_end, 0);

      /** The scan ended at the first character that is inconsistent with
       *  the base. If failed, p_end = p_str.
       *  */
      if (p_end == p_str) {
          /* Failed to convert the current string to long integer. E.g: 0xMJ */
          goto error;
      }

      /* Eliminate all invalid characters, p_end must point to "," separator */
      while (*p_end != ',') p_end++;

      /* Skips the "," comma separator */
      p_str = p_end + 1;
  }
  return 0; /* success */

error:
  printf("%s\r\n", err_msg);
  return -1;
}

/***************************************************************************//**
 * @brief
 *    This function checks wifi the validity of registered wifi params.
 * @param[in]
 *    + param: The param_t struct pointer to the current param
 *
 * @param[out] None
 *
 * @return
 *    + true if valid
 *    + false if invalid
 ******************************************************************************/
static bool check_validity (param_t *param)
{
  bool is_valid = false;

  if ((param->name != NULL)
      && (param->name[0] != '\0')
      && (param->rights > 0)
      && (param->type >= SL_WFX_CLI_PARAM_TYPE_INTEGER) /* First */
      && (param->type <= SL_WFX_CLI_PARAM_TYPE_CUSTOM)  /* Last */) {

    if ((param->address == NULL)
        && (param->type != SL_WFX_CLI_PARAM_TYPE_CUSTOM)) {
      /* Only allow parameter address null with custom types */ 
      LOG_DEBUG("Address can't be NULL! \
                Parameter: %s\r\n", param->name);
    } else if ((param->type == SL_WFX_CLI_PARAM_TYPE_INTEGER)
               || (param->type == SL_WFX_CLI_PARAM_TYPE_UNSIGNED_INTEGER)
               || (param->type == SL_WFX_CLI_PARAM_TYPE_HEXADECIMAL)) {
      /* Check the parameter size */
      if ((param->size == 1)
          || (param->size == 2)
          || (param->size == 4)) {
        is_valid = true;
      } else {
          LOG_DEBUG("Unsupported size! Only support 1/2/4 byte-size! \
                     Parameter: %s\r\n", param->name);
      }
    } else {
      is_valid = true;
    }
  } else {
      LOG_DEBUG("Wrong input (name, address, rights or type)\r\n");
  }

  return is_valid;
}

/***************************************************************************//**
 * @brief
 *    This function registers wifi get/set parameters & corresponding processing
 *    callback functions to the global wifi_params struct array.
 *
 * @param[in]
 *    + name: The name of parameter
 *    + address: The address of parameter
 *    + description: The parameter description
 *    + get_func: The get callback function
 *    + set_func: The set callback function
 *    + type: The type of parameter
 *    + size: The size of parameter
 *    + right: The permission to get/set
 *
 * @param[out] None
 *
 * @return
 *    0 if sucess
 *    -1 if failed
 ******************************************************************************/
static int sl_wfx_cli_register_wifi_param(char *name,
                                          void *address,
                                          char *description,
                                          sl_wfx_cli_param_custom_get_func_t get_func,
                                          sl_wfx_cli_param_custom_set_func_t set_func,
                                          sl_wfx_cli_param_type_t type,
                                          uint8_t size,
                                          uint8_t rights)
{
  int ret = -1;
  param_t param = {0};

  if (number_registered_params < SL_WFX_CLI_MAX_PARAMS) {
    param.name = name;
    param.address = address;
    param.description = description;
    param.size = size;
    param.rights = rights;
    param.type = type;
    param.get_func = get_func;
    param.set_func = set_func;

    /* Check the validity of the param registered in the wifi_params array */
    if (check_validity(&param)) {
      memcpy(&wifi_params[number_registered_params], &param, sizeof(param_t));
      number_registered_params++;
      ret = 0;
    }
  }

  return ret;
}

/***************************************************************************//**
 * @brief
 *    This function registers all wifi param to the global wifi_param struct array
 *
 * @param[in]
 *
 * @param[out] None
 *
 * @return
 *    0 if success
 *    -1 if failed
 ******************************************************************************/
int register_wifi_params(void)
{
  int ret = -1;

  /* Add wifi-bus to global params struct */
  ret = sl_wfx_cli_register_wifi_param("wifi.bus",
                                       (void *)&bus_name[0],
                                       "bus between the WiFi chip & the host",
                                       get_string_param,
                                       NULL,
                                       SL_WFX_CLI_PARAM_TYPE_STRING,
                                       sizeof(bus_name),
                                       SL_WFX_CLI_PARAM_GET_RIGHT);

  /* Add station ssid to global params struct */
  ret |= sl_wfx_cli_register_wifi_param("station.ssid",
                                        (void *)&wlan_ssid[0],
                                        "wlan ssid (max 32 characters)",
                                        get_string_param,
                                        set_string_param,
                                        SL_WFX_CLI_PARAM_TYPE_STRING,
                                        sizeof(wlan_ssid),
                                        SL_WFX_CLI_PARAM_GET_RIGHT | \
                                        SL_WFX_CLI_PARAM_SET_RIGHT);

  /* Add station passkey to global params struct */
  ret |= sl_wfx_cli_register_wifi_param("station.passkey",
                                        (void *)&wlan_passkey[0],
                                        "wlan passkey (max 64 characters)",
                                        get_string_param,
                                        set_string_param,
                                        SL_WFX_CLI_PARAM_TYPE_STRING,
                                        sizeof(wlan_passkey),
                                        SL_WFX_CLI_PARAM_GET_RIGHT | \
                                        SL_WFX_CLI_PARAM_SET_RIGHT);

  /* Add station security to global params struct */
  ret |= sl_wfx_cli_register_wifi_param("station.security",
                                        (void *)&wlan_security,
                                        "WLAN security mode"
                                        "[OPEN, WEP, WPA1/WPA2, WPA2,WPA3]",
                                        get_security_mode,
                                        set_security_mode,
                                        SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                        sizeof(wlan_security),
                                        SL_WFX_CLI_PARAM_GET_RIGHT | \
                                        SL_WFX_CLI_PARAM_SET_RIGHT);

  /* Add station dhcp client state to global params struct */
  ret |= sl_wfx_cli_register_wifi_param("station.dhcp_client_state",
                                        (void *)&use_dhcp_client,
                                        "Station DHCP client state",
                                        NULL,
                                        NULL,
                                        SL_WFX_CLI_PARAM_TYPE_INTEGER,
                                        sizeof(use_dhcp_client),
                                        SL_WFX_CLI_PARAM_GET_RIGHT | \
                                        SL_WFX_CLI_PARAM_SET_RIGHT);

  /* Add station network interface to global params struct */
  ret |= sl_wfx_cli_register_wifi_param("station.netmask",
                                        (void *)&sta_netif,
                                        "Station net mask (IPv4)",
                                        get_network_interface,
                                        set_network_interface,
                                        SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                        sizeof(sta_netif.netmask),
                                        SL_WFX_CLI_PARAM_GET_RIGHT | \
                                        SL_WFX_CLI_PARAM_SET_RIGHT);

  /* Add station gateway to global params struct */
  ret |= sl_wfx_cli_register_wifi_param("station.gateway",
                                        (void *)&sta_netif,
                                        "Station gateway IP address (IPv4)",
                                        get_network_interface,
                                        set_network_interface,
                                        SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                        sizeof(sta_netif.gw),
                                        SL_WFX_CLI_PARAM_GET_RIGHT | \
                                        SL_WFX_CLI_PARAM_SET_RIGHT);

  /* Add station ip to global params struct */
  ret |= sl_wfx_cli_register_wifi_param("station.ip",
                                        (void *)&sta_netif,
                                        "Station IP (IPv4)",
                                        get_network_interface,
                                        set_network_interface,
                                        SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                        sizeof(sta_netif.ip_addr),
                                        SL_WFX_CLI_PARAM_GET_RIGHT | \
                                        SL_WFX_CLI_PARAM_SET_RIGHT);

  /* Add station pmk to global params struct */
  ret |= sl_wfx_cli_register_wifi_param("station.pmk",
                                        (void *)&wlan_pmk,
                                        "Station wlan pairwise master key",
                                        NULL,
                                        NULL,
                                        SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                        sizeof(wlan_pmk),
                                        SL_WFX_CLI_PARAM_GET_RIGHT);

  /* Add station mac to global params struct */
  ret |= sl_wfx_cli_register_wifi_param("station.mac",
                                       (void *)&wifi.mac_addr_0.octet,
                                       "Station wlan MAC address (EUI-48 format)",
                                       NULL,
                                       set_mac_addr,
                                       SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                       sizeof(wifi.mac_addr_0.octet),
                                       SL_WFX_CLI_PARAM_GET_RIGHT | \
                                       SL_WFX_CLI_PARAM_SET_RIGHT);

  /** ---- Adding SoftAP wifi params to the global wifi_params struct --------*/
  /* Add softap ssid to the global wifi_params struct */
  ret |= sl_wfx_cli_register_wifi_param("softap.ssid",
                                        (void *)&softap_ssid[0],
                                        "Softap ssid (Max 32 bytes length)",
                                        get_string_param,
                                        set_string_param,
                                        SL_WFX_CLI_PARAM_TYPE_STRING,
                                        sizeof(softap_ssid),
                                        SL_WFX_CLI_PARAM_GET_RIGHT | \
                                        SL_WFX_CLI_PARAM_SET_RIGHT);

  /* Add softap passkey to the global wifi_params struct */
  ret |= sl_wfx_cli_register_wifi_param("softap.passkey",
                                        (void *)&softap_passkey[0],
                                        "Softap passkey (Max 64 bytes length)",
                                        get_string_param,
                                        set_string_param,
                                        SL_WFX_CLI_PARAM_TYPE_STRING,
                                        sizeof(softap_passkey),
                                        SL_WFX_CLI_PARAM_GET_RIGHT | \
                                        SL_WFX_CLI_PARAM_SET_RIGHT);

  /* Add softap security to the global wifi_params struct */
  ret |= sl_wfx_cli_register_wifi_param("softap.security",
                                        (void *)&softap_security,
                                        "SoftAP security mode "
                                        "[OPEN, WEP, WPA1/WPA2, WPA2, WPA3, WPA2/WPA3]",
                                        get_security_mode,
                                        set_security_mode,
                                        SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                        sizeof(softap_security),
                                        SL_WFX_CLI_PARAM_GET_RIGHT | \
                                        SL_WFX_CLI_PARAM_SET_RIGHT);

  /* Add softap channel to global params struct */
  ret |= sl_wfx_cli_register_wifi_param("softap.channel",
                                        (void *)&softap_channel,
                                        "SoftAP channel (decimal)",
                                        NULL,
                                        NULL,
                                        SL_WFX_CLI_PARAM_TYPE_UNSIGNED_INTEGER,
                                        sizeof(softap_channel),
                                        SL_WFX_CLI_PARAM_GET_RIGHT | \
                                        SL_WFX_CLI_PARAM_SET_RIGHT);

  /* Add softap network interface to global params struct */
  ret |= sl_wfx_cli_register_wifi_param("softap.netmask",
                                        (void *)&ap_netif,
                                        "SoftAP net mask (IPv4)",
                                        get_network_interface,
                                        set_network_interface,
                                        SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                        sizeof(ap_netif.netmask),
                                        SL_WFX_CLI_PARAM_GET_RIGHT | \
                                        SL_WFX_CLI_PARAM_SET_RIGHT);

  /* Add softap gateway to global params struct */
  ret |= sl_wfx_cli_register_wifi_param("softap.gateway",
                                        (void *)&ap_netif,
                                        "SoftAP gateway IP address (IPv4)",
                                        get_network_interface,
                                        set_network_interface,
                                        SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                        sizeof(ap_netif.gw),
                                        SL_WFX_CLI_PARAM_GET_RIGHT | \
                                        SL_WFX_CLI_PARAM_SET_RIGHT);

  /* Add softap ip to global params struct */
  ret |= sl_wfx_cli_register_wifi_param("softap.ip",
                                        (void *)&ap_netif,
                                        "SoftAP IP (IPv4)",
                                        get_network_interface,
                                        set_network_interface,
                                        SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                        sizeof(ap_netif.ip_addr),
                                        SL_WFX_CLI_PARAM_GET_RIGHT | \
                                        SL_WFX_CLI_PARAM_SET_RIGHT);

  /* Add Softap pmk to global params struct */
  ret |= sl_wfx_cli_register_wifi_param("softap.pmk",
                                        (void *)&softap_pmk,
                                        "SoftAP wlan pairwise master key",
                                        NULL,
                                        NULL,
                                        SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                        sizeof(softap_pmk),
                                        SL_WFX_CLI_PARAM_GET_RIGHT);

  /* Add SoftAP mac to global params struct */
  ret |= sl_wfx_cli_register_wifi_param("softap.mac",
                                       (void *)&wifi.mac_addr_1.octet,
                                       "SoftAP MAC address (EUI-48 format)",
                                       NULL,
                                       set_mac_addr,
                                       SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                       sizeof(wifi.mac_addr_1.octet),
                                       SL_WFX_CLI_PARAM_GET_RIGHT | \
                                       SL_WFX_CLI_PARAM_SET_RIGHT);

  /* Add SoftAP dhcp server state to global params struct */
  ret |= sl_wfx_cli_register_wifi_param("softap.dhcp_server_state",
                                        (void *)&use_dhcp_server,
                                        "SoftAP DHCP server state",
                                        NULL,
                                        NULL,
                                        SL_WFX_CLI_PARAM_TYPE_UNSIGNED_INTEGER,
                                        sizeof(use_dhcp_server),
                                        SL_WFX_CLI_PARAM_GET_RIGHT | \
                                        SL_WFX_CLI_PARAM_SET_RIGHT);
  /* NOTE: Add maximum SL_WFX_CLI_MAX_PARAMS parameters to this global struct */

  nvm3_readData(nvm3_defaultHandle, 
                NVM3_KEY_AP_SSID, 
                (void *)wlan_ssid, 
                sizeof(wlan_ssid));

  nvm3_readData(nvm3_defaultHandle, 
                NVM3_KEY_AP_PASSKEY,
                (void *)wlan_passkey, 
                sizeof(wlan_passkey));

  nvm3_readData(nvm3_defaultHandle, 
                NVM3_KEY_AP_SECURITY_MODE, 
                (void *)&wlan_security, 
                sizeof(wlan_security));

  nvm3_readData(nvm3_defaultHandle,
                NVM3_KEY_AP_SECURITY_WPA3_PMKSA,
                (void *)&wlan_security_wpa3_pmksa,
                sizeof(wlan_security_wpa3_pmksa));

  return ret;
}

/***************************************************************************//**
 * @brief
 *    The task registers all wifi get/set parameters
 *
 * @param[in]
 *
 * @param[out] None
 *
 * @return  None
 ******************************************************************************/
static void wfx_cli_params_task(void *p_arg) {
  RTOS_ERR err;
  PP_UNUSED_PARAM(p_arg);

  /* Register global get/set wifi parameters */
  if (register_wifi_params()) {
      LOG_DEBUG("Failed to register the Wi-Fi's get/set parameters\r\n");
  }

  /* Delete this task */
  OSTaskDel(NULL, &err);
}

/***************************************************************************//**
 * @brief
 *    Create Wi-Fi CLI's get/set parameter initialization task
 *
 * @param[in]
 *
 * @param[out] None
 *
 * @return  None
 ******************************************************************************/
void wifi_cli_params_init(void)
{
  RTOS_ERR err;

  /* Create the wifi cli params registration task. */
  OSTaskCreate(&wfx_cli_params_task_tcb,
               "Start wifi CLI params registration task",
               wfx_cli_params_task,
               DEF_NULL,
               WFX_CLI_PARAMS_TASK_PRIO,
               &wfx_cli_params_task_stk[0],
               (WFX_CLI_PARAMS_TASK_STK_SIZE / 10u),
               WFX_CLI_PARAMS_TASK_STK_SIZE,
               0u,
               0u,
               DEF_NULL,
               (OS_OPT_TASK_STK_CLR),
               &err);

  /* Check err code */
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
}
