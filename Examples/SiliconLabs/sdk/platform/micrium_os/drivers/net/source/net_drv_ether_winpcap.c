/***************************************************************************//**
 * @file
 * @brief Network Device Driver - Winpcap
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

/********************************************************************************************************
 ********************************************************************************************************
 *                                       DEPENDENCIES & AVAIL CHECK(S)
 ********************************************************************************************************
 *******************************************************************************************************/

#include  <rtos_description.h>

#if (defined(RTOS_MODULE_NET_IF_ETHER_AVAIL))

#if (!defined(RTOS_MODULE_NET_AVAIL))
#error Ethernet Driver requires Network Core module. Make sure it is part of your project \
  and that RTOS_MODULE_NET_AVAIL is defined in rtos_description.h.
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                               INCLUDE FILES
 ********************************************************************************************************
 *******************************************************************************************************/

#include  <drivers/net/include/net_drv_ether.h>

#include  <net/include/net_ascii.h>
#include  <net/include/net_util.h>
#include  <net/include/net_if_ether.h>
#include  <net/source/tcpip/net_priv.h>
#include  <net/source/tcpip/net_if_priv.h>
#include  <net/source/tcpip/net_if_802x_priv.h>
#include  <common/source/rtos/rtos_utils_priv.h>
#include  <common/source/logging/logging_priv.h>

#ifdef  _MSC_VER
#ifndef  WIN32
#define  WIN32
#endif // _MSC_VER
#endif // _MSC_VER

#define  WPCAP
#define _WS2TCPIP_H_
#define _WINSOCKAPI_
#define _WINSOCK2API_

#ifdef  _WIN32
#define    WINDOWS_LEAN_AND_MEAN
#include  <windows.h>
#include "winpcap/include/pcap.h"

#else                                                           // _WIN32
#include  <winpcap/include/pcap.h>

#endif // _WIN32

/********************************************************************************************************
 ********************************************************************************************************
 *                                               LOCAL DEFINES
 ********************************************************************************************************
 *******************************************************************************************************/

#define  LOG_DFLT_CH                        (NET)
#define  RTOS_MODULE_CUR                     RTOS_CFG_MODULE_NET

#define  INTERFACE_NUM_BUF_SIZE                            5u

#define  NET_DEV_FILTER_LEN                              117u
#define  NET_DEV_FILTER_QTY                               20u

#define PCAP_NETMASK_UNKNOWN    0xffffffff

#define NET_DEV_FILTER_PT1      "ether["
#define NET_DEV_FILTER_PT2      "] == 0x"
#define NET_DEV_FILTER_AND      " && "
#define NET_DEV_FILTER_OR       " || "

/********************************************************************************************************
 ********************************************************************************************************
 *                                           LOCAL DATA TYPES
 ********************************************************************************************************
 *******************************************************************************************************/
typedef struct net_dev_filter NET_DEV_FILTER;

struct net_dev_filter {
  NET_DEV_FILTER *p_next;
  CPU_INT08U     filter[6];
  CPU_INT16U     len;
};

//                                                                 --------------- DEVICE INSTANCE DATA ---------------
typedef struct net_dev_data_winpcap {
  KAL_TASK_HANDLE    RxTaskHandle;
  KAL_SEM_HANDLE     RxSignalHandle;
  pcap_t             *PcapHandlePtr;
  struct pcap_pkthdr *PcapPktHdrPtr;
  CPU_INT08U         *RxDataPtr;
  MEM_DYN_POOL       FilterPool;
  NET_DEV_FILTER     *p_filter_head;
} NET_DEV_DATA;

/********************************************************************************************************
 ********************************************************************************************************
 *                                       LOCAL FUNCTION PROTOTYPES
 *
 * Note(s) : (1) Device driver functions may be arbitrarily named.  However, it is recommended that device
 *               driver functions be named using the names provided below.  All driver function prototypes
 *               should be located within the driver C source file ('net_dev_&&&.c') & be declared as
 *               static functions to prevent name clashes with other network protocol suite device drivers.
 ********************************************************************************************************
 *******************************************************************************************************/

//                                                                 -------- FNCT'S COMMON TO ALL DEV'S --------
static void NetDev_Init(NET_IF   *p_if,
                        RTOS_ERR *p_err);

static void NetDev_Start(NET_IF   *p_if,
                         RTOS_ERR *p_err);

static void NetDev_Stop(NET_IF   *p_if,
                        RTOS_ERR *p_err);

static void NetDev_Rx(NET_IF     *p_if,
                      CPU_INT08U **p_data,
                      CPU_INT16U *size,
                      RTOS_ERR   *p_err);

static void NetDev_Tx(NET_IF     *p_if,
                      CPU_INT08U *p_data,
                      CPU_INT16U size,
                      RTOS_ERR   *p_err);

static void NetDev_AddrMulticastAdd(NET_IF     *p_if,
                                    CPU_INT08U *p_addr_hw,
                                    CPU_INT08U addr_hw_len,
                                    RTOS_ERR   *p_err);

static void NetDev_AddrMulticastRemove(NET_IF     *p_if,
                                       CPU_INT08U *p_addr_hw,
                                       CPU_INT08U addr_hw_len,
                                       RTOS_ERR   *p_err);

static void NetDev_IO_Ctrl(NET_IF     *p_if,
                           CPU_INT08U opt,
                           void       *p_data,
                           RTOS_ERR   *p_err);

static void NetDev_RxTask(void *p_data);

static void NetDev_PcapInit(NET_DEV_DATA *p_dev_data,
                            CPU_INT32U   pc_if_nbr,
                            RTOS_ERR     *p_err);

static void NetDev_WinPcap_UpdateFilter(NET_IF         *p_if,
                                        NET_DEV_FILTER *p_filter_head,
                                        RTOS_ERR       *p_err);

static void NetDev_WinPcap_RxAddFilter(NET_IF     *p_if,
                                       CPU_INT08U *p_filter_str,
                                       CPU_INT16U len,
                                       RTOS_ERR   *p_err);

