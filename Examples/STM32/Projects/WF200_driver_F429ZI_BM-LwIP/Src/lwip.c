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
  
/* LwIP includes. */
#include "lwip.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/apps/httpd.h"

#include "sl_wfx.h"
#include "sl_wfx_host_pin.h"
#include "demo_config.h"
#include "dhcp_server.h"

extern sl_wfx_context_t wifi;
extern sl_wfx_scan_result_ind_body_t scan_list[];
extern uint8_t scan_count_web; 

/* station and softAP network interface structures */
struct netif sta_netif, ap_netif;
char ssid_json_list[4096];
char ssid[32], passkey[64];
sl_wfx_security_mode_t secu;

int use_dhcp_client = USE_DHCP_CLIENT_DEFAULT;

sl_wfx_context_t wifi;
char wlan_ssid[32]                        = WLAN_SSID_DEFAULT;
char wlan_passkey[64]                     = WLAN_PASSKEY_DEFAULT;
sl_wfx_security_mode_t wlan_security      = WLAN_SECURITY_DEFAULT;
char softap_ssid[32]                      = SOFTAP_SSID_DEFAULT;
char softap_passkey[64]                   = SOFTAP_PASSKEY_DEFAULT;
sl_wfx_security_mode_t softap_security    = SOFTAP_SECURITY_DEFAULT;
uint8_t softap_channel                    = SOFTAP_CHANNEL_DEFAULT;

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

uint32_t DHCPfineTimer = 0;
uint32_t DHCPcoarseTimer = 0;

#ifdef LWIP_HTTP_SERVER
static char from_hex(char ch);
static sl_status_t url_decode(char *str);
// prototype CGI handler
static const char *LedToggleCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
static const char *GetUpdatedStatesCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
static const char *StartScanCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
static const char *ConnectNetworkCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
static const char *DisconnectNetworkCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
// this structure contains the name of the LED CGI and corresponding handler for the LEDs
static const tCGI LedToggleCGI={"/led_toggle.cgi", LedToggleCGIhandler};
static const tCGI GetUpdatedStatesCGI={"/get_updated_states.cgi", GetUpdatedStatesCGIhandler};
// this structure contains the name of the scan CGI and corresponding handler for the scan
static const tCGI StartScanCGI={"/start_scan.cgi", StartScanCGIhandler};
static const tCGI ConnectNetworkCGI={"/connect_network.cgi", ConnectNetworkCGIhandler};
static const tCGI DisconnectNetworkCGI={"/disconnect_network.cgi", DisconnectNetworkCGIhandler};
//table of the CGI names and handlers
static tCGI theCGItable[5];
// Server-Side Include (SSI) tags
static char const *ssi_tags[] = {
    "led0_state",
    "led1_state",
    "t_scan",
    "status",
    "ip_address"
};
static uint16_t ssi_handler(int iIndex, char *pcInsert, int iInsertLen);

/***************************************************************************//**
 * @brief Initialize the web server CGI handlers
 ******************************************************************************/
static void myCGIinit(void)
{
    //add LED control CGI to the table
    theCGItable[0] = LedToggleCGI;
    theCGItable[1] = GetUpdatedStatesCGI;
    
    //add scan control CGI to the table
    theCGItable[2] = StartScanCGI;
    
    theCGItable[3] = ConnectNetworkCGI;
    theCGItable[4] = DisconnectNetworkCGI;
    
    //give the table to the HTTP server
    http_set_cgi_handlers(theCGItable, 5);
    http_set_ssi_handler(ssi_handler, ssi_tags, 5);
}

/***************************************************************************//**
 * @brief Web server CGI handler for controlling the LEDs. The function pointer for a CGI 
 * script handler is defined in httpd.h as tCGIHandler.
 ******************************************************************************/
static const char *LedToggleCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
  // Check the cgi parameters, e.g., GET /led_toggle.cgi?led_id=1
  for (uint32_t i=0; i<iNumParams; i++) {
    if (strcmp(pcParam[i], "led_id") == 0) 
    {
      if (strcmp(pcValue[i], "0") == 0) 
      {
        printf("LED 0 toggle\r\n");
        HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
        HAL_GPIO_TogglePin(SL_WFX_LED0_PORT, SL_WFX_LED0_GPIO);
      }else if (strcmp(pcValue[i], "1") == 0)
      {
        printf("LED 1 toggle\r\n");
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        HAL_GPIO_TogglePin(SL_WFX_LED1_PORT, SL_WFX_LED1_GPIO);
      }
    }
  }
  
  //uniform resource identifier to send after CGI call, i.e., path and filename of the response
  return "/empty.json";
} //LedCGIhandler

