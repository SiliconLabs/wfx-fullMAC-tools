/***************************************************************************//**
 * @file
 * @brief LwIP task and related functions
 *******************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
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

#include <bridge.h>
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
#include "sleep.h"
#include "console.h"
#include "dhcp_client.h"
#include "sl_wfx_host.h"
#include "sl_wfx_task.h"
#include <net/include/net_if_ether.h>
#include <net/source/tcpip/net_if_priv.h>

#ifdef LWIP_IPERF_SERVER
#include "lwip/apps/lwiperf.h"
#endif

extern OS_SEM scan_sem;

#define WIFI_TASK_PRIO              22u
#define WIFI_TASK_STK_SIZE         800u

#define WIFI_APP_ECHO_ENABLED        1u

/* wfx_fmac_driver context*/
sl_wfx_context_t wifi_context;
char wlan_ssid[32+1]                    = WLAN_SSID_DEFAULT;
char wlan_passkey[64+1]                 = WLAN_PASSKEY_DEFAULT;
uint8_t wlan_bssid[SL_WFX_BSSID_SIZE];
sl_wfx_password_t wlan_pmk              = {0};
sl_wfx_security_mode_t wlan_security    = WLAN_SECURITY_DEFAULT;
char softap_ssid[32+1]                  = SOFTAP_SSID_DEFAULT;
char softap_passkey[64+1]               = SOFTAP_PASSKEY_DEFAULT;
sl_wfx_password_t softap_pmk            = {0};
sl_wfx_security_mode_t softap_security  = SOFTAP_SECURITY_DEFAULT;
uint8_t softap_channel                  = SOFTAP_CHANNEL_DEFAULT;

/// Memory to store an event to display in the web page
char event_log[50];
/// LwIP task stack
static CPU_STK wifi_task_stk[WIFI_TASK_STK_SIZE];
/// LwIP task TCB
static OS_TCB wifi_task_tcb;

static void wifi_config(void);
/**************************************************************************//**
 * Prompt the user to provide the Wi-Fi configuration.
 *****************************************************************************/
static void start_softap (void)
{
	RTOS_ERR err;
	// Start the SoftAP with the default configuration
	sl_wfx_start_ap_command(softap_channel,
							(uint8_t*) softap_ssid,
							strlen(softap_ssid),
							0,
							0,
							softap_security,
							0,
							(uint8_t*) softap_passkey,
							strlen(softap_passkey),
							NULL,
							0,
							NULL,
							0);

	/* Delay for 1000ms */
	OSTimeDly(1000, OS_OPT_TIME_DLY, &err);
	if (sl_wfx_set_unicast_filter(0) != SL_STATUS_OK)
	  printf("Set sl_wfx_set_unicast_filter fail\n");
}

/***************************************************************************//**
 * Start Wifi task(s).
 *
 * @param p_arg Unused parameter.
 ******************************************************************************/
static void wifi_task(void *p_arg)
{
  RTOS_ERR err;

  // Initialize Wifi and Start SoftAP
  wifi_config();

  for (;; ) {
    // Delete the Init Thread
    OSTaskDel(NULL, &err);
  }
}

/***************************************************************************//**
 * Initializes WF200 and start SoftAP
 ******************************************************************************/
static void wifi_config(void)
{
  sl_status_t status;

  /* Initialize the WF200 used by the two interfaces */
  status = sl_wfx_init(&wifi_context);
  printf("FMAC Driver version    %s\r\n", FMAC_DRIVER_VERSION_STRING);
  switch (status) {
    case SL_STATUS_OK:
      wifi_context.state = SL_WFX_STARTED;
      printf("WF200 Firmware version %d.%d.%d\r\n",
    		  wifi_context.firmware_major,
			  wifi_context.firmware_minor,
			  wifi_context.firmware_build);
      printf("WF200 initialization successful\r\n");
      break;
    case SL_STATUS_WIFI_INVALID_KEY:
      printf("Failed to init WF200: Firmware keyset invalid\r\n");
      break;
    case SL_STATUS_WIFI_FIRMWARE_DOWNLOAD_TIMEOUT:
      printf("Failed to init WF200: Firmware download timeout\r\n");
      break;
    case SL_STATUS_TIMEOUT:
      printf("Failed to init WF200: Poll for value timeout\r\n");
      break;
    case SL_STATUS_FAIL:
      printf("Failed to init WF200: Error\r\n");
      break;
    default:
      printf("Failed to init WF200: Unknown error\r\n");
  }
#ifdef SLEEP_ENABLED
#ifdef SL_WFX_USE_SDIO
  status = sl_wfx_host_switch_to_wirq();
#endif
#endif

#ifndef DISABLE_CFG_MENU
  start_softap();
#else
  sl_wfx_generic_confirmation_t* reply = NULL;
  sl_wfx_start_ap_command(softap_channel, (uint8_t*) softap_ssid, strlen(softap_ssid), 0, 0, softap_security, 0, (uint8_t*) softap_passkey, strlen(softap_passkey), NULL, 0, NULL, 0);
#endif
}

