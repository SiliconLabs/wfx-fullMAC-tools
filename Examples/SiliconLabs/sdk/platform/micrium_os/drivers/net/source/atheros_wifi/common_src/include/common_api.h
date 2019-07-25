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
#ifndef _COMMON_API_H_
#define _COMMON_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <a_config.h>
#include <a_types.h>
#include <a_osapi.h>
#include <htc_api.h>
#include <driver_cxt.h>

//                                                                IGX_UD_CHANGES
typedef struct hcd_intr_callback_t{
  QOSAL_VOID* pCxt;
}HCD_INTR_CB, *HCD_INTR_CALLBACK_T;

A_STATUS
Driver_CreateThread(QOSAL_VOID *pCxt);
A_STATUS
Driver_DestroyThread(QOSAL_VOID *pCxt);
QOSAL_VOID
HW_EnableDisableIRQ(QOSAL_VOID *pCxt, QOSAL_BOOL Enable, QOSAL_BOOL FromIrq);
A_STATUS
HW_ProcessPendingInterrupts(QOSAL_VOID *pCxt);
A_STATUS
wmi_dset_host_cfg_cmd (QOSAL_VOID* handle);
A_STATUS
Driver_Init(QOSAL_VOID *pCxt);
A_STATUS
Driver_DeInit(QOSAL_VOID *pCxt);
A_STATUS
Driver_ContextInit(QOSAL_VOID *pCxt);
A_STATUS
Driver_ContextDeInit(QOSAL_VOID *pCxt);
QOSAL_UINT32
Htc_ReadCreditCounter(QOSAL_VOID *pCxt, QOSAL_UINT32 index);
QOSAL_VOID
Htc_GetCreditCounterUpdate(QOSAL_VOID *pCxt, QOSAL_UINT16 epid);
QOSAL_VOID
Htc_PrepareRecvPacket(QOSAL_VOID *pCxt, QOSAL_VOID *pReq);
A_STATUS
Htc_SendPacket(QOSAL_VOID *pCxt, QOSAL_VOID *pReq);
QOSAL_VOID
Htc_RxComplete(QOSAL_VOID *pCxt, QOSAL_VOID *pReq);
A_STATUS
Htc_ProcessTxComplete(QOSAL_VOID *pCxt, QOSAL_VOID *pReq);
A_STATUS
Htc_ProcessRecvHeader(QOSAL_VOID *pCxt, QOSAL_VOID *pReq,
                      QOSAL_UINT32   *pNextLookAheads,
                      QOSAL_UINT32        *pNumLookAheads);
A_STATUS
HTC_Start(QOSAL_VOID *pCxt);
A_STATUS
HTC_WaitTarget(QOSAL_VOID *pCxt);
A_STATUS
HTC_ConnectService(QOSAL_VOID *pCxt,
                   HTC_SERVICE_CONNECT_REQ  *pConnectReq,
                   HTC_SERVICE_CONNECT_RESP *pConnectResp);
QOSAL_VOID
HTC_ProcessCpuInterrupt(QOSAL_VOID *pCxt);

QOSAL_VOID Hcd_Irq(void *pCxt);

A_STATUS
Driver_Main(QOSAL_VOID *pCxt, QOSAL_UINT8 scope, QOSAL_BOOL *pCanBlock, QOSAL_UINT16 *pBlock_msec);

#if defined(DRIVER_CONFIG_IMPLEMENT_RX_FREE_MULTIPLE_QUEUE)
QOSAL_VOID
Driver_ReportRxBuffStatus(QOSAL_VOID *pCxt, QOSAL_BOOL status, QOSAL_UINT8 epid);
#else
QOSAL_VOID
Driver_ReportRxBuffStatus(QOSAL_VOID *pCxt, QOSAL_BOOL status);
#endif

