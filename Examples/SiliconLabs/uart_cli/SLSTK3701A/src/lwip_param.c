/**************************************************************************//**
 * Copyright 2019, Silicon Laboratories Inc.
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

#include "app_version.h"
#include "demo_config.h"
#include "sl_wfx_host.h"
#include "sl_wfx_cli_common.h"
#include "dhcp_client.h"
#include "dhcp_server.h"
#include "ethernetif.h"
#include "lwip/ip_addr.h"
#include "lwip/dhcp.h"
#include "lwip/netif.h"

#ifdef SL_WFX_USE_SECURE_LINK
extern uint8_t secure_link_mac_key[SL_WFX_SECURE_LINK_MAC_KEY_LENGTH];
#endif

#ifdef EFM32GG11B820F2048GM64
static char target_name[] = "wgm160p";
#else
static char target_name[] = "efm32gg11";
#endif

#if SL_WFX_USE_SDIO
static char bus_name[] = "sdio";
#else
static char bus_name[] = "spi";
#endif

static const char *security_modes[] = {
  "OPEN",
  "WEP",
  "WPA1/WPA2",
  NULL,       /* Trick to link directly the security mode to its "name".*/
  "WPA2",
  NULL,
  "WPA3"
};

static int set_security_mode (char *param_name,
                              void *param_addr,
                              char *new_value) {
  sl_wfx_security_mode_t *mode = (sl_wfx_security_mode_t *)param_addr;
  int ret = -1;
  uint8_t i;

  (void)param_name;

  /* Iterate through the security modes*/
  for (i=0; i<(sizeof(security_modes)/sizeof(char*)); i++) {
    if ((strncmp(new_value, security_modes[i], strlen(security_modes[i])) == 0)
        && (new_value[strlen(security_modes[i])] == '\0')) {

      /* Security mode found*/
      *mode = (sl_wfx_security_mode_t)i;
      ret = 0;
      break;
    }
  }

  return ret;
}

static int get_security_mode (char *param_name,
                              void *param_addr,
                              char *output_buf,
                              uint32_t output_buf_len) {
  int ret = -1;
  sl_wfx_security_mode_t mode = *(sl_wfx_security_mode_t *)param_addr;

  (void)param_name;

  if (mode < (sizeof(security_modes)/sizeof(char*))) {
    snprintf(output_buf, output_buf_len, "%s\r\n", (char *)security_modes[mode]);
    ret = 0;
  }

  return ret;
}

static int convert_str_to_ipv4 (char *ipv4, ip_addr_t *ip) {
  uint8_t *ptr = (uint8_t *)(&(ip->addr));
  char buf[16] = {0}; //IP v4 max size
  int ret = -1;

  /* Ensure the input size*/
  strncpy(buf, ipv4, sizeof(buf));
  buf[sizeof(buf)-1] = 0;

  /* Extract the IP address
     Should be "%hhu.%hhu.%hhu.%hhu" but it
     doesn't work with nano LibC*/
  ret = sscanf(buf,
               "%hu.%hu.%hu.%hu",
               (short unsigned int *)&ptr[0],
               (short unsigned int *)&ptr[1],
               (short unsigned int *)&ptr[2],
               (short unsigned int *)&ptr[3]);
  if (ret == 4) {
    /* Success, 4 numbers have been extracted*/
    ret = 0;
  }

  return ret;
}

static int convert_ipv4_to_str (ip_addr_t ip,
                                char *output_buf,
                                uint32_t output_buf_len) {
  uint8_t *ptr = (uint8_t *)&ip.addr;

  snprintf(output_buf,
           output_buf_len,
           "%d.%d.%d.%d\r\n",
           ptr[0], ptr[1], ptr[2], ptr[3]);

  return 0;
}

static int set_netif_netmask (char *param_name,
                              void *param_addr,
                              char *new_value) {
  struct netif *netif = (struct netif *)param_addr;
  ip_addr_t new_ip;
  int ret;

  (void)param_name;

  /* Convert the IP address*/
  ret = convert_str_to_ipv4(new_value, &new_ip);
  if (ret == 0) {
    /* Update the interface's netmask address*/
    netif_set_netmask(netif, &new_ip);
  }

  return ret;
}

static int get_netif_netmask (char *param_name,
                              void *param_addr,
                              char *output_buf,
                              uint32_t output_buf_len) {
  struct netif *netif = (struct netif *)param_addr;

  (void)param_name;

  return convert_ipv4_to_str(netif->netmask, output_buf, output_buf_len);
}

