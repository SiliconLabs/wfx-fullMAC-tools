//                                                                ------------------------------------------------------------------------------
//                                                                 Copyright (c) 2014 Qualcomm Atheros, Inc.
//                                                                 All Rights Reserved.
//                                                                 Qualcomm Atheros Confidential and Proprietary.
//                                                                 Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is
//                                                                 hereby granted, provided that the above copyright notice and this permission notice appear in all copies.
//
//                                                                 THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
//                                                                 INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//                                                                 ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
//                                                                 USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//                                                                 OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//                                                                ------------------------------------------------------------------------------
//                                                                ==============================================================================
//                                                                 Author(s): ="Atheros"
//                                                                ==============================================================================

#ifndef __QCOM_API_H__
#define __QCOM_API_H__

#include <a_config.h>
#include <a_types.h>
#include <a_osapi.h>
#include <driver_cxt.h>
#include <common_api.h>
#include <custom_wlan_api.h>
#include <wmi_api.h>
#include <atheros_stack_offload.h>
#include <atheros_wifi_api.h>

#define QCOM_WPS_MAX_KEY_LEN    64
#define WPS_MAX_DEVNAME_LEN 32
#define MAX_SSID_LEN 32
#define MAC_LEN 6
#define MAX_SSID_LENGTH (32 + 1)
#define P2P_WPS_PIN_LEN 9
#define MAX_PASSPHRASE_SIZE 64
#define P2P_MAX_NOA_DESCRIPTORS 4
#define WPS_MAX_PASSPHRASE_LEN 9
#define P2P_PERSISTENT_FLAG 0x80
#define MAX_LIST_COUNT 8
#define P2P_AUTO_CHANNEL 0
#define P2P_DEFAULT_CHAN 1

#define QCOM_PARAM_GROUP_SYSTEM         1
#define QCOM_PARAM_GROUP_WLAN           2
#define QCOM_PARAM_GROUP_NETWORK        3

/*define new param groups here*/

/*QCOM_PARAM_GROUP_SYSTEM Parameters*/
#define QCOM_PARAM_GROUP_SYSTEM_SLEEP_MAINT_TIMEOUT_MSECS            1
#define QCOM_PARAM_GROUP_SYSTEM_SLEEP_WATCHDOG_TIMEOUT_SECS         2
/* Add new system tunables here, at the end */

/*QCOM_PARAM_GROUP_WLAN_Parameters*/
#define QCOM_PARAM_GROUP_WLAN_WOW_ENABLE  1
#define QCOM_PARAM_GROUP_WLAN_WOW_PATTERN 2
#define QCOM_PARAM_GROUP_WLAN_WOW_GPIO    3
#define QCOM_PARAM_GROUP_WLAN_WOW_HOSTSLEEP 4

/*QCOM_PARAM_GROUP_NETWORK_Parameters*/
#define QCOM_PARAM_GROUP_NETWORK_DNS_TIMEOUT_SECS 1

#define QCOM_WOW_PATTERN_MAX_SIZE 8

typedef void *P2P_DEV_CTXT;
typedef void (*QCOM_PROMISCUOUS_CB)(unsigned char *buf, int length);

typedef struct _qcom_scan_info{
  QOSAL_UINT8 channel;
  QOSAL_UINT8 ssid_len;
  QOSAL_UINT8 rssi;
  QOSAL_UINT8 security_enabled;
  QOSAL_INT16 beacon_period;
  QOSAL_UINT8 preamble;
  QOSAL_UINT8 bss_type;
  QOSAL_UINT8 bssid[6];
  QOSAL_UINT8 ssid[32];
  QOSAL_UINT8 rsn_cipher;
  QOSAL_UINT8 rsn_auth;
  QOSAL_UINT8 wpa_cipher;
  QOSAL_UINT8 wpa_auth;
} QCOM_BSS_SCAN_INFO, * QCOM_BSS_SCAN_INFO_PTR;

