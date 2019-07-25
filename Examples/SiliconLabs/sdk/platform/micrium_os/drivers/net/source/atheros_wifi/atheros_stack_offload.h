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
#ifndef __ATHEROS_STACK_OFFLOAD_H__
#define __ATHEROS_STACK_OFFLOAD_H__
#include "a_config.h"
#include <a_config.h>
#include <a_types.h>
#include <a_osapi.h>
#include <driver_cxt.h>
#include <common_api.h>
#include <custom_wlan_api.h>
#include <wmi_api.h>
#include <htc.h>

#if ENABLE_STACK_OFFLOAD

/**********************Macros and config parameters**********/
#define MAX_SOCKETS_SUPPORTED      (8)
#define SOCKET_NOT_FOUND           (-1)
#define EMPTY_SOCKET_CONTEXT       (0)
#define SOCKET_HANDLE_PLACEHOLDER  (0xFFFFFFFF)
#define ATH_AF_INET                (2)
#define ATH_PF_INET                (ATH_AF_INET)
#define ATH_AF_INET6               (3)
#define ATH_PF_INET6               (ATH_AF_INET6)
#define IPV4_HEADER_LENGTH       (20)
#define IPV6_HEADER_LENGTH       (40)
#define TCP_HEADER_LENGTH          (20)
#define UDP_HEADER_LENGTH          (8)
#define IPV6_FRAGMENTATION_THRESHOLD   (1280)
#define IPV4_FRAGMENTATION_THRESHOLD   (AR4100_BUFFER_SIZE - TCP6_HEADROOM)
#define MAX_ETHERNET_FRAME_SIZE    (1500)

#define GLOBAL_SOCK_INDEX          (MAX_SOCKETS_SUPPORTED)        //Index used for global commands e.g. ipconfig, ping

/*The following macro enables the zero copy feature. In memory-constrained systems,
   the application will provide a pointer instead of an allocated buffer. The driver
   will return the pointer to received packet instead of copying the packet over.
   The application must call zero_copy_free() API after it is done with the buffer
   and pass the pointer to buffer.*/
#define ZERO_COPY                     1

/* NON_BLOCKING_TX- Macro used to control SEND behavior.
   SECTION 1- Macro is disabled.
    If this macro is disabled, application thread will block until a packet is
    sent over SPI. This is not desirable in a single buffer scenario as it may
    lead to deadlocks.
   SECTION 2- Macro is enabled.
   If this macro is enabled, the application thread will return after submitting
   packet to the driver thread. Under this setting, the application MUST NOT
   TRY TO REUSE OR FREE THIS BUFFER. This buffer is now owned by the driver.
   The application should call custom_alloc again to get a new buffer. */
#define NON_BLOCKING_TX            0

/* There may be scenarios where application does not wish to block on a receive operation.
   This macro will enable non blocking receive behavior. Note that this change is only
   limited to the host and does not affect target behavior.*/
#define NON_BLOCKING_RX            0

/* If the host has only 1 RX buffer, receive operation can never be blocking, as
   this may lead to deadlocks.*/
#if WLAN_CONFIG_NUM_PRE_ALLOC_RX_BUFFERS < 2

#if NON_BLOCKING_RX == 0
#error "Blocking receive not permitted with single Rx buffer. Enable NON_BLOCKING_RX to remove this warning."
#endif

#if NON_BLOCKING_TX == 0
#error "Blocking transmit not permitted with single Rx buffer. Enable NON_BLOCKING_TX to remove this warning."
#endif

#endif

/* BSD Sockets errors */
#define ENOBUFS         1
#define ETIMEDOUT       2
#define EISCONN         3
#define EOPNOTSUPP      4
#define ECONNABORTED    5
#define EWOULDBLOCK     6
#define ECONNREFUSED    7
#define ECONNRESET      8
#define ENOTCONN        9
#define EALREADY        10
#define EINVAL          11
#define EMSGSIZE        12
#define EPIPE           13
#define EDESTADDRREQ    14
#define ESHUTDOWN       15
#define ENOPROTOOPT     16
#define EHAVEOOB        17
#define ENOMEM          18
#define EADDRNOTAVAIL   19
#define EADDRINUSE      20
#define EAFNOSUPPORT    21

/* Generic/TCP socket options          */

#define SO_KEEPALIVE       0x0008    /* keep connections alive    */
#define SO_MAXMSG          0x1010    /* get TCP_MSS (max segment size) */
#define SO_LINGER          0x0080    /* linger on close if data present    */

/* TCP User-settable options (used with setsockopt).   */
#define TCP_MAXSEG         0x2003    /* set maximum segment size    */

/* RAW socket option */
#define IP_HDRINCL 2          /* u_char; set/get IP header rcv/send or not  */

