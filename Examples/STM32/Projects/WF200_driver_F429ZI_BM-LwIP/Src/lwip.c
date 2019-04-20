/**
 ******************************************************************************
  * File Name          : LWIP.c
  * Description        : This file provides initialization code for LWIP
  *                      middleWare.
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2018 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
  
/* Includes ------------------------------------------------------------------*/
#include "lwip.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/apps/httpd.h"
#include "lwip/apps/lwiperf.h"

/* USER CODE BEGIN 0 */
#include "demo_config.h"
#include "sl_wfx_host_pin.h"
#include "string.h"
/* USER CODE END 0 */
/* Private function prototypes -----------------------------------------------*/
/* ETH Variables initialization ----------------------------------------------*/
void _Error_Handler(char * file, int line);

/* DHCP Variables initialization ---------------------------------------------*/
uint32_t DHCPfineTimer = 0;
uint32_t DHCPcoarseTimer = 0;
/* USER CODE BEGIN 1 */
#ifdef LWIP_HTTP_SERVER
void myCGIinit(void);
// prototype CGI handler for the LED control
static const char * LedCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
// this structure contains the name of the LED CGI and corresponding handler for the LEDs
static const tCGI LedCGI={"/leds.cgi", LedCGIhandler};
//table of the CGI names and handlers
static tCGI theCGItable[1];
#endif

#ifdef LWIP_IPERF_SERVER
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
/* USER CODE END 1 */

/* Variables Initialization */
struct netif gnetif;
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

/* USER CODE BEGIN 2 */
void MX_LWIP_Connect (void)
{
  netif_set_link_up(&gnetif);
  /* Start DHCP negotiation for a network interface (IPv4) */
}

void MX_LWIP_Disconnect (void)
{
  netif_set_link_down(&gnetif);
  /* Start DHCP negotiation for a network interface (IPv4) */
  dhcp_stop(&gnetif);
}

/* USER CODE END 2 */

/**
  * LwIP initialization function
  */
void MX_LWIP_Init(void)
{
  ip_addr_t ipaddr;
  ip_addr_t netmask;
  ip_addr_t gw;
  /* Initilialize the LwIP stack without RTOS */
  lwip_init();

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

  /* add the network interface (IPv4/IPv6) without RTOS */
  netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &ethernet_input);

  /* Registers the default network interface */
  netif_set_default(&gnetif);

  if (netif_is_link_up(&gnetif))
  {
    /* When the netif is fully configured this function must be called */
    netif_set_up(&gnetif);
  }
  else
  {
    /* When the netif link is down this function must be called */
    netif_set_down(&gnetif);
  }
  dhcp_start(&gnetif);
/* USER CODE BEGIN 3 */
#ifdef LWIP_HTTP_SERVER
  //start web server
  httpd_init();
  //initialise the CGI handlers
  myCGIinit();
#endif
#ifdef LWIP_IPERF_SERVER
  lwiperf_start_tcp_server_default(lwip_iperf_results,0);
#endif
/* USER CODE END 3 */
}

#ifdef USE_OBSOLETE_USER_CODE_SECTION_4
/* Kept to help code migration. (See new 4_1, 4_2... sections) */
/* Avoid to use this user section which will become obsolete. */
/* USER CODE BEGIN 4 */
/* USER CODE END 4 */
#endif

/**
 * ----------------------------------------------------------------------
 * Function given to help user to continue LwIP Initialization
 * Up to user to complete or change this function ...
 * Up to user to call this function in main.c in while (1) of main(void) 
 *-----------------------------------------------------------------------
 * Read a received packet from the Ethernet buffers 
 * Send it to the lwIP stack for handling
 * Handle timeouts if LWIP_TIMERS is set and without RTOS
 * Handle the llink status if LWIP_NETIF_LINK_CALLBACK is set and without RTOS 
 */
void MX_LWIP_Process(void)
{
/* USER CODE BEGIN 4_1 */
  static uint8_t printAddr = 0;
/* USER CODE END 4_1 */
  //ethernetif_input(&gnetif);
  
/* USER CODE BEGIN 4_2 */
/* USER CODE END 4_2 */  
  /* Handle timeouts */
  sys_check_timeouts();

/* USER CODE BEGIN 4_3 */
  if ((gnetif.ip_addr.addr) && (!printAddr))
    {
      /* Print IP address if valid */
      printAddr = 1;
      /* Print MAC address */
      printf("MAC address : %02X:%02X:%02X:%02X:%02X:%02X\r\n",
          gnetif.hwaddr[0], gnetif.hwaddr[1],
          gnetif.hwaddr[2], gnetif.hwaddr[3],
          gnetif.hwaddr[4], gnetif.hwaddr[5]);
      printf("IP address : %3d.%3d.%3d.%3d\r\n",
              gnetif.ip_addr.addr & 0xff,
              (gnetif.ip_addr.addr >> 8) & 0xff,
              (gnetif.ip_addr.addr >> 16) & 0xff,
              (gnetif.ip_addr.addr >> 24) & 0xff);
    }
/* USER CODE END 4_3 */
}

void MX_LWIP_Frame_Received (sl_wfx_received_ind_t* rx_buffer)
{
  ethernetif_input(&gnetif,rx_buffer);
}

#ifdef LWIP_HTTP_SERVER
/***************************************************************************//**
 * @brief Initialize the web server CGI handlers
 ******************************************************************************/
void myCGIinit(void)
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
  if (iIndex == 0)
  {
    // Check the cgi parameters, e.g., GET /leds.cgi?btn_led0=off&btn_led1=on
    for (uint32_t i=0; i<iNumParams; i++)
    {
      //if pcParmeter contains "btn_led", then one of the LED check boxes has been set on
      if (strcmp(pcParam[i], "btn_led1") == 0)
      {
        //see if checkbox for LED has been set
        if(strcmp(pcValue[i], "on") == 0)
        {
          printf("LED 1 on \r\n");
          HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
          HAL_GPIO_WritePin(SL_WFX_LED1_PORT, SL_WFX_LED1_GPIO, GPIO_PIN_SET);
        }else{
          printf("LED 1 off \r\n");
          HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
          HAL_GPIO_WritePin(SL_WFX_LED1_PORT, SL_WFX_LED1_GPIO, GPIO_PIN_RESET);
        }
      }
      if (strcmp(pcParam[i], "btn_led2") == 0)
      {
        //see if checkbox for LED has been set
        if(strcmp(pcValue[i], "on") == 0)
        {
          printf("LED 2 on \r\n");
          HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
          HAL_GPIO_WritePin(SL_WFX_LED2_PORT, SL_WFX_LED2_GPIO, GPIO_PIN_SET);
        }else{
          printf("LED 2 off \r\n");
          HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
          HAL_GPIO_WritePin(SL_WFX_LED2_PORT, SL_WFX_LED2_GPIO, GPIO_PIN_RESET);
        }
      }
    } //for
  } //if
  //uniform resource identifier to send after CGI call, i.e., path and filename of the response
  return "/index.shtml";
} //LedCGIhandler
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