typedef struct _qcom_scan_params{
  QOSAL_UINT16 fgStartPeriod;
  QOSAL_UINT16 fgEndPeriod;
  QOSAL_UINT16 bgPeriod;
  QOSAL_UINT16 maxActChDwellTimeInMs;
  QOSAL_UINT16 pasChDwellTimeInMs;
  QOSAL_UINT8  shortScanRatio;
  QOSAL_UINT8  scanCtrlFlags;
  QOSAL_UINT16 minActChDwellTimeInMs;
  QOSAL_UINT16 maxActScanPerSsid;
  QOSAL_UINT32 maxDfsChActTimeInMs;
} qcom_scan_params_t;

#define QCOM_START_SCAN_PARAMS_CHANNEL_LIST_MAX 12

typedef struct _qcom_start_scan_params{
  QOSAL_BOOL   forceFgScan;
  QOSAL_UINT32 homeDwellTimeInMs;
  QOSAL_UINT32 forceScanIntervalInMs;
  QOSAL_UINT8  scanType;
  QOSAL_UINT8  numChannels;
  QOSAL_UINT16 channelList[QCOM_START_SCAN_PARAMS_CHANNEL_LIST_MAX];
} qcom_start_scan_params_t;

typedef enum {
  QCOM_WLAN_DEV_MODE_STATION = 0,
  QCOM_WLAN_DEV_MODE_AP,
  QCOM_WLAN_DEV_MODE_ADHOC,
  QCOM_WLAN_DEV_MODE_INVALID
}QCOM_WLAN_DEV_MODE;

typedef enum {
  QCOM_11A_MODE = 0x1,
  QCOM_11G_MODE = 0x2,
  QCOM_11N_MODE = 0x3,
  QCOM_11B_MODE = 0x4,
  QCOM_HT40_MODE = 0x5,
} QCOM_WLAN_DEV_PHY_MODE;

typedef enum {
  WLAN_AUTH_NONE  = 0,
  WLAN_AUTH_WPA,
  WLAN_AUTH_WPA2,
  WLAN_AUTH_WPA_PSK,
  WLAN_AUTH_WPA2_PSK,
  WLAN_AUTH_WPA_CCKM,
  WLAN_AUTH_WPA2_CCKM,
  WLAN_AUTH_WPA2_PSK_SHA256,
  WLAN_AUTH_WEP,
  WLAN_AUTH_INVALID
} WLAN_AUTH_MODE;

typedef enum {
  WLAN_CRYPT_NONE          = 0,
  WLAN_CRYPT_WEP_CRYPT,
  WLAN_CRYPT_TKIP_CRYPT,
  WLAN_CRYPT_AES_CRYPT,
  WLAN_CRYPT_WAPI_CRYPT,
  WLAN_CRYPT_BIP_CRYPT,
  WLAN_CRYPT_KTK_CRYPT,
  WLAN_CRYPT_INVALID
} WLAN_CRYPT_TYPE;

/* WPS credential information */
typedef struct {
  QOSAL_UINT16            ap_channel;
  QOSAL_UINT8             ssid[WMI_MAX_SSID_LEN];
  QOSAL_UINT8             ssid_len;
  WLAN_AUTH_MODE      auth_type;   /* WPS_AUTH_TYPE */
  WLAN_CRYPT_TYPE     encr_type;   /* WPS_ENCR_TYPE */
  QOSAL_UINT8             key_idx;
  QOSAL_UINT8             key[QCOM_WPS_MAX_KEY_LEN + 1];
  QOSAL_UINT8             key_len;
  QOSAL_UINT8             mac_addr[ATH_MAC_LEN];
} qcom_wps_credentials_t;

typedef enum  {
  P2P_WPS_NOT_READY, P2P_WPS_PIN_LABEL, P2P_WPS_PIN_DISPLAY, P2P_WPS_PIN_KEYPAD, P2P_WPS_PBC
} P2P_WPS_METHOD;

#if ENABLE_P2P_MODE
typedef struct {
  QOSAL_UINT8 enable;
  QOSAL_UINT8 count;
  P2P_NOA_DESCRIPTOR noas[P2P_MAX_NOA_DESCRIPTORS];
} P2P_NOA_INFO;

