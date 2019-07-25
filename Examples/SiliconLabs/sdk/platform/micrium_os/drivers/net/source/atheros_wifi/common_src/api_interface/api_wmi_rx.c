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
#include <a_config.h>
#include <a_types.h>
#include <a_osapi.h>
#include <common_api.h>
#include <custom_wlan_api.h>
#include <a_drv_api.h>
#include <wmi_api.h>
#include <aggr_recv_api.h>
#include <wmi.h>
#include <atheros_wifi_api.h> //IGX_UD_CHANGES
#include <atheros_wifi_internal.h> //IGX_UD_CHANGES
#include <wlan_api.h>

#if ENABLE_P2P_MODE
#include "p2p.h"
#include "wmi.h"
#include "wmi_api.h"
#include "wmi_host.h"
//                                                                WPS_CREDENTIAL persistCredential;
//                                                                WMI_WPS_PROFILE_EVENT wpsProfEv;
#endif

#include  "dset.h"
#include  "dset_api.h"

extern QOSAL_CONST QOSAL_UINT8 max_performance_power_param;
extern A_STATUS restore_power_state(void *pDCxt, QOSAL_INT32 module);
/*****************************************************************************/
/*  Api_TargetStatsEvent - Processes WMI_REPORT_STATISTICS_EVENTID
 *	 from the device.
 *              QOSAL_VOID *pCxt - the driver context.
 *		QOSAL_UINT8 *ptr - pointer to the stats buffer.
 *		QOSAL_UINT32 len - length of the ptr buffer.
 *****************************************************************************/
QOSAL_VOID
Api_TargetStatsEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *ptr, QOSAL_UINT32 len)
{
  UNUSED_ARGUMENT(devId);
  if (ptr != NULL && len != 0) {
    GET_DRIVER_COMMON(pCxt)->rssi = ((WMI_TARGET_STATS*)ptr)->cservStats.cs_snr;
  }
  GET_DRIVER_COMMON(pCxt)->rssiFlag = QOSAL_FALSE;
  API_TARGET_STATS_EVENT(pCxt, ptr, len);
}

/*****************************************************************************/
/*  Api_RegDomainEvent - Processes WMI_REGDOMAIN_EVENTID from the device.
 *              QOSAL_VOID *pCxt - the driver context.
 *		QOSAL_UINT32 regCode - the encoded regulatory domain value.
 *****************************************************************************/
QOSAL_VOID
Api_RegDomainEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT32 regCode)
{
  UNUSED_ARGUMENT(devId);
  //                                                              A_PRINTF("AR4XXX Reg Code = 0x%x\n", regCode);
  GET_DRIVER_COMMON(pCxt)->regCode = regCode;
  API_REGDOMAIN_EVENT(pCxt, regCode);
}

/*****************************************************************************/
/*  Api_BitrateEvent - Processes WMI_BIT_RATE_REPLY from the device.
 *              QOSAL_VOID *pCxt - the driver context.
 *		QOSAL_INT32 rateKbps - the rate in Kbps used for recent transmission.
 *****************************************************************************/
QOSAL_VOID
Api_BitrateEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_INT32 rateKbps)
{
  UNUSED_ARGUMENT(devId);
  GET_DRIVER_COMMON(pCxt)->bitRate = rateKbps;
  API_BITRATE_EVENT(pCxt, rateKbps);
}

/*****************************************************************************/
/*  Api_GetBitRateEvent - Processes WMI_GET_BITRATE_CMDID from the device.
 *              QOSAL_VOID *pCxt - the driver context.
 *		QOSAL_UINT8 *datap - the rate index for recent transmission is in this pointer.
 *****************************************************************************/
QOSAL_VOID
Api_GetBitRateEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_VOID *datap)
{
  UNUSED_ARGUMENT(devId);
  CUSTOM_API_BITRATE_EVENT_TX(pCxt, devId, ((WMI_BIT_RATE_REPLY *)datap)->rateIndex);
}

/*****************************************************************************/
/*  Api_ChannelListEvent - Processes a WMI_GET_CHANNEL_LIST_CMD from the device.
 *	 This event will come when the host issues a WMI_GET_CHANNEL_LIST_CMD to
 *	 the device to request a current channel list.
 *              QOSAL_VOID *pCxt - the driver context.
 *		QOSAL_INT8 numChan - number of channels in the list.
 *		QOSAL_UINT16 *chanList - array of numChan channels.
 *****************************************************************************/
QOSAL_VOID
Api_ChannelListEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_INT8 numChan, QOSAL_UINT16 *chanList)
{
  QOSAL_UINT16 i;
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);
  UNUSED_ARGUMENT(devId);
  A_MEMCPY(pDCxt->channelList, chanList, numChan * sizeof (QOSAL_UINT16));
  pDCxt->numChannels = numChan;
  /* convert each channel to proper endianness */
  for (i = 0; i < pDCxt->numChannels; i++) {
    pDCxt->channelList[i] = A_LE2CPU16(pDCxt->channelList[i]);
  }

  API_CHANNEL_LIST_EVENT(pCxt, pDCxt->numChannels, pDCxt->channelList);
}

/*****************************************************************************/
/*  Api_ScanCompleteEvent - Processes a WMI_SCAN_COMPLETE_EVENTID from the device.
 *	 These events will occur if the host has previously initiated a scan request.
 *	 This event indicates the final conclusion of the scan operation.
 *              QOSAL_VOID *pCxt - the driver context.
 *		A_STATUS status - indicates the status of the scan operation.
 *****************************************************************************/
QOSAL_VOID
Api_ScanCompleteEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, A_STATUS status)
{
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);

  UNUSED_ARGUMENT(status);
  UNUSED_ARGUMENT(devId);

  wmi_bssfilter_cmd(pDCxt->pWmiCxt, NONE_BSS_FILTER, 0);
  /* host initiated scan */
  pDCxt->scanDone = QOSAL_TRUE;

  Api_BootProfile(pCxt, BOOT_PROFILE_DONE_SCAN);

  API_SCAN_COMPLETE_EVENT(pCxt, status);
}

/*****************************************************************************/
/*  Api_BssInfoEvent - Processes a WMI_BSS_INFO_EVENT from the device. These
 *	 events will occur if the host has previously initiated a scan request
 *              QOSAL_VOID *pCxt - the driver context.
 *		QOSAL_UINT8 *datap - the original event buffer.
 *		QOSAL_INT32 len - the length of datap.
 *****************************************************************************/
QOSAL_VOID
Api_BssInfoEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_INT32 len)
{
  UNUSED_ARGUMENT(devId);
  CUSTOM_API_BSS_INFO_EVENT(pCxt, datap, len);
}

/*****************************************************************************/
/*  Api_TkipMicErrorEvent - Processes a TKIP error event from the device. This
*	 event can happen when the device is using TKIP security and it detects a
*	 decryption error.
*               QOSAL_VOID *pCxt - the driver context.
*		QOSAL_UINT8 keyid - the Key ID used when the error occurred.
*		QOSAL_BOOL ismcast - 1 if packet was multi-cast 0 otherwise.
*****************************************************************************/
QOSAL_VOID
Api_TkipMicErrorEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 keyid,
                      QOSAL_BOOL ismcast)
{
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);
  UNUSED_ARGUMENT(ismcast);
  UNUSED_ARGUMENT(keyid);
  UNUSED_ARGUMENT(devId);
  pDCxt->tkipCountermeasures = 1;
  POWER_MODE pwrmode;
  pwrmode.pwr_mode = MAX_PERF_POWER;
  pwrmode.pwr_module = TKIP;

  Api_SetPowerMode(pCxt, &pwrmode);

  /* receiving this event informs the host that the wifi device
   * encountered 2 TKIP MIC errors within the restricted time span.
   * consequently, to fullfill the TKIP countermeasure requirement the
   * host must disconnect from the network and remain disconnected for
   * the specified period.
   */
  wmi_cmd_start(pDCxt->pWmiCxt, NULL, WMI_DISCONNECT_CMDID, 0);
  /* can't use A_MDELAY here as the wmi_disconnect_cmd() will
   * cause a disconnect_event which should get processed. */
  //                                                              A_MDELAY(IEEE80211_TKIP_COUNTERMEASURES_TIMEOUT_MSEC);
  API_TKIP_MIC_ERROR_EVENT(pCxt, keyid, ismcast);
}

/*****************************************************************************/
/*  Api_GetPmkEvent - Processes a WMI_GET_PMK_REPLY from the
 *	 device.
 *	 This event will come when the caller has previously sent a WMI_GET_PMC_CMD.
 *	 The function stores the result in wpaPmk for asynchronous access.
 *              QOSAL_VOID *pCxt - the driver context.
 *		QOSAL_UINT8 *datap - original event buffer.
 *****************************************************************************/
