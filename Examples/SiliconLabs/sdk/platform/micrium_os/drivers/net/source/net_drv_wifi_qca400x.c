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
 *                  - (a) QCA4002
 *                  - (b) QCA4004
 *******************************************************************************************************/

/********************************************************************************************************
 ********************************************************************************************************
 *                                       DEPENDENCIES & AVAIL CHECK(S)
 ********************************************************************************************************
 *******************************************************************************************************/

#include  <rtos_description.h>

#if (defined(RTOS_MODULE_NET_IF_WIFI_AVAIL))

#if (!defined(RTOS_MODULE_NET_AVAIL))
#error WiFi Driver requires Network Core module. Make sure it is part of your project \
  and that RTOS_MODULE_NET_AVAIL is defined in rtos_description.h.
#endif

#if (!defined(RTOS_MODULE_IO_SERIAL_SPI_AVAIL))
#error WiFi Driver requires IO Serial SPI module. Make sure it is part of your project \
  and that RTOS_MODULE_IO_SERIAL_SPI_AVAIL is defined in rtos_description.h.
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                               INCLUDE FILES
 ********************************************************************************************************
 *******************************************************************************************************/

#include  <net/include/net_if_wifi.h>
#include  <net/source/tcpip/net_if_priv.h>

#include  <io/include/spi.h>
#include  <io/include/spi_slave.h>

#include  <cpu/include/cpu.h>
#include  <common/include/rtos_prio.h>
#include  <common/include/rtos_err.h>

#include  "net_drv_wifi_qca400x_priv.h"
#include  <drivers/net/source/atheros_wifi/custom_src/include/cust_netbuf.h>
#include  <drivers/net/source/atheros_wifi/common_src/include/common_api.h>
#include  <drivers/net/source/atheros_wifi/custom_src/include/custom_api.h>
#include  <drivers/net/source/atheros_wifi/atheros_wifi_api.h>
#include  <drivers/net/source/atheros_wifi/common_src/include/wmi_api.h>
#include  <drivers/net/source/atheros_wifi/common_src/wmi/wmi_host.h>
#include  <drivers/net/source/atheros_wifi/include/htc.h>
#include  <drivers/net/source/atheros_wifi/custom_src/include/a_osapi.h>
#include  <drivers/net/source/atheros_wifi/include/wmi.h>

/********************************************************************************************************
 ********************************************************************************************************
 *                                               LOCAL DEFINES
 *
 * Note(s) : (1) Receive buffers MUST contain a prefix of at least one octet for the packet type
 *               and any other data that help the driver to demux the management frame.
 *
 *           (2) The driver MUST handle and implement generic commands defined in net_if_wifi.h. But the driver
 *               can also define and implement its own management command which need an response by calling
 *               the wireless manager api (p_mgr_api->Mgmt()) to send the management command and to receive
 *               the response.
 *
 *               (a) Driver management command code '100' series reserved for driver.
 ********************************************************************************************************
 *******************************************************************************************************/

#define  LOG_DFLT_CH                                       (NET)
#define  RTOS_MODULE_CUR                                   RTOS_CFG_MODULE_NET

#define  NET_DEV_DRIVER_START_EVENT                        "NetDev Driver Start Event"
#define  NET_DEV_DRIVER_RX_QUEUE                           "NetDev Driver Rx Queue"
#define  NET_DEV_WMI_BUF_SEM                               "NetDev WMI buffer sem"

//                                                                 ---------------- NET DEV CFG DEFINES -----------------
#define  NET_DEV_MGMT_NONE                                  0u
#define  NET_DEV_MGMT_DRIVER_INIT                           100u
#define  NET_DEV_MGMT_WPA_HANSHAKE                          101u;

#define  NET_DEV_RX_BUF_SPI_OFFSET_OCTETS                   64u

#define  NET_DEV_MGMT_RESP_COMMON_TIMEOUT_MS                10000u
#define  NET_DEV_MGMT_RESP_SCAN_TIMEOUT_MS                  10000u
#define  NET_DEV_MGMT_RESP_WPA_TIMEOUT_MS                   20000u

#define  NET_DEV_NB_A_NETBUF                                60u //8u
#define  NET_DEV_NB_WMI_BUF                                 1u

#define  NET_DEV_WIFI_WEP64_KEY_LEN                         5u
#define  NET_DEV_WIFI_WEP128_KEY_LEN                        13u

#define  NET_DEV_MAX_NB_PEER                                LIB_MEM_BLK_QTY_UNLIMITED

/********************************************************************************************************
 ********************************************************************************************************
 *                                           LOCAL DATA TYPES
 *
 * Note(s) : (1) Instance specific data area structures should be defined within this section.
 *               The data area structure typically includes error counters and variables used
 *               to track the state of the device.  Variables required for correct operation
 *               of the device MUST NOT be defined globally and should instead be included within
 *               the instance specific data area structure and referenced as pif->Dev_Data structure
 *               members.
 ********************************************************************************************************
 *******************************************************************************************************/

typedef struct net_dev_rx_hdr {
  NET_IF_WIFI_FRAME_TYPE type;
  CPU_INT16U             len;
} NET_DEV_RX_HDR;

/********************************************************************************************************
 ********************************************************************************************************
 *                                       LOCAL GLOBAL VARIABLES
 ********************************************************************************************************
 *******************************************************************************************************/

/********************************************************************************************************
 *                                               SPI INFO
 *******************************************************************************************************/

static const SPI_SLAVE_INFO NetWiFi_SPI_SlaveInfo_QCA400x = {
  .Mode = 0,
  .BitsPerFrame = 8,
  .LSbFirst = DEF_NO,
  .SClkFreqMax = 15000000u,
  .SlaveID = 0,
  .TxDummyByte = 0xFFu,
  .ActiveLow = DEF_YES
};

/********************************************************************************************************
 ********************************************************************************************************
 *                                       LOCAL FUNCTION PROTOTYPES
 *
 * Note(s) : (1) Device driver functions may be arbitrarily named.  However, it is recommended that device
 *               driver functions be named using the names provided below.  All driver function prototypes
 *               should be located within the driver C source file ('net_drv_&&&.c') & be declared as
 *               static functions to prevent name clashes with other network protocol suite device drivers.
 ********************************************************************************************************
 *******************************************************************************************************/
//                                                                 ------------ FNCT'S COMMON TO ALL DEV'S ------------

static void NetDev_Init(NET_IF   *p_if,
                        RTOS_ERR *p_err);

static void NetDev_Start(NET_IF   *p_if,
                         RTOS_ERR *p_err);

static void NetDev_Stop(NET_IF   *p_if,
                        RTOS_ERR *p_err);