typedef enum {
  P2P_INV_ROLE_GO,
  P2P_INV_ROLE_ACTIVE_GO,
  P2P_INV_ROLE_CLIENT,
} P2P_INV_ROLE;

typedef enum {
  P2P_CONFIG_LISTEN_CHANNEL,
  P2P_CONFIG_CROSS_CONNECT,
  P2P_CONFIG_SSID_POSTFIX,
  P2P_CONFIG_INTRA_BSS,
  P2P_CONFIG_CONCURRENT_MODE,
  P2P_CONFIG_GO_INTENT,
  P2P_CONFIG_DEV_NAME,
  P2P_CONFIG_P2P_OPMODE,
  P2P_CONFIG_CCK_RATES,
  P2P_CONFIG_MAX,
} P2P_CONF_ID;
#endif
typedef struct { QOSAL_UINT8 addr[6]; } A_MACADDR;

typedef struct {
  QOSAL_UINT8 rate_index;
  QOSAL_UINT8 tries;
  QOSAL_UINT32 size;
  QOSAL_UINT32 chan;
  QOSAL_UINT8 header_type;
  QOSAL_UINT16  seq;
  A_MACADDR addr1;
  A_MACADDR addr2;
  A_MACADDR addr3;
  A_MACADDR addr4;
  QOSAL_UINT8 *pdatabuf;
  QOSAL_UINT32 buflen;
} QCOM_RAW_MODE_PARAM_t;

typedef enum {
  GPIO_EDGE_TRIGGER,
  GPIO_LEVEL_TRIGGER
}QCOM_WOW_GPIO_TRIGGER;

typedef struct {
  QOSAL_UINT32  gpio;
  QOSAL_BOOL    isActiveLow;
  QOSAL_UINT32  triggerMechanism; /*0: edge triggered 1: level triggered*/
}QCOM_WOW_GPIO;

typedef struct {
  QOSAL_UINT32  pattern_size;
  QOSAL_UINT32  offset;
  QOSAL_UINT32  pattern_index;
  QOSAL_UINT8   pattern[QCOM_WOW_PATTERN_MAX_SIZE];
}QCOM_WOW_PATTERN;

typedef struct {
  QOSAL_UINT32  enable;
}QCOM_WOW_ENABLE;

typedef struct {
  QOSAL_UINT32  awake;
}QCOM_WOW_HOST_SLEEP;

#if ENABLE_STACK_OFFLOAD

typedef struct sockaddr{
  QOSAL_UINT16  sa_family;
  char  sa_data[32];
} sockaddr_t;

/*===================================================================================*/
/* OTA */
/* Set to upgrade Target FW. Otherwise, update customer image */
#define QCOM_OTA_TARGET_FIRMWARE_UPGRADE  (1 << 0)
/* Set to preserve the last active FW */
#define QCOM_OTA_PRESERVE_LAST      (1 << 1)
/* Set to erase upgrade partition rw dset */
#define QCOM_OTA_ERASING_RW_DSET                (1 << 2)
/* OTA Upgrade partitions    */
#define PARTITION_AUTO      0 /* Auto-select OTA partition */

typedef enum {
  QCOM_OTA_PROTOCOL_TFTP = 0,
  QCOM_OTA_PROTOCOL_FTP,
  QCOM_OTA_PROTOCOL_HTTP,
  QCOM_OTA_PROTOCOL_HTTPS,
  QCOM_OTA_PROTOCOL_MAX = QCOM_OTA_PROTOCOL_TFTP,
}QCOM_OTA_PROTOCOL_t;

int qcom_socket(int family, int type, int protocol);
int qcom_connect(int s, struct sockaddr* addr, int addrlen);
int qcom_bind(int s, struct sockaddr* addr, int addrlen);
int qcom_listen(int s, int backlog);
int qcom_accept(int s, struct sockaddr* addr, int *addrlen);
int qcom_setsockopt(int s, int level, int name, void* arg, int arglen);
int qcom_select(QOSAL_UINT32 handle, QOSAL_UINT32 tv);
#if ZERO_COPY
int qcom_recv(int s, char** buf, int len, int flags);
#else
int qcom_recv(int s, char* buf, int len, int flags);
#endif