QOSAL_VOID
Api_GetPmkEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap)
{
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);
  WMI_GET_PMK_REPLY *pReply = (WMI_GET_PMK_REPLY *)datap;

  A_MEMCPY(pDCxt->conn[devId].wpaPmk, pReply->pmk, WMI_PMK_LEN);
  pDCxt->conn[devId].wpaPmkValid = QOSAL_TRUE;
  API_GET_PMK_EVENT(pCxt, datap);
}

/*****************************************************************************/
/*  Api_ConnectEvent - Processes a WMI_CONNECT_EVENT from the
 *	 device.
 *	 This event will come when the device has connected to a BSS
 *	 network.
 *              QOSAL_VOID *pCxt - the driver context.
 *		QOSAL_UINT16 channel - RF channel on which connection exists.
 *		QOSAL_UINT8 *bssid - bssid address of the network.
 *		QOSAL_UINT16 listenInterval - assigned listen interval for sleep mode.
 *		QOSAL_UINT16 beaconInterval - BSS beacon interval.
 *		NETWORK_TYPE networkType - identifies the type of network.
 *              QOSAL_UINT8 beaconIeLen - length of beacon Information element.
 *		QOSAL_UINT8 assocReqLen - length of assocation request frame.
 *		QOSAL_UINT8 assocRespLen - length of assocation response length.
 *		QOSAL_UINT8 *assocInfo - pointer to assocation frame(s).
 *****************************************************************************/
QOSAL_VOID
Api_ConnectEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT16 channel, QOSAL_UINT8 *bssid,
                 QOSAL_UINT16 listenInterval, QOSAL_UINT16 beaconInterval,
                 NETWORK_TYPE networkType, QOSAL_UINT8 beaconIeLen,
                 QOSAL_UINT8 assocReqLen, QOSAL_UINT8 assocRespLen,
                 QOSAL_UINT8 *assocInfo)
{
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);
  A_WEPKEY_T *pKey;
  WMI_ADD_CIPHER_KEY_CMD add_key_param;

  UNUSED_ARGUMENT(listenInterval);
  UNUSED_ARGUMENT(beaconInterval);
  UNUSED_ARGUMENT(beaconIeLen);
  UNUSED_ARGUMENT(assocReqLen);
  UNUSED_ARGUMENT(assocRespLen);
  UNUSED_ARGUMENT(assocInfo);

  /* In SoftAP mode, we will get connect event first time when we start as AP and then whenever a station connects to us.
   * We should not set the channel and bssid, when the later connect event comes.
   * The later connect event has network type as 0.
   */
  if (networkType != 0) {
    A_MEMCPY(pDCxt->conn[devId].bssid, bssid, ATH_MAC_LEN);
    pDCxt->conn[devId].bssChannel = channel;
  }

  if ((networkType & ADHOC_NETWORK)
      && (OPEN_AUTH == pDCxt->conn[devId].dot11AuthMode)
      && (NONE_AUTH == pDCxt->conn[devId].wpaAuthMode)
      && (WEP_CRYPT == pDCxt->conn[devId].wpaPairwiseCrypto)) {
    /* if we are connecting for the first time to an ad-hoc network with
     * WEP. then submit the key via wmi_addKey_cmd */
    if (QOSAL_FALSE == pDCxt->conn[devId].isConnected) {
      pKey = &(pDCxt->conn[devId].wepKeyList[pDCxt->conn[devId].wepDefTxKeyIndex]);
      A_MEMZERO(&add_key_param, sizeof(add_key_param));
      add_key_param.keyIndex = pDCxt->conn[devId].wepDefTxKeyIndex;
      add_key_param.keyType  = WEP_CRYPT;
      add_key_param.keyUsage = GROUP_USAGE | TX_USAGE;
      add_key_param.keyLength = pKey->keyLen;
      A_MEMCPY(&add_key_param.key, pKey->key, add_key_param.keyLength);
      add_key_param.key_op_ctrl = KEY_OP_INIT_VAL;
      wmi_cmd_start(pDCxt->pWmiCxt, &add_key_param, WMI_ADD_CIPHER_KEY_CMDID, sizeof(WMI_ADD_CIPHER_KEY_CMD));
    }
  }

  /* Update connect & link status atomically */
  DRIVER_SHARED_RESOURCE_ACCESS_ACQUIRE(pCxt);
  {
    pDCxt->conn[devId].isConnected = QOSAL_TRUE;
    pDCxt->conn[devId].isConnectPending = QOSAL_FALSE;
    if ((channel > 2000) && (channel < 6500)) {
      pDCxt->conn[devId].channelHint = channel;
    }
  }
  DRIVER_SHARED_RESOURCE_ACCESS_RELEASE(pCxt);

  Api_BootProfile(pCxt, BOOT_PROFILE_DONE_CONNECT);

  pDCxt->rssi = 0;

  /* reset the rx aggr state */

#if WLAN_CONFIG_11N_AGGR_SUPPORT
  aggr_reset_state(pDCxt->pAggrCxt);
#endif
  if (pDCxt->wps_in_progress && pDCxt->devId == devId) {
    pDCxt->wps_in_progress = 0;
  }
  /*printf("connect\r\n");*/
  CUSTOM_API_CONNECT_EVENT(pCxt, devId, channel, bssid,
                           listenInterval, beaconInterval,
                           networkType, beaconIeLen,
                           assocReqLen, assocRespLen,
                           assocInfo);
}

/*****************************************************************************/
/*  Api_DisconnectEvent - Processes a WMI_DISCONNECT_EVENT from the
 *	 device.
 *	 This event will come when the device has disconnected from the BSS
 *	 network.
 *              QOSAL_VOID *pCxt - the driver context.
 *		QOSAL_UINT8 reason - encoded reason for the disconnection.
 *		QOSAL_UINT8 *bssid - bssid address of the network.
 *		QOSAL_UINT8 assocRespLen - association response frame length.
 *		QOSAL_UINT8 *assocInfo - association response frame payload.
 *		QOSAL_UINT16 protocolReasonStatus - reason field from assocation response
 *		 frame.
 *****************************************************************************/
QOSAL_VOID
Api_DisconnectEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 reason, QOSAL_UINT8 *bssid,
                    QOSAL_UINT8 assocRespLen, QOSAL_UINT8 *assocInfo, QOSAL_UINT16 protocolReasonStatus)
{
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);
  POWER_MODE pwrmode;
  /*printf("disconenct\r\n");*/
  UNUSED_ARGUMENT(bssid);
  UNUSED_ARGUMENT(assocRespLen);
  UNUSED_ARGUMENT(assocInfo);
  UNUSED_ARGUMENT(protocolReasonStatus);
  /* it is necessary to clear the host-side rx aggregation state */
#if WLAN_CONFIG_11N_AGGR_SUPPORT
  aggr_reset_state(pDCxt->pAggrCxt);
#endif

  /*
   * If the event is due to disconnect cmd from the host, only they the target
   * would stop trying to connect. Under any other condition, target would
   * keep trying to connect.
   *
   */
  if ( reason == DISCONNECT_CMD) {
    pDCxt->conn[devId].isConnectPending = QOSAL_FALSE;
  } else {
    pDCxt->conn[devId].isConnectPending = QOSAL_TRUE;
  }

  /* Update connect & link status atomically */
  DRIVER_SHARED_RESOURCE_ACCESS_ACQUIRE(pCxt);
  pDCxt->conn[devId].isConnected = QOSAL_FALSE;
  pDCxt->conn[devId].channelHint = 0;
  DRIVER_SHARED_RESOURCE_ACCESS_RELEASE(pCxt);
  pDCxt->rssi = 0;
  A_MEMZERO(pDCxt->conn[devId].bssid, ATH_MAC_LEN);
  pDCxt->conn[devId].bssChannel = 0;

  if (pDCxt->wps_in_progress && pDCxt->devId == devId) {
    pDCxt->wps_in_progress = 0;
  }
  CUSTOM_API_DISCONNECT_EVENT(pCxt, devId, reason, bssid,
                              assocRespLen, assocInfo, protocolReasonStatus);
  /* complete TKIP countermeasures operation. TKIP countermeasures
   * requires that the STA avoid any contact with the AP for 60 seconds.
   * this implementation simply blocks for that time via A_MDELAY().
   * Other implementations may choose to behave differently. */
  if (pDCxt->tkipCountermeasures != 0) {
    /* FIXME: should instead implement a way to block the driver at driver_main thus
     * allowing the thread to continue running in single-threaded systems.*/
    A_MDELAY(IEEE80211_TKIP_COUNTERMEASURES_TIMEOUT_MSEC);
    pDCxt->tkipCountermeasures = 0;
    Api_ConnectWiFi(pCxt);
    A_MDELAY(200);

    pwrmode.pwr_mode = REC_POWER;
    pwrmode.pwr_module = TKIP;
    Api_SetPowerMode(pCxt, &pwrmode);
  }
}

