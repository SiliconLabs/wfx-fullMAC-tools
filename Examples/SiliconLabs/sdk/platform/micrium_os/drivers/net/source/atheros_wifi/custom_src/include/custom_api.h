//                                                                ------------------------------------------------------------------------------
//                                                                 Copyright (c) 2011 Qualcomm Atheros, Inc.
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
#ifndef _CUSTOM_API_H_
#define _CUSTOM_API_H_

#include <wmi.h>

A_VOID
Custom_HW_SetClock(A_VOID *pCxt, A_UINT32 *pClockRate);
A_VOID
Custom_HW_EnableDisableSPIIRQ(A_VOID *pCxt, A_BOOL enable);
A_VOID
Custom_HW_UsecDelay(A_VOID *pCxt, A_UINT32 uSeconds);
A_VOID
Custom_HW_PowerUpDown(A_VOID *pCxt, A_UINT32 powerUp);
A_STATUS
Custom_HW_Init(A_VOID *pCxt);
A_STATUS
Custom_HW_DeInit(A_VOID *pCxt);

A_STATUS
Custom_Bus_InOutBuffer(A_VOID *pCxt,
                       A_UINT8 *pBuffer,
                       A_UINT16 length,
                       A_UINT8 doRead,
                       A_BOOL sync);
A_STATUS
Custom_Bus_InOutToken(A_VOID *pCxt,
                      A_UINT32        OutToken,
                      A_UINT8         DataSize,
                      A_UINT32    *pInToken);
A_STATUS
Custom_Bus_StartTransfer(A_VOID *pCxt, A_BOOL sync);
A_STATUS
Custom_Bus_CompleteTransfer(A_VOID *pCxt, A_BOOL sync);
A_VOID
Custom_HW_InterruptHandler(A_VOID *pointer);

A_STATUS
Custom_Driver_WaitForCondition(A_VOID *pCxt, volatile A_BOOL *pCond,
                               A_BOOL value, A_UINT32 msec);
A_STATUS
Custom_Driver_CreateThread(A_VOID *pCxt);
A_STATUS
Custom_Driver_DestroyThread(A_VOID *pCxt);
A_STATUS
Custom_Driver_ContextInit(A_VOID *pCxt);
A_VOID
Custom_Driver_ContextDeInit(A_VOID *pCxt);
A_VOID *
Custom_GetRxRequest(A_VOID *pCxt, A_UINT16 length);
A_VOID
Custom_Driver_WakeDriver(A_VOID *pCxt);
A_VOID
Custom_Driver_WakeUser(A_VOID *pCxt);
A_VOID
Custom_DeliverFrameToNetworkStack(A_VOID *pCxt, A_VOID *pReq);

A_VOID
Custom_Api_BssInfoEvent(A_VOID *pCxt, A_UINT8 *datap, A_INT32 len);

A_VOID
Custom_Api_ConnectEvent(A_VOID *pCxt, A_UINT8 devId, A_UINT16 channel, A_UINT8 *bssid);
A_VOID
Custom_Api_DisconnectEvent(A_VOID *pCxt, A_UINT8 devId, A_UINT8 reason, A_UINT8 *bssid,
                           A_UINT8 assocRespLen, A_UINT8 *assocInfo, A_UINT16 protocolReasonStatus);
A_VOID
Custom_Api_RSNASuccessEvent(A_VOID *pCxt, A_UINT8 devid, A_UINT8 code);
A_VOID
Custom_Api_ReadyEvent(A_VOID *pCxt, A_UINT8 *datap, A_UINT8 phyCap, A_UINT32 sw_ver, A_UINT32 abi_ver);
A_VOID
Custom_Api_BitRateEvent_tx(A_VOID *pCxt, A_UINT8 devId, A_INT8 rateIndex);
#if MANUFACTURING_SUPPORT
A_VOID
Custom_Api_Test_Cmd_Event(A_VOID *pCxt, A_UINT8 *datap, A_UINT32 len);
#endif
#if ENABLE_P2P_MODE
A_VOID
Custom_Api_p2p_go_neg_event(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_UINT32 len, WMI_P2P_PROV_INFO *wps_info);

A_VOID
Custom_Api_p2p_node_list_event(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_UINT32 len);

A_VOID
Custom_Api_p2p_list_persistent_network_event(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_UINT32 len);

A_VOID
Custom_Api_p2p_req_auth_event(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_UINT32 len);

A_VOID
Custom_Api_get_p2p_ctx(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_UINT32 len);