#if ZERO_COPY
int qcom_recvfrom(int s, char** buf, int len, int flags, struct sockaddr* from, int* fromlen);
#else
int qcom_recvfrom(int s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen);

#endif
A_STATUS qcom_ipconfig(QOSAL_UINT8 device_id, QOSAL_INT32 mode, QOSAL_UINT32* address, QOSAL_UINT32* submask, QOSAL_UINT32* gateway);
SSL* qcom_SSL_new(SSL_CTX *ctx);
SSL_CTX* qcom_SSL_ctx_new(SSL_ROLE_T role, QOSAL_INT32 inbufSize, QOSAL_INT32 outbufSize, QOSAL_INT32 reserved);
QOSAL_INT32 qcom_SSL_setCaList(SSL_CTX *ctx, SslCAList caList, QOSAL_UINT32 size);
QOSAL_INT32 qcom_SSL_addCert(SSL_CTX *ctx, SslCert cert, QOSAL_UINT32 size);
QOSAL_INT32 qcom_SSL_storeCert(QOSAL_CHAR *name, SslCert cert, QOSAL_UINT32 size);
QOSAL_INT32 qcom_SSL_loadCert(SSL_CTX *ctx, SSL_CERT_TYPE_T type, char *name);
QOSAL_INT32 qcom_SSL_listCert(SSL_FILE_NAME_LIST *fileNames);
QOSAL_INT32 qcom_SSL_set_fd(SSL *ssl, QOSAL_UINT32 fd);
QOSAL_INT32 qcom_SSL_accept(SSL *ssl);
QOSAL_INT32 qcom_SSL_connect(SSL *ssl);
QOSAL_INT32 qcom_SSL_shutdown(SSL *ssl);
QOSAL_INT32 qcom_SSL_ctx_free(SSL_CTX *ctx);
QOSAL_INT32 qcom_SSL_configure(SSL *ssl, SSL_CONFIG *cfg);
#if ZERO_COPY
QOSAL_INT32 qcom_SSL_read(SSL *ssl, void **buf, QOSAL_INT32 num);
#else
QOSAL_INT32 qcom_SSL_read(SSL *ssl, void *buf, QOSAL_INT32 num);
#endif
QOSAL_INT32 qcom_SSL_write(SSL *ssl, const void *buf, QOSAL_INT32 num);

int qcom_sendto(int s, char* buffer, int length, int flags, struct sockaddr* to, int tolen);
int qcom_send(int s, char* buffer, int length, int flags);
int qcom_socket_close(int s);
A_STATUS qcom_ip6_address_get(QOSAL_UINT8 device_id, QOSAL_UINT8 *v6Global, QOSAL_UINT8 *v6Link, QOSAL_UINT8 *v6DefGw, QOSAL_UINT8 *v6GlobalExtd, QOSAL_INT32 *LinkPrefix,
                              QOSAL_INT32 *GlbPrefix, QOSAL_INT32 *DefgwPrefix, QOSAL_INT32 *GlbPrefixExtd);