/*Multicast options*/
#define  IP_MULTICAST_IF   9  /* u_char; set/get IP multicast i/f  */
#define  IP_MULTICAST_TTL  10 /* u_char; set/get IP multicast ttl */
#define  IP_MULTICAST_LOOP 11 /* u_char; set/get IP multicast loopback */
#define  IP_ADD_MEMBERSHIP 12 /* ip_mreq; add an IP group membership */
#define  IP_DROP_MEMBERSHIP 13 /* ip_mreq; drop an IP group membership */
#define  IP_HDRINCL         2 /* int ; header is included with data */

/*IPv6*/
#define IPV6_MULTICAST_IF   80 /* unisgned int; set IF for outgoing MC pkts */
#define IPV6_MULTICAST_HOPS 81 /* int; set MC hopcount */
#define IPV6_MULTICAST_LOOP 82 /* unisgned int; set to 1 to loop back */
#define IPV6_JOIN_GROUP     83 /* ipv6_mreq; join MC group */
#define IPV6_LEAVE_GROUP    84 /* ipv6_mreq; leave MC group */
#define IPV6_V6ONLY         85 /* int; IPv6 only on this socket */

#define ATH_IN_MULTICAST(a)       (((a) & 0xF0000000L) == 0xE0000000L)

/*Driver level socket errors*/
#define A_SOCK_INVALID           -2  /*Socket handle is invalid*/

#define CUSTOM_ALLOC(size) \
  custom_alloc(size)

#define CUSTOM_FREE(buf) \
  custom_free(buf)

#define     ATH_IPPROTO_IP     0
#define     ATH_IPPROTO_ICMP   1
#define     ATH_IPPROTO_IGMP   2  /* added for IP multicasting changes */
#define     ATH_IPPROTO_TCP    6
#define     ATH_IPPROTO_UDP    17
#define     ATH_IPPROTO_RAW    255
#define     ATH_IPPROTO_IPV6      41 /* IPv6 header */
#define     ATH_IPPROTO_ROUTING   43 /* IPv6 Routing header */
#define     ATH_IPPROTO_FRAGMENT  44 /* IPv6 fragmentation header */

/* SSL cipher suites. To be used with SSL_configure(). */
#define TLS_NULL_WITH_NULL_NULL                    0x0000
#define TLS_RSA_WITH_NULL_MD5                      0x0001
#define TLS_RSA_WITH_NULL_SHA                      0x0002
#define TLS_RSA_WITH_RC4_128_MD5                   0x0004
#define TLS_RSA_WITH_RC4_128_SHA                   0x0005
#define TLS_RSA_WITH_DES_CBC_SHA                   0x0009
#define TLS_RSA_WITH_3DES_EDE_CBC_SHA              0x000A
#define TLS_DHE_RSA_WITH_DES_CBC_SHA               0x0015
#define TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA          0x0016
#define TLS_RSA_WITH_AES_128_CBC_SHA               0x002F
#define TLS_DHE_RSA_WITH_AES_128_CBC_SHA           0x0033
#define TLS_RSA_WITH_AES_256_CBC_SHA               0x0035
#define TLS_DHE_RSA_WITH_AES_256_CBC_SHA           0x0039
#define TLS_RSA_WITH_NULL_SHA256                   0x003B
#define TLS_RSA_WITH_AES_128_CBC_SHA256            0x003C
#define TLS_RSA_WITH_AES_256_CBC_SHA256            0x003D
#define TLS_DHE_RSA_WITH_AES_128_CBC_SHA256        0x0067
#define TLS_DHE_RSA_WITH_AES_256_CBC_SHA256        0x006B
#define TLS_PSK_WITH_RC4_128_SHA                   0x008A
#define TLS_PSK_WITH_3DES_EDE_CBC_SHA              0x008B
#define TLS_PSK_WITH_AES_128_CBC_SHA               0x008C
#define TLS_PSK_WITH_AES_256_CBC_SHA               0x008D
#define TLS_RSA_WITH_AES_128_GCM_SHA256            0x009C
#define TLS_RSA_WITH_AES_256_GCM_SHA384            0x009D
#define TLS_DHE_RSA_WITH_AES_128_GCM_SHA256        0x009E
#define TLS_DHE_RSA_WITH_AES_256_GCM_SHA384        0x009F
#define TLS_ECDH_ECDSA_WITH_NULL_SHA               0xC001
#define TLS_ECDH_ECDSA_WITH_RC4_128_SHA            0xC002
#define TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA       0xC003
#define TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA        0xC004
#define TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA        0xC005
#define TLS_ECDHE_ECDSA_WITH_NULL_SHA              0xC006
#define TLS_ECDHE_ECDSA_WITH_RC4_128_SHA           0xC007
#define TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA      0xC008
#define TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA       0xC009
#define TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA       0xC00A
#define TLS_ECDH_RSA_WITH_NULL_SHA                 0xC00B
#define TLS_ECDH_RSA_WITH_RC4_128_SHA              0xC00C
#define TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA         0xC00D
#define TLS_ECDH_RSA_WITH_AES_128_CBC_SHA          0xC00E
#define TLS_ECDH_RSA_WITH_AES_256_CBC_SHA          0xC00F
#define TLS_ECDHE_RSA_WITH_NULL_SHA                0xC010
#define TLS_ECDHE_RSA_WITH_RC4_128_SHA             0xC011
#define TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA        0xC012
#define TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA         0xC013
#define TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA         0xC014
#define TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256    0xC023
#define TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384    0xC024
#define TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256     0xC025
#define TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384     0xC026
#define TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256      0xC027
#define TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384      0xC028
#define TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256       0xC029
#define TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384       0xC02A
#define TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256    0xC02B
#define TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384    0xC02C
#define TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256     0xC02D
#define TLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384     0xC02E
#define TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256      0xC02F
#define TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384      0xC030
#define TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256       0xC031
#define TLS_ECDH_RSA_WITH_AES_256_GCM_SHA384       0xC032
#define TLS_RSA_WITH_AES_128_CCM                   0xC09C
#define TLS_RSA_WITH_AES_256_CCM                   0xC09D
#define TLS_DHE_RSA_WITH_AES_128_CCM               0xC09E
#define TLS_DHE_RSA_WITH_AES_256_CCM               0xC09F
#define TLS_RSA_WITH_AES_128_CCM_8                 0xC0A0
#define TLS_RSA_WITH_AES_256_CCM_8                 0xC0A1
#define TLS_DHE_RSA_WITH_AES_128_CCM_8             0xC0A2
#define TLS_DHE_RSA_WITH_AES_256_CCM_8             0xC0A3

