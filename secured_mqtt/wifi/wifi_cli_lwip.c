/**************************************************************************//**
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
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
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "wifi_cli_lwip.h"
#include "ethernetif.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/apps/httpd.h"
#include "lwip/netifapi.h"
#include "dhcp_client.h"
#include "dhcp_server.h"
#include "sl_wfx_task.h"
#include "sl_wfx_host.h"
#include "lwip/dns.h"
/*******************************************************************************
 ******************   Wi-Fi CLI lwIP App Task Configuration   *****************
 ******************************************************************************/
/* LWIP task priority */
#define WFX_CLI_LWIP_TASK_PRIO      32u
/* LWIP task stack size */
#define WFX_CLI_LWIP_TASK_STK_SIZE  800u

/* LWIP task stack */
static CPU_STK  wfx_cli_lwip_task_stk[WFX_CLI_LWIP_TASK_STK_SIZE];
/* LWIP task tcb */
static OS_TCB   wfx_cli_lwip_task_tcb;

/************************ Private variables ***********************************/
static uint16_t ping_nb_packet_received = 0;
static uint16_t ping_nb_packet_sent = 0;
static uint32_t ping_echo_total_time = 0;
static uint32_t ping_echo_min_time = 0xFFFFFFFF;
static uint32_t ping_echo_max_time = 0;
static uint16_t ping_seq_num = 0;
static uint32_t ping_time = 0;

/**************************************************************************//**
 * Set station link status to up.
 *****************************************************************************/
sl_status_t set_sta_link_up(void)
{
  netifapi_netif_set_up(&sta_netif);
  netifapi_netif_set_link_up(&sta_netif);
  if (use_dhcp_client) {
    dhcpclient_set_link_state(1);
  }
  return SL_STATUS_OK;
}
/**************************************************************************//**
 * Set station link status to down.
 *****************************************************************************/
sl_status_t set_sta_link_down(void)
{
  if (use_dhcp_client) {
    dhcpclient_set_link_state(0);
  }
  netifapi_netif_set_link_down(&sta_netif);
  netifapi_netif_set_down(&sta_netif);
  return SL_STATUS_OK;
}

#if LWIP_RAW  /*!< LWIP_RAW is configured in lwipopts.h */
/***************************************************************************//**
 * @brief
 *    This callback function handles received messages
 *
 * @param[in]
 *
 * @param[out] None
 *
 * @return  None
 ******************************************************************************/
static u8_t ping_recv_fnp(void *arg,
                         struct raw_pcb *pcb,
                         struct pbuf *p,
                         const ip_addr_t *addr)
{
  (void) arg;
  (void) pcb;

  LWIP_ASSERT("p != NULL", p != NULL);

  struct icmp_echo_hdr *iecho_hdr_ptr = NULL;

  /** Filter out the received messages by length, only receive ICMP messages.
   * ICMP message size = sizeof(struct icmp_echo_hdr) + data_size.
   * Thus, tot_len >= PBUF_IP_HLEN + sizeof(struct icmp_echo_hdr)
   * Additionally, remove/peel the IP header.
   * */
  if ((p->tot_len >= (PBUF_IP_HLEN + sizeof(struct icmp_echo_hdr)))
      && (pbuf_remove_header(p, PBUF_IP_HLEN) == 0)) {

      iecho_hdr_ptr = (struct icmp_echo_hdr *) p->payload; /*!< ICMP header */

      if ((iecho_hdr_ptr->id == PING_ID)
          && (iecho_hdr_ptr->seqno == lwip_htons(ping_seq_num))) {

          /* Update statistic variables */
          ping_nb_packet_received++;

          uint32_t duration = sys_now() - ping_time;
          ping_echo_total_time += duration;

          if (duration < ping_echo_min_time) {
              ping_echo_min_time = duration;
          }

          if (duration > ping_echo_max_time) {
              ping_echo_max_time = duration;
          }

          printf("Reply from %s: bytes=%d, time=%lums\r\n", ipaddr_ntoa(addr),
                                                            p->len,
                                                            duration);
          pbuf_free(p);

          /* Eat the packet (received ICMP) */
          return 1; /*!< Be careful with '1'. Because it would be deleted */
      }

      /* Packet not eaten, restore the original one */
      pbuf_add_header(p, PBUF_IP_HLEN);
  }

  /* Packet has not eaten */
  return 0;
}