static void NetDev_WinPcap_RxRemoveFilter(NET_IF     *p_if,
                                          CPU_INT08U *p_filter_str,
                                          CPU_INT16U len,
                                          RTOS_ERR   *p_err);

static void NetDev_WinPcap_ResetFilters(NET_IF   *p_if,
                                        RTOS_ERR *p_err);

/********************************************************************************************************
 ********************************************************************************************************
 *                                       NETWORK DEVICE DRIVER API
 *
 * Note(s) : (1) Device driver API structures are used by applications during calls to NetIF_Add().  This
 *               API structure allows higher layers to call specific device driver functions via function
 *               pointer instead of by name.  This enables the network protocol suite to compile & operate
 *               with multiple device drivers.
 *
 *           (2) In most cases, the API structure provided below SHOULD suffice for most device drivers
 *               exactly as is with the exception that the API structure's name which MUST be unique &
 *               SHOULD clearly identify the device being implemented.  For example, the Cirrus Logic
 *               CS8900A Ethernet controller's API structure should be named NetDev_API_CS8900A[].
 *
 *               The API structure MUST also be externally declared in the device driver header file
 *               ('net_dev_&&&.h') with the exact same name & type.
 ********************************************************************************************************
 *******************************************************************************************************/
//                                                                 WinPcap dev API fnct ptrs :
const NET_DEV_API_ETHER NetDev_API_WinPcap = { NetDev_Init,                // Init/add
                                               NetDev_Start,               // Start
                                               NetDev_Stop,                // Stop
                                               NetDev_Rx,                  // Rx
                                               NetDev_Tx,                  // Tx
                                               NetDev_AddrMulticastAdd,    // Multicast addr add
                                               NetDev_AddrMulticastRemove, // Multicast addr remove
                                               DEF_NULL,                   // ISR handler
                                               NetDev_IO_Ctrl,             // I/O ctrl
                                               DEF_NULL,                   // Phy reg rd
                                               DEF_NULL                    // Phy reg wr
};

/********************************************************************************************************
 ********************************************************************************************************
 *                                           LOCAL FUNCTIONS
 ********************************************************************************************************
 *******************************************************************************************************/

/****************************************************************************************************//**
 *                                               NetDev_Init()
 *
 * @brief    (1) Initialize Network Driver Layer :
 *
 *           - (a) Perform Driver Layer OS initialization
 *           - (b) Initialize driver status
 *           - (c) Initialize driver statistics & error counters
 *
 * @param    p_if    Pointer to the interface requiring service.
 *
 * @param    p_err   Pointer to return error code.
 *******************************************************************************************************/
static void NetDev_Init(NET_IF   *p_if,
                        RTOS_ERR *p_err)
{
  NET_DEV_DATA            *p_dev_data;
  NET_DEV_CFG_ETHER       *p_dev_cfg = (NET_DEV_CFG_ETHER *)p_if->Dev_Cfg;
  NET_DEV_CFG_EXT_WINPCAP *p_cfg_ext = (NET_DEV_CFG_EXT_WINPCAP *)p_dev_cfg->CfgExtPtr;
  CPU_SIZE_T              reqd_octets;
  CPU_SR_ALLOC();

  //                                                               --------------- VALIDATE DEVICE CFG ----------------
  //                                                               Validate Rx buf ix offset equal 0.
  RTOS_ASSERT_DBG_ERR_SET((p_dev_cfg->RxBufIxOffset == 0u), *p_err, RTOS_ERR_INVALID_CFG,; );
  RTOS_ASSERT_DBG_ERR_SET((p_dev_cfg->RxBufLargeSize >= NET_IF_ETHER_FRAME_MAX_SIZE), *p_err, RTOS_ERR_INVALID_CFG,; );
  RTOS_ASSERT_DBG_ERR_SET((p_dev_cfg->TxBufIxOffset == 0u), *p_err, RTOS_ERR_INVALID_CFG,; );

  //                                                               -------------- ALLOCATE DEV DATA AREA --------------

  p_dev_data = Mem_SegAllocExt("winPcap driver Data",
                               DEF_NULL,
                               sizeof(NET_DEV_DATA),
                               sizeof(CPU_ALIGN),
                               &reqd_octets,
                               p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    goto exit;
  }

  RTOS_ASSERT_CRITICAL_ERR_SET((p_dev_data != DEF_NULL), *p_err, RTOS_ERR_ASSERT_CRITICAL_FAIL,; );

  p_if->Dev_Data = p_dev_data;

  p_dev_data->RxSignalHandle = KAL_SemCreate("Pcap Rx completed signal", DEF_NULL, p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    goto exit;
  }

  p_dev_data->RxTaskHandle = KAL_TaskAlloc("Pcap Rx task", p_cfg_ext->TaskCfg.StkPtr, p_cfg_ext->TaskCfg.StkSizeElements, DEF_NULL, p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    goto exit;
  }

  CPU_CRITICAL_ENTER();
  NetDev_PcapInit(p_dev_data, p_cfg_ext->PC_IFNbr, p_err);
  CPU_CRITICAL_EXIT();

exit:
  return;
}

