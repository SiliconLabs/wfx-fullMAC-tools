/***************************************************************************//**
 * @file
 * @brief Network Device Driver - Qca400X
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc.  Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement.
 * The software is governed by the sections of the MSLA applicable to Micrium
 * Software.
 *
 ******************************************************************************/

/****************************************************************************************************//**
 * @note     (1) The net_drv_wifi_qca400x module is essentially a port of the Qualcomm/Atheros driver.
 *               Please see the Qualcomm/Atheros copyright in the files in the "atheros_wifi" folder.
 *
 * @note     (2) Device driver compatible with these hardware:
 *               - (a) QCA4002
 *               - (b) QCA4004
 *******************************************************************************************************/

/********************************************************************************************************
 ********************************************************************************************************
 *                                               MODULE
 ********************************************************************************************************
 *******************************************************************************************************/

#ifndef  NET_DEV_QCA400X_PRIV_H
#define  NET_DEV_QCA400X_PRIV_H

#include  <net/include/net_cfg_net.h>

#ifdef  NET_IF_WIFI_MODULE_EN

/********************************************************************************************************
 ********************************************************************************************************
 *                                               INCLUDE FILES
 ********************************************************************************************************
 *******************************************************************************************************/

#include  <net/include/net_if_wifi.h>
#include  <net/source/tcpip/net_wifi_mgr_priv.h>
#include  <drivers/net/source/atheros_wifi/custom_src/include/a_types.h>

#include  <io/include/spi.h>
#include  <io/include/spi_slave.h>

/********************************************************************************************************
 ********************************************************************************************************
 *                                               DEFINES
 ********************************************************************************************************
 *******************************************************************************************************/

#define  NET_DEV_ATHEROS_PACKET_HEADER         20u
#define  NET_DEV_IO_CTRL_FW_VER_GET            127u

/********************************************************************************************************
 ********************************************************************************************************
 *                                               DATA TYPES
 ********************************************************************************************************
 *******************************************************************************************************/

typedef enum net_dev_state_scan {
  NET_DEV_STATE_SCAN_CMD_CONFIGURE = 1u,
  NET_DEV_STATE_SCAN_START = 2u,
  NET_DEV_STATE_SCAN_WAIT_RESP = 4u,
  NET_DEV_STATE_SCAN_WAIT_COMPLETE = 3u,
} NET_DEV_STATE_SCAN;

typedef enum net_dev_state_connect {
  NET_DEV_STATE_CONNECT_NONE = 1u,
  NET_DEV_STATE_CONNECT_802_11_WAITING = 2u,
  NET_DEV_STATE_CONNECT_802_11_AUTH = 3u,
  NET_DEV_STATE_CONNECT_WPA_WAITING = 4u,
  NET_DEV_STATE_CONNECT_WPA_AUTH = 5u,
  NET_DEV_STATE_CONNECT_WPA_FAIL = 6u,
} NET_DEV_STATE_CONNECT;

typedef struct net_dev_state {
  NET_DEV_STATE_CONNECT Connect;
  NET_DEV_STATE_SCAN    Scan;
  CPU_INT08U            ScanAPCount;
} NET_DEV_STATE;

typedef struct net_dev_peer_info NET_DEV_PEER_INFO;

struct net_dev_peer_info {
  NET_IF_WIFI_PEER  Info;
  NET_DEV_PEER_INFO *Next;
};

/********************************************************************************************************
 *                                           NET DEV DATA STRUCTURE
 * Note(s) : (1) This function has been inspired from the Api_RxComplete() in api_rxtx.c. It has been
 *               modified for a better integration and to respect the Micrum's coding standard.
 *
 *           (2) The following notes are comments from the original code.
 *
 *               (a) PORT_NOTE: utility_mutex is a mutex used to protect resources that might be accessed
 *                   by multiple task/threads in a system.It is used by the ACQUIRE/RELEASE macros below.
 *                   If the target system is single threaded then this mutex can be omitted and the macros
 *                   should be blank.
 *
 *               (b) PORT_NOTE: These 2 events are used to block & wake their respective tasks.
 *
 *               (c) PORT_NOTE: pCommonCxt stores the drivers common context. It is accessed by the
 *                   common source via GET_DRIVER_COMMON() below.
 *******************************************************************************************************/
typedef struct net_dev_data {
  CPU_INT08U        *GlobalBufPtr;

  CPU_INT08U        WaitRespType;

  CPU_BOOLEAN       LinkStatus;
  CPU_INT08U        LinkStatusQueryCtr;

  NET_DEV_STATE     State;

  MEM_DYN_POOL      PeerInfoPool;
  NET_DEV_PEER_INFO *PeerList;

  MEM_DYN_POOL      A_NetBufPool;

  KAL_Q_HANDLE      DriverRxQueue;

  CPU_INT08U        DeviceMacAddr[NET_IF_802x_HW_ADDR_LEN];
  CPU_INT32U        SoftVersion;
  //                                                               See note 2a.
  KAL_LOCK_HANDLE   UtilityMutex;
  //                                                               See note 2b.
  KAL_TASK_HANDLE   DriverTaskHandle;

  KAL_SEM_HANDLE    DriverStartSemHandle;
  KAL_SEM_HANDLE    GlobalBufSemHandle;

  KAL_SEM_HANDLE    driverWakeEvent;
  KAL_SEM_HANDLE    userWakeEvent;

  //                                                               See note 2c.
  QOSAL_VOID        *CommonCxt;                                 // the common driver context link.
  QOSAL_BOOL        DriverShutdown;                             // used to shutdown the driver.

  QOSAL_VOID        *connectStateCB;
  QOSAL_VOID        *promiscuous_cb;

  QOSAL_UINT8       *pScanOut;
  QOSAL_UINT16      scanOutSize;
  QOSAL_UINT16      scanOutCount;

  void              *AthSocketCxtPtr;
  void              *SocketCustomCxtPtr;
  QOSAL_VOID        *otaCB;
  QOSAL_VOID        *probeReqCB;
  QOSAL_VOID        *httpPostCb;
  QOSAL_VOID        *httpPostCbCxt;

  SPI_SLAVE_HANDLE  SPI_Handle;
} NET_DEV_DATA;

/********************************************************************************************************
 ********************************************************************************************************
 *                                               MODULE END
 ********************************************************************************************************
 *******************************************************************************************************/

#endif // NET_IF_WIFI_MODULE_EN
#endif // NET_DEV_QCA400X_PRIV_H