#define SSL_PROTOCOL_UNKNOWN                  0x00
#define SSL_PROTOCOL_SSL_3_0                  0x30
#define SSL_PROTOCOL_TLS_1_0                  0x31
#define SSL_PROTOCOL_TLS_1_1                  0x32
#define SSL_PROTOCOL_TLS_1_2                  0x33

#define     ATH_ETH_P_PAE       0x888E

#define     ATH_MAX_ROUTES      3
/******************************************************************************
* *****************************     CAUTION     ******************************
* THESE DATA STRUCTURES ARE USED BY FIRMWARE ALSO. MAKE SURE THAT BOTH ARE IN
* SYNCH WHEN YOU MODIFY THESE.
******************************************************************************/

typedef PREPACK struct ip6_addr {
  QOSAL_UINT8   addr[16];    /* 128 bit IPv6 address */
}POSTPACK IP6_ADDR_T;

typedef PREPACK struct sockaddr_4 { ///E.Y. add _4
  QOSAL_UINT16 sin_port FIELD_PACKED;   //Port number
  QOSAL_UINT16 sin_family FIELD_PACKED; //ATH_AF_INET
  QOSAL_UINT32 sin_addr FIELD_PACKED;   //IPv4 Address
} POSTPACK SOCKADDR_T;

typedef PREPACK struct sockaddr_6{
  QOSAL_UINT16        sin6_family FIELD_PACKED;    // ATH_AF_INET6
  QOSAL_UINT16        sin6_port FIELD_PACKED;      // transport layer port #
  QOSAL_UINT32        sin6_flowinfo FIELD_PACKED;  // IPv6 flow information
  IP6_ADDR_T     sin6_addr FIELD_PACKED;       // IPv6 address
  QOSAL_UINT32        sin6_scope_id FIELD_PACKED;  // set of interfaces for a scope
}POSTPACK SOCKADDR_6_T;

typedef PREPACK struct _ip_mreq{
  QOSAL_UINT32 imr_multiaddr FIELD_PACKED;      //Multicast group address
  QOSAL_UINT32 imr_interface FIELD_PACKED;      //Interface address
}POSTPACK IP_MREQ_T;

typedef PREPACK struct _ipv6_mreq {
  IP6_ADDR_T ipv6mr_multiaddr FIELD_PACKED;  /* IPv6 multicast addr */
  IP6_ADDR_T ipv6mr_interface FIELD_PACKED;  /* IPv6 interface address */
}POSTPACK IPV6_MREQ_T;

enum SOCKET_TYPE{
  SOCK_STREAM_TYPE=1,    /* TCP */
  SOCK_DGRAM_TYPE,       /* UDP */
  SOCK_RAW_TYPE          /* IP */
};

enum IPCONFIG_MODE{
  IPCFG_QUERY=0,            /*Retrieve IP parameters*/
  IPCFG_STATIC,             /*Set static IP parameters*/
  IPCFG_DHCP,               /*Run DHCP client*/
  IPCFG_AUTO,             /*Run AUTO IP*/
};

