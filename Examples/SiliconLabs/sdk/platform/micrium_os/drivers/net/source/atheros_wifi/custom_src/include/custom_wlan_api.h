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
#ifndef _CUSTOM_API_H_
#define _CUSTOM_API_H_

#include <wmi.h>
#include <wlan_api.h>

QOSAL_VOID
Custom_HW_PowerUpDown(QOSAL_VOID *pCxt, QOSAL_UINT32 powerUp);
/*1A_STATUS
   Custom_HW_Init(QOSAL_VOID *pCxt);
   A_STATUS
   Custom_HW_DeInit(QOSAL_VOID *pCxt);*/

QOSAL_VOID
custom_os_usec_delay(QOSAL_UINT32 uSeconds);

A_STATUS
Custom_Driver_CreateThread(QOSAL_VOID *pCxt);
A_STATUS
Custom_Driver_DestroyThread(QOSAL_VOID *pCxt);
A_STATUS
Custom_Driver_ContextInit(QOSAL_VOID *pCxt);
QOSAL_VOID
Custom_Driver_ContextDeInit(QOSAL_VOID *pCxt);
QOSAL_VOID *
Custom_GetRxRequest(QOSAL_VOID *pCxt, QOSAL_UINT16 length);
QOSAL_VOID
Custom_DeliverFrameToNetworkStack(QOSAL_VOID *pCxt, QOSAL_VOID *pReq);
QOSAL_VOID
Custom_Api_BssInfoEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 *datap, QOSAL_INT32 len);
QOSAL_VOID
Custom_Api_ConnectEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT16 channel, QOSAL_UINT8 *bssid);
QOSAL_VOID
Custom_Api_DisconnectEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 reason, QOSAL_UINT8 *bssid,
                           QOSAL_UINT8 assocRespLen, QOSAL_UINT8 *assocInfo, QOSAL_UINT16 protocolReasonStatus);
QOSAL_VOID
Custom_Api_RSNASuccessEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devid, QOSAL_UINT8 code);
QOSAL_VOID
Custom_Api_ReadyEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 *datap, QOSAL_UINT8 phyCap, QOSAL_UINT32 sw_ver, QOSAL_UINT32 abi_ver);
QOSAL_VOID
Custom_Api_BitRateEvent_tx(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_INT8 rateIndex);
QOSAL_VOID
Custom_Api_ProbeReq_Event(QOSAL_VOID *pCxt, QOSAL_UINT8 *datap, QOSAL_UINT32 len);

A_STATUS
Custom_HW_Init(QOSAL_VOID *pCxt);
A_STATUS
Custom_HW_DeInit(QOSAL_VOID *pCxt);

QOSAL_VOID* Custom_Api_GetDriverCxt(QOSAL_UINT8 device_id);

#define CUSTOM_HW_INIT(pCxt) Custom_HW_Init((pCxt))
#define CUSTOM_HW_DEINIT(pCxt) Custom_HW_DeInit((pCxt))

QOSAL_VOID
Custom_Api_PfmDataEvent(QOSAL_VOID *wmip, QOSAL_UINT8 *datap, QOSAL_INT32 len);
QOSAL_VOID
Custom_Api_PfmDataDoneEvent(QOSAL_VOID *wmip, QOSAL_UINT8 *datap, QOSAL_INT32 len);
QOSAL_VOID
Custom_Api_Http_Post_Event(QOSAL_VOID *pCxt, QOSAL_UINT8 *datap);

#define CUSTOM_API_WMIX_PFM_DATA_EVENT(wmip, datap, len) Custom_Api_PfmDataEvent((wmip), (datap), (len))
#define CUSTOM_API_WMIX_PFM_DATA_DONE_EVENT(wmip, datap, len)  Custom_Api_PfmDataDoneEvent((wmip), (datap), (len))

#if ENABLE_P2P_MODE
//                                                                QOSAL_VOID
//                                                                Custom_Api_wps_profile_event_rx(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_INT32 len, QOSAL_VOID *pReq);

QOSAL_VOID
Api_GpioDataEvent(QOSAL_VOID *wmip, QOSAL_UINT8 *datap, QOSAL_INT32 len);//IGX_UD_CHANGES

#endif

/* COMMON -> CUSTOM APIs :: These macros are used by common code to call into custom code.
 *      For a given system many of these will resolve to function calls however for some it
 *      may be appropriate to simply define them as A_OK. */
#define CUSTOM_DELIVER_FRAME(pCxt, pReq) UNUSED_ARGUMENT((pCxt)); UNUSED_ARGUMENT((pReq));

