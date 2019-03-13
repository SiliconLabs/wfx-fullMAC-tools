/*
* Copyright 2018, Silicon Laboratories Inc.  All rights reserved.
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
*/

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "stm32f4xx_hal.h"

#include "cmsis_os.h"
#include "wf200.h"
#include "wf200_host_pin.h"
#include "string.h" 
   
/* LwIP includes. */
#include "ethernetif.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "app_ethernet.h"
#include "lwip_freertos.h"
#include "lwip/apps/httpd.h"
#include "lwip/netifapi.h"
#include "dhcpserver.h"
#include "uart_input.h"


int soft_ap_mode = SOFT_AP_MODE_DEFAULT;
int use_dhcp_client = USE_DHCP_CLIENT_DEFAULT;
uint8_t ip_addr0 = IP_ADDR0_DEFAULT;
uint8_t ip_addr1 = IP_ADDR1_DEFAULT;
uint8_t ip_addr2 = IP_ADDR2_DEFAULT;
uint8_t ip_addr3 = IP_ADDR3_DEFAULT;

uint8_t netmask_addr0 = NETMASK_ADDR0_DEFAULT;
uint8_t netmask_addr1 = NETMASK_ADDR1_DEFAULT;
uint8_t netmask_addr2 = NETMASK_ADDR2_DEFAULT;
uint8_t netmask_addr3 = NETMASK_ADDR3_DEFAULT;

uint8_t gw_addr0 = GW_ADDR0_DEFAULT;
uint8_t gw_addr1 = GW_ADDR1_DEFAULT;
uint8_t gw_addr2 = GW_ADDR2_DEFAULT;
uint8_t gw_addr3 = GW_ADDR3_DEFAULT;

//wait 5 seconds
#define WAIT_TIME_FOR_INPUT ( 5000UL / portTICK_RATE_MS ) 

static void StartThread(void const * argument);
static void Netif_Config(void);

/***************************************************************************//**
 * Static variables
 ******************************************************************************/
struct netif gnetif; /* network interface structure */
#ifdef LWIP_HTTP_SERVER
// prototype CGI handler for the LED control
static const char * LedCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
// this structure contains the name of the LED CGI and corresponding handler for the LEDs
static const tCGI LedCGI={"/leds.cgi", LedCGIhandler};
//table of the CGI names and handlers
static tCGI theCGItable[1];
#endif

#ifdef LWIP_HTTP_SERVER
/***************************************************************************//**
 * @brief Initialize the web server CGI handlers
 ******************************************************************************/
static void myCGIinit(void)
{
    //add LED control CGI to the table
    theCGItable[0] = LedCGI;
    //give the table to the HTTP server
    http_set_cgi_handlers(theCGItable, 1);
} 

/***************************************************************************//**
 * @brief Web server CGI handler for controlling the LEDs. The function pointer for a CGI 
 * script handler is defined in httpd.h as tCGIHandler.
 ******************************************************************************/
