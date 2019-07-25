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
//                                                                ==============================================================================
//                                                                 Author(s): ="Atheros"
//                                                                ==============================================================================

#ifndef _WMI_API_H_
#define _WMI_API_H_

#ifdef __cplusplus
extern "C" {
#endif

/* WMI converts a dix frame with an ethernet payload (up to 1500 bytes)
* to an 802.3 frame (adds SNAP header) and adds on a WMI data header */
#define WMI_MAX_TX_DATA_FRAME_LENGTH (1500 + sizeof(WMI_DATA_HDR) + WMI_MAX_TX_META_SZ + sizeof(ATH_MAC_HDR) + sizeof(ATH_LLC_SNAP_HDR))

/* A normal WMI data frame */
#define WMI_MAX_NORMAL_RX_DATA_FRAME_LENGTH (1500 + HTC_HEADER_LEN + sizeof(WMI_DATA_HDR) + WMI_MAX_RX_META_SZ + sizeof(ATH_MAC_HDR) + sizeof(ATH_LLC_SNAP_HDR))

/* An AMSDU frame */
#define WMI_MAX_AMSDU_RX_DATA_FRAME_LENGTH  (4096 + sizeof(WMI_DATA_HDR) + sizeof(ATH_MAC_HDR) + sizeof(ATH_LLC_SNAP_HDR))

/*
 * IP QoS Field definitions according to 802.1p
 */
#define BEST_EFFORT_PRI         0
#define BACKGROUND_PRI          1
#define EXCELLENT_EFFORT_PRI    3
#define CONTROLLED_LOAD_PRI     4
#define VIDEO_PRI               5
#define VOICE_PRI               6
#define NETWORK_CONTROL_PRI     7
#define MAX_NUM_PRI             8

#define UNDEFINED_PRI           (0xff)

#define WMI_IMPLICIT_PSTREAM_INACTIVITY_INT 5000 /* 5 seconds */

#define A_ROUND_UP(x, y)  ((((x) + ((y) - 1)) / (y)) * (y))

typedef enum {
  ATHEROS_COMPLIANCE = 0x1
}TSPEC_PARAM_COMPLIANCE;

struct wmi_t;

void *wmi_init(void *devt);

void wmi_qos_state_init(struct wmi_t *wmip);
void wmi_shutdown(struct wmi_t *wmip);
HTC_ENDPOINT_ID wmi_get_control_ep(struct wmi_t * wmip);
void wmi_set_control_ep(struct wmi_t * wmip, HTC_ENDPOINT_ID eid);
A_STATUS wmi_dix_2_dot3(struct wmi_t *wmip, void *osbuf);
A_STATUS wmi_meta_add(struct wmi_t *wmip, void *osbuf, QOSAL_UINT8 *pVersion, void *pTxMetaS);
A_STATUS wmi_data_hdr_add(struct wmi_t *wmip, void *osbuf, QOSAL_UINT8 msgType, QOSAL_BOOL bMoreData, WMI_DATA_HDR_DATA_TYPE data_type, QOSAL_UINT8 metaVersion, void *pTxMetaS);
A_STATUS wmi_dot3_2_dix(void *osbuf);

A_STATUS wmi_dot11_hdr_remove (struct wmi_t *wmip, void *osbuf);
A_STATUS wmi_dot11_hdr_add(struct wmi_t *wmip, void *osbuf, NETWORK_TYPE mode);

A_STATUS wmi_data_hdr_remove(struct wmi_t *wmip, void *osbuf);

QOSAL_UINT8 wmi_implicit_create_pstream(struct wmi_t *wmip, void *osbuf, QOSAL_UINT32 layer2Priority, QOSAL_BOOL wmmEnabled);

A_STATUS wmi_control_rx(struct wmi_t *wmip, void *osbuf);

typedef enum {
  NO_SYNC_WMIFLAG = 0,
  SYNC_BEFORE_WMIFLAG,              /* transmit all queued data before cmd */
  SYNC_AFTER_WMIFLAG,               /* any new data waits until cmd execs */
  SYNC_BOTH_WMIFLAG,
  END_WMIFLAG                       /* end marker */
} WMI_SYNC_FLAG;

A_STATUS
wmi_cmd_start(struct wmi_t *wmip, QOSAL_CONST QOSAL_VOID *pInput, WMI_COMMAND_ID cmdID, QOSAL_UINT16 buffsize);

A_STATUS wmi_cmd_send(struct wmi_t *wmip, void *osbuf, WMI_COMMAND_ID cmdId,
                      WMI_SYNC_FLAG flag);

A_STATUS wmi_bssfilter_cmd(struct wmi_t *wmip, QOSAL_UINT8 filter, QOSAL_UINT32 ieMask);

#if WLAN_CONFIG_ENABLE_WMI_SYNC
A_STATUS wmi_dataSync_send(struct wmi_t *wmip, void *osbuf, HTC_ENDPOINT_ID eid);
#endif

A_STATUS
wmi_storerecall_recall_cmd(struct wmi_t *wmip, QOSAL_UINT32 length, void* pData);

A_STATUS wmi_socket_cmd(struct wmi_t *wmip, QOSAL_UINT32 cmd_type, void* pData, QOSAL_UINT32 length);

#if ENABLE_P2P_MODE
void
wmi_save_key_info(WMI_P2P_PROV_INFO *p2p_info);

void
p2p_go_neg_complete_rx(void *ctx, const QOSAL_UINT8 *datap, QOSAL_UINT8 len);

A_STATUS
wmi_p2p_set_noa(struct wmi_t *wmip, WMI_NOA_INFO_STRUCT *buf);

A_STATUS
wmi_p2p_set_oppps(struct wmi_t *wmip, WMI_OPPPS_INFO_STRUCT *pOpp);

A_STATUS wmi_sdpd_send_cmd(struct wmi_t *wmip, WMI_P2P_SDPD_TX_CMD *buf);
#endif

#if ENABLE_AP_MODE
A_STATUS
wmi_ap_set_param(struct wmi_t *wmip, QOSAL_VOID *data);
#endif

A_STATUS wmi_reverse_credit_cmd (QOSAL_VOID* handle, QOSAL_BOOL enable, QOSAL_UINT8 *endpoints, QOSAL_UINT8 *credits);
A_STATUS wmi_rcv_data_classifier_cmd (QOSAL_VOID* handle, QOSAL_UINT8 offset, QOSAL_UINT8 shift, QOSAL_UINT32 mask, QOSAL_UINT8 count,
                                      QOSAL_UINT32 *class_mapping, QOSAL_UINT8 *ep_mapping);
A_STATUS wmi_update_reverse_credits_cmd (QOSAL_VOID* handle, QOSAL_UINT8 endpoint, QOSAL_UINT32 nCredits);

#ifdef __cplusplus
}
#endif

#endif /* _WMI_API_H_ */