int   dset_length;

QOSAL_VOID
Api_HostDsetStoreEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_INT32 len, QOSAL_VOID *pReq)
{
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);
  QOSAL_UINT8* strrclData = pDCxt->tempStorage;
  QOSAL_UINT16 cur_len = pDCxt->strrclCxtLen;

  WMI_HOST_DSET_STORE_EVENT  *pDsetEvent = (WMI_HOST_DSET_STORE_EVENT *)datap;
  UNUSED_ARGUMENT(devId);

  dset_length = pDsetEvent->length;

  strrclData = pDCxt->tempStorage + cur_len;

  if ( cur_len + dset_length <= pDCxt->tempStorageLength ) {
    A_MEMCPY(strrclData, pDsetEvent->data, dset_length);
  } else {
    A_ASSERT(0);
  }
  pDCxt->strrclCxtLen += dset_length;
}

#if 0
A_STATUS
wmi_storerecall_recall_cmd(struct wmi_t *wmip, QOSAL_UINT32 length, void* pData)
{
  void *osbuf;
  WMI_STORERECALL_RECALL_CMD *p;
  QOSAL_UINT32 msgLength = sizeof(WMI_STORERECALL_RECALL_CMD) - sizeof(QOSAL_UINT8);

  osbuf = A_NETBUF_ALLOC((QOSAL_INT32)msgLength);
  if (osbuf == NULL) {
    return A_NO_MEMORY;
  }

  A_NETBUF_PUT(osbuf, (QOSAL_INT32)msgLength);

  p = (WMI_STORERECALL_RECALL_CMD *)(A_NETBUF_DATA(osbuf));

  p->length = A_CPU2LE32(length);
  //                                                              A_MEMCPY(&p->data[0], pData, length);
  /* here we append the payload to the msg rather than copy it. this is a
   * memory and CPU optimization as the payload can be very large.
   */
  A_NETBUF_APPEND_FRAGMENT(osbuf, pData, length);

  return (wmi_cmd_send(wmip, osbuf, WMI_STORERECALL_RECALL_CMDID, NO_SYNC_WMIFLAG));
}
#endif

QOSAL_VOID
Api_HostDsetCreateEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_INT32 len, QOSAL_VOID *pReq)
{
  void *osbuf;
  QOSAL_UINT32    msgLength;
  WMI_HOST_DSET_RESP_CREATE_CMD   *p;
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);
  HOST_DSET  *pHost;

  UNUSED_ARGUMENT(devId);

  WMI_HOST_DSET_CREATE_EVENT  *pDsetEvent = (WMI_HOST_DSET_CREATE_EVENT *)datap;
  pHost = dset_get(pDsetEvent->dset_id, pDsetEvent->length);
  A_PRINTF("dset create  id=%d  length=%d\n", pDsetEvent->dset_id, pDsetEvent->length);

/*
 *  respose KF dset create request
 */
  msgLength = sizeof(WMI_HOST_DSET_RESP_CREATE_CMD);
  osbuf = A_NETBUF_ALLOC((QOSAL_INT32)msgLength);
  A_NETBUF_PUT(osbuf, (QOSAL_INT32)(msgLength));

  p = (WMI_HOST_DSET_RESP_CREATE_CMD *)(A_NETBUF_DATA(osbuf));
  if (pHost != NULL) {
    p->flags = 0;
  } else {
    p->flags = 1;
  }

  wmi_cmd_send(pDCxt->pWmiCxt, osbuf, WMI_HOST_DSET_RESP_CREATE_CMDID, NO_SYNC_WMIFLAG);
}

QOSAL_VOID
Api_HostDsetWriteEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_INT32 len, QOSAL_VOID *pReq)
{
  void *osbuf;
  QOSAL_UINT32    msgLength;
  WMI_HOST_DSET_RESP_WRITE_CMD   *p;
  HOST_DSET  *pDset = NULL;
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);

  WMI_HOST_DSET_WRITE_EVENT  *pDsetEvent = (WMI_HOST_DSET_WRITE_EVENT *)datap;

  if (IS_STRRCL_DSET(pDsetEvent->dset_id)) {
    pDset = dset_get(pDsetEvent->dset_id, pDsetEvent->length);
  }

  if (pDset == NULL) {
    return;
  }

  A_PRINTF("dset write  id=%d  offset:%d  length=%d\n", pDsetEvent->dset_id, pDsetEvent->offset, pDsetEvent->length);
  dset_write(pDset, pDsetEvent->data, pDsetEvent->offset, pDsetEvent->length);

/*
 *  respose KF dset write request
 */
  msgLength = sizeof(WMI_HOST_DSET_RESP_WRITE_CMD);
  osbuf = A_NETBUF_ALLOC((QOSAL_INT32)msgLength);
  A_NETBUF_PUT(osbuf, (QOSAL_INT32)(msgLength));

  p = (WMI_HOST_DSET_RESP_WRITE_CMD *)(A_NETBUF_DATA(osbuf));
  p->dset_id = pDsetEvent->dset_id;
  p->offset = pDsetEvent->offset;
  p->length = pDsetEvent->length;

  wmi_cmd_send(pDCxt->pWmiCxt, osbuf, WMI_HOST_DSET_RESP_WRITE_CMDID, NO_SYNC_WMIFLAG);
}

QOSAL_VOID
Api_HostDsetReadbackEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_INT32 len, QOSAL_VOID *pReq)
{
  HOST_DSET  *pDset;
  QOSAL_UINT32    msgLength;
  QOSAL_UINT32    strrcl_offset;
  QOSAL_UINT32    offset, length;
//                                                                    WMI_STORERECALL_RECALL_DSET *pRecallDset;

  QOSAL_UINT16    snd_size;
  void *osbuf;
  WMI_HOST_DSET_READBACK_CMD   *p;
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);

  UNUSED_ARGUMENT(devId);

  WMI_HOST_DSET_READBACK_EVENT  *pDsetEvent = (WMI_HOST_DSET_READBACK_EVENT *)datap;

  msgLength = sizeof(WMI_HOST_DSET_READBACK_CMD) - sizeof(QOSAL_UINT8);
  A_PRINTF("dset id=%d\n", pDsetEvent->dset_id);

  pDset = dset_find(pDsetEvent->dset_id);
  if (IS_STRRCL_DSET(pDsetEvent->dset_id)) {
//                                                                      pRecallDset = (WMI_STORERECALL_RECALL_DSET *)pDset->data_ptr;
    strrcl_offset = sizeof(WMI_STORERECALL_RECALL_DSET);
  }

  offset = pDsetEvent->offset;
  length = pDsetEvent->length;

  if (pDset->length < offset + length) {
    length = pDset->length - offset;
  }

  while (length > 0) {
    if (length > MAX_EVENT_SIZE) {
      snd_size = MAX_EVENT_SIZE;
    } else {
      snd_size = length;
    }

    osbuf = A_NETBUF_ALLOC((QOSAL_INT32)msgLength + snd_size);
    if (osbuf == NULL) {
      return;
    }

    A_NETBUF_PUT(osbuf, (QOSAL_INT32)(msgLength + snd_size));
    p = (WMI_HOST_DSET_READBACK_CMD *)(A_NETBUF_DATA(osbuf));
    p->dset_id = pDsetEvent->dset_id;

    p->offset = offset;
    p->length = snd_size;

    dset_read(pDset, p->data, offset + strrcl_offset, snd_size);
    wmi_cmd_send(pDCxt->pWmiCxt, osbuf, WMI_HOST_DSET_READBACK_CMDID, NO_SYNC_WMIFLAG);

    offset += snd_size;
    length -= snd_size;
  }

  A_FREE(pDset->data_ptr, MALLOC_ID_CONTEXT);
  pDset->data_ptr = NULL;

  restore_power_state(pDCxt, 1);
  A_MDELAY(50);
}