A_STATUS
Driver_CompleteRequest(QOSAL_VOID *pCxt, QOSAL_VOID *pReq);
QOSAL_BOOL
Driver_TxReady(QOSAL_VOID *pCxt);
A_STATUS
Driver_SubmitTxRequest(QOSAL_VOID *pCxt, QOSAL_VOID *pReq);
A_STATUS
Driver_SubmitEp0TxRequest(QOSAL_VOID *pCxt, QOSAL_VOID *pReq);
QOSAL_BOOL
Driver_RxReady(QOSAL_VOID *pCxt);
QOSAL_VOID
Driver_DropTxDataPackets(QOSAL_VOID *pCxt);
A_STATUS
Driver_SendPacket(QOSAL_VOID *pCxt, QOSAL_VOID *pReq);
A_STATUS
Driver_RecvPacket(QOSAL_VOID *pCxt, QOSAL_VOID *pReq);
A_STATUS
Driver_GetTargetInfo(QOSAL_VOID *pCxt);
A_STATUS
Driver_BootComm(QOSAL_VOID *pCxt);
QOSAL_VOID
Driver_RxComplete(QOSAL_VOID *pCxt, QOSAL_VOID *pReq);

A_STATUS
Driver_ReadRegDiag(QOSAL_VOID *pCxt, QOSAL_UINT32 *address, QOSAL_UINT32 *data);
A_STATUS
Driver_WriteRegDiag(QOSAL_VOID *pCxt, QOSAL_UINT32 *address, QOSAL_UINT32 *data);
A_STATUS
Driver_ReadDataDiag(QOSAL_VOID *pCxt, QOSAL_UINT32 address,
                    QOSAL_UCHAR *data, QOSAL_UINT32 length);
A_STATUS
Driver_WriteDataDiag(QOSAL_VOID *pCxt, QOSAL_UINT32 address,
                     QOSAL_UCHAR *data, QOSAL_UINT32 length);
A_STATUS
Driver_DumpAssertInfo(QOSAL_VOID *pCxt, QOSAL_UINT32 *pData, QOSAL_UINT16 *pLength);
A_STATUS
Driver_StoreRecallFirmwareDownload(QOSAL_VOID *pCxt);

QOSAL_VOID
HW_EnableDisableSPIIRQ(QOSAL_VOID *pCxt, QOSAL_BOOL Enable, QOSAL_BOOL FromIrq);
QOSAL_VOID
HW_PowerUpDown(QOSAL_VOID *pCxt, QOSAL_UINT8 powerUp);
A_STATUS
Bus_InOutToken(QOSAL_VOID *pCxt,
               QOSAL_UINT32        OutToken,
               QOSAL_UINT8         DataSize,
               QOSAL_UINT32        *pInToken);
QOSAL_VOID
Bus_TransferComplete(QOSAL_VOID *pCxt, A_STATUS status);
QOSAL_VOID
HW_InterruptHandler(QOSAL_VOID *pCxt);
QOSAL_VOID
Strrcl_Recall(QOSAL_VOID *pCxt, QOSAL_UINT32 msec_sleep);

