//                                                                ------------------------------------------------------------------------------
//                                                                 Copyright (c) Qualcomm Atheros, Inc.
//                                                                 All rights reserved.
//                                                                 Redistribution and use in source and binary forms, with or without modification, are permitted (subject to
//                                                                 the limitations in the disclaimer below) provided that the following conditions are met:
//
//                                                                 · Redistributions of source code must retain the above copyright notice, this list of conditions and the
//                                                                   following disclaimer.
//                                                                 · Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
//                                                                   following disclaimer in the documentation and/or other materials provided with the distribution.
//                                                                 · Neither the name of nor the names of its contributors may be used to endorse or promote products derived
//                                                                   from this software without specific prior written permission.
//
//                                                                 NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS
//                                                                 PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
//                                                                 BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//                                                                 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//                                                                 INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//                                                                 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
//                                                                 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//                                                                 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//                                                                ------------------------------------------------------------------------------
//                                                                ==============================================================================
//                                                                 Author(s): ="Atheros"
//                                                                ==============================================================================
#ifndef __COMMON_STACK_OFFLOAD_H__
#define __COMMON_STACK_OFFLOAD_H__

#if ENABLE_STACK_OFFLOAD

#include "atheros_stack_offload.h"

#define COMMAND_BLOCK_TIMEOUT       (960000)   //Socket will block for this period in msec, if no
                                               //                 response is received, socket will unblock
#define TRANSMIT_BLOCK_TIMEOUT      (900)  //Time in ms for which a send operation blocks,
//                                                                after this time, error is returned to application
#define LAST_FRAGMENT               (0x01)    //Indicates this is the last fragment
#define RX_DIRECTION                (0)
#define TX_DIRECTION                (1)
#define TCP_CONNECTION_AVAILABLE    (99)      //Sent by Target in Listen event that a new TCP
                                              //                   incoming connection is available. Application
                                              //                   should call accept on receving this from target.
#define DHCP_WAIT_TIME    60000   // Wait for 1 Minute. On time out configure Auto Address
#define TCP_MSS  1452
#define DHCP_AUTO_WAIT_TIME 60000 * 2 //wait for 2 minutes. 1 minute is for DHCP, 1 minute is for Auto IP
#if DEBUG
    #define DEBUG_PRINT(arg)        //User may define
#endif

#define TCP_FIN  1
#define TCP_LISTEN 2

enum SOCKET_CMDS{
  SOCK_OPEN = 0,    /*Open a socket*/
  SOCK_CLOSE,   /*Close existing socket*/
  SOCK_CONNECT,   /*Connect to a peer*/
  SOCK_BIND,    /*Bind to interface*/
  SOCK_LISTEN,    /*Listen on socket*/
  SOCK_ACCEPT,    /*Accept incoming connection*/
  SOCK_SELECT,    /*Wait for specified file descriptors*/
  SOCK_SETSOCKOPT,  /*Set specified socket option*/
  SOCK_GETSOCKOPT,  /*Get socket option*/
  SOCK_ERRNO,   /*Get error number for last error*/
  SOCK_IPCONFIG,          /*Set static IP information, or get current IP config*/
  SOCK_PING,
  SOCK_STACK_INIT,        /*Command to initialize stack*/
  SOCK_STACK_MISC,    /*Used to exchanges miscellaneous info, e.g. reassembly etc*/
  SOCK_PING6,
  SOCK_IP6CONFIG,          /*Set static IP information, or get current IP config*/
  SOCK_IPCONFIG_DHCP_POOL,          /*Set DHCP Pool  */
  SOCK_IP6CONFIG_ROUTER_PREFIX,    /* Set ipv6 router prefix */
  SOCK_IP_SET_TCP_EXP_BACKOFF_RETRY,  /* set tcp exponential backoff retry */
  SOCK_IP_SET_IP6_STATUS,  /* set ip6 module status enable/disable */
  SOCK_IP_DHCP_RELEASE, /* Release the DHCP IP Addres */
  SOCK_IP_SET_TCP_RX_BUF,  /* set tcp rx buffer space */
  SOCK_HTTP_SERVER,             /* Http Server Command*/
  SOCK_HTTP_SERVER_CMD,         /* Commands to get and post data */
  SOCK_DNC_CMD,         /* Commands related to resolver */
  SOCK_DNC_ENABLE,       /* Command to enable/disable DNS Client */
  SOCK_DNS_SRVR_CFG_ADDR,  /* Command to configure DNS Server Address */
  SOCK_HTTPC,               /* HTTP Client commands */
  SOCK_DNS_LOCAL_DOMAIN,   /* Configures the local domain */
  SOCK_IP_HOST_NAME,   /* Configures the local host name */
  SOCK_IP_DNS,              /* Configures DNS Database */
  SOCK_IP_SNTP_SRVR_ADDR,    /* Configures the sntp server addr */
  SOCK_IP_SNTP_GET_TIME,  /* GET UTC Time from SNTP Client */
  SOCK_IP_SNTP_GET_TIME_OF_DAY, /* Get time of day (secs)*/
  SOCK_IP_SNTP_CONFIG_TIMEZONE_DSE, /*Command to modify time zone and to enable/disable DSE */
  SOCK_IP_SNTP_QUERY_SNTP_ADDRESS, /* Command to query SNTP Server Address*/
  SOCK_IP_SNTP_CLIENT_ENABLE,         /* Command to enable/disable SNTP client */
  SOCK_IPV4_ROUTE,                    /* Commands to add,del and show IPv4 routes */
  SOCK_IPV6_ROUTE,                    /* Commands to add,del and show IPv6 routes */
  SOCK_IP_BRIDGEMODE,    /* Command to enable bridge mode */
  SOCK_DNS_SERVER_STATUS,   /*Command to enable/disable DNS Server*/
  SOCK_SSL_CTX_NEW,         /* Create a new SSL context */
  SOCK_SSL_CTX_FREE,        /* Free/close SSL context */
  SOCK_SSL_NEW,             /* Create new SSL connection object/instance */
  SOCK_SSL_SET_FD,          /* Add socket handle to a SSL connection */
  SOCK_SSL_ACCEPT,          /* Accept SSL connection request from SSL client */
  SOCK_SSL_CONNECT,         /* Establish SSL connection from SSL client to SSL server */
  SOCK_SSL_SHUTDOWN,        /* Shutdown/close SSL connection */
  SOCK_SSL_ADD_CERT,        /* Add a certificate to SSL context */
  SOCK_SSL_STORE_CERT,      /* Store a certificate or CA list file in FLASH */
  SOCK_SSL_LOAD_CERT,       /* Reads a certificate or CA list from FLASH and adds it to SSL context */
  SOCK_SSL_CONFIGURE,       /* Configure a SSL connection */
  SOCK_SSL_LIST_CERT,        /* Request the names of the cert's stored in FLASH */
  SOCK_GET_DEV_ID_FROM_DEST_IP,   /* Gets the device ID for the given destination IP address */
  SOCK_OTA_UPGRADE,           /* 54*/ /*Intializes OTA upgrade*/
  SOCK_OTA_READ,         /*55*//*Reads OTA Area after OTA download*/
  SOCK_OTA_DONE,         /*56*//*OTA download Complete Event*/
  SOCK_SET_DHCP_HOSTNAME,       /*57*/ /*set the DHCP Hostname*/
  SOCK_TCP_CONN_TIMEOUT,         /*58*//*TCP Connection Timeout */
  SOCK_HTTP_POST_EVENT,         /*59*/ /*HTTP server post event*/
  SOCK_OTA_SESSION_START,    /*60*/ /*start OTA session */
  SOCK_OTA_PARTITION_GET_SIZE,      /*61*/ /*start OTA session */
  SOCK_OTA_PARTITION_ERASE,      /*62*/ /*start OTA session */
  SOCK_OTA_PARSE_IMAGE_HDR,    /*63*/ /*parse OTA image header */
  SOCK_OTA_PARTITION_VERIFY_CHECKSUM,    /*64*/ /*veirfy OTA partition checksum */
  SOCK_OTA_PARTITION_WRITE_DATA,    /*65*/ /*write OTA partition data */
  SOCK_DHCPS_SUCCESS_CALLBACK,      /*66*/ /*DHCP Server callback event*/
  SOCK_DHCPC_SUCCESS_CALLBACK,    /*66*/ /*DHCP Client callback event*/
  /************************************/
  /* add new socket commands above this line */
  /************************************/
  SOCK_CMD_LAST   /* NOTE: ensure that this is the last entry in the enum */
};

