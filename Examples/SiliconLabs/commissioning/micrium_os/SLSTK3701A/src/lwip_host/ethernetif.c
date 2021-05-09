/***************************************************************************//**
 * @file
 * @brief Ethernet interface implementation for LwIP and WFX
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

#include "lwip/timeouts.h"
#include "netif/etharp.h"
#include "ethernetif.h"
#include <string.h>
#include "sl_wfx_constants.h"
#include "sl_wfx_host_api.h"
#include "sl_wfx.h"
#include "lwip_micriumos.h"

#include <kernel/include/os.h>
#include <common/include/rtos_utils.h>
#include <common/include/rtos_err.h>
#include <common/source/kal/kal_priv.h>
#include <common/include/rtos_err.h>
#include "sl_wfx_task.h"

#define STATION_NETIF0 's'
#define STATION_NETIF1 't'
#define SOFTAP_NETIF0  'a'
#define SOFTAP_NETIF1  'p'


/***************************************************************************//**
 * Initializes the hardware parameters. Called from ethernetif_init().
 *
 * @param netif The already initialized lwip network interface structure
 ******************************************************************************/
static void low_level_init(struct netif *netif)
{
  // set netif MAC hardware address length
  netif->hwaddr_len = ETH_HWADDR_LEN;

  // set netif MAC hardware address
  if (netif->name[0] == SOFTAP_NETIF0) { //AP Mode
    netif->hwaddr[0] =  wifi.mac_addr_1.octet[0];
    netif->hwaddr[1] =  wifi.mac_addr_1.octet[1];
    netif->hwaddr[2] =  wifi.mac_addr_1.octet[2];
    netif->hwaddr[3] =  wifi.mac_addr_1.octet[3];
    netif->hwaddr[4] =  wifi.mac_addr_1.octet[4];
    netif->hwaddr[5] =  wifi.mac_addr_1.octet[5];
  } else {
    netif->hwaddr[0] =  wifi.mac_addr_0.octet[0];
    netif->hwaddr[1] =  wifi.mac_addr_0.octet[1];
    netif->hwaddr[2] =  wifi.mac_addr_0.octet[2];
    netif->hwaddr[3] =  wifi.mac_addr_0.octet[3];
    netif->hwaddr[4] =  wifi.mac_addr_0.octet[4];
    netif->hwaddr[5] =  wifi.mac_addr_0.octet[5];
  }

  // set netif maximum transfer unit
  netif->mtu = 1500;

  // Accept broadcast address and ARP traffic
  netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

  // Set netif link flag
  netif->flags |= NETIF_FLAG_LINK_UP;
}

/***************************************************************************//**
 * Transmits packet(s).
 *
 * @param netif the lwip network interface structure
 * @param p the packet to send
 * @returns ERR_OK if successful
 ******************************************************************************/
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
  err_t errval = 0;
  RTOS_ERR err;
  struct pbuf *q;
  sl_wfx_send_frame_req_t*      tx_buffer;
  uint8_t *buffer;
  uint32_t framelength = 0;
  uint32_t bufferoffset = 0;
  uint32_t byteslefttocopy = 0;
  uint32_t i;
  bufferoffset = 0;
  uint32_t padding;
  sl_status_t result = SL_STATUS_ALLOCATION_FAILED;
  sl_wfx_interface_t interface;

  for (q = p; q != NULL; q = q->next) {
    framelength = framelength + q->len;
  }
  if (framelength < 60) {
    padding = 60 - framelength;
  } else {
    padding = 0;
  }

  result = sl_wfx_host_allocate_buffer((void**)(&tx_buffer), SL_WFX_TX_FRAME_BUFFER, SL_WFX_ROUND_UP(framelength + padding, 64) + sizeof(sl_wfx_send_frame_req_t)); //12 is size of other data in buffer struct, user shouldn't have to care about this?
  if (result == SL_STATUS_OK) {
    buffer = tx_buffer->body.packet_data;

    framelength = 0;
    /* copy frame from pbufs to driver buffers */
    for (q = p; q != NULL; q = q->next) {
      /* Get bytes in current lwIP buffer */
      byteslefttocopy = q->len;

      /* Copy the bytes */
      memcpy( (uint8_t*)((uint8_t*)buffer + bufferoffset), (uint8_t*)((uint8_t*)q->payload), byteslefttocopy);
      bufferoffset = bufferoffset + byteslefttocopy;
      framelength = framelength + byteslefttocopy;
    }
    for (i = framelength; i < framelength + padding; i++) {
      //fill zeros
      buffer[i] = 0;
    }

    /* transmit */
    if (netif->name[0] == SOFTAP_NETIF0) {
      interface = SL_WFX_SOFTAP_INTERFACE;
    } else {
      interface = SL_WFX_STA_INTERFACE;
    }
    wfx_bus_tx_frame.data_length = framelength + padding;
    wfx_bus_tx_frame.frame = tx_buffer;
    wfx_bus_tx_frame.interface = interface;
    wfx_bus_tx_frame.priority = 0;

    OSFlagPost(&wfx_bus_evts, SL_WFX_BUS_EVENT_FLAG_TX, OS_OPT_POST_FLAG_SET, &err);
    OSSemPend(&wfx_bus_tx_complete, 0, OS_OPT_PEND_BLOCKING, 0, &err);
    result = SL_STATUS_OK;

    sl_wfx_host_free_buffer(tx_buffer, SL_WFX_TX_FRAME_BUFFER);

    if (result == SL_STATUS_OK) {
      errval = ERR_OK;
    } else {
      errval = ERR_IF;
      printf("Failed to send ethernet frame\r\n");
    }
  } else {
    //unable to send
    errval = ERR_MEM;
  }

  return errval;
}

