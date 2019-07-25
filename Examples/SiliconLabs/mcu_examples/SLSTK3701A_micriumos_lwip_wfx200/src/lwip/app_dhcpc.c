

#include "lwip/dhcp.h"
#include "lwip_micriumos.h"
#include <kernel/include/os.h>
#include <common/include/rtos_utils.h>
#include <common/include/rtos_err.h>

/* DHCP client state */
#define DHCP_OFF                   (uint8_t) 0
#define DHCP_START                 (uint8_t) 1
#define DHCP_WAIT_ADDRESS          (uint8_t) 2
#define DHCP_ADDRESS_ASSIGNED      (uint8_t) 3
#define DHCP_TIMEOUT               (uint8_t) 4
#define DHCP_LINK_DOWN             (uint8_t) 5
void KAL_Dly(CPU_INT32U dly_ms);

#define MAX_DHCP_TRIES  4
volatile uint8_t DHCP_state = DHCP_OFF;

extern struct netif gnetif;

/**
  * @brief  Notify task about the wifi status
  * @param  link_up: link status
  * @retval None
  */
void link_state_notification(int link_up)
{
  if (link_up)
  {
    DHCP_state = DHCP_START;
  }
  else
  {
    /* Update DHCP state machine */
    DHCP_state = DHCP_LINK_DOWN;
  }
}


/**
  * @brief  DHCP Client Task
  * @param  argument: network interface
  * @retval None
  */
void DHCP_client_task(void *arg)
{
  struct netif *netif = (struct netif *) arg;
  ip_addr_t ipaddr;
  ip_addr_t netmask;
  ip_addr_t gw;
  struct dhcp *dhcp;
  for (;;)
  {
    switch (DHCP_state)
    {
    case DHCP_START:
      {
        ip_addr_set_zero_ip4(&netif->ip_addr);
        ip_addr_set_zero_ip4(&netif->netmask);
        ip_addr_set_zero_ip4(&netif->gw);
        dhcp_start(netif);
        DHCP_state = DHCP_WAIT_ADDRESS;
      }
      break;

    case DHCP_WAIT_ADDRESS:
      {
        if (dhcp_supplied_address(netif))
        {
          DHCP_state = DHCP_ADDRESS_ASSIGNED;
          printf("IP address : %3d.%3d.%3d.%3d\r\n",
                  (int)(gnetif.ip_addr.addr & 0xff),
                  (int)((gnetif.ip_addr.addr >> 8) & 0xff),
                  (int)((gnetif.ip_addr.addr >> 16) & 0xff),
                  (int)((gnetif.ip_addr.addr >> 24) & 0xff));

        }
        else
        {
          dhcp = (struct dhcp *)netif_get_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP);

          /* DHCP timeout */
          if (dhcp->tries > MAX_DHCP_TRIES)
          {
            DHCP_state = DHCP_TIMEOUT;

            /* Stop DHCP */
            dhcp_stop(netif);

            /* Static address used */
            IP_ADDR4(&ipaddr, ip_addr0 ,ip_addr1 , ip_addr2 , ip_addr3 );
            IP_ADDR4(&netmask, netmask_addr0, netmask_addr1, netmask_addr2, netmask_addr3);
            IP_ADDR4(&gw, gw_addr0, gw_addr1, gw_addr2, gw_addr3);
            netif_set_addr(netif, ip_2_ip4(&ipaddr), ip_2_ip4(&netmask), ip_2_ip4(&gw));

          }
        }
      }
      break;
  case DHCP_LINK_DOWN:
    {
      /* Stop DHCP */
      dhcp_stop(netif);
      DHCP_state = DHCP_OFF;
    }
    break;
    default: break;
    }

    /* wait 250 ms */
    KAL_Dly(250);
  }
}