/***socket context. This must be adjusted based on the underlying OS ***/
typedef struct socket_context {
  QOSAL_EVENT_STRUCT     sockRxWakeEvent;   //Event to unblock waiting RX socket
  QOSAL_EVENT_STRUCT     sockTxWakeEvent;   //Event to unblock waiting TX socket
  A_NETBUF_QUEUE_T   rxqueue;         //Queue to hold incoming packets
  QOSAL_UINT8        blockFlag;
  QOSAL_BOOL             respAvailable;     //Flag to indicate a response from target is available
  QOSAL_BOOL             txUnblocked;
  QOSAL_BOOL             txBlock;
  QOSAL_BOOL             rxBlock;
#if NON_BLOCKING_TX
  A_NETBUF_QUEUE_T   non_block_queue;   //Queue to hold packets to be freed later
  A_MUTEX_T          nb_tx_mutex;             //Mutex to synchronize access to non blocking queue
#endif
} SOCKET_CONTEXT, *SOCKET_CONTEXT_PTR;

#define GET_SOCKET_CONTEXT(ctxt) \
  ((SOCKET_CONTEXT_PTR)(((ATH_SOCKET_CONTEXT_PTR)ctxt)->sock_context))

typedef struct ath_socket_context {
  QOSAL_INT32    handle;          //Socket handle
  QOSAL_UINT32   sock_st_mask[SOCK_CMD_LAST / 32 + 1];
  QOSAL_INT32    result;          //API return value
  QOSAL_VOID*    sock_context;   //Pointer to custom socket context
  QOSAL_VOID*    pReq;                  //Used to hold wmi netbuf to be freed from the user thread
  QOSAL_UINT8*   data;          //Generic pointer to data recevied from target
  QOSAL_UINT8    domain;          //IPv4/v6
  QOSAL_UINT8    type;          //TCP vs UDP
  QOSAL_UINT16   remaining_bytes;
  QOSAL_VOID     *old_netbuf;
#if ENABLE_SSL
  SSL *ssl;    // SSL connection object
#endif
  QOSAL_UINT8    TCPCtrFlag;
} ATH_SOCKET_CONTEXT, *ATH_SOCKET_CONTEXT_PTR;

typedef struct ath_sock_stack_init {
  QOSAL_UINT8 stack_enabled;            //Flag to indicate if stack should be enabled in the target
  QOSAL_UINT8 num_sockets;              //number of sockets supported by the host
  QOSAL_UINT8 num_buffers;              //Number of RX buffers supported by host
  QOSAL_UINT8 reserved;
} ATH_STACK_INIT;

typedef PREPACK struct sock_open {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32 domain FIELD_PACKED;   //ATH_AF_INET, ATH_AF_INET6
  QOSAL_UINT32 type FIELD_PACKED;     //SOCK_STREAM_TYPE, SOCK_DGRAM_TYPE
  QOSAL_UINT32 protocol FIELD_PACKED;   // 0
}POSTPACK SOCK_OPEN_T;

typedef PREPACK struct sock_close {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32 handle FIELD_PACKED;   //socket handle
}POSTPACK SOCK_CLOSE_T;

typedef PREPACK struct sock_connect_cmd {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32 handle FIELD_PACKED;   //socket handle
  union {
    SOCKADDR_T name FIELD_PACKED;
    SOCKADDR_6_T name6 FIELD_PACKED;
  }addr;
  QOSAL_UINT16 length FIELD_PACKED;   //socket address length
}POSTPACK SOCK_CONNECT_CMD_T, SOCK_BIND_CMD_T, SOCK_ACCEPT_CMD_T;

