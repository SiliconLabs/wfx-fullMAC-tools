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
#include <a_config.h>
#include <a_types.h>
#include <a_osapi.h>
#include <driver_cxt.h>
#include <custom_api.h>
#include <common_api.h>
#include <wmi_api.h>
#include <wmi_host.h>
#include <targaddrs.h>
#include <../hcd/hcd_api.h>
#include <spi_hcd_if.h>
#include "AR6002/hw2.0/hw/mbox_host_reg.h"
#include <atheros_wifi_api.h>

extern A_CONST WMI_SCAN_PARAMS_CMD default_scan_param;

#if DRIVER_CONFIG_PROGRAM_MAC_ADDR
/*****************************************************************************/
/*  program_mac_addr - Used to program a new mac address into the attached
 *	 wifi device. the mac address is in pDCxt->reqBssid.  Special firmware
 *	 is loaded to perform the operation.  the MAC address is provided to
 *       the firmware through the MBOX scratch registers.
 *              A_VOID *pCxt - the driver context.
 *****************************************************************************/
static A_STATUS
program_mac_addr(A_VOID *pCxt)
{
  A_DRIVER_CONTEXT *pDCxt;
  A_STATUS status = A_ERROR;
  A_UINT32 param;
  A_UINT8 pass_cycle = 0;
  A_NETBUF_DECLARE req;
  A_VOID *pReq = (A_VOID*)&req;
  A_UINT32 mac_word;
  A_UINT8 *ptr_mac_word = (A_UINT8*)&mac_word;
  A_UINT32 i;

  A_NETBUF_CONFIGURE(pReq, &mac_word, 0, sizeof(A_UINT32), sizeof(A_UINT32));
  pDCxt = GET_DRIVER_COMMON(pCxt);

  do {
    /* bring the chip down */
    pDCxt->wmiReady = A_FALSE;
    HW_PowerUpDown(pCxt, A_FALSE);
    /* bring the chip back up */
    HW_PowerUpDown(pCxt, A_TRUE);
    pDCxt->htc_creditInit = 0;
    /* re-initialize host controller (sub-)driver = HCD */
    if ((status = Hcd_ReinitTarget(pCxt)) != A_OK) {
      A_ASSERT(0);
    }

    if (pass_cycle == 0) {
      A_MDELAY(5);
      /* wait for chip to reach point in initialization */
      do {
        if (Driver_ReadDataDiag(pCxt,
                                TARG_VTOP(HOST_INTEREST_ITEM_ADDRESS(hi_refclk_hz)),
                                (A_UCHAR *)&param,
                                4) != A_OK) {
          A_ASSERT(0);
        }
      } while (param != A_CPU2LE32(EXPECTED_REF_CLK_AR4100)
               && param != A_CPU2LE32(EXPECTED_REF_CLK_AR400X));
      /* on 1st pass the mac address is loaded into the
       * scratch registers and the parameter is set to
       * cause the firmware to read/program the mac address.
       */
      ptr_mac_word[0] = 0;
      ptr_mac_word[1] = 0;
      A_MEMCPY(&ptr_mac_word[2], &pDCxt->conn[pDCxt->devId].reqBssid[0], 2);
      ATH_SET_PIO_EXTERNAL_WRITE_OPERATION(pReq, SCRATCH_ADDRESS, A_TRUE, sizeof(A_UINT32));

      if (A_OK != (status = Hcd_DoPioExternalAccess(pCxt, pReq))) {
        A_ASSERT(0);
      }

      A_MEMCPY(&ptr_mac_word[0], &pDCxt->conn[pDCxt->devId].reqBssid[2], 4);
      ATH_SET_PIO_EXTERNAL_WRITE_OPERATION(pReq, SCRATCH_ADDRESS + 4, A_TRUE, sizeof(A_UINT32));

      if (A_OK != (status = Hcd_DoPioExternalAccess(pCxt, pReq))) {
        A_ASSERT(0);
      }
      A_MDELAY(5);
      param = A_CPU2LE32(AR4XXX_PARAM_MACPROG_MODE);

      if (Driver_WriteDataDiag(pCxt,
                               TARG_VTOP(HOST_INTEREST_ITEM_ADDRESS(hi_flash_is_present)),
                               (A_UCHAR *)&param,
                               4) != A_OK) {
        A_ASSERT(0);
      }
      /* wait some sufficient number of attempts before giving up. */
      for (i = 0; i < 100; i++) {
        /* poll scratch register for indication of completion */
        ATH_SET_PIO_EXTERNAL_READ_OPERATION(pReq, SCRATCH_ADDRESS, A_TRUE, sizeof(A_UINT32));

        if (A_OK != (status = Hcd_DoPioExternalAccess(pCxt, pReq))) {
          A_ASSERT(0);
        }

        if (ptr_mac_word[1] != 0) {
          break; /* done - exit loop */
        }
        /* delay 1 msec before polling again to give the op time to complete */
        A_MDELAY(1);
      }
    } else {
      /* on 2nd pass the chip is brought up normally */
      A_MDELAY(5);
      Driver_BootComm(pCxt);
      pDCxt->chipDown = A_FALSE;
    }

    pass_cycle++;
  } while (pass_cycle < 2);

  if (pass_cycle >= 2) {
    /* store result in reqBssid for use in calling function */
    pDCxt->conn[pDCxt->devId].reqBssid[0] = ptr_mac_word[1];
  } else {
    pDCxt->conn[pDCxt->devId].reqBssid[0] = 0;
  }
  status = A_OK;
  pDCxt->asynchRequest = NULL;
  pDCxt->macProgramming = A_FALSE;
  CUSTOM_DRIVER_WAKE_USER(pCxt);

  return status;
}
#endif /* DRIVER_CONFIG_PROGRAM_MAC_ADDR */