A_STATUS
Hcd_ProgramWriteBufferWaterMark(QOSAL_VOID *pCxt, QOSAL_UINT32 length);
QOSAL_VOID
Hcd_RefreshWriteBufferSpace(QOSAL_VOID *pCxt);
//                                                                Todo
A_STATUS
Hcd_DoPioInternalAccess(QOSAL_VOID *pCxt, QOSAL_UINT16 addr, QOSAL_UINT32 *pValue, QOSAL_BOOL isRead);
/**********TBD**********/
QOSAL_UINT16
Api_TxGetStatus(QOSAL_VOID* pCxt);
QOSAL_VOID
Api_TxComplete(QOSAL_VOID *pCxt, QOSAL_VOID *pReq);
QOSAL_VOID
Api_RxComplete(QOSAL_VOID *pCxt, QOSAL_VOID *pReq);
A_STATUS
Api_WmiTxStart(QOSAL_VOID *pCxt, QOSAL_VOID *pReq, HTC_ENDPOINT_ID eid);
A_STATUS
Api_DataTxStart(QOSAL_VOID *pCxt, QOSAL_VOID *pReq);
QOSAL_VOID
Api_BootProfile(QOSAL_VOID *pCxt, QOSAL_UINT8 val);
A_STATUS
Api_InitStart(QOSAL_VOID *pCxt);
A_STATUS
Api_InitFinish(QOSAL_VOID *pCxt);
QOSAL_VOID
Api_WMIInitFinish(QOSAL_VOID *pCxt);
A_STATUS
Api_DeInitStart(QOSAL_VOID *pCxt);
A_STATUS
Api_DeInitFinish(QOSAL_VOID *pCxt);
A_STATUS
Api_DisconnectWiFi(QOSAL_VOID *pCxt);
A_STATUS
Api_ConnectWiFi(QOSAL_VOID *pCxt);
A_STATUS
Api_ParseInfoElem(QOSAL_VOID *pCxt, WMI_BSS_INFO_HDR *bih, QOSAL_INT32 len, A_SCAN_SUMMARY *pSummary);
A_STATUS
Api_DriverAccessCheck(QOSAL_VOID *pCxt, QOSAL_UINT8 block_allowed, QOSAL_UINT8 request_reason);
A_STATUS
Api_ProgramCountryCode(QOSAL_VOID *pCxt, QOSAL_UINT8* country_code, QOSAL_UINT16 length, QOSAL_UINT8 *pResult);
A_STATUS
Api_ProgramMacAddress(QOSAL_VOID *pCxt, QOSAL_UINT8* addr, QOSAL_UINT16 length, QOSAL_UINT8 *pResult);
A_STATUS
Api_SetPowerMode(QOSAL_VOID *pCxt, POWER_MODE *pwrmode);
QOSAL_UINT32
HW_GetMboxAddress(QOSAL_VOID *pCxt, QOSAL_UINT16 mbox, QOSAL_UINT32 length);
A_ENDPOINT_T *
Util_GetEndpoint(QOSAL_VOID *pCxt, QOSAL_UINT16 id);
QOSAL_UINT16
Util_Ieee2freq(QOSAL_INT32 chan);
QOSAL_UINT32
Util_Freq2ieee(QOSAL_UINT16 freq);
HTC_ENDPOINT_ID
Util_AC2EndpointID(QOSAL_VOID *pCxt, QOSAL_UINT8 ac);
QOSAL_UINT8
Util_EndpointID2AC(QOSAL_VOID *pCxt, HTC_ENDPOINT_ID ep);
QOSAL_UINT8
Util_Ascii2Hex(char val);
QOSAL_UINT32
scan_setup(QOSAL_VOID *pCxt, QOSAL_VOID *pWmi, WMI_START_SCAN_CMD *start_scan);
A_STATUS
wait_scan_done(QOSAL_VOID* pCxt, QOSAL_VOID* pWmi);
A_STATUS
move_power_state_to_maxperf(void *pCxt, QOSAL_INT32 module);

#if MANUFACTURING_SUPPORT
QOSAL_VOID
Api_Test_Cmd_Event(QOSAL_VOID *pCxt, QOSAL_UINT8 *datap, QOSAL_UINT32 len);
#endif

#if MANUFACTURING_SUPPORT
#define API_TEST_RESP_EVENT(pCxt, datap, len) Api_Test_Cmd_Event((pCxt), (datap), (len))
#endif

QOSAL_VOID
Driver_WakeDriver(QOSAL_VOID *pCxt);
QOSAL_VOID
Driver_WakeUser(QOSAL_VOID *pCxt);

#if DRIVER_CONFIG_MULTI_TASKING
#define DRIVER_WAKE_DRIVER(pCxt) Driver_WakeDriver((pCxt))
#define DRIVER_WAKE_USER(pCxt)   Driver_WakeUser((pCxt))
#else
#define DRIVER_WAKE_DRIVER(pCxt)
#define DRIVER_WAKE_USER(pCxt)
#endif  //#if DRIVER_CONFIG_MULTI_TASKING

#if ENABLE_STACK_OFFLOAD

#define SOCKET_CONTEXT_INIT      socket_context_init()
#define STACK_INIT(pCxt)         send_stack_init((pCxt))
#define SOCKET_CONTEXT_DEINIT()  socket_context_deinit()
#define MIN_HDR_LEN              sizeof (WMI_DATA_HDR)
#define WMI_DOT3_2_DIX(pReq)     A_OK