typedef PREPACK struct {
  QOSAL_UINT32    length;
  QOSAL_UINT32    resp_code;
  QOSAL_UINT32    flags;
  QOSAL_UINT8     data[1];
} POSTPACK HTTPC_RESPONSE;

enum DNSC_CMDS{
  GETHOSTBYNAME=0,
  GETHOSTBYNAME2,
  RESOLVEHOSTNAME
};

/* HTTP Server socket command MACROS*/
#define HTTP_UNKNOWN    -1
#define HTTP_SERVER_STOP    0
#define HTTP_SERVER_START   1
#define HTTPS_SERVER_STOP   2
#define HTTPS_SERVER_START  3

/* HTTP Server Get/Post socket MACROS */
#define HTTP_GET_METHOD     0
#define HTTP_POST_METHOD    1

enum {
  /* Host command macros */
  HTTPC_CONNECT_CMD,
  HTTPC_GET_CMD,
  HTTPC_POST_CMD,
  HTTPC_DATA_CMD,
  HTTPC_DISCONNECT_CMD,
  HTTPC_CONNECT_SSL_CMD
};

enum {
  ROUTE_ADD,
  ROUTE_DEL,
  ROUTE_SHOW,
  ROUTE_MAX
};

#define GET_TLV_TYPE(x, y)    (A_MEMCPY(&(y), (x), sizeof(QOSAL_UINT16)))
#define GET_TLV_LENGTH(x, y)  (A_MEMCPY(&(y), (x + 2), sizeof(QOSAL_UINT16)))
#define GET_NEXT_TLV(x, y)    ((x) + sizeof(QOSAL_UINT16) + sizeof(QOSAL_UINT16) + (y))
#define GET_TLV_VAL(x)       ((x) + sizeof(QOSAL_UINT16) + sizeof(QOSAL_UINT16))

enum WI_POST_EVENT_TLV_TYPE{
  POST_TYPE_URI,
  POST_TYPE_NAME,
  POST_TYPE_VALUE
};

typedef struct {
  QOSAL_UINT32 numTLV;     /*Number of TLVs encapsulated in the event*/
  QOSAL_UINT8 data[0];     /*Start of TLV data*/
}HTTP_POST_EVENT_T;

#define  MAX_DNSADDRS 3
#define  MAX_SNTP_SERVERS 2
#define DNS_NAME_NOT_SPECIFIED 0

typedef PREPACK struct dnccfgcmd {
  WMI_SOCKET_CMD wmi_cmd FIELD_PACKED;
  QOSAL_UINT32        mode FIELD_PACKED;        //0-gethostbyname, 1-gethostbyname2, 2-resolvehostname
  QOSAL_UINT32        domain FIELD_PACKED;       // AF_INET ,AF_INET6
  QOSAL_UINT8  ahostname[36];  FIELD_PACKED
}POSTPACK DNC_CFG_CMD;

typedef PREPACK struct dncrespinfo {
  char           dns_names[64] FIELD_PACKED;   /* Buffer of names (usually w/domain) */
  QOSAL_INT32        h_length; FIELD_PACKED   /* Length of the address */
  QOSAL_INT32        h_addrtype FIELD_PACKED;   //host address type
  QOSAL_INT32        ipaddrs FIELD_PACKED;   // count of entries in ipaddr_list
  QOSAL_UINT32       ipaddrs_list[MAX_DNSADDRS]  FIELD_PACKED;    // list of ipv4 address
  QOSAL_INT32        ip6addrs FIELD_PACKED;    //count of ip6 entries in ip6addr_list
  IP6_ADDR_T     ip6addrs_list[MAX_DNSADDRS] FIELD_PACKED;   //list of ip6 address
}POSTPACK DNC_RESP_INFO;

typedef PREPACK struct ip46addr {
  QOSAL_UINT8 type  FIELD_PACKED;
  QOSAL_UINT8 au1Rsvd[3] FIELD_PACKED;
  QOSAL_UINT32 addr4 FIELD_PACKED;
  IP6_ADDR_T addr6 FIELD_PACKED;
}POSTPACK IP46ADDR;

typedef  struct sntp_time{
  QOSAL_INT32  Sec FIELD_PACKED;
  QOSAL_INT32  min FIELD_PACKED;
  QOSAL_INT32  hour FIELD_PACKED;
  QOSAL_INT32  mon FIELD_PACKED;
  QOSAL_INT32  year FIELD_PACKED;
  QOSAL_INT32  wday FIELD_PACKED;
  QOSAL_INT32  yday FIELD_PACKED;
}POSTPACK tSntpTime;

typedef PREPACK struct sntp_tm{
  QOSAL_UINT32  tv_sec FIELD_PACKED;      /* seconds */
  QOSAL_UINT32  tv_usec FIELD_PACKED;     /* and microseconds */
}POSTPACK tSntpTM;

typedef struct sntp_dns_addr{
  QOSAL_UINT8 addr[68] FIELD_PACKED;
  QOSAL_UINT8 resolve FIELD_PACKED;
}POSTPACK tSntpDnsAddr;