A_VOID
Custom_Api_p2p_prov_disc_req(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_UINT32 len);
A_VOID
Custom_Api_p2p_serv_disc_req(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_UINT32 len);
A_VOID
Custom_Api_p2p_invite_req(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_UINT32 len);
A_VOID
Custom_Api_p2p_invite_rcvd_result(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_UINT32 len);
A_VOID
Custom_Api_p2p_invite_send_result(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_UINT32 len);
A_VOID
Custom_Api_wps_profile_event_rx(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_INT32 len, A_VOID *pReq);

A_VOID
Custom_Api_PfmDataEvent(A_VOID *wmip, A_UINT8 *datap, A_INT32 len);
A_VOID
Custom_Api_PfmDataDoneEvent(A_VOID *wmip, A_UINT8 *datap, A_INT32 len);

#endif

/* COMMON -> CUSTOM APIs :: These macros are used by common code to call into custom code.
 *      For a given system many of these will resolve to function calls however for some it
 *      may be appropriate to simply define them as A_OK. */
#define CUSTOM_API_TXCOMPLETE(pCxt, pReq) UNUSED_ARGUMENT((pCxt)); UNUSED_ARGUMENT((pReq));
#define CUSTOM_DELIVER_FRAME(pCxt, pReq) UNUSED_ARGUMENT((pCxt)); UNUSED_ARGUMENT((pReq));
#define CUSTOM_API_TARGET_STATS_EVENT(pCxt, ptr, len) Custom_Driver_WakeUser((pCxt))

#define A_CUSTOM_WMI_EVENT(evt, pCxt, datap, len, osbuf)  A_ERROR
#define CUSTOM_API_REGDOMAIN_EVENT(pCxt, regCode)
#define CUSTOM_WAIT_FOR_WMI_RESPONSE(pCxt)

#define CUSTOM_API_WMIX_DBGLOG_EVENT(wmip, datap, len)
#define CUSTOM_API_WMIX_GPIO_DATA_EVENT(wmip, datap, len)

#define CUSTOM_API_WMIX_PFM_DATA_EVENT(wmip, datap, len)
#define CUSTOM_API_WMIX_PFM_DATA_DONE_EVENT(wmip, datap, len)