typedef PREPACK struct sock_connect_recv {
  QOSAL_UINT32 handle FIELD_PACKED;     //socket handle
  union {
    SOCKADDR_T name FIELD_PACKED;
    SOCKADDR_6_T name6 FIELD_PACKED;
  }addr;
  QOSAL_UINT16 length FIELD_PACKED;   //socket address length
}POSTPACK SOCK_CONNECT_RECV_T, SOCK_BIND_RECV_T, SOCK_ACCEPT_RECV_T;

typedef PREPACK struct sock_errno {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32 errno;
}POSTPACK SOCK_ERRNO_T;

typedef struct sock_listen {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32 handle FIELD_PACKED;   //Socket handle
  QOSAL_UINT32 backlog FIELD_PACKED;    //Max length of queue of backlog connections
}POSTPACK SOCK_LISTEN_T;

typedef PREPACK struct sock_setopt{
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32 handle FIELD_PACKED;   //socket handle
  QOSAL_UINT32 level FIELD_PACKED;    //Option level (ATH_IPPROTO_IP, ATH_IPPROTO_UDP, ATH_IPPROTO_TCP...)
  QOSAL_UINT32 optname FIELD_PACKED;    //Choose from list above
  QOSAL_UINT32 optlen FIELD_PACKED;   // option of length
  QOSAL_UINT8  optval[1] FIELD_PACKED;  // option value
}POSTPACK SOCK_OPT_T;

typedef PREPACK struct ipconfig_recv {
  QOSAL_UINT32        mode FIELD_PACKED;        //0-query, 1-static, 2-dhcp
  QOSAL_UINT32        ipv4 FIELD_PACKED;          //IPv4 address
  QOSAL_UINT32        subnetMask FIELD_PACKED;
  QOSAL_UINT32        gateway4 FIELD_PACKED;
  IP6_ADDR_T      ipv6LinkAddr FIELD_PACKED;       /* IPv6 Link Local address */
  IP6_ADDR_T      ipv6GlobalAddr FIELD_PACKED;     /* IPv6 Global address */
  IP6_ADDR_T      ipv6DefGw FIELD_PACKED;           /* IPv6 Default Gateway */
  IP6_ADDR_T      ipv6LinkAddrExtd FIELD_PACKED;       /* IPv6 Link Local address for Logo*/
  QOSAL_INT32         LinkPrefix FIELD_PACKED;
  QOSAL_INT32         GlbPrefix FIELD_PACKED;
  QOSAL_INT32         DefGwPrefix FIELD_PACKED;
  QOSAL_INT32         GlbPrefixExtd FIELD_PACKED;
  IP46ADDR        dnsaddr[MAX_DNSADDRS] FIELD_PACKED;
  QOSAL_CHAR          hostname[33] FIELD_PACKED;
}POSTPACK IPCONFIG_RECV_T;

typedef PREPACK struct ipconfig {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32        mode FIELD_PACKED;        //0-query, 1-static, 2-dhcp
  QOSAL_UINT32        ipv4 FIELD_PACKED;          //IPv4 address
  QOSAL_UINT32        subnetMask FIELD_PACKED;
  QOSAL_UINT32        gateway4 FIELD_PACKED;
  IP6_ADDR_T      ipv6LinkAddr FIELD_PACKED;       /* IPv6 Link Local address */
  IP6_ADDR_T      ipv6GlobalAddr FIELD_PACKED;     /* IPv6 Global address */
  IP6_ADDR_T      ipv6DefGw FIELD_PACKED;           /* IPv6 Default Gateway */
  IP6_ADDR_T      ipv6LinkAddrExtd FIELD_PACKED;       /* IPv6 Link Local address for Logo*/
  QOSAL_INT32         LinkPrefix;
  QOSAL_INT32         GlbPrefix;
  QOSAL_INT32         DefGwPrefix;
  QOSAL_INT32         GlbPrefixExtd;
}POSTPACK IPCONFIG_CMD_T;

typedef PREPACK struct ping {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32 ip_addr FIELD_PACKED;        //Destination IPv4 address
  QOSAL_UINT32 size FIELD_PACKED;
}POSTPACK PING_T;

typedef PREPACK struct ping6 {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT8 ip6addr[16] FIELD_PACKED;     //Destination IPv6 address
  QOSAL_UINT32 size FIELD_PACKED;
}POSTPACK PING_6_T;

typedef PREPACK struct ipconfigdhcppool {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32   startaddr FIELD_PACKED;
  QOSAL_UINT32   endaddr FIELD_PACKED;
  QOSAL_INT32   leasetime FIELD_PACKED;
}POSTPACK IPCONFIG_DHCP_POOL_T;

typedef PREPACK struct  ip6config_router_prefix  {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT8  v6addr[16]  FIELD_PACKED;
  QOSAL_INT32  prefixlen  FIELD_PACKED;
  QOSAL_INT32  prefix_lifetime FIELD_PACKED;
  QOSAL_INT32  valid_lifetime FIELD_PACKED;
}POSTPACK IP6CONFIG_ROUTER_PREFIX_T;

typedef PREPACK struct  sock_ip_backoff  {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_INT32  max_retry FIELD_PACKED;
}POSTPACK SOCK_IP_BACKOFF_T;

typedef PREPACK struct sock_ipv6_status {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT16 ipv6_status FIELD_PACKED;
}POSTPACK SOCK_IPv6_STATUS_T;

typedef PREPACK struct sock_ip_bridgemode {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT16 bridgemode FIELD_PACKED;
}POSTPACK SOCK_IP_BRIDGEMODE_T;

typedef PREPACK struct sock_ip_dhcp_release {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT16  ifIndex FIELD_PACKED;
}POSTPACK SOCK_IP_DHCP_RELEASE_T;