QOSAL_VOID
Api_HostDsetSyncEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_INT32 len, QOSAL_VOID *pReq)
{
  QOSAL_UINT32    msgLength;
  QOSAL_UINT16    count;
  void *osbuf;
  WMI_HOST_DSET_SYNC_CMD   *p;
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);

  UNUSED_ARGUMENT(devId);

  WMI_HOST_DSET_SYNC_EVENT  *pDsetEvent = (WMI_HOST_DSET_SYNC_EVENT *)datap;

  if (pDsetEvent->dset_count < MAX_DSET_SIZE) {
    count = pDsetEvent->dset_count;
  } else {
    count = MAX_DSET_SIZE;
  }

  msgLength = sizeof(WMI_HOST_DSET_SYNC_CMD) - sizeof(QOSAL_UINT8);

  dset_length = sizeof(HOST_DSET_ITEM) * count;
  osbuf = A_NETBUF_ALLOC((QOSAL_INT32)msgLength + dset_length);
  if (osbuf == NULL) {
    return;
  }

  A_NETBUF_PUT(osbuf, (QOSAL_INT32)(msgLength + dset_length));
  p = (WMI_HOST_DSET_SYNC_CMD *)(A_NETBUF_DATA(osbuf));

  count = dset_fill(&p->data[0], count);
  p->dset_count = count;

  wmi_cmd_send(pDCxt->pWmiCxt, osbuf, WMI_HOST_DSET_SYNC_CMDID, NO_SYNC_WMIFLAG);
}

QOSAL_VOID
Api_HostDsetReadEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_INT32 len, QOSAL_VOID *pReq)
{
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);
  QOSAL_UINT8* strrclData = pDCxt->tempStorage;
  void *osbuf;
  WMI_STORERECALL_READ_CMD *p;
  QOSAL_UINT32 msgLength = sizeof(WMI_STORERECALL_READ_CMD) - sizeof(QOSAL_UINT8);

  UNUSED_ARGUMENT(devId);

  osbuf = A_NETBUF_ALLOC((QOSAL_INT32)msgLength + dset_length);
  if (osbuf == NULL) {
    return;
  }

  A_NETBUF_PUT(osbuf, (QOSAL_INT32)(msgLength + dset_length));

  p = (WMI_STORERECALL_READ_CMD *)(A_NETBUF_DATA(osbuf));

  p->length = A_CPU2LE32(dset_length);
  A_MEMCPY(&p->data[0], strrclData, dset_length);

  A_PRINTF("[%02x][%02x][%02x][%02x] [%02x][%02x]\n", strrclData[0], strrclData[1], strrclData[2], strrclData[3], strrclData[4], strrclData[5]);

  /* here we append the payload to the msg rather than copy it. this is a
   * memory and CPU optimization as the payload can be very large.
   */
//                                                                    A_NETBUF_APPEND_FRAGMENT(osbuf, strrclData, dset_length);

  wmi_cmd_send(pDCxt->pWmiCxt, osbuf, WMI_DSET_HOST_READ_CMDID, NO_SYNC_WMIFLAG);
}

QOSAL_VOID
Api_dset_read_event(A_DRIVER_CONTEXT *pDCxt, HOST_DSET_HANDLE *pDsetHandle, WMI_DSET_OP_READ_EVENT  *pRespRead)
{
  QOSAL_UINT32   length;
  QOSAL_UINT32   offset, buf_off;

  length = pRespRead->length;
  offset = pRespRead->offset;
  buf_off = offset - pRespRead->offset;

  if (pRespRead->status == 0) {
    A_PRINTF("read off:%d len:%d\n", pRespRead->offset, pRespRead->length);

    if (offset + length > pDsetHandle->offset + pDsetHandle->length) {
      length = pDsetHandle->offset + pDsetHandle->length - offset;
    }

    A_MEMCPY(pDsetHandle->data_ptr + buf_off, pRespRead->data, length);

    pDsetHandle->left_len -= length;

    if (pDsetHandle->left_len != 0) {
      return;
    }
  }

  A_PRINTF("read done\n");

  if (pDsetHandle->cb != NULL) {
    pDsetHandle->cb(pRespRead->status, pDsetHandle->cb_args);
  }
}

QOSAL_VOID
Api_dset_write_event(A_DRIVER_CONTEXT *pDCxt, HOST_DSET_HANDLE *pDsetHandle, WMI_DSET_OP_READ_EVENT  *pRespRead)
{
  QOSAL_UINT32   length;
  QOSAL_UINT32   offset;

  length = pRespRead->length;
  offset = pRespRead->offset;

  A_PRINTF("read off:%d len:%d\n", pRespRead->offset, pRespRead->length);

  if (offset + length > pDsetHandle->offset + pDsetHandle->length) {
    length = pDsetHandle->offset + pDsetHandle->length - offset;
  }

  A_MEMCPY(pDsetHandle->data_ptr + offset, pRespRead->data, length);

  pDsetHandle->left_len -= length;

  if (pDsetHandle->left_len != 0) {
    return;
  }

  A_PRINTF("write done\n");

  if (pDsetHandle->cb != NULL) {
    pDsetHandle->cb(pRespRead->status, pDsetHandle->cb_args);
  }
}

QOSAL_VOID
Api_DsetOPEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_INT32 len, QOSAL_VOID *pReq)
{
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);

  struct WMIX_DSET_OP_SET_EVENT {
    QOSAL_UINT32      dset_id;
  } *pCmd;
  HOST_DSET_HANDLE *pDsetHandle;

  A_PRINTF("dset op event\n");

  WMI_DSET_OP_EVENT  *pDsetOPEvent = (WMI_DSET_OP_EVENT *)datap;

  pCmd = (struct WMIX_DSET_OP_SET_EVENT *)pDsetOPEvent->data;
  pDsetHandle = dset_find_handle(pCmd->dset_id);

  if (pDsetHandle->cb == NULL && (pDsetOPEvent->hdr.commandId != WMIX_DSET_READ_CMDID)) {
    pDCxt->dset_op_done = QOSAL_TRUE;
  }

  switch (pDsetOPEvent->hdr.commandId) {
    case  WMIX_DSET_CREATE_CMDID:
    {
      WMI_DSET_OP_CREATE_EVENT  *pCreateEvent;
      pCreateEvent = (WMI_DSET_OP_CREATE_EVENT *)pDsetOPEvent->data;

      A_PRINTF("create return: %d\n", pCreateEvent->status);
      if (pDsetHandle->cb != NULL) {
        pDsetHandle->cb(pCreateEvent->status, pDsetHandle->cb_args);
      }

      pDCxt->setparamStatus = pCreateEvent->status;
      pDCxt->dset_op_done = QOSAL_TRUE;
      break;
    }
    case  WMIX_DSET_OPEN_CMDID:
    {
      WMI_DSET_OP_CREATE_EVENT  *pCreateEvent;
      pCreateEvent = (WMI_DSET_OP_CREATE_EVENT *)pDsetOPEvent->data;

      A_PRINTF("open return: %d\n", pCreateEvent->status);
      if (pDsetHandle->cb != NULL) {
        pDsetHandle->cb(pCreateEvent->status, pDsetHandle->cb_args);
      }

      pDCxt->setparamStatus = pCreateEvent->status;
      pDCxt->dset_op_done = QOSAL_TRUE;
      break;
    }
    case  WMIX_DSET_READ_CMDID:
    {
      WMI_DSET_OP_READ_EVENT  *pRespRead;
      pRespRead = (WMI_DSET_OP_READ_EVENT *)pDsetOPEvent->data;

      Api_dset_read_event(pDCxt, pDsetHandle, pRespRead);

      pDCxt->setparamStatus = pRespRead->status;
      pDCxt->dset_op_done = QOSAL_TRUE;
      break;
    }
    case  WMIX_DSET_WRITE_CMDID:
    {
      WMI_DSET_OP_CREATE_EVENT  *pCreateEvent;
      pCreateEvent = (WMI_DSET_OP_CREATE_EVENT *)pDsetOPEvent->data;

      A_PRINTF("write return: %d\n", pCreateEvent->status);
      if (pDsetHandle->cb != NULL) {
        pDsetHandle->cb(pCreateEvent->status, pDsetHandle->cb_args);
      }

      pDCxt->setparamStatus = pCreateEvent->status;
      pDCxt->dset_op_done = QOSAL_TRUE;
      break;
    }
    case  WMIX_DSET_COMMIT_CMDID:
    {
      WMI_DSET_OP_COMMIT_EVENT  *pRespCommit;
      pRespCommit = (WMI_DSET_OP_COMMIT_EVENT *)pDsetOPEvent->data;

      A_PRINTF("commit return: %d\n", pRespCommit->status);
      if (pDsetHandle->cb != NULL) {
        pDsetHandle->cb(pRespCommit->status, pDsetHandle->cb_args);
      }

      pDCxt->setparamStatus = pRespCommit->status;
      pDCxt->dset_op_done = QOSAL_TRUE;
      break;
    }
    case  WMIX_DSET_CLOSE_CMDID:
    {
      WMI_DSET_OP_CLOSE_EVENT  *pRespClose;
      pRespClose = (WMI_DSET_OP_CLOSE_EVENT *)pDsetOPEvent->data;

      A_PRINTF("close return: %d\n", pRespClose->status);
      if (pDsetHandle->cb != NULL) {
        pDsetHandle->cb(pRespClose->status, pDsetHandle->cb_args);
      }

      pDCxt->setparamStatus = pRespClose->status;
      pDCxt->dset_op_done = QOSAL_TRUE;
      break;
    }
    case  WMIX_DSET_SIZE_CMDID:
    {
      WMI_DSET_OP_CREATE_EVENT  *pCreateEvent;
      pCreateEvent = (WMI_DSET_OP_CREATE_EVENT *)pDsetOPEvent->data;

      A_PRINTF("size return: %d\n", pCreateEvent->status);
      if (pDsetHandle->cb != NULL) {
        pDsetHandle->cb(pCreateEvent->status, pDsetHandle->cb_args);
      }

      pDCxt->setparamStatus = pCreateEvent->status;
      pDCxt->dset_op_done = QOSAL_TRUE;
      break;
    }
    case  WMIX_DSET_DELETE_CMDID:
    {
      WMI_DSET_OP_CREATE_EVENT  *pCreateEvent;
      pCreateEvent = (WMI_DSET_OP_CREATE_EVENT *)pDsetOPEvent->data;

      A_PRINTF("delete return: %d\n", pCreateEvent->status);
      if (pDsetHandle->cb != NULL) {
        pDsetHandle->cb(pCreateEvent->status, pDsetHandle->cb_args);
      }

      pDCxt->setparamStatus = pCreateEvent->status;
      pDCxt->dset_op_done = QOSAL_TRUE;
      break;
    }
  }
  DRIVER_WAKE_DRIVER(pCxt);
}
#if DRIVER_CONFIG_ENABLE_STORE_RECALL
/*****************************************************************************/
/*  Api_StoreRecallEvent - Processes a WMI_STORERECALL_STORE_EVENT from the
 *	 device.
 *	 This event will come when the device is ready to be shutdown by the host
 *	 as part of a store-and-recall sequence.
 *              QOSAL_VOID *pCxt - the driver context.
 *		QOSAL_UINT8 *datap - original event buffer.
 *		QOSAL_INT32 len - length in bytes of datap
 *		QOSAL_VOID *pReq - original event rx-request object.
 *****************************************************************************/