static void NetDev_Rx(NET_IF     *p_if,
                      CPU_INT08U **p_data,
                      CPU_INT16U *p_size,
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

static void NetDev_ISR_Handler(NET_IF           *p_if,
                               NET_DEV_ISR_TYPE type);

static void NetDev_MgmtDemux(NET_IF   *p_if,
                             NET_BUF  *p_buf,
                             RTOS_ERR *p_err);

static CPU_INT32U NetDev_MgmtExecuteCmd(NET_IF           *p_if,
                                        NET_IF_WIFI_CMD  cmd,
                                        NET_WIFI_MGR_CTX *p_ctx,
                                        void             *p_cmd_data,
                                        CPU_INT16U       cmd_data_len,
                                        CPU_INT08U       *p_buf_rtn,
                                        CPU_INT08U       buf_rtn_len_max,
                                        RTOS_ERR         *p_err);

static CPU_INT32U NetDev_MgmtProcessResp(NET_IF           *p_if,
                                         NET_IF_WIFI_CMD  cmd,
                                         NET_WIFI_MGR_CTX *p_ctx,
                                         CPU_INT08U       *p_buf_rxd,
                                         CPU_INT16U       buf_rxd_len,
                                         CPU_INT08U       *p_buf_rtn,
                                         CPU_INT16U       buf_rtn_len_max,
                                         RTOS_ERR         *p_err);

//                                                                 ------------ FNCT'S SPECIFIC TO QCA400X ------------

static void NetDev_MgmtExecuteCmdScan(NET_IF           *p_if,
                                      NET_WIFI_MGR_CTX *p_ctx,
                                      NET_IF_WIFI_SCAN *p_scan,
                                      RTOS_ERR         *p_err);

static CPU_INT32U NetDev_MgmtProcessRespScan(NET_IF           *p_if,
                                             NET_WIFI_MGR_CTX *p_ctx,
                                             CPU_CHAR         *p_frame,
                                             CPU_INT16U       p_frame_len,
                                             CPU_INT08U       *p_ap_buf,
                                             CPU_INT16U       buf_len,
                                             RTOS_ERR         *p_err);

static void NetDev_MgmtExecuteCmdConnect(NET_IF             *p_if,
                                         NET_IF_WIFI_CMD    cmd,
                                         NET_IF_WIFI_AP_CFG *p_ap_cfg,
                                         RTOS_ERR           *p_err);

static A_NETBUF *NetDev_GetDriverTxANetBuf(NET_IF     *p_if,
                                           CPU_INT08U *p_data,
                                           CPU_INT16U size,
                                           RTOS_ERR   *p_err);

static A_STATUS NetDev_WMICmdSend(NET_IF         *p_if,
                                  WMI_COMMAND_ID cmd,
                                  void           *pParam,
                                  CPU_INT16U     length);

static CPU_INT16U NetDev_GetEventID(NET_IF     *p_if,
                                    CPU_INT08U *p_buf);

static void NetDev_RxFramePost(NET_IF                 *p_if,
                               A_NETBUF               *p_a_netbuf,
                               NET_IF_WIFI_FRAME_TYPE type,
                               RTOS_ERR               *p_err);

static void NetDev_PeerInfoAdd(NET_IF     *p_if,
                               CPU_INT08U *p_data,
                               RTOS_ERR   *p_err);

static void NetDev_PeerInfoDelete(NET_IF     *p_if,
                                  CPU_INT08U *p_data,
                                  RTOS_ERR   *p_err);

static CPU_INT16U NetDev_PeerInfoGet(NET_IF     *p_if,
                                     CPU_INT08U *p_data,
                                     CPU_INT16U buf_max_len,
                                     RTOS_ERR   *p_err);

static CPU_INT08U *NetDev_GetEventDataPtr(NET_IF     *p_if,
                                          CPU_INT08U *p_buf);

/********************************************************************************************************
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
 *               The API structure MUST also be referenced in a NET_IF_WIFI_PART_INFO structure that MUST
 *               be used for registering the device to the Platform Manager. See WIFI_SPI_REG in bsp.c
 *               and refer to Micrium's online documentation for more details.
 *******************************************************************************************************/
const NET_DEV_API_WIFI NetDev_API_QCA400X = {                   // Device driver API fnct ptrs :
  .Init = &NetDev_Init,
  .Start = &NetDev_Start,
  .Stop = &NetDev_Stop,
  .Rx = &NetDev_Rx,
  .Tx = &NetDev_Tx,
  .AddrMulticastAdd = &NetDev_AddrMulticastAdd,
  .AddrMulticastRemove = &NetDev_AddrMulticastRemove,
  .ISR_Handler = &NetDev_ISR_Handler,
  .MgmtDemux = &NetDev_MgmtDemux,
  .MgmtExecuteCmd = &NetDev_MgmtExecuteCmd,
  .MgmtProcessResp = &NetDev_MgmtProcessResp
};

const NET_IF_WIFI_PART_INFO NetWiFi_Info_QCA400x_AddOn = {
  .DrvAPI_Ptr = &NetDev_API_QCA400X,
  .RxBufIxOffset = 64u,
  .TxBufIxOffset = 64u,
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
 *               - (a) Allocate driver memory
 *               - (b) Initialize external interrupt controller
 *               - (c) Initialize external GPIO controller
 *               - (d) Initialize driver state variables
 *               - (e) Initialize driver statistics & error counters
 *               - (f) Create driver task.
 *
 * @param    p_if    Pointer to an wireless network interface.
 *
 * @param    p_err   Pointer to variable that will receive the return error code from this function.
 *
 * @note     (2) The application developer SHOULD define NetDev_CfgGPIO() within net_bsp.c
 *               in order to properly configure any necessary GPIO necessary for the device
 *               to operate properly.  Micrium recommends defining and calling this NetBSP
 *               function even if no additional GPIO initialization is required.
 *
 * @note     (3) The application developper SHOULD define NetDev_SPI_Init within net_bsp.c
 *               in order to properly configure SPI registers for the device to operate
 *               properly.
 *
 * @note     (4) The application developer SHOULD define NetDev_CfgIntCtrl() within net_bsp.c
 *               in order to properly enable interrupts on an external or CPU integrated
 *               interrupt controller.  Interrupt sources that are specific to the DEVICE
 *               hardware MUST NOT be initialized from within NetDev_CfgIntCtrl() and
 *               SHOULD only be modified from within the device driver.
 *               - (a) External interrupt sources are cleared within the NetBSP first level
 *                     ISR handler either before or after the call to the device driver ISR
 *                     handler function.  The device driver ISR handler function SHOULD only
 *                     clear the device specific interrupts and NOT external or CPU interrupt
 *                     controller interrupt sources.
 *
 * @note     (5) The application developer SHOULD define NetDev_IntCtrl() within net_bsp.c
 *               in order to properly enable  or disable interrupts on an external or CPU integrated
 *               interrupt controller.
 *
 * @note     (6) All functions that require device register access MUST obtain reference
 *               to the device hardware register space PRIOR to attempting to access
 *               any registers.  Register definitions SHOULD NOT be absolute and SHOULD
 *               use the provided base address within the device configuration structure,
 *               as well as the device register definition structure in order to properly
 *               resolve register addresses during run-time.
 *
 * @note     (7) All device drivers that store instance specific data MUST declare all
 *               instance specific variables within the device data area defined above.
 *
 * @note     (8) Drivers SHOULD validate device configuration values and set *p_err to
 *               RTOS_ERR_INVALID_CFG if unacceptible values have been specified. Fields
 *               of interest generally include, but are not limited to :
 *               - (a) pdev_cfg->RxBufPoolType :
 *                   - (1) NET_IF_MEM_TYPE_MAIN
 *                   - (2) NET_IF_MEM_TYPE_DEDICATED
 *               - (b) pdev_cfg->TxBufPoolType :
 *                   - (1) NET_IF_MEM_TYPE_MAIN
 *                   - (2) NET_IF_MEM_TYPE_DEDICATED
 *               - (c) pdev_cfg->RxBufAlignOctets
 *               - (d) pdev_cfg->TxBufAlignOctets
 *               - (e) pdev_cfg->DataBusSizeNbrBits
 *               - (f) pdev_cfg->SPI_ClkFreq
 *               - (g) pdev_cfg->SPI_ClkPol
 *                   - (1) NET_DEV_SPI_CLK_POL_INACTIVE_LOW
 *                   - (2) NET_DEV_SPI_CLK_POL_INACTIVE_HIGH
 *               - (h) pdev_cfg->SPI_ClkPhase
 *                   - (1) NET_DEV_SPI_CLK_PHASE_FALLING_EDGE
 *                   - (2) NET_DEV_SPI_CLK_PHASE_RASING_EDGE
 *               - (i) pdev_cfg->SPI_XferUnitLen
 *                   - (1) NET_DEV_SPI_XFER_UNIT_LEN_8_BITS
 *                   - (2) NET_DEV_SPI_XFER_UNIT_LEN_16_BITS
 *                   - (3) NET_DEV_SPI_XFER_UNIT_LEN_32_BITS
 *                   - (4) NET_DEV_SPI_XFER_UNIT_LEN_64_BITS
 *               - (j) pdev_cfg->SPI_XferShiftDir
 *                   - (1) NET_DEV_SPI_XFER_SHIFT_DIR_FIRST_MSB
 *                   - (2) NET_DEV_SPI_XFER_SHIFT_DIR_FIRST_LSB
 *
 * @note     (9) NetDev_Init() should exit with :
 *               - (a) All device interrupt source disabled. External interrupt controllers
 *                     should however be ready to accept interrupt requests.
 *               - (b) All device interrupt sources cleared.
 *******************************************************************************************************/
static void NetDev_Init(NET_IF   *p_if,
                        RTOS_ERR *p_err)
{
  NET_IF_WIFI_HW_INFO *p_hw_info;
  NET_DEV_BSP_WIFI    *p_dev_bsp;
  NET_DEV_CFG_WIFI    *p_dev_cfg;
  NET_DEV_CFG_QCA_EXT *p_dev_cfg_ext;
  NET_DEV_CFG_QCA_EXT dev_task_cfg;
  NET_DEV_DATA        *p_dev_data;
  NET_BUF_SIZE        buf_rx_size_max;
  CPU_SIZE_T          reqd_octets;
  CPU_SIZE_T          nbytes;
  CPU_INT16U          buf_size_max;
  A_STATUS            status;
  SPI_BUS_HANDLE      spi_bus_handle;

  //                                                               ---------- OBTAIN REFERENCE TO CFGs/BSP ------------
  p_dev_cfg = (NET_DEV_CFG_WIFI *)p_if->Dev_Cfg;
  p_dev_cfg_ext = (NET_DEV_CFG_QCA_EXT *)p_dev_cfg->CfgExtPtr;
  p_dev_bsp = (NET_DEV_BSP_WIFI *)p_if->Dev_BSP;
  p_hw_info = (NET_IF_WIFI_HW_INFO *)p_if->Ext_Cfg;
  //                                                               --------------- VALIDATE DEVICE CFG ----------------
  //                                                               Validate buf alignment.
  if (p_dev_cfg->RxBufAlignOctets != p_dev_cfg->TxBufAlignOctets) {
    RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_CFG);
    return;
  }

  if (p_dev_cfg_ext != DEF_NULL) {
    if (p_dev_cfg_ext->Prio == 0u) {
      RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_CFG);
      return;
    }

    if (p_dev_cfg_ext->StkSizeElements == 0u) {
      RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_CFG);
      return;
    }
  } else {
    dev_task_cfg.Prio = NET_CORE_IF_WIFI_TASK_PRIO_DFLT;
    dev_task_cfg.StkPtr = DEF_NULL;
    dev_task_cfg.StkSizeElements = 256u;

    p_dev_cfg_ext = &dev_task_cfg;
    p_dev_cfg->CfgExtPtr = (void *)p_dev_cfg_ext;
    p_if->Dev_Cfg = (void *)p_dev_cfg;
  }

  buf_rx_size_max = NetBuf_GetMaxSize(p_if->Nbr,
                                      NET_TRANSACTION_RX,
                                      DEF_NULL,
                                      NET_IF_IX_RX);
  if (buf_rx_size_max < NET_IF_802x_FRAME_MAX_SIZE) {
    RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_CFG);
    return;
  }

  //                                                               -------------- ALLOCATE DEV DATA AREA --------------
  p_if->Dev_Data = Mem_SegAllocExt("QCA Dev Data",
                                   DEF_NULL,
                                   sizeof(NET_DEV_DATA),
                                   sizeof(CPU_INT32U),
                                   &reqd_octets,
                                   p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }

  //                                                               --------- ALLOCATE GLOBAL BUF PTR DATA AREA --------
  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;
  buf_size_max = DEF_MAX(p_dev_cfg->RxBufLargeSize, p_dev_cfg->TxBufLargeSize);

  p_dev_data->GlobalBufPtr = Mem_SegAllocExt("Global Buffer",
                                             DEF_NULL,
                                             buf_size_max,
                                             p_dev_cfg->RxBufAlignOctets,
                                             &reqd_octets,
                                             p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }
  //                                                               ----- ALLOCATE ATHEROS COMMON DRIVER DATA AREA -----

  p_dev_data->CommonCxt = Mem_SegAllocExt("QCA Common Driver Data",
                                          DEF_NULL,
                                          sizeof(A_DRIVER_CONTEXT),
                                          sizeof(CPU_INT32U),
                                          &reqd_octets,
                                          p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }
  //                                                               -------------- ALLOCATE A_NETBUF POOL --------------

  //                                                               A_NETBUF memory pool create.
  nbytes = sizeof(A_NETBUF);                                    // Determine block size.
  Mem_DynPoolCreate("QCA A_NET_BUF",
                    &p_dev_data->A_NetBufPool,
                    0u,
                    nbytes,
                    sizeof(CPU_INT32U),
                    NET_DEV_NB_A_NETBUF,
                    NET_DEV_NB_A_NETBUF,
                    p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }
  //                                                               ------------- ALLOCCATE PEER_INFO LIST -------------
  Mem_DynPoolCreate("PeerInfo pool",
                    &p_dev_data->PeerInfoPool,
                    DEF_NULL,
                    sizeof(NET_DEV_PEER_INFO),
                    sizeof(CPU_ALIGN),
                    0u,
                    NET_DEV_MAX_NB_PEER,
                    p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }
  //                                                               -------------- CREATE DRIVER RX QUEUE --------------

  p_dev_data->DriverRxQueue = KAL_QCreate(NET_DEV_DRIVER_RX_QUEUE,
                                          p_dev_cfg->RxBufLargeNbr,
                                          DEF_NULL,
                                          p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }
  //                                                               Create Atheros driver start semaphore.
  p_dev_data->DriverStartSemHandle = KAL_SemCreate(NET_DEV_DRIVER_START_EVENT, DEF_NULL, p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }
  //                                                               Create WMI buffer access semaphore.
  p_dev_data->GlobalBufSemHandle = KAL_SemCreate(NET_DEV_WMI_BUF_SEM, DEF_NULL, p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }
  //                                                               Set the nb Global Device buffer access semaphore.
  KAL_SemSet(p_dev_data->GlobalBufSemHandle,
             NET_DEV_NB_WMI_BUF,
             p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }
  //                                                               ---------- ATHEROS COMMON INITIALIZATION -----------
  status = Api_InitStart((A_VOID *) p_if);
  if (status != A_OK) {
    RTOS_ERR_SET(*p_err, RTOS_ERR_INIT);
    return;
  }
  //                                                               ------- INITIALIZE EXTERNAL GPIO CONTROLLER --------
  p_dev_bsp->CfgGPIO(p_if, p_err);                              // Configure Wireless Controller GPIO (see Note #2).
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }

  //                                                               -------- GET SPI BUS FROM PLATFORM MANAGER  --------
  spi_bus_handle = SPI_BusHandleGetFromName((CPU_CHAR const *)p_hw_info->IO_BusNamePtr);
  if (spi_bus_handle == SPI_BusHandleNull) {
    RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
    return;
  }
  //                                                               ------------------- OPEN SPI BUS -------------------
  p_dev_data->SPI_Handle = SPI_SlaveOpen(spi_bus_handle,
                                         &NetWiFi_SPI_SlaveInfo_QCA400x,
                                         p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }
  //                                                               ----- INITIALIZE EXTERNAL INTERRUPT CONTROLLER -----
  p_dev_bsp->CfgIntCtrl(p_if, p_err);                           // Configure ext int ctrl'r (see Note #4).
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }
  //                                                               ------------ DISABLE EXTERNAL INTERRUPT ------------
  p_dev_bsp->IntCtrl(p_if, DEF_NO, p_err);                      // Disable ext int ctrl'r (See Note #5)
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }
  //                                                               ---------------- CREATE DRIVER TASK ----------------
  status = Custom_Driver_CreateThread((A_VOID *) p_if);
  if (status != A_OK) {
    RTOS_ERR_SET(*p_err, RTOS_ERR_INIT);
    return;
  }
  //                                                               ------------- INITIALIZE DEV DATA AREA -------------

  p_dev_data->State.Scan = NET_DEV_STATE_SCAN_CMD_CONFIGURE;
  p_dev_data->State.Connect = NET_DEV_STATE_CONNECT_NONE;
  p_dev_data->LinkStatus = NET_IF_LINK_DOWN;
  p_dev_data->LinkStatusQueryCtr = 0u;
  p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;

  Mem_Clr(p_dev_data->DeviceMacAddr, NET_IF_802x_ADDR_SIZE);
}

/****************************************************************************************************//**
 *                                               NetDev_Start()
 *
 * @brief    (1) Start network interface hardware :
 *               - (a) Start      Wireless device
 *               - (b) Initialize Wireless device
 *               - (c) Validate   MAC addr retrieved from the device
 *               - (d) Boot       Wireless firmware
 *               - (e) Initialize transmit semaphore count
 *
 * @param    p_if    Pointer to a network interface.
 *
 * @param    p_err   Pointer to variable that will receive the return error code from this function.
 *                       - NET_IF_ERR_NULL_PTR             Argument(s) passed a NULL pointer.
 *                       - NET_IF_ERR_NULL_FNCT            Invalid NULL function pointer.
 *                       - NET_IF_ERR_INVALID_CFG          Invalid/NULL API configuration.
 *                       - NET_IF_ERR_INVALID_STATE        Invalid network interface state.
 *                       - NET_IF_ERR_INVALID_ADDR         Invalid hardware address.
 *                       - NET_IF_ERR_INVALID_ADDR_LEN     Invalid hardware address length.
 *
 * @note     (2) Setting the maximum number of frames queued for transmission is optional.  By
 *               default, all network interfaces are configured to block until the previous frame
 *               has completed transmission.  However, some devices can queue multiple frames for
 *               transmission before blocking is required.  The default semaphore value is one.
 *
 * @note     (3) The physical hardware address should not be configured from NetDev_Init(). Instead,
 *               it should be configured from within NetDev_Start() to allow for the proper use
 *               of NetIF_802x_HW_AddrSet(), hard coded hardware addresses from the device
 *               configuration structure, or auto-loading EEPROM's. Changes to the physical address
 *               only take effect when the device transitions from the DOWN to UP state.
 *
 * @note     (4) The device hardware address is set from one of the data sources below. Each source
 *               is listed in the order of precedence.
 *
 *               - (a) Device Configuration Structure      Configure a valid HW address during
 *                                                         compile time.
 *
 *                                                         Configure either "00:00:00:00:00:00" or
 *                                                         an empty string, "", in order to
 *                                                         configure the HW address using using
 *                                                         method (b).
 *
 *               - (b) NetIF_802x_HW_AddrSet()             Call NetIF_802x_HW_AddrSet() if the HW
 *                                                         address needs to be configured via
 *                                                         run-time from a different data
 *                                                         source. E.g. Non auto-loading
 *                                                         memory such as I2C or SPI EEPROM
 *                                                         (see Note #3).
 *
 *               - (c) Auto-Loading via EEPROM             If neither options a) or b) are used,
 *                                                         the IF layer will use the HW address
 *                                                         obtained from the network hardware
 *                                                         address registers.
 *
 * @note     (5) More than one SPI device could share the same SPI controller, thus we MUST protect
 *               the access to the SPI controller and these step MUST be followed:
 *               - (a) Acquire SPI register lock.
 *                   - (1) If no other device share the same SPI controller, it is NOT required to
 *                         implement any type of ressource lock.
 *               - (b) Enable the Device Chip Select.
 *                   - (1) The Chip Select of the device SHOULD be disabled between each device access
 *               - (c) Set the SPI configuration of the device.
 *                   - (1) If no other device share the same SPI controller, SPI configuration can be
 *                         done during the SPI initialization.
 * @note     (6) The Qualcomm/Atheros driver don't allow to configure the MAC addr. The only method
 *               available is the one mentioned in 4b.
 *******************************************************************************************************/