/***************************************************************************//**
 * Transfers the receive packets from the wfx to lwip.
 *
 * @param netif lwip network interface structure
 * @param rx_buffer the ethernet frame received by the wf200
 * @returns LwIP pbuf filled with received packet, or NULL on error
 ******************************************************************************/
static struct pbuf * low_level_input(struct netif *netif, sl_wfx_received_ind_t* rx_buffer)
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
  len = rx_buffer->body.frame_length;
  buffer = (uint8_t *)&(rx_buffer->body.frame[rx_buffer->body.frame_padding]);

  if (len > 0) {
    /* We allocate a pbuf chain of pbufs from the Lwip buffer pool */
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
  }

  if (p != NULL) {
    bufferoffset = 0;
    for (q = p; q != NULL; q = q->next) {
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
 * WFX received frame callback.
 *
 * @param rx_buffer the ethernet frame received by the wfx
 ******************************************************************************/
void sl_wfx_host_received_frame_callback(sl_wfx_received_ind_t* rx_buffer)
{
  struct pbuf *p;
  struct netif *netif;
  /* Check packet interface to send to AP or STA interface */
  if ((rx_buffer->header.info & SL_WFX_MSG_INFO_INTERFACE_MASK)
      == (SL_WFX_STA_INTERFACE << SL_WFX_MSG_INFO_INTERFACE_OFFSET)) {
    /* Send to station interface */
    netif = &sta_netif;
  } else {
    /* Send to softAP interface */
    netif = &ap_netif;
  }
  if (netif != NULL) {
    p = low_level_input(netif, rx_buffer);
    if (p != NULL) {
      if (netif->input(p, netif) != ERR_OK ) {
        pbuf_free(p);
      }
    }
  }
}

/***************************************************************************//**
 * Sets up the station network interface.
 *
 * @param netif the lwip network interface structure
 * @returns ERR_OK if successful
 ******************************************************************************/
err_t sta_ethernetif_init(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));
#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "lwip_sta";
#endif /* LWIP_NETIF_HOSTNAME */
  /* Set the netif name to identify the interface */
  netif->name[0] = STATION_NETIF0;
  netif->name[1] = STATION_NETIF1;

  netif->output = etharp_output;
  netif->linkoutput = low_level_output;

  /* initialize the hardware */
  low_level_init(netif);

  return ERR_OK;
}

/***************************************************************************//**
 * Sets up the AP network interface.
 *
 * @param netif the lwip network interface structure
 * @returns ERR_OK if successful
 ******************************************************************************/
err_t ap_ethernetif_init(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));
#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "lwip_ap";
#endif /* LWIP_NETIF_HOSTNAME */

  netif->name[0] = SOFTAP_NETIF0;
  netif->name[1] = SOFTAP_NETIF1;

  netif->output = etharp_output;
  netif->linkoutput = low_level_output;

  /* initialize the hardware */
  low_level_init(netif);

  return ERR_OK;
}
