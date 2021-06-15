/**************************************************************************//**
 * Copyright 2021, Silicon Laboratories Inc.
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

#include "stm32f4xx_hal.h"
#include "sl_wfx_host.h"
#include "lwip_common.h"
#include "lwip/timeouts.h"
#include "netif/etharp.h"
#include "ethernetif.h"

/***************************************************************************//**
 * Defines
******************************************************************************/
#define STATION_NETIF "st"
#define SOFTAP_NETIF "ap"

/***************************************************************************//**
 * Variables
******************************************************************************/
/* Default parameters */
sl_wfx_context_t wifi_context;
char wlan_ssid[32+1]                    = WLAN_SSID_DEFAULT;
char wlan_passkey[64+1]                 = WLAN_PASSKEY_DEFAULT;
sl_wfx_password_t wlan_pmk              = {0};
sl_wfx_security_mode_t wlan_security    = WLAN_SECURITY_DEFAULT;
char softap_ssid[32+1]                  = SOFTAP_SSID_DEFAULT;
char softap_passkey[64+1]               = SOFTAP_PASSKEY_DEFAULT;
sl_wfx_password_t softap_pmk            = {0};
sl_wfx_security_mode_t softap_security  = SOFTAP_SECURITY_DEFAULT;
uint8_t softap_channel                  = SOFTAP_CHANNEL_DEFAULT;
/* Default parameters */

/***************************************************************************//**
 * @brief
 *    Initializes the hardware parameters. Called from ethernetif_init().
 *
 * @param[in] netif: the already initialized lwip network interface structure
 *
 * @return
 *    None
 ******************************************************************************/
static void low_level_init (struct netif *netif) {
  /* set netif MAC hardware address length */
  netif->hwaddr_len = ETH_HWADDR_LEN;
  
  /* Check which netif is initialized and set netif MAC hardware address */
  if (memcmp(netif->name, STATION_NETIF, 2) == 0) {
    memcpy(netif->hwaddr, wifi_context.mac_addr_0.octet, 6);
  } else {
    memcpy(netif->hwaddr, wifi_context.mac_addr_1.octet, 6);
  }

  /* Set netif maximum transfer unit */
  netif->mtu = 1500;

  /* Accept broadcast address and ARP traffic */
  netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

  /* Set netif link flag */
  netif->flags |= NETIF_FLAG_LINK_UP;
}

/***************************************************************************//**
 * @brief
 *    This function should does the actual transmission of the packet(s).
 *    The packet is contained in the pbuf that is passed to the function. 
 *    This pbuf might be chained.
 *
 * @param[in] netif: the lwip network interface structure
 *
 * @param[in] p: the packet to send
 *
 * @return
 *    ERR_OK if successful
 ******************************************************************************/
static err_t low_level_output (struct netif *netif, struct pbuf *p) {
  struct pbuf *q;
  uint8_t *buffer;
  sl_wfx_packet_queue_item_t *queue_item;
  sl_status_t result;
  
  /* Take TX queue mutex */
  xSemaphoreTake(sl_wfx_tx_queue_mutex, portMAX_DELAY);

  /* Allocate a buffer for a queue item */
  result = sl_wfx_allocate_command_buffer((sl_wfx_generic_message_t**)(&queue_item),
                                          SL_WFX_SEND_FRAME_REQ_ID,
                                          SL_WFX_TX_FRAME_BUFFER,
                                          p->tot_len + sizeof(sl_wfx_packet_queue_item_t));
  
  if ((result != SL_STATUS_OK) || (queue_item == NULL)) {
    return ERR_MEM;
  }

  buffer = queue_item->buffer.body.packet_data;
  
  for (q = p; q != NULL; q = q->next) {
    /* Copy the bytes */
    memcpy(buffer, q->payload, q->len);
    buffer += q->len;
  }

  /* Provide the data length the interface information to the pbuf */
  queue_item->interface = (memcmp(netif->name, STATION_NETIF, 2) == 0)?  SL_WFX_STA_INTERFACE : SL_WFX_SOFTAP_INTERFACE;
  queue_item->data_length = p->tot_len;
  
  /* Determine if there is anything on the tx packet queue */
  if (sl_wfx_tx_queue_context.head_ptr != NULL) {
    sl_wfx_tx_queue_context.tail_ptr->next = queue_item;
  } else {
    /* If tx packet queue is empty, setup head & tail pointers */
    sl_wfx_tx_queue_context.head_ptr = queue_item;
  }
  
  /* Update the tail pointer */
  sl_wfx_tx_queue_context.tail_ptr = queue_item;
  
  /* Notify that a TX frame is ready */
  xEventGroupSetBits(sl_wfx_event_group, SL_WFX_TX_PACKET_AVAILABLE);
  
  /* Release TX queue mutex */
  xSemaphoreGive(sl_wfx_tx_queue_mutex);
  
  return ERR_OK;
}