static void NetDev_Start(NET_IF   *p_if,
                         RTOS_ERR *p_err)
{
  NET_WIFI_MGR_API *p_mgr_api;
  NET_DEV_CFG_WIFI *p_dev_cfg;
  NET_DEV_DATA     *p_dev_data;
  CPU_REG32        reg;
  CPU_BOOLEAN      valid;
  POWER_MODE       pwrmode;
  RTOS_ERR         local_err;

  RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

  //                                                               ------- OBTAIN REFERENCE TO CFG/DEV/MGR/BSP --------
  p_mgr_api = (NET_WIFI_MGR_API *)p_if->Ext_API;
  p_dev_cfg = (NET_DEV_CFG_WIFI *)p_if->Dev_Cfg;
  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;

  //                                                               -------------- DRIVER INITITIALIZATION -------------
  p_mgr_api->Mgmt(p_if,                                         // Start the driver task and initialize the driver.
                  NET_DEV_MGMT_DRIVER_INIT,
                  DEF_NULL,
                  0,
                  DEF_NULL,
                  0,
                  p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }

  Api_InitFinish(p_if);
  Api_WMIInitFinish(p_if);

  pwrmode.pwr_mode = MAX_PERF_POWER;
  pwrmode.pwr_module = PWR_MAX;

  Api_SetPowerMode(p_if, &pwrmode);
  //                                                               ------------ CONFIGURE HARDWARE ADDRESS ------------
  //                                                               The QCA400X doesn't allow to reprogram the MAC addr.
  //                                                               Validate only the MAC addr received on the READY...
  //                                                               ... event.

  valid = NetIF_AddrHW_IsValidHandler(p_if->Nbr, p_dev_data->DeviceMacAddr, &local_err);
  if (valid == DEF_YES) {
    NetIF_AddrHW_SetHandler(p_if->Nbr,                          // See Note #4b.
                            p_dev_data->DeviceMacAddr,
                            sizeof(p_dev_data->DeviceMacAddr),
                            p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
      return;
    }
  } else {
    RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
    return;
  }
  //                                                               ------- INITIALIZE TRANSMIT SEMAPHORE COUNT --------
  NetIF_DevCfgTxRdySignal(p_if, p_dev_cfg->TxBufLargeNbr);      // See Note #2.

  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }

  p_dev_data->LinkStatusQueryCtr = 0;
  p_dev_data->LinkStatus = NET_IF_LINK_DOWN;
}

/****************************************************************************************************//**
 *                                               NetDev_Stop()
 *
 * @brief    (1) Shutdown network interface hardware :
 *               - (a) Disable the receiver and transmitter
 *               - (b) Disable receive and transmit interrupts
 *
 * @param    p_if    Pointer to a network interface.
 *
 * @param    p_err   Pointer to variable that will receive the return error code from this function.
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
  NET_DEV_BSP_WIFI *p_dev_bsp;

  //                                                               ------------- OBTAIN REFERENCE TO BSP --------------
  p_dev_bsp = (NET_DEV_BSP_WIFI *)p_if->Dev_BSP;

  p_dev_bsp->Stop(p_if, p_err);                                 // Power down.
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }

  p_dev_bsp->IntCtrl(p_if, DEF_NO, p_err);                      // Disable ext int.
}

/****************************************************************************************************//**
 *                                               NetDev_Rx()
 *
 * @brief    (1) This function returns a pointer to the received data to the caller :
 *               - (a) Get the new data from the driver Rx Queue.
 *               - (b) Get the packet type and the size from the message header.
 *               - (c) Refresh the Rx Buffer pool status.
 *
 * @param    p_if    Pointer to a network interface.
 *
 * @param    p_data  Pointer to pointer to received data area. The received data
 *                   area address should be returned to the stack by dereferencing
 *                   p_data as *p_data = (address of receive data area).
 *
 * @param    p_size  Pointer to size. The number of bytes received should be returned
 *                   to the stack by dereferencing size as *size = (number of bytes).
 *
 * @param    p_err   Pointer to variable that will receive the return error code from this function.
 *
 *           none.
 *
 * @note     (1) If a receive error occurs, the function SHOULD return 0 for the size, a
 *               NULL pointer to the data area AND an error code equal to RTOS_ERR_RX.
 *               Some devices may require that driver instruct the hardware to drop the
 *               frame if it has been commited to internal device memory.
 *               - (a) If a new data area is unavailable, the driver MUST instruct hardware
 *                     to discard the frame.
 *
 * @note     (2) Reading data from the device hardware may occur in various sized reads :
 *               - (a) Device drivers that require read sizes equivalent to the size of the
 *                     device data bus MAY examine pdev_cfg->DataBusSizeNbrBits in order to
 *                     determine the number of required data reads.
 *               - (b) Devices drivers that require read sizes equivalent to the size of the
 *                     Rx FIFO width SHOULD use the known FIFO width to determine the number
 *                     of required data reads.
 *               - (c) It may be necessary to round the number of data reads up, OR perform the
 *                     last data read outside of the loop.
 *
 * @note     (3) A pointer set equal to pbuf_new and sized according to the required data
 *               read size determined in (2) should be used to read data from the device
 *               into the receive buffer.
 *
 * @note     (4) Some devices may interrupt only ONCE for a recieved frame.  The driver MAY need
 *               check if additional frames have been received while processing the current
 *               received frame.  If additional frames have been received, the driver MAY need
 *               to signal the receive task before exiting NetDev_Rx().
 *******************************************************************************************************/
static void NetDev_Rx(NET_IF     *p_if,
                      CPU_INT08U **p_data,
                      CPU_INT16U *p_size,
                      RTOS_ERR   *p_err)
{
  NET_DEV_DATA   *p_dev_data;
  NET_DEV_RX_HDR *p_hdr;

  //                                                               ----------- OBTAIN REFERENCE TO DEV DATA -----------
  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;

  //                                                               Get the next Rx buffer available.
  *p_data = KAL_QPend(p_dev_data->DriverRxQueue,
                      KAL_OPT_PEND_NON_BLOCKING,
                      KAL_TIMEOUT_INFINITE,
                      p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    *p_data = DEF_NULL;
    *p_size = 0u;
    return;
  }
  p_hdr = (NET_DEV_RX_HDR *)*p_data;
  *p_size = p_hdr->len;
  //                                                               Refresh the rx buffer pool status.
  Driver_ReportRxBuffStatus(p_if, TRUE);
}

/****************************************************************************************************//**
 *                                               NetDev_Tx()
 *
 * @brief    (1) This function transmits the specified data :
 *               - (a) Get a new request descriptor (A_NETBUF)
 *               - (b) Submit a TX request to the driver Task.
 *               - (c) Refresh the Rx Buffer pool status.
 *
 * @param    p_if    Pointer to a network interface.
 *
 * @param    p_data  Pointer to data to transmit.
 *
 * @param    size    Size of data to transmit.
 *
 * @param    p_err   Pointer to variable that will receive the return error code from this function.
 *
 * @note     (2) More than one SPI device could share the same SPI controller, thus we MUST protect
 *               the access to the SPI controller and these step MUST be followed:
 *               - (a) Acquire SPI register lock.
 *                   - (1) If no other device are shared the same SPI controller, it's not required to
 *                         implement any type of ressource lock.
 *               - (b) Enable the Device Chip Select.
 *                   - (1) The Chip Select of the device SHOULD be disbaled between each device access
 *               - (c) Set the SPI configuration of the device.
 *                   - (1) If no other device are shared the same SPI controller, SPI configuration can be
 *                         done during the SPI initialization.
 *
 * @note     (3) Software MUST track all transmit buffer addresses that are that are queued for
 *               transmission but have not received a transmit complete notification (interrupt).
 *               Once the frame has been transmitted, software must post the buffer address of
 *               the frame that has completed transmission to the transmit deallocation task.
 *               - (a) If the device doesn't support transmit complete notification software must
 *                     post the buffer address of the frame once the data is wrote on the device FIFO.
 *
 * @note     (4) Writing data to the device hardware may occur in various sized writes :
 *               - (a) Devices drivers that require write sizes equivalent to the size of the
 *                     Tx FIFO width SHOULD use the known FIFO width to determine the number
 *                     of required data reads.
 *               - (b) It may be necessary to round the number of data writes up, OR perform the
 *                     last data write outside of the loop.
 *******************************************************************************************************/
static void NetDev_Tx(NET_IF     *p_if,
                      CPU_INT08U *p_data,
                      CPU_INT16U size,
                      RTOS_ERR   *p_err)
{
  A_NETBUF *p_a_netbuf;
  A_STATUS status;

  //                                                               Get and Set the A_NETBUF before submitting the...
  //                                                               ...packet to the driver task.
  p_a_netbuf = NetDev_GetDriverTxANetBuf(p_if,
                                         p_data,
                                         size,
                                         p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }
  //                                                               Submit the TX packet to the driver task.

  status = Api_DataTxStart(p_if, p_a_netbuf);
  if (status != A_OK ) {
    RTOS_ERR_SET(*p_err, RTOS_ERR_TX);
    return;
  }
  //                                                               Refresh the rx buffer pool status.
  Driver_ReportRxBuffStatus(p_if, TRUE);
}

/****************************************************************************************************//**
 *                                           NetDev_AddrMulticastAdd()
 *
 * @brief    Configure hardware address filtering to accept specified hardware address.
 *
 * @param    p_if        Pointer to a network interface.
 *
 * @param    paddr_hw    Pointer to hardware address.
 *
 * @param    addr_len    Length  of hardware address.
 *
 * @param    p_err       Pointer to variable that will receive the return error code from this function.
 *                           - NET_UTIL_ERR_NULL_PTR           Argument 'pdata_buf' passed NULL pointer.
 *                           - NET_UTIL_ERR_NULL_SIZE          Argument 'len' passed equal to 0.
 *******************************************************************************************************/
static void NetDev_AddrMulticastAdd(NET_IF     *p_if,
                                    CPU_INT08U *p_addr_hw,
                                    CPU_INT08U addr_hw_len,
                                    RTOS_ERR   *p_err)
{
  PP_UNUSED_PARAM(p_if);
  PP_UNUSED_PARAM(p_addr_hw);
  PP_UNUSED_PARAM(addr_hw_len);
  PP_UNUSED_PARAM(p_err);
}

/****************************************************************************************************//**
 *                                       NetDev_AddrMulticastRemove()
 *
 * @brief    Configure hardware address filtering to reject specified hardware address.
 *
 * @param    p_if        Pointer to a network interface.
 *
 * @param    p_addr_hw   Pointer to hardware address.
 *
 * @param    addr_len    Length  of hardware address.
 *
 * @param    p_err       Pointer to variable that will receive the return error code from this function.
 *                           - NET_UTIL_ERR_NULL_PTR           Argument 'pdata_buf' passed NULL pointer.
 *                           - NET_UTIL_ERR_NULL_SIZE          Argument 'len' passed equal to 0.
 *******************************************************************************************************/
static void NetDev_AddrMulticastRemove(NET_IF     *p_if,
                                       CPU_INT08U *p_addr_hw,
                                       CPU_INT08U addr_hw_len,
                                       RTOS_ERR   *p_err)
{
  PP_UNUSED_PARAM(p_if);
  PP_UNUSED_PARAM(p_addr_hw);
  PP_UNUSED_PARAM(addr_hw_len);
  PP_UNUSED_PARAM(p_err);
}

/****************************************************************************************************//**
 *                                           NetDev_ISR_Handler()
 *
 * @brief    This function serves as the device Interrupt Service Routine Handler. This ISR
 *           handler MUST service and clear all necessary and enabled interrupt events for
 *           the device.
 *
 * @param    p_if    Pointer to a network interface.
 *
 * @param    type    Network Interface defined argument representing the type of ISR in progress. Codes
 *                   for Rx, Tx, Overrun, Jabber, etc... are defined within net_if.h and are passed
 *                   into this function by the corresponding Net BSP ISR handler function. The Net
 *                   BSP ISR handler function may be called by a specific ISR vector and therefore
 *                   know which ISR type code to pass.  Otherwise, the Net BSP may pass
 *                   NET_DEV_ISR_TYPE_UNKNOWN and the device driver MAY ignore the parameter when
 *                   the ISR type can be deduced by reading an available interrupt status register.
 *                   Type codes that are defined within net_if.c include but are not limited to :
 *                       - NET_DEV_ISR_TYPE_RX
 *                       - NET_DEV_ISR_TYPE_TX_COMPLETE
 *                       - NET_DEV_ISR_TYPE_UNKNOWN
 *
 * @note     (1) This function is called via function pointer from the context of an ISR.
 *
 * @note     (2) In the case of an interrupt occurring prior to Network Protocol Stack initialization,
 *               the device driver should ensure that the interrupt source is cleared in order
 *               to prevent the potential for an infinite interrupt loop during system initialization.
 *
 * @note     (3) Many devices generate only one interrupt event for several ready frames.
 *               - (a) It is NOT recommended to read from the SPI controller in the ISR handler as the SPI
 *                     can be shared with another peripheral. If the SPI lock has been acquired by another
 *                     application/chip the entire application could be locked forever.
 *               - (b) If the device support the transmit completed notification and it is NOT possible to know
 *                     the interrupt type without reading on the device, we suggest to notify the receive task
 *                     where the interrupt register is read. And in this case, NetDev_rx() should return a
 *                     management frame, which will be send automaticcly to NetDev_Demux() and where the stack
 *                     is called to dealloc the packet transmitted.
 *******************************************************************************************************/
static void NetDev_ISR_Handler(NET_IF           *p_if,
                               NET_DEV_ISR_TYPE type)
{
  HW_InterruptHandler(p_if);
  PP_UNUSED_PARAM(type);                                        // Prevent possible 'variable unused' warnings.
}

/****************************************************************************************************//**
 *                                           NetDev_MgmtDemux()
 *
 * @brief    (1) This function should analyse the management frame received and apply some operations on the device or
 *               call the stack to change the link state or signal the wireless manager when an response is
 *               received which has been requested by a previous management command sent.
 *
 * @param    p_if    Pointer to a network interface.
 *
 * @param    p_buf   Pointer to a network buffer that received a management frame.
 *
 * @param    p_err   Pointer to variable that will receive the return error code from this function.
 *
 *           none.
 *
 * @note     (2) The network buffer MUST be freed by this functions when the buffer is only used by this function.
 *               - (a) The network buffer MUST NOT be freed by this function when an error occured and is returned,
 *                     the upper layer free the buffer and increment discarded management frame global counter.
 *               - (b) The netowrk buffer MUST NOT be freed by this function when the wireless manager is signaled as
 *                     the network buffer will be used and freed by the wireless manager.
 *
 * @note     (3) The wireless manager MUST be signaled only when the response received is for a management
 *               command previously sent.
 *******************************************************************************************************/