typedef PREPACK struct IPv4Route {
  QOSAL_UINT32  reserved FIELD_PACKED;
  QOSAL_UINT32 address FIELD_PACKED;
  QOSAL_UINT32 mask FIELD_PACKED;
  QOSAL_UINT32 gateway FIELD_PACKED;
  QOSAL_UINT32 ifIndex FIELD_PACKED;
  QOSAL_UINT32 prot FIELD_PACKED;
} POSTPACK IPV4_ROUTE_T;

typedef PREPACK struct IPv4RouteLists {
  QOSAL_UINT32    rtcount FIELD_PACKED;
  IPV4_ROUTE_T  route[ATH_MAX_ROUTES] FIELD_PACKED;
} POSTPACK IPV4_ROUTE_LIST_T;

typedef PREPACK struct IPv6Route {
  QOSAL_UINT32 command FIELD_PACKED;
  QOSAL_UINT8  address[16] FIELD_PACKED;
  QOSAL_INT32  prefixlen FIELD_PACKED;
  QOSAL_UINT8  nexthop[16] FIELD_PACKED;
  QOSAL_UINT32 ifindex;
  QOSAL_UINT32 prot;
}POSTPACK IPV6_ROUTE_T;

typedef PREPACK struct IPv6RouteLists {
  QOSAL_UINT32    rtcount FIELD_PACKED;
  IPV6_ROUTE_T  route[ATH_MAX_ROUTES] FIELD_PACKED;
} POSTPACK IPV6_ROUTE_LIST_T;

#define MAX_OTA_AREA_READ_SIZE 1024
typedef PREPACK struct ota_upgrade_resp{
  QOSAL_UINT32 resp_code;
  QOSAL_UINT32 size;
}POSTPACK tOtaUpgradeResp;

typedef PREPACK struct ota_info{
  QOSAL_UINT32 resp_code;
  QOSAL_UINT32 size;
  QOSAL_UCHAR *data;
}POSTPACK tOtaReadResp;

typedef PREPACK struct ota_done{
  QOSAL_UINT32 resp_code;
}POSTPACK tOtaDoneResp;

typedef struct ota_response_s{
  QOSAL_UINT32 resp_code;
}POSTPACK tOtaResp;

typedef struct ota_partition_get_size_response_s{
  QOSAL_UINT32 resp_code;
  QOSAL_UINT32 size;
}POSTPACK tOtaPartitionGetSizeResp;

typedef struct ota_parse_image_hdr_response_s{
  QOSAL_UINT32 resp_code;
  QOSAL_UINT32 offset;
}POSTPACK tOtaParseImageHdrResp;

typedef struct ota_partition_write_data_response_s{
  QOSAL_UINT32 resp_code;
  QOSAL_UINT32 size;
}POSTPACK tOtaPartitionWriteDataResp;

typedef enum OTA_STATUS {
  QCOM_OTA_ERROR = -1,

  QCOM_OTA_OK = 0,
  QCOM_OTA_COMPLETED = 1,
  QCOM_OTA_FLASH_AUTH_PENDING = 3,

  QCOM_OTA_ERR_UNKOWN_MSG = 1000,
  QCOM_OTA_ERR_IMAGE_NOT_FOUND = 1001,
  QCOM_OTA_ERR_IMAGE_DOWNLOAD_FAIL = 1002,
  QCOM_OTA_ERR_IMAGE_CHECKSUM_INCORRECT = 1003,
  QCOM_OTA_ERR_SERVER_RSP_TIMEOUT = 1004,
  QCOM_OTA_ERR_INVALID_FILENAME = 1005,
  QCOM_OTA_ERR_UNSUPPORT_PROTOCOL = 1006,
  QCOM_OTA_ERR_INVALID_PARTITION_INDEX = 1007,
  QCOM_OTA_ERR_IMAGE_HDR_INCORRECT = 1008,
  QCOM_OTA_ERR_INSUFFICIENT_MEMORY = 1009,
  QCOM_OTA_ERR_PRESERVE_LAST_FAILED = 1010,
  QCOM_OTA_ERR_NO_ACTIVE_OTA_SESSION = 1011,
  QCOM_OTA_ERR_INVALID_PARTITION_ACESS = 1012,
  QCOM_OTA_ERR_OTA_SESS_IN_PROGRESS = 1013,
  QCOM_OTA_ERR_FLASH_READ_TIMEOUT = 1014,
  QCOM_OTA_ERR_FLASH_ERASE_ERROR = 1015,
  QCOM_OTA_ERR_IMAGE_OVERFLOW = 1016,
  QCOM_OTA_ERR_IMAGE_UNDERFLOW = 1017,
  QCOM_OTA_ERR_WRITE_DATA_ERROR = 1018,
} QCOM_OTA_STATUS_CODE_t;