QOSAL_VOID
Api_StoreRecallEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_INT32 len, QOSAL_VOID *pReq)
{
  WMI_STORERECALL_STORE_EVENT *pEv;
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);
  QOSAL_UINT8* strrclData = pDCxt->tempStorage;

  UNUSED_ARGUMENT(devId);

  switch (pDCxt->strrclState) {
    case STRRCL_ST_START:
      A_MEMCPY(strrclData, datap, len);
      pEv = (WMI_STORERECALL_STORE_EVENT *)strrclData;

      if (pEv->msec_sleep == 0) {
        /* a 0 msec_sleep indicates a firmware condition which
         * prevented StoreRecall from completing.  in this event this
         * driver merely aborts the operation and continues normally.
         */
        pDCxt->strrclState = STRRCL_ST_INIT;
        pDCxt->strrclBlock = QOSAL_FALSE;
        API_STORE_RECALL_EVENT(pCxt);
        /* FIXME: Notify the application that S&R did not complete
         * successfully.
         */
      } else {
        /* indicate chip shutdown via chip_down. */
        //                                                        pDCxt->strrclCxt = datap;
        pDCxt->strrclCxt = strrclData;
        pDCxt->strrclCxtLen = len;
        pDCxt->strrclState = STRRCL_ST_ACTIVE;
        /* this call should shutdown the chip and maintain that state for msec_sleep milliseconds */
        Strrcl_Recall(pCxt, A_LE2CPU32(pEv->msec_sleep));
      }
      break;
    case STRRCL_ST_AWAIT_FW:
      pDCxt->strrclState = STRRCL_ST_INIT;
      pDCxt->strrclBlock = QOSAL_FALSE;
      /* Indicate completion to the custom layer */
      API_STORE_RECALL_EVENT(pCxt);
      break;
    default:
      /* do nothing but this should never happen */
      break;
  }
}
#endif
/*****************************************************************************/
/*  Api_StoreRecallEvent - Processes a WMI_STORERECALL_STORE_EVENT from the
 *	 device.
 *	 This event will come when the device is ready to be shutdown by the host
 *	 as part of a store-and-recall sequence.
 *              QOSAL_VOID *pCxt - the driver context.
 *		QOSAL_UINT8 *datap - original event buffer.
 *		QOSAL_INT32 len - length in bytes of datap
 *		QOSAL_VOID *pReq - original event rx-request object.
 *****************************************************************************/
QOSAL_VOID
Api_StoreRecallStartEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_INT32 len, QOSAL_VOID *pReq)
{
  WMI_STORERECALL_START_EVENT *pEv;
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);
//                                                                    QOSAL_UINT8* strrclData = pDCxt->tempStorage;

  UNUSED_ARGUMENT(devId);

  pEv = (WMI_STORERECALL_START_EVENT *)datap;
  if (pEv->msec_sleep == 0) {
    /* a 0 msec_sleep indicates a firmware condition which
     * prevented StoreRecall from completing.  in this event this
     * driver merely aborts the operation and continues normally.
     */
    pDCxt->strrclState = STRRCL_ST_INIT;
    pDCxt->strrclBlock = QOSAL_FALSE;
    API_STORE_RECALL_EVENT(pCxt);
    /* FIXME: Notify the application that S&R did not complete
     * successfully.
     */
  } else {
    /* indicate chip shutdown via chip_down. */
    //                                                            pDCxt->strrclCxt = datap;
    //                                                            pDCxt->strrclCxt = strrclData;
    //                                                            pDCxt->strrclCxtLen = len;
    A_PRINTF("total = %d   buff len=%d\n", pDCxt->strrclCxtLen, pDCxt->tempStorageLength);

    pDCxt->strrclState = STRRCL_ST_ACTIVE;
    /* this call should shutdown the chip and maintain that state for msec_sleep milliseconds */
    Strrcl_Recall(pCxt, A_LE2CPU32(pEv->msec_sleep));
  }
}

/*****************************************************************************/
/*  Api_WpsProfileEvent - Processes a WMI_WPS_PROFILE_EVENT from the device.
 *	 this event will come when the device has finished a WPS operation. it
 *	 announcess to the host the results of that operation. This function
 *	 stores the information so that it can be processed asynchronously by
 *	 an application thread waiting for the result.
 *              QOSAL_VOID *pCxt - the driver context.
 *		QOSAL_UINT8 *datap - original event buffer.
 *		QOSAL_INT32 len - length in bytes of datap
 *		QOSAL_VOID *pReq - original event rx-request object.
 *****************************************************************************/
QOSAL_VOID
Api_WpsProfileEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_INT32 len, QOSAL_VOID *pReq)
{
#if ENABLE_P2P_MODE
  //                                                              void *wmip;
#endif
  WMI_WPS_PROFILE_EVENT *pEv = (WMI_WPS_PROFILE_EVENT *)datap;
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);

#if ENABLE_P2P_MODE
  //                                                              WMI_P2P_PERSISTENT_PROFILE_CMD wpsProfile;
  //                                                              wmip = (struct wmi_t *)pCxt;
#endif

  UNUSED_ARGUMENT(len);
  UNUSED_ARGUMENT(devId);
  DRIVER_SHARED_RESOURCE_ACCESS_ACQUIRE(pCxt);

  do {
    /* cleanup previous buffer */
    if (pDCxt->wpsBuf != NULL) {
      A_NETBUF_FREE(pDCxt->wpsBuf);
      pDCxt->wpsBuf = NULL;
    }

    pDCxt->wpsBuf = pReq;
    pDCxt->wpsEvent = (QOSAL_VOID*)pEv;
    /* convert event values to host endian format */
    pEv->credential.auth_type = A_LE2CPU16(pEv->credential.auth_type);
    pEv->credential.encr_type = A_LE2CPU16(pEv->credential.encr_type);
    pEv->credential.ap_channel = A_LE2CPU16(pEv->credential.ap_channel);
    /* announce event receive to any waiting threads */
    pDCxt->wpsState = QOSAL_FALSE;
    API_WPS_PROFILE_EVENT(pCxt, datap, len, pReq);
  } while (0);

  DRIVER_SHARED_RESOURCE_ACCESS_RELEASE(pCxt);
}

#if WLAN_CONFIG_11N_AGGR_SUPPORT
/*****************************************************************************/
/*  Api_AggrRecvAddbaReqEvent - processes a WMI_ADDBA_REQ_EVENT from the device.
 *	 These events come when the device reveives a add-block-ACK packet from
 *	 an AP or peer device. This is only relevant for 11n aggregation.
 *      QOSAL_VOID *pCxt - the driver context.
 *		WMI_ADDBA_REQ_EVENT *evt - the event structure
 *****************************************************************************/