A_STATUS qcom_ping(QOSAL_UINT32 host, QOSAL_UINT32 size);
A_STATUS qcom_ping6(QOSAL_UINT8* host, QOSAL_UINT32 size);
A_STATUS qcom_ip6config_router_prefix(QOSAL_UINT8 device_id, QOSAL_UINT8 *addr, QOSAL_INT32 prefixlen, QOSAL_INT32 prefix_lifetime, QOSAL_INT32 valid_lifetime);
QOSAL_INT32 qcom_dhcps_set_pool(QOSAL_UINT8 device_id, QOSAL_UINT32 startip, QOSAL_UINT32 endip, QOSAL_INT32 leasetime);
A_STATUS qcom_http_server(QOSAL_INT32 enable);
A_STATUS qcom_http_set_post_cb(void* cxt, void *callback);
A_STATUS qcom_ip_http_server_method(QOSAL_INT32 cmd, QOSAL_UINT8 *pagename, QOSAL_UINT8 *objname, QOSAL_INT32 objtype, QOSAL_INT32 objlen, QOSAL_UINT8 * value);
A_STATUS qcom_http_client_method(QOSAL_UINT32 cmd, QOSAL_UINT8* url, QOSAL_UINT8 *data, QOSAL_UINT8 *value);
A_STATUS qcom_ip6_route(QOSAL_UINT8 device_id, QOSAL_UINT32 cmd, QOSAL_UINT8* ip6addr, QOSAL_UINT32* prefixLen, QOSAL_UINT8* gw, QOSAL_UINT32* ifindex, IPV6_ROUTE_LIST_T *rtlist);
A_STATUS qcom_ip4_route(QOSAL_UINT8 device_id, QOSAL_UINT32 cmd, QOSAL_UINT32* addr, QOSAL_UINT32* subnet, QOSAL_UINT32* gw, QOSAL_UINT32* ifindex, IPV4_ROUTE_LIST_T *rtlist);
A_STATUS qcom_tcp_conn_timeout(QOSAL_UINT32 timeout_val);
A_STATUS qcom_tcp_set_exp_backoff(QOSAL_UINT8 device_id, QOSAL_INT32 retry);
A_STATUS qcom_dhcps_release_pool(QOSAL_UINT8 device_id);
A_STATUS qcom_bridge_mode_enable(QOSAL_UINT16 bridgemode);
QOSAL_UINT32 qcom_dhcps_register_cb(QOSAL_UINT8 device_id, QOSAL_VOID *callback);
QOSAL_UINT32 qcom_dhcpc_register_cb(QOSAL_UINT8 device_id, QOSAL_VOID *callback);
#if ENABLE_DNS_CLIENT
A_STATUS qcom_dnsc_enable(QOSAL_BOOL enable);
A_STATUS qcom_dnsc_add_server_address(QOSAL_UINT8 *ipaddress, QOSAL_UINT8 type);
A_STATUS qcom_dnsc_del_server_address(QOSAL_UINT8 *ipaddress, QOSAL_UINT8 type);
A_STATUS qcom_dnsc_get_host_by_name(QOSAL_CHAR *pname, QOSAL_UINT32 *pipaddress);
A_STATUS qcom_dnsc_get_host_by_name2(QOSAL_CHAR *pname, QOSAL_UINT32 *pipaddress, QOSAL_INT32 domain, QOSAL_UINT32 mode);
#endif

#if ENABLE_DNS_SERVER
A_STATUS qcom_dns_server_address_get(QOSAL_UINT32 pdns[], QOSAL_UINT32* number);
A_STATUS qcom_dnss_enable(QOSAL_BOOL enable);
A_STATUS qcom_dns_local_domain(QOSAL_CHAR* local_domain);
A_STATUS qcom_dns_entry_create(QOSAL_CHAR* hostname, QOSAL_UINT32 ipaddress);
A_STATUS qcom_dns_entry_delete(QOSAL_CHAR* hostname, QOSAL_UINT32 ipaddress);
A_STATUS qcom_dns_6entry_create(QOSAL_CHAR* hostname, QOSAL_UINT8* ip6addr);
A_STATUS qcom_dns_6entry_delete(QOSAL_CHAR* hostname, QOSAL_UINT8* ip6addr);
#endif

#if ENABLE_SNTP_CLIENT
void qcom_sntp_srvr_addr(int flag, char* srv_addr);
void qcom_sntp_get_time(QOSAL_UINT8 device_id, tSntpTime* time);
void qcom_sntp_get_time_of_day(QOSAL_UINT8 device_id, tSntpTM* time);
void qcom_sntp_zone(int hour, int min, int add_sub, int enable);
void qcom_sntp_query_srvr_address(QOSAL_UINT8 device_id, tSntpDnsAddr* addr);
void qcom_enable_sntp_client(int enable);
#endif