static const char * LedCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
uint32_t i=0;
// index of the CGI within the theCGItable array passed to http_set_cgi_handlers
// Given how this example is structured, this may be a redundant check.
// Here there is only one handler iIndex == 0
if (iIndex == 0)
{
    // turn off the LEDs
    HAL_GPIO_WritePin(LD3_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(WF200_LED1_PORT, WF200_LED1_GPIO, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(WF200_LED2_PORT, WF200_LED2_GPIO, GPIO_PIN_RESET);
    // Check the cgi parameters, e.g., GET /leds.cgi?btn_led0=off&btn_led1=on
    for (i=0; i<iNumParams; i++)
    {
        
       //see if checkbox for LED has been set
       if(strcmp(pcValue[i], "on") == 0)
       {
            //if pcParmeter contains "btn_led", then one of the LED check boxes has been set on
            if (strcmp(pcParam[i], "btn_led0") == 0)
            {
               // switch led 1 ON
               HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
               HAL_GPIO_WritePin(WF200_LED1_PORT, WF200_LED1_GPIO, GPIO_PIN_SET);
            }
            else if (strcmp(pcParam[i], "btn_led1") == 0)
            {
              // switch led 2 ON 
              HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
              HAL_GPIO_WritePin(WF200_LED2_PORT, WF200_LED2_GPIO, GPIO_PIN_SET);
            }
       }
    } //for
} //if
//uniform resource identifier to send after CGI call, i.e., path and filename of the response
return "/index.html";
} //LedCGIhandler
#endif

#ifdef LWIP_IPERF_SERVER
#include "lwip/ip_addr.h"
#include "lwip/apps/lwiperf.h"
/***************************************************************************//**
 * @brief Function to handle iperf results report
 ******************************************************************************/
static void lwip_iperf_results (void *arg, enum lwiperf_report_type report_type,
  const ip_addr_t* local_addr, u16_t local_port, const ip_addr_t* remote_addr, u16_t remote_port,
  u32_t bytes_transferred, u32_t ms_duration, u32_t bandwidth_kbitpsec)
{
  printf("\r\nIperf Server Report:\r\n" );
  printf("Duration %dms\r\n",ms_duration);
  printf("Bytes transferred %d\r\n",bytes_transferred);
  printf("%d kbit/s\r\n\r\n",bandwidth_kbitpsec);
}

#endif

static uint32_t  waitForString (TickType_t waitTime)
{
  //start receive loop
  xSemaphoreGive (uartInputSemaphore);
  //wait for data 
  if (xSemaphoreTake(stringRcvSemaphore,waitTime) == pdFALSE)
  {
    vUARTInputStop();
    return 0;
  }
  return 1;
}


static void getUserInput (void)
{
  printf ("Press enter within 5 seconds to configure the demo...\r\n\r\n");
  if (waitForString(WAIT_TIME_FOR_INPUT) == pdTRUE )
  {
    printf ("Choose mode:\r\n1. Station\r\n2. AP\r\nType 1 or 2:\r\n");
    waitForString(portMAX_DELAY);
    if (UART_Input_String[0] == '2')
    {
       soft_ap_mode = 1;
       use_dhcp_client = 0;
       printf ("\r\n AP mode selected\r\n");
    }
    else
    {
       soft_ap_mode = 0;
       use_dhcp_client = 1;
       printf ("\r\n Station mode selected\r\n");
    }
    printf ("\r\nEnter SSID:\r\n");
    waitForString(portMAX_DELAY);
    if (soft_ap_mode)
    {
      strcpy(softap_ssid,UART_Input_String);
    }
    else
    {
      strcpy(wlan_ssid,UART_Input_String);
    }
    printf ("\r\n\r\nEnter passkey:\r\n");
    waitForString(portMAX_DELAY);
    if (soft_ap_mode)
    {
      strcpy(softap_passkey,UART_Input_String);
    }
    else
    {
      strcpy(wlan_passkey,UART_Input_String);
    }
    printf ("\r\n\r\nChoose security mode:\r\n1. OPEN\r\n2. WEP\r\n3. WPA2 WPA1 PSK\r\n4. WPA2 PSK\r\n");
    waitForString(portMAX_DELAY);
    if (soft_ap_mode)
    {
      switch (UART_Input_String[0]) {
      case '1':
        softap_security = WFM_SECURITY_MODE_OPEN;
        break;
      case '2':
        softap_security = WFM_SECURITY_MODE_WEP;
        break;
      case '3':
        softap_security = WFM_SECURITY_MODE_WPA2_WPA1_PSK;
        break;
      default:
        softap_security = WFM_SECURITY_MODE_WPA2_PSK;
        break;
      }
    }
    else
    {
      switch (UART_Input_String[0]) {
      case '1':
        wlan_security = WFM_SECURITY_MODE_OPEN;
        break;
      case '2':
        wlan_security = WFM_SECURITY_MODE_WEP;
        break;
      case '3':
        wlan_security = WFM_SECURITY_MODE_WPA2_WPA1_PSK;
        break;
      default:
        wlan_security = WFM_SECURITY_MODE_WPA2_PSK;
        break;
      }
    }
  }
  
  printf ("\r\n\r\nStarting demo...\r\n");
  if (soft_ap_mode)
  {
    printf ("AP mode\r\n");
    printf ("SSID = %s\r\n",softap_ssid);
    printf ("Passkey = %s\r\n",softap_passkey);
    printf ("IP address = %d.%d.%d.%d\r\n",
                  ip_addr0,
                  ip_addr1,
                  ip_addr2,
                  ip_addr3);
  }
  else
  {
    printf ("Station mode\r\n");
    printf ("SSID = %s\r\n",wlan_ssid);
    printf ("Passkey = %s\r\n",wlan_passkey);
  }
            
}

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

  getUserInput ();
  /* Create tcp_ip stack thread */
  tcpip_init(NULL, NULL);
  
  /* Initialize the LwIP stack */
  Netif_Config();
  
#ifdef LWIP_HTTP_SERVER  
  /* Initialize webserver demo */
  httpd_init();
  //initialise the CGI handlers
  myCGIinit();
#endif
#ifdef LWIP_IPERF_SERVER
  lwiperf_start_tcp_server_default(lwip_iperf_results,0);
#endif
   
if (use_dhcp_client)
{
  /* Start DHCP Client */
  osThreadDef(DHCP, DHCP_thread, osPriorityBelowNormal, 0, configMINIMAL_STACK_SIZE * 2);
  osThreadCreate (osThread(DHCP), &gnetif);
}
if (soft_ap_mode)
  {
    dhcpserver_start();
  }

  for( ;; )
  {
    /* Delete the Init Thread */ 
    osThreadTerminate(NULL);
  }
}
/***************************************************************************//**
 * @brief
 *    Sets link status to up in LwIP
 *
 * @param[in] 
 *    none
 *
 * @return
 *    none
 ******************************************************************************/