/*****************************************************************************/
/*  Install_static_wep_keys - Used to install WEP keys to the device at the
 *	 appropriate time.
 *              A_VOID *pCxt - the driver context.
 *****************************************************************************/
static A_VOID
Install_static_wep_keys(A_VOID *pCxt)
{
  A_UINT8 index;
  A_UINT8 keyUsage;
  A_DRIVER_CONTEXT *pDCxt;
  WMI_ADD_CIPHER_KEY_CMD add_key_param;

  pDCxt = GET_DRIVER_COMMON(pCxt);

  for (index = WMI_MIN_KEY_INDEX; index <= WMI_MAX_KEY_INDEX; index++) {
    if (pDCxt->conn[pDCxt->devId].wepKeyList[index].keyLen) {
      keyUsage = GROUP_USAGE;
      if (index == pDCxt->conn[pDCxt->devId].wepDefTxKeyIndex) {
        keyUsage |= TX_USAGE;
      }

      A_MEMZERO(&add_key_param, sizeof(add_key_param));
      add_key_param.keyIndex = index;
      add_key_param.keyType  = WEP_CRYPT;
      add_key_param.keyUsage = keyUsage;
      add_key_param.keyLength = pDCxt->conn[pDCxt->devId].wepKeyList[index].keyLen;
      A_MEMCPY(&add_key_param.key, pDCxt->conn[pDCxt->devId].wepKeyList[index].key, add_key_param.keyLength);
      add_key_param.key_op_ctrl = KEY_OP_INIT_VAL;
      wmi_cmd_start(pDCxt->pWmiCxt, &add_key_param,
                    WMI_ADD_CIPHER_KEY_CMDID, sizeof(WMI_ADD_CIPHER_KEY_CMD));
    }
  }
}

/*****************************************************************************/
/*  Api_DisconnectWiFi - Called by upper layer to disconnect a network
 *   connection. Executes wmi_Disconnnect_cmd
 *              A_VOID *pCxt - the driver context.
 *****************************************************************************/
A_STATUS
Api_DisconnectWiFi(A_VOID *pCxt)
{
  A_DRIVER_CONTEXT *pDCxt;

  pDCxt = GET_DRIVER_COMMON(pCxt);

  if ((pDCxt->conn[pDCxt->devId].isConnected == A_TRUE) || (pDCxt->conn[pDCxt->devId].isConnectPending == A_TRUE)) {
    wmi_cmd_start(pDCxt->pWmiCxt, NULL, WMI_DISCONNECT_CMDID, 0);
    /*
     * Disconnect cmd is issued, clear connectPending.
     * arConnected will be cleard in disconnect_event notification.
     */
    pDCxt->conn[pDCxt->devId].isConnectPending = A_FALSE;
    /*
     * clear connect state so that subsequent connect commands start with same
     * initial configuration every time.
     */
    pDCxt->conn[pDCxt->devId].dot11AuthMode = OPEN_AUTH;
    pDCxt->conn[pDCxt->devId].wpaAuthMode = NONE_AUTH;
    pDCxt->conn[pDCxt->devId].wpaPairwiseCrypto = pDCxt->conn[pDCxt->devId].wpaGroupCrypto = NONE_CRYPT;
    pDCxt->conn[pDCxt->devId].wpaPairwiseCryptoLen = pDCxt->conn[pDCxt->devId].wpaGroupCryptoLen = 0;
    A_MEMZERO(pDCxt->conn[pDCxt->devId].reqBssid, sizeof(pDCxt->conn[pDCxt->devId].reqBssid));
    pDCxt->conn[pDCxt->devId].channelHint        = 0;
    pDCxt->conn[pDCxt->devId].connectCtrlFlags = 0;
  }

  return A_OK;
}