A_STATUS qcom_ota_upgrade(QOSAL_UINT8 device_id, QOSAL_UINT32 addr, QOSAL_CHAR *filename, QOSAL_UINT8 mode, QOSAL_UINT8 preserve_last, QOSAL_UINT8 protocol, QOSAL_UINT32 *resp_code, QOSAL_UINT32 *length);
A_STATUS qcom_ota_read_area(QOSAL_UINT32 offset, QOSAL_UINT32 size, QOSAL_UCHAR * buffer, QOSAL_UINT32 *ret_len);
A_STATUS qcom_ota_done(QOSAL_BOOL good_image);
QCOM_OTA_STATUS_CODE_t qcom_ota_session_start(QOSAL_UINT32 flags, QOSAL_UINT32 partition_index);
QOSAL_UINT32 qcom_ota_partition_get_size(QOSAL_VOID);
QCOM_OTA_STATUS_CODE_t qcom_ota_partition_erase(QOSAL_VOID);
QCOM_OTA_STATUS_CODE_t qcom_ota_parse_image_hdr(QOSAL_UINT8 *header, QOSAL_UINT32 *offset);
QCOM_OTA_STATUS_CODE_t qcom_ota_partition_verify_checksum(QOSAL_VOID);
QOSAL_INT32 qcom_ota_partition_read_data(QOSAL_UINT32 offset, QOSAL_UINT8 *buf, QOSAL_UINT32 size);
QCOM_OTA_STATUS_CODE_t qcom_ota_partition_write_data(QOSAL_UINT32 offset, QOSAL_UINT8 *buf, QOSAL_UINT32 size, QOSAL_UINT32 *ret_size);
QCOM_OTA_STATUS_CODE_t qcom_ota_session_end(QOSAL_UINT32 good_image);
QCOM_OTA_STATUS_CODE_t qcom_ota_partition_format(QOSAL_UINT32 partition_index);
A_STATUS qcom_ota_set_callback(QOSAL_VOID *callback);
#endif

#define qcom_set_scan(device_id, pScanParams) _qcom_set_scan(device_id, pScanParams)
A_STATUS _qcom_set_scan(QOSAL_UINT8 device_id, qcom_start_scan_params_t* pScanParams);
A_STATUS qcom_get_scan(QOSAL_UINT8 device_id, QCOM_BSS_SCAN_INFO** buf, QOSAL_INT16* numResults);
A_STATUS qcom_set_ssid(QOSAL_UINT8 device_id, QOSAL_CHAR *pssid);
A_STATUS qcom_get_ssid(QOSAL_UINT8 device_id, QOSAL_CHAR *pssid);
A_STATUS qcom_scan_set_mode(QOSAL_UINT8 device_id, QOSAL_UINT32 mode);
A_STATUS qcom_set_connect_callback (QOSAL_UINT8 device_id, void *callback);
A_STATUS qcom_commit(QOSAL_UINT8 device_id);
A_STATUS qcom_sta_get_rssi(QOSAL_UINT8 device_id, QOSAL_UINT8 *prssi);
A_STATUS qcom_sta_set_listen_time(QOSAL_UINT8 device_id, QOSAL_UINT32 listentime);
A_STATUS qcom_ap_set_beacon_interval(QOSAL_UINT8 device_id, QOSAL_UINT16 beacon_interval);
A_STATUS qcom_ap_hidden_mode_enable(QOSAL_UINT8 device_id, QOSAL_BOOL enable);
A_STATUS qcom_op_get_mode(QOSAL_UINT8 device_id, QOSAL_UINT32 *pmode);
A_STATUS qcom_op_set_mode(QOSAL_UINT8 device_id, QOSAL_UINT32 mode);
A_STATUS qcom_disconnect(QOSAL_UINT8 device_id);
A_STATUS qcom_set_phy_mode(QOSAL_UINT8 device_id, QOSAL_UINT8 phymode);
A_STATUS qcom_get_phy_mode(QOSAL_UINT8 device_id, QOSAL_UINT8 *pphymode);
A_STATUS qcom_set_channel(QOSAL_UINT8 device_id, QOSAL_UINT16 channel);
A_STATUS qcom_get_channel(QOSAL_UINT8 device_id, QOSAL_UINT16 *pchannel);
A_STATUS qcom_set_tx_power(QOSAL_UINT8 device_id, QOSAL_UINT32 dbm);
A_STATUS qcom_allow_aggr_set_tid(QOSAL_UINT8 device_id, QOSAL_UINT16 tx_allow_aggr, QOSAL_UINT16 rx_allow_aggr);
A_STATUS qcom_sec_set_wepkey(QOSAL_UINT8 device_id, QOSAL_UINT32 keyindex, QOSAL_CHAR *pkey);
A_STATUS qcom_sec_get_wepkey(QOSAL_UINT8 device_id, QOSAL_UINT32 keyindex, QOSAL_CHAR *pkey);
A_STATUS qcom_sec_set_wepkey_index(QOSAL_UINT8 device_id, QOSAL_UINT32 keyindex);
A_STATUS qcom_sec_get_wepkey_index(QOSAL_UINT8 device_id, QOSAL_UINT32 *pkeyindex);
A_STATUS qcom_sec_set_auth_mode(QOSAL_UINT8 device_id, QOSAL_UINT32 mode);
A_STATUS qcom_sec_set_encrypt_mode(QOSAL_UINT8 device_id, QOSAL_UINT32 mode);
A_STATUS qcom_sec_set_passphrase(QOSAL_UINT8 device_id, QOSAL_CHAR *passphrase);
A_STATUS qcom_sec_set_pmk(QOSAL_UINT8 device_id, QOSAL_CHAR *pmk);

