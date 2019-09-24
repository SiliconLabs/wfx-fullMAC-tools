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

/* Includes */
#include "stm32f4xx_hal.h"
#include <string.h>

#include "sl_wfx.h"
#include "sl_wfx_host.h"

/* LwIP includes. */
#include "lwip_freertos.h"
#include "lwip/timeouts.h"
#include "netif/etharp.h"
#include "ethernetif.h"

/***************************************************************************//**
 * Defines
******************************************************************************/
#define STATION_NETIF0 's'
#define STATION_NETIF1 't'
#define SOFTAP_NETIF0  'a'
#define SOFTAP_NETIF1  'p'

/***************************************************************************//**
 * Variables
******************************************************************************/
extern sl_wfx_context_t wifi;
extern char wlan_ssid[];
extern char wlan_passkey[];
extern sl_wfx_security_mode_t wlan_security;

extern char softap_ssid[];
extern char softap_passkey[];
extern sl_wfx_security_mode_t softap_security;
extern uint8_t softap_channel;

extern osSemaphoreId s_xDriverSemaphore;

/***************************************************************************//**
 * @brief
 *    Initializes the hardware parameters. Called from ethernetif_init().
 *
 * @param[in] netif: the already initialized lwip network interface structure
 *
 * @return
 *    None
 ******************************************************************************/
static void low_level_init(struct netif *netif)
{
  /* set netif MAC hardware address length */
  netif->hwaddr_len = ETH_HWADDR_LEN;
  
  /* Check which netif is initialized and set netif MAC hardware address */
  if (netif->name[0] == SOFTAP_NETIF0)
  {
    netif->hwaddr[0] =  wifi.mac_addr_1.octet[0];
    netif->hwaddr[1] =  wifi.mac_addr_1.octet[1];
    netif->hwaddr[2] =  wifi.mac_addr_1.octet[2];
    netif->hwaddr[3] =  wifi.mac_addr_1.octet[3];
    netif->hwaddr[4] =  wifi.mac_addr_1.octet[4];
    netif->hwaddr[5] =  wifi.mac_addr_1.octet[5];
  }
  else
  {
    netif->hwaddr[0] =  wifi.mac_addr_0.octet[0];
    netif->hwaddr[1] =  wifi.mac_addr_0.octet[1];
    netif->hwaddr[2] =  wifi.mac_addr_0.octet[2];
    netif->hwaddr[3] =  wifi.mac_addr_0.octet[3];
    netif->hwaddr[4] =  wifi.mac_addr_0.octet[4];
    netif->hwaddr[5] =  wifi.mac_addr_0.octet[5];
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
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
  err_t errval;
  struct pbuf *q;
  sl_wfx_send_frame_req_t* tx_buffer;
  sl_status_t result;
  uint8_t *buffer ;
  uint32_t frame_length;
  
  /* Compute packet frame length */
  frame_length = SL_WFX_ROUND_UP(p->tot_len, 2);
  
  if(xSemaphoreTake(s_xDriverSemaphore, 500) == pdTRUE)
  {
    sl_wfx_allocate_command_buffer((sl_wfx_generic_message_t**)(&tx_buffer),
                                   SL_WFX_SEND_FRAME_REQ_ID,
                                   SL_WFX_TX_FRAME_BUFFER,
                                   frame_length + sizeof(sl_wfx_send_frame_req_t));
    buffer = tx_buffer->body.packet_data;
    
    for(q = p; q != NULL; q = q->next)
    {
      /* Copy the bytes */
      memcpy(buffer, q->payload, q->len);
      buffer += q->len;
    }
    
    /* Transmit to the station or softap interface */ 
    if (netif->name[0] == SOFTAP_NETIF0)
    {
      result = sl_wfx_send_ethernet_frame(tx_buffer,
                                          frame_length,
                                          SL_WFX_SOFTAP_INTERFACE,
                                          WFM_PRIORITY_BE);
    }
    else
    {
      result = sl_wfx_send_ethernet_frame(tx_buffer,
                                          frame_length,
                                          SL_WFX_STA_INTERFACE,
                                          WFM_PRIORITY_BE);
    }
    sl_wfx_free_command_buffer((sl_wfx_generic_message_t*) tx_buffer,
                               SL_WFX_SEND_FRAME_REQ_ID,
                               SL_WFX_TX_FRAME_BUFFER);
    xSemaphoreGive(s_xDriverSemaphore);
    if(result == SL_SUCCESS)
    {
      errval = ERR_OK;
    }else{
      errval = ERR_MEM;
    }
  }
  else
  {
    printf("Wi-Fi TX sem timeout\r\n");
    errval = ERR_TIMEOUT;
  }
  
  return errval;
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
static struct pbuf *low_level_input(struct netif *netif, sl_wfx_received_ind_t* rx_buffer)
{
  struct pbuf *p, *q;
  uint8_t *buffer;
  
  /* Obtain the packet by removing the padding. */
  buffer = (uint8_t *)&(rx_buffer->body.frame[rx_buffer->body.frame_padding]);
 
  if (rx_buffer->body.frame_length > 0)
  {
    /* We allocate a pbuf chain of pbufs from the Lwip buffer pool */
    p = pbuf_alloc(PBUF_RAW, rx_buffer->body.frame_length, PBUF_POOL);
  }
  
  if (p != NULL)
  {
    for(q = p; q != NULL; q = q->next)
    {
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
void sl_wfx_host_received_frame_callback(sl_wfx_received_ind_t* rx_buffer)
{
  struct pbuf *p;
  struct netif *netif;
  
  /* Check packet interface to send to AP or STA interface */
  if((rx_buffer->header.info & SL_WFX_MSG_INFO_INTERFACE_MASK) == 
     (SL_WFX_STA_INTERFACE << SL_WFX_MSG_INFO_INTERFACE_OFFSET))
  {
    /* Send to station interface */
    netif = &sta_netif;
  }else{
    /* Send to softAP interface */
    netif = &ap_netif;
  }
     
  if (netif != NULL)
  {
    p = low_level_input(netif, rx_buffer);
    if (p != NULL)
    {
      if (netif->input(p, netif) != ERR_OK)
      {
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
err_t sta_ethernetif_init(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));

  /* Set the netif name to identify the interface */
  netif->name[0] = STATION_NETIF0;
  netif->name[1] = STATION_NETIF1;

  netif->output = etharp_output;
  netif->linkoutput = low_level_output;

  /* initialize the hardware */
  low_level_init(netif);
  sta_netif = *netif;
  
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
err_t ap_ethernetif_init(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));

  /* Set the netif name to identify the interface */
  netif->name[0] = SOFTAP_NETIF0;
  netif->name[1] = SOFTAP_NETIF1;

  netif->output = etharp_output;
  netif->linkoutput = low_level_output;

  /* initialize the hardware */
  low_level_init(netif);
  ap_netif = *netif;
  
  return ERR_OK;
}

u32_t sys_now(void)
{
  return HAL_GetTick();
}