enum SslErrors{
  ESSL_OK =              0,   // success
  ESSL_INVAL =          -1,   // Invalid argument
  ESSL_NOSOCKET =       -2,   // No more SSL socket descriptors available
  ESSL_HSNOTDONE =      -3,   // Handshake not done
  ESSL_HSDONE =         -4,   // Handshake already done
  ESSL_NOMEM =          -5,   // Out of memory
  ESSL_CONN =           -6,   // SharkSslCon_Error
  ESSL_CERT =           -7,   // SharkSslCon_CertificateError
  ESSL_ALERTRECV =      -8,   // SharkSslCon_AlertRecv
  ESSL_ALERTFATAL =     -9,   // SharkSslCon_AlertSend FATAL received. Connection must be closed.
  ESSL_TIMEOUT =       -10,   // Timeout during handshake
  ESSL_OOPS =          -29,   // Oops (something is terribly wrong)
  ESSL_OK_HANDSHAKE =  -32,   // handshake complete (internal reason code, not an error)

  //                                                               Following TRUST reason codes are returned by sslValidate()

  /** The peer's SSL certificate is trusted, CN matches the host name, time is valid */
  ESSL_TRUST_CertCnTime = -32,   // Same as ESSL_OK_HANDSHAKE
  /** The peer's SSL certificate is trusted, CN matches the host name, time is expired */
  ESSL_TRUST_CertCn =     -33,   // name    OK, time NOTOK
  /** The peer's SSL certificate is trusted, CN does NOT match the host name, time is valid */
  ESSL_TRUST_CertTime =   -34,   // name NOTOK, time    OK
  /** The peer's SSL certificate is trusted, CN does NOT match host name, time is expired */
  ESSL_TRUST_Cert =       -35,   // name NOTOK, time NOTOK
  /** The peer's SSL certificate is NOT trusted */
  ESSL_TRUST_None =       -36,
};

typedef void SSL_CTX;
typedef void SSL;
typedef const QOSAL_UINT8 *SslCert;
typedef const QOSAL_UINT8 *SslCAList;

typedef enum {
  SSL_SERVER = 1,
  SSL_CLIENT = 2
} SSL_ROLE_T;

typedef enum {
  SSL_CERTIFICATE = 1,
  SSL_CA_LIST = 2
} SSL_CERT_TYPE_T;

typedef PREPACK struct sslVerifyPolicy {
  /** True to verify certificate commonName against peer's domain name */
  QOSAL_UINT8 domain FIELD_PACKED;
  /** True to verify certificate time validity */
  QOSAL_UINT8 timeValidity FIELD_PACKED;
  /** True to immediately send a fatal alert on detection of untrusted certificate */
  QOSAL_UINT8 sendAlert FIELD_PACKED;
  /** Reserved */
  QOSAL_UINT8 reserved FIELD_PACKED;
} POSTPACK SSL_VERIFY_POLICY;

#define SSL_CIPHERSUITE_LIST_DEPTH 8
typedef PREPACK struct SSL_config {
  QOSAL_UINT16 protocol FIELD_PACKED;
  QOSAL_UINT16 cipher[SSL_CIPHERSUITE_LIST_DEPTH] FIELD_PACKED;
  SSL_VERIFY_POLICY verify FIELD_PACKED;
  QOSAL_CHAR matchName[40] FIELD_PACKED;
} POSTPACK SSL_CONFIG;

typedef PREPACK struct ssl_file_name_list {
  QOSAL_UINT8  name[10][20] FIELD_PACKED;   // The file names of the certificates or CA lists
}POSTPACK SSL_FILE_NAME_LIST;

#if ZERO_COPY
QOSAL_VOID zero_copy_free(QOSAL_VOID* buffer);
#endif

#if ZERO_COPY
QOSAL_INT32 t_recvfrom(QCA_CTX * enet_ptr, QOSAL_UINT32 handle, void** buffer, QOSAL_UINT32 length, QOSAL_UINT32 flags, QOSAL_VOID* name, QOSAL_UINT32* socklength);
QOSAL_INT32 t_recv(QCA_CTX * enet_ptr, QOSAL_UINT32 handle, void** buffer, QOSAL_UINT32 length, QOSAL_UINT32 flags);
#else
QOSAL_INT32 t_recvfrom(QCA_CTX * enet_ptr, QOSAL_UINT32 handle, void* buffer, QOSAL_UINT32 length, QOSAL_UINT32 flags, QOSAL_VOID* name, QOSAL_UINT32* socklength);
QOSAL_INT32 t_recv(QCA_CTX * enet_ptr, QOSAL_UINT32 handle, void* buffer, QOSAL_UINT32 length, QOSAL_UINT32 flags);
#endif
QOSAL_INT32 t_select(QCA_CTX * enet_ptr, QOSAL_UINT32 handle, QOSAL_UINT32 tv);