static int set_netif_gateway (char *param_name,
                              void *param_addr,
                              char *new_value) {
  struct netif *netif = (struct netif *)param_addr;
  ip_addr_t new_ip;
  int ret;

  (void)param_name;

  /* Convert the IP address*/
  ret = convert_str_to_ipv4(new_value, &new_ip);
  if (ret == 0) {
    /* Update the interface's gateway address*/
    netif_set_gw(netif, &new_ip);
  }

  return ret;
}

static int get_netif_gateway (char *param_name,
                              void *param_addr,
                              char *output_buf,
                              uint32_t output_buf_len) {
  struct netif *netif = (struct netif *)param_addr;

  (void)param_name;

  return convert_ipv4_to_str(netif->gw, output_buf, output_buf_len);
}

static int set_netif_ipaddr (char *param_name,
                             void *param_addr,
                             char *new_value) {
  struct netif *netif = (struct netif *)param_addr;
  ip_addr_t new_ip;
  int ret;

  (void)param_name;

  /* Convert the IP address*/
  ret = convert_str_to_ipv4(new_value, &new_ip);
  if (ret == 0) {
    /* Update the interface's IP address*/
    netif_set_ipaddr(netif, &new_ip);
  }

  return ret;
}

static int get_netif_ipaddr (char *param_name,
                             void *param_addr,
                             char *output_buf,
                             uint32_t output_buf_len) {
  struct netif *netif = (struct netif *)param_addr;

  (void)param_name;

  return convert_ipv4_to_str(netif->ip_addr, output_buf, output_buf_len);
}

static int set_dhcp_client_state (char *param_name,
                                  void *param_addr,
                                  char *new_value) {
  ip_addr_t sta_ipaddr, sta_netmask, sta_gw;
  int res;

  (void)param_name;
  (void)param_addr;

  /* To limit undefined behaviors and ease the development
     only accept a DHCP state change while the WLAN interface is down.*/
  if ((wifi_context.state & SL_WFX_STA_INTERFACE_CONNECTED) == 0) {
    /* Update the DHCP client indicator*/
    use_dhcp_client = !!atoi(new_value);

    if (use_dhcp_client == 0) {
      /* Disable the DHCP requests*/
      dhcpclient_set_link_state(0);

        /* Restore default static addresses*/
        IP_ADDR4(&sta_ipaddr, sta_ip_addr0, sta_ip_addr1, sta_ip_addr2, sta_ip_addr3);
        IP_ADDR4(&sta_netmask, sta_netmask_addr0, sta_netmask_addr1, sta_netmask_addr2, sta_netmask_addr3);
        IP_ADDR4(&sta_gw, sta_gw_addr0, sta_gw_addr1, sta_gw_addr2, sta_gw_addr3);
        netif_set_addr(&sta_netif, &sta_ipaddr, &sta_netmask, &sta_gw);
    } else {
        /* Clear current address*/
        netif_set_addr(&sta_netif, NULL, NULL, NULL);
        /* Let the connect event enable the DHCP requests*/
    }

    res = 0;
  } else {
    printf("Operation denied: stop WLAN first\r\n");
    res = -1;
  }

  return res;
}

static int get_dhcp_client_state (char *param_name,
                                  void *param_addr,
                                  char *output_buf,
                                  uint32_t output_buf_len) {
  (void)param_name;
  (void)param_addr;

  snprintf(output_buf, output_buf_len,  "%u\r\n", use_dhcp_client);

  return 0;
}

static int set_dhcp_server_state (char *param_name,
                                  void *param_addr,
                                  char *new_value) {
  int res;

  (void)param_name;
  (void)param_addr;

  /* No known issues but limit this update for consistency with the DHCP client.*/
  if ((wifi_context.state & SL_WFX_AP_INTERFACE_UP) == 0) {
    use_dhcp_server = !!atoi(new_value);

    if ((use_dhcp_server == 0)
        && dhcpserver_is_started()) {
      dhcpserver_stop();
    } /*else let the SoftAP start configure the DHCP server*/

    res = 0;
  } else {
    printf("Operation denied: stop SoftAP first\r\n");
    res = -1;
  }

  return res;
}

static int get_dhcp_server_state (char *param_name,
                                  void *param_addr,
                                  char *output_buf,
                                  uint32_t output_buf_len) {
  (void)param_name;
  (void)param_addr;

  snprintf(output_buf, output_buf_len, "%u\r\n", use_dhcp_server);

  return 0;
}