#define CUSTOM_API_BSS_INFO_EVENT(pCxt, datap, len)
#define CUSTOM_API_TKIP_MIC_ERROR_EVENT(pCxt, keyid, ismcast)
#define CUSTOM_API_BITRATE_EVENT_TX(pCxt, devId, rateIndex)
#define CUSTOM_API_CONNECT_EVENT(pCxt, devId, a, b, c, d, e, f, g, h, i)
#define CUSTOM_API_DISCONNECT_EVENT(pCxt, devId, a, b, c, d, e)
#define CUSTOM_API_RSNA_SUCCESS_EVENT(pCxt, devId, code)
#define CUSTOM_API_AGGR_RECV_ADDBA_REQ_EVENT(pCxt, evt)
#define CUSTOM_API_AGGR_RECV_DELBA_REQ_EVENT(pCxt, evt)
#define CUSTOM_API_READY_EVENT(pCxt, datap, phyCap, sw_ver, abi_ver)
#define CUSTOM_HW_ENABLEDISABLESPIIRQ(pCxt, enable) Custom_HW_EnableDisableSPIIRQ((pCxt), (enable))
#define CUSTOM_HW_USEC_DELAY(pCxt, usec) Custom_HW_UsecDelay((pCxt), (usec))
#define CUSTOM_HW_POWER_UP_DOWN(pCxt, powerup) Custom_HW_PowerUpDown((pCxt), (powerup))
#define CUSTOM_HW_INIT(pCxt) Custom_HW_Init((pCxt))
#define CUSTOM_HW_DEINIT(pCxt) Custom_HW_DeInit((pCxt))
//                                                                #define CUSTOM_HW_SET_CLOCK(pCxt, pClockRate) Custom_HW_SetClock((pCxt), (pClockRate))
#define CUSTOM_BUS_INOUT_BUFFER(pCxt, pBuffer, length, doRead, sync) Custom_Bus_InOutBuffer((pCxt), (pBuffer), (length), (doRead), (sync))
#define CUSTOM_BUS_INOUT_TOKEN(pCxt, oT, dS, pInT) Custom_Bus_InOutToken((pCxt), (oT), (dS), (pInT))
#define CUSTOM_BUS_START_TRANSFER(pCxt, sync) Custom_Bus_StartTransfer((pCxt), (sync))
#define CUSTOM_BUS_COMPLETE_TRANSFER(pCxt, sync) Custom_Bus_CompleteTransfer((pCxt), (sync))
#define CUSTOM_DRIVER_INSERT(pCxt) A_OK
#define CUSTOM_DRIVER_REMOVE(pCxt)
#define CUSTOM_DRIVER_WAIT_FOR_CONDITION(pCxt, pC, v, ms) Custom_Driver_WaitForCondition((pCxt), (pC), (v), (ms))
#define CUSTOM_DRIVER_CXT_INIT(pCxt) Custom_Driver_ContextInit((pCxt))
#define CUSTOM_DRIVER_CXT_DEINIT(pCxt) Custom_Driver_ContextDeInit((pCxt))
#define CUSTOM_DRIVER_GET_RX_REQ(pCxt, l) Custom_GetRxRequest((pCxt), (l))
#if MANUFACTURING_SUPPORT
#define CUSTOM_API_TEST_RESP_EVENT(pCxt, datap, len)
#endif
#define CUSTOM_API_GET_TEMPERATURE_REPLY(pCxt, datap, len)
#if DRIVER_CONFIG_MULTI_TASKING
#define CUSTOM_API_SCAN_COMPLETE_EVENT(pCxt, status) Custom_Driver_WakeUser((pCxt))
#define CUSTOM_API_GET_PMK_EVENT(pCxt, datap) Custom_Driver_WakeUser((pCxt))
#define CUSTOM_API_BITRATE_EVENT(pCxt, rateKbps) Custom_Driver_WakeUser((pCxt))
#define CUSTOM_API_CHANNEL_LIST_EVENT(pCxt, numChan, chanList) Custom_Driver_WakeUser((pCxt))
#define CUSTOM_API_STORE_RECALL_EVENT(pCxt) Custom_Driver_WakeUser((pCxt))
#define CUSTOM_API_WPS_PROFILE_EVENT(pCxt, datap, len, pReq) Custom_Driver_WakeUser((pCxt))
#define CUSTOM_DRIVER_WAKE_DRIVER(pCxt) Custom_Driver_WakeDriver((pCxt))
#define CUSTOM_DRIVER_WAKE_USER(pCxt) Custom_Driver_WakeUser((pCxt))
#else
#define CUSTOM_API_SCAN_COMPLETE_EVENT(pCxt, status)
#define CUSTOM_API_GET_PMK_EVENT(pCxt, datap)
#define CUSTOM_API_BITRATE_EVENT(pCxt, rateKbps)
#define CUSTOM_API_CHANNEL_LIST_EVENT(pCxt, numChan, chanList)
#define CUSTOM_API_STORE_RECALL_EVENT(pCxt)
#define CUSTOM_API_WPS_PROFILE_EVENT(pCxt, datap, len, pReq)
#define CUSTOM_DRIVER_WAKE_DRIVER(pCxt)
#define CUSTOM_DRIVER_WAKE_USER(pCxt)
#endif

#define CUSTOM_OS_INT_ENABLE(pCxt) //TODO
#define CUSTOM_OS_INT_DISABLE(pCxt) //TODO

#if ENABLE_P2P_MODE
#define CUSTOM_API_P2P_GO_NEG_EVENT(devt, devId, res, len, wps_info)
#define CUSTOM_API_P2P_NODE_LIST_EVENT(devt, devId, res, len)
#define CUSTOM_API_P2P_LIST_PERSISTENT_NETWORK_EVENT(devt, devId, res, len)
#define CUSTOM_API_P2P_REQ_AUTH_EVENT(devt, devId, res, len)
#define CUSTOM_API_GET_P2P_CTX(devt, devId, res, len)
#define CUSTOM_API_P2P_PROV_DISC_REQ(devt, devId, res, len)
#define CUSTOM_API_P2P_SERV_DISC_REQ(devt, devId, res, len)
#define CUSTOM_API_P2P_INVITE_REQ(pCxt, devId, datap, len)
#define CUSTOM_API_P2P_INVITE_RCVD_RESULT(pCxt, devId, datap, len)
#define CUSTOM_API_P2P_INVITE_SEND_RESULT(pCxt, devId, datap, len)
//                                                                #define CUSTOM_API_WPS_PROFILE_EVENT_RX(pCxt, datap, len, pReq) Custom_Api_wps_profile_event_rx((pCxt), (datap), (len), (pReq))
#define CUSTOM_API_P2P_SAVE_WPS_CRED(pCxt)
#endif
#define CUSTOM_API_TEST_RESP_EVENT(pCxt, datap, len)

#endif /* _CUSTOM_API_H_ */