QOSAL_VOID Custom_Api_GpioDataEvent(QOSAL_VOID *wmip, QOSAL_UINT8 *datap, QOSAL_INT32 len);
#define CUSTOM_API_WMIX_GPIO_DATA_EVENT(wmip, datap, len) Custom_Api_GpioDataEvent((wmip), (datap), (len))
#define CUSTOM_API_BSS_INFO_EVENT(pCxt, datap, len) Custom_Api_BssInfoEvent((pCxt), (datap), (len))

#define CUSTOM_API_PROBEREQ_EVENT(pCxt, datap, len) Custom_Api_ProbeReq_Event((pCxt), (datap), (len))

#define CUSTOM_API_BITRATE_EVENT_TX(pCxt, devId, rateIndex) Custom_Api_BitRateEvent_tx((pCxt), (devId), (rateIndex))
#define CUSTOM_API_CONNECT_EVENT(pCxt, devId, channel, bssid, c, d, e, f, g, h, i) Custom_Api_ConnectEvent((pCxt), (devId), (channel), (bssid))
#define CUSTOM_API_DISCONNECT_EVENT(pCxt, devId, a, b, c, d, e) Custom_Api_DisconnectEvent((pCxt), (devId), (a), (b), (c), (d), (e))
#define CUSTOM_API_RSNA_SUCCESS_EVENT(pCxt, devId, code) Custom_Api_RSNASuccessEvent((pCxt), (devId), (code))

#define CUSTOM_API_READY_EVENT(pCxt, datap, phyCap, sw_ver, abi_ver) Custom_Api_ReadyEvent((pCxt), (datap), (phyCap), (sw_ver), (abi_ver))

#define CUSTOM_HW_POWER_UP_DOWN(pCxt, powerup) Custom_HW_PowerUpDown((pCxt), (powerup))
//                                                                #define CUSTOM_HW_INIT(pCxt) Custom_HW_Init((pCxt))
//                                                                #define CUSTOM_HW_DEINIT(pCxt) Custom_HW_DeInit((pCxt))

#define CUSTOM_DRIVER_CXT_INIT(pCxt) Custom_Driver_ContextInit((pCxt))
#define CUSTOM_DRIVER_CXT_DEINIT(pCxt) Custom_Driver_ContextDeInit((pCxt))
#define CUSTOM_DRIVER_GET_RX_REQ(pCxt, l) Custom_GetRxRequest((pCxt), (l))
#define CUSTOM_API_HTTP_POST_EVENT(pCxt, datap)     Custom_Api_Http_Post_Event((pCxt), (datap))
#define CUSTOM_API_OTA_RESP_RESULT(pCxt, cmd, resp_code, result)     Custom_Api_Ota_Resp_Result((pCxt), (cmd), (resp_code), (result))
#define CUSTOM_API_DHCPS_SUCCESS_CALLBACK_EVENT(pCxt, datap)     Custom_Api_Dhcps_Success_Callback_Event((pCxt), (datap))
#define CUSTOM_API_DHCPC_SUCCESS_CALLBACK_EVENT(pCxt, datap)     Custom_Api_Dhcpc_Success_Callback_Event((pCxt), (datap))
#if DRIVER_CONFIG_MULTI_TASKING

#define CUSTOM_API_SCAN_COMPLETE_EVENT(pCxt, status)

#else
#define CUSTOM_API_SCAN_COMPLETE_EVENT(pCxt, status)
#define CUSTOM_API_GET_PMK_EVENT(pCxt, datap)
#define CUSTOM_API_BITRATE_EVENT(pCxt, rateKbps)
#define CUSTOM_API_CHANNEL_LIST_EVENT(pCxt, numChan, chanList)
#define CUSTOM_API_STORE_RECALL_EVENT(pCxt)
#define CUSTOM_API_WPS_PROFILE_EVENT(pCxt, datap, len, pReq)
#endif

#if ENABLE_P2P_MODE
//                                                                #define CUSTOM_API_WPS_PROFILE_EVENT_RX(pCxt, datap, len, pReq) Custom_Api_wps_profile_event_rx((pCxt), (datap), (len), (pReq))
//                                                                #define CUSTOM_API_P2P_SAVE_WPS_CRED(pCxt)    Custom_Api_p2p_save_wps_credentials((pCxt))
#endif

typedef QOSAL_VOID (*CUSTOM_HW_INTR_CB)(QOSAL_VOID*);

QOSAL_VOID Custom_HW_InterruptHandler(CUSTOM_HW_INTR_CB pHcdCallback, QOSAL_VOID *pHcdContext);

#endif /* _CUSTOM_API_H_ */