typedef PREPACK struct  sock_ip_tcp_rx_buf  {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_INT32  rxbuf FIELD_PACKED;
}POSTPACK SOCK_IP_TCP_RX_BUF_T;

typedef PREPACK struct  sock_ip_http_server  {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_INT32        enable FIELD_PACKED;
}POSTPACK SOCK_IP_HTTP_SERVER_T;

typedef PREPACK struct  sock_ip_http_server_cmd  {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_INT32   command FIELD_PACKED;
  QOSAL_UINT8    pagename[32] FIELD_PACKED;
  QOSAL_UINT8    objname[32] FIELD_PACKED;
  QOSAL_UINT32   objlen FIELD_PACKED;
  QOSAL_UINT8    value[32] FIELD_PACKED;
}POSTPACK SOCK_IP_HTTP_SERVER_CMD_T;

typedef PREPACK struct  sock_ip_dns_client  {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_INT32  command FIELD_PACKED;
}POSTPACK SOCK_IP_DNS_CLIENT_T;

typedef PREPACK struct sock_ip_dns_config_server_addr {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  IP46ADDR addr FIELD_PACKED;       //Server address
}POSTPACK SOCK_IP_CFG_DNS_SRVR_ADDR;

typedef PREPACK struct sock_ip_dns_local_domain {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_CHAR domain_name[33] FIELD_PACKED;        //Server address
}POSTPACK SOCK_IP_CFG_DNS_LOCAL_DOMAIN;

typedef PREPACK struct sock_ip_dns_hostname {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_CHAR domain_name[33] FIELD_PACKED;        //Server address
}POSTPACK SOCK_IP_CFG_HOST_NAME;

typedef PREPACK struct sock_ip_dns {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_INT32 command FIELD_PACKED;
  QOSAL_CHAR domain_name[36] FIELD_PACKED;        //Server address
  IP46ADDR addr FIELD_PACKED;
}POSTPACK SOCK_IP_DNS_T;

typedef PREPACK struct  sock_ip_dns_server  {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_INT32        enable FIELD_PACKED;
}POSTPACK SOCK_IP_DNS_SERVER_STATUS_T;

#if ENABLE_HTTP_CLIENT
typedef PREPACK struct httpc_command {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32  command FIELD_PACKED;
  QOSAL_UINT8   url[256] FIELD_PACKED;
  QOSAL_UINT8   data[128] FIELD_PACKED;
}POSTPACK SOCK_HTTPC_T;
#endif /* ENABLE_HTTP_CLIENT */

typedef PREPACK struct sock_ip_sntp_local_domain {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_INT32 command FIELD_PACKED;
  QOSAL_CHAR addr[68] FIELD_PACKED;       //Server address
}POSTPACK SOCK_IP_CFG_SNTP_SRVR_ADDR;

typedef PREPACK struct sock_ip_sntp_zone{
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT8 hour FIELD_PACKED;
  QOSAL_UINT8 min FIELD_PACKED;
  QOSAL_UINT8 add_sub FIELD_PACKED;   // add=1,sub=0
  QOSAL_UINT8 dse FIELD_PACKED;  //enable/disable day light saving
}POSTPACK SOCK_SNTP_MODIFY_TIMEZONE;

typedef PREPACK struct sock_ip_sntp_config{
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  tSntpDnsAddr SntpDnsAddr[MAX_SNTP_SERVERS] FIELD_PACKED;
}POSTPACK SOCK_IP_QUERY_SNTP_CONFIG;

typedef PREPACK struct sDestIpToDevMapEvtType{
  QOSAL_INT8      result;
  QOSAL_UINT8     domain;
  QOSAL_INT16     device_id;
  union {
    QOSAL_UINT32   ip_addr FIELD_PACKED;
    IP6_ADDR_T ip6_addr FIELD_PACKED;
  }dev_addr;
  union {
    QOSAL_UINT32   ip_addr FIELD_PACKED;
    IP6_ADDR_T ip6_addr FIELD_PACKED;
  }dest_addr;
} POSTPACK tDestIpToDevMapEvtType;

typedef PREPACK struct sock_destip_dev_map_evt_type{
  WMI_SOCKET_CMD         wmi_cmd FIELD_PACKED;
  tDestIpToDevMapEvtType ipToDevMap FIELD_PACKED;
}POSTPACK WMI_SOCK_DESTIP_TO_DEV_MAP_EVENT;

typedef PREPACK struct  sock_ip_sntp_client  {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_INT32  command FIELD_PACKED;
}POSTPACK SOCK_IP_SNTP_CLIENT_T;

typedef PREPACK struct sock_ipv4_route {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_INT32  command FIELD_PACKED;
  QOSAL_UINT32 address FIELD_PACKED;
  QOSAL_UINT32 mask FIELD_PACKED;
  QOSAL_UINT32 gateway FIELD_PACKED;
  QOSAL_UINT32 ifIndex FIELD_PACKED;
  QOSAL_UINT32 prot FIELD_PACKED;
} POSTPACK SOCK_IPV4_ROUTE_T;

typedef PREPACK struct sock_ipv6_route {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32 command FIELD_PACKED;
  QOSAL_UINT8  address[16] FIELD_PACKED;
  QOSAL_INT32  prefixLen FIELD_PACKED;
  QOSAL_UINT8  nexthop[16] FIELD_PACKED;
  QOSAL_UINT32 ifIndex FIELD_PACKED;
  QOSAL_UINT32 prot FIELD_PACKED;
}POSTPACK SOCK_IPV6_ROUTE_T;

typedef PREPACK struct sock_tcp_conn_timeout{
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32 timeout_val;
}POSTPACK SOCK_TCP_CONN_TIMEOUT_T;

typedef PREPACK struct sock_ota_upgrade {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32 ipaddress FIELD_PACKED;
  QOSAL_CHAR   filename[256] FIELD_PACKED;
  QOSAL_UINT8  mode FIELD_PACKED;
  QOSAL_UINT8  preserve_last FIELD_PACKED;
  QOSAL_UINT8  protocol FIELD_PACKED;
}POSTPACK SOCK_OTA_UPGRADE_T;

