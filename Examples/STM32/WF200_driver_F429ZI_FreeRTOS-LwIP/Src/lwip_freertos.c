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
#include "SEGGER_SYSVIEW_FreeRTOS.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "stm32f4xx_hal.h"

#include "cmsis_os.h"
#include "wf200.h"
#include "string.h" 
   
/* LwIP includes. */
#include "ethernetif.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "app_ethernet.h"
#include "lwip_freertos.h"
#include "lwip/apps/httpd.h"

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
    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
    // Check the cgi parameters, e.g., GET /leds.cgi?led=1&led=2
    for (i=0; i<iNumParams; i++)
    {
        //if pcParmeter contains "led", then one of the LED check boxes has been set on
        if (strcmp(pcParam[i], "led") == 0)
        {
           //see if checkbox for LED 1 has been set
           if(strcmp(pcValue[i], "1") == 0)
           {
               // switch led 1 ON if 1
               HAL_GPIO_WritePin(LD3_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
           }
          //see if checkbox for LED 2 has been set
          else if(strcmp(pcValue[i], "2") == 0)
          {
              // switch led 2 ON if 2
              HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
          }
       }  //if
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
  //printf("\r\nIperf Server Report:\r\n" );
  //printf("Duration %dms\r\n",ms_duration);
 // printf("Bytes transferred %d\r\n",bytes_transferred);
 // printf("%d kbit/s\r\n\r\n",bandwidth_kbitpsec);
}

#endif

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
  
  /* Notify user about the network interface config */
  User_notification(&gnetif);
  
#ifdef USE_DHCP
  /* Start DHCP Client */
  osThreadDef(DHCP, DHCP_thread, osPriorityBelowNormal, 0, configMINIMAL_STACK_SIZE * 2);
  osThreadCreate (osThread(DHCP), &gnetif);
#endif

  for( ;; )
  {
    /* Delete the Init Thread */ 
    osThreadTerminate(NULL);
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
	
#ifdef USE_DHCP
  ip_addr_set_zero_ip4(&ipaddr);
  ip_addr_set_zero_ip4(&netmask);
  ip_addr_set_zero_ip4(&gw);
#else
  IP_ADDR4(&ipaddr,IP_ADDR0,IP_ADDR1,IP_ADDR2,IP_ADDR3);
  IP_ADDR4(&netmask,NETMASK_ADDR0,NETMASK_ADDR1,NETMASK_ADDR2,NETMASK_ADDR3);
  IP_ADDR4(&gw,GW_ADDR0,GW_ADDR1,GW_ADDR2,GW_ADDR3);
#endif /* USE_DHCP */
    
  netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);
  
  /*  Registers the default network interface. */
  netif_set_default(&gnetif);
  
  if (netif_is_link_up(&gnetif))
  {
    /* When the netif is fully configured this function must be called.*/
    netif_set_up(&gnetif);
  }
  else
  {
    /* When the netif link is down this function must be called */
    netif_set_down(&gnetif);
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