#if T_SELECT_VER1
QOSAL_INT32 FD_IsSet(QOSAL_UINT32 handle, QOSAL_UINT32 mask);
QOSAL_INT32 FD_Set(QOSAL_UINT32 handle, QOSAL_UINT32* mask);
QOSAL_INT32 FD_Clr(QOSAL_UINT32 handle, QOSAL_UINT32* mask);
QOSAL_INT32 FD_Zero(QOSAL_UINT32* mask);

QOSAL_INT32 t_select_ver1(QCA_CTX * enet_ptr, QOSAL_INT32 num, QOSAL_UINT32 *r_fd, QOSAL_UINT32 *w_fd, QOSAL_UINT32 *e_fd, QOSAL_UINT32 tv);
#endif //T_SELECT_VER1

QOSAL_INT32 custom_ipbridgemode(void* handle, QOSAL_UINT16 status);
QOSAL_INT32 custom_ipconfig_set_tcp_exponential_backoff_retry(QCA_CTX * enet_ptr, QOSAL_INT32 retry);
QOSAL_INT32 custom_ipconfig_set_ip6_status(QCA_CTX * enet_ptr, QOSAL_UINT16 status);
QOSAL_INT32 custom_ipconfig_dhcp_release(QCA_CTX * enet_ptr);
QOSAL_INT32 custom_ipconfig_set_tcp_rx_buffer(QCA_CTX * enet_ptr, QOSAL_INT32 rxbuf);
QOSAL_INT32 custom_ip_http_server(void *handle, QOSAL_INT32 command);
QOSAL_INT32 custom_ip_http_server_method(void *handle, QOSAL_INT32 command, QOSAL_UINT8 *pagename, QOSAL_UINT8 *objname, QOSAL_INT32 objtype, QOSAL_INT32 objlen, QOSAL_UINT8 * value);

#if ENABLE_DNS_CLIENT
QOSAL_INT32 custom_ip_set_dns_block_time(void *handle, QOSAL_INT32 blockSec);
QOSAL_INT32 custom_ip_resolve_hostname(void *handle, DNC_CFG_CMD *DncCfg, DNC_RESP_INFO *DncRespInfo);
QOSAL_INT32 custom_ip_dns_client(void *handle, QOSAL_INT32 command);
QOSAL_INT32 custom_ip_dns_server_addr(void *handle, IP46ADDR *addr);
#endif
QOSAL_INT32 custom_ip_hostname(void *handle, char *domain_name);
#if ENABLE_DNS_SERVER
QOSAL_INT32 custom_ip_dns_local_domain(void *handle, char *domain_name);
QOSAL_INT32 custom_ipsetdns(void *handle, QOSAL_INT32 command, char *domain_name, IP46ADDR *dnsaddr);
QOSAL_INT32 custom_ip_dns_server(void *handle, QOSAL_UINT32 command);
QOSAL_INT32 custom_ipdns(void *handle, QOSAL_INT32 command, char *domain_name, IP46ADDR *dnsaddr);
#endif
#if ENABLE_SNTP_CLIENT
QOSAL_INT32 custom_ip_sntp_srvr_addr(void *handle, QOSAL_INT32 command, char * sntp_srvr_addr);
QOSAL_INT32 custom_ip_sntp_get_time(void *handle, tSntpTime *SntpTime);
QOSAL_INT32 custom_ip_sntp_get_time_of_day(void *handle, tSntpTM *SntpTm);
QOSAL_INT32 custom_ip_sntp_modify_zone_dse(void *handle, QOSAL_UINT8 hour, QOSAL_UINT8 min, QOSAL_UINT8 add_sub, QOSAL_UINT8 dse);
QOSAL_INT32 custom_ip_sntp_query_srvr_address(void *handle, tSntpDnsAddr SntpDnsAddr[MAX_SNTP_SERVERS]);
QOSAL_INT32 custom_ip_sntp_client(void *handle, QOSAL_INT32 command);
#endif
#if ENABLE_HTTP_CLIENT
QOSAL_INT32 custom_httpc_method(void* a_handle, QOSAL_UINT32 command, QOSAL_UINT8 *url, QOSAL_UINT8 *data, QOSAL_UINT8 **output);
#endif /* ENABLE_HTTP_CLIENT */
QOSAL_VOID zero_copy_http_free(QOSAL_VOID* buffer);
#if ENABLE_ROUTING_CMDS
QOSAL_INT32 custom_ipv4_route(void* handle, QOSAL_UINT32 command, QOSAL_UINT32* ipv4_addr, QOSAL_UINT32* subnetMask, QOSAL_UINT32* gateway, QOSAL_UINT32* ifIndex, IPV4_ROUTE_LIST_T *routelist);
QOSAL_INT32 custom_ipv6_route(void* handle, QOSAL_UINT32 command, QOSAL_UINT8* ipv6_addr, QOSAL_UINT32* prefixLen, QOSAL_UINT8* gateway, QOSAL_UINT32* ifIndex, IPV6_ROUTE_LIST_T *routelist);
#endif
QOSAL_INT32 custom_tcp_connection_timeout(void *handle, QOSAL_UINT32 timeout_val);