static void NetDev_MgmtDemux(NET_IF   *p_if,
                             NET_BUF  *p_buf,
                             RTOS_ERR *p_err)
{
  NET_DEV_DATA         *p_dev_data;
  NET_WIFI_MGR_API     *p_mgr_api;
  CPU_BOOLEAN          signal;
  CPU_BOOLEAN          valid;
  A_UINT16             id;
  NET_WIFI_MGR_DATA    *p_mgr_data;
  WMI_DISCONNECT_EVENT *p_wmi_disconnect;
  NET_DEV_CFG_WIFI     *p_dev_cfg;

  //                                                               --------- OBTAIN REFERENCE TO DEV/CFG/MGR ----------
  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;
  p_mgr_data = (NET_WIFI_MGR_DATA *)p_if->Ext_Data;
  p_dev_cfg = (NET_DEV_CFG_WIFI *)p_if->Dev_Cfg;

  //                                                               ------------------ DEMUX EVENT ID ------------------
  id = NetDev_GetEventID(p_if, p_buf->DataPtr);
  signal = DEF_NO;

  switch (id) {
    case WMI_READY_EVENTID:
      if (p_dev_data->WaitRespType == NET_DEV_MGMT_DRIVER_INIT) {
        signal = DEF_YES;
      }
      break;

    case WMI_CONNECT_EVENTID:
      if (p_dev_data->WaitRespType == NET_IF_WIFI_CMD_JOIN ) {
        if (p_dev_data->State.Connect == NET_DEV_STATE_CONNECT_802_11_WAITING) {
          signal = DEF_YES;
        }
      } else if (p_dev_data->WaitRespType == NET_IF_WIFI_CMD_CREATE) {
        signal = DEF_YES;
      } else {
        if (p_mgr_data->AP_Created == DEF_YES) {
          //                                                       Peer association event.
          NetDev_PeerInfoAdd(p_if,
                             p_buf->DataPtr,
                             p_err);
        }
        signal = DEF_NO;
      }
      break;

    case WMI_DISCONNECT_EVENTID:
      p_wmi_disconnect = (WMI_DISCONNECT_EVENT *)(p_buf->DataPtr + p_dev_cfg->RxBufIxOffset + HTC_HDR_LENGTH + sizeof(WMI_CMD_HDR) - NET_DEV_ATHEROS_PACKET_HEADER);
      if (p_dev_data->WaitRespType == NET_IF_WIFI_CMD_JOIN ) {
        if ((p_wmi_disconnect->disconnectReason != DISCONNECT_CMD)
            || (p_dev_data->State.Connect == NET_DEV_STATE_CONNECT_WPA_FAIL)) {
          signal = DEF_YES;
        }
      } else if (p_dev_data->WaitRespType == NET_IF_WIFI_CMD_LEAVE) {
        if (p_wmi_disconnect->disconnectReason == DISCONNECT_CMD) {
          signal = DEF_YES;
        }
      } else {
        p_wmi_disconnect = (WMI_DISCONNECT_EVENT *)(p_buf->DataPtr + p_dev_cfg->RxBufIxOffset + HTC_HDR_LENGTH + sizeof(WMI_CMD_HDR) - NET_DEV_ATHEROS_PACKET_HEADER);

        valid = NetIF_AddrHW_IsValidHandler(p_if->Nbr, p_wmi_disconnect->bssid, p_err);

        if (valid == DEF_YES) {
          if (p_mgr_data->AP_Created == DEF_YES) {
            //                                                     Peer disassociation event.
            NetDev_PeerInfoDelete(p_if,
                                  p_buf->DataPtr,
                                  p_err);
          } else {
            p_if->Link = NET_IF_LINK_DOWN;
          }
        } else {
          p_if->Link = NET_IF_LINK_DOWN;                        // Disassociation event
        }
        signal = DEF_NO;
      }
      break;

    case WMI_BSSINFO_EVENTID:
    case WMI_SCAN_COMPLETE_EVENTID:
      if (p_dev_data->WaitRespType == NET_IF_WIFI_CMD_SCAN) {
        signal = DEF_YES;
      }
      break;

    case WMI_REPORT_STATISTICS_EVENTID:
      if ((p_dev_data->WaitRespType == NET_IF_IO_CTRL_LINK_STATE_GET)
          || (p_dev_data->WaitRespType == NET_IF_IO_CTRL_LINK_STATE_GET_INFO) ) {
        signal = DEF_YES;
      }
      break;

    case WMI_PEER_NODE_EVENTID:
      if (p_dev_data->WaitRespType == NET_IF_WIFI_CMD_JOIN) {
        if (p_dev_data->State.Connect == NET_DEV_STATE_CONNECT_WPA_WAITING) {
          signal = DEF_YES;
        }
      }
      break;
    //                                                             Unsupported event id.

    case WMI_GET_PMK_EVENTID:
    case WMI_CMDERROR_EVENTID:
    case WMI_ERROR_REPORT_EVENTID:
    case WMI_SET_CHANNEL_EVENTID:
    case WMI_REGDOMAIN_EVENTID:
    case WMI_GET_CHANNEL_LIST_CMDID:
    case WMI_GET_BITRATE_CMDID:
    case WMI_STORERECALL_STORE_EVENTID:
    case WMI_WPS_PROFILE_EVENTID:
    case WMI_ADDBA_REQ_EVENTID:
    case WMI_DELBA_REQ_EVENTID:
    case WMI_TKIP_MICERR_EVENTID:
    case WMI_SOCKET_RESPONSE_EVENTID:
    case WMI_P2P_GO_NEG_RESULT_EVENTID:
    case WMI_P2P_NODE_LIST_EVENTID:
    case WMI_P2P_REQ_TO_AUTH_EVENTID:
    case WMI_P2P_PROV_DISC_RESP_EVENTID:
    case WMI_P2P_PROV_DISC_REQ_EVENTID:
    case WMI_P2P_INVITE_REQ_EVENTID:
    case WMI_P2P_INVITE_RCVD_RESULT_EVENTID:
    case WMI_P2P_INVITE_SENT_RESULT_EVENTID:
    case WMI_P2P_SDPD_RX_EVENTID:
    case WMI_TEST_EVENTID:
    default:
      signal = DEF_NO;
      break;
  }

  if (signal == DEF_YES) {
    p_mgr_api = (NET_WIFI_MGR_API *)p_if->Ext_API;
    p_mgr_api->Signal(p_if, p_buf, p_err);
  } else {
    //                                                             Free the buffer if not transferred to the wifi mgr.
    NetBuf_Free(p_buf);
  }
}

/****************************************************************************************************//**
 *                                           NetDev_MgmtExecuteCmd()
 *
 * @brief    This function MUST initializes or continues a management command.
 *
 * @param    p_if                Pointer to a network interface.
 *
 * @param    cmd                 Management command to be executed:
 *                                   - NET_IF_WIFI_CMD_SCAN
 *                                   - NET_IF_WIFI_CMD_JOIN
 *                                   - NET_IF_WIFI_CMD_LEAVE
 *                                   - NET_IF_IO_CTRL_LINK_STATE_GET
 *                                   - NET_IF_IO_CTRL_LINK_STATE_GET_INFO
 *                                   - NET_IF_IO_CTRL_LINK_STATE_UPDATE
 *                               Others management commands defined by this driver.
 *
 * @param    p_ctx               State machine context See Note #1.
 *
 * @param    p_cmd_data          Pointer to a buffer that contains data to be used by the driver to execute
 *                               the command
 *
 * @param    cmd_data_len        Command data length.
 *
 * @param    p_buf_rtn           Pointer to buffer that will receive return data.
 *
 * @param    buf_rtn_len_max     Return data length max.
 *
 * @param    p_err               Pointer to variable that will receive the return error code from this function
 *                                   - RTOS_ERR_NONE            Management     successfully executed
 *                                   - RTOS_ERR_IO              Management NOT successfully executed
 *
 * @return   length of data wrote in the return buffer in octet.
 *
 * @note     (1) The state machine context is used by the Wifi manager to know what it MUST do after this call:
 *           - (a) WaitResp is used by the wireless manager to know if an asynchronous response is
 *                 required.
 *           - (b) WaitRespTimeout_ms is used by the wireless manager to know what is the timeout to receive
 *                 the response.
 *           - (c) MgmtCompleted is used by the wireless manager to know if the management process is
 *                 completed.
 *******************************************************************************************************/
static CPU_INT32U NetDev_MgmtExecuteCmd(NET_IF           *p_if,
                                        NET_IF_WIFI_CMD  cmd,
                                        NET_WIFI_MGR_CTX *p_ctx,
                                        void             *p_cmd_data,
                                        CPU_INT16U       cmd_data_len,
                                        CPU_INT08U       *p_buf_rtn,
                                        CPU_INT08U       buf_rtn_len_max,
                                        RTOS_ERR         *p_err)
{
  NET_DEV_DATA       *p_dev_data;
  CPU_BOOLEAN        *p_link_state;
  CPU_INT16U         ctn;
  A_STATUS           status;
  NET_IF_WIFI_AP_CFG *p_ap_cfg;
  A_DRIVER_CONTEXT   *p_dcxt;
  CPU_INT32U         rtn = 0;

  //                                                               ----------- OBTAIN REFERENCE TO DEV DATA -----------
  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;
  p_dcxt = (A_DRIVER_CONTEXT *)p_dev_data->CommonCxt;

  p_ctx->WaitRespTimeout_ms = NET_DEV_MGMT_RESP_COMMON_TIMEOUT_MS;

  switch (cmd) {
    case NET_DEV_MGMT_DRIVER_INIT:
      //                                                           ---------------- START DRIVER TASK -----------------
      KAL_SemPost(p_dev_data->DriverStartSemHandle,
                  KAL_OPT_PEND_NONE,
                  p_err);
      if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        break;
      }
      //                                                           Launch the driver initialization sequence.
      status = Driver_Init(p_if);
      if (status != A_OK) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_IO);
        break;
      }

      p_ctx->WaitResp = DEF_YES;
      p_ctx->MgmtCompleted = DEF_NO;
      p_dev_data->WaitRespType = NET_DEV_MGMT_DRIVER_INIT;
      break;

    case NET_IF_WIFI_CMD_SCAN:                                  // --------------------- SCAN CMD ---------------------

      NetDev_MgmtExecuteCmdScan(p_if,
                                p_ctx,
                                (NET_IF_WIFI_SCAN *)p_cmd_data,
                                p_err);
      if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;
        p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
        break;
      }
      break;

    case NET_IF_WIFI_CMD_JOIN:                                  // --------------------- JOIN CMD --------------------
      switch (p_dev_data->State.Connect) {
        case NET_DEV_STATE_CONNECT_NONE:
          if (p_dcxt->conn[p_dcxt->devId].isConnected == A_FALSE) {
            NetDev_MgmtExecuteCmdConnect(p_if,
                                         cmd,
                                         (NET_IF_WIFI_AP_CFG *)p_cmd_data,
                                         p_err);
            if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
              p_ctx->WaitResp = DEF_NO;
              p_ctx->MgmtCompleted = DEF_YES;
              p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
            } else {
              p_dev_data->State.Connect = NET_DEV_STATE_CONNECT_802_11_WAITING;
              p_ctx->WaitResp = DEF_YES;
              p_ctx->MgmtCompleted = DEF_NO;
              p_dev_data->WaitRespType = NET_IF_WIFI_CMD_JOIN;
            }
          } else {
            RTOS_ERR_SET(*p_err, RTOS_ERR_IO);
            p_ctx->WaitResp = DEF_NO;
            p_ctx->MgmtCompleted = DEF_YES;
            p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
          }
          break;

        case NET_DEV_STATE_CONNECT_802_11_AUTH:
          p_ap_cfg = (NET_IF_WIFI_AP_CFG *) p_cmd_data;
          switch (p_ap_cfg->SecurityType) {
            case NET_IF_WIFI_SECURITY_OPEN:
            case NET_IF_WIFI_SECURITY_WEP:
              p_ctx->WaitResp = DEF_NO;
              p_ctx->MgmtCompleted = DEF_YES;
              p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
              p_dev_data->LinkStatus = NET_IF_LINK_UP;
              p_dcxt->conn[p_dcxt->devId].isConnected = A_TRUE;
              p_dev_data->State.Connect = NET_DEV_STATE_CONNECT_NONE;
              break;

            case NET_IF_WIFI_SECURITY_WPA:
            case NET_IF_WIFI_SECURITY_WPA2:
              p_dev_data->State.Connect = NET_DEV_STATE_CONNECT_WPA_WAITING;
              p_ctx->WaitRespTimeout_ms = NET_DEV_MGMT_RESP_WPA_TIMEOUT_MS;
              p_ctx->WaitResp = DEF_YES;
              p_ctx->MgmtCompleted = DEF_NO;
              p_dev_data->WaitRespType = NET_IF_WIFI_CMD_JOIN;
              break;
          }
          break;

        case NET_DEV_STATE_CONNECT_WPA_AUTH:
          p_ctx->WaitResp = DEF_NO;
          p_ctx->MgmtCompleted = DEF_YES;
          p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
          p_dev_data->LinkStatus = NET_IF_LINK_UP;
          p_dcxt->conn[p_dcxt->devId].isConnected = A_TRUE;
          p_dev_data->State.Connect = NET_DEV_STATE_CONNECT_NONE;
          break;

        case NET_DEV_STATE_CONNECT_WPA_FAIL:
          status = Api_DisconnectWiFi(p_if);
          if (status != A_OK) {
            RTOS_ERR_SET(*p_err, RTOS_ERR_IO);
            p_ctx->WaitResp = DEF_NO;
            p_ctx->MgmtCompleted = DEF_YES;
            p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
          } else {
            p_ctx->WaitResp = DEF_YES;
            p_ctx->MgmtCompleted = DEF_NO;
            p_dev_data->WaitRespType = NET_IF_WIFI_CMD_JOIN;
          }
          break;

        case NET_DEV_STATE_CONNECT_WPA_WAITING:
        case NET_DEV_STATE_CONNECT_802_11_WAITING:
          p_dev_data->State.Connect = NET_DEV_STATE_CONNECT_NONE;
          p_ctx->WaitResp = DEF_NO;
          p_ctx->MgmtCompleted = DEF_YES;
          p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
          RTOS_ERR_SET(*p_err, RTOS_ERR_IO);
      }
      break;

    case NET_IF_WIFI_CMD_CREATE:                                // -------------------- CREATE CMD -------------------
      NetDev_MgmtExecuteCmdConnect(p_if,
                                   cmd,
                                   (NET_IF_WIFI_AP_CFG *)p_cmd_data,
                                   p_err);
      if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;
        p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
        break;
      }
      p_ctx->WaitResp = DEF_YES;
      p_ctx->MgmtCompleted = DEF_NO;
      p_dev_data->WaitRespType = NET_IF_WIFI_CMD_CREATE;
      break;

    //                                                             -------------------- LEAVE CMD ---------------------
    case NET_IF_WIFI_CMD_LEAVE:
      if (p_dcxt->conn[p_dcxt->devId].isConnected == A_TRUE) {
        status = Api_DisconnectWiFi(p_if);
        if (status != A_OK) {
          RTOS_ERR_SET(*p_err, RTOS_ERR_IO);
          p_ctx->WaitResp = DEF_NO;
          p_ctx->MgmtCompleted = DEF_YES;
          p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
          break;
        }
        p_ctx->WaitResp = DEF_YES;
        p_ctx->MgmtCompleted = DEF_NO;
        p_dev_data->WaitRespType = NET_IF_WIFI_CMD_LEAVE;
      } else {
        RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;
        p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
      }
      break;

    //                                                             ---------------- GET PEER INFO CMD -----------------
    case NET_IF_WIFI_CMD_GET_PEER_INFO:
      ctn = NetDev_PeerInfoGet(p_if,
                               p_buf_rtn,
                               buf_rtn_len_max,
                               p_err);
      if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;
        p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
        break;
      }
      p_ctx->WaitResp = DEF_NO;
      p_ctx->MgmtCompleted = DEF_YES;
      p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;

      rtn = (ctn * sizeof(NET_IF_WIFI_PEER));
      break;

    //                                                             ------------------- STATUS CMD --------------------
    case NET_IF_IO_CTRL_LINK_STATE_GET:
    case NET_IF_IO_CTRL_LINK_STATE_GET_INFO:
      p_link_state = (CPU_BOOLEAN *)p_buf_rtn;
      if (p_dcxt->conn[p_dcxt->devId].isConnected == DEF_YES) {
        *p_link_state = NET_IF_LINK_UP;
        p_dev_data->LinkStatus = NET_IF_LINK_UP;
      } else {
        *p_link_state = NET_IF_LINK_DOWN;
        p_dev_data->LinkStatus = NET_IF_LINK_DOWN;
      }
      p_ctx->WaitResp = DEF_NO;
      p_ctx->MgmtCompleted = DEF_YES;
      rtn = sizeof(CPU_BOOLEAN);
      break;

    //                                                             ------------- FIRMWARE VERSION GET CMD -------------
    case NET_DEV_IO_CTRL_FW_VER_GET:
      p_ctx->WaitResp = DEF_NO;
      p_ctx->MgmtCompleted = DEF_YES;
      p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
      Mem_Copy(p_buf_rtn, &p_dev_data->SoftVersion, 4);
      break;

    //                                                             ------------ UNKNOWN AND UNSUPORTED CMD ------------
    case NET_IF_IO_CTRL_LINK_STATE_UPDATE:
    default:
      p_ctx->WaitResp = DEF_NO;
      p_ctx->MgmtCompleted = DEF_YES;
      p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
      RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
      break;
  }

  return (rtn);
}