/***************************************************************************//**
 * @brief Web server CGI handler for monitoring the state of example.
 * The function pointer for a CGI script handler is defined in httpd.h as tCGIHandler.
 ******************************************************************************/
static const char *GetUpdatedStatesCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
  //uniform resource identifier to send after CGI call, i.e., path and filename of the response
  return "/updated_states.json";
}

/***************************************************************************//**
* @brief Web server CGI handler for starting a scan. The function pointer for a CGI 
* script handler is defined in httpd.h as tCGIHandler.
******************************************************************************/
static const char *StartScanCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
  sl_status_t result;
  
  result = sl_wfx_send_scan_command(WFM_SCAN_MODE_ACTIVE, NULL,0, NULL,0,NULL,0);
  if ((result == SL_SUCCESS) || (result == SL_WIFI_WARNING))
  {
    sl_wfx_host_setup_waited_event(SL_WFX_SCAN_COMPLETE_IND_ID);
    sl_wfx_host_wait_for_confirmation(SL_WFX_SCAN_COMPLETE_IND_ID, SL_WFX_DEFAULT_REQUEST_TIMEOUT_MS, NULL);
  }

  return "/scan_results.json";
} //ScanCGIhandler

/***************************************************************************//**
 * @brief Web server CGI handler to connect to a Wi-Fi network.
 * The function pointer for a CGI script handler is defined in httpd.h as tCGIHandler.
 ******************************************************************************/
static const char *ConnectNetworkCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
  sl_status_t status;
  int ssid_length, passkey_length;
  
  if (iNumParams == 3) {
    if (strcmp(pcParam[0], "ssid") == 0) 
    {
      url_decode(pcValue[0]);
      printf("%s\n\r", pcValue[0]);
      ssid_length = strlen(pcValue[0]);
      memset(ssid, 0, 32);
      strncpy(ssid, pcValue[0], ssid_length);
    }
    if (strcmp(pcParam[1], "pwd") == 0) 
    {
      url_decode(pcValue[1]);
      printf("%s\n\r", pcValue[1]);
      passkey_length = strlen(pcValue[1]);
      memset(passkey, 0, 64);
      strncpy(passkey, pcValue[1], passkey_length);
    }
    if (strcmp(pcParam[2], "secu") == 0)
    {
      url_decode(pcValue[2]);
      printf("%s\n\r", pcValue[2]);
      if ((strcmp(pcValue[2], "WPA2") == 0) || (strcmp(pcValue[2], "WPA") == 0))
      {
        secu = WFM_SECURITY_MODE_WPA2_WPA1_PSK;
      }else if (strcmp(pcValue[2], "WEP") == 0)
      {
        secu = WFM_SECURITY_MODE_WEP;
      }else if (strcmp(pcValue[2], "OPEN") == 0)
      {
        secu = WFM_SECURITY_MODE_OPEN;
      }
    }
    if (!(sl_wfx_context->state & SL_WFX_STA_INTERFACE_CONNECTED))
    {
      status = sl_wfx_send_join_command((uint8_t*) ssid, ssid_length,
                                        NULL, 0, secu, 0, 0,
                                        (uint8_t*) passkey, passkey_length,
                                        NULL, 0);
      if(status != SL_SUCCESS)
      {
        printf("Command error\r\n");
      }
    }
  }else{
    printf("Invalid Connection Request\r\n");
  }
  
  //uniform resource identifier to send after CGI call, i.e., path and filename of the response
  return "/empty.json";
}

/***************************************************************************//**
* @brief Web server CGI handler for starting a scan. The function pointer for a CGI 
* script handler is defined in httpd.h as tCGIHandler.
******************************************************************************/
static const char *DisconnectNetworkCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
  sl_wfx_send_disconnect_command();
  
  //uniform resource identifier to send after CGI call, i.e., path and filename of the response
  return "/empty.json";
}