QOSAL_INT32 custom_ota_upgrade(QOSAL_VOID *handle, QOSAL_UINT32 addr, QOSAL_CHAR *filename, QOSAL_UINT8 mode, QOSAL_UINT8 preserve_last, QOSAL_UINT8 protocol, QOSAL_UINT32 * resp_code, QOSAL_UINT32 *length);
QOSAL_INT32 custom_ota_read_area(QOSAL_VOID *handle, QOSAL_UINT32 offset, QOSAL_UINT32 size, QOSAL_UCHAR *buffer, QOSAL_UINT32 *retlen);
QOSAL_INT32 custom_ota_done(QOSAL_VOID *handle, QOSAL_BOOL good_image);
QOSAL_INT32  custom_ota_session_start(QOSAL_VOID* handle, QOSAL_UINT32 flags, QOSAL_UINT32 partition_index);
QOSAL_UINT32 custom_ota_partition_get_size(QOSAL_VOID* handle);
QOSAL_INT32  custom_ota_partition_erase(QOSAL_VOID* handle);
QOSAL_INT32  custom_ota_partition_verify_checksum(QOSAL_VOID* handle);
QOSAL_INT32  custom_ota_parse_image_hdr(QOSAL_VOID* handle, QOSAL_UINT8 *header, QOSAL_UINT32 *offset);
QOSAL_INT32 custom_ota_partition_write_data(QOSAL_VOID* handle, QOSAL_UINT32 offset, QOSAL_UINT8 *buf, QOSAL_UINT32 size, QOSAL_UINT32 *ret_size);
QOSAL_INT32 custom_ota_set_response_cb(QOSAL_VOID *handle, QOSAL_VOID* callback);
QOSAL_INT32 Custom_Api_Dhcps_Success_Callback_Event(QOSAL_VOID *pCxt, QOSAL_UINT8 *datap);
QOSAL_INT32 Custom_Api_Dhcpc_Success_Callback_Event(QOSAL_VOID *pCxt, QOSAL_UINT8 *datap);
QOSAL_VOID  Custom_Api_Ota_Resp_Result(QOSAL_VOID *pCxt, QOSAL_UINT32 cmd, QOSAL_UINT32 resp_code, QOSAL_UINT32 result);

/***************************************************************************************************/

QOSAL_VOID* custom_alloc(QOSAL_UINT32 size);
QOSAL_VOID custom_free(QOSAL_VOID* buf);

#if ENABLE_SSL
SSL_CTX* SSL_ctx_new(SSL_ROLE_T role, QOSAL_INT32 inbufSize, QOSAL_INT32 outbufSize, QOSAL_INT32 reserved);
QOSAL_INT32 SSL_ctx_free(SSL_CTX *ctx);
SSL* SSL_new(SSL_CTX *ctx);
QOSAL_INT32 SSL_setCaList(SSL_CTX *ctx, SslCAList caList, QOSAL_UINT32 size);
QOSAL_INT32 SSL_addCert(SSL_CTX *ctx, SslCert cert, QOSAL_UINT32 size);
QOSAL_INT32 SSL_storeCert(QOSAL_CHAR *name, SslCert cert, QOSAL_UINT32 size);
QOSAL_INT32 SSL_loadCert(SSL_CTX *ctx, SSL_CERT_TYPE_T type, char *name);
QOSAL_INT32 SSL_listCert(SSL_FILE_NAME_LIST *fileNames);

QOSAL_INT32 SSL_set_fd(SSL *ssl, QOSAL_UINT32 fd);
QOSAL_INT32 SSL_accept(SSL *ssl);
QOSAL_INT32 SSL_connect(SSL *ssl);
QOSAL_INT32 SSL_shutdown(SSL *ssl);
QOSAL_INT32 SSL_configure(SSL *ssl, SSL_CONFIG *cfg);

#if ZERO_COPY
QOSAL_INT32 SSL_read(SSL *ssl, void **buf, QOSAL_INT32 num);
#else
QOSAL_INT32 SSL_read(SSL *ssl, void *buf, QOSAL_INT32 num);
#endif
QOSAL_INT32 SSL_write(SSL *ssl, const void *buf, QOSAL_INT32 num);
#endif
#else //ENABLE_STACK_OFFLOAD
typedef PREPACK struct ip6_addr {
  QOSAL_UINT8   addr[16];    /* 128 bit IPv6 address */
}POSTPACK IP6_ADDR_T;
#endif //ENABLE_STACK_OFFLOAD
#endif
