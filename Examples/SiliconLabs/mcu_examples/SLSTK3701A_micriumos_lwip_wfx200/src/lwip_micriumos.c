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

/* FreeRTOS includes. */
#include  <bsp_os.h>
#include  "bsp.h"

#include  <cpu/include/cpu.h>
#include  <kernel/include/os.h>
#include  <kernel/include/os_trace.h>
#include  <common/include/common.h>
#include  <common/include/lib_def.h>
#include  <common/include/rtos_utils.h>
#include  <common/include/toolchains.h>

#include "sl_wfx.h"
#include "string.h"

/* LwIP includes. */
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip_micriumos.h"
#include "lwip/apps/httpd.h"
#include "lwip/netifapi.h"

void dhcpserver_stop(void);
void dhcpserver_start(void);

err_t ethernetif_init(struct netif *netif);
void DHCP_client_task(void *arg);

#define LWIP_TASK_PRIO              22u
#define LWIP_TASK_STK_SIZE         512u

//Task Data Structures
static CPU_STK LWIPTaskStk[LWIP_TASK_STK_SIZE];
static OS_TCB LWIPTaskTCB;

#define DHCP_TASK_PRIO              22u
#define DHCP_TASK_STK_SIZE         512u

//Task Data Structures
static CPU_STK DHCPTaskStk[LWIP_TASK_STK_SIZE];
static OS_TCB DHCPTaskTCB;

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

static void Netif_Config(void);

/***************************************************************************//**
 * Static variables
 ******************************************************************************/
struct netif gnetif; /* network interface structure */
#ifdef LWIP_HTTP_SERVER
// prototype CGI handler for the LED control
static const char * LedToggleCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
static const char * LedGetStateCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
// this structure contains the name of the LED CGI and corresponding handler for the LEDs
static const tCGI LedToggleCGI={"/led_toggle.cgi", LedToggleCGIhandler};
static const tCGI LedGetStateCGI={"/led_get_state.cgi", LedGetStateCGIhandler};
//table of the CGI names and handlers
static tCGI theCGItable[2];
// Server-Side Include (SSI) tags
static char const * const ssi_tags[] = {
    "led0_state",
    "led1_state"
};
static int ssi_handler(int iIndex, char *pcInsert, int iInsertLen);
#endif



#ifdef LWIP_HTTP_SERVER
/***************************************************************************//**
 * @brief Initialize the web server CGI handlers
 ******************************************************************************/
static void myCGIinit(void)
{
    //add LED control CGI to the table
    theCGItable[0] = LedToggleCGI;
    theCGItable[1] = LedGetStateCGI;
    //give the table to the HTTP server
    http_set_cgi_handlers(theCGItable, 2);

    http_set_ssi_handler(&ssi_handler, ssi_tags, 2);

}

/***************************************************************************//**
 * @brief Web server CGI handler for controlling the LEDs. The function pointer for a CGI
 * script handler is defined in httpd.h as tCGIHandler.
 ******************************************************************************/
static const char * LedToggleCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
  if (iIndex == 0) {
    // Check the cgi parameters, e.g., GET /led_toggle.cgi?led_id=1
    for (uint32_t i=0; i<iNumParams; i++) {
      if (strcmp(pcParam[i], "led_id") == 0) {
        if (strcmp(pcValue[i], "0") == 0) {
        	BSP_LedToggle(0);
  	    } else if (strcmp(pcValue[i], "1") == 0) {
  	    	BSP_LedToggle(1);
        }
      }
    }
  }
  //uniform resource identifier to send after CGI call, i.e., path and filename of the response
  return "/led_get_state.json";
} //LedCGIhandler

static int ssi_handler(int iIndex, char *pcInsert, int iInsertLen) {
  int value;

  switch (iIndex) {
    case 0:
      value = BSP_LedGet(0);
      break;
    case 1:
      value = BSP_LedGet(1);
      break;
  }

  return snprintf(pcInsert, 2, "%d", value);
}


/***************************************************************************//**
 * @brief Web server CGI handler for monitoring the state of the LEDs.
 * The function pointer for a CGI script handler is defined in httpd.h as tCGIHandler.
 ******************************************************************************/
static const char * LedGetStateCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
  //uniform resource identifier to send after CGI call, i.e., path and filename of the response
  return "/led_get_state.json";
}


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
  printf("Interval %d.%ds\r\n",(int)(ms_duration/1000),(int)(ms_duration%1000));
  printf("Bytes transferred %d.%dM\r\n",(int)(bytes_transferred/1024/1024),(int)((((bytes_transferred/1024)*1000)/1024)%1000));
  printf("%d.%d Mbps\r\n\r\n",(int)(bandwidth_kbitpsec/1024),(int)(((bandwidth_kbitpsec*1000)/1024)%1000));
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
static void LWIPTask(void *p_arg)
{
  RTOS_ERR err;
  //getUserInput ();
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
	OSTaskCreate(&DHCPTaskTCB,
	                 "DHCP Task",
					  DHCP_client_task,
	                  &gnetif,
					  DHCP_TASK_PRIO,
	                 &DHCPTaskStk[0],
	                 (DHCP_TASK_STK_SIZE / 10u),
					  DHCP_TASK_STK_SIZE,
	                  0u,
	                  0u,
	                  DEF_NULL,
	                 (OS_OPT_TASK_STK_CLR),
	                 &err);
	                                                                /*   Check error code.                                  */
	    APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
}
if (soft_ap_mode)
  {
    dhcpserver_start();
  }

  for( ;; )
  {
    /* Delete the Init Thread */
	  OSTaskDel (NULL, &err);
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
    link_state_notification(1);
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
    link_state_notification(0);
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
    RTOS_ERR err;

    OSTaskCreate(&LWIPTaskTCB,
                 "LWIP Task",
				  LWIPTask,
                  DEF_NULL,
				  LWIP_TASK_PRIO,
                 &LWIPTaskStk[0],
                 (LWIP_TASK_STK_SIZE / 10u),
				  LWIP_TASK_STK_SIZE,
                  0u,
                  0u,
                  DEF_NULL,
                 (OS_OPT_TASK_STK_CLR),
                 &err);
                                                                /*   Check error code.                                  */
    APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
}