static uint16_t ssi_handler(int iIndex, char *pcInsert, int iInsertLen)
{
  int value, result;
  char ssid_json_lign[100];
    
  switch (iIndex) {
  case 0: /* <!--#led0_state--> */
    value = HAL_GPIO_ReadPin(SL_WFX_LED0_PORT, SL_WFX_LED0_GPIO);
    result = snprintf(pcInsert, 2, "%d", value);
    break;
  case 1: /* <!--#led1_state--> */
    value = HAL_GPIO_ReadPin(SL_WFX_LED1_PORT, SL_WFX_LED1_GPIO);
    result = snprintf(pcInsert, 2, "%d", value);
    break;
  case 2: /* <!--#t_scan--> */
    for(int i = 0; i < scan_count_web; i++)
    {
      snprintf(ssid_json_lign, 100, "{\"ssid\":\"%s\", \"rssi\":\"%d\", \"secu\":\"%s\"}",
               (strlen((char *) scan_list[i].ssid_def.ssid) != 0) ? scan_list[i].ssid_def.ssid : "Hidden",
               ((int16_t)(scan_list[i].rcpi - 220)/2),
               (scan_list[i].security_mode.wpa2) ? "WPA2" : \
                 ((scan_list[i].security_mode.wpa) ? "WPA" : \
                   ((scan_list[i].security_mode.wep) ? "WEP": "OPEN")));
      strcat(ssid_json_list, ssid_json_lign);
      if(i != (scan_count_web - 1)) strcat(ssid_json_list, ", ");
    }
    result = snprintf(pcInsert, strlen(ssid_json_list) + 1, ssid_json_list);
    scan_count_web = 0;
    memset(scan_list, 0, sizeof(sl_wfx_scan_result_ind_body_t) * 50);
    memset(ssid_json_list, 0, sizeof(ssid_json_list));
    break;
  case 3: /* <!--#status--> */
    if(wifi.state & SL_WFX_STA_INTERFACE_CONNECTED)
    {
      result = snprintf(pcInsert, 50, "Connected to %s", ssid);
    }else{
      result = snprintf(pcInsert, 50, "Not Connected");
    }
    break;
  case 4: /* <!--#ip_address--> */
    result = snprintf(pcInsert, 25, "%d.%d.%d.%d",
                      sta_netif.ip_addr.addr & 0xff,
                      (sta_netif.ip_addr.addr >> 8) & 0xff,
                      (sta_netif.ip_addr.addr >> 16) & 0xff,
                      (sta_netif.ip_addr.addr >> 24) & 0xff);
    break;
  }
  
  return result;
}