/*****************************************************************************/
/*  Api_ConnectWiFi - Called by upper layer to start a network connection
 *	 operation.  Executes wmi_connnect_cmd
 *              A_VOID *pCxt - the driver context.
 *****************************************************************************/
A_STATUS
Api_ConnectWiFi(A_VOID *pCxt)
{
  A_DRIVER_CONTEXT *pDCxt;
  A_STATUS status;
  WMI_CONNECT_CMD conn_cmd     = { 0 };
  WMI_SCAN_PARAMS_CMD scan_param_cmd;
  A_UINT8 devId;

  pDCxt = GET_DRIVER_COMMON(pCxt);
  devId = pDCxt->devId;
  /* The ssid length check prevents second "essid off" from the user,
       to be treated as a connect cmd. The second "essid off" is ignored.
   */
  if ((pDCxt->wmiReady == A_TRUE) && (pDCxt->conn[devId].ssidLen > 0) && pDCxt->conn[devId].networkType != AP_NETWORK) {
    if (/*(ADHOC_NETWORK != ar->arNetworkType) &&*/         //Allow wep key installation in AD-HOC mode
      (NONE_AUTH == pDCxt->conn[pDCxt->devId].wpaAuthMode)
      && (WEP_CRYPT == pDCxt->conn[pDCxt->devId].wpaPairwiseCrypto)) {
      Install_static_wep_keys(pCxt);
    }
    /* set scan ctrl flags to default. this will ensure that the firmware will scan for
     * an appropriate AP during connect operation. All params that are zero will remain
     * unchanged from their pre existing value. If WPS had been previously used it would
     * have set the scan ctrl flags in such a way that a subsequent connect might fail.
     * This wmi_scanparams_cmd() fixes that issue.
     */
    do {
      A_MEMCPY(&scan_param_cmd, &default_scan_param, sizeof(WMI_SCAN_PARAMS_CMD));
      scan_param_cmd.bg_period        = A_CPU2LE16(0xffff);
      if (A_OK != wmi_cmd_start(pDCxt->pWmiCxt, &scan_param_cmd,
                                WMI_SET_SCAN_PARAMS_CMDID, sizeof(WMI_SCAN_PARAMS_CMD))) {
        break;
      } else {
        CUSTOM_WAIT_FOR_WMI_RESPONSE(pCxt);
      }

      if (pDCxt->conn[devId].ssidLen) {
        A_MEMCPY(conn_cmd.ssid, pDCxt->conn[devId].ssid, pDCxt->conn[devId].ssidLen);
      }

      conn_cmd.ssidLength          = pDCxt->conn[devId].ssidLen;
      conn_cmd.networkType         = pDCxt->conn[devId].networkType;
      conn_cmd.dot11AuthMode       = pDCxt->conn[devId].dot11AuthMode;
      conn_cmd.authMode            = pDCxt->conn[devId].wpaAuthMode;
      conn_cmd.pairwiseCryptoType  = pDCxt->conn[devId].wpaPairwiseCrypto;
      conn_cmd.pairwiseCryptoLen   = pDCxt->conn[devId].wpaPairwiseCryptoLen;
      conn_cmd.groupCryptoType     = pDCxt->conn[devId].wpaGroupCrypto;
      conn_cmd.groupCryptoLen      = pDCxt->conn[devId].wpaGroupCryptoLen;
      conn_cmd.channel             = A_CPU2LE16(pDCxt->conn[devId].channelHint);
      conn_cmd.ctrl_flags          = A_CPU2LE32(pDCxt->conn[devId].connectCtrlFlags);

      if (pDCxt->conn[pDCxt->devId].reqBssid != NULL) {
        A_MEMCPY(conn_cmd.bssid, pDCxt->conn[devId].reqBssid, ATH_MAC_LEN);
      }

      status = wmi_cmd_start(pDCxt->pWmiCxt, (A_VOID*)&conn_cmd,
                             WMI_CONNECT_CMDID, sizeof(WMI_CONNECT_CMD));

      if (status != A_OK) {
        break;
      } else {
        CUSTOM_WAIT_FOR_WMI_RESPONSE(pCxt);
      }

      pDCxt->conn[pDCxt->devId].isConnectPending = A_TRUE;
    } while (0);

    return status;
  }
#if ENABLE_AP_MODE
  else if (pDCxt->wmiReady == A_TRUE && pDCxt->conn[pDCxt->devId].networkType == AP_NETWORK) {
    if ((NONE_AUTH == pDCxt->conn[pDCxt->devId].wpaAuthMode)
        && (WEP_CRYPT == pDCxt->conn[pDCxt->devId].wpaPairwiseCrypto)) {
      Install_static_wep_keys(pCxt);
    }
    A_MEMCPY(conn_cmd.ssid, (A_UCHAR*)pDCxt->conn[devId].ssid, sizeof(conn_cmd.ssid));
    conn_cmd.ssidLength          = (int)pDCxt->conn[devId].ssidLen;
    conn_cmd.pairwiseCryptoLen   = (A_UINT8)pDCxt->conn[devId].wpaPairwiseCryptoLen;
    conn_cmd.groupCryptoLen      = (A_UINT8)pDCxt->conn[devId].wpaGroupCryptoLen;
    conn_cmd.networkType        = pDCxt->conn[devId].networkType;
    conn_cmd.dot11AuthMode       = pDCxt->conn[devId].dot11AuthMode;
    conn_cmd.authMode       = pDCxt->conn[devId].wpaAuthMode;
    conn_cmd.pairwiseCryptoType  = pDCxt->conn[devId].wpaPairwiseCrypto;
    conn_cmd.groupCryptoType     = pDCxt->conn[devId].wpaGroupCrypto;

    conn_cmd.ctrl_flags          = (pDCxt->apmodeWPS) ? A_CPU2LE32(pDCxt->conn[devId].connectCtrlFlags | CONNECT_WPS_FLAG) : A_CPU2LE32(pDCxt->conn[devId].connectCtrlFlags);
    conn_cmd.channel             = A_CPU2LE16(pDCxt->conn[devId].channelHint);

    status = wmi_cmd_start(pDCxt->pWmiCxt, (A_VOID*)&conn_cmd,
                           WMI_AP_CONFIG_COMMIT_CMDID, sizeof(WMI_CONNECT_CMD));
    return status;
  }
#endif /* ENABLE_AP_MODE */

  return A_ERROR;
}