/**************************************************************************//**
 * Start LwIP task.
 *****************************************************************************/
sl_status_t wifi_start(void)
{
  RTOS_ERR err;

  OSSemCreate(&scan_sem, "wifi_scan_sem", 0, &err);
  /*   Check error code.                                  */
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

  OSTaskCreate(&wifi_task_tcb,
               "WiFi Task",
			   wifi_task,
               DEF_NULL,
               WIFI_TASK_PRIO,
               &wifi_task_stk[0],
               (WIFI_TASK_STK_SIZE / 10u),
               WIFI_TASK_STK_SIZE,
               0u,
               0u,
               DEF_NULL,
               (OS_OPT_TASK_STK_CLR),
               &err);
  /*   Check error code.                                  */
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
  return SL_STATUS_OK;
}

/* From ethernetif.c old file */

static NET_IF* get_net_if_for_ethernet_interface()
{
	NET_IF   *p_if;
	RTOS_ERR          local_err;
	NET_IF_NBR if_nbr = NetIF_NbrGetFromName("eth0");
	p_if = NetIF_Get(if_nbr, &local_err);

	return p_if;
}

/***************************************************************************//**
 * Transmits packet(s)from ethernet interface.
 *
 * @param netif the lwip network interface structure
 * @param p the packet to send
 * @returns ERR_OK if successful
 ******************************************************************************/
int8_t low_level_output_ethernet(char *data, uint32_t size)
{
  uint8_t *buffer;
  sl_wfx_packet_queue_item_t *queue_item;
  sl_status_t result;

	if (sl_wfx_context == NULL  || ((sl_wfx_context->state & SL_WFX_STARTED) != SL_WFX_STARTED) )
	  return 0;

  /* Allocate a buffer for a queue item */
  result = sl_wfx_allocate_command_buffer((sl_wfx_generic_message_t**)(&queue_item),
										  SL_WFX_SEND_FRAME_REQ_ID,
										  SL_WFX_TX_FRAME_BUFFER,
										  size + sizeof(sl_wfx_packet_queue_item_t));

  if ((result != SL_STATUS_OK) || (queue_item == NULL)) {
	return -1;
  }

  buffer = queue_item->buffer.body.packet_data;
  memcpy( buffer, (uint8_t*)data, size);

  /* Provide the data length the interface information to the pbuf */
  queue_item->interface = SL_WFX_SOFTAP_INTERFACE;
  queue_item->data_length = size;

  /* Send the packet */
    result = sl_wfx_send_ethernet_frame(&queue_item->buffer,
										queue_item->data_length,
										queue_item->interface,
                                        WFM_PRIORITY_BE0);

    /* The packet has been sent, release the packet  */
    sl_wfx_free_command_buffer((sl_wfx_generic_message_t*) queue_item,
                               SL_WFX_SEND_FRAME_REQ_ID,
                               SL_WFX_TX_FRAME_BUFFER);

  return 0;
}

/***************************************************************************//**
 * Transfers the receive packets from the wfx to lwip.
 *
 * @param netif lwip network interface structure
 * @param rx_buffer the ethernet frame received by the wf200
 * @returns LwIP pbuf filled with received packet, or NULL on error
 ******************************************************************************/
extern void NetIF_DevTxRdyWait(NET_IF   *p_if,
                               RTOS_ERR *p_err);

/***************************************************************************//**
 * WFX received frame callback.
 *
 * @param rx_buffer the ethernet frame received by the wfx
 ******************************************************************************/
void sl_wfx_host_received_frame_callback(sl_wfx_received_ind_t* rx_buffer)
{
  // Received on SoftAP. Forward to ethernet
  uint16_t len = 0;
  uint8_t *buffer;
  NET_IF *p_if = DEF_NULL;
  NET_DEV_API  *p_dev_api = DEF_NULL;
  RTOS_ERR local_err = {.Code = RTOS_ERR_NONE};
  if ((rx_buffer->header.info & SL_WFX_MSG_INFO_INTERFACE_MASK)
  	      == (SL_WFX_SOFTAP_INTERFACE << SL_WFX_MSG_INFO_INTERFACE_OFFSET))
  {
	  /* Obtain the size of the packet and put it into the "len" variable. */
	  len = rx_buffer->body.frame_length;
	  buffer = (uint8_t *)&(rx_buffer->body.frame[rx_buffer->body.frame_padding]);
	  p_if = get_net_if_for_ethernet_interface();
	  if (p_if == NULL)
		  return;

	  p_dev_api = (NET_DEV_API *)p_if->Dev_API;
	  if (p_dev_api == NULL)
		  return;

	  p_dev_api->Tx(p_if, buffer, len, &local_err);
	  NetIF_DevTxRdyWait(p_if, &local_err);
  }
}