A_STATUS qcom_power_set_mode(QOSAL_UINT8 device_id, QOSAL_UINT32 powerMode, QOSAL_UINT8 powerModule);
A_STATUS qcom_power_get_mode(QOSAL_UINT8 device_id, QOSAL_UINT32 *powerMode);
A_STATUS qcom_suspend_enable(QOSAL_BOOL enable);
A_STATUS qcom_suspend_start(QOSAL_UINT32 susp_time);
A_STATUS qcom_power_set_parameters(QOSAL_UINT8 device_id,
                                   QOSAL_UINT16 idlePeriod,
                                   QOSAL_UINT16 psPollNum,
                                   QOSAL_UINT16 dtimPolicy,
                                   QOSAL_UINT16 tx_wakeup_policy,
                                   QOSAL_UINT16 num_tx_to_wakeup,
                                   QOSAL_UINT16 ps_fail_event_policy);
A_STATUS qcom_get_bssid(QOSAL_UINT8 device_id, QOSAL_UINT8 mac_addr[ATH_MAC_LEN]);

A_STATUS qcom_sec_get_auth_mode(QOSAL_UINT8 device_id, QOSAL_UINT32 *pmode);
A_STATUS qcom_sec_get_encrypt_mode(QOSAL_UINT8 device_id, QOSAL_UINT32 *pmode);
A_STATUS qcom_sec_get_passphrase(QOSAL_UINT8 device_id, QOSAL_CHAR *passphrase);

A_STATUS qcom_wps_start(QOSAL_UINT8 device_id, int connect, int use_pinmode, char *pin);
A_STATUS qcom_wps_connect(QOSAL_UINT8 device_id);
A_STATUS qcom_wps_set_credentials(QOSAL_UINT8 device_id, qcom_wps_credentials_t *pwps_prof);