typedef PREPACK struct sock_ota_read_ota_area{
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32 offset;
  QOSAL_UINT32 size;
}POSTPACK SOCK_OTA_READ_OTA_AREA_T;

typedef PREPACK struct sock_ota_done {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32 data;
}POSTPACK SOCK_OTA_DONE_T;

typedef PREPACK struct sock_ota_session_start_s{
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32 flags;
  QOSAL_UINT32 partition_index;
}POSTPACK SOCK_OTA_SESSION_START_T;

typedef PREPACK struct sock_ota_parse_image_hdr_s{
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT8 header[40];
}POSTPACK SOCK_OTA_PARSE_IMAGE_HDR_T;

typedef PREPACK struct sock_ota_partion_write_data_s{
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32 offset;
  QOSAL_UINT32 size;
  QOSAL_UINT8  data[1];
}POSTPACK SOCK_OTA_PARTITON_WRITE_DATA_T;

typedef PREPACK struct dhcps_success_cb_resp{
  QOSAL_UINT32 resp_code;
  QOSAL_UINT8  mac[16];
  QOSAL_UINT32 ip;
}POSTPACK SOCK_DHCPS_SUCCESS_CALLBACK_T;

typedef PREPACK struct dhcpc_success_cb_resp{
  QOSAL_UINT32 resp_code;
  QOSAL_UINT32 ip;
  QOSAL_UINT32 mask;
  QOSAL_UINT32 gw;
}POSTPACK SOCK_DHCPC_SUCCESS_CALLBACK_T;

/*This structure is sent to the target in a data packet.
   It allows the target to route the data to correct socket with
   all the necessary parameters*/
typedef PREPACK struct sock_send{
  QOSAL_UINT32 handle FIELD_PACKED;     //Socket handle
  QOSAL_UINT16 length FIELD_PACKED;     //Payload length
  QOSAL_UINT16 reserved FIELD_PACKED;     //Reserved
  QOSAL_UINT32 flags FIELD_PACKED;      //Send flags
  SOCKADDR_T name FIELD_PACKED;       //IPv4 destination socket information
  QOSAL_UINT16 socklength FIELD_PACKED;
}POSTPACK SOCK_SEND_T;

typedef PREPACK struct sock_send6{
  QOSAL_UINT32 handle FIELD_PACKED;     //Socket handle
  QOSAL_UINT16 length FIELD_PACKED;     //Payload length
  QOSAL_UINT16 reserved FIELD_PACKED;     //Reserved
  QOSAL_UINT32 flags FIELD_PACKED;      //Send flags
  SOCKADDR_6_T name6 FIELD_PACKED;    //IPv6 destination socket information
  QOSAL_UINT16 socklength FIELD_PACKED;
}POSTPACK SOCK_SEND6_T;

typedef PREPACK struct sock_recv{
  QOSAL_UINT32 handle FIELD_PACKED;     //Socket handle
  SOCKADDR_T name FIELD_PACKED;         //IPv4 destination socket information
  QOSAL_UINT16 socklength FIELD_PACKED;   // Length of sockaddr structure
  QOSAL_UINT16 reserved FIELD_PACKED;       // Length of sockaddr structure
  QOSAL_UINT32 reassembly_info FIELD_PACKED;   //Placeholder for reassembly info
}POSTPACK SOCK_RECV_T;

typedef PREPACK struct sock_recv6{
  QOSAL_UINT32 handle FIELD_PACKED;     //Socket handle
  SOCKADDR_6_T name6 FIELD_PACKED;      //IPv6 destination socket information
  QOSAL_UINT16 socklength FIELD_PACKED;
  QOSAL_UINT16 reserved FIELD_PACKED;     //Reserved
  QOSAL_UINT32 reassembly_info FIELD_PACKED;  //Placeholder for reassembly info
}POSTPACK SOCK_RECV6_T;

#if ENABLE_SSL
typedef PREPACK struct sock_ssl_new {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32 ctx FIELD_PACKED;    // SSL context
}POSTPACK SOCK_SSL_NEW_T;

typedef PREPACK struct sock_ssl_set_fd {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32 ssl FIELD_PACKED;    // SSL connection object
  QOSAL_UINT32 fd FIELD_PACKED;     // Socket handle
}POSTPACK SOCK_SSL_SET_FD_T;

typedef PREPACK struct sock_ssl_accept {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32 ssl FIELD_PACKED;    // SSL connection object
}POSTPACK SOCK_SSL_ACCEPT_T;

typedef PREPACK struct sock_ssl_connect {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32 ssl FIELD_PACKED;    // SSL connection object
}POSTPACK SOCK_SSL_CONNECT_T;

typedef PREPACK struct sock_ssl_shutdown {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32 ssl FIELD_PACKED;    // SSL connection object
}POSTPACK SOCK_SSL_SHUTDOWN_T;

typedef PREPACK struct sock_ssl_configure {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32 ssl FIELD_PACKED;       // SSL connection object
  QOSAL_UINT32 size FIELD_PACKED;      // the size of the configuration data
  QOSAL_UINT8  data[sizeof(SSL_CONFIG)] FIELD_PACKED;   // The configuration data
}POSTPACK SOCK_SSL_CONFIGURE_T;

typedef PREPACK struct sock_ssl_ctx_new {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32 role FIELD_PACKED;
  QOSAL_INT32 inbufSize FIELD_PACKED;
  QOSAL_INT32 outbufSize FIELD_PACKED;
  QOSAL_INT32 reserved FIELD_PACKED;
}POSTPACK SOCK_SSL_CTX_NEW_T;

typedef PREPACK struct sock_ssl_ctx_free {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32 ctx FIELD_PACKED;    // SSL context
}POSTPACK SOCK_SSL_CTX_FREE_T;