QOSAL_VOID
Api_AggrRecvAddbaReqEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, WMI_ADDBA_REQ_EVENT *evt)
{
  UNUSED_ARGUMENT(devId);
  evt->st_seq_no = A_LE2CPU16(evt->st_seq_no);

  if (evt->status == 0) {
    aggr_recv_addba_req_evt(GET_DRIVER_COMMON(pCxt)->pAggrCxt, evt->tid, evt->st_seq_no, evt->win_sz);
  }
  API_AGGR_RECV_ADDBA_REQ_EVENT(pCxt, evt);
}

/*****************************************************************************/
/*  Api_AggrRecvDelbaReqEvent - processes a WMI_DELBA_EVENT from the device.
 *	 These events come when the device reveives a delete-block-ACK packet from
 *	 an AP or peer device. This is only relevant for 11n aggregation.
 *      QOSAL_VOID *pCxt - the driver context.
 *		WMI_DELBA_EVENT *evt - the event structure
 *****************************************************************************/
QOSAL_VOID
Api_AggrRecvDelbaReqEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, WMI_DELBA_EVENT *evt)
{
  UNUSED_ARGUMENT(devId);
  aggr_recv_delba_req_evt(GET_DRIVER_COMMON(pCxt)->pAggrCxt, evt->tid);
  API_AGGR_RECV_DELBA_REQ_EVENT(pCxt, evt);
}
#endif /* WLAN_CONFIG_11N_AGGR_SUPPORT */

/*****************************************************************************/
/*  Api_ReadyEvent - processes a WMI_READY_EVENT from the device. records
 *	 usefull information from the event.
 *      QOSAL_VOID *pCxt - the driver context.
 *		QOSAL_UINT8 *datap - original event buffer.
 *		QOSAL_UINT8 phyCap - device phy capabilities.
 *		QOSAL_UINT32 sw_ver - device firmware version.
 *		QOSAL_UINT32 abi_ver - device ABI version.
 *****************************************************************************/
QOSAL_VOID
Api_ReadyEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT8 phyCap, QOSAL_UINT32 sw_ver, QOSAL_UINT32 abi_ver)
{
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);

  UNUSED_ARGUMENT(devId);
  Api_BootProfile(pCxt, BOOT_PROFILE_WMI_READY);

  pDCxt->wmiReady = QOSAL_TRUE;
  pDCxt->phyCapability = phyCap;
  pDCxt->wlanVersion = sw_ver;
  pDCxt->abiVersion = abi_ver;

  CUSTOM_API_READY_EVENT(pCxt, datap, phyCap, sw_ver, abi_ver);
}

/*****************************************************************************/
/*  Api_RSNASuccessEvent - processes a RSNA SUCCESS Message from the device.
 *      QOSAL_VOID *pCxt - the driver context.
 *		QOSAL_UINT8 code - success code.
 *****************************************************************************/
QOSAL_VOID
Api_RSNASuccessEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 code)
{
  UNUSED_ARGUMENT(devId);
  /* this will call the custom event which will call
   * the WifiCallback which will indicate the status
   */
  CUSTOM_API_RSNA_SUCCESS_EVENT(pCxt, devId, code);
}

/*****************************************************************************/
/* GetTemperatureReply - process the temperature reply from the device
 *      QOSAL_VOID *pCxt   - the driver context.
 *      QOSAL_UINT8 *datap - original event buffer.
 *      QOSAL_UINT8 len    - buffer length
 *****************************************************************************/
QOSAL_VOID
Api_GetTemperatureReply(QOSAL_VOID *pCxt, QOSAL_UINT8 *datap, QOSAL_UINT32 len)
{
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);
  WMI_GET_TEMPERATURE_REPLY *pReply = (WMI_GET_TEMPERATURE_REPLY *) datap;
  pDCxt->temperatureValid = QOSAL_TRUE;
  pDCxt->raw_temperature  = pReply->tempRegVal;
  pDCxt->tempDegree       = pReply->tempDegree;
  API_GET_TEMPERATURE_REPLY(pCxt, datap, len);
}

/*****************************************************************************/
/* GetCountryCodeReply - process the countrycode reply from the device
 *      QOSAL_VOID *pCxt   - the driver context.
 *      QOSAL_UINT8 *datap - original event buffer.
 *      QOSAL_UINT8 len    - buffer length
 *****************************************************************************/
QOSAL_VOID
Api_GetCountryCodeReply(QOSAL_VOID *pCxt, QOSAL_UINT8 *datap, QOSAL_UINT32 len)
{
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);
  WMI_GET_COUNTRY_CODE_REPLY *pReply = (WMI_GET_COUNTRY_CODE_REPLY *) datap;
  pDCxt->countryCodeValid = QOSAL_TRUE;
  A_MEMCPY(pDCxt->raw_countryCode, pReply->country_code, 3);
  A_PRINTF("code is %c , %c.\n", pDCxt->raw_countryCode[0], pDCxt->raw_countryCode[1]);
  API_GET_COUNTRY_CODE_REPLY(pCxt, datap, len);
}
/* GetSetParamReply - recv param set ack from the device
 *      QOSAL_VOID *pCxt   - the driver context.
 *      QOSAL_UINT8 *datap - original event buffer.
 *****************************************************************************/
QOSAL_VOID
Api_GetSetParamReply(QOSAL_VOID *pCxt, QOSAL_UINT8 *datap)
{
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);
  WMI_PARAM_SET_REPLY *reply = (WMI_PARAM_SET_REPLY *) datap;

  pDCxt->setparamValid = QOSAL_TRUE;
  pDCxt->setparamStatus = reply->status;
  API_SET_PARAM_REPLY_EVENT(pCxt);
}

#if ENABLE_P2P_MODE
/*****************************************************************************/
/*  p2p_go_neg_event   - processes a P2P GO Negotiation event from the device.
*      QOSAL_VOID *pCxt   - the driver context.
*      QOSAL_UINT8 *datap - original event buffer.
*      QOSAL_UINT8 len    - buffer length
*****************************************************************************/
QOSAL_VOID
p2p_go_neg_event(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len, WMI_P2P_PROV_INFO *wps_info)
{
  //                                                              A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);
  API_P2P_GO_NEG_EVENT(pCxt, devId, datap, len, wps_info);
}

QOSAL_VOID
p2p_invite_sent_result_event(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len)
{
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);
  API_P2P_INVITE_SEND_RESULT(pCxt, devId, datap, len);
}
/*****************************************************************************/
/*  p2p_node_list_event- event which gives the discovered nodes.
 *      QOSAL_VOID *pCxt   - the driver context.
 *      QOSAL_UINT8 *datap - original event buffer.
 *      QOSAL_UINT8 len    - buffer length
 *****************************************************************************/
QOSAL_VOID
p2p_node_list_event(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len)
{
  API_P2P_NODE_LIST_EVENT(pCxt, devId, datap, len);
}

/*****************************************************************************/
/*  p2p_list_persistent_network_event - event which gives the discovered nodes.
 *      QOSAL_VOID *pCxt   - the driver context.
 *      QOSAL_UINT8 *datap - original event buffer.
 *      QOSAL_UINT8 len    - buffer length
 *****************************************************************************/
QOSAL_VOID
p2p_list_persistent_network_event(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len)
{
  API_P2P_LIST_PERSISTENT_NETWORK_EVENT(pCxt, devId, datap, len);
}

/*****************************************************************************/
/*  p2p_req_auth_event - processes a response event for authentication request.
 *      QOSAL_VOID *pCxt   - the driver context.
 *      QOSAL_UINT8 *datap - original event buffer.
 *      QOSAL_UINT8 len    - buffer length
 *****************************************************************************/
QOSAL_VOID
p2p_req_auth_event(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len)
{
  API_P2P_REQ_AUTH_EVENT(pCxt, devId, datap, len);
}

QOSAL_VOID
get_p2p_ctx(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len)
{
  API_GET_P2P_CTX(pCxt, devId, datap, len);
}

QOSAL_VOID
get_p2p_prov_disc_req(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len)
{
  API_P2P_PROV_DISC_REQ(pCxt, devId, datap, len);
}
#if 1//KK
QOSAL_VOID
get_p2p_serv_disc_req(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len)
{
  API_P2P_SERV_DISC_REQ(pCxt, devId, datap, len);
}

QOSAL_VOID
p2p_invite_req_rx(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT8 len)
{
  API_P2P_INVITE_REQ(pCxt, devId, datap, len);
}

QOSAL_VOID
p2p_invite_rcvd_result_ev(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len)
{
  API_P2P_INVITE_RCVD_RESULT(pCxt, devId, datap, len);
}
#endif
#endif

#if MANUFACTURING_SUPPORT
QOSAL_VOID
Api_TestCmdEventRx(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len)
{
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);
  UNUSED_ARGUMENT(devId);
  API_TEST_RESP_EVENT(pCxt, datap, len);

  pDCxt->testResp = QOSAL_TRUE;
  DRIVER_WAKE_USER(pCxt);
}
#endif

/*****************************************************************************/
/*  Api_RxDbglogEvent - Processes a WMIX_DBGLOG_EVENT from the device.
 *****************************************************************************/
QOSAL_VOID
Api_RxDbglogEvent(QOSAL_VOID *wmip, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_INT32 len)
{
  UNUSED_ARGUMENT(devId);
  API_WMIX_DBGLOG_EVENT(wmip, datap, len);
}

/*****************************************************************************/
/*  Api_RxGpioDataEvent - Processes a WMIX_GPIO_DATA_EVENT from the device.
 *****************************************************************************/
QOSAL_VOID
Api_RxGpioDataEvent(QOSAL_VOID *wmip, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_INT32 len)
{
  UNUSED_ARGUMENT(devId);
  CUSTOM_API_WMIX_GPIO_DATA_EVENT(wmip, datap, len);
}

#if ENABLE_KF_PERFORMANCE

/*****************************************************************************/
/*  Api_RxPfmDataEvent - Processes a WMIX_PFM_DATA_EVENT from the device.
 *****************************************************************************/
QOSAL_VOID
Api_RxPfmDataEvent(QOSAL_VOID *wmip, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_INT32 len)
{
  UNUSED_ARGUMENT(devId);
  CUSTOM_API_WMIX_PFM_DATA_EVENT(wmip, datap, len);
}

/*****************************************************************************/
/*  Api_RxPfmDataDoneEvent - Processes a WMIX_PFM_DATA_DONE_EVENT from the device.
 *****************************************************************************/
QOSAL_VOID
Api_RxPfmDataDoneEvent(QOSAL_VOID *wmip, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_INT32 len)
{
  UNUSED_ARGUMENT(devId);
  CUSTOM_API_WMIX_PFM_DATA_DONE_EVENT(wmip, datap, len);
}
#endif

/*****************************************************************************/
/*  Api_PFMReportEvent - Processes a WMI_PFM_REPORT_EVENTID from the device.
 *****************************************************************************/
void add_one_profile(QOSAL_UINT8 event_id, QOSAL_UINT16  timestamp);

QOSAL_VOID
Api_PFMReportEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, WMI_PFM_REPORT_EVENT *pEv)
{
  QOSAL_UINT8   i;
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);

  UNUSED_ARGUMENT(devId);
  for (i = 0; i < MAX_TIMESTAMP; i++) {
    if (pEv->eventID[i] == 0) {
      break;
    }

    add_one_profile(pEv->eventID[i] + 15, (QOSAL_UINT16)pEv->timestamp[i] / 32);
  }
  pDCxt->pfmDone = QOSAL_TRUE;

  DRIVER_WAKE_USER(pCxt);
}

#if 0
/*****************************************************************************/
/*  Api_PFMReportEvent - Processes a WMI_PFM_REPORT_EVENTID from the device.
 *****************************************************************************/
QOSAL_VOID
Api_DiagReportEvent(QOSAL_VOID *pCxt, WMI_DIAG_EVENT *pEv)
{
  if (ath_custom_init.Boot_Profile == NULL) {
    return;
  }

/*	if (pEv->commandId == WMID_FSM_AUTH_EVENTID)
    {
    ath_custom_init.Boot_Profile(BOOT_PROFILE_AUTH_EVENT);
    }

   else if (pEv->commandId == WMID_FSM_ASSOC_EVENTID)
    {
    ath_custom_init.Boot_Profile(BOOT_PROFILE_ASSOC_EVENT);
    }
   else if (pEv->commandId == WMID_START_SCAN_EVENTID)
   {
    ath_custom_init.Boot_Profile(BOOT_PROFILE_START_SCAN);
   }
 */
}
#endif

#if ENABLE_P2P_MODE
QOSAL_VOID
Api_p2p_go_neg_event(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len, WMI_P2P_PROV_INFO *wps_info)
{
  QOSAL_UINT32 evt_id = 0;
  evt_id = WMI_P2P_GO_NEG_RESULT_EVENTID;
  A_DRIVER_CONTEXT* pDCxt = GET_DRIVER_COMMON(pCxt);

  DRIVER_SHARED_RESOURCE_ACCESS_ACQUIRE(pCxt);
  {
    A_MEMZERO(GET_DRIVER_COMMON(pCxt)->pScanOut, pDCxt->tempStorageLength);

    A_MEMCPY(GET_DRIVER_COMMON(pCxt)->pScanOut, &evt_id, sizeof(QOSAL_UINT32));

    A_MEMCPY((GET_DRIVER_COMMON(pCxt)->pScanOut) + sizeof(QOSAL_UINT32), datap, sizeof(WMI_P2P_GO_NEG_RESULT_EVENT));

    if (GET_DRIVER_COMMON(pCxt)->p2pEvtState == QOSAL_TRUE) {
      GET_DRIVER_COMMON(pCxt)->p2pEvtState = QOSAL_FALSE;
    } else {
      GET_DRIVER_COMMON(pCxt)->p2pevtflag = QOSAL_TRUE;
    }
  }
  DRIVER_SHARED_RESOURCE_ACCESS_RELEASE(pCxt);

  DRIVER_WAKE_USER(pCxt);
}

QOSAL_VOID
Api_p2p_node_list_event(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len)
{
  QOSAL_UINT32 evt_id = 0;
  QOSAL_UINT8 *tmpBuf, i;
  struct p2p_device *p2pDev;
  evt_id = WMI_P2P_NODE_LIST_EVENTID;
  WMI_P2P_NODE_LIST_EVENT *handleP2PDev = (WMI_P2P_NODE_LIST_EVENT *)datap;
  A_DRIVER_CONTEXT* pDCxt = GET_DRIVER_COMMON(pCxt);

  tmpBuf = GET_DRIVER_COMMON(pCxt)->pScanOut;
  DRIVER_SHARED_RESOURCE_ACCESS_ACQUIRE(pCxt);
  {
    A_MEMZERO(GET_DRIVER_COMMON(pCxt)->pScanOut, pDCxt->tempStorageLength);
    A_MEMCPY(GET_DRIVER_COMMON(pCxt)->pScanOut, &evt_id, sizeof(QOSAL_UINT32));
    tmpBuf += sizeof(QOSAL_UINT32);
    *tmpBuf = handleP2PDev->num_p2p_dev;
    tmpBuf++;

    A_MEMCPY((QOSAL_UINT8 *)tmpBuf, ((QOSAL_UINT8 *)(handleP2PDev->data)), (sizeof(P2P_DEVICE_LITE) * (handleP2PDev->num_p2p_dev)));

    GET_DRIVER_COMMON(pCxt)->p2pEvtState = QOSAL_FALSE;
  }
  DRIVER_SHARED_RESOURCE_ACCESS_RELEASE(pCxt);

  DRIVER_WAKE_USER(pCxt);
}

QOSAL_VOID
Api_p2p_req_auth_event(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len)
{
  QOSAL_UINT32 evt_id = 0;
  evt_id =  WMI_P2P_REQ_TO_AUTH_EVENTID;
  A_DRIVER_CONTEXT* pDCxt = GET_DRIVER_COMMON(pCxt);

  DRIVER_SHARED_RESOURCE_ACCESS_ACQUIRE(pCxt);
  {
    A_MEMZERO(GET_DRIVER_COMMON(pCxt)->pScanOut, pDCxt->tempStorageLength);

    A_MEMCPY(GET_DRIVER_COMMON(pCxt)->pScanOut, &evt_id, sizeof(QOSAL_UINT32));

    A_MEMCPY((GET_DRIVER_COMMON(pCxt)->pScanOut) + sizeof(QOSAL_UINT32), datap, sizeof(WMI_P2P_REQ_TO_AUTH_EVENT));

    GET_DRIVER_COMMON(pCxt)->p2pevtflag = QOSAL_TRUE;
  }
  DRIVER_SHARED_RESOURCE_ACCESS_RELEASE(pCxt);

  DRIVER_WAKE_USER(pCxt);
}