/****************************************************************************************************//**
 *                                               NetDev_Start()
 *
 * @brief    Start network interface hardware.
 *
 * @param    p_if    Pointer to a network interface.
 *
 * @param    p_err   Pointer to variable that will receive the return error code from this function.
 *
 * @note     (1) The physical HW address should not be configured from NetDev_Init(). Instead,
 *               it should be configured from within NetDev_Start() to allow for the proper use
 *               of NetIF_Ether_HW_AddrSet(), hard coded HW addresses from the device configuration
 *               structure, or auto-loading EEPROM's. Changes to the physical address only take
 *               effect when the device transitions from the DOWN to UP state.
 *
 * @note     (2) The device HW address is set from one of the data sources below. Each source is
 *               listed in the order of precedence.
 *               - (a) NetIF_Ether_HW_AddrSet()             Call NetIF_Ether_HW_AddrSet() if the HW
 *                                                          address needs to be configured via
 *                                                          run-time from a different data
 *                                                          source. E.g. Non auto-loading
 *                                                          memory such as I2C or SPI EEPROM.
 *                                                          (see Note #2).
 *               - (b) Device Configuration Structure       Configure a valid HW address in order
 *                                                          to hardcode the HW via compile time.
 *               - (c) Auto-Loading via EEPROM.             If neither options a) or b) are used,
 *                                                          the IF layers copy of the HW address
 *                                                          will be obtained from the network
 *                                                          hardware HW address registers.
 *
 * @note     (3) Setting the maximum number of frames queued for transmission is optional.  By
 *               default, all network interfaces are configured to block until the previous frame
 *               has completed transmission.  However, DMA based interfaces may have several
 *               frames configured for transmission before blocking is required. The number
 *               of queued transmit frames depends on the number of configured transmit
 *               descriptors.
 *******************************************************************************************************/
static void NetDev_Start(NET_IF   *p_if,
                         RTOS_ERR *p_err)
{
  NET_DEV_CFG_ETHER       *p_dev_cfg = (NET_DEV_CFG_ETHER *)p_if->Dev_Cfg;
  NET_DEV_CFG_EXT_WINPCAP *p_cfg_ext = (NET_DEV_CFG_EXT_WINPCAP *)p_dev_cfg->CfgExtPtr;
  NET_DEV_DATA            *p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;
  CPU_INT08U              hw_addr[NET_IF_ETHER_ADDR_SIZE];
  CPU_INT08U              hw_addr_len;
  CPU_BOOLEAN             hw_addr_cfg;
  RTOS_ERR                local_err;

  RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

  //                                                               ------------------- CFG HW ADDR --------------------
  hw_addr_cfg = DEF_NO;                                         // See Notes #1 & #2.

  //                                                               Get  app-configured IF layer HW MAC address, ...
  //                                                               ... if any (see Note #2a).
  hw_addr_len = sizeof(hw_addr);
  NetIF_AddrHW_GetHandler(p_if->Nbr, &hw_addr[0u], &hw_addr_len, &local_err);
  if (RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE) {
    hw_addr_cfg = NetIF_AddrHW_IsValidHandler(p_if->Nbr, &hw_addr[0u], &local_err);
  }

  if (hw_addr_cfg != DEF_YES) {                                 // Else get configured HW MAC address string, if any ...
                                                                // ... (see Note #4b).
    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);
    NetASCII_Str_to_MAC(p_dev_cfg->HW_AddrStr,                  // Check if configured HW MAC address format is valid.
                        &hw_addr[0u],
                        &local_err);
    if (RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE) {
      //                                                           Check if configured HW MAC address is valid.
      NetIF_AddrHW_SetHandler(p_if->Nbr,                        // return error if invalid.
                              &hw_addr[0u],
                              sizeof(hw_addr),
                              &local_err);
      if (RTOS_ERR_CODE_GET(local_err) != RTOS_ERR_NONE) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_CFG);
        return;
      }

      hw_addr_cfg = DEF_YES;                                    // If no errors, configure device    HW MAC address.
    } else {
      //                                                           Else attempt to get device's automatically loaded ...
      //                                                           ... HW MAC address, if any (see Note #4c).
      hw_addr[0] = 0x00;
      hw_addr[1] = 0x50;
      hw_addr[2] = 0xC2;
      hw_addr[3] = 0x25;
      hw_addr[4] = 0x60;
      hw_addr[5] = 0x02;

      NetIF_AddrHW_SetHandler(p_if->Nbr,                        // Configure IF layer to use automatically-loaded ...
                              &hw_addr[0u],                     // ... HW MAC address.
                              sizeof(hw_addr),
                              &local_err);
      if (RTOS_ERR_CODE_GET(local_err) != RTOS_ERR_NONE) {      // No valid HW MAC address configured, return error.
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_CFG);
        return;
      }
    }
  }

  if (hw_addr_cfg == DEF_YES) {                                 // If necessary, set device HW MAC address.
    ;                                                           // No device HW MAC address to configure.
  }

  //                                                               Add Unicast HW add to captude device.
  NetDev_WinPcap_RxAddFilter(p_if,
                             hw_addr,
                             sizeof(hw_addr),
                             p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    goto exit;
  }

  Mem_Set(hw_addr, 0xFF, sizeof(hw_addr));                      // Add Broadcast HW add to capture device.
  NetDev_WinPcap_RxAddFilter(p_if,
                             hw_addr,
                             sizeof(hw_addr),
                             p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    goto exit;
  }

  KAL_TaskCreate(p_dev_data->RxTaskHandle, NetDev_RxTask, p_if, p_cfg_ext->TaskCfg.Prio, DEF_NULL, p_err);
  RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(*p_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL,; )

  //                                                               ---------------- CFG TX RDY SIGNAL -----------------
  NetIF_DevCfgTxRdySignal(p_if,                                 // See Note #3.
                          p_dev_cfg->TxDescNbr);

exit:
  return;
}

/****************************************************************************************************//**
 *                                               NetDev_Stop()
 *
 * @brief    (1) Shutdown network interface hardware :
 *               - (a) Disable receive and transmit interrupts
 *               - (b) Disable the receiver and transmitter
 *               - (c) Clear pending interrupt requests
 *
 * @param    p_if    Pointer to the interface requiring service.
 *
 * @param    p_err   Pointer to return error code.
 *
 * @note     (2) It is recommended that a device driver should only post all currently-used,
 *               i.e. not-fully-transmitted, transmit buffers to the network interface transmit
 *               deallocation queue.
 *               - (a) However, a driver MAY attempt to post all queued &/or transmitted buffers.
 *                     The network interface transmit deallocation task will silently ignore any
 *                     unknown or duplicate transmit buffers.  This allows device drivers to
 *                     indiscriminately & easily post all transmit buffers without determining
 *                     which buffers have NOT yet been transmitted.
 *               - (b) Device drivers should assume that the network interface transmit deallocation
 *                     queue is large enough to post all currently-used transmit buffers.
 *               - (c) If the transmit deallocation queue is NOT large enough to post all transmit
 *                     buffers, some transmit buffers may/will be leaked/lost.
 *******************************************************************************************************/