static A_UINT8
wpa_auth_parse(A_UINT8 *sel)
{
#define WPA_SEL(x)  (((x) << 24) | WPA_OUI)
  A_UINT32 w = A_LE_READ_4(sel);

  switch (w) {
    case WPA_SEL(WPA_ASE_8021X_UNSPEC):
      return WPA_AUTH;
    case WPA_SEL(WPA_ASE_8021X_PSK):
      return WPA_PSK_AUTH;
    case WPA_SEL(WPA_ASE_NONE):
      return NONE_AUTH;
  }

  return 0;
#undef WPA_SEL
}

static A_UINT8
wpa_cipher_parse(A_UINT8 *sel, A_UINT8 *keylen)
{
#define WPA_SEL(x)  (((x) << 24) | WPA_OUI)
  A_UINT32 w = A_LE_READ_4(sel);

  switch (w) {
    case WPA_SEL(WPA_CSE_NULL):
      return NONE_CRYPT;
    case WPA_SEL(WPA_CSE_WEP40):
      if (keylen) {
        *keylen = 40 >> 3;  //40/bits_per_byte
      }
      return WEP_CRYPT;
    case WPA_SEL(WPA_CSE_WEP104):
      if (keylen) {
        *keylen = 104 >> 3;  //104/bits_per_byte
      }
      return WEP_CRYPT;
    case WPA_SEL(WPA_CSE_TKIP):
      return TKIP_CRYPT;
    case WPA_SEL(WPA_CSE_CCMP):
      return AES_CRYPT;
  }

  return 0;
#undef WPA_SEL
}