QOSAL_VOID
Api_p2p_list_persistent_network_event(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len)
{
  QOSAL_UINT32 evt_id = 0;
  evt_id =  WMI_P2P_LIST_PERSISTENT_NETWORK_EVENTID;
  A_DRIVER_CONTEXT* pDCxt = GET_DRIVER_COMMON(pCxt);
  WMI_P2P_PERSISTENT_LIST_NETWORK_EVENT *ev = (WMI_P2P_PERSISTENT_LIST_NETWORK_EVENT *)datap;

  DRIVER_SHARED_RESOURCE_ACCESS_ACQUIRE(pCxt);
  {
    A_MEMZERO(GET_DRIVER_COMMON(pCxt)->pScanOut, pDCxt->tempStorageLength);

    A_MEMCPY(GET_DRIVER_COMMON(pCxt)->pScanOut, &evt_id, sizeof(QOSAL_UINT32));

    A_MEMCPY((GET_DRIVER_COMMON(pCxt)->pScanOut) + sizeof(QOSAL_UINT32), ev->data, (MAX_LIST_COUNT * sizeof(WMI_PERSISTENT_MAC_LIST)));

    GET_DRIVER_COMMON(pCxt)->p2pEvtState = QOSAL_FALSE;
  }
  DRIVER_SHARED_RESOURCE_ACCESS_RELEASE(pCxt);

  DRIVER_WAKE_USER(pCxt);
}

QOSAL_VOID
Api_get_p2p_ctx(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len)
{
  QOSAL_UINT32 evt_id = 0;
  evt_id = WMI_P2P_PROV_DISC_RESP_EVENTID;
  A_DRIVER_CONTEXT* pDCxt = GET_DRIVER_COMMON(pCxt);

  DRIVER_SHARED_RESOURCE_ACCESS_ACQUIRE(pCxt);
  {
    A_MEMZERO(GET_DRIVER_COMMON(pCxt)->pScanOut, pDCxt->tempStorageLength);

    A_MEMCPY(GET_DRIVER_COMMON(pCxt)->pScanOut, &evt_id, sizeof(QOSAL_UINT32));

    A_MEMCPY((GET_DRIVER_COMMON(pCxt)->pScanOut) + sizeof(QOSAL_UINT32), datap, sizeof(WMI_P2P_PROV_DISC_RESP_EVENT));

    GET_DRIVER_COMMON(pCxt)->p2pEvtState = QOSAL_FALSE;
  }
  DRIVER_SHARED_RESOURCE_ACCESS_RELEASE(pCxt);

  DRIVER_WAKE_USER(pCxt);
}

QOSAL_VOID
Api_p2p_prov_disc_req(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len)
{
  QOSAL_UINT32 evt_id = 0;
  evt_id = WMI_P2P_PROV_DISC_REQ_EVENTID;
  A_DRIVER_CONTEXT* pDCxt = GET_DRIVER_COMMON(pCxt);

  DRIVER_SHARED_RESOURCE_ACCESS_ACQUIRE(pCxt);
  {
    A_MEMZERO(GET_DRIVER_COMMON(pCxt)->pScanOut, pDCxt->tempStorageLength);

    A_MEMCPY(GET_DRIVER_COMMON(pCxt)->pScanOut, &evt_id, sizeof(QOSAL_UINT32));

    A_MEMCPY((GET_DRIVER_COMMON(pCxt)->pScanOut) + sizeof(QOSAL_UINT32), datap, sizeof(WMI_P2P_PROV_DISC_REQ_EVENT));

    GET_DRIVER_COMMON(pCxt)->p2pevtflag = QOSAL_TRUE;
  }
  DRIVER_SHARED_RESOURCE_ACCESS_RELEASE(pCxt);

  DRIVER_WAKE_USER(pCxt);
}

QOSAL_VOID
Api_p2p_serv_disc_req(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len)
{
  QOSAL_UINT32 evt_id = 0;
  evt_id = WMI_P2P_SDPD_RX_EVENTID;
  A_DRIVER_CONTEXT* pDCxt = GET_DRIVER_COMMON(pCxt);
  DRIVER_SHARED_RESOURCE_ACCESS_ACQUIRE(pCxt);
  {
    A_MEMZERO(GET_DRIVER_COMMON(pCxt)->pScanOut, pDCxt->tempStorageLength);

    A_MEMCPY(GET_DRIVER_COMMON(pCxt)->pScanOut, &evt_id, sizeof(QOSAL_UINT32));

    A_MEMCPY((GET_DRIVER_COMMON(pCxt)->pScanOut) + sizeof(QOSAL_UINT32), datap, sizeof(WMI_P2P_SDPD_RX_EVENT));

    GET_DRIVER_COMMON(pCxt)->p2pevtflag = QOSAL_TRUE;
  }

  DRIVER_SHARED_RESOURCE_ACCESS_RELEASE(pCxt);

  DRIVER_WAKE_USER(pCxt);
}

QOSAL_VOID
Api_p2p_invite_req(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len)
{
  //                                                              async event for peer invite
  QOSAL_UINT32 evt_id = 0;
  evt_id = WMI_P2P_INVITE_REQ_EVENTID;
  A_DRIVER_CONTEXT* pDCxt = GET_DRIVER_COMMON(pCxt);

  DRIVER_SHARED_RESOURCE_ACCESS_ACQUIRE(pCxt);
  {
    A_MEMZERO(GET_DRIVER_COMMON(pCxt)->pScanOut, pDCxt->tempStorageLength);

    A_MEMCPY(GET_DRIVER_COMMON(pCxt)->pScanOut, &evt_id, sizeof(QOSAL_UINT32));

    A_MEMCPY((GET_DRIVER_COMMON(pCxt)->pScanOut) + sizeof(QOSAL_UINT32), datap, sizeof(WMI_P2P_FW_INVITE_REQ_EVENT));

    GET_DRIVER_COMMON(pCxt)->p2pevtflag = QOSAL_TRUE;
  }

  DRIVER_SHARED_RESOURCE_ACCESS_RELEASE(pCxt);

  DRIVER_WAKE_USER(pCxt);
}

QOSAL_VOID
Api_p2p_invite_rcvd_result(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len)
{
  QOSAL_UINT32 evt_id = 0;
  evt_id = WMI_P2P_INVITE_RCVD_RESULT_EVENTID;
  A_DRIVER_CONTEXT* pDCxt = GET_DRIVER_COMMON(pCxt);

  DRIVER_SHARED_RESOURCE_ACCESS_ACQUIRE(pCxt);
  {
    A_MEMZERO(GET_DRIVER_COMMON(pCxt)->pScanOut, pDCxt->tempStorageLength);

    A_MEMCPY(GET_DRIVER_COMMON(pCxt)->pScanOut, &evt_id, sizeof(QOSAL_UINT32));

    A_MEMCPY((GET_DRIVER_COMMON(pCxt)->pScanOut) + sizeof(QOSAL_UINT32), datap, sizeof(WMI_P2P_INVITE_RCVD_RESULT_EVENT));

    GET_DRIVER_COMMON(pCxt)->p2pevtflag = QOSAL_TRUE;
  }
  DRIVER_SHARED_RESOURCE_ACCESS_RELEASE(pCxt);

  DRIVER_WAKE_USER(pCxt);
}

QOSAL_VOID
Api_p2p_invite_send_result(QOSAL_VOID *pCxt, QOSAL_UINT8 devId, QOSAL_UINT8 *datap, QOSAL_UINT32 len)
{
  QOSAL_UINT32 evt_id = 0;
  A_DRIVER_CONTEXT* pDCxt = GET_DRIVER_COMMON(pCxt);
  evt_id = WMI_P2P_INVITE_SENT_RESULT_EVENTID;

  DRIVER_SHARED_RESOURCE_ACCESS_ACQUIRE(pCxt);
  {
    A_MEMZERO(GET_DRIVER_COMMON(pCxt)->pScanOut, pDCxt->tempStorageLength);

    A_MEMCPY(GET_DRIVER_COMMON(pCxt)->pScanOut, &evt_id, sizeof(QOSAL_UINT32));

    A_MEMCPY((GET_DRIVER_COMMON(pCxt)->pScanOut) + sizeof(QOSAL_UINT32), datap, sizeof(WMI_P2P_INVITE_SENT_RESULT_EVENT));

    GET_DRIVER_COMMON(pCxt)->p2pevtflag = QOSAL_TRUE;
  }
  DRIVER_SHARED_RESOURCE_ACCESS_RELEASE(pCxt);

  DRIVER_WAKE_USER(pCxt);
}

#endif
#if MANUFACTURING_SUPPORT
QOSAL_VOID
Api_Test_Cmd_Event(QOSAL_VOID *pCxt, QOSAL_UINT8 *datap, QOSAL_UINT32 len)
{
  UNUSED_ARGUMENT(len);
  A_MEMCPY(GET_DRIVER_COMMON(pCxt)->pScanOut, datap, len);
  GET_DRIVER_COMMON(pCxt)->testCmdRespBufLen = len;
}
#endif

QOSAL_VOID
Api_ProbeReqEvent(QOSAL_VOID *pCxt, QOSAL_UINT8 *datap, QOSAL_UINT32 len)
{
  CUSTOM_API_PROBEREQ_EVENT(pCxt, datap, len);
}
