
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

/* Includes */
#include "stm32f4xx_hal.h"
#include "lwip/timeouts.h"
#include "netif/etharp.h"
#include "ethernetif.h"
#include <string.h>
#include "wf200_constants.h"
#include "wf200_host_api.h"
#include "wf200.h"
#include "wf200_host.h"
#include "wf200_host.h"
#include "demo_config.h"

/***************************************************************************//**
 * Defines
******************************************************************************/
#define IFNAME0 's' ///< Network interface name 0
#define IFNAME1 'l' ///< Network interface name 1

wf200_context_t wifi;
wf200_status_t wf200_status;
char wlan_ssid[]                 = WLAN_SSID_DEFAULT;
char wlan_passkey[]              = WLAN_PASSKEY_DEFAULT;
WfmSecurityMode wlan_security    = WLAN_SECURITY_DEFAULT;

char softap_ssid[]               = SOFTAP_SSID_DEFAULT;
char softap_passkey[]            = SOFTAP_PASSKEY_DEFAULT;
WfmSecurityMode softap_security  = SOFTAP_SECURITY_DEFAULT;
uint8_t softap_channel           = SOFTAP_CHANNEL_DEFAULT;

/***************************************************************************//**
 * Private variables
******************************************************************************/
static struct netif *netifGbl = NULL;

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
  WfmHiConnectInd_t* reply = NULL;

  /* set netif MAC hardware address length */
  netif->hwaddr_len = ETH_HWADDR_LEN;

  /* set netif MAC hardware address */
if (soft_ap_mode)
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

  /* set netif maximum transfer unit */
  netif->mtu = 1500;

  /* Accept broadcast address and ARP traffic */
  netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