static int get_station_pmk (char *param_name,
                            void *param_addr,
                            char *output_buf,
                            uint32_t output_buf_len) {
  uint32_t password_length;
  sl_status_t status;
  int ret = -1;

  (void)param_name;
  (void)param_addr;

  status = sl_wfx_get_pmk(&wlan_pmk, &password_length, SL_WFX_STA_INTERFACE);

  if (status == SL_STATUS_OK) {
    if (output_buf_len > (password_length + 2/*EOF*/)) {
      memcpy((uint8_t *)output_buf, wlan_pmk.password, password_length);
      sprintf(&output_buf[password_length], "\r\n");
      ret = 0;
    } else {
      printf("Output buffer too small (%ld/%ld)\r\n", password_length, output_buf_len);
    }
  } else {
    printf("Interface down\r\n");
  }

  return ret;
}

static int get_softap_pmk (char *param_name,
                           void *param_addr,
                           char *output_buf,
                           uint32_t output_buf_len) {
  uint32_t password_length = 0;
  sl_status_t status;
  int ret = -1;

  (void)param_name;
  (void)param_addr;

  status = sl_wfx_get_pmk(&softap_pmk, &password_length, SL_WFX_SOFTAP_INTERFACE);

  if (status == SL_STATUS_OK) {
    if (output_buf_len > (password_length + 2/*EOF*/)) {
      memcpy((uint8_t *)output_buf, softap_pmk.password, password_length);
      sprintf(&output_buf[password_length], "\r\n");
      ret = 0;
    } else {
      printf("Output buffer too small (%ld/%ld)\r\n", password_length, output_buf_len);
    }
  } else {
    printf("Interface down\r\n");
  }

  return ret;
}

static int set_mac_addr (char *param_name,
                         void *param_addr,
                         char *new_value) {
  netif_init_fn netif_init;
  struct netif *netif;
  sl_wfx_interface_t interface;
  sl_wfx_mac_address_t new_mac;
  char buf[18] = {0}; /* Mac address size*/
  int ret;

  /* Ensure the input size*/
  strncpy(buf, new_value, sizeof(buf));
  buf[sizeof(buf)-1] = 0;

  /* Extract the IP address
     Should be "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx" but it
     doesn't work with the nano LibC*/
  ret = sscanf(buf,
               "%02hx:%02hx:%02hx:%02hx:%02hx:%02hx",
               (short unsigned int *)&new_mac.octet[0],
               (short unsigned int *)&new_mac.octet[1],
               (short unsigned int *)&new_mac.octet[2],
               (short unsigned int *)&new_mac.octet[3],
               (short unsigned int *)&new_mac.octet[4],
               (short unsigned int *)&new_mac.octet[5]);
  if (ret == 6) {
    /* Success, 6 numbers have been extracted*/

    /* Check which interface it is*/
    if ((uint8_t *)param_addr == &wifi_context.mac_addr_0.octet[0]) {
      interface = SL_WFX_STA_INTERFACE;
      netif_init = sta_ethernetif_init;
      netif = &sta_netif;
    } else {
      interface = SL_WFX_SOFTAP_INTERFACE;
      netif_init = ap_ethernetif_init;
      netif = &ap_netif;
    }

    /* Apply the new mac address*/
    ret = (int)sl_wfx_set_mac_address(&new_mac, interface);
    if (ret == 0) {
      /* Update the parameter wit the new mac address*/
      memcpy(param_addr, new_mac.octet, sizeof(new_mac.octet));
      /* Update the LwIP stack state*/
      ret = netif_init(netif);
    }
  } else {
    /* Parsing error*/
    ret = -1;
  }

  return ret;
}

static int get_mac_addr (char *param_name,
                         void *param_addr,
                         char *output_buf,
                         uint32_t output_buf_len) {
  uint8_t *mac = (uint8_t *)param_addr;

  snprintf(output_buf,
           output_buf_len,
           "%02X:%02X:%02X:%02X:%02X:%02X\r\n",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  return 0;
}

static int get_wifi_state (char *param_name,
                           void *param_addr,
                           char *output_buf,
                           uint32_t output_buf_len) {
  (void)param_name;
  (void)param_addr;

  if ((wifi_context.state & SL_WFX_STARTED) == SL_WFX_STARTED) {
    snprintf(output_buf,
             output_buf_len,
             "WiFi chip started:\r\n\tSTA: %sconnected\r\n\tAP:  %sstarted\r\n\tPS:  %sactive",
			 wifi_context.state & SL_WFX_STA_INTERFACE_CONNECTED ? "" : "not ",
			 wifi_context.state & SL_WFX_AP_INTERFACE_UP ? "" : "not ",
			 wifi_context.state & SL_WFX_POWER_SAVE_ACTIVE ? "" : "in");

    if ((wifi_context.state & SL_WFX_POWER_SAVE_ACTIVE) == SL_WFX_POWER_SAVE_ACTIVE) {
      snprintf(output_buf,
               output_buf_len,
               "%s(%s)\r\n",
               output_buf,
			   wifi_context.state & SL_WFX_SLEEPING ? "sleeping" : "awake");
    } else if ((strlen(output_buf) + 2) <= output_buf_len) {
      strcat(output_buf, "\r\n");
    }
  } else {
    strncpy(output_buf, "WiFi chip not started\r\n", output_buf_len);
  }

  return 0;
}

static int get_app_version (char *param_name,
                            void *param_addr,
                            char *output_buf,
                            uint32_t output_buf_len) {
  (void)param_name;
  (void)param_addr;

  snprintf(output_buf,
           output_buf_len,
           "%u.%u.%u\r\n",
           WFX_UART_CLI_APP_VERSION_MAJOR,
           WFX_UART_CLI_APP_VERSION_MINOR,
           WFX_UART_CLI_APP_VERSION_BUILD);

  return 0;
}

static int get_driver_version (char *param_name,
                               void *param_addr,
                               char *output_buf,
                               uint32_t output_buf_len) {
  (void)param_name;
  (void)param_addr;

  snprintf(output_buf, output_buf_len, "%s\r\n", FMAC_DRIVER_VERSION_STRING);

  return 0;
}

static int get_firmware_version (char *param_name,
                                 void *param_addr,
                                 char *output_buf,
                                 uint32_t output_buf_len) {
  (void)param_name;
  (void)param_addr;

  snprintf(output_buf,
           output_buf_len,
           "%u.%u.%u\r\n",
		   wifi_context.firmware_major,
		   wifi_context.firmware_minor,
		   wifi_context.firmware_build);

  return 0;
}

static int get_target (char *param_name,
                       void *param_addr,
                       char *output_buf,
                       uint32_t output_buf_len) {
  (void)param_name;
  (void)param_addr;

  snprintf(output_buf, output_buf_len, "%s\r\n", target_name);

  return 0;
}

static int get_bus_wifi (char *param_name,
                         void *param_addr,
                         char *output_buf,
                         uint32_t output_buf_len) {
  (void)param_name;
  (void)param_addr;

  snprintf(output_buf, output_buf_len, "%s\r\n", bus_name);

  return 0;
}


int lwip_param_register (void) {
  int ret;

  ret  = sl_wfx_cli_param_register("version_app",
                                   NULL,
                                   0,
                                   SL_WFX_CLI_PARAM_GET_RIGHT,
                                   SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                   "App version",
                                   get_app_version,
                                   NULL);

  ret |= sl_wfx_cli_param_register("version_drv",
                                   NULL,
                                   0,
                                   SL_WFX_CLI_PARAM_GET_RIGHT,
                                   SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                   "WFX Driver version",
                                   get_driver_version,
                                   NULL);

  ret |= sl_wfx_cli_param_register("version_fw",
                                   NULL,
                                   0,
                                   SL_WFX_CLI_PARAM_GET_RIGHT,
                                   SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                   "WFX Firmware version",
                                   get_firmware_version,
                                   NULL);

  ret |= sl_wfx_cli_param_register("target",
                                   NULL,
                                   0,
                                   SL_WFX_CLI_PARAM_GET_RIGHT,
                                   SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                   "Target running the application",
                                   get_target,
                                   NULL);

  ret |= sl_wfx_cli_param_register("wifi_bus",
                                   NULL,
                                   0,
                                   SL_WFX_CLI_PARAM_GET_RIGHT,
                                   SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                   "Bus to use between the WiFi chip and the host",
                                   get_bus_wifi,
                                   NULL);

  ret |= sl_wfx_cli_param_register("wifi_state",
                                   (void *)&wifi_context.state,
                                   sizeof(wifi_context.state),
                                   SL_WFX_CLI_PARAM_GET_RIGHT,
                                   SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                   "WiFi chip state",
                                   get_wifi_state,
                                   NULL);

  ret |= sl_wfx_cli_param_register("wlan.ssid",
                                   (void *)&wlan_ssid[0],
                                   sizeof(wlan_ssid),
                                   SL_WFX_CLI_PARAM_GET_RIGHT | SL_WFX_CLI_PARAM_SET_RIGHT,
                                   SL_WFX_CLI_PARAM_TYPE_STRING,
                                   "WLAN SSID (max length 32 bytes)",
                                   NULL,
                                   NULL);

  ret |= sl_wfx_cli_param_register("wlan.passkey",
                                   (void *)&wlan_passkey[0],
                                   sizeof(wlan_passkey),
                                   SL_WFX_CLI_PARAM_GET_RIGHT | SL_WFX_CLI_PARAM_SET_RIGHT,
                                   SL_WFX_CLI_PARAM_TYPE_STRING,
                                   "WLAN passkey  (max length 64 bytes)",
                                   NULL,
                                   NULL);

  ret |= sl_wfx_cli_param_register("wlan.security",
                                   (void *)&wlan_security,
                                   sizeof(wlan_security),
                                   SL_WFX_CLI_PARAM_GET_RIGHT | SL_WFX_CLI_PARAM_SET_RIGHT,
                                   SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                   "WLAN security mode [OPEN, WEP, WPA1/WPA2, WPA2,WPA3]",
                                   get_security_mode,
                                   set_security_mode);

  ret |= sl_wfx_cli_param_register("wlan.dhcpc_state",
                                   (void *)&use_dhcp_client,
                                   sizeof(use_dhcp_client),
                                   SL_WFX_CLI_PARAM_GET_RIGHT | SL_WFX_CLI_PARAM_SET_RIGHT,
                                   SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                   "WLAN DHCP client state [0 (OFF), 1 (ON)]",
                                   get_dhcp_client_state,
                                   set_dhcp_client_state);

  ret |= sl_wfx_cli_param_register("wlan.netmask",
                                   (void *)&sta_netif,
                                   sizeof(sta_netif.netmask),
                                   SL_WFX_CLI_PARAM_GET_RIGHT | SL_WFX_CLI_PARAM_SET_RIGHT,
                                   SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                   "WLAN network mask (IPv4 format)",
                                   get_netif_netmask,
                                   set_netif_netmask);

  ret |= sl_wfx_cli_param_register("wlan.gateway",
                                   (void *)&sta_netif,
                                   sizeof(sta_netif.gw),
                                   SL_WFX_CLI_PARAM_GET_RIGHT | SL_WFX_CLI_PARAM_SET_RIGHT,
                                   SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                   "WLAN gateway IP address (IPv4 format)",
                                   get_netif_gateway,
                                   set_netif_gateway);

  ret |= sl_wfx_cli_param_register("wlan.ip",
                                   (void *)&sta_netif,
                                   sizeof(sta_netif.ip_addr),
                                   SL_WFX_CLI_PARAM_GET_RIGHT | SL_WFX_CLI_PARAM_SET_RIGHT,
                                   SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                   "WLAN IP address  (IPv4 format)",
                                   get_netif_ipaddr,
                                   set_netif_ipaddr);

  ret |= sl_wfx_cli_param_register("wlan.pmk",
                                   (void *)&wlan_pmk,
                                   sizeof(wlan_pmk),
                                   SL_WFX_CLI_PARAM_GET_RIGHT,
                                   SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                   "WLAN pairwise master key",
                                   get_station_pmk,
                                   NULL);

  ret |= sl_wfx_cli_param_register("wlan.mac",
                                   (void *)&wifi_context.mac_addr_0.octet,
                                   sizeof(wifi_context.mac_addr_0.octet),
                                   SL_WFX_CLI_PARAM_GET_RIGHT | SL_WFX_CLI_PARAM_SET_RIGHT,
                                   SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                   "WLAN mac address (EUI-48 format)",
                                   get_mac_addr,
                                   set_mac_addr);

  ret |= sl_wfx_cli_param_register("softap.ssid",
                                   (void *)&softap_ssid[0],
                                   sizeof(softap_ssid),
                                   SL_WFX_CLI_PARAM_GET_RIGHT | SL_WFX_CLI_PARAM_SET_RIGHT,
                                   SL_WFX_CLI_PARAM_TYPE_STRING,
                                   "SoftAP SSID  (max length 32 bytes)",
                                   NULL,
                                   NULL);

  ret |= sl_wfx_cli_param_register("softap.passkey",
                                   (void *)&softap_passkey[0],
                                   sizeof(softap_passkey),
                                   SL_WFX_CLI_PARAM_GET_RIGHT | SL_WFX_CLI_PARAM_SET_RIGHT,
                                   SL_WFX_CLI_PARAM_TYPE_STRING,
                                   "SoftAP passkey  (max length 64 bytes)",
                                   NULL,
                                   NULL);

  ret |= sl_wfx_cli_param_register("softap.security",
                                   (void *)&softap_security,
                                   sizeof(softap_security),
                                   SL_WFX_CLI_PARAM_GET_RIGHT | SL_WFX_CLI_PARAM_SET_RIGHT,
                                   SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                   "SoftAP security mode [OPEN, WEP, WPA1/WPA2, WPA2,WPA3]",
                                   get_security_mode,
                                   set_security_mode);

  ret |= sl_wfx_cli_param_register("softap.channel",
                                   (void *)&softap_channel,
                                   sizeof(softap_channel),
                                   SL_WFX_CLI_PARAM_GET_RIGHT | SL_WFX_CLI_PARAM_SET_RIGHT,
                                   SL_WFX_CLI_PARAM_TYPE_UNSIGNED_INTEGER,
                                   "SoftAP channel (decimal)",
                                   NULL,
                                   NULL);

  ret |= sl_wfx_cli_param_register("softap.netmask",
                                   (void *)&ap_netif,
                                   sizeof(ap_netif.netmask),
                                   SL_WFX_CLI_PARAM_GET_RIGHT | SL_WFX_CLI_PARAM_SET_RIGHT,
                                   SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                   "SoftAP network mask (IPv4 format)",
                                   get_netif_netmask,
                                   set_netif_netmask);

  ret |= sl_wfx_cli_param_register("softap.gateway",
                                   (void *)&ap_netif,
                                   sizeof(ap_netif.gw),
                                   SL_WFX_CLI_PARAM_GET_RIGHT | SL_WFX_CLI_PARAM_SET_RIGHT,
                                   SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                   "SoftAP gateway IP address (IPv4 format)",
                                   get_netif_gateway,
                                   set_netif_gateway);

  ret |= sl_wfx_cli_param_register("softap.ip",
                                   (void *)&ap_netif,
                                   sizeof(ap_netif.ip_addr),
                                   SL_WFX_CLI_PARAM_GET_RIGHT | SL_WFX_CLI_PARAM_SET_RIGHT,
                                   SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                   "SoftAP IP address (IPv4 format)",
                                   get_netif_ipaddr,
                                   set_netif_ipaddr);

  ret |= sl_wfx_cli_param_register("softap.pmk",
                                   (void *)&softap_pmk,
                                   sizeof(softap_pmk),
                                   SL_WFX_CLI_PARAM_GET_RIGHT,
                                   SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                   "SoftAP pairwise master key",
                                   get_softap_pmk,
                                   NULL);

  ret |= sl_wfx_cli_param_register("softap.mac",
                                   (void *)&wifi_context.mac_addr_1.octet,
                                   sizeof(wifi_context.mac_addr_1.octet),
                                   SL_WFX_CLI_PARAM_GET_RIGHT | SL_WFX_CLI_PARAM_SET_RIGHT,
                                   SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                   "SoftAP mac address (EUI-48 format)",
                                   get_mac_addr,
                                   set_mac_addr);

  ret |= sl_wfx_cli_param_register("softap.dhcps_state",
                                   (void *)&use_dhcp_server,
                                   sizeof(use_dhcp_server),
                                   SL_WFX_CLI_PARAM_GET_RIGHT | SL_WFX_CLI_PARAM_SET_RIGHT,
                                   SL_WFX_CLI_PARAM_TYPE_CUSTOM,
                                   "SoftAP DHCP server state [0 (OFF), 1 (ON)]",
                                   get_dhcp_server_state,
                                   set_dhcp_server_state);

#ifdef SL_WFX_USE_SECURE_LINK
  ret |= sl_wfx_cli_param_register("slk.mac_key",
                                   (void *)&secure_link_mac_key,
                                   sizeof(secure_link_mac_key),
                                   SL_WFX_CLI_PARAM_SET_RIGHT,
                                   SL_WFX_CLI_PARAM_TYPE_ARRAY_HEXADECIMAL,
                                   "Secure Link MAC key (32 bytes hex array format)",
                                   NULL,
                                   NULL);
#endif

  return ret;
}