#if ENABLE_P2P_MODE
A_STATUS qcom_p2p_func_init(QOSAL_UINT8 device_id, QOSAL_INT32 enable);
A_STATUS qcom_p2p_func_cancel(QOSAL_UINT8 device_id);
A_STATUS qcom_p2p_func_set_pass_ssid(QOSAL_UINT8 device_id, QOSAL_CHAR *ppass, QOSAL_CHAR *pssid);
A_STATUS qcom_p2p_func_auth(QOSAL_UINT8 device_id, QOSAL_INT32 dev_auth, P2P_WPS_METHOD wps_method, QOSAL_UINT8 *peer_mac, QOSAL_BOOL persistent);
A_STATUS qcom_p2p_func_connect(QOSAL_UINT8 device_id, P2P_WPS_METHOD wps_method, QOSAL_UINT8 *peer_mac, QOSAL_BOOL persistent);
A_STATUS qcom_p2p_func_set_config(QOSAL_UINT8 device_id, QOSAL_UINT32 uigo_intent, QOSAL_UINT32 uiclisten_ch, QOSAL_UINT32 uiop_ch, QOSAL_UINT32 uiage, QOSAL_UINT32 reg_class, QOSAL_UINT32 op_reg_class, QOSAL_UINT32 max_node_count);
A_STATUS qcom_p2p_func_set_oppps(QOSAL_UINT8 device_id, QOSAL_UINT8 enable, QOSAL_UINT8 ctwin);
A_STATUS qcom_p2p_func_set_noa(QOSAL_UINT8 device_id, QOSAL_UINT8 uccount, QOSAL_UINT32 uistart, QOSAL_UINT32 uiduration, QOSAL_UINT32 uiinterval);
A_STATUS qcom_p2p_func_invite(QOSAL_UINT8 device_id, QOSAL_CHAR *pssid, P2P_WPS_METHOD wps_method, QOSAL_UINT8 *pmac, QOSAL_BOOL persistent, P2P_INV_ROLE role);
A_STATUS qcom_p2p_func_find(QOSAL_UINT8 device_id, void *dev, QOSAL_UINT8 type, QOSAL_UINT32 timeout);
A_STATUS qcom_p2p_func_stop_find(QOSAL_UINT8 device_id);
A_STATUS qcom_p2p_func_get_node_list(QOSAL_UINT8 device_id, void *app_buf, QOSAL_UINT32 buf_size);
A_STATUS qcom_p2p_func_get_network_list(QOSAL_UINT8 device_id, void *app_buf, QOSAL_UINT32 buf_size);
A_STATUS qcom_p2p_func_invite_auth(QOSAL_UINT8 device_id, QOSAL_UINT8 *inv);
A_STATUS qcom_p2p_func_listen(QOSAL_UINT8 device_id, QOSAL_UINT32 timeout);
A_STATUS qcom_p2p_func_join_profile(QOSAL_UINT8 device_id, QOSAL_UINT8 *pmac);
A_STATUS qcom_p2p_set(QOSAL_UINT8 device_id, P2P_CONF_ID config_id, void *data, QOSAL_UINT32 data_length);
A_STATUS qcom_p2p_func_prov(QOSAL_UINT8 device_id, P2P_WPS_METHOD wps_method, QOSAL_UINT8 *pmac);
A_STATUS qcom_p2p_func_join(QOSAL_UINT8 device_id, P2P_WPS_METHOD wps_method, QOSAL_UINT8 *pmac, char *ppin, QOSAL_UINT16 channel);
A_STATUS qcom_p2p_func_start_go(QOSAL_UINT8 device_id, QOSAL_UINT8 *pssid, QOSAL_UINT8 *ppass, QOSAL_INT32 channel, QOSAL_BOOL ucpersistent);
#endif
A_STATUS qcom_raw_mode_send_pkt(QCOM_RAW_MODE_PARAM_t *ppara);
A_STATUS qcom_scan_params_set(QOSAL_UINT8 device_id, qcom_scan_params_t *pParam);
A_STATUS qcom_param_set(QOSAL_UINT8 device_id, QOSAL_UINT16 grp_id, QOSAL_UINT16 param_id, void * data, QOSAL_UINT32 data_length, QOSAL_BOOL wait_for_status);
A_STATUS qcom_roaming_ctrl(A_UINT8 device_id, WMI_SET_ROAM_CTRL_CMD *roam_ctrl);
A_STATUS qcom_get_version(QOSAL_UINT32 *host_ver, QOSAL_UINT32 *target_ver, QOSAL_UINT32 *wlan_ver, QOSAL_UINT32 *abi_ver);
A_STATUS qcom_ipconfig_set_ip6_status(QOSAL_UINT8  device_id, QOSAL_BOOL enable);
#endif