/****************************************************************************************************//**
 *                                           NetDev_MgmtProcessResp()
 *
 * @brief    After that the wireless manager has received the response, this function is called to
 *           analyse, set the state machine context and fill the return buffer.
 *
 * @param    p_if                Pointer to a network interface.
 *
 * @param    cmd                 Management command to be executed:
 *                                   - NET_IF_WIFI_CMD_SCAN
 *                                   - NET_IF_WIFI_CMD_JOIN
 *                                   - NET_IF_WIFI_CMD_LEAVE
 *                                   - NET_IF_IO_CTRL_LINK_STATE_GET
 *                                   - NET_IF_IO_CTRL_LINK_STATE_GET_INFO
 *                                   - NET_IF_IO_CTRL_LINK_STATE_UPDATE
 *                               Others management commands defined by this driver.
 *
 * @param    p_ctx               State machine context See Note #1.
 *
 * @param    p_buf_rxd           Pointer to a network buffer that contains the command data response.
 *
 * @param    buf_rxd_len         Length of the data response.
 *
 * @param    p_buf_rtn           Pointer to buffer that will receive return data.
 *
 * @param    buf_rtn_len_max     Return data length max.
 *
 * @param    p_err               Pointer to variable that will receive the return error code from this function.
 *                                   - RTOS_ERR_NONE        Management response     successfully processed
 *                                   - RTOS_ERR_IO          Management response NOT successfully processed
 *                                   - RTOS_ERR_NULL_PTR    Pointer argument(s) passed NULL pointer(s).
 *
 * @return   length of data wrote in the return buffer in octet.
 *
 * @note     (1) The network buffer is always freed by the wireless manager, no matter the error returned.
 *******************************************************************************************************/
static CPU_INT32U NetDev_MgmtProcessResp(NET_IF           *p_if,
                                         NET_IF_WIFI_CMD  cmd,
                                         NET_WIFI_MGR_CTX *p_ctx,
                                         CPU_INT08U       *p_buf_rxd,
                                         CPU_INT16U       buf_rxd_len,
                                         CPU_INT08U       *p_buf_rtn,
                                         CPU_INT16U       buf_rtn_len_max,
                                         RTOS_ERR         *p_err)
{
  CPU_INT32U       rtn;
  CPU_CHAR         *p_frame;
  NET_DEV_DATA     *p_dev_data;
  NET_DEV_CFG_WIFI *p_dev_cfg;
  A_DRIVER_CONTEXT *p_dcxt;
  A_UINT16         id;
  WMI_READY_EVENT  *p_ready_data;

  //                                                               -------- OBTAIN REFERENCE TO CFGS/DEV/DCXT ---------
  p_dev_cfg = (NET_DEV_CFG_WIFI *)p_if->Dev_Cfg;
  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;
  p_dcxt = (A_DRIVER_CONTEXT *)p_dev_data->CommonCxt;

  p_frame = (CPU_CHAR *)(p_buf_rxd + p_dev_cfg->RxBufIxOffset - NET_DEV_ATHEROS_PACKET_HEADER);
  rtn = 0;

  switch (cmd) {
    case NET_DEV_MGMT_DRIVER_INIT:
      //                                                           ----------------- DRIVER INIT CMD ------------------
      p_ready_data = (WMI_READY_EVENT *)(p_frame + HTC_HDR_LENGTH + sizeof(WMI_CMD_HDR));

      Mem_Copy(p_dev_data->DeviceMacAddr,
               p_ready_data->macaddr,
               NET_IF_802x_ADDR_SIZE);

      Api_WMIInitFinish(p_if);
      p_dcxt->wmiReady = A_TRUE;
      p_dev_data->SoftVersion = p_ready_data->sw_version;
      p_ctx->WaitResp = DEF_NO;
      p_ctx->MgmtCompleted = DEF_YES;
      p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
      break;

    //                                                             --------------------- SCAN CMD ---------------------
    case NET_IF_WIFI_CMD_SCAN:
      if (p_dev_data->WaitRespType == NET_IF_WIFI_CMD_SCAN) {
        rtn = NetDev_MgmtProcessRespScan(p_if,
                                         p_ctx,
                                         p_frame,
                                         buf_rxd_len,
                                         p_buf_rtn,
                                         buf_rtn_len_max,
                                         p_err);
      }
      break;

    //                                                             -------------------- CREATE CMD --------------------
    case NET_IF_WIFI_CMD_CREATE:
      if (p_dev_data->WaitRespType == NET_IF_WIFI_CMD_CREATE) {
        id = NetDev_GetEventID(p_if, p_buf_rxd);

        if (id == WMI_CONNECT_EVENTID) {
          //                                                       In case of connect event, the network creation is...
          //                                                       ...successful.
          p_dev_data->LinkStatus = NET_IF_LINK_UP;
          p_dcxt->conn[p_dcxt->devId].isConnected = A_TRUE;
          RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
        } else if (id == WMI_DISCONNECT_EVENTID) {
          //                                                       In case of disconnect event, the network creation...
          //                                                       ... has failed.
          RTOS_ERR_SET(*p_err, RTOS_ERR_IO);
          p_dev_data->LinkStatus = NET_IF_LINK_DOWN;
          p_dcxt->conn[p_dcxt->devId].isConnected = A_FALSE;
        }

        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;
        p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
        p_dcxt->conn[p_dcxt->devId].isConnectPending = A_FALSE;
      }
      break;

    case NET_IF_WIFI_CMD_JOIN:                                  // --------------- JOIN SEQUENCE CMD SET --------------
      if (p_dev_data->WaitRespType == NET_IF_WIFI_CMD_JOIN) {
        id = NetDev_GetEventID(p_if, p_buf_rxd);
        if ( p_dev_data->State.Connect == NET_DEV_STATE_CONNECT_802_11_WAITING) {
          if (id == WMI_CONNECT_EVENTID) {
            //                                                     In case of connect event, the AP association is...
            //                                                     ...successful.
            p_dev_data->State.Connect = NET_DEV_STATE_CONNECT_802_11_AUTH;
            p_ctx->WaitResp = DEF_YES;
            p_ctx->MgmtCompleted = DEF_NO;
            p_dcxt->conn[p_dcxt->devId].isConnectPending = A_FALSE;
            p_dcxt->conn[p_dcxt->devId].isConnected = A_TRUE;
            RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
          } else if (id == WMI_DISCONNECT_EVENTID) {
            //                                                     In case of disconnect event, the AP association...
            //                                                     ... has failed.
            RTOS_ERR_SET(*p_err, RTOS_ERR_IO);
            p_dev_data->LinkStatus = NET_IF_LINK_DOWN;
            p_dev_data->State.Connect = NET_DEV_STATE_CONNECT_NONE;
            p_ctx->WaitResp = DEF_NO;
            p_ctx->MgmtCompleted = DEF_YES;
            p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
            p_dcxt->conn[p_dcxt->devId].isConnected = A_FALSE;
            p_dcxt->conn[p_dcxt->devId].isConnectPending = A_FALSE;
          }
        } else if ( p_dev_data->State.Connect == NET_DEV_STATE_CONNECT_WPA_WAITING) {
          if (id == WMI_PEER_NODE_EVENTID) {
            //                                                     In case of connect event, the AP association is...
            //                                                     ...successful.
            p_dev_data->State.Connect = NET_DEV_STATE_CONNECT_WPA_AUTH;
            p_ctx->WaitResp = DEF_YES;
            p_ctx->MgmtCompleted = DEF_NO;
            p_dcxt->conn[p_dcxt->devId].isConnectPending = A_TRUE;
            RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
          } else if (id == WMI_DISCONNECT_EVENTID) {
            //                                                     In case of disconnect event, the AP association...
            //                                                     ... has failed.
            p_dev_data->State.Connect = NET_DEV_STATE_CONNECT_WPA_FAIL;
            p_ctx->WaitResp = DEF_YES;
            p_ctx->MgmtCompleted = DEF_NO;
            p_dcxt->conn[p_dcxt->devId].isConnectPending = A_FALSE;
          }
        } else if ( p_dev_data->State.Connect == NET_DEV_STATE_CONNECT_WPA_FAIL) {
          if (id == WMI_DISCONNECT_EVENTID) {
            //                                                     In case of disconnect event, the AP association...
            //                                                     ... has failed.
            RTOS_ERR_SET(*p_err, RTOS_ERR_IO);
            p_dev_data->State.Connect = NET_DEV_STATE_CONNECT_NONE;
            p_ctx->WaitResp = DEF_NO;
            p_ctx->MgmtCompleted = DEF_YES;
            p_dcxt->conn[p_dcxt->devId].isConnectPending = A_FALSE;
            p_dcxt->conn[p_dcxt->devId].isConnected = A_FALSE;
            p_dev_data->LinkStatus = NET_IF_LINK_DOWN;
          }
        }
      }
      break;

    //                                                             -------------------- LEAVE CMD ---------------------
    case NET_IF_WIFI_CMD_LEAVE:
      if (p_dev_data->WaitRespType == NET_IF_WIFI_CMD_LEAVE) {
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;
        p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
        p_dcxt->conn[p_dcxt->devId].isConnected = A_FALSE;
        p_dev_data->LinkStatus = NET_IF_LINK_DOWN;
      }
      break;

    //                                                             ------------------- STATUS CMD --------------------
    case NET_IF_IO_CTRL_LINK_STATE_GET:
    case NET_IF_IO_CTRL_LINK_STATE_GET_INFO:
      RTOS_ERR_SET(*p_err, RTOS_ERR_IO);
      break;

    //                                                             ------------ UNKNOWN AND UNSUPORTED CMD ------------
    default:
    case NET_DEV_IO_CTRL_FW_VER_GET:
      RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
      break;
  }

  return (rtn);
}

/********************************************************************************************************
 ********************************************************************************************************
 *                                           LOCAL FUNCTIONS
 ********************************************************************************************************
 *******************************************************************************************************/

/****************************************************************************************************//**
 *                                       NetDev_MgmtExecuteCmdScan()
 *
 * @brief    Send command to the device to start the scan for available wireless network.
 *
 * @param    p_if    Pointer to a network interface.
 *
 * @param    p_scan  Pointer to a variable that contains the information to scan for.
 *
 * @param    p_err   Pointer to variable  that will receive the return error code from this function.
 *
 * @note     (1) Prior calling this function, the SPI bus must be configured correctly:
 *
 * @note     (2) The scan is completed the device and then results are returned through a response.
 *******************************************************************************************************/
static void NetDev_MgmtExecuteCmdScan(NET_IF           *p_if,
                                      NET_WIFI_MGR_CTX *p_ctx,
                                      NET_IF_WIFI_SCAN *p_scan,
                                      RTOS_ERR         *p_err)
{
  NET_DEV_DATA        *p_dev_data;
  WMI_START_SCAN_CMD  scan_cmd;
  WMI_BSS_FILTER_CMD  bss_filter_param;
  WMI_PROBED_SSID_CMD probed_ssid;
  A_STATUS            status;
  CPU_BOOLEAN         is_bssid_filter;

  //                                                               ----------- OBTAIN REFERENCE TO DEV DATA -----------
  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;

  switch (p_dev_data->State.Scan) {
    //                                                             ------------- SET BSS FILTER FOR SCAN --------------
    case NET_DEV_STATE_SCAN_WAIT_RESP:                          // If the state is for waiting responsee, the...
    //                                                             last scan has failed and it must start from the ...
    //                                                             beginning.
    case NET_DEV_STATE_SCAN_CMD_CONFIGURE:
      is_bssid_filter = (p_scan->SSID.SSID[0] != ASCII_CHAR_NULL) ? DEF_YES : DEF_NO;

      if (is_bssid_filter == DEF_YES) {                         // In case the command look for a specified SSID.
        bss_filter_param.bssFilter = PROBED_SSID_FILTER;

        probed_ssid.entryIndex = 0;
        probed_ssid.flag = SPECIFIC_SSID_FLAG;
        probed_ssid.ssidLength = Str_Len_N(p_scan->SSID.SSID, NET_IF_WIFI_STR_LEN_MAX_SSID);
        Mem_Copy(probed_ssid.ssid, p_scan->SSID.SSID, probed_ssid.ssidLength);

        status = NetDev_WMICmdSend(p_if,
                                   WMI_SET_PROBED_SSID_CMDID,
                                   &probed_ssid,
                                   sizeof(WMI_PROBED_SSID_CMD));
        if (status != A_OK) {
          RTOS_ERR_SET(*p_err, RTOS_ERR_IO);
          return;
        }
      } else {                                                  // Otherwise, look for all the BSS.
        bss_filter_param.bssFilter = ALL_BSS_FILTER;
      }

      bss_filter_param.ieMask = 0;

      status = NetDev_WMICmdSend(p_if,
                                 WMI_SET_BSS_FILTER_CMDID,
                                 &bss_filter_param,
                                 sizeof(WMI_BSS_FILTER_CMD));
      if (status != A_OK) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_IO);
        return;
      }

      p_dev_data->State.Scan = NET_DEV_STATE_SCAN_START;
      p_ctx->WaitResp = DEF_NO;
      p_ctx->MgmtCompleted = DEF_NO;
      p_ctx->WaitRespTimeout_ms = NET_DEV_MGMT_RESP_SCAN_TIMEOUT_MS;
      p_dev_data->WaitRespType = NET_IF_WIFI_CMD_SCAN;
      break;

    case NET_DEV_STATE_SCAN_START:
      //                                                           ---------------- START THE SCAN CMD ----------------
      p_dev_data->State.ScanAPCount = 0u;
      scan_cmd.scanType = WMI_LONG_SCAN;
      scan_cmd.forceFgScan = A_FALSE;
      scan_cmd.isLegacy = A_FALSE;
      scan_cmd.homeDwellTime = 0u;
      scan_cmd.forceScanInterval = 0u;
      //                                                           Specified the channel(s) to scan.
      if (p_scan->Ch == NET_IF_WIFI_CH_ALL) {
        scan_cmd.numChannels = 0u;
      } else {
        scan_cmd.numChannels = 1u;
        scan_cmd.channelList[0] = Util_Ieee2freq(p_scan->Ch);
      }
      status = NetDev_WMICmdSend(p_if,
                                 WMI_START_SCAN_CMDID,
                                 &scan_cmd,
                                 sizeof(WMI_START_SCAN_CMD));
      if (status != A_OK) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_IO);
      }

      p_dev_data->State.Scan = NET_DEV_STATE_SCAN_WAIT_RESP;
      p_ctx->WaitResp = DEF_YES;
      p_ctx->MgmtCompleted = DEF_NO;
      p_ctx->WaitRespTimeout_ms = NET_DEV_MGMT_RESP_SCAN_TIMEOUT_MS;
      p_dev_data->WaitRespType = NET_IF_WIFI_CMD_SCAN;
      break;

    //                                                             -- WAIT FOR OTHER BSS_INFO OR SCAN_COMPLETE EVENT --
    case NET_DEV_STATE_SCAN_WAIT_COMPLETE:
      p_dev_data->State.Scan = NET_DEV_STATE_SCAN_WAIT_RESP;
      p_ctx->WaitResp = DEF_YES;
      p_ctx->MgmtCompleted = DEF_NO;
      p_ctx->WaitRespTimeout_ms = NET_DEV_MGMT_RESP_SCAN_TIMEOUT_MS;
      p_dev_data->WaitRespType = NET_IF_WIFI_CMD_SCAN;
      break;

    default:
      break;
  }
}

/****************************************************************************************************//**
 *                                       NetDev_MgmtProcessRespScan()
 *
 * @brief    Analyse a scan response and fill application buffer.
 *
 * @param    p_if        Pointer to a network interface.
 *
 * @param    p_frame     Pointer to a network buffer that contains the management frame response.
 *
 * @param    frame_len   Length of the data response.
 *
 * @param    p_ap_buf    Pointer to the access point buffer.
 *
 * @param    buf_len     Length of the access point buffer in octet.
 *
 * @param    p_err       Pointer to variable  that will receive the return error code from this function.
 *                           - RTOS_ERR_NONE        Scan response successfully processed
 *                           - RTOS_ERR_NULL_PTR    Pointer argument(s) passed NULL pointer(s).
 *                           - RTOS_ERR_IO          Invalid scan response.
 *
 * @return   Number of octet wrote in the access point buffer.
 *
 * @note     (1) This function call Api_ParseInfoElem() which is part of the Qualcomm/Atheros Driver.
 *******************************************************************************************************/
CPU_INT32U NetDev_MgmtProcessRespScan(NET_IF           *p_if,
                                      NET_WIFI_MGR_CTX *p_ctx,
                                      CPU_CHAR         *p_frame,
                                      CPU_INT16U       p_frame_len,
                                      CPU_INT08U       *p_ap_buf,
                                      CPU_INT16U       buf_len,
                                      RTOS_ERR         *p_err)
{
  NET_DEV_DATA      *p_dev_data;
  WMI_CMD_HDR       *cmd;
  A_UINT16          id;
  CPU_CHAR          *p_data;
  WMI_BSS_INFO_HDR  *bih;
  WMI_BSS_INFO_HDR2 bih2;
  A_SCAN_SUMMARY    scan_summary;
  NET_IF_WIFI_AP    *p_ap;
  CPU_INT08U        index;
  CPU_INT08U        i;
  CPU_BOOLEAN       bssid_found;
  CPU_INT08U        rem_space;
  CPU_INT16U        bss_type;
  CPU_BOOLEAN       privacy;
  CPU_INT32U        nb_max_ap;

  //                                                               ----------- OBTAIN REFERENCE TO DEV DATA -----------
  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;

  p_ap = (NET_IF_WIFI_AP *)p_ap_buf;

  switch (p_dev_data->State.Scan) {
    case NET_DEV_STATE_SCAN_WAIT_RESP:
      p_data = p_frame + HTC_HDR_LENGTH;
      cmd = (WMI_CMD_HDR *)p_data;
      id = cmd->commandId;
      p_data += sizeof(WMI_CMD_HDR);
      //                                                           If it is a BSSINFO event, get all the SSID...
      //                                                           ... information available.
      if (id == WMI_BSSINFO_EVENTID) {
        //                                                         Convert the event data to fit with BSS_INFO_HDR.
        Mem_Copy(&bih2, p_data, sizeof(WMI_BSS_INFO_HDR2));

        p_data -= 4;
        bih = (WMI_BSS_INFO_HDR *)p_data;
        bih->channel = bih2.channel;
        bih->frameType = bih2.frameType;
        bih->snr = bih2.snr;
        bih->rssi = (CPU_INT16S)(bih2.snr - 95);
        bih->ieMask = bih2.ieMask;
        Mem_Copy(bih->bssid, bih2.bssid, ATH_MAC_LEN);
        //                                                         Parse the BSS_INFO event.
        Api_ParseInfoElem(p_if, bih, p_frame_len, &scan_summary);

        index = p_dev_data->State.ScanAPCount;
        bssid_found = DEF_NO;
        //                                                         Check if we already receive the info on this BSSID.
        for (i = 0; i < index; i++) {
          bssid_found = Mem_Cmp(p_ap[i].BSSID.BSSID,
                                scan_summary.bssid,
                                NET_IF_802x_ADDR_SIZE);
          if (bssid_found == DEF_YES) {
            break;
          }
        }

        if (bssid_found == DEF_NO) {
          p_ap[index].Ch = scan_summary.channel;
          //                                                       Parse the Network type.
          bss_type = scan_summary.caps & (IEEE80211_CAPINFO_ESS | IEEE80211_CAPINFO_IBSS);

          if (bss_type == IEEE80211_CAPINFO_ESS) {
            p_ap[index].NetType = NET_IF_WIFI_NET_TYPE_INFRASTRUCTURE;
          } else {
            p_ap[index].NetType = NET_IF_WIFI_NET_TYPE_ADHOC;
          }
          //                                                       Get the Security type.
          privacy = (scan_summary.caps & IEEE80211_CAPINFO_PRIVACY) ? DEF_YES : DEF_NO;
          if (privacy == DEF_NO) {
            p_ap[index].SecurityType = NET_IF_WIFI_SECURITY_OPEN;
          } else {
            switch (scan_summary.rsn_auth ) {                   // First check if the rsn information element has...
              //                                                   ... been received.
              case NONE_AUTH:
                p_ap[index].SecurityType = NET_IF_WIFI_SECURITY_OPEN;
                break;

              case WPA_AUTH:
              case WPA_PSK_AUTH:
              case WPA_AUTH_CCKM:
                p_ap[index].SecurityType = NET_IF_WIFI_SECURITY_WPA;
                break;

              case WPA2_AUTH:
              case WPA2_PSK_AUTH:
              case WPA2_AUTH_CCKM:
                p_ap[index].SecurityType = NET_IF_WIFI_SECURITY_WPA2;
                break;

              default:
                switch (scan_summary.wpa_auth) {                // Then check if the wpa information element has...
                  //                                               ... been received.
                  case NONE_AUTH:
                    p_ap[index].SecurityType = NET_IF_WIFI_SECURITY_OPEN;
                    break;

                  case WPA_AUTH:
                  case WPA_PSK_AUTH:
                  case WPA_AUTH_CCKM:
                    p_ap[index].SecurityType = NET_IF_WIFI_SECURITY_WPA;
                    break;

                  case WPA2_AUTH:
                  case WPA2_PSK_AUTH:
                  case WPA2_AUTH_CCKM:
                    p_ap[index].SecurityType = NET_IF_WIFI_SECURITY_WPA2;
                    break;
                  default:
                    p_ap[index].SecurityType = NET_IF_WIFI_SECURITY_WEP;
                    break;
                }
                break;
            }
          }
          nb_max_ap = buf_len / sizeof(NET_IF_WIFI_AP);
          if (nb_max_ap > index) {
            //                                                     Get the RSSI.
            p_ap[index].SignalStrength = -bih->rssi;
            //                                                     Copy the SSID and fill the remaining space with 0s.
            Mem_Copy(p_ap[index].SSID.SSID, scan_summary.ssid, scan_summary.ssid_len);
            rem_space = NET_IF_WIFI_STR_LEN_MAX_SSID - scan_summary.ssid_len;
            Mem_Set(&p_ap[index].SSID.SSID[scan_summary.ssid_len],
                    ASCII_CHAR_NULL,
                    rem_space);
            //                                                     Copy the BSSID.
            Mem_Copy(p_ap[index].BSSID.BSSID, scan_summary.bssid, NET_IF_802x_ADDR_SIZE);

            p_dev_data->State.ScanAPCount++;
          }
        }
        //                                                         Wait for the next event.
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_NO;
        p_dev_data->State.Scan = NET_DEV_STATE_SCAN_WAIT_COMPLETE;
      } else if (id == WMI_SCAN_COMPLETE_EVENTID) {             // If it's the Scan complete event, finish the command.
        if ((*p_data == A_EBUSY)
            || (*p_data == A_ECANCELED)) {
          RTOS_ERR_SET(*p_err, RTOS_ERR_IO);
        }
        p_dev_data->State.Scan = NET_DEV_STATE_SCAN_CMD_CONFIGURE;
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;
        p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
      }
      break;

    case NET_DEV_STATE_SCAN_WAIT_COMPLETE:
    case NET_DEV_STATE_SCAN_CMD_CONFIGURE:
    case NET_DEV_STATE_SCAN_START:
    default:
      RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
      break;
  }

  return (p_dev_data->State.ScanAPCount * sizeof(NET_IF_WIFI_AP));
}

/****************************************************************************************************//**
 *                                       NetDev_MgmtExecuteCmdConnect()
 *
 * @brief    Send command to associate the device to a network / create an access point.
 *
 * @param    p_if    Pointer to a network interface.
 *
 * @param    cmd     Management command
 *
 * @param    p_join  Pointer to variable that contains the wireless network to join.
 *
 * @param    p_err   Pointer to variable  that will receive the return error code from this function.
 *
 * @note     (1) This function call Api_ConnectWiFi() which is part of the Qualcomm/Atheros Driver.
 *******************************************************************************************************/
static void NetDev_MgmtExecuteCmdConnect(NET_IF             *p_if,
                                         NET_IF_WIFI_CMD    cmd,
                                         NET_IF_WIFI_AP_CFG *p_ap_cfg,
                                         RTOS_ERR           *p_err)
{
  NET_DEV_DATA           *p_dev_data;
  A_DRIVER_CONTEXT       *p_dcxt;
  WMI_SET_PASSPHRASE_CMD pass_cmd;
  A_STATUS               status;
  CPU_SIZE_T             pwd_len;
  A_UINT8                hidden_ssid_flag;
  WMI_SET_CHANNEL_CMD    ch;

  //                                                               -------- OBTAIN REFERENCE TO DEV DATA/DCXT ---------
  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;
  p_dcxt = (A_DRIVER_CONTEXT *)p_dev_data->CommonCxt;

  //                                                               --------- SET BSS INFO IN DRIVER'S CONTEXT ---------
  //                                                               Set the network Type.
  if (cmd == NET_IF_WIFI_CMD_JOIN) {
    if (p_ap_cfg->NetType == NET_IF_WIFI_NET_TYPE_INFRASTRUCTURE) {
      p_dcxt->conn[p_dcxt->devId].networkType = INFRA_NETWORK;
    } else {
      RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_SUPPORTED);
      return;
    }
  } else if (cmd == NET_IF_WIFI_CMD_CREATE) {
    if (p_ap_cfg->NetType == NET_IF_WIFI_NET_TYPE_INFRASTRUCTURE) {
      p_dcxt->conn[p_dcxt->devId].networkType = AP_NETWORK;
    } else {
      RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_SUPPORTED);
      return;
    }
  }
  //                                                               Set SSID Name and length.
  p_dcxt->conn[p_dcxt->devId].ssidLen = Str_Len_N(p_ap_cfg->SSID.SSID, NET_IF_WIFI_STR_LEN_MAX_SSID);
  Mem_Copy(p_dcxt->conn[p_dcxt->devId].ssid,
           p_ap_cfg->SSID.SSID,
           p_dcxt->conn[p_dcxt->devId].ssidLen);
  //                                                               Set the Security Type.
  switch (p_ap_cfg->SecurityType) {
    case NET_IF_WIFI_SECURITY_OPEN:                             // Open network.
      p_dcxt->conn[p_dcxt->devId].wpaAuthMode = NONE_AUTH;
      p_dcxt->conn[p_dcxt->devId].wpaPairwiseCrypto = NONE_CRYPT;
      p_dcxt->conn[p_dcxt->devId].wpaPairwiseCryptoLen = 0;
      p_dcxt->conn[p_dcxt->devId].wpaGroupCrypto = NONE_CRYPT;
      p_dcxt->conn[p_dcxt->devId].wpaPairwiseCryptoLen = 0;
      p_dcxt->conn[p_dcxt->devId].dot11AuthMode = OPEN_AUTH;
      p_dcxt->conn[p_dcxt->devId].connectCtrlFlags &= ~CONNECT_DO_WPA_OFFLOAD;
      break;

    case NET_IF_WIFI_SECURITY_WEP:                              // WEP network.
      pwd_len = Str_Len_N(p_ap_cfg->PSK.PSK, NET_IF_WIFI_STR_LEN_MAX_PSK);
      if ((pwd_len != NET_DEV_WIFI_WEP64_KEY_LEN)
          && (pwd_len != NET_DEV_WIFI_WEP128_KEY_LEN)) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_IO);
        return;
      }

      p_dcxt->conn[p_dcxt->devId].wpaAuthMode = NONE_AUTH;
      p_dcxt->conn[p_dcxt->devId].wpaPairwiseCrypto = WEP_CRYPT;
      p_dcxt->conn[p_dcxt->devId].wpaPairwiseCryptoLen = (CPU_INT08U) pwd_len;
      p_dcxt->conn[p_dcxt->devId].wpaGroupCrypto = WEP_CRYPT;
      p_dcxt->conn[p_dcxt->devId].wpaGroupCryptoLen = (CPU_INT08U) pwd_len;
      p_dcxt->conn[p_dcxt->devId].dot11AuthMode = (OPEN_AUTH | SHARED_AUTH);
      p_dcxt->conn[p_dcxt->devId].connectCtrlFlags &= ~CONNECT_DO_WPA_OFFLOAD;
      break;

    case NET_IF_WIFI_SECURITY_WPA:                              // WPA network. TKIP Crypt.
      p_dcxt->conn[p_dcxt->devId].wpaAuthMode = WPA_PSK_AUTH;
      p_dcxt->conn[p_dcxt->devId].dot11AuthMode = OPEN_AUTH;
      p_dcxt->conn[p_dcxt->devId].wpaPairwiseCrypto = TKIP_CRYPT;
      p_dcxt->conn[p_dcxt->devId].wpaGroupCrypto = TKIP_CRYPT;
      p_dcxt->conn[p_dcxt->devId].connectCtrlFlags |= CONNECT_DO_WPA_OFFLOAD | CONNECT_IGNORE_WPAx_GROUP_CIPHER;
      break;

    case NET_IF_WIFI_SECURITY_WPA2:                             // WPA2 network. AES Crypt.
      p_dcxt->conn[p_dcxt->devId].wpaAuthMode = WPA2_PSK_AUTH;
      p_dcxt->conn[p_dcxt->devId].dot11AuthMode = OPEN_AUTH;
      p_dcxt->conn[p_dcxt->devId].wpaPairwiseCrypto = AES_CRYPT;
      p_dcxt->conn[p_dcxt->devId].wpaGroupCrypto = AES_CRYPT;
      p_dcxt->conn[p_dcxt->devId].connectCtrlFlags |= CONNECT_DO_WPA_OFFLOAD | CONNECT_IGNORE_WPAx_GROUP_CIPHER;
      break;

    default:
      RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_ARG);
      return;
  }
  //                                                               Set the channel.
  if (p_ap_cfg->Ch != NET_IF_WIFI_CH_ALL) {
    p_dcxt->conn[p_dcxt->devId].channelHint = Util_Ieee2freq(p_ap_cfg->Ch);
  } else {
    p_dcxt->conn[p_dcxt->devId].channelHint = 0;
  }
  //                                                               Set the BSSID to 00:00:00:00:00:00.
  Mem_Set(p_dcxt->conn[p_dcxt->devId].reqBssid, 0x00, NET_IF_802x_ADDR_SIZE);

  status = A_OK;
  //                                                               ----------- SET PASSPHRASE OR CIPHER KEY -----------

  if ((p_ap_cfg->SecurityType == NET_IF_WIFI_SECURITY_WPA)
      || (p_ap_cfg->SecurityType == NET_IF_WIFI_SECURITY_WPA2)) {
    //                                                             If WPA/WPA2, use the cmd SET_PASSPHRASE.
    pass_cmd.ssid_len = (A_UINT8) p_dcxt->conn[p_dcxt->devId].ssidLen;
    pass_cmd.passphrase_len = Str_Len_N(p_ap_cfg->PSK.PSK,
                                        NET_IF_WIFI_STR_LEN_MAX_PSK);
    Mem_Copy(pass_cmd.ssid,
             p_dcxt->conn[p_dcxt->devId].ssid,
             p_dcxt->conn[p_dcxt->devId].ssidLen);

    Mem_Copy(pass_cmd.passphrase,
             p_ap_cfg->PSK.PSK,
             pass_cmd.passphrase_len);

    status = NetDev_WMICmdSend(p_if,
                               WMI_SET_PASSPHRASE_CMDID,
                               &pass_cmd,
                               sizeof(WMI_SET_PASSPHRASE_CMD));
    if (status != A_OK) {
      RTOS_ERR_SET(*p_err, RTOS_ERR_IO);
      return;
    }
  } else if (p_ap_cfg->SecurityType == NET_IF_WIFI_SECURITY_WEP) {
    //                                                             If WEP, set the cipher Key in the driver's context.
    p_dcxt->conn[p_dcxt->devId].wepDefTxKeyIndex = 0;
    p_dcxt->conn[p_dcxt->devId].wepKeyList[0].keyLen = Str_Len_N(p_ap_cfg->PSK.PSK,
                                                                 NET_IF_WIFI_STR_LEN_MAX_PSK);
    p_dcxt->conn[p_dcxt->devId].wepKeyList[1].keyLen = Str_Len_N(p_ap_cfg->PSK.PSK,
                                                                 NET_IF_WIFI_STR_LEN_MAX_PSK);

    Mem_Copy(p_dcxt->conn[p_dcxt->devId].wepKeyList[0].key,
             p_ap_cfg->PSK.PSK,
             p_dcxt->conn[p_dcxt->devId].wepKeyList[0].keyLen);

    Mem_Copy(p_dcxt->conn[p_dcxt->devId].wepKeyList[1].key,
             p_ap_cfg->PSK.PSK,
             p_dcxt->conn[p_dcxt->devId].wepKeyList[1].keyLen);
  }
  //                                                               ---------------- SET CREATE AP INFO ----------------
  if (p_dcxt->conn[p_dcxt->devId].networkType == AP_NETWORK) {
    hidden_ssid_flag = HIDDEN_SSID_FALSE;
    status = NetDev_WMICmdSend(p_if,
                               WMI_AP_HIDDEN_SSID_CMDID,
                               &hidden_ssid_flag,
                               sizeof(WMI_AP_HIDDEN_SSID_CMD));
    if (status != A_OK) {
      RTOS_ERR_SET(*p_err, RTOS_ERR_IO);
      return;
    }

    ch.channel = p_dcxt->conn[p_dcxt->devId].channelHint;
    status = NetDev_WMICmdSend(p_if,
                               WMI_SET_CHANNEL_CMDID,
                               &ch,
                               sizeof(WMI_SET_CHANNEL_CMD));
    if (status != A_OK) {
      RTOS_ERR_SET(*p_err, RTOS_ERR_IO);
      return;
    }
  }
  //                                                               ------------- JOIN OR CREATE A NETWORK -------------
  status = Api_ConnectWiFi(p_if);
  if (status != A_OK) {
    RTOS_ERR_SET(*p_err, RTOS_ERR_IO);
  }
}

/********************************************************************************************************
 *                                       NetDev_DeliverFrameToRxTask()
 *
 * @brief  Set the header for the RX Queue and deliver to the RX Task.
 *
 * @param  p_if        Pointer to a network interface.
 *
 * @param  p_a_netbuf  Pointer on the A_NETBUF struct containing the packet information.
 *
 * @param  type        Type of the packet to be queued.
 *                         - NET_IF_WIFI_MGMT_FRAME      Must be handled by the WiFi Manager.
 *                         - NET_IF_WIFI_DATA_PKT        Must be handled by the Network Stack.
 *
 * @param  p_err       Pointer to variable  that will receive the return error code from this function.
 *
 * @note   (1) If the buffer came from the RX Net buffer pool, set the delivered buffer flag. This
 *             means that the RX task now owns this buffer and should release it. In this case, the
 *             Driver task will not free the buffer.
 *******************************************************************************************************/
static void NetDev_RxFramePost(NET_IF                 *p_if,
                               A_NETBUF               *p_a_netbuf,
                               NET_IF_WIFI_FRAME_TYPE type,
                               RTOS_ERR               *p_err)
{
  NET_DEV_DATA   *p_dev_data;
  NET_DEV_RX_HDR *p_hdr;
  RTOS_ERR       local_err;

  RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

  //                                                               ----------- OBTAIN REFERENCE TO DEV DATA -----------
  p_dev_data = (NET_DEV_DATA *) p_if->Dev_Data;                 // Obtain ptr to dev data area.
  p_hdr = (NET_DEV_RX_HDR *) p_a_netbuf->head;
  //                                                               Initialize the message header.
  p_hdr->type = type;
  p_hdr->len = A_NETBUF_LEN(p_a_netbuf);

  if (type == NET_IF_WIFI_DATA_PKT) {
    if (p_hdr->len < NET_IF_802x_FRAME_MIN_SIZE) {
      p_hdr->len = NET_IF_802x_FRAME_MIN_SIZE;
      Mem_Set(p_a_netbuf->data + p_hdr->len,
              0x00,
              NET_IF_802x_FRAME_MIN_SIZE - p_hdr->len);
    }
  }

  KAL_QPost(p_dev_data->DriverRxQueue,                          // Post the packet in the Driver's Rx Queue.
            p_a_netbuf->head,
            KAL_OPT_POST_NONE,
            p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }

  if (p_a_netbuf->pool_id == A_RX_NET_POOL) {                   // Set the buffer as delivered. See Note 1.
    p_a_netbuf->RxBufDelivered = DEF_YES;
  }
  //                                                               Wake the RX Task.
  NetIF_RxQPost(p_if->Nbr, &local_err);
  if (RTOS_ERR_CODE_GET(local_err) != RTOS_ERR_NONE) {
    RTOS_ERR_SET(*p_err, RTOS_ERR_IO);
  }
}

/****************************************************************************************************//**
 *                                           Custom_Api_RxComplete()
 *
 * @brief    Validate the packet received and its type then deliver it to the RX task.
 *
 * @param    p_cxt   Pointer to a network interface. (equivalent to NET_IF *p_if)
 *
 * @param    p_req   Pointer on the A_NETBUF struct that describe the received packet.
 *
 * @note     (1) This function has been inspired from the Api_RxComplete() in api_rxtx.c. It has been
 *               modified for a better integration and to respect the Micrum's coding standard.
 *
 * @note     (2) The following notes are comments from the original code.
 *               - (a) In the case of AP mode we may receive NULL data frames that do not have LLC hdr.
 *                     They are 16 bytes in size. Allow these frames in the AP mode.  ACL data frames
 *                     don't follow ethernet frame bounds for min length.
 *               - (b) rssi is not currently managed on a per peer basis.therefore, if multiple peers
 *                     are sending packets to this radio the rssi will be averaged across all
 *                     indiscriminately.
 *               - (c) simple averaging algorithm.  7/8 of original value is combined with 1/8 of new
 *                     value unless original value is 0 in which case just adopt the new value as
 *                     its probably the first in the connection
 *******************************************************************************************************/