static CRYPTO_TYPE
rsn_cipher_parse(A_UINT8 *sel, A_UINT8 *keylen)
{
#define RSN_SEL(x)  (((x) << 24) | RSN_OUI)
  A_UINT32 w = A_LE_READ_4(sel);

  switch (w) {
    case RSN_SEL(RSN_CSE_NULL):
      return NONE_CRYPT;
    case RSN_SEL(RSN_CSE_WEP40):
      if (keylen) {
        *keylen = 40 >> 3;  //40/bits_per_byte
      }
      return WEP_CRYPT;
    case RSN_SEL(RSN_CSE_WEP104):
      if (keylen) {
        *keylen = 104 >> 3;  //104/bits_per_byte
      }
      return WEP_CRYPT;
    case RSN_SEL(RSN_CSE_TKIP):
      return TKIP_CRYPT;
    case RSN_SEL(RSN_CSE_CCMP):
      return AES_CRYPT;
  }
  return NONE_CRYPT;
#undef RSN_SEL
}

static A_UINT8
rsn_auth_parse(A_UINT8 *sel)
{
#define RSN_SEL(x)  (((x) << 24) | RSN_OUI)
  A_UINT32 w = A_LE_READ_4(sel);

  switch (w) {
    case RSN_SEL(RSN_ASE_8021X_UNSPEC):
      return WPA2_AUTH;
    case RSN_SEL(RSN_ASE_8021X_PSK):
      return WPA2_PSK_AUTH;
    case RSN_SEL(RSN_ASE_NONE):
      return NONE_AUTH;
  }

  return 0;
#undef RSN_SEL
}

static A_VOID
security_ie_parse(A_UINT8* pie, A_UINT8 ie_len, A_UINT8 *pResult, A_UINT8 ie_type)
{
  A_UINT16 cnt;
  A_UINT16 i;
  A_UINT8 wepKeyLen;
  /* skip mcast cipher */
  if (ie_len >= 4) {
    ie_len -= 4;
    pie += 4;
  }
  /* examine ucast cipher(s) */
  if (ie_len > 2) {
    cnt = A_LE_READ_2(pie);
    ie_len -= 2;
    pie += 2;

    for (i = 0; ((i < cnt) && (ie_len > 0)); i++) {
      if (ie_type == IEEE80211_ELEMID_RSN) {
        pResult[0] |= rsn_cipher_parse(pie, &wepKeyLen);
      } else {
        pResult[0] |= wpa_cipher_parse(pie, &wepKeyLen);
      }

      ie_len -= 4;
      pie += 4;
    }
  } else {
    /* assume default TKIP for wpa */
    pResult[0] |= (ie_type == IEEE80211_ELEMID_RSN) ? AES_CRYPT : TKIP_CRYPT;
  }
  /* parse auth types */
  if (ie_len > 2) {
    cnt = A_LE_READ_2(pie);
    ie_len -= 2;
    pie += 2;

    for (i = 0; ((i < cnt) && (ie_len > 0)); i++) {
      if (ie_type == IEEE80211_ELEMID_RSN) {
        pResult[1] |= rsn_auth_parse(pie);
      } else {
        pResult[1] |= wpa_auth_parse(pie);
      }

      ie_len -= 4;
      pie += 4;
    }
  } else {
    pResult[1] |= (ie_type == IEEE80211_ELEMID_RSN) ? WPA2_AUTH : WPA_AUTH;
  }
}