if (soft_ap_mode)
{
  wf200_start_ap_command(softap_channel, (uint8_t*) softap_ssid, strlen(softap_ssid), softap_security, (uint8_t*) softap_passkey, strlen(softap_passkey));
  wf200_host_setup_waited_event( WFM_HI_START_AP_IND_ID );
  if(wf200_host_wait_for_confirmation(WF200_DEFAULT_REQUEST_TIMEOUT, (void**)&reply) == SL_TIMEOUT)
  {
    return;
  }
}
else
{
  wf200_send_join_command((uint8_t*) wlan_ssid, strlen(wlan_ssid), wlan_security, (uint8_t*) wlan_passkey, strlen(wlan_passkey));
  wf200_host_setup_waited_event( WFM_HI_CONNECT_IND_ID );
  if(wf200_host_wait_for_confirmation(WF200_DEFAULT_REQUEST_TIMEOUT, (void**)&reply) == SL_TIMEOUT)
  {
    return;
  }
}
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
  sl_status_t status;
  err_t errval=0;
  struct pbuf *q;
  wf200_frame_t*      tx_buffer;
  uint8_t *buffer ;
  uint32_t framelength = 0;
  uint32_t bufferoffset = 0;
  uint32_t byteslefttocopy = 0;
  uint32_t i;
  bufferoffset = 0;
  uint32_t padding;
  
  for(q = p; q != NULL; q = q->next)
  {
    framelength = framelength + q->len;
  }
  if (framelength < 60)
  {
    padding = 60 - framelength;
  }
  else {
    padding = 0;
  }
  
  wf200_host_allocate_buffer ((wf200_buffer_t**)(&tx_buffer),WF200_TX_FRAME_BUFFER,ROUND_UP( framelength+padding, 64 )+sizeof(wf200_frame_t),0); //12 is size of other data in buffer struct, user shouldn't have to care about this?
  buffer = tx_buffer->data;
  
  framelength = 0;
  /* copy frame from pbufs to driver buffers */
  for(q = p; q != NULL; q = q->next)
  {
    /* Get bytes in current lwIP buffer */
    byteslefttocopy = q->len;
    
    /* Copy the bytes */
    memcpy( (uint8_t*)((uint8_t*)buffer + bufferoffset), (uint8_t*)((uint8_t*)q->payload), byteslefttocopy );
    bufferoffset = bufferoffset + byteslefttocopy;
    framelength = framelength + byteslefttocopy;
  }
  for (i=framelength;i< framelength+padding;i++)
  {
    //fill zeros
    buffer[i] = 0;
  }
  
  /* transmit */ 
  if (soft_ap_mode)
  {
    status = wf200_send_ethernet_frame( tx_buffer, framelength + padding, WF200_SOFTAP_INTERFACE);
  }
  else
  {
    status = wf200_send_ethernet_frame( tx_buffer, framelength + padding, WF200_STA_INTERFACE);
  }
  wf200_host_free_buffer((wf200_buffer_t*)  tx_buffer, WF200_TX_FRAME_BUFFER );
  if(status == SL_SUCCESS){
    errval = ERR_OK;
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
static struct pbuf * low_level_input(struct netif *netif, wf200_ethernet_frame_t* rx_buffer)
{
  struct pbuf *p = NULL;
  struct pbuf *q = NULL;
  uint16_t len = 0;
  uint8_t *buffer;
  uint32_t bufferoffset = 0;
  uint32_t payloadoffset = 0;
  uint32_t byteslefttocopy = 0;
  /* get received frame */
  
  /* Obtain the size of the packet and put it into the "len" variable. */
  len = rx_buffer->frame_length-rx_buffer->frame_padding;
  buffer = (uint8_t *)&(rx_buffer->padding_and_data[rx_buffer->frame_padding]);
 
  if (len > 0)
  {
    /* We allocate a pbuf chain of pbufs from the Lwip buffer pool */
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
  }
  
  if (p != NULL)
  {
    bufferoffset = 0;
    for(q = p; q != NULL; q = q->next)
    {
      byteslefttocopy = q->len;
      payloadoffset = 0;
      
      /* Copy remaining data in pbuf */
      memcpy( (uint8_t*)((uint8_t*)q->payload + payloadoffset), (uint8_t*)((uint8_t*)buffer + bufferoffset), byteslefttocopy);
      bufferoffset = bufferoffset + byteslefttocopy;
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
void wf200_host_received_frame_callback( wf200_ethernet_frame_t* rx_buffer )
{
    struct pbuf *p;
    struct netif *netif = netifGbl;
    if (netif != NULL)
    {
        p = low_level_input( netif , rx_buffer);
        if (netif->input( p, netif) != ERR_OK )
        {
            pbuf_free(p);
        }
    }
}

/**
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */
void ethernetif_input(struct netif *netif,wf200_ethernet_frame_t* rx_buffer)
{
  err_t err;
  struct pbuf *p;

  /* move received packet into a new pbuf */
  p = low_level_input(netif, rx_buffer);
    
  /* no packet could be read, silently ignore this */
  if (p == NULL) return;
    
  /* entry point to the LwIP stack */
  err = netif->input(p, netif);
    
  if (err != ERR_OK)
  {
    LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
    pbuf_free(p);
    p = NULL;    
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
err_t ethernetif_init(struct netif *netif)
{
  sl_status_t status;
  LWIP_ASSERT("netif != NULL", (netif != NULL));

  /* configure WiFi */
  status = wf200_init( &wifi );
  switch(status) {
  case SL_SUCCESS:
    wf200_status = WF200_STARTED;
    printf("WF200 init successful\r\n");
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
#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;

  netif->output = etharp_output;
  netif->linkoutput = low_level_output;

  /* initialize the hardware */
  low_level_init(netif);
  netifGbl = netif;
  
  return ERR_OK;
}

u32_t sys_now(void)
{
  return HAL_GetTick();
}

#if LWIP_NETIF_LINK_CALLBACK
/**
  * @brief  Link callback function, this function is called on change of link status
  *         to update low level driver configuration.
* @param  netif: The network interface
  * @retval None
  */
void ethernetif_update_config(struct netif *netif)
{
  __IO uint32_t tickstart = 0;
  printf ("ethernetif_update_config \r\n");
  if(netif_is_link_up(netif))
  { 
    /* Restart the auto-negotiation */
      
      printf ("netif_is_link_up = true \r\n");
      /* Enable Auto-Negotiation */

      /* Get tick */
      tickstart = HAL_GetTick();
      
      /* Wait until the auto-negotiation will be completed */
   
  
      /* Configure the MAC with the speed fixed by the auto-negotiation process */
      
    
    

    /* ETHERNET MAC Re-Configuration */
    

    /* Restart MAC interface */
    
  }
  else
  {
    /* Stop MAC interface */
    
  }

  ethernetif_notify_conn_changed(netif);
}

/**
  * @brief  This function notify user about link status changement.
  * @param  netif: the network interface
  * @retval None
  */
__weak void ethernetif_notify_conn_changed(struct netif *netif)
{
  /* NOTE : This is function could be implemented in user file 
            when the callback is needed,
  */

}

#endif /* LWIP_NETIF_LINK_CALLBACK */