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
#ifndef _DRIVER_CXT_H_
#define _DRIVER_CXT_H_

#include "a_types.h"
#include <athdefs.h>
#include <htc_api.h>
#include <wmi.h>
#include "ieee80211.h"

#define MIN_STRRCL_MSEC (100)

#define AR4100_DEFAULT_CREDIT_COUNT     6

#define AR4100_BUFFER_SIZE                1664

#define AR4100_MAX_AMSDU_RX_BUFFERS       4
#define AR4100_AMSDU_REFILL_THRESHOLD     3

#define AR4100_AMSDU_BUFFER_SIZE          (WMI_MAX_AMSDU_RX_DATA_FRAME_LENGTH + 128)

#if WLAN_CONFIG_AMSDU_ON_HOST
#define AR4100_MAX_RX_MESSAGE_SIZE        (max(WMI_MAX_NORMAL_RX_DATA_FRAME_LENGTH, WMI_MAX_AMSDU_RX_DATA_FRAME_LENGTH))
/* The RX MESSAGE SIZE is ultimately dictated by the size of the firmware buffers used to
 * hold packets.  This is because any remaining space may be used by HTC for trailers. hence
 * a full size buffer must be anticipated by the host. */
#else
#define AR4100_MAX_RX_MESSAGE_SIZE      (AR4100_BUFFER_SIZE)
#endif

#define MAX_NUM_CHANLIST_CHANNELS (16)

#define MAX_WEP_KEY_SZ (16)

#define STRRCL_ST_DISABLED 0
#define STRRCL_ST_INIT 1
#define STRRCL_ST_START 2
#define STRRCL_ST_ACTIVE 3
#define STRRCL_ST_ACTIVE_B 4
#define STRRCL_ST_AWAIT_FW 5

#define AES_KEY_SIZE_BYTES  16

#define ACCESS_REQUEST_IOCTL 0
#define ACCESS_REQUEST_TX    1
#define ACCESS_REQUEST_RX    2

#define DRIVER_SCOPE_RX 0x01
#define DRIVER_SCOPE_TX 0x02

#define DRIVER_STATE_IDLE 0x00 /* the driver task is idle/sleeping */
#define DRIVER_STATE_RX_PROCESSING 0x01 /* the driver task is processing a RX request */
#define DRIVER_STATE_TX_PROCESSING 0x02 /* the driver task is processing a TX request */
#define DRIVER_STATE_INTERRUPT_PROCESSING 0x03 /* the driver task is processing a chip Interrupt */
#define DRIVER_STATE_ASYNC_PROCESSING 0x04 /* the driver task is processing an asynch operation */
#define DRIVER_STATE_PENDING_CONDITION_A 0x05 /* the driver task is waiting for a condition to be met */
#define DRIVER_STATE_PENDING_CONDITION_B 0x06
#define DRIVER_STATE_PENDING_CONDITION_C 0x07
#define DRIVER_STATE_PENDING_CONDITION_D 0x08
#define DRIVER_STATE_PENDING_CONDITION_E 0x09
#define DRIVER_STATE_PENDING_CONDITION_F 0x0a
#define DRIVER_STATE_PENDING_CONDITION_G 0x0b

#define MAX_CREDIT_DEADLOCK_ATTEMPTS 2
#define DEADLOCK_BLOCK_MSEC 10

typedef struct ar_wep_key {
  QOSAL_UINT8 keyLen;
  QOSAL_UINT8 key[MAX_WEP_KEY_SZ];
}A_WEPKEY_T;

/* A_SCAN_SUMMARY provides useful information that
 * has been parsed from bss info headers and probe/beacon information elements.
 */
typedef struct scan_summary {
  QOSAL_UINT16 beacon_period;
  QOSAL_UINT16 caps;
  QOSAL_UINT8 bssid[6];
  QOSAL_UINT8 ssid[32];
  QOSAL_UINT8 channel;
  QOSAL_UINT8 ssid_len;
  QOSAL_UINT8 rssi;
  QOSAL_UINT8 bss_type;
  QOSAL_UINT8 rsn_cipher;
  QOSAL_UINT8 rsn_auth;
  QOSAL_UINT8 wpa_cipher;
  QOSAL_UINT8 wpa_auth;
  QOSAL_UINT8 wep_support;
  QOSAL_UINT8 reserved;   //keeps struct on 4-byte boundary
}A_SCAN_SUMMARY;

typedef struct {
  QOSAL_UINT16 maxMsgLength;
  QOSAL_UINT16 serviceID;
  QOSAL_UINT32 intCredits;
  QOSAL_UINT8 intCreditCounter;
  QOSAL_UINT8 rptCreditCounter;
  QOSAL_UINT8 epIdx;
  QOSAL_UINT8 credits;
  QOSAL_UINT8 maxCredits;
  QOSAL_UINT8 seqNo;
}A_ENDPOINT_T;

#if USE_16BIT_CREDIT_COUNTER
typedef struct {
  QOSAL_UINT16 maxMsgLength;
  QOSAL_UINT16 serviceID;
  QOSAL_UINT32 intCredits;
  QOSAL_UINT16 intCreditCounter;
  QOSAL_UINT16 rptCreditCounter;
  QOSAL_UINT8 epIdx;
  QOSAL_UINT8 rsvd;
  QOSAL_UINT16 credits;
  QOSAL_UINT16 maxCredits;
  QOSAL_UINT8 seqNo;
}A_ENDPOINT_T;
#endif

typedef A_STATUS (*TEMP_FUNC_T)(QOSAL_VOID *pCxt);

typedef struct _a_connection_elements {
  QOSAL_INT32         ssidLen;
  QOSAL_UINT8         ssid[32];
  QOSAL_UINT8         networkType;   /* one of NETWORK_TYPE enum */
  QOSAL_UINT32        connectCtrlFlags;
  QOSAL_UINT8         dot11AuthMode;
  QOSAL_UINT8         wpaAuthMode;
  QOSAL_UINT8         wpaPairwiseCrypto;
  QOSAL_UINT8         wpaPairwiseCryptoLen;
  QOSAL_UINT8         wpaGroupCrypto;
  QOSAL_UINT8         wpaGroupCryptoLen;
  QOSAL_UINT8         wepDefTxKeyIndex;
  A_WEPKEY_T    wepKeyList[WMI_MAX_KEY_INDEX + 1];
  QOSAL_UINT8       wpaPmk[WMI_PMK_LEN];
  QOSAL_BOOL          wpaPmkValid;
  QOSAL_UINT8         bssid[ATH_MAC_LEN];
  QOSAL_UINT8         reqBssid[ATH_MAC_LEN];
  QOSAL_UINT16        channelHint;
  QOSAL_UINT16        bssChannel;
  QOSAL_BOOL          isConnected;
  QOSAL_BOOL          isConnectPending;
  QOSAL_UINT8     phyMode;
}A_CONNECTION_ELEMENTS_T;

typedef struct _a_spi_hcd_context{
    #define SDHD_BOOL_SHUTDOWN          0x00000001
    #define SDHD_BOOL_EXTERNALIO_PENDING    0x00000002
    #define SDHD_BOOL_DMA_WRITE_WAIT_FOR_BUFFER 0x00000004
    #define SDHD_BOOL_DMA_IN_PROG         0x00000008
    #define SDHD_BOOL_TRANSFER_DIR_RX     0x00000010
    #define SDHD_BOOL_HOST_DMA_COPY_MODE    0x00000020
    #define SDHD_BOOL_HOST_ACCESS_COPY_MODE   0x00000040
    #define SDHD_BOOL_FATAL_ERROR       0x00000080
    #define SDHD_BOOL_DMA_COMPLETE              0x00000100

#define BYTE_SWAP    QOSAL_TRUE
#define NO_BYTE_SWAP QOSAL_FALSE

  /*******************************************
   *
   * the following fields must be filled in by the hardware specific layer
   *
   ********************************************/
  //                                                               from SDHCD START HERE
  QOSAL_UINT8         PendingIrqAcks;   /* mask of pending interrupts that have not been ACK'd */
  QOSAL_UINT8         IrqsEnabled;
  QOSAL_UINT8         IrqDetected;
  //                                                               from SDHCD END HERE
  QOSAL_UINT8     CpuInterrupt;
  QOSAL_UINT8                     CpuInterruptCause;

  QOSAL_UINT16        SpiIntEnableShadow;       /* shadow copy of interrupt enables */
  QOSAL_UINT16        SpiConfigShadow;          /* shadow copy of configuration register */
  QOSAL_UINT16        irqMask;
  QOSAL_UINT8         HostAccessDataWidth;      /* data width to use for host access */
  QOSAL_UINT8         DMADataWidth;             /* data width to use for DMA access */
  QOSAL_UINT32        ReadBufferSpace;          /* cached copy of read buffer available register */
  QOSAL_UINT32        WriteBufferSpace;           /* cached copy of space remaining in the SPI write buffer */
  QOSAL_UINT32        MaxWriteBufferSpace;        /* max write buffer space that the SPI interface supports */
  QOSAL_UINT32        PktsInSPIWriteBuffer;       /* number of packets in SPI write buffer so far */
  QOSAL_VOID          *pCurrentRequest;         /* pointer to a request in-progress used only for deferred SPI ops. */

  QOSAL_UINT32        OperationalClock;         /* spi module operational clock */
  QOSAL_UINT32        PowerUpDelay;             /* delay before the common layer should initialize over spi */
  /* SPI SPECIFICE ELEMENTS Starts here */
#define       HW_SPI_FRAME_WIDTH_8    0x01
#define       HW_SPI_FRAME_WIDTH_16   0x02
#define       HW_SPI_FRAME_WIDTH_24   0x04
#define       HW_SPI_FRAME_WIDTH_32   0x08
#define       HW_SPI_INT_EDGE_DETECT  0x80
#define       HW_SPI_NO_DMA           0x40
  QOSAL_UINT8         SpiHWCapabilitiesFlags;   /* SPI hardware capabilities flags */

#define       MISC_FLAG_SPI_SLEEP_WAR          0x04
#define       MISC_FLAG_RESET_SPI_IF_SHUTDOWN  0x02
#define       MISC_FLAG_DUMP_STATE_ON_SHUTDOWN 0x01
  QOSAL_UINT8       MiscFlags;
  A_MUTEX_T     lock;
}A_SPI_HCD_CXT;