A_STATUS
Api_ParseInfoElem(A_VOID *pCxt, WMI_BSS_INFO_HDR *bih, A_INT32 len, A_SCAN_SUMMARY *pSummary)
{
  A_UINT8* buf;
  A_UINT8 *pie, *pieEnd, *pieTemp;
  A_UINT8 ie_result[2];
  A_UINT16 ie_len;

  A_MEMZERO(pSummary, sizeof(A_SCAN_SUMMARY));

  pSummary->channel = (A_UINT8)Util_Freq2ieee(bih->channel);
  pSummary->rssi = bih->snr;/* snr is a value from 0 -> 95 where 95 is a strong signal */
  A_MEMCPY(pSummary->bssid, bih->bssid, ATH_MAC_LEN);

  if (len < sizeof(WMI_BSS_INFO_HDR) + IE_INDEX) {
    return A_ERROR;
  }

  buf = (A_UINT8*)(bih + 1);
  len -= sizeof(WMI_BSS_INFO_HDR);

  pSummary->caps = (A_UINT16)((A_UINT16)buf[CAPS_IE_INDEX] | (A_UINT16)(buf[CAPS_IE_INDEX + 1] << 8));
  pSummary->beacon_period = (A_UINT16)((A_UINT16)buf[BEACON_PERIOD_IE_INDEX] | (A_UINT16)(buf[BEACON_PERIOD_IE_INDEX + 1] << 8));
  /* loop through IE's to collect additional info */
  pie = &buf[IE_INDEX];
  len -= IE_INDEX;
  pieEnd = &pie[len];

  while (pie < pieEnd) {
    switch (*pie) {
      case IEEE80211_ELEMID_SSID:
        if (pie[1] <= 32) {
          pSummary->ssid_len = pie[1];

          if (pie[1]) {
            A_MEMCPY(pSummary->ssid, &pie[2], pie[1]);
          }
        }
        break;
      case IEEE80211_ELEMID_RSN:
        /*******************/
        /* parse RSN IE    */
        /*******************/
        ie_len = pie[1];     /* init ie_len - sizeof wpa_oui */
        pieTemp = &pie[2];     /* init pieTemp beyond wpa_oui */

        if (A_LE_READ_2(pieTemp) != RSN_VERSION) {
          break;
        }
        ie_len -= 2;
        pieTemp += 2;
        ie_result[0] = ie_result[1] = 0;

        security_ie_parse(pieTemp, ie_len, &ie_result[0], IEEE80211_ELEMID_RSN);
        pSummary->rsn_cipher = ie_result[0];
        pSummary->rsn_auth = ie_result[1];
        break;
      case IEEE80211_ELEMID_VENDOR:
        if (pie[1] > 6 && A_LE_READ_4(pie + 2) == ((WPA_OUI_TYPE << 24) | WPA_OUI)) {
          /*******************/
          /* parse WPA IE    */
          /*******************/
          ie_len = pie[1] - 4;   /* init ie_len - sizeof wpa_oui */
          pieTemp = &pie[6];     /* init pieTemp beyond wpa_oui */

          if (A_LE_READ_2(pieTemp) != WPA_VERSION) {
            break;   /* bad version */
          }

          ie_len -= 2;
          pieTemp += 2;
          ie_result[0] = ie_result[1] = 0;

          security_ie_parse(pieTemp, ie_len, &ie_result[0], IEEE80211_ELEMID_VENDOR);
          pSummary->wpa_cipher = ie_result[0];
          pSummary->wpa_auth = ie_result[1];
        }
        break;
    }

    pie += pie[1] + 2;
  }

  return A_OK;
}

/*****************************************************************************/
/*  Api_DriverAccessCheck - Called by upper layer to confirm that the driver
 *   is in a State to allow IOCTL's and other requests.
 *              A_VOID *pCxt - the driver context.
 *      A_UINT8 block_allowed - caller can tolerate a task block if the
 *      check is not immediately satisfied.
 *      A_UINT8 request_reason - callers reason to request driver access.
 *****************************************************************************/
A_STATUS
Api_DriverAccessCheck(A_VOID *pCxt, A_UINT8 block_allowed, A_UINT8 request_reason)
{
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);
  A_STATUS status = A_OK;
  A_UINT8 blocked;

  UNUSED_ARGUMENT(request_reason);

  do {
    blocked = 0;
    /* if the chip is powered down when a tx operation is submitted
     * then the policy is to wait for the chip to come back */
    if (A_TRUE == pDCxt->chipDown) {
      blocked = 1;

      if (block_allowed == 0) {
        /* caller forbids blocking so return A_ERROR */
        status = A_ERROR;
        break;
      }

      do {
        CUSTOM_DRIVER_WAIT_FOR_CONDITION(pCxt, &(pDCxt->chipDown), A_FALSE, 5000);
      } while (A_TRUE == pDCxt->chipDown);
    }
#if DRIVER_CONFIG_ENABLE_STORE_RECALL
    /*Check str-rcl state only if applications requests it*/
    if (request_reason != ACCESS_REQUEST_RX) {
      /* if a strrcl command is in process all ioctl's and tx operations
       * must be blocked/deferred until it completes.
       */
      if (A_TRUE == pDCxt->strrclBlock) {
        blocked = 1;

        if (block_allowed == 0) {
          /* caller forbids blocking so return A_ERROR */
          status = A_ERROR;
          break;
        }

        do {
          CUSTOM_DRIVER_WAIT_FOR_CONDITION(pCxt, &(pDCxt->strrclBlock), A_FALSE, 5000);
        } while (A_TRUE == pDCxt->strrclBlock);
      }
    }
#endif
  } while (blocked); /* loop until all conditions are met at one pass */

  return status;
}