/***************************************************************************//**
 * @brief
 *    This function sends ping ICMP request message
 *
 * @param[in]
 *
 * @param[out] None
 *
 * @return  None
 ******************************************************************************/
static int ping_send(struct raw_pcb *ping_raw_pcb,
                     const ip_addr_t *dest_ip_addr,
                     uint16_t data_size)
{

  struct pbuf *pbf = NULL;  /*!< packet buffer pointer (layer 3) */
  struct icmp_echo_hdr *iecho;
  size_t ping_msg_size, i;
  int res = -1;

  /* ping message size */
  ping_msg_size = sizeof(struct icmp_echo_hdr) + data_size;

  /* Allocate a packet buffer memory with ping msg size and IPv4 layer header size */
  pbf = pbuf_alloc(PBUF_IP, ping_msg_size, PBUF_RAM);
  if (pbf != NULL) {
      iecho = (struct icmp_echo_hdr *) pbf->payload;

      ICMPH_TYPE_SET(iecho, ICMP_ECHO);
      ICMPH_CODE_SET(iecho, 0);
      iecho->chksum = 0;
      iecho->id = PING_ID;     /*!< MSB = LSB (0xAFAF) - no need to convert */
      iecho->seqno = lwip_htons(++ping_seq_num);

      /* Fill the additional data buffer with some data */
      for (i = 0; i < data_size; i++) {
          ((char *)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i;
      }

      iecho->chksum = inet_chksum(iecho, ping_msg_size);

      res = raw_sendto(ping_raw_pcb, pbf, dest_ip_addr);
      if (res == 0) {
          ping_time = sys_now();
          ping_nb_packet_sent++;
      }
      pbuf_free(pbf);
  }
  return res;
}

/**************************************************************************//**
 * @brief: Implementation of the ping command.
 *
 * @param[in]
 *         + req_num: Number of request messages (ICMP)
 *         + ip_str: Remote IP's address
 *
 * @param[out]  None
 *
 * @return
 *        SL_STATUS_OK if success
 *        SL_STATUS_FAIL if error
 *****************************************************************************/
sl_status_t ping_cmd(uint32_t req_num, char *ip_str)
{

  int res = -1;

  struct raw_pcb *ping_pcb = NULL; /*!< protocol control block */
  ip_addr_t ping_addr;

  /* Parsing IP address */
  res = ipaddr_aton(ip_str, &ping_addr);
  if (res == 0) {
      printf("Failed to convert string (%s) to IP\r\n", ip_str);
      return SL_STATUS_FAIL;
  }

  /* Allocate a new resource raw_pcb */
  ping_pcb = raw_new(IP_PROTO_ICMP);
  if (ping_pcb == NULL) {
      LOG_DEBUG("Failed to allocate resource for ping_pcb\r\n");
      return SL_STATUS_ALLOCATION_FAILED;
  }

  /* Configure the IP stack */
  raw_recv(ping_pcb, ping_recv_fnp, NULL);
  raw_bind(ping_pcb, IP_ADDR_ANY);

  printf("Pinging %s with %d bytes of data:\r\n",
           ip_str, PING_DEFAULT_DATA_SIZE);

  /* Reset internal variables */
  ping_seq_num = 0;
  ping_nb_packet_sent = 0;
  ping_nb_packet_received = 0;
  ping_echo_total_time = 0;
  ping_echo_min_time = 0xFFFFFFFF;
  ping_echo_max_time = 0;

  while(req_num-- > 0) {
      /* Send ping request to IP destination */
      res = ping_send(ping_pcb, &ping_addr, PING_DEFAULT_DATA_SIZE);
      if (res != 0) {
          printf("Send ping request to IP (%s) failed\r\n", ip_str);
          raw_remove(ping_pcb); /*!< release ping_pcv */
          return SL_STATUS_FAIL;
      }
      /* Wait for receiving ICMP responses from the destination IP */
      sys_msleep(PING_DEFAULT_INTERVAL_SEC * 1000);
  }

  if (res == 0) {
    /* Display statistics */
    printf("\r\nPing statistics for %s:\r\n", ip_str);
    printf("\tPackets: Sent = %u, Received = %u, Lost = %u (%u%% loss),\r\n",
           ping_nb_packet_sent,
           ping_nb_packet_received,
           ping_nb_packet_sent-ping_nb_packet_received,
           ((ping_nb_packet_sent-ping_nb_packet_received)*100)/ping_nb_packet_sent);
    printf("Approximate round trip times in milli-seconds:\r\n");
    printf("\tMinimum = %lums, Maximum = %lums, Average = %lums\r\n",
           ping_echo_min_time,
           ping_echo_max_time,
           ping_echo_total_time/ping_nb_packet_received);
  }
  raw_remove(ping_pcb); /*!< relese ping_pcv */
  return SL_STATUS_OK;
}
#endif /* LWIP_RAW */

/***************************************************************************//**
 * @brief
 *    This function initializes Wi-Fi network interfaces
 *
 * @param[in]
 *
 * @param[out] None
 *
 * @return  None
 ******************************************************************************/
static void netif_config(void)
{
  ip_addr_t sta_ipaddr, sta_netmask, sta_gw;

  if (use_dhcp_client) {
    ip_addr_set_zero_ip4(&sta_ipaddr);
    ip_addr_set_zero_ip4(&sta_netmask);
    ip_addr_set_zero_ip4(&sta_gw);
  } else {
    IP_ADDR4(&sta_ipaddr, \
             sta_ip_addr0, \
             sta_ip_addr1, \
             sta_ip_addr2, \
             sta_ip_addr3);

    IP_ADDR4(&sta_netmask, \
             sta_netmask_addr0, \
             sta_netmask_addr1, \
             sta_netmask_addr2, \
             sta_netmask_addr3);

    IP_ADDR4(&sta_gw, \
             sta_gw_addr0, \
             sta_gw_addr1, \
             sta_gw_addr2, \
             sta_gw_addr3);
  }

  /* Add Station interfaces */
  netif_add(&sta_netif,
            &sta_ipaddr,
            &sta_netmask,
            &sta_gw,
            NULL,
            &sta_ethernetif_init,
            &tcpip_input);

  /* Register the default network interface. */
  netif_set_default(&sta_netif);
}

/***************************************************************************//**
 * @brief
 *    This task create lwIP TCP/IP stack; configure default network interface
 *    & start DHCP client if used
 *
 * @param[in]
 *
 * @param[out] None
 *
 * @return  None
 ******************************************************************************/
void lwip_init_start_task(void *p_arg)
{
  (void)p_arg;
  RTOS_ERR err;
  ip_addr_t prim_dns_addr = IPADDR4_INIT_BYTES(8, 8, 8, 8);
  ip_addr_t sec_dns_addr = IPADDR4_INIT_BYTES(8, 8, 4, 4);

  /* Create tcip_ip stack thread */
  tcpip_init(NULL, NULL);

  /* Initialize Wifi network interfaces */
  netif_config();

  /* Start DHCP Client*/
  if (use_dhcp_client) {
      dhcpclient_start();
  }

  // Initialize the DNS client (connecting to Google DNS servers)
  dns_setserver(0, &prim_dns_addr);
  dns_setserver(0, &sec_dns_addr);
  dns_init();

  /* Delete Lwip init task */
  OSTaskDel(NULL, &err);
}

/***************************************************************************//**
 * @brief
 *    Start lwip tcp/ip stack & related tasks
 *
 * @param[in]
 *
 * @param[out] None
 *
 * @return  None
 ******************************************************************************/
void lwip_init_start(void)
{
  RTOS_ERR err;

  /* Create lwip initialization task */
  OSTaskCreate(&wfx_cli_lwip_task_tcb,
               "lwip start task",
               lwip_init_start_task,
               DEF_NULL,
               WFX_CLI_LWIP_TASK_PRIO,
               &wfx_cli_lwip_task_stk[0],
               (WFX_CLI_LWIP_TASK_STK_SIZE/10u),
               WFX_CLI_LWIP_TASK_STK_SIZE,
               0u,
               0u,
               DEF_NULL,
               (OS_OPT_TASK_STK_CLR),
               &err);

  /* Check err code */
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
}