typedef struct _a_driver_context{
  QOSAL_UINT16     driver_state;   /* maintains current state of driver one of DRIVER_STATE_... */
  QOSAL_UINT8      txQueryResult;
  QOSAL_BOOL       txQueryInProgress;
  QOSAL_BOOL           driver_up;     /* maintains driver status for external access */
  QOSAL_BOOL           chipDown;
  QOSAL_BOOL           rxBufferStatus;   /* maintains available status of rx buffers */
  QOSAL_BOOL       creditDeadlock;
  QOSAL_UINT32     creditDeadlockCounter;
  A_NETBUF_QUEUE_T txQueue;
  QOSAL_VOID           *pRxReq;
  A_ENDPOINT_T   ep[ENDPOINT_MANAGED_MAX];
  HTC_ENDPOINT_ID  ac2EpMapping[WMM_NUM_AC];
  QOSAL_UINT8          ep2AcMapping[ENDPOINT_MAX];
  QOSAL_UINT16     creditCount;   /* number of tx credits available at init */
  QOSAL_UINT16         creditSize;   //size in bytes of a credit
  /* the 5 version values maintained by the context */
  QOSAL_UINT32    targetType;   /* general type value */
  QOSAL_UINT32        targetVersion;   /* identifies the target chip */
  QOSAL_UINT32        wlanVersion;   /* identifies firmware running on target chip */
  QOSAL_UINT32        abiVersion;   /* identifies the interface compatible with wlan firmware */
  QOSAL_UINT32        hostVersion;   /* identifies version of host driver */

  QOSAL_UINT8         phyCapability;   /* identifies chip phy capability */
  QOSAL_BOOL          wmiReady;
  QOSAL_BOOL          wmiEnabled;
  QOSAL_VOID          *pWmiCxt;   /* context for wmi driver component */
  QOSAL_VOID          *pAggrCxt;   /* context for rx aggregate driver component */
  QOSAL_UINT16    txAggrTidMask;
  QOSAL_UINT16    rxAggrTidMask;

  QOSAL_INT8          numChannels;
  QOSAL_UINT16        channelList[MAX_NUM_CHANLIST_CHANNELS];

  QOSAL_UINT32        regCode;
  QOSAL_INT8          rssi;
  QOSAL_INT32         bitRate;
  QOSAL_BOOL          wmmEnabled;
  QOSAL_BOOL          tx_complete_pend;   /* tracks new tx completes for task sync */
  TEMP_FUNC_T   asynchRequest;
  /* STORE&RECALL SPECIFICE ELEMENTS Starts here */
  QOSAL_VOID        *strrclCxt;
  QOSAL_INT32     strrclCxtLen;
  QOSAL_BOOL          strrclBlock;
  QOSAL_UINT8         strrclState;
  /* STORE&RECALL SPECIFICE ELEMENTS Ends here */
  /* WPS SPECIFICE ELEMENTS Starts here */
  QOSAL_VOID          *wpsBuf;
  QOSAL_VOID          *wpsEvent;
  QOSAL_BOOL          wpsState;
  /* WPS SPECIFIC ELEMENTS Ends here */
  QOSAL_UINT8     userPwrMode;
  QOSAL_UINT8     tkipCountermeasures;
  /* CONNECT SPECIFIC ELEMENTS Start Here */
  QOSAL_UINT8         devId;
  A_CONNECTION_ELEMENTS_T conn[WLAN_NUM_OF_DEVICES];
  /* CONNECT SPECIFIC ELEMENTS End Here */
  /* from hif_device START HERE */
  QOSAL_UINT16     enabledSpiInts;
  QOSAL_UINT32     lookAhead;
  QOSAL_UINT32     mboxAddress;   /* the address for mailbox reads/writes. */
  QOSAL_UINT32     blockSize;   /* the mailbox block size */
  QOSAL_UINT32     blockMask;   /* the mask derived from BlockSize */
  QOSAL_UINT32     *padBuffer;
  QOSAL_UINT16    booleans;
  /* from hif_device ENDS HERE */
  /* SPI SPECIFICE ELEMENTS Starts here */
  A_SPI_HCD_CXT   spi_hcd;

  /* SPI SPECIFIC ELEMENTS Ends here */
  QOSAL_BOOL      scanDone;  /* scan done indicator, set by WMI_SCAN_COMPLETE_EVENTID */
  QOSAL_BOOL      driverSleep;
  QOSAL_BOOL          htcStart;
  QOSAL_UINT8     htc_creditInit;
  QOSAL_UINT8*        tempStorage;             /*temporary storage shared by independent modules*/
  QOSAL_UINT16        tempStorageLength;       /*size of temp storage allocated at init time, decided by feature set*/
  QOSAL_UINT8     promiscuous_mode;     /* if 1 promiscuous mode is enabled */
  QOSAL_BOOL      rssiFlag;

  QOSAL_UINT8         apmodeWPS;
#if ENABLE_P2P_MODE
  QOSAL_BOOL                  p2p_avail;
  QOSAL_UINT8                 wpsRole;
#endif
  QOSAL_BOOL          macProgramming;
#if MANUFACTURING_SUPPORT
  QOSAL_BOOL          testResp;
#endif
  QOSAL_UINT8         rxMultiBufferStatus;   /* maintains available status of rx buffers */
  QOSAL_BOOL      pfmDone;  /* scan done indicator, set by WMI_SCAN_COMPLETE_EVENTID */
  QOSAL_BOOL          wps_in_progress;
  QOSAL_UINT32        pwrStateSetting;
  QOSAL_BOOL      dset_op_done;          /* dset op done indicator, set by WMI_SCAN_COMPLETE_EVENTID */
  QOSAL_UINT8          raw_temperature;    /*register value */
  QOSAL_UINT32         tempDegree;    /*temperature degree*/
  QOSAL_BOOL          temperatureValid;
  QOSAL_UINT8    raw_countryCode[3];
  QOSAL_BOOL     countryCodeValid;
  QOSAL_BOOL    setparamValid;
  QOSAL_UINT32    setparamStatus;
  /* PORT_NOTE: rxFreeQueue stores pre-allocated rx net-buffers. If dynamic heap
   *  allocation is preferred for RX net-buffers then this option should be disabled. */
#if DRIVER_CONFIG_IMPLEMENT_RX_FREE_QUEUE
  A_NETBUF_QUEUE_T  rxFreeQueue;
#endif

  /* PORT_NOTE: pScanOut stores scan results in a buffer so that they
   *  can be retrieved aysnchronously by a User task. */
  QOSAL_UINT8*    pScanOut;   /* callers buffer to hold results. */
  QOSAL_UINT16    scanOutSize;  /* size of pScanOut buffer */
  QOSAL_UINT16    scanOutCount;  /* current fill count of pScanOut buffer */

  QOSAL_BOOL      driverShutdown;   /* used to shutdown the driver */
  QOSAL_UINT32    line;
  QOSAL_UINT8     securityType;   /* used to satisfy SEC_TYPE */
  QOSAL_UINT8     extended_scan;   /* used to indicate whether an extended scan is in progress */
#if ENABLE_P2P_MODE
  QOSAL_VOID          *p2pevtCB;
  QOSAL_BOOL          p2pEvtState;
  QOSAL_BOOL          p2pevtflag;
#endif
#if MANUFACTURING_SUPPORT
  QOSAL_UINT32        testCmdRespBufLen;
#endif
}A_DRIVER_CONTEXT;

extern QOSAL_UINT32 last_driver_error;

#define DEV_CALC_RECV_PADDED_LEN(pDCxt, length) (((length) + (pDCxt)->blockMask) & (~((pDCxt)->blockMask)))
#define DEV_CALC_SEND_PADDED_LEN(pDCxt, length) DEV_CALC_RECV_PADDED_LEN(pDCxt, length)
#define DEV_IS_LEN_BLOCK_ALIGNED(pDCxt, length) (((length) % (pDCxt)->blockSize) == 0)

#endif /* _DRIVER_CXT_H_ */