static void NetDev_Stop(NET_IF   *p_if,
                        RTOS_ERR *p_err)
{
  NET_DEV_DATA *p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;

  NetDev_WinPcap_ResetFilters(p_if, p_err);

  KAL_TaskDel(p_dev_data->RxTaskHandle);

  KAL_SemSet(p_dev_data->RxSignalHandle, 0u, p_err);
  RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(*p_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL,; );
}

/****************************************************************************************************//**
 *                                               NetDev_Rx()
 *
 * @brief    Returns a pointer to the received data to the caller.
 *               - (1) Determine the descriptor that caused the interrupt.
 *               - (2) Obtain a pointer to a Network Buffer Data Area for storing new data.
 *               - (3) Reconfigure the descriptor with the pointer to the new data area.
 *               - (4) Pass a pointer to the received data area back to the caller via p_data;
 *               - (5) Clear interrupts
 *
 * @param    p_if    Pointer to the interface requiring service.
 *
 * @param    p_data  Pointer to pointer to received DMA data area. The recevied data
 *                   area address should be returned to the stack by dereferencing
 *                   p_data as *p_data = (address of receive data area).
 *
 * @param    size    Pointer to size. The number of bytes received should be returned
 *                   to the stack by dereferencing size as *size = (number of bytes).
 *
 * @param    p_err   Pointer to return error code.
 *******************************************************************************************************/
static void NetDev_Rx(NET_IF     *p_if,
                      CPU_INT08U **p_data,
                      CPU_INT16U *size,
                      RTOS_ERR   *p_err)
{
  NET_DEV_DATA       *p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;
  struct pcap_pkthdr *p_hdr;
  CPU_INT16U         len;
  CPU_INT08U         *p_buf;

  p_hdr = p_dev_data->PcapPktHdrPtr;
  len = p_hdr->len;
  //                                                               Verify frame len.
  if (len > NET_IF_ETHER_FRAME_MAX_SIZE) {
    RTOS_ERR err;

    *size = 0;
    *p_data = DEF_NULL;
    RTOS_ERR_SET(*p_err, RTOS_ERR_RX);
    KAL_SemPost(p_dev_data->RxSignalHandle, KAL_OPT_POST_NONE, &err);
    RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL,; );
    goto exit;
  }

  //                                                               Request a buffer
  p_buf = NetBuf_GetDataPtr(p_if,
                            NET_TRANSACTION_RX,
                            NET_IF_ETHER_FRAME_MAX_SIZE,
                            NET_IF_IX_RX,
                            DEF_NULL,
                            DEF_NULL,
                            DEF_NULL,
                            p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {             // If unable to get a buffer, discard the frame
    RTOS_ERR err;

    *size = 0;
    *p_data = DEF_NULL;
    KAL_SemPost(p_dev_data->RxSignalHandle, KAL_OPT_POST_NONE, &err);
    RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL,; );
    goto exit;
  }

  Mem_Copy(p_buf, p_dev_data->RxDataPtr, len);                  // Mem_Copy received data into new buffer.
  if (len < NET_IF_ETHER_FRAME_MIN_SIZE) {                      // See Note #1.
    len = NET_IF_ETHER_FRAME_MIN_SIZE;
  }

  *p_data = p_buf;
  *size = len;

  KAL_SemPost(p_dev_data->RxSignalHandle, KAL_OPT_POST_NONE, p_err);
  RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(*p_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL,; );

exit:
  return;
}

/****************************************************************************************************//**
 *                                               NetDev_Tx()
 *
 * @brief    Transmit the specified data.
 *               - (1) Check if the transmitter is ready.
 *               - (2) Configure the next transmit descriptor for pointer to data and data size.
 *               - (3) Issue the transmit command.
 *
 * @param    p_if    Pointer to the interface requiring service.
 *
 * @param    p_err   Pointer to return error code.
 *******************************************************************************************************/
static void NetDev_Tx(NET_IF     *p_if,
                      CPU_INT08U *p_data,
                      CPU_INT16U size,
                      RTOS_ERR   *p_err)
{
  NET_DEV_DATA *pdev_data = (NET_DEV_DATA *)p_if->Dev_Data;
  CPU_INT32U   tx;
  CPU_SR_ALLOC();

  CPU_CRITICAL_ENTER();
  tx = pcap_sendpacket(pdev_data->PcapHandlePtr, p_data, size);
  if (tx != 0u) {
    RTOS_ERR_SET(*p_err, RTOS_ERR_TX);
  }
  CPU_CRITICAL_EXIT();

  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    goto exit;
  }

  NetIF_TxDeallocQPost(p_data, p_err);
  if (RTOS_ERR_CODE_GET(*p_err) == RTOS_ERR_NONE) {
    NetIF_DevTxRdySignal(p_if);                                 // Signal Net IF that Tx resources are available
  }

exit:
  return;
}

/****************************************************************************************************//**
 *                                           NetDev_AddrMulticastAdd()
 *
 * @brief    Configure hardware address filtering to accept specified hardware address.
 *
 * @param    p_if        Pointer to an Ethernet network interface.
 *
 * @param    p_addr_hw   Pointer to hardware address.
 *
 * @param    addr_len    Length  of hardware address.
 *
 * @param    p_err       Pointer to variable that will receive the return error code from this function.
 *
 * @note     (1) The device is capable of the following multicast address filtering techniques :
 *               - (a) Promiscous non filtering.
 *
 * @note     (2) This function implements the filtering mechanism described in 1a.
 *******************************************************************************************************/
static void NetDev_AddrMulticastAdd(NET_IF     *p_if,
                                    CPU_INT08U *p_addr_hw,
                                    CPU_INT08U addr_hw_len,
                                    RTOS_ERR   *p_err)
{
  NetDev_WinPcap_RxAddFilter(p_if,
                             p_addr_hw,
                             addr_hw_len,
                             p_err);
}

/********************************************************************************************************
 *                                       NetDev_AddrMulticastRemove()
 *
 * @brief    Configure hardware address filtering to reject specified hardware address.
 *
 * @param    p_if        Pointer to an Ethernet network interface.
 *
 * @param    p_addr_hw   Pointer to hardware address.
 *
 * @param    addr_len    Length  of hardware address.
 *
 * @param    p_err       Pointer to variable that will receive the return error code from this function.
 *
 * @note     (1) The device is capable of the following multicast address filtering techniques :
 *               - (a) Promiscous non filtering.
 *
 * @note     (2) This function implements the filtering mechanism described in 1a.
 *******************************************************************************************************/
static void NetDev_AddrMulticastRemove(NET_IF     *p_if,
                                       CPU_INT08U *p_addr_hw,
                                       CPU_INT08U addr_hw_len,
                                       RTOS_ERR   *p_err)
{
  NetDev_WinPcap_RxRemoveFilter(p_if,
                                p_addr_hw,
                                addr_hw_len,
                                p_err);
}

/****************************************************************************************************//**
 *                                               NetDev_IO_Ctrl()
 *
 * @brief    Implement various hardware functions.
 *
 * @param    p_if    Pointer to interface requiring service.
 *
 * @param    opt     Option code representing desired function to perform. The Network Protocol Suite
 *                   specifies the option codes below. Additional option codes may be defined by the
 *                   driver developer in the driver's header file.
 *                   NET_DEV_LINK_STATE_GET
 *                   NET_DEV_LINK_STATE_UPDATE
 *                   Driver defined operation codes MUST be defined starting from 20 or higher
 *                   to prevent clashing with the pre-defined operation code types. See fec.h
 *                   for more details.
 *
 * @param    p_data  Pointer to optional data for either sending or receiving additional function
 *                   arguments or return data.
 *
 * @param    p_err   Pointer to return error code.
 *******************************************************************************************************/
static void NetDev_IO_Ctrl(NET_IF     *p_if,
                           CPU_INT08U opt,
                           void       *p_data,
                           RTOS_ERR   *p_err)
{
  NET_DEV_LINK_ETHER *p_link_state;

  PP_UNUSED_PARAM(p_if);                                        // Prevent 'variable unused' compiler warning.

  switch (opt) {
    case NET_IF_IO_CTRL_LINK_STATE_GET_INFO:
      p_link_state = (NET_DEV_LINK_ETHER *)p_data;
      RTOS_ASSERT_DBG_ERR_SET((p_link_state != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR,; );

      p_link_state->Duplex = NET_PHY_DUPLEX_FULL;
      p_link_state->Spd = NET_PHY_SPD_100;
      break;

    case NET_IF_IO_CTRL_LINK_STATE_UPDATE:
      break;

    default:
      RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_SUPPORTED);
      break;
  }
}

/****************************************************************************************************//**
 *                                               NetDev_RxTask()
 *
 * @brief    Driver Reception task.
 *
 * @param    p_data  Pointer to task argument.
 *******************************************************************************************************/
static void NetDev_RxTask(void *p_data)
{
  NET_IF       *p_if;
  NET_DEV_DATA *p_dev_data;
  CPU_INT16U   fail_cnt;
  CPU_INT08U   ret;
  CPU_SR_ALLOC();

  PP_UNUSED_PARAM(p_data);                                      // Prevent compiler warning.

  p_if = (NET_IF *)p_data;
  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;
  fail_cnt = 0u;

  while (DEF_TRUE) {
    //                                                             To optimize CPU usage, give up CPU for 10ms once ...
    //                                                             ... fail threshold has been reached.
    if (fail_cnt >= 1000) {
      KAL_Dly(1u);
      fail_cnt = 0u;
    }

    Net_GlobalLockAcquire(&NetDev_RxTask);

    //                                                             Retrieve packet.
    CPU_CRITICAL_ENTER();
    ret = pcap_next_ex(p_dev_data->PcapHandlePtr,
                       &p_dev_data->PcapPktHdrPtr,
                       (const u_char **)&p_dev_data->RxDataPtr);
    CPU_CRITICAL_EXIT();
    Net_GlobalLockRelease();

    if (ret == 1u) {                                            // Packet captured successfully.
      RTOS_ERR err;

      NetIF_RxQPost(p_if->Nbr, &err);                           // Signal Net Task of new frame
      if (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) {
        KAL_SemPend(p_dev_data->RxSignalHandle, KAL_OPT_PEND_NONE, 0, &err);
        RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL,; );
      }
    } else {
      fail_cnt++;
    }
  }
}

/****************************************************************************************************//**
 *                                               WinPcap_Init()
 *
 * @brief    (1) Initialize & start WinPcap :
 *               - (a) Retrieve the device list on the local machine.
 *               - (b) Print the list.
 *               - (c) Ask the user for a particular device to use.
 *               - (d) Open the device.
 *               - (e) Initialize filter pool for packet capture filtering.
 *
 * @param    if_ix       Index of the interface to be initialized.
 *
 * @param    p_handle    Winpcap handle.
 *
 * @param    p_err       Pointer to variable that will receive the return error code from this function.
 *
 * @note     (1) This number guarentees that whole packet is captured on all link layers.
 *******************************************************************************************************/
static void NetDev_PcapInit(NET_DEV_DATA *p_dev_data,
                            CPU_INT32U   pc_if_nbr,
                            RTOS_ERR     *p_err)
{
  CPU_INT08U interfaceNumStr[INTERFACE_NUM_BUF_SIZE];
  CPU_CHAR   *atoiRetVal;
  pcap_t     *p_adhandle;
  pcap_if_t  *alldevs;
  pcap_if_t  *d;
  CPU_INT08U errbuf[PCAP_ERRBUF_SIZE];
  CPU_INT08U inum;
  CPU_INT08U pc_if_ix;
  CPU_SIZE_T reqd_octets;

  p_dev_data->PcapHandlePtr = NULL;

  //                                                               Retrieve device list on the local machine.
  if (pcap_findalldevs(&alldevs, (char *)errbuf) == -1) {
    printf("\nError in pcap_findalldevs: %s\n", errbuf);
    //                                                             LOG_VRB(("Error in pcap_findalldevs: ", (s)errbuf));
    RTOS_ERR_SET(*p_err, RTOS_ERR_INIT);
    return;
  }

  //                                                               Print device list.
  pc_if_ix = 1u;
  for (d = alldevs; d != NULL; d = d->next) {
    if (pc_if_ix == 0u) {
      printf("\nPlease choose among the following adapter(s):\n\r");
      //                                                           OG_VRB(("Please choose among the following adapter(s):\n\r\n\r"));
    }
    //                                                             Open selected device.
    p_adhandle = pcap_open_live(d->name,                        // Name of the device.
                                65536u,                         // Portion of packet to capture (see Note #1).
                                1,                              // Promoscuous mode.
                                -1,                             // No read timeout.
                                (char *)errbuf);                // Error buffer.

    if (p_adhandle != (pcap_t *)NULL) {
      if (pcap_datalink(p_adhandle) == DLT_EN10MB) {
        pcap_addr_t *dev_addr;                                  // interface address that used by pcap_findalldevs()
        CPU_CHAR    str_ip[NET_ASCII_LEN_MAX_ADDR_IPv6];
        CPU_CHAR    str_mask[NET_ASCII_LEN_MAX_ADDR_IPv6];

        printf("%u. %s\n", pc_if_ix, d->name);
        //                                                         LOG_VRB(((u)pc_if_ix, " ", (s)d->name));

        pc_if_ix++;
        if (d->description) {
          printf("   (%s)\n\n", d->description);
          //                                                       LOG_VRB(("    ", (s)d->description));
        } else {
          printf("   (No description available)\n\n");
          //                                                       LOG_VRB(("    ", "No description available"));
        }

        //                                                         check if the device capturable
        for (dev_addr = d->addresses; dev_addr != NULL; dev_addr = dev_addr->next) {
          if (dev_addr->addr->sa_family == AF_INET && dev_addr->addr && dev_addr->netmask) {
            if (dev_addr->addr->sa_family == AF_INET) {
              NetASCII_IPv4_to_Str(NET_UTIL_NET_TO_HOST_32(((struct sockaddr_in *)dev_addr->addr)->sin_addr.s_addr),
                                   str_ip,
                                   DEF_NO,
                                   p_err);
              NetASCII_IPv4_to_Str(NET_UTIL_NET_TO_HOST_32(((struct sockaddr_in *)dev_addr->netmask)->sin_addr.s_addr),
                                   str_mask,
                                   DEF_NO,
                                   p_err);
            }
          } else if (dev_addr->addr->sa_family == AF_INET6 && dev_addr->addr && dev_addr->netmask) {
            NetASCII_IPv6_to_Str((NET_IPv6_ADDR *)&((struct sockaddr_in *)dev_addr->addr)->sin_addr.s_addr, str_ip, DEF_NO, DEF_NO, p_err);
            NetASCII_IPv6_to_Str((NET_IPv6_ADDR *)&((struct sockaddr_in *)dev_addr->netmask)->sin_addr.s_addr, str_mask, DEF_NO, DEF_NO, p_err);
          }

          printf("Device %s on address %s with netmask %s\n", d->name, str_ip, str_mask);
          //                                                       LOG_VRB(("Device ", (s)d->name, "on address ", (s)str_ip, "with netmask ", (s)str_mask));
        }
      }

      pcap_close(p_adhandle);
    }
  }

  if (pc_if_ix == 0u) {
    printf("\nNo interfaces found! Make sure WinPcap is installed.\n");
    //                                                             LOG_ERR(("No interfaces found! Make sure WinPcap is installed."));
    RTOS_ERR_SET(*p_err, RTOS_ERR_INIT);
    return;
  }

  if ((pc_if_nbr < 1u) || (pc_if_nbr > pc_if_ix)) {
    inum = 0u;
    while ((inum < 1u) || (inum > pc_if_ix)) {
      printf("\nEnter the interface number (1-%u):\n", pc_if_ix);
      //                                                           LOG_VRB(("Enter the interface number (1-", (u)pc_if_ix, ")"));

      atoiRetVal = fgets((CPU_CHAR *)&interfaceNumStr[0], INTERFACE_NUM_BUF_SIZE, stdin);
      if (atoiRetVal != NULL) {
        inum = atoi((const char *)interfaceNumStr);
      }

      if ((inum < 1u) || (inum > pc_if_ix)) {
        printf("\nInterface number out of range.");
        //                                                        LOG_ERR(("Interface number out of range"));
      }
    }
  } else {
    inum = pc_if_nbr;
  }
  printf("\n\n");

  //                                                               Point to selected adapter.
  pc_if_ix = 0u;
  d = alldevs;
  for (d = alldevs, pc_if_ix = 0u; pc_if_ix < (inum - 1u); pc_if_ix++) {
    d = d->next;
  }

  //                                                               Open selected device.
  p_adhandle = pcap_open_live(d->name,                          // Name of the device.
                              65536u,                           // Portion of packet to capture (see Note #1).
                              1,                                // Promiscuous mode.
                              -1,                               // No read timeout.
                              (char *)errbuf);                  // Error buffer.

  if (p_adhandle == NULL) {
    printf("Unable to open the adapter. %s is not supported by WinPcap.\n\n", d->name);
    //                                                             LOG_ERR(("Unable to open the adapter. ", (s)d->name, " is not supported by WinPcap."));
    pcap_freealldevs(alldevs);                                  // Free device list.
    RTOS_ERR_SET(*p_err, RTOS_ERR_INIT);
    return;
  }

#ifdef _WIN32
  //                                                               Set large kernel buffer size (10 MB).
  if (pcap_setbuff(p_adhandle, (10u * 1024u * 1024u)) == -1 ) {
    printf("Unable to change kernel buffer size.\n\n");
    //                                                             LOG_ERR(("Unable to change kernel buffer size."));
    pcap_freealldevs(alldevs);                                  // Free device list.
    pcap_close(p_adhandle);
    RTOS_ERR_SET(*p_err, RTOS_ERR_INIT);
    return;
  }
#endif

  pcap_freealldevs(alldevs);                                    // Free device list.

  //                                                               Init filter pool for packet capture (see Note #1e).
  Mem_DynPoolCreate("Filter_pool",
                    &p_dev_data->FilterPool,
                    DEF_NULL,
                    sizeof(NET_DEV_FILTER),
                    CPU_WORD_SIZE_32,
                    NET_DEV_FILTER_QTY,
                    NET_DEV_FILTER_QTY,
                    p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }

  p_dev_data->p_filter_head = NULL;
  p_dev_data->PcapHandlePtr = p_adhandle;
}

/****************************************************************************************************//**
 *                                       NetDev_WinPcap_RxAddFilter()
 *
 * @brief    Add an address to the packet capture filter.
 *
 * @param    if_nbr          Index of the interface on which to add the filter.
 *
 * @param    p_filter_addr   Pointer to an address to add to the packet capture filter.
 *
 * @param    len             Length of the filter address.
 *
 * @param    p_err           Pointer to variable that will receive the return error code from this function.
 *******************************************************************************************************/
static void NetDev_WinPcap_RxAddFilter(NET_IF     *p_if,
                                       CPU_INT08U *p_filter_addr,
                                       CPU_INT16U len,
                                       RTOS_ERR   *p_err)
{
  NET_DEV_DATA   *p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;
  NET_DEV_FILTER *p_filter;
  CPU_BOOLEAN    cmp;
  CPU_SR_ALLOC();

  //                                                               Verify if addr is not already present in filters.
  p_filter = p_dev_data->p_filter_head;
  cmp = DEF_NO;

  while (p_filter != NULL) {
    cmp |= Mem_Cmp(p_filter->filter, p_filter_addr, len);

    p_filter = p_filter->p_next;
  }

  if (cmp == DEF_YES) {                                         // If filter is already present, no need to add ...
    return;                                                     // ... it to the filter list.
  }

  p_filter = (NET_DEV_FILTER *)Mem_DynPoolBlkGet(&p_dev_data->FilterPool,
                                                 p_err);

  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }

  Mem_Copy(&p_filter->filter, p_filter_addr, len);

  p_filter->len = len;
  p_filter->p_next = NULL;

  CPU_CRITICAL_ENTER();
  //                                                               Insert filter at filter list head.
  p_filter->p_next = p_dev_data->p_filter_head;
  p_dev_data->p_filter_head = p_filter;
  CPU_CRITICAL_EXIT();

  //                                                               Update dev packet filter.
  NetDev_WinPcap_UpdateFilter(p_if,
                              p_dev_data->p_filter_head,
                              p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    goto exit;
  }

exit:
  return;
}

/****************************************************************************************************//**
 *                                       NetDev_WinPcap_RxRemoveFilter()
 *
 * @brief    Update the packet capture filter.
 *
 * @param    if_nbr          Index of the interface on which to update the filter.
 *
 * @param    p_filter_head   Pointer to the head of the address filter list.
 *
 * @param    p_err           Pointer to variable that will receive the return error code from this function.
 *
 * @note     (1) An Address Filter (AF) is in the following format:
 *               "ether[0] == 0xFF && ether[1] == 0xFF && ether[2] == 0xFF &&
 *               ether[3] == 0xFF && ether[4] == 0xFF && ether[5] == 0xFF"
 *               - (a) AFs are OR'd together to extend the filter capabilities:
 *                     (AF) || (AF) || ... || (AF)
 *               - (b) The resulting filter expression in then compiled and feed to Winpcap to perform the
 *                     desired filtering function.
 *******************************************************************************************************/
static void NetDev_WinPcap_RxRemoveFilter(NET_IF     *p_if,
                                          CPU_INT08U *p_filter_addr,
                                          CPU_INT16U len,
                                          RTOS_ERR   *p_err)
{
  NET_DEV_DATA   *p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;
  NET_DEV_FILTER *p_filter;
  NET_DEV_FILTER *p_filter_prev;
  CPU_BOOLEAN    cmp;
  CPU_SR_ALLOC();

  //                                                               Verify if addr is not already present in filters.
  p_filter = p_dev_data->p_filter_head;
  p_filter_prev = DEF_NULL;
  cmp = DEF_NO;

  while ((p_filter != NULL) && (cmp == DEF_NO)) {
    cmp = Mem_Cmp(p_filter->filter, p_filter_addr, len);

    if (cmp == DEF_NO) {
      p_filter_prev = p_filter;
      p_filter = p_filter->p_next;
    }
  }

  if (cmp == DEF_NO) {                                          // If filter is not in the filter list then ...
    RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_FOUND);                   // ... return err.
    goto exit;
  }

  CPU_CRITICAL_ENTER();                                         // Remove filter from the filter list.
  if (p_filter == p_dev_data->p_filter_head) {
    p_dev_data->p_filter_head = p_filter->p_next;
  } else {
    p_filter_prev->p_next = p_filter->p_next;
  }
  CPU_CRITICAL_EXIT();

  Mem_DynPoolBlkFree(&p_dev_data->FilterPool,                   // Free the filter memory block.
                     p_filter,
                     p_err);
  RTOS_ASSERT_CRITICAL(RTOS_ERR_CODE_GET(*p_err) == RTOS_ERR_NONE, RTOS_ERR_ASSERT_CRITICAL_FAIL,; );

  //                                                               Update dev packet filter.
  NetDev_WinPcap_UpdateFilter(p_if,
                              p_dev_data->p_filter_head,
                              p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    goto exit;
  }