#else

#define STACK_INIT(pCxt)            A_OK
#define SOCKET_CONTEXT_INIT         A_OK
#define SOCKET_CONTEXT_DEINIT()       A_OK
#define MIN_HDR_LEN                 sizeof (WMI_DATA_HDR) + sizeof(ATH_MAC_HDR) + sizeof(ATH_LLC_SNAP_HDR)
#define WMI_DOT3_2_DIX(pReq)        wmi_dot3_2_dix((pReq))

#endif

enum BOOT_PROFILE {
  BOOT_PROFILE_WMI_READY = 0,
  BOOT_PROFILE_READ_REFCLK,
  BOOT_PROFILE_DONE_CONFIG,
  BOOT_PROFILE_START_CONNECT,
  BOOT_PROFILE_DONE_CONNECT,
  BOOT_PROFILE_DRIVE_READY,
  BOOT_PROFILE_START_SCAN,
  BOOT_PROFILE_DONE_SCAN,
  BOOT_PROFILE_POWER_UP,
  BOOT_PROFILE_BOOT_PARAMETER,
};

#if ENABLE_P2P_MODE

#define API_P2P_GO_NEG_EVENT(devt, devId, res, len, wps_info) Api_p2p_go_neg_event((devt), (devId), (res), (len), (wps_info))
#define API_P2P_INVITE_SEND_RESULT(pCxt, devId, datap, len)    Api_p2p_invite_send_result((pCxt), (devId), (datap), (len))
#define API_P2P_NODE_LIST_EVENT(devt, devId, res, len) Api_p2p_node_list_event((devt), (devId), (res), (len))
#define API_P2P_LIST_PERSISTENT_NETWORK_EVENT(devt, devId, res, len) Api_p2p_list_persistent_network_event((devt), (devId), (res), (len))
#define API_GET_P2P_CTX(devt, devId, res, len) Api_get_p2p_ctx((devt), (devId), (res), (len))
#define API_P2P_PROV_DISC_REQ(devt, devId, res, len) Api_p2p_prov_disc_req((devt), (devId), (res), (len))
#define API_P2P_SERV_DISC_REQ(devt, devId, res, len) Api_p2p_serv_disc_req((devt), (devId), (res), (len))
#define API_P2P_INVITE_REQ(pCxt, devId, datap, len)    Api_p2p_invite_req((pCxt), (devId), (datap), (len))
#define API_P2P_INVITE_RCVD_RESULT(pCxt, devId, datap, len)    Api_p2p_invite_rcvd_result((pCxt), (devId), (datap), (len))
#define API_P2P_REQ_AUTH_EVENT(devt, devId, res, len) Api_p2p_req_auth_event((devt), (devId), (res), (len))

QOSAL_VOID
Api_p2p_go_neg_event(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len, WMI_P2P_PROV_INFO *wps_info);
QOSAL_VOID
Api_p2p_invite_send_result(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len);
QOSAL_VOID
Api_p2p_node_list_event(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len);
QOSAL_VOID
Api_p2p_list_persistent_network_event(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len);
QOSAL_VOID
Api_get_p2p_ctx(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len);
QOSAL_VOID
Api_p2p_prov_disc_req(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len);

QOSAL_VOID
Api_p2p_serv_disc_req(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len);

QOSAL_VOID
Api_p2p_invite_req(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len);

QOSAL_VOID
Api_p2p_req_auth_event(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len);

QOSAL_VOID
Api_p2p_invite_rcvd_result(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len);
#endif // ENABLE_P2P_MODE

#if MANUFACTURING_SUPPORT
#define API_TEST_RESP_EVENT(pCxt, datap, len) Api_Test_Cmd_Event((pCxt), (datap), (len))

QOSAL_VOID
Api_Test_Cmd_Event(QOSAL_VOID *pCxt, QOSAL_UINT8 *datap, QOSAL_UINT32 len);

#endif

#ifdef __cplusplus
}
#endif

#endif /* _COMMON_API_H_ */