typedef PREPACK struct sock_ssl_add_cert {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32 ctx FIELD_PACKED;      // SSL context
  QOSAL_UINT32 type FIELD_PACKED;     // Type: 1 for device certificate, 2: CA list
  QOSAL_UINT32 total FIELD_PACKED;    // The size of ca_list
  QOSAL_UINT32 offset FIELD_PACKED;   // offset of this fragment
  QOSAL_UINT32 size FIELD_PACKED;     // fragment size
  QOSAL_UINT8 data[1] FIELD_PACKED;   // CA list or certificate data
}POSTPACK SOCK_SSL_ADD_CERT_T;

typedef PREPACK struct sock_ssl_store_cert {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT8  name[32] FIELD_PACKED;   // The name of the certificate or CA list
  QOSAL_UINT32 total FIELD_PACKED;      // The size of the certificate or CA list
  QOSAL_UINT32 offset FIELD_PACKED;     // offset of this fragment
  QOSAL_UINT32 size FIELD_PACKED;       // fragment size
  QOSAL_UINT8 data[1] FIELD_PACKED;     // certificate or CA list data
}POSTPACK SOCK_SSL_STORE_CERT_T;

typedef PREPACK struct sock_ssl_load_cert {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32 ctx FIELD_PACKED;        // SSL context
  QOSAL_UINT32 type FIELD_PACKED;       // Type: 1 for device certificate, 2: CA list
  QOSAL_UINT8  name[32] FIELD_PACKED;   // The name of the certificate or CA list
}POSTPACK SOCK_SSL_LOAD_CERT_T;

typedef PREPACK struct sock_ssl_list_cert {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32 reserved FIELD_PACKED;
}POSTPACK SOCK_SSL_LIST_CERT_T;

#endif

typedef PREPACK struct sock_get_dev_from_dest_ip {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32 handle FIELD_PACKED;     //socket handle
  union {
    QOSAL_UINT32   ipaddr FIELD_PACKED;
    IP6_ADDR_T ip6addr FIELD_PACKED;
  }addr;
  QOSAL_UINT8 domain FIELD_PACKED;      //socket domain
}POSTPACK SOCK_GET_DEV_FROM_DEST_T;

#define SOCK_EV_MASK_SET(_pcxt, _cmd)  (_pcxt)->sock_st_mask[(_cmd) >> 5] |= (QOSAL_UINT32) 1 << ((_cmd) & 0x1f)

#define SOCK_EV_MASK_CLEAR(_pcxt, _cmd) (_pcxt)->sock_st_mask[(_cmd) >> 5] &= ~(1 << ((_cmd) & 0x1f))

#define SOCK_EV_MASK_TEST(_pcxt, _cmd) (((_pcxt)->sock_st_mask[(_cmd) >> 5] & (QOSAL_UINT32)(1 << ((_cmd) & 0x1f))) ? 1 : 0)

extern ATH_SOCKET_CONTEXT* ath_sock_context[];

/************************** Internal Function declarations*****************************/

QOSAL_UINT32 queue_empty(QOSAL_UINT32 index);

#define QUEUE_EMPTY(index) \
  queue_empty((index))

#define BLOCK_FOR_DATA(pCxt, ctxt, msec, direction) \
  blockForDataRx((pCxt), (ctxt), (msec), (direction))

/******* Function Declarations *******************/
A_STATUS unblock(QOSAL_VOID* ctxt, QOSAL_UINT8 direction);
A_STATUS blockForResponse(QOSAL_VOID* pCxt, QOSAL_VOID* ctxt, QOSAL_UINT32 msec, QOSAL_UINT8 direction);
A_STATUS blockForDataRx(QOSAL_VOID* pCxt, QOSAL_VOID* ctxt, QOSAL_UINT32 msec, QOSAL_UINT8 direction);

#define BLOCK(pCxt, ctxt, msec, direction) \
  blockForResponse((pCxt), (ctxt), (msec), (direction))

#define UNBLOCK(ctxt, direction) \
  unblock((ctxt), (direction))

#if T_SELECT_VER1
A_STATUS unblockSelect(QOSAL_VOID* ctxt);
A_STATUS blockSelect(QOSAL_VOID* pCxt, QOSAL_UINT32 msec);
#endif

#if T_SELECT_VER1

#define BLOCK_SELECT(pCxt, msec) \
  blockSelect((pCxt), (msec))

#define UNBLOCK_SELECT(pCxt) \
  unblockSelect((pCxt))
#else

#define BLOCK_SELECT(pCxt, msec)

#define UNBLOCK_SELECT(pCxt)

#endif //T_SELECT_VER1

A_STATUS socket_context_init();
QOSAL_INT32 find_socket_context(QOSAL_UINT32 handle, QOSAL_UINT8 retrieve);
QOSAL_UINT32 getTransportLength(QOSAL_UINT8 proto);
QOSAL_UINT32 getIPLength(QOSAL_UINT8 version);
A_STATUS move_power_state_to_maxperf(void *pDCxt, QOSAL_INT32 module);
A_STATUS restore_power_state(void *pDCxt, QOSAL_INT32 module);

/************************* Socket APIs *************************************************/
QOSAL_INT32 Api_socket(QOSAL_VOID *pCxt, QOSAL_UINT32 domain, QOSAL_UINT32 type, QOSAL_UINT32 protocol);
QOSAL_INT32 Api_shutdown(QOSAL_VOID *pCxt, QOSAL_UINT32 handle);
QOSAL_INT32 Api_connect(QOSAL_VOID *pCxt, QOSAL_UINT32 handle, QOSAL_VOID* name, QOSAL_UINT16 length);
QOSAL_INT32 Api_bind(QOSAL_VOID *pCxt, QOSAL_UINT32 handle, QOSAL_VOID* name, QOSAL_UINT16 length);
QOSAL_INT32 Api_listen(QOSAL_VOID *pCxt, QOSAL_UINT32 handle, QOSAL_UINT32 backlog);
QOSAL_INT32 Api_accept(QOSAL_VOID *pCxt, QOSAL_UINT32 handle, QOSAL_VOID* name, QOSAL_UINT16 length);