/* Converts a hex character to its integer value */
static char from_hex(char ch) {
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/* Returns a url-decoded version of str */
static sl_status_t url_decode(char *str) {
  char *pstr = str, rstr[64];
  int i = 0;
  
  if(strlen(str) > 64) return SL_ERROR;
  
  while (*pstr) {
    if (*pstr == '%') {
      if (pstr[1] && pstr[2]) {
        rstr[i++] = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
        pstr += 2;
      }
    }else if (*pstr == '+') { 
      rstr[i++]  = ' ';
    }else{
      rstr[i++] = *pstr;
    }
    pstr++;
  }
  rstr[i] = '\0';
  strcpy(str, &rstr[0]);
  return SL_SUCCESS;
}

#endif //LWIP_HTTP_SERVER

#ifdef LWIP_IPERF_SERVER
#include "lwip/ip_addr.h"
#include "lwip/apps/lwiperf.h"
/***************************************************************************//**
 * @brief Function to handle iperf results report
 ******************************************************************************/
void lwip_iperf_results (void *arg, enum lwiperf_report_type report_type,
                         const ip_addr_t* local_addr, u16_t local_port, const ip_addr_t* remote_addr, u16_t remote_port,
                         u32_t bytes_transferred, u32_t ms_duration, u32_t bandwidth_kbitpsec)
{
  printf("\r\nIperf Report:\r\n");
  printf("Interval %d.%ds\r\n",(int)(ms_duration/1000),(int)(ms_duration%1000));
  printf("Bytes transferred %d.%dM\r\n",(int)(bytes_transferred/1024/1024),(int)((((bytes_transferred/1024)*1000)/1024)%1000));
  printf("%d.%d Mbps\r\n\r\n",(int)(bandwidth_kbitpsec/1024),(int)(((bandwidth_kbitpsec*1000)/1024)%1000));
}

#endif //LWIP_IPERF_SERVER

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
  netif_set_up(&sta_netif);
  netif_set_link_up(&sta_netif);
  if (use_dhcp_client)
  {
    dhcp_start(&sta_netif);
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
  if (use_dhcp_client)
  {
    dhcp_stop(&sta_netif);
  }
  netif_set_link_down(&sta_netif);
  netif_set_down(&sta_netif);
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
  netif_set_up(&ap_netif);
  netif_set_link_up(&ap_netif);
  dhcpserver_start();
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
  dhcpserver_stop();
  netif_set_link_down(&ap_netif);
  netif_set_down(&ap_netif);
}

/***************************************************************************//**
 * @brief
 *    Initializes LwIP network interface
 *
 * @param[in] 
 *    none
 *
 * @return
 *    none
 ******************************************************************************/
void sl_lwip_init(void)
{
  sl_status_t status;
  ip_addr_t sta_ipaddr, ap_ipaddr;
  ip_addr_t sta_netmask, ap_netmask;
  ip_addr_t sta_gw, ap_gw;
  
  /* Initilialize the LwIP stack without RTOS */
  lwip_init();
  
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
  printf("FMAC Driver version    %s\r\n", FMAC_DRIVER_VERSION_STRING);
  switch(status) {
  case SL_SUCCESS:
    wifi.state = SL_WFX_STARTED;
    printf("WF200 Firmware version %d.%d.%d\r\n", 
           wifi.firmware_major,
           wifi.firmware_minor,
           wifi.firmware_build);
    printf("WF200 initialization successful\r\n");
    break;
  case SL_WIFI_INVALID_KEY:
    printf("Failed to init WF200: Firmware keyset invalid\r\n");
    break;
  case SL_WIFI_FIRMWARE_DOWNLOAD_TIMEOUT:
    printf("Failed to init WF200: Firmware download timeout\r\n");
    break;
  case SL_TIMEOUT:
    printf("Failed to init WF200: Poll for value timeout\r\n");
    break;
  case SL_ERROR:
    printf("Failed to init WF200: Error\r\n");
    break;
  default :
    printf("Failed to init WF200: Unknown error\r\n");
  }
  
  /* Add station and softAP interfaces */
  netif_add(&sta_netif, &sta_ipaddr, &sta_netmask, &sta_gw, NULL, &sta_ethernetif_init, &ethernet_input);
  netif_add(&ap_netif, &ap_ipaddr, &ap_netmask, &ap_gw, NULL, &ap_ethernetif_init, &ethernet_input);
  
  /* Registers the default network interface */
  netif_set_default(&sta_netif);  
  
#ifdef LWIP_HTTP_SERVER
  //start web server
  httpd_init();
  //initialise the CGI handlers
  myCGIinit();
#endif
#ifdef LWIP_IPERF_SERVER
  lwiperf_start_tcp_server_default(lwip_iperf_results,0);
#endif
  
  sl_wfx_start_ap_command(softap_channel, (uint8_t*) softap_ssid, 
                          strlen(softap_ssid), 0, 0, softap_security, 0, 
                          (uint8_t*) softap_passkey, strlen(softap_passkey), 
                          NULL, 0, NULL, 0);
  printf ("IP address = %d.%d.%d.%d\r\n",
          ap_ip_addr0,
          ap_ip_addr1,
          ap_ip_addr2,
          ap_ip_addr3);
}

/***************************************************************************//**
 * @brief
 *    LwIP main loop
 *
 * @param[in] 
 *    none
 *
 * @return
 *    none
 ******************************************************************************/
void sl_lwip_process(void)
{
  static uint8_t printAddr = 0;
  
  sys_check_timeouts();
  
  if ((sta_netif.ip_addr.addr) && (!printAddr))
  {
    /* Print IP address if valid */
    printAddr = 1;
    /* Print MAC address */
    printf("MAC address : %02X:%02X:%02X:%02X:%02X:%02X\r\n",
           sta_netif.hwaddr[0], sta_netif.hwaddr[1],
           sta_netif.hwaddr[2], sta_netif.hwaddr[3],
           sta_netif.hwaddr[4], sta_netif.hwaddr[5]);
    printf("IP address : %3d.%3d.%3d.%3d\r\n",
           sta_netif.ip_addr.addr & 0xff,
           (sta_netif.ip_addr.addr >> 8) & 0xff,
           (sta_netif.ip_addr.addr >> 16) & 0xff,
           (sta_netif.ip_addr.addr >> 24) & 0xff);
  }
}