void lwip_set_link_up (void)
{

  netifapi_netif_set_link_up(&gnetif);
  netifapi_netif_set_up(&gnetif);
  if (use_dhcp_client)
  {
    User_notification(1);
  }
  if (soft_ap_mode)
  {
    dhcpserver_start();
  }

}
/***************************************************************************//**
 * @brief
 *    Sets link status to down in LwIP
 *
 * @param[in] 
 *    none
 *
 * @return
 *    none
 ******************************************************************************/
void lwip_set_link_down (void)
{
  if (soft_ap_mode)
  {
    dhcpserver_stop();
  }
  netifapi_netif_set_down(&gnetif);
  netifapi_netif_set_link_down(&gnetif);
  if (use_dhcp_client)
  {
    User_notification(0);
  }
 
  
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
static void Netif_Config(void)
{
  ip_addr_t ipaddr;
  ip_addr_t netmask;
  ip_addr_t gw;
	
if (use_dhcp_client)
{
  ip_addr_set_zero_ip4(&ipaddr);
  ip_addr_set_zero_ip4(&netmask);
  ip_addr_set_zero_ip4(&gw);
}
else
{
  IP_ADDR4(&ipaddr,ip_addr0,ip_addr1,ip_addr2,ip_addr3);
  IP_ADDR4(&netmask,netmask_addr0,netmask_addr1,netmask_addr2,netmask_addr3);
  IP_ADDR4(&gw,gw_addr0,gw_addr1,gw_addr2,gw_addr3);
}
    
  netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);
  
  /*  Registers the default network interface. */
  netif_set_default(&gnetif);  
  if (soft_ap_mode)
  {
    netif_set_link_up(&gnetif);  
    netif_set_up(&gnetif);  
  }
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