#if T_SELECT_VER1
QOSAL_INT32 Api_accept_ver1(QOSAL_VOID *pCxt, QOSAL_UINT32 handle, QOSAL_VOID* name, QOSAL_UINT16 length);
QOSAL_INT32 Api_select_ver1(QOSAL_VOID* pCxt, QOSAL_INT32 num, QOSAL_UINT32 *r_fd, QOSAL_UINT32 *w_fd, QOSAL_UINT32 *e_fd, QOSAL_UINT32 tv);
#endif

QOSAL_INT32 Api_sendto(QOSAL_VOID *pCxt, QOSAL_UINT32 handle, QOSAL_UINT8* buffer, QOSAL_UINT32 length, QOSAL_UINT32 flags, SOCKADDR_T* name, QOSAL_UINT32 socklength);
QOSAL_INT32 Api_select(QOSAL_VOID *pCxt, QOSAL_UINT32 handle, QOSAL_UINT32 tv);
QOSAL_INT32 Api_errno(QOSAL_VOID *pCxt, QOSAL_UINT32 handle);
QOSAL_INT32 Api_getsockopt(QOSAL_VOID *pCxt, QOSAL_UINT32 handle, QOSAL_UINT32 level, QOSAL_UINT32 optname, QOSAL_UINT8* optval, QOSAL_UINT32 optlen);
QOSAL_INT32 Api_setsockopt(QOSAL_VOID *pCxt, QOSAL_UINT32 handle, QOSAL_UINT32 level, QOSAL_UINT32 optname, QOSAL_UINT8* optval, QOSAL_UINT32 optlen);
#if ZERO_COPY
QOSAL_INT32 Api_recvfrom(QOSAL_VOID *pCxt, QOSAL_UINT32 handle, void** buffer, QOSAL_UINT32 length, QOSAL_UINT32 flags, QOSAL_VOID* name, QOSAL_UINT32* socklength);
#else
QOSAL_INT32 Api_recvfrom(QOSAL_VOID *pCxt, QOSAL_UINT32 handle, void* buffer, QOSAL_UINT32 length, QOSAL_UINT32 flags, QOSAL_VOID* name, QOSAL_UINT32* socklength);
#endif
QOSAL_INT32 Api_ipconfig(QOSAL_VOID *pCxt, QOSAL_UINT32 mode, QOSAL_UINT32* ipv4_addr, QOSAL_UINT32* subnetMask, QOSAL_UINT32* gateway4, IP46ADDR *dnsaddr, QOSAL_CHAR *hostname);
QOSAL_VOID clear_socket_context(QOSAL_INT32 index);
QOSAL_INT32 Api_ping(QOSAL_VOID *pCxt, QOSAL_UINT32 ipv4_addr, QOSAL_UINT32 size);
QOSAL_INT32 Api_ping6(QOSAL_VOID *pCxt, QOSAL_UINT8 *ip6addr, QOSAL_UINT32 size);
QOSAL_INT32 Api_ip6config(QOSAL_VOID *pCxt, QOSAL_UINT32 mode, IP6_ADDR_T *v6Global, IP6_ADDR_T *v6Link, IP6_ADDR_T *v6DefGw, IP6_ADDR_T *v6GlobalExtd, QOSAL_INT32 *LinkPrefix,
                          QOSAL_INT32 *GlbPrefix, QOSAL_INT32 *DefgwPrefix, QOSAL_INT32 *GlbPrefixExtd);
A_STATUS send_stack_init(QOSAL_VOID* pCxt);
QOSAL_VOID socket_context_deinit();
QOSAL_INT32 Api_ipconfig_dhcp_pool(QOSAL_VOID *pCxt, QOSAL_UINT32* start_ipv4_addr, QOSAL_UINT32* end_ipv4_addr, QOSAL_INT32 leasetime);
QOSAL_INT32 Api_ip6config_router_prefix(QOSAL_VOID *pCxt, QOSAL_UINT8 *v6addr, QOSAL_INT32 prefixlen, QOSAL_INT32 prefix_lifetime, QOSAL_INT32 valid_lifetime);
QOSAL_INT32 Api_ipconfig_set_tcp_exponential_backoff_retry(QOSAL_VOID *pCxt, QOSAL_INT32 retry);
QOSAL_INT32 Api_ipconfig_set_ip6_status(QOSAL_VOID *pCxt, QOSAL_UINT16 status);
QOSAL_INT32 Api_ipconfig_dhcp_release(QOSAL_VOID *pCxt);
QOSAL_INT32 Api_ipbridgemode(void* handle, QOSAL_UINT16 status);
QOSAL_INT32 Api_ipconfig_set_tcp_rx_buffer(QOSAL_VOID *pCxt, QOSAL_INT32 rxbuf);
QOSAL_INT32 Api_ip_http_server(QOSAL_VOID *pCxt, QOSAL_INT32 command);
QOSAL_INT32 Api_ip_http_server_method(QOSAL_VOID *pCxt, QOSAL_INT32 command, QOSAL_UINT8 *pagename, QOSAL_UINT8 *objname, QOSAL_INT32 objtype, QOSAL_INT32 objlen, QOSAL_UINT8 * value);
QOSAL_INT32 Api_httpc_method(QOSAL_VOID* pCxt, QOSAL_UINT32 command, QOSAL_UINT8 *url, QOSAL_UINT8 *data, QOSAL_UINT8 **output);
QOSAL_INT32 Api_ip_resolve_host_name(QOSAL_VOID *pCxt, DNC_CFG_CMD *DncCfg, DNC_RESP_INFO *DncRespInfo);
QOSAL_INT32 Api_ip_dns_client(QOSAL_VOID *pCxt, QOSAL_INT32 command);
QOSAL_INT32 Api_ip_dns_server_addr(QOSAL_VOID *pCxt, IP46ADDR *addr);
QOSAL_INT32 Api_ip_dns_local_domain(QOSAL_VOID *pCxt, QOSAL_CHAR *domain_name);
QOSAL_INT32 Api_ip_hostname(QOSAL_VOID *pCxt, QOSAL_CHAR *domain_name);
QOSAL_INT32 Api_ipdns(QOSAL_VOID *pCxt, QOSAL_INT32 command, QOSAL_CHAR *domain_name, IP46ADDR *dnsaddr);
QOSAL_INT32 Api_ip_sntp_srvr_addr(QOSAL_VOID *pCxt, QOSAL_INT32 command, QOSAL_CHAR *sntp_srvr_addr);
QOSAL_INT32 Api_ip_sntp_get_time(QOSAL_VOID *pCxt, tSntpTime *SntpTime);
QOSAL_INT32 Api_ip_sntp_get_time_of_day(QOSAL_VOID *pCxt, tSntpTM *SntpTm);
QOSAL_INT32 Api_ip_sntp_modify_zone_dse(QOSAL_VOID *pCxt, QOSAL_UINT8 hr, QOSAL_UINT8 mn, QOSAL_UINT8 zone_cal, QOSAL_UINT8 dse_en_dis);
QOSAL_INT32 Api_ip_sntp_query_srvr_address(QOSAL_VOID *pcxt, tSntpDnsAddr SntpDnsAddr[MAX_SNTP_SERVERS]);
QOSAL_INT32 Api_ip_sntp_client(QOSAL_VOID *pCxt, QOSAL_INT32 command);