#if DRIVER_CONFIG_PROGRAM_MAC_ADDR
A_STATUS
Api_ProgramMacAddress(A_VOID *pCxt, A_UINT8* addr, A_UINT16 length, A_UINT8 *pResult)
{
  A_STATUS status = A_ERROR;
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);
  /* init pResult to an error code of 0 */
  *pResult = ATH_PROGRAM_MAC_RESULT_DRIVER_FAILED;

  do {
    if (length != sizeof(ATH_PROGRAM_MAC_ADDR_PARAM)) {
      break;
    }

    if (Api_DriverAccessCheck(pCxt, 0, ACCESS_REQUEST_IOCTL) != A_OK) {
      break;
    }

    if (pDCxt->asynchRequest != NULL) {
      break;
    }
    /* use the reqBssid as storage for the mac address.  the program_mac_addr
     * function will retrieve this value for use in the operation.
     */
    A_MEMCPY(pDCxt->conn[pDCxt->devId].reqBssid, addr, sizeof(pDCxt->conn[pDCxt->devId].reqBssid));
    pDCxt->macProgramming = A_TRUE;
    pDCxt->asynchRequest = program_mac_addr;
    CUSTOM_DRIVER_WAKE_DRIVER(pCxt);
    CUSTOM_DRIVER_WAIT_FOR_CONDITION(pCxt, &(pDCxt->macProgramming), A_FALSE, 5000);

    if (pDCxt->macProgramming == A_FALSE) {
      switch (pDCxt->conn[pDCxt->devId].reqBssid[0]) {
        case 1:/*successful result*/
          *pResult = ATH_PROGRAM_MAC_RESULT_SUCCESS;
          break;
        case 2:/* device failed in the attempt */
          *pResult = ATH_PROGRAM_MAC_RESULT_DEV_FAILED;
          break;
        case 4:/* the same mac address is already programmed */
        case 8:/* the device rejected the mac address */
          *pResult = ATH_PROGRAM_MAC_RESULT_DEV_DENIED;
          break;
        case 0:/* driver failure to communicate with device */
        default:
          break;
      }

      Api_InitFinish(pCxt);
      Api_WMIInitFinish(pCxt);
      status = A_OK;
    }
  } while (0);

  A_MEMZERO(pDCxt->conn[pDCxt->devId].reqBssid, sizeof(pDCxt->conn[pDCxt->devId].reqBssid));

  return status;
}
#endif /* DRIVER_CONFIG_PROGRAM_MAC_ADDR */
A_STATUS
Api_SetPowerMode(A_VOID *pCxt, POWER_MODE *app_setting)
{
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);

  if (app_setting->pwr_module < PWR_MAX) {
    (app_setting->pwr_mode == REC_POWER) ? (pDCxt->pwrStateSetting &= ~(1 << app_setting->pwr_module)) : (pDCxt->pwrStateSetting |= (1 << app_setting->pwr_module));
  }
  /* Set MAX Perf */
  if ((app_setting->pwr_mode == MAX_PERF_POWER)
      && (pDCxt->userPwrMode == REC_POWER)) {
    pDCxt->userPwrMode = MAX_PERF_POWER;
  }
  /* Set REC Power */
  if ((app_setting->pwr_mode == REC_POWER) && (pDCxt->pwrStateSetting == 0x00)) {
    pDCxt->userPwrMode = REC_POWER;
  }

  if (A_OK != wmi_cmd_start(pDCxt->pWmiCxt, &pDCxt->userPwrMode, WMI_SET_POWER_MODE_CMDID, sizeof(WMI_POWER_MODE_CMD))) {
    return A_ERROR;
  }

  return A_OK;
}
/* EOF */