/***************************************************************************//**
 * @brief
 *    This function transfers the receive packets from the wf200 to lwip.
 *
 * @param[in] netif: the lwip network interface structure
 *
 * @param[in] rx_buffer: the ethernet frame received by the wf200
 *
 * @return
 *    LwIP pbuf filled with received packet, or NULL on error
 ******************************************************************************/
static struct pbuf *low_level_input (struct netif *netif, sl_wfx_received_ind_t* rx_buffer) {
  struct pbuf *p, *q;
  uint8_t *buffer;
  
  /* Obtain the packet by removing the padding. */
  buffer = (uint8_t *)&(rx_buffer->body.frame[rx_buffer->body.frame_padding]);
 
  if (rx_buffer->body.frame_length > 0) {
    /* We allocate a pbuf chain of pbufs from the Lwip buffer pool */
    p = pbuf_alloc(PBUF_RAW, rx_buffer->body.frame_length, PBUF_POOL);
  }
  
  if (p != NULL) {
    for (q = p; q != NULL; q = q->next) {
      /* Copy remaining data in pbuf */
      memcpy(q->payload, buffer, q->len);
      buffer += q->len;
    }
  }  
   
  return p;
}

/***************************************************************************//**
 * @brief
 *    This function implements the wf200 received frame callback.
 *
 * @param[in] rx_buffer: the ethernet frame received by the wf200
 *
 * @return
 *    None
******************************************************************************/
void sl_wfx_host_received_frame_callback (sl_wfx_received_ind_t* rx_buffer) {
  struct pbuf *p;
  struct netif *netif;
  
  /* Check packet interface to send to AP or STA interface */
  if ((rx_buffer->header.info & SL_WFX_MSG_INFO_INTERFACE_MASK) == 
     (SL_WFX_STA_INTERFACE << SL_WFX_MSG_INFO_INTERFACE_OFFSET)) {
    /* Send to station interface */
    netif = &sta_netif;
  } else {
    /* Send to softAP interface */
    netif = &ap_netif;
  }

  if (netif != NULL) {
    p = low_level_input(netif, rx_buffer);
    if (p != NULL) {
      if (netif->input(p, netif) != ERR_OK) {
        pbuf_free(p);
      }
    }
  }
}

/***************************************************************************//**
 * @brief
 *    called at the beginning of the program to set up the network interface.
 *
 * @param[in] netif: the lwip network interface structure
 *
 * @return
 *    ERR_OK if successful
 ******************************************************************************/
err_t sta_ethernetif_init (struct netif *netif) {
  LWIP_ASSERT("netif != NULL", (netif != NULL));

  /* Set the netif name to identify the interface */
  memcpy(netif->name, STATION_NETIF, 2);

  netif->output = etharp_output;
  netif->linkoutput = low_level_output;

  /* initialize the hardware */
  low_level_init(netif);
  
  return ERR_OK;
}

/***************************************************************************//**
 * @brief
 *    called at the beginning of the program to set up the network interface.
 *
 * @param[in] netif: the lwip network interface structure
 *
 * @return
 *    ERR_OK if successful
 ******************************************************************************/
err_t ap_ethernetif_init (struct netif *netif) {
  LWIP_ASSERT("netif != NULL", (netif != NULL));

  /* Set the netif name to identify the interface */
  memcpy(netif->name, SOFTAP_NETIF, 2);

  netif->output = etharp_output;
  netif->linkoutput = low_level_output;

  /* initialize the hardware */
  low_level_init(netif);
  
  return ERR_OK;
}

u32_t sys_now (void) {
  return HAL_GetTick();
}