exit:
  return;
}

/****************************************************************************************************//**
 *                                       NetDev_WinPcap_RxRemoveFilter()
 *
 * @brief    Update the packet capture filter.
 *
 * @param    if_nbr          Index of the interface on which to update the filter.
 *
 * @param    p_filter_head   Pointer to the head of the address filter list.
 *
 * @param    p_err           Pointer to variable that will receive the return error code from this function.
 *
 * @note     (1) An Address Filter (AF) is in the following format:
 *               "ether[0] == 0xFF && ether[1] == 0xFF && ether[2] == 0xFF &&
 *               ether[3] == 0xFF && ether[4] == 0xFF && ether[5] == 0xFF"
 * @note     (2) AFs are OR'd together to extend the filter capabilities:
 *               (AF) || (AF) || ... || (AF)
 * @note     (3) The resulting filter expression in then compiled and feed to Winpcap to perform the
 *               desired filtering function.
 *******************************************************************************************************/
static void NetDev_WinPcap_UpdateFilter(NET_IF         *p_if,
                                        NET_DEV_FILTER *p_filter_head,
                                        RTOS_ERR       *p_err)
{
  NET_DEV_DATA       *p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;
  CPU_INT08U         str[NET_DEV_FILTER_QTY * (NET_DEV_FILTER_LEN + 7)];
  CPU_CHAR           addr_str[6u * 3u];
  NET_DEV_FILTER     *p_filter;
  CPU_INT16U         ix;
  CPU_INT08U         *p_str;
  CPU_INT16U         str_ix;
  struct bpf_program prog;
  CPU_INT32S         result;

  str_ix = 0u;
  p_str = str;
  p_filter = p_filter_head;

  //                                                               Appending filter to the filter expression.
  while (p_filter != NULL) {
    NetASCII_MAC_to_Str(p_filter->filter, addr_str, DEF_NO, DEF_NO, p_err);

    //                                                             Append "(filter) || " to filter str.

    p_str[str_ix++] = '(';

    for (ix = 0; ix < p_filter->len; ix++) {                    // Writing "ether["               to buf.
      Mem_Copy(&p_str[str_ix], NET_DEV_FILTER_PT1, sizeof(NET_DEV_FILTER_PT1) - 1);
      str_ix += sizeof(NET_DEV_FILTER_PT1) - 1;

      p_str[str_ix++] = ix + '0';                               // Writing "ether[X"              to buf.

      //                                                           Writing "ether[X] == 0x"       to buf.
      Mem_Copy(&p_str[str_ix], NET_DEV_FILTER_PT2, sizeof(NET_DEV_FILTER_PT2) - 1);
      str_ix += sizeof(NET_DEV_FILTER_PT2) - 1;

      //                                                           Writing "ether[X] == 0xXX"     to buf.
      Mem_Copy(&p_str[str_ix], &addr_str[3 * ix], 2);
      str_ix += 2;

      //                                                           If the octet is not the last one of the addr, ...
      //                                                           ... it must me AND'd wiht the next octet.
      if (ix != (p_filter->len - 1)) {                          // Writing "ether[X] == 0xXX && " to buf.
        Mem_Copy(&p_str[str_ix], NET_DEV_FILTER_AND, sizeof(NET_DEV_FILTER_AND) - 1);
        str_ix += sizeof(NET_DEV_FILTER_AND) - 1;
      }
    }
    //                                                             Closing condition of this addr filter.
    p_str[str_ix++] = ')';

    //                                                             If the filter is not the last of the list, ...
    //                                                             ... it must me OR'd wiht the next filter.
    if (p_filter->p_next != NULL) {
      Mem_Copy(&p_str[str_ix], NET_DEV_FILTER_OR, sizeof(NET_DEV_FILTER_OR) - 1);
      str_ix += sizeof(NET_DEV_FILTER_OR) - 1;
    }

    p_filter = p_filter->p_next;
  }

  //                                                               Filter str must be null terminated.
  p_str[str_ix++] = '\0';

  //                                                               Compile the string into a winpcap filter.
  result = pcap_compile(p_dev_data->PcapHandlePtr, &prog, (char *)p_str, 1, PCAP_NETMASK_UNKNOWN);
  RTOS_ASSERT_CRITICAL((result >= 0), RTOS_ERR_ASSERT_CRITICAL_FAIL,; );

  //                                                               Set the filter to the capturing device.
  pcap_setfilter(p_dev_data->PcapHandlePtr, &prog);
  RTOS_ASSERT_CRITICAL((result >= 0), RTOS_ERR_ASSERT_CRITICAL_FAIL,; );
}

/****************************************************************************************************//**
 *                                       NetDev_WinPcap_ResetFilters()
 *
 * @brief    Removes all the filters from the packet capture filter.
 *
 * @param    if_nbr  Index of the interface on which to remove all the filters.
 *******************************************************************************************************/
static void NetDev_WinPcap_ResetFilters(NET_IF   *p_if,
                                        RTOS_ERR *p_err)
{
  NET_DEV_DATA   *p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;
  NET_DEV_FILTER *p_filter;

  p_filter = p_dev_data->p_filter_head;

  while (p_filter != NULL) {
    NetDev_WinPcap_RxRemoveFilter(p_if,
                                  p_filter->filter,
                                  p_filter->len,
                                  p_err);

    p_filter = p_filter->p_next;
  }
}

/********************************************************************************************************
 ********************************************************************************************************
 *                                       DEPENDENCIES & AVAIL CHECK(S)
 ********************************************************************************************************
 *******************************************************************************************************/

#endif // RTOS_MODULE_NET_IF_ETHER_AVAIL