QOSAL_INT32 Api_tcp_connection_timeout(QOSAL_VOID *pCxt, QOSAL_UINT32 timeout_val);

QOSAL_INT32 Api_ota_upgrade(QOSAL_VOID *pCxt, QOSAL_UINT32 addr, QOSAL_CHAR *filename, QOSAL_UINT8 mode, QOSAL_UINT8 preserve_last, QOSAL_UINT8 protocol, QOSAL_UINT32 * resp_code, QOSAL_UINT32 *length);
QOSAL_INT32 Api_ota_read_area(QOSAL_VOID *pCxt, QOSAL_UINT32 offset, QOSAL_UINT32 size, QOSAL_UCHAR *buffer, QOSAL_UINT32 *retlen);
QOSAL_INT32 Api_ota_done(QOSAL_VOID *pCxt, QOSAL_BOOL good_image);
QOSAL_INT32  Api_ota_session_start(QOSAL_VOID *pCxt, QOSAL_UINT32 flags, QOSAL_UINT32 partition_index);
QOSAL_UINT32 Api_ota_partition_get_size(QOSAL_VOID *pCxt);
QOSAL_INT32  Api_ota_partition_erase(QOSAL_VOID *pCxthandle);
QOSAL_INT32  Api_ota_partition_verify_checksum(QOSAL_VOID *pCxthandle);
QOSAL_INT32  Api_ota_parse_image_hdr(QOSAL_VOID *pCxt, QOSAL_UINT8 *header, QOSAL_UINT32 *offset);
QOSAL_INT32 Api_ota_partition_write_data(QOSAL_VOID *pCxt, QOSAL_UINT32 offset, QOSAL_UINT8 *buf, QOSAL_UINT32 size, QOSAL_UINT32 *ret_size);

#if ENABLE_ROUTING_CMDS
QOSAL_INT32 Api_ipv4_route(QOSAL_VOID *pCxt, QOSAL_UINT32 command, QOSAL_UINT32* ipv4_addr, QOSAL_UINT32* subnetMask, QOSAL_UINT32* gateway, QOSAL_UINT32* ifIndex, IPV4_ROUTE_LIST_T *routelist);
QOSAL_INT32 Api_ipv6_route(QOSAL_VOID *pCxt, QOSAL_UINT32 command, QOSAL_UINT8* ipv6_addr, QOSAL_UINT32* prefixLen, QOSAL_UINT8* gateway, QOSAL_UINT32* ifIndex, IPV6_ROUTE_LIST_T *routelist);
#endif

QOSAL_INT32 Api_ip_dns_server(QOSAL_VOID *pCxt, QOSAL_INT32 command);
#if ENABLE_SSL
SSL_CTX* Api_SSL_ctx_new(QOSAL_VOID *pCxt, SSL_ROLE_T role, QOSAL_INT32 inbufSize, QOSAL_INT32 outbufSize, QOSAL_INT32 reserved);
QOSAL_INT32 Api_SSL_ctx_free(QOSAL_VOID *pCxt, SSL_CTX *ctx);
SSL* Api_SSL_new(QOSAL_VOID *pCxt, SSL_CTX *ctx);
QOSAL_INT32 Api_SSL_set_fd(QOSAL_VOID *pCxt, SSL *ssl, QOSAL_UINT32 fd);
QOSAL_INT32 Api_SSL_accept(QOSAL_VOID *pCxt, SSL *ssl);
QOSAL_INT32 Api_SSL_connect(QOSAL_VOID *pCxt, SSL *ssl);
QOSAL_INT32 Api_SSL_shutdown(QOSAL_VOID *pCxt, SSL *ssl);
QOSAL_INT32 Api_SSL_configure(QOSAL_VOID *pCxt, SSL *ssl, SSL_CONFIG *cfg);
QOSAL_INT32 Api_SSL_addCert(QOSAL_VOID *pCxt, SSL_CTX *ctx, SSL_CERT_TYPE_T type, QOSAL_UINT8 *cert, QOSAL_UINT32 size);
QOSAL_INT32 Api_SSL_storeCert(QOSAL_VOID *pCxt, QOSAL_CHAR *name, QOSAL_UINT8 *cert, QOSAL_UINT32 size);
QOSAL_INT32 Api_SSL_loadCert(QOSAL_VOID *pCxt, SSL_CTX *ctx, SSL_CERT_TYPE_T type, QOSAL_CHAR *name);
QOSAL_INT32 Api_SSL_listCert(QOSAL_VOID *pCxt, SSL_FILE_NAME_LIST *fileNames);
#endif
#endif //ENABLE_STACK_OFFLOAD
#endif