A_VOID Custom_Api_RxComplete(A_VOID * p_cxt,
                             A_VOID * p_req)
{
  A_STATUS         status;
  A_UINT16         min_hdr_len;
  A_UINT16         packet_len;
  A_UINT8          contains_dot11_hdr;
  WMI_DATA_HDR     *dhdr;
  A_UINT8          is_amsdu;
  A_UINT8          is_acl_data_frame;
  A_UINT8          meta_type;
  HTC_ENDPOINT_ID  ept;
  A_DRIVER_CONTEXT *p_dcxt;
  NET_IF           *p_if;
  A_NETBUF         *p_a_netbuf;
  RTOS_ERR         local_err;

  RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

  //                                                               ----------------- OBTAIN REFERENCE -----------------
  p_if = (NET_IF *)p_cxt;
  p_a_netbuf = (A_NETBUF *)p_req;
  ept = (HTC_ENDPOINT_ID)A_NETBUF_GET_ELEM(p_req, A_REQ_EPID);
  p_dcxt = GET_DRIVER_COMMON(p_cxt);

  contains_dot11_hdr = 0;

  if (p_dcxt->wmiEnabled == A_TRUE) {
    if (ept == ENDPOINT_1) {                                    // -------------------- MGMT PACKET ------------------
                                                                // Deliver the management frame to the the RX task.
      NetDev_RxFramePost(p_if,
                         p_a_netbuf,
                         NET_IF_WIFI_MGMT_FRAME,
                         &local_err);
      if (RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE) {
        status = A_OK;
      } else {
        status = A_ERROR;
      }
    } else if (p_dcxt->promiscuous_mode) {
      status = A_ERROR;                                         // Not Implemented Yet.
    } else {                                                    // -------------------- DATA PACKET ------------------
      dhdr = (WMI_DATA_HDR *)A_NETBUF_DATA(p_req);
      packet_len = A_NETBUF_LEN(p_req);
      dhdr->info2 = (dhdr->info2);
      is_acl_data_frame = (A_UINT8)(WMI_DATA_HDR_GET_DATA_TYPE(dhdr) == WMI_DATA_HDR_DATA_TYPE_ACL);
      min_hdr_len = MIN_HDR_LEN;

      do {                                                      // See note 2a.
        if (((p_dcxt->conn[p_dcxt->devId].networkType) != AP_NETWORK)
            && (packet_len < min_hdr_len)
            || (packet_len > AR4100_MAX_RX_MESSAGE_SIZE)) {
          status = A_ERROR;                                     // The size of the packet is invalid.
          break;
        } else {
          is_amsdu = (A_UINT8)WMI_DATA_HDR_IS_AMSDU(dhdr);
          meta_type = (A_UINT8)WMI_DATA_HDR_GET_META(dhdr);
          contains_dot11_hdr = (A_UINT8)WMI_DATA_HDR_GET_DOT11(dhdr);

          //                                                       See note 2b and 2c.
          p_dcxt->rssi = (A_INT8)((p_dcxt->rssi) ? (p_dcxt->rssi * 7 + dhdr->rssi) >> 3 : dhdr->rssi);
          wmi_data_hdr_remove(p_dcxt->pWmiCxt, p_req);

          if (meta_type) {
            status = A_ERROR;
            break;                                              // This code does not accept per frame meta data.
          }

          if (contains_dot11_hdr) {
            status = A_ERROR;
            break;                                              // This code does not accept dot11 headers.
          } else if (!is_amsdu && !is_acl_data_frame) {
            status = WMI_DOT3_2_DIX(p_req);
          } else {
            status = A_ERROR;
            break;                                              // This code does not accept amsdu or acl data.
          }

          if (status != A_OK) {
            break;                                              // Drop frames that could not be processed.
          }

          //                                                       Deliver the data frame to the the RX task.
          NetDev_RxFramePost(p_if,
                             p_a_netbuf,
                             NET_IF_WIFI_DATA_PKT,
                             &local_err);
          if (RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE) {
            status = A_OK;
          } else {
            status = A_ERROR;
          }
        }
      } while (0);
    }
    A_NETBUF_FREE(p_req);
  }
}

/****************************************************************************************************//**
 *                                           NetDev_WMICmdSend()
 *
 * @brief    Send a WMI command request to the Driver task and wait to be completely send.
 *
 * @param    pCxt    Pointer to a network interface. (equivalent to NET_IF *p_if)
 *
 * @param    cmd     ID of the WMI command.
 *
 * @param    pParam  Pointer to a parameters struct of the command to send.
 *
 * @param    length  Length in byte of the parameters structure.
 *
 * @return   Status code : A_OK only, otherwise any error should block (A_ASSERT)
 *
 * @note     (1) This function call wmi_cmd_start() which is part of the Qualcomm/Atheros Driver.
 *******************************************************************************************************/
static A_STATUS NetDev_WMICmdSend(NET_IF         *p_if,
                                  WMI_COMMAND_ID cmd,
                                  void           *p_param,
                                  CPU_INT16U     length)
{
  A_STATUS         status;
  A_DRIVER_CONTEXT *p_dcxt;
  NET_DEV_DATA     *p_dev_data;

  //                                                               -------- OBTAIN REFERENCE TO DEV DATA/DCXT ---------
  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;
  p_dcxt = (A_DRIVER_CONTEXT *)p_dev_data->CommonCxt;

  status = wmi_cmd_start(p_dcxt->pWmiCxt, p_param, cmd, length);

  if (status == A_NO_MEMORY) {
    p_dcxt->tx_complete_pend = A_TRUE;
    status = CUSTOM_DRIVER_WAIT_FOR_CONDITION(p_if, &(p_dcxt->tx_complete_pend), A_FALSE, 5000);
  }

  return status;
}

/****************************************************************************************************//**
 *                                           NetDev_GetEventID()
 *
 * @brief    Get the WMI Event ID from a packet.
 *
 * @param    p_if    Pointer to a network interface.
 *
 * @param    p_buf   Pointer at the beginning of the buffer to be parsed.
 *
 * @return   Value of the WMI Event Id.
 *
 * @note     (1) This function don't validate the Event ID .
 *******************************************************************************************************/
static CPU_INT16U NetDev_GetEventID(NET_IF     *p_if,
                                    CPU_INT08U *p_buf)
{
  NET_DEV_CFG_WIFI *p_dev_cfg;
  WMI_CMD_HDR      *cmd;
  A_UINT16         id;
  CPU_INT08U       *p_data;

  //                                                               ----------- OBTAIN REFERENCE TO DEV CFG ------------
  p_dev_cfg = (NET_DEV_CFG_WIFI *)p_if->Dev_Cfg;

  //                                                               Retrieve the Event ID.
  p_data = p_buf + p_dev_cfg->RxBufIxOffset + HTC_HDR_LENGTH - NET_DEV_ATHEROS_PACKET_HEADER;
  cmd = (WMI_CMD_HDR *)p_data;
  id = (cmd->commandId);

  return (id);
}

/****************************************************************************************************//**
 *                                       NetDev_GetDriverTxANetBuf()
 *
 * @brief    Get and Initialize a A_NETBUF structure for a Tramsmit Request to the driver.
 *
 * @param    p_if    Pointer to a network interface.
 *
 * @param    p_data  Pointer to a packet to send.
 *
 * @param    size    Size of the packet to send.
 *
 * @param    p_err   Pointer to variable  that will receive the return error code from this function.
 *
 * @return   A_NETBUF struture initialized with p_data pointer and size value.
 *
 * @note     (1) The Qualcomm/Atheros Driver use the A_NETBUF structure to describe the buffer for
 *               TX/RX. This is the structure that is use for transmit request to the driver.
 *
 * @note     (2) Once the transmit request is deliver to the driver task, it will free the A_NETBUF
 *               structure after completing the transmission.
 *******************************************************************************************************/
static A_NETBUF *NetDev_GetDriverTxANetBuf(NET_IF     *p_if,
                                           CPU_INT08U *p_data,
                                           CPU_INT16U size,
                                           RTOS_ERR   *p_err)
{
  NET_DEV_CFG_WIFI *p_dev_cfg;
  A_NETBUF         *p_a_netbuf;

  //                                                               ----------- OBTAIN REFERENCE TO DEV CFG ------------
  p_dev_cfg = (NET_DEV_CFG_WIFI *)p_if->Dev_Cfg;

  //                                                               -------------- GET A_NETBUF FROM POOL --------------
  p_a_netbuf = (A_NETBUF *) Mem_DynPoolBlkGet(&(GET_DRIVER_CXT(p_if)->A_NetBufPool),
                                              p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return (DEF_NULL);
  }
  //                                                               --------------- INITIALIZE A_NETBUF ----------------
  p_a_netbuf->head = p_data - p_dev_cfg->TxBufIxOffset;
  p_a_netbuf->data = p_data;
  p_a_netbuf->tail = p_data + size;
  p_a_netbuf->end = p_data + p_dev_cfg->TxBufLargeSize;
  p_a_netbuf->pool_id = A_TX_NET_POOL;
  p_a_netbuf->dealloc = p_data;
  p_a_netbuf->RxBufDelivered = DEF_NO;

  return (p_a_netbuf);
}

/****************************************************************************************************//**
 *                                           NetDev_AddPeerInfo()
 *
 * @brief    Add a new peer in the list in the dev data area.
 *
 * @param    p_if    Pointer to a network interface.
 *
 * @param    p_data  Pointer to a connect event with the peer info.
 *
 * @param    p_err   Pointer to variable  that will receive the return error code from this function.
 *******************************************************************************************************/
static void NetDev_PeerInfoAdd(NET_IF     *p_if,
                               CPU_INT08U *p_data,
                               RTOS_ERR   *p_err)
{
  NET_DEV_DATA      *p_dev_data;
  WMI_CONNECT_EVENT *p_wmi_connect;
  NET_DEV_PEER_INFO *p_peer;
  NET_DEV_PEER_INFO *p_buf;

  //                                                               ----------- OBTAIN REFERENCE TO DEV DATA -----------
  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;
  p_wmi_connect = (WMI_CONNECT_EVENT *)NetDev_GetEventDataPtr(p_if, p_data);

  //                                                               Get a new memory Block.
  p_buf = (NET_DEV_PEER_INFO *)Mem_DynPoolBlkGet(&p_dev_data->PeerInfoPool, p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }
  //                                                               Find the next place available.
  if (p_dev_data->PeerList == DEF_NULL) {
    p_dev_data->PeerList = p_buf;
    p_peer = p_dev_data->PeerList;
  } else {
    p_peer = p_dev_data->PeerList;
    while (p_peer->Next != DEF_NULL) {
      p_peer = p_peer->Next;
    }
    p_peer->Next = p_buf;
    p_peer = p_peer->Next;
  }

  p_peer->Next = DEF_NULL;                                      // Set the list element info.
  Mem_Copy(&p_peer->Info.HW_Addr, p_wmi_connect->bssid, NET_IF_802x_ADDR_SIZE);
}

/****************************************************************************************************//**
 *                                           NetDev_DeletePeerInfo()
 *
 * @brief    Remove a peer from the peer list.
 *
 * @param    p_if    Pointer to a network interface.
 *
 * @param    p_data  Pointer to a disconnect event with the peer info.
 *
 * @param    p_err   Pointer to variable  that will receive the return error code from this function.
 *******************************************************************************************************/
static void NetDev_PeerInfoDelete(NET_IF     *p_if,
                                  CPU_INT08U *p_data,
                                  RTOS_ERR   *p_err)
{
  NET_DEV_DATA         *p_dev_data;
  WMI_DISCONNECT_EVENT *p_wmi_disconnect;
  CPU_BOOLEAN          is_matched;
  NET_DEV_PEER_INFO    *p_peer;
  NET_DEV_PEER_INFO    *p_peer_prev;

  //                                                               ----------- OBTAIN REFERENCE TO DEV_DATA -----------
  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;
  p_wmi_disconnect = (WMI_DISCONNECT_EVENT *)NetDev_GetEventDataPtr(p_if, p_data);

  //                                                               Find the matching element.
  p_peer_prev = DEF_NULL;
  p_peer = p_dev_data->PeerList;
  is_matched = DEF_NO;
  while (p_peer != DEF_NULL) {
    is_matched = Mem_Cmp(p_peer->Info.HW_Addr, p_wmi_disconnect->bssid, NET_IF_802x_ADDR_SIZE);
    if (is_matched == DEF_YES) {
      break;
    }

    p_peer_prev = p_peer;
    p_peer = p_peer->Next;
  }
  //                                                               Remove of the list if found.
  if (is_matched == DEF_YES) {
    if (p_peer_prev == DEF_NULL) {
      p_dev_data->PeerList = p_peer->Next;
    } else {
      p_peer_prev->Next = p_peer->Next;
    }

    Mem_DynPoolBlkFree(&p_dev_data->PeerInfoPool,
                       p_peer,
                       p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
      RTOS_ASSERT_CRITICAL(RTOS_ERR_CODE_GET(*p_err) == RTOS_ERR_NONE, RTOS_ERR_CODE_GET(*p_err),; );
    }
  } else {
    RTOS_ERR_SET(*p_err, RTOS_ERR_IO);
  }
}

/****************************************************************************************************//**
 *                                           NetDev_GetPeerInfo()
 *
 * @brief    Get all the information of the active peers in the the peer list..
 *
 * @param    p_if            Pointer to a network interface.
 *
 * @param    p_data          Pointer to return the information of the active peer.=s.
 *
 * @param    buf_max_len     The maximum buffer length of p_data
 *
 * @param    p_err           Pointer to variable that will receive the return error code from this function.
 *
 * @return   Number of active peer in the peer list.
 *******************************************************************************************************/
static CPU_INT16U NetDev_PeerInfoGet(NET_IF     *p_if,
                                     CPU_INT08U *p_data,
                                     CPU_INT16U buf_max_len,
                                     RTOS_ERR   *p_err)
{
  NET_IF_WIFI_PEER  *p_rtn;
  CPU_INT16U        ctn;
  CPU_INT16U        nb_max_peer;
  NET_DEV_DATA      *p_dev_data;
  NET_DEV_PEER_INFO *p_peer;

  //                                                               ----------- OBTAIN REFERENCE TO DEV_DATA -----------
  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;

  ctn = 0u;
  p_rtn = (NET_IF_WIFI_PEER *)p_data;
  nb_max_peer = buf_max_len / sizeof(NET_IF_WIFI_PEER);
  p_peer = p_dev_data->PeerList;

  while (p_peer != DEF_NULL) {
    Mem_Copy(&p_rtn[ctn], &p_peer->Info, sizeof(NET_IF_WIFI_PEER));
    ctn++;
    //                                                             Stop if we have reach the buffer capacity.
    if (ctn >= nb_max_peer) {
      break;
    }
    p_peer = p_peer->Next;
  }

  return ctn;
}

/****************************************************************************************************//**
 *                                           NetDev_GetEventDataPtr()
 *
 * @brief    Get the pointer to the event data from the a net buffer.
 *
 * @param    p_if    Pointer to a network interface.
 *
 * @param    p_buf   Pointer to the buffer to parse.
 *
 * @return   Pointer to the event data
 *******************************************************************************************************/
static CPU_INT08U *NetDev_GetEventDataPtr(NET_IF     *p_if,
                                          CPU_INT08U *p_buf)
{
  CPU_INT08U       *p_event;
  CPU_INT08U       *p_data;
  NET_DEV_CFG_WIFI *p_dev_cfg;

  //                                                               ----------- OBTAIN REFERENCE TO DEV CFG ------------
  p_dev_cfg = (NET_DEV_CFG_WIFI *)p_if->Dev_Cfg;                // Obtain ptr to dev cfg area.

  p_data = p_buf + p_dev_cfg->RxBufIxOffset - NET_DEV_ATHEROS_PACKET_HEADER;
  p_event = p_data + HTC_HDR_LENGTH + sizeof(WMI_CMD_HDR);

  return (p_event);
}

/********************************************************************************************************
 ********************************************************************************************************
 *                                       DEPENDENCIES & AVAIL CHECK(S)
 ********************************************************************************************************
 *******************************************************************************************************/

#endif // RTOS_MODULE_NET_IF_WIFI_AVAIL
