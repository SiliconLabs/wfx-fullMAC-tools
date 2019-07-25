/***************************************************************************//**
 * @file
 * @brief Network Device Driver - Rs9110-N-2X
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
 * @note     (1) Device driver compatible with these hardware:
 *
 *               (a) RS9110-N-22
 *               (b) RS9110-N-23
 *
 * @note     (2) Redpine firmware V4.7.1 command:
 *              - (a) Band.
 *                   - (1) RS9110-N-22 support only band 2.4 Ghz.
 *                   - (2) RS9110-N-23 support band 2.4 Ghz and 5Ghz
 *                   The band is set when the device is started (NetDev_Start()). Currently it's NOT possible to
 *                   change the band after the start is completed.
 *              - (b) The initialization command is sent when the device is started (NetDev_Start()).
 *              - (c) Scan is FULLY supported.
 *              - (d) Join
 *              - (c) Query MAC address and Netowrk Type of Scanned Networks is NOT currently supported.
 *              - (d) Scan
 *              - (e) Power Save
 *              - (f) Query Connection status
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
#include  <drivers/net/include/net_drv_wifi.h>

#include  <net/source/tcpip/net_if_priv.h>
#include  <net/source/tcpip/net_wifi_mgr_priv.h>

#include  <io/include/spi.h>
#include  <io/include/spi_slave.h>

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

#define  LOG_DFLT_CH                        (NET)
#define  RTOS_MODULE_CUR                     RTOS_CFG_MODULE_NET

#define  NET_DEV_MGMT_NONE                                 0u
#define  NET_DEV_MGMT_BOOT                               100u
#define  NET_DEV_MGMT_FIRMWARE_START                     101u
#define  NET_DEV_MGMT_FIRMWARE_UPGRADE                   102u
#define  NET_DEV_MGMT_FIRMWARE_LOAD_FF_TAIM1             103u
#define  NET_DEV_MGMT_FIRMWARE_LOAD_FF_TAIM2             104u
#define  NET_DEV_MGMT_FIRMWARE_LOAD_FF_DATA              105u
#define  NET_DEV_MGMT_FIRMWARE_GET_VERSION               106u
#define  NET_DEV_MGMT_SET_PWR_SAVE                       107u
#define  NET_DEV_MGMT_GET_HW_ADDR                        108u
#define  NET_DEV_MGMT_SET_HW_ADDR                        109u
#define  NET_DEV_MGMT_SET_SPD_MODE                       110u
#define  NET_DEV_MGMT_BAND                               111u
#define  NET_DEV_MGMT_STACK_BYPASS                       112u

#define  NET_DEV_CTRL_CMD_GET_FIRMWARE                  0x0D
#define  NET_DEV_CTRL_CMD_SET_HW_ADDR                   0x0E
#define  NET_DEV_CTRL_CMD_GET_HW_ADDR                   0x0F
#define  NET_DEV_CTRL_CMD_LEAVE                         0x08
#define  NET_DEV_CTRL_CMD_GET_STATUS                    0x07

#define  NET_DEV_RX_BUF_SPI_OFFSET_OCTETS                  4u
#define  NET_DEV_SPI_CLK_FREQ_MIN                          1u
#define  NET_DEV_SPI_CLK_FREQ_MAX                   80000000L

#define  NET_DEV_CMD_INIT_SPI                           0x15
#define  NET_DEV_CMD_RD_WR                              0x40
#define  NET_DEV_CMD_WR                                 0x20
#define  NET_DEV_CMD_AHB_BUS_ACCESS                     0x10
#define  NET_DEV_CMD_AHB_SLAVE                          0x08
#define  NET_DEV_CMD_WR_RD_32_BIT                       0x40
#define  NET_DEV_CMD_TRANSFER_LEN_16_BITS               0x04
#define  NET_DEV_CMD_TRANSFER_LEN_2_BYTES               0x02

#define  NET_DEV_TOKEN_START                            0x55
#define  NET_DEV_TOKEN_MAX_RETRY                       26000u

#define  NET_DEV_STATUS_SUCCESS                         0x58
#define  NET_DEV_STATUS_BUSY                            0x54
#define  NET_DEV_STATUS_FAIL                            0x52

#define  NET_DEV_INT_TYPE_MASK_BUF_FULL                 DEF_BIT_00
#define  NET_DEV_INT_TYPE_MASK_BUF_EMPTY                DEF_BIT_01
#define  NET_DEV_INT_TYPE_MASK_MGMT                     DEF_BIT_02
#define  NET_DEV_INT_TYPE_MASK_DATA                     DEF_BIT_03
#define  NET_DEV_INT_TYPE_MASK_SLEEP                    DEF_BIT_04
#define  NET_DEV_INT_TYPE_MASK_WAKE_UP                  DEF_BIT_05

#define  NET_DEV_RST_CLR                                   0u
#define  NET_DEV_RST_SET                                   1u
#define  NET_DEV_RST_ADDR                         0x22000004

#define  NET_DEV_SOFT_BLK_SIZE                           256u

#define  NET_DEV_SOFT_ADDR_SB_IM1                 0x00000000
#define  NET_DEV_SOFT_ADDR_SB_IM2                 0x02014010
#define  NET_DEV_SOFT_ADDR_SB_DM1                 0x20003100
#define  NET_DEV_SOFT_ADDR_SB_DM2                 0x0200F800

#define  NET_DEV_SOFT_ADDR_IU_IM1                 0x00000000
#define  NET_DEV_SOFT_ADDR_IU_IM2                 0x02000000
#define  NET_DEV_SOFT_ADDR_IU_DM                  0x20003100

#define  NET_DEV_SOFT_ADDR_IU                     0x02008000

#define  NET_DEV_REG_INT                                0x00
#define  NET_DEV_REG_SPI_MODE                           0x04
#define  NET_DEV_REG_DEV_ID                             0x0B
#define  NET_DEV_REG_VERSION                            0x0C

#define  NET_DEV_HDR_SIZE                                 12u
#define  NET_DEV_RX_BUF_LEN_MAX                         1600u

#define  NET_DEV_RESP_CTRL                              0x44
#define  NET_DEV_RESP_TAIM1                             0x91
#define  NET_DEV_RESP_TAIM2                             0x92
#define  NET_DEV_RESP_TADM                              0x93
#define  NET_DEV_RESP_CARD_RDY                          0x89
#define  NET_DEV_RESP_INIT                              0x94
#define  NET_DEV_RESP_SCAN                              0x95
#define  NET_DEV_RESP_JOIN                              0x96
#define  NET_DEV_RESP_LEAVE                             0x
#define  NET_DEV_RESP_PWR_SAVE                          0x85
#define  NET_DEV_RESP_WAKEUP                            0x8A
#define  NET_DEV_RESP_STACK_BYPASS                      0xA3

#define  NET_DEV_FRAME_DATA                             0x02
#define  NET_DEV_FRAME_MGMT                             0x04

#define  NET_DEV_DESC_SIZE                                16u

#define  NET_DEV_DESC_INIT                            0x1000
#define  NET_DEV_DESC_GET_MAC                         0x0000
#define  NET_DEV_DESC_GET_FIRMWARE_VERSION            0x0000
#define  NET_DEV_DESC_SCAN                            0x1100
#define  NET_DEV_DESC_JOIN                            0x1200
#define  NET_DEV_DESC_PWR_SAVE                        0x0200
#define  NET_DEV_DESC_UPGRADE_FF_TAIM1                0x1300
#define  NET_DEV_DESC_UPGRADE_FF_TAIM2                0x1400
#define  NET_DEV_DESC_UPGRADE_FF_DATA                 0x1500
#define  NET_DEV_DESC_TCPIP_BYPASS                    0x2D00

#define  NET_DEV_DESC_Q_MASK                            0x80

#define  NET_DEV_PART_FF_TAIM1                             1u
#define  NET_DEV_PART_FF_TAIM2                             2u
#define  NET_DEV_PART_FF_TADM                              3u

//                                                                 Queue Addresses
#define  NET_DEV_Q_LMAC                                 0x01
#define  NET_DEV_Q_DATA                                 0x02
#define  NET_DEV_Q_MGMT                                 0x04

#define  NET_DEV_Q_MASK                                 0x07

#define  NET_DEV_AP_PER_SCAN_MAX                          32u

#define  NET_DEV_JOIN_NET_INFRASTRUCT                      1u
#define  NET_DEV_JOIN_NET_ADHOC                            0u

#define  NET_DEV_JOIN_DATA_RATE_AUTO                       0u
#define  NET_DEV_JOIN_DATA_RATE_1_MBPS                     1u
#define  NET_DEV_JOIN_DATA_RATE_2_MBPS                     2u
#define  NET_DEV_JOIN_DATA_RATE_5_5_MBPS                   3u
#define  NET_DEV_JOIN_DATA_RATE_11_MBPS                    4u
#define  NET_DEV_JOIN_DATA_RATE_6_MBPS                     5u
#define  NET_DEV_JOIN_DATA_RATE_9_MBPS                     6u
#define  NET_DEV_JOIN_DATA_RATE_12_MBPS                    7u
#define  NET_DEV_JOIN_DATA_RATE_18_MBPS                    8u
#define  NET_DEV_JOIN_DATA_RATE_24_MBPS                    9u
#define  NET_DEV_JOIN_DATA_RATE_36_MBPS                   10u
#define  NET_DEV_JOIN_DATA_RATE_48_MBPS                   11u
#define  NET_DEV_JOIN_DATA_RATE_54_MBPS                   12u
#define  NET_DEV_JOIN_DATA_RATE_MCS0                      13u
#define  NET_DEV_JOIN_DATA_RATE_MCS1                      14u
#define  NET_DEV_JOIN_DATA_RATE_MCS2                      15u
#define  NET_DEV_JOIN_DATA_RATE_MCS3                      16u
#define  NET_DEV_JOIN_DATA_RATE_MCS4                      17u
#define  NET_DEV_JOIN_DATA_RATE_MCS5                      18u
#define  NET_DEV_JOIN_DATA_RATE_MCS6                      19u
#define  NET_DEV_JOIN_DATA_RATE_MCS7                      20u

#define  NET_DEV_JOIN_PWR_LEVEL_LO                         0u
#define  NET_DEV_JOIN_PWR_LEVEL_MED                        1u
#define  NET_DEV_JOIN_PWR_LEVEL_HI                         2u

#define  NET_DEV_JOIN_SECURITY_OPEN                        0u
#define  NET_DEV_JOIN_SECURITY_WPA                         1u
#define  NET_DEV_JOIN_SECURITY_WPA2                        2u
#define  NET_DEV_JOIN_SECURITY_WEP                         3u

#define  NET_DEV_FIRMWARE_VERSION_STR                 "4.7.1"
#define  NET_DEV_FIRMWARE_VERSION_STR_LEN                  6u

#define  NET_DEV_IBSS_MODE_JOIN                            0u
#define  NET_DEV_IBSS_MODE_CREATE                          1u

#define  NET_DEV_MGMT_CMD_OCTET                         0x55
#define  NET_DEV_MGMT_QUERY_OCTET                       0x44

#define  NET_DEV_MGMT_RESP_COMMON_TIMEOUT_MS           20000
#define  NET_DEV_MGMT_RESP_FIRMWARE_BOOT_TIMEOUT_MS    60000
#define  NET_DEV_MGMT_RESP_FIRMWARE_LOAD_TIMEOUT_MS    60000
#define  NET_DEV_MGMT_RESP_SCAN_TIMEOUT_MS             30000

#define  NET_DEV_SPI_XFER_TIMEOUT_MS                    5000

/********************************************************************************************************
 ********************************************************************************************************
 *                                           LOCAL DATA TYPES
 ********************************************************************************************************
 *******************************************************************************************************/

//                                                                 --------------- DEVICE INSTANCE DATA ---------------
typedef struct net_dev_data {
#if (NET_CTR_CFG_STAT_EN == DEF_ENABLED)
  NET_CTR           StatRxPktCtr;
  NET_CTR           StatTxPktCtr;
#endif
#if (NET_CTR_CFG_ERR_EN == DEF_ENABLED)
  NET_CTR           ErrRxPktDiscardedCtr;
  NET_CTR           ErrTxPktDiscardedCtr;
#endif

  CPU_INT08U        *GlobalBufPtr;
  CPU_BOOLEAN       DevPktBuffFull;

  CPU_INT08U        WaitRespType;
  CPU_INT08U        SubState;

  NET_IF_LINK_STATE LinkStatus;
  CPU_INT08U        LinkStatusQueryCtr;

  SPI_SLAVE_HANDLE  SPI_Handle;
} NET_DEV_DATA;

typedef struct net_dev_cmd {
  CPU_INT08U C1;
  CPU_INT08U C2;
  CPU_INT08U C3;
  CPU_INT08U C4;
} NET_DEV_CMD;

typedef struct net_dev_answer {
  CPU_INT08S A1;
  CPU_INT08S A2;
  CPU_INT08S A3;
  CPU_INT08S A4;
} NET_DEV_ANSWER;

typedef struct net_dev_mgmt_frame_desc {
  CPU_INT16U Word0;
  CPU_INT16U Word1;
  CPU_INT16U Word2;
  CPU_INT16U Word3;
  CPU_INT16U Word4;
  CPU_INT16U Word5;
  CPU_INT16U Word6;
  CPU_INT16U Word7;
} NET_DEV_MGMT_DESC;

typedef struct net_dev_mgmt_frame_scan {
  CPU_INT32U       Channel;
  NET_IF_WIFI_SSID SSID;
} NET_DEV_MGMT_FRAME_SCAN;

typedef struct net_dev_mgmt_frame_join {
  CPU_INT08U       NetworkType;
  CPU_INT08U       SecType;
  CPU_INT08U       DataRate;
  CPU_INT08U       PwrLevel;
  NET_IF_WIFI_PSK  PSK;
  NET_IF_WIFI_SSID SSID;
  CPU_INT08U       Mode;
  CPU_INT08U       Ch;
  CPU_INT08U       Action;
} NET_DEV_MGMT_FRAME_JOIN;

typedef struct net_dev_mgmt_frame_power_save {
  CPU_INT08U Mode;
  CPU_INT08U Interval;
} NET_DEV_MGMT_FRAME_PWR_SAVE;

typedef struct net_dev_mgmt_frame_stack_bypass {
  CPU_INT08U Mode;
} NET_DEV_MGMT_FRAME_STACK_BYPASS;

typedef struct net_dev_mgmt_frame_ctrl {
  CPU_INT08U Desc[16];
  CPU_INT32U SubType;
} NET_DEV_MGMT_FRAME_CTRL;

typedef struct net_dev_mgmt_scan_info {
  CPU_INT08U       Ch;
  CPU_INT08U       SecMode;
  CPU_INT08U       RSSID;
  NET_IF_WIFI_SSID SSID;
} NET_DEV_MGMT_SCAN_AP;

typedef struct net_dev_mgmt_scan_response {
  CPU_INT32U           ScanCnt;
  CPU_INT32U           Error;
  NET_DEV_MGMT_SCAN_AP APs[NET_DEV_AP_PER_SCAN_MAX];
} NET_DEV_MGMT_SCAN_RESP;

typedef union net_dev_mgmt_frame {
  NET_DEV_MGMT_FRAME_SCAN         Scan;
  NET_DEV_MGMT_FRAME_JOIN         Join;
  NET_DEV_MGMT_FRAME_PWR_SAVE     Pwr_Save;
  NET_DEV_MGMT_FRAME_STACK_BYPASS Stack_Bypass;
} NET_DEV_MGMT_FRAME;

typedef struct net_dev_mgmt_desc_frame {
  NET_DEV_MGMT_DESC  Desc;
  NET_DEV_MGMT_FRAME Frame;
} NET_DEV_MGMT_DESC_FRAME;

/********************************************************************************************************
 ********************************************************************************************************
 *                                       LOCAL GLOBAL VARIABLES
 ********************************************************************************************************
 *******************************************************************************************************/

/********************************************************************************************************
 *                                               SPI INFO
 *******************************************************************************************************/

static const SPI_SLAVE_INFO NetWiFi_SPI_SlaveInfo_RS9110N2x = {
  .Mode = 0,
  .BitsPerFrame = 8,
  .LSbFirst = DEF_NO,
  .SClkFreqMax = 25000000u,
  .SlaveID = 0,
  .TxDummyByte = 0xFFu,
  .ActiveLow = DEF_YES
};

/********************************************************************************************************
 *                                               FIRMWARE
 *******************************************************************************************************/

static const CPU_INT08U NetDev_sbtaim1[] = {
#include "firmware/sbinst1"
};
static const CPU_INT08U NetDev_sbtaim2[] = {
#include "firmware/sbinst2"
};

static const CPU_INT08U NetDev_sbtadm1[] = {
#include "firmware/sbdata1"
};
static const CPU_INT08U NetDev_sbtadm2[] = {
#include "firmware/sbdata2"
};

static const CPU_INT08U NetDev_iutaim1[] = {
#include "firmware/iuinst1"
};
static const CPU_INT08U NetDev_iutaim2[] = {
#include "firmware/iuinst2"
};
static const CPU_INT08U NetDev_iutadm[] = {
#include "firmware/iudata"
};

const CPU_INT08U NetDev_fftaim1[] = {
#include "firmware/ffinst1"
};
const CPU_INT08U NetDev_fftaim2[] = {
#include "firmware/ffinst2"
};
const CPU_INT08U NetDev_ffdata[] = {
#include "firmware/ffdata"
};

/********************************************************************************************************
 ********************************************************************************************************
 *                                                   MACRO'S
 ********************************************************************************************************
 *******************************************************************************************************/

#define  NET_DEV_LO_BYTE(val)          (MEM_VAL_HOST_TO_LITTLE_16(val) & 0x00ff)
#define  NET_DEV_HI_BYTE(val)         ((MEM_VAL_HOST_TO_LITTLE_16(val) & 0xff00) >> 8)

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

//                                                                 -------- FNCT'S COMMON TO ALL DEV'S --------
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

static void NetDev_InitSPI(NET_IF   *p_if,
                           RTOS_ERR *p_err);

static CPU_INT08U NetDev_RdIntReg(NET_IF   *p_if,
                                  RTOS_ERR *p_err);

static void NetDev_MgmtExecuteCmdFirmwareBoot(NET_IF      *p_if,
                                              CPU_BOOLEAN upgrade,
                                              RTOS_ERR    *p_err);

static void NetDev_MgmtExecuteCmdFirmwareUpgrade(NET_IF     *p_if,
                                                 CPU_INT08U part,
                                                 RTOS_ERR   *p_err);

static void NetDev_MgmtExecuteCmdCtrlQuery(NET_IF     *p_if,
                                           CPU_INT08U cmd,
                                           RTOS_ERR   *p_err);

static void NetDev_MgmtExecuteCmdFirmwareStart(NET_IF   *p_if,
                                               RTOS_ERR *p_err);

static void NetDev_MgmtExecuteCmdScan(NET_IF           *p_if,
                                      NET_IF_WIFI_SCAN *p_scan,
                                      RTOS_ERR         *p_err);

static CPU_INT32U NetDev_MgmtProcessRespScan(NET_IF     *p_if,
                                             CPU_INT08U *p_frame,
                                             CPU_INT16U frame_len,
                                             CPU_INT08U *p_ap_buf,
                                             CPU_INT16U buf_len,
                                             RTOS_ERR   *p_err);

static void NetDev_MgmtExecuteCmdJoin(NET_IF             *p_if,
                                      NET_IF_WIFI_AP_CFG *p_join,
                                      CPU_BOOLEAN        create_adhoc,
                                      RTOS_ERR           *p_err);

static void NetDev_MgmtExecuteCmdStackBypass(NET_IF   *p_if,
                                             RTOS_ERR *p_err);

static void NetDev_WrMgmtCmd(NET_IF                  *p_if,
                             CPU_INT16U              mgmt_cmd,
                             CPU_BOOLEAN             send_frame,
                             NET_DEV_MGMT_DESC_FRAME *p_mgmt,
                             RTOS_ERR                *p_err);

static void NetDev_WrMem(NET_IF     *p_if,
                         CPU_INT32U reg_addr,
                         CPU_INT16U data_len,
                         CPU_INT08U *p_wr_data,
                         CPU_INT08U *p_rd_data,
                         RTOS_ERR   *p_err);

static void NetDev_WrData(NET_IF     *p_if,
                          CPU_INT08U addr,
                          CPU_INT16U data_len,
                          CPU_INT08U *p_wr_data,
                          CPU_INT08U *p_rd_data,
                          RTOS_ERR   *p_err);

static void NetDev_RdData(NET_IF     *p_if,
                          CPU_INT08U *p_buf,
                          CPU_INT16U *p_size,
                          CPU_INT08U *p_queue,
                          CPU_INT08U *p_type,
                          RTOS_ERR   *p_err);

static void NetDev_Rd(NET_IF     *p_if,
                      CPU_INT32U addr,
                      CPU_INT08U *p_data_wr,
                      CPU_INT08U *p_data_rd,
                      CPU_INT16U data_len,
                      RTOS_ERR   *p_err);

static void NetDev_RdStartToken(NET_IF   *p_if,
                                RTOS_ERR *p_err);

/********************************************************************************************************
 ********************************************************************************************************
 *                                           GLOBAL VARIABLES
 ********************************************************************************************************
 *******************************************************************************************************/

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

const NET_DEV_API_WIFI NetDev_API_RS9110N2x = {                 // WiFi dev API fnct ptrs :
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

/********************************************************************************************************
 *                                           PART HARDWARE INFO
 *******************************************************************************************************/

const NET_IF_WIFI_PART_INFO NetWiFi_Info_RS9110N2x = {
  .DrvAPI_Ptr = &NetDev_API_RS9110N2x,
  .RxBufIxOffset = 4u,
  .TxBufIxOffset = 0u,
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
 *               - (a) Initialize required clock sources
 *               - (b) Initialize external interrupt controller
 *               - (c) Initialize external GPIO controller
 *               - (d) Initialize driver state variables
 *               - (e) Initialize driver statistics & error counters
 *               - (f) Allocate memory for device DMA descriptors
 *               - (g) Initialize additional device registers
 *                   - (1) (R)MII mode / Phy bus type
 *                   - (2) Disable device interrupts
 *                   - (3) Disable device receiver and transmitter
 *                   - (4) Other necessary device initialization
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
 * @note     (8) Drivers SHOULD validate device configuration values and set *perr to
 *               NET_DEV_ERR_INVALID_CFG if unacceptible values have been specified. Fields
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
 *               - (c) Both the receiver and transmitter disabled.
 *******************************************************************************************************/
static void NetDev_Init(NET_IF   *p_if,
                        RTOS_ERR *p_err)
{
  NET_IF_WIFI_HW_INFO *p_hw_info;
  NET_DEV_BSP_WIFI    *p_dev_bsp;
  NET_DEV_CFG_WIFI    *p_dev_cfg;
  NET_DEV_DATA        *p_dev_data;
  NET_BUF_SIZE        buf_rx_size_max = 0;
  CPU_SIZE_T          reqd_octets;
  CPU_INT16U          buf_size_max;
  SPI_BUS_HANDLE      bus_handle;
  SPI_SLAVE_INFO      slave_info;

  //                                                               -------- OBTAIN REFERENCE TO CFGs/REGs/BSP ---------
  p_dev_cfg = (NET_DEV_CFG_WIFI *)p_if->Dev_Cfg;
  p_dev_bsp = (NET_DEV_BSP_WIFI *)p_if->Dev_BSP;
  p_hw_info = (NET_IF_WIFI_HW_INFO *)p_if->Ext_Cfg;

  //                                                               --------------- VALIDATE DEVICE CFG ----------------
  RTOS_ASSERT_DBG_ERR_SET((p_dev_cfg->RxBufAlignOctets != 0u), *p_err, RTOS_ERR_INVALID_CFG,; );
  RTOS_ASSERT_DBG_ERR_SET((p_dev_cfg->RxBufAlignOctets == p_dev_cfg->TxBufAlignOctets), *p_err, RTOS_ERR_INVALID_CFG,; );

#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
  buf_rx_size_max = NetBuf_GetMaxSize(p_if->Nbr,
                                      NET_TRANSACTION_RX,
                                      DEF_NULL,
                                      NET_IF_IX_RX);
  if (buf_rx_size_max < NET_IF_802x_FRAME_MAX_SIZE) {
    RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_CFG,; );
  }
#endif

  PP_UNUSED_PARAM(buf_rx_size_max);

  //                                                               -------------- ALLOCATE DEV DATA AREA --------------
  p_if->Dev_Data = Mem_SegAlloc("Device data",
                                DEF_NULL,
                                sizeof(NET_DEV_DATA),
                                p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }

  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;

  buf_size_max = DEF_MAX(p_dev_cfg->RxBufLargeSize, p_dev_cfg->TxBufLargeSize);
  p_dev_data->GlobalBufPtr = (CPU_INT08U *)Mem_SegAllocHW("Global Buffer",
                                                          DEF_NULL,
                                                          buf_size_max,
                                                          p_dev_cfg->RxBufAlignOctets,
                                                          &reqd_octets,
                                                          p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }

  //                                                               ------- INITIALIZE EXTERNAL GPIO CONTROLLER --------
  p_dev_bsp->CfgGPIO(p_if, p_err);                              // Configure Wireless Controller GPIO (see Note #2).
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }

  bus_handle = SPI_BusHandleGetFromName((CPU_CHAR const *)p_hw_info->IO_BusNamePtr);
  if (bus_handle == SPI_BusHandleNull) {
    RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
    return;
  }

  slave_info = NetWiFi_SPI_SlaveInfo_RS9110N2x;
  slave_info.SlaveID = p_hw_info->IO_SlaveID;

  p_dev_data->SPI_Handle = SPI_SlaveOpen(bus_handle,
                                         &slave_info,
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

  //                                                               ---- INITIALIZE ALL DEVICE DATA AREA VARIABLES -----
  p_dev_data->DevPktBuffFull = DEF_NO;
#if (NET_CTR_CFG_STAT_EN == DEF_ENABLED)
  p_dev_data->StatRxPktCtr = 0;
  p_dev_data->StatTxPktCtr = 0;
#endif

#if (NET_CTR_CFG_ERR_EN == DEF_ENABLED)
  p_dev_data->ErrRxPktDiscardedCtr = 0;
  p_dev_data->ErrTxPktDiscardedCtr = 0;
#endif
}

/****************************************************************************************************//**
 *                                               NetDev_Start()
 *
 * @brief    (1) Start network interface hardware :
 *               - (a) Initialize transmit semaphore count
 *               - (b) Start      Wireless device
 *               - (c) Initialize Wireless device
 *               - (d) Validate   Wireless Firmware version
 *                   - (1) Boot Wireless device to upgrade firmware
 *                   - (2) Load Firmware
 *                   - (3) Reset Wireless device
 *               - (d) Boot       Wireless firmware
 *               - (e) Initialize Wireless firmware MAC layer
 *               - (f) Configure hardware address
 *
 * @param    p_if    Pointer to a network interface.
 *
 * @param    p_err   Pointer to variable that will receive the return error code from this function.
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
 *               - (a) Device Configuration Structure        Configure a valid HW address during
 *                                                           compile time.
 *                                                           Configure either "00:00:00:00:00:00" or
 *                                                           an empty string, "", in order to
 *                                                           configure the HW address using
 *                                                           method (b).
 *               - (b) NetIF_802x_HW_AddrSet()               Call NetIF_802x_HW_AddrSet() if the HW
 *                                                           address needs to be configured via
 *                                                           run-time from a different data
 *                                                           source. E.g. Non auto-loading
 *                                                           memory such as I2C or SPI EEPROM
 *                                                           (see Note #3).
 *               - (c) Auto-Loading via EEPROM               If neither options a) or b) are used,
 *                                                           the IF layer will use the HW address
 *                                                           obtained from the network hardware
 *                                                           address registers.
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
 *******************************************************************************************************/
static void NetDev_Start(NET_IF   *p_if,
                         RTOS_ERR *p_err)
{
  NET_DEV_BSP_WIFI *p_dev_bsp;
  NET_WIFI_MGR_API *p_mgr_api;
  NET_DEV_DATA     *p_dev_data;
  NET_IF_DATA_WIFI *p_if_data;
  CPU_INT08U       hw_addr[NET_IF_802x_ADDR_SIZE];
  CPU_INT08U       hw_addr_len;
  CPU_INT16U       len;
  CPU_INT16S       cmp;
  CPU_CHAR         firmware_version[NET_DEV_FIRMWARE_VERSION_STR_LEN];
  CPU_BOOLEAN      firmware_upgrade;
  CPU_BOOLEAN      hw_addr_cfg;
  RTOS_ERR         local_err;

  //                                                               ----------- OBTAIN REFERENCE TO MGR/BSP ------------
  p_mgr_api = (NET_WIFI_MGR_API *)p_if->Ext_API;                // Obtain ptr to the mgr api struct.
  p_dev_bsp = (NET_DEV_BSP_WIFI *)p_if->Dev_BSP;                // Obtain ptr to the bsp api struct.
  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;                  // Obtain ptr to dev data area.
  p_if_data = (NET_IF_DATA_WIFI *)p_if->IF_Data;

  RTOS_ASSERT_DBG_ERR_SET((p_if_data->Band == NET_DEV_BAND_2_4_GHZ), *p_err, RTOS_ERR_INVALID_CFG,; );

  p_dev_bsp->Stop(p_if, p_err);                                 // Power down.
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }
  //                                                               -------------- START WIRELESS DEVICE ---------------
  p_dev_bsp->Start(p_if, p_err);                                // Power up.
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }

  //                                                               -------------- EN EXTERNAL INTERRUPT ---------------
  p_dev_bsp->IntCtrl(p_if, DEF_YES, p_err);                     // En ext int needed to receive mgmt resp.
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }

  //                                                               ------------ INITIALIZE WIRELESS DEVICE ------------
  NetDev_InitSPI(p_if, p_err);                                  // Init Dev SPI.
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }

  firmware_upgrade = DEF_NO;

  //                                                               When Timeout on WIFI Device Response occurs.
  //                                                               Command is not recognized by the current...
  //                                                               ...firmware so we downgrade or upgrade the firmware.

  p_mgr_api->Mgmt(p_if,                                         // Boot firmware.
                  NET_DEV_MGMT_BOOT,
                  (CPU_INT08U *)&firmware_upgrade,
                  sizeof(firmware_upgrade),
                  DEF_NULL,
                  0,
                  p_err);
  switch (RTOS_ERR_CODE_GET(*p_err)) {
    case RTOS_ERR_NONE:
      p_mgr_api->Mgmt(p_if,                                     // Send Stack Bypass command.
                      NET_DEV_MGMT_STACK_BYPASS,
                      DEF_NULL,
                      0,
                      DEF_NULL,
                      0,
                      p_err);
      switch (RTOS_ERR_CODE_GET(*p_err)) {
        case RTOS_ERR_NONE:
          p_mgr_api->Mgmt(p_if,                                 // Start dev Firmware.
                          NET_DEV_MGMT_FIRMWARE_START,
                          DEF_NULL,
                          0,
                          DEF_NULL,
                          0,
                          p_err);
          switch (RTOS_ERR_CODE_GET(*p_err)) {
            case RTOS_ERR_NONE:
              //                                                   Get current firmware version.
              p_mgr_api->Mgmt(p_if,
                              NET_DEV_MGMT_FIRMWARE_GET_VERSION,
                              DEF_NULL,
                              0,
                              (CPU_INT08U *)firmware_version,
                              sizeof(firmware_version),
                              p_err);
              switch (RTOS_ERR_CODE_GET(*p_err)) {
                case RTOS_ERR_NONE:
                  //                                               Valid the firmware version.
                  cmp = Str_Cmp_N(NET_DEV_FIRMWARE_VERSION_STR,
                                  firmware_version,
                                  NET_DEV_FIRMWARE_VERSION_STR_LEN);
                  if (cmp != 0) {
                    firmware_upgrade = DEF_YES;
                  }
                  break;

                case RTOS_ERR_TIMEOUT:
                  firmware_upgrade = DEF_YES;
                  break;

                default:
                  return;
              }
              break;

            case RTOS_ERR_TIMEOUT:
              firmware_upgrade = DEF_YES;
              break;

            default:
              return;
          }
          break;

        case RTOS_ERR_TIMEOUT:
          firmware_upgrade = DEF_YES;
          break;

        default:
          return;
      }
      break;

    case RTOS_ERR_TIMEOUT:
      firmware_upgrade = DEF_YES;
      break;

    default:
      return;
  }

  if (firmware_upgrade == DEF_YES) {                            // If current firmware is not compatible.
                                                                // Restart device.
    p_dev_bsp->Stop(p_if, p_err);                               // Power down.
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
      return;
    }

    p_dev_bsp->Start(p_if, p_err);                              // Power up.
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
      return;
    }

    NetDev_InitSPI(p_if, p_err);                                // Init Dev SPI.
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
      return;
    }

    p_mgr_api->Mgmt(p_if,                                       // Boot for firmware update.
                    NET_DEV_MGMT_BOOT,
                    (CPU_INT08U *)&firmware_upgrade,
                    sizeof(firmware_upgrade),
                    DEF_NULL,
                    0,
                    p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
      return;
    }

    p_mgr_api->Mgmt(p_if,                                       // Load firmware.
                    NET_DEV_MGMT_FIRMWARE_UPGRADE,
                    DEF_NULL,
                    0,
                    DEF_NULL,
                    0,
                    p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
      return;
    }

    //                                                             Restart device.
    p_dev_bsp->Stop(p_if, p_err);                               // Power down.
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
      return;
    }

    p_dev_bsp->Start(p_if, p_err);                              // Power up.
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
      return;
    }

    NetDev_InitSPI(p_if, p_err);                                // Init Dev SPI.
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
      return;
    }

    firmware_upgrade = DEF_NO;
    p_mgr_api->Mgmt(p_if,                                       // Boot firmware.
                    NET_DEV_MGMT_BOOT,
                    (CPU_INT08U *)&firmware_upgrade,
                    sizeof(firmware_upgrade),
                    DEF_NULL,
                    0,
                    p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
      return;
    }

    p_mgr_api->Mgmt(p_if,                                       // Send Stack Bypass command.
                    NET_DEV_MGMT_STACK_BYPASS,
                    DEF_NULL,
                    0,
                    DEF_NULL,
                    0,
                    p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
      return;
    }

    p_mgr_api->Mgmt(p_if,                                       // Start dev Firmware.
                    NET_DEV_MGMT_FIRMWARE_START,
                    DEF_NULL,
                    0,
                    DEF_NULL,
                    0,
                    p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
      return;
    }
  }
  //                                                               ------------ CONFIGURE HARDWARE ADDRESS ------------
  hw_addr_cfg = DEF_NO;                                         // See Notes #3 & #4.

  RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

  hw_addr_len = sizeof(hw_addr);
  NetIF_AddrHW_GetHandler(p_if->Nbr,
                          &hw_addr[0],
                          &hw_addr_len,
                          &local_err);
  if (RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE) {          // Check if the IF HW address has been user configured.
    hw_addr_cfg = NetIF_AddrHW_IsValidHandler(p_if->Nbr, &hw_addr[0], &local_err);
  } else {
    hw_addr_cfg = DEF_NO;
  }

  if (hw_addr_cfg != DEF_YES) {                                 // If NOT valid, attempt to use automatically ...
                                                                // ... loaded HW MAC address (see Note #4c).
    len = sizeof(hw_addr);
    p_mgr_api->Mgmt(p_if,                                       // Attempt to use automatically loaded HW address.
                    NET_DEV_MGMT_GET_HW_ADDR,
                    DEF_NULL,
                    0,
                    hw_addr,
                    len,
                    p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
      return;
    }

    NetIF_AddrHW_SetHandler(p_if->Nbr,                          // See Note #4a.
                            hw_addr,
                            sizeof(hw_addr),
                            p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
      return;
    }
  }

  if (hw_addr_cfg == DEF_YES) {                                 // If necessary, set device HW MAC address.
    //                                                             TODO_NET : Setting the MAC address is currently not supported.
  }

  //                                                               ------- INITIALIZE TRANSMIT SEMAPHORE COUNT --------
  NetIF_DevCfgTxRdySignal(p_if, 1);                             // See Note #2.

  p_dev_data->LinkStatusQueryCtr = 0;
  p_dev_data->LinkStatus = NET_IF_LINK_DOWN;
}

/****************************************************************************************************//**
 *                                               NetDev_Stop()
 *
 * @brief    (1) Shutdown network interface hardware :
 *               - (a) Disable the receiver and transmitter
 *               - (b) Disable receive and transmit interrupts
 *               - (c) Clear pending interrupt requests
 *               - (d) Free ALL receive descriptors (Return ownership to hardware)
 *               - (e) Deallocate ALL transmit buffers
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
 *
 * @note     (3) Device drivers should assume that the network interface transmit deallocation
 *               queue is large enough to post all currently-used transmit buffers.
 *               - (a) If the transmit deallocation queue is NOT large enough to post all transmit
 *                     buffers, some transmit buffers may/will be leaked/lost.
 *******************************************************************************************************/
static void NetDev_Stop(NET_IF   *p_if,
                        RTOS_ERR *p_err)
{
  NET_DEV_BSP_WIFI *p_dev_bsp;

  //                                                               -------- OBTAIN REFERENCE TO CFGs/REGs/BSP ---------
  p_dev_bsp = (NET_DEV_BSP_WIFI *)p_if->Dev_BSP;                // Obtain ptr to the bsp api struct.

  p_dev_bsp->Stop(p_if, p_err);                                 // Power down.
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }
}

/****************************************************************************************************//**
 *                                               NetDev_Rx()
 *
 * @brief    (1) This function returns a pointer to the received data to the caller :
 *               - (a) Acquire   SPI Lock
 *               - (b) Enable    SPI Chip Select
 *               - (c) Configure SPI Controller
 *               - (d) Determine what caused the interrupt
 *               - (e) Obtain pointer to data area to replace existing data area
 *               - (f) Read data from the SPI
 *               - (g) Set packet type
 *               - (h) Set return values, pointer to received data area and size
 *               - (i) Increment counters.
 *               - (j) Disable SPI Chip Select
 *               - (k) Release SPI lock
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
 * @note     (3) If a receive error occurs, the function SHOULD return 0 for the size, a
 *               NULL pointer to the data area AND an error code equal to NET_DEV_ERR_RX.
 *               Some devices may require that driver instruct the hardware to drop the
 *               frame if it has been commited to internal device memory.
 *               - (a) If a new data area is unavailable, the driver MUST instruct hardware
 *                     to discard the frame.
 *
 * @note     (4) Reading data from the device hardware may occur in various sized reads :
 *               - (a) Device drivers that require read sizes equivalent to the size of the
 *                     device data bus MAY examine pdev_cfg->DataBusSizeNbrBits in order to
 *                     determine the number of required data reads.
 *               - (b) Devices drivers that require read sizes equivalent to the size of the
 *                     Rx FIFO width SHOULD use the known FIFO width to determine the number
 *                     of required data reads.
 *               - (c) It may be necessary to round the number of data reads up, OR perform the
 *                     last data read outside of the loop.
 *
 * @note     (5) A pointer set equal to pbuf_new and sized according to the required data
 *               read size determined in (4) should be used to read data from the device
 *               into the receive buffer.
 *
 * @note     (6) Some devices may interrupt only ONCE for a recieved frame.  The driver MAY need
 *               check if additional frames have been received while processing the current
 *               received frame.  If additional frames have been received, the driver MAY need
 *               to signal the receive task before exiting NetDev_Rx().
 *******************************************************************************************************/
static void NetDev_Rx(NET_IF     *p_if,
                      CPU_INT08U **p_data,
                      CPU_INT16U *p_size,
                      RTOS_ERR   *p_err)
{
  NET_IF_WIFI_FRAME_TYPE *p_frame_type;
  CPU_INT08U             int_reg;
  CPU_INT08U             *p_buf;
  CPU_INT08U             from_q;

  //                                                               ------------- READ INTERRUPT REGISTER --------------
  int_reg = NetDev_RdIntReg(p_if, p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    *p_size = 0;
    *p_data = DEF_NULL;
    return;
  }

  //                                                               ------------- CHECK FOR RECEIVE ERRORS -------------
  if ((int_reg & NET_DEV_INT_TYPE_MASK_WAKE_UP) > 0) {
    RTOS_ERR_SET(*p_err, RTOS_ERR_RX);
  }

  if ((int_reg & NET_DEV_INT_TYPE_MASK_MGMT) > 0) {
    RTOS_ERR_SET(*p_err, RTOS_ERR_RX);
  }

  if ((int_reg & NET_DEV_INT_TYPE_MASK_BUF_EMPTY) > 0) {
    RTOS_ERR_SET(*p_err, RTOS_ERR_RX);
  }

  if ((int_reg & NET_DEV_INT_TYPE_MASK_BUF_FULL) > 0) {
    RTOS_ERR_SET(*p_err, RTOS_ERR_RX);
  }

  if ((int_reg & NET_DEV_INT_TYPE_MASK_DATA) > 0) {
    //                                                             ----------- OBTAIN PTR TO NEW DATA AREA ------------
    p_buf = NetBuf_GetDataPtr(p_if,
                              NET_TRANSACTION_RX,
                              NET_IF_ETHER_FRAME_MAX_SIZE,
                              NET_IF_IX_RX,
                              0,
                              0,
                              0,
                              p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {           // If unable to get a buffer.
      *p_size = 0;
      *p_data = DEF_NULL;
      return;
    }

    NetDev_RdData(p_if,
                  p_buf,
                  p_size,
                  &from_q,
                  &int_reg,
                  p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
      *p_size = 0;
      *p_data = DEF_NULL;
      return;
    }

    //                                                             ------------ ANALYSE & SET PACKET TYPE -------------
    *p_data = p_buf;
    p_frame_type = p_buf;
    switch (from_q) {
      case NET_DEV_Q_MGMT:
        switch (int_reg) {
          case NET_DEV_RESP_WAKEUP:
          case NET_DEV_RESP_PWR_SAVE:
          case NET_DEV_RESP_TAIM1:
          case NET_DEV_RESP_TAIM2:
          case NET_DEV_RESP_TADM:
          case NET_DEV_RESP_CARD_RDY:
          case NET_DEV_RESP_INIT:
          case NET_DEV_RESP_SCAN:
          case NET_DEV_RESP_JOIN:
          case NET_DEV_RESP_STACK_BYPASS:
            *p_frame_type = NET_IF_WIFI_MGMT_FRAME;
            p_buf[1] = from_q;
            p_buf[2] = int_reg;
            *p_size = 60;
            break;

          default:
            RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
            return;
        }
        break;

      case NET_DEV_Q_DATA:
        switch (int_reg) {
          case NET_DEV_RESP_CTRL:
            *p_frame_type = NET_IF_WIFI_MGMT_FRAME;
            p_buf[1] = from_q;
            p_buf[2] = int_reg;
            break;

          default:
            *p_frame_type = NET_IF_WIFI_DATA_PKT;
            p_buf[1] = from_q;
            if (*p_size < NET_IF_802x_FRAME_MIN_SIZE) {
              *p_size = NET_IF_802x_FRAME_MIN_SIZE;
            }
            break;
        }
        break;

      case NET_DEV_Q_LMAC:
      default:
        RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
        break;
    }
  } else {
    RTOS_ERR_SET(*p_err, RTOS_ERR_RX);
  }
}

/****************************************************************************************************//**
 *                                               NetDev_Tx()
 *
 * @brief    (1) This function transmits the specified data :
 *               - (a) Acquire   SPI Lock
 *               - (b) Enable    SPI Chip Select
 *               - (c) Configure SPI Controller
 *               - (d) Write data to the device to transmit
 *               - (e) Increment counters.
 *               - (f) Disable SPI Chip Select
 *               - (g) Release SPI lock
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
  NET_DEV_DATA            *p_dev_data;
  NET_DEV_MGMT_DESC_FRAME mgmt;

  //                                                               -------- OBTAIN REFERENCE TO CFGs/REGs/BSP ---------
  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;

  //                                                               -------------------- SEND DATA ---------------------
  Mem_Clr(&mgmt.Desc, sizeof(mgmt.Desc));
  mgmt.Desc.Word0 = size;
  mgmt.Desc.Word7 = MEM_VAL_LITTLE_TO_HOST_16(NET_DEV_FRAME_DATA);
  NetDev_WrData(p_if,
                NET_DEV_Q_DATA,
                sizeof(mgmt.Desc),
                (CPU_INT08U *)&mgmt.Desc,
                p_dev_data->GlobalBufPtr,
                p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }

  NetDev_WrData(p_if,
                NET_DEV_Q_DATA,
                size,
                p_data,
                p_dev_data->GlobalBufPtr,
                p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }

  NetIF_TxDeallocQPost(p_data, p_err);                          // See Note #3a.
  NetIF_DevTxRdySignal(p_if);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }
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
#if 0
#ifdef NET_MCAST_MODULE_EN
#endif

  RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
#endif
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
#if 0
#ifdef NET_MCAST_MODULE_EN
#endif

  RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
#endif
}

/****************************************************************************************************//**
 *                                           NetDev_ISR_Handler()
 *
 * @brief    This function serves as the device Interrupt Service Routine Handler. This ISR
 *           handler MUST service and clear all necessary and enabled interrupt events for
 *           the device.
 *
 * @param    p_if    Pointer to a network interface.
 *                   Argument validated in NetIF_ISR_Handler().
 *
 * @param    type    Network Interface defined argument representing the type of ISR in progress. Codes
 *                   for Rx, Tx, Overrun, Jabber, etc... are defined within net_if.h and are passed
 *                   into this function by the corresponding Net BSP ISR handler function. The Net
 *                   BSP ISR handler function may be called by a specific ISR vector and therefore
 *                   know which ISR type code to pass.  Otherwise, the Net BSP may pass
 *                   NET_DEV_ISR_TYPE_UNKNOWN and the device driver MAY ignore the parameter when
 *                   the ISR type can be deduced by reading an available interrupt status register.
 *                   Type codes that are defined within net_if.c include but are not limited to :
 *                   NET_DEV_ISR_TYPE_RX
 *                   NET_DEV_ISR_TYPE_TX_COMPLETE
 *                   NET_DEV_ISR_TYPE_UNKNOWN
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
 *                     management frame, which will be send automatically to NetDev_Demux() and where the stack
 *                     is called to dealloc the packet transmitted.
 *******************************************************************************************************/
static void NetDev_ISR_Handler(NET_IF           *p_if,
                               NET_DEV_ISR_TYPE type)
{
  RTOS_ERR err;

  RTOS_ERR_SET(err, RTOS_ERR_NONE);
  NetIF_RxQPost(p_if->Nbr, &err);

  PP_UNUSED_PARAM(err);                                         // Prevent possible 'variable unused' warnings.
  PP_UNUSED_PARAM(type);
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
 * @note     (2) The network buffer MUST be freed by this functions when the buffer is only used by this function.
 *               - (a) The network buffer MUST NOT be freed by this function when an error occurred and is returned,
 *                     the upper layer free the buffer and increment discarded management frame global counter.
 *               - (b) The network buffer MUST NOT be freed by this function when the wireless manager is signaled as
 *                     the network buffer will be used and freed by the wireless manager.
 *
 * @note     (3) The wireless manager MUST be signaled only when the response received is for a management
 *               command previously sent.
 *******************************************************************************************************/
static void NetDev_MgmtDemux(NET_IF   *p_if,
                             NET_BUF  *p_buf,
                             RTOS_ERR *p_err)
{
  NET_DEV_DATA     *p_dev_data;
  NET_WIFI_MGR_API *p_mgr_api;
  CPU_INT08U       type;
  CPU_BOOLEAN      signal;

  p_dev_data = (NET_DEV_DATA *) p_if->Dev_Data;
  type = p_buf->DataPtr[2];

  switch (p_dev_data->WaitRespType) {
    case NET_DEV_MGMT_BOOT:
      if (type == NET_DEV_RESP_CARD_RDY) {
        signal = DEF_YES;
      }
      break;

    case NET_DEV_MGMT_GET_HW_ADDR:
      if (type == NET_DEV_RESP_CTRL) {
        signal = DEF_YES;
      }
      break;

    case NET_DEV_MGMT_FIRMWARE_GET_VERSION:
      if (type == NET_DEV_RESP_CTRL) {
        signal = DEF_YES;
      }
      break;

    case NET_DEV_MGMT_FIRMWARE_START:
      if (type == NET_DEV_RESP_INIT) {
        signal = DEF_YES;
      }
      break;

    case NET_DEV_MGMT_SET_PWR_SAVE:
      if (type == NET_DEV_RESP_WAKEUP) {
        signal = DEF_YES;
      }
      break;

    case NET_IF_WIFI_CMD_SCAN:
      if (type == NET_DEV_RESP_SCAN) {
        signal = DEF_YES;
      }
      break;

    case NET_IF_WIFI_CMD_JOIN:
      if (type == NET_DEV_RESP_JOIN) {
        signal = DEF_YES;
      }
      break;

    case NET_IF_WIFI_CMD_LEAVE:
      if (type == NET_DEV_RESP_CTRL) {
        signal = DEF_YES;
      }
      break;

    case NET_DEV_MGMT_FIRMWARE_UPGRADE:
      if ((type == NET_DEV_RESP_TAIM1)
          | (type == NET_DEV_RESP_TAIM2)
          | (type == NET_DEV_RESP_TADM)) {
        signal = DEF_YES;
      }
      break;

    case NET_IF_IO_CTRL_LINK_STATE_GET:
    case NET_IF_IO_CTRL_LINK_STATE_GET_INFO:
      if (type == NET_DEV_RESP_CTRL) {
        signal = DEF_YES;
      }
      break;

    case NET_DEV_MGMT_STACK_BYPASS:
      if (type == NET_DEV_RESP_STACK_BYPASS) {
        signal = DEF_YES;
      }
      break;

    default:
      signal = DEF_NO;
      break;
  }

  if (signal == DEF_YES) {
    p_mgr_api = (NET_WIFI_MGR_API *)p_if->Ext_API;
    p_mgr_api->Signal(p_if, p_buf, p_err);
  } else {
    RTOS_ERR_SET(*p_err, RTOS_ERR_RX);
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
 * @param    p_err               Pointer to variable that will receive the return error code from this function.
 *
 * @return   length of data wrote in the return buffer in octet.
 *
 * @note     (1) The state machine context is used by the Wifi manager to know what it MUST do after this call:
 *               - (a) WaitResp is used by the wireless manager to know if an asynchronous response is
 *                     required.
 *               - (b) WaitRespTimeout_ms is used by the wireless manager to know what is the timeout to receive
 *                     the response.
 *               - (c) MgmtCompleted is used by the wireless manager to know if the management process is
 *                     completed.
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
  NET_DEV_DATA      *p_dev_data;
  CPU_BOOLEAN       *p_firmware_upgrade;
  NET_IF_LINK_STATE *p_link_state;

  PP_UNUSED_PARAM(cmd_data_len);
  PP_UNUSED_PARAM(buf_rtn_len_max);

  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;

  p_ctx->WaitRespTimeout_ms = NET_DEV_MGMT_RESP_COMMON_TIMEOUT_MS;

  switch (cmd) {
    case NET_DEV_MGMT_BOOT:
      p_firmware_upgrade = (CPU_BOOLEAN *)p_cmd_data;
      NetDev_MgmtExecuteCmdFirmwareBoot(p_if, *p_firmware_upgrade, p_err);
      if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;
        p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
        break;
      }
      p_ctx->WaitResp = DEF_YES;
      p_ctx->MgmtCompleted = DEF_NO;
      p_ctx->WaitRespTimeout_ms = NET_DEV_MGMT_RESP_FIRMWARE_BOOT_TIMEOUT_MS;
      p_dev_data->WaitRespType = NET_DEV_MGMT_BOOT;
      p_dev_data->SubState = NET_DEV_MGMT_NONE;
      break;

    case NET_DEV_MGMT_FIRMWARE_UPGRADE:
      p_ctx->WaitRespTimeout_ms = NET_DEV_MGMT_RESP_FIRMWARE_LOAD_TIMEOUT_MS;
      switch (p_dev_data->SubState) {
        case NET_DEV_MGMT_NONE:
          NetDev_MgmtExecuteCmdFirmwareUpgrade(p_if, NET_DEV_PART_FF_TAIM1, p_err);
          if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
            p_ctx->WaitResp = DEF_NO;
            p_ctx->MgmtCompleted = DEF_YES;
            p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
            p_dev_data->SubState = NET_DEV_MGMT_NONE;
            break;
          }
          p_ctx->WaitResp = DEF_YES;
          p_ctx->MgmtCompleted = DEF_NO;
          p_dev_data->WaitRespType = NET_DEV_MGMT_FIRMWARE_UPGRADE;
          p_dev_data->SubState = NET_DEV_MGMT_FIRMWARE_LOAD_FF_TAIM1;
          break;

        case NET_DEV_MGMT_FIRMWARE_LOAD_FF_TAIM2:
          NetDev_MgmtExecuteCmdFirmwareUpgrade(p_if, NET_DEV_PART_FF_TAIM2, p_err);
          if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
            p_ctx->WaitResp = DEF_NO;
            p_ctx->MgmtCompleted = DEF_YES;
            p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
            break;
          }
          p_ctx->WaitResp = DEF_YES;
          p_ctx->MgmtCompleted = DEF_NO;
          p_dev_data->WaitRespType = NET_DEV_MGMT_FIRMWARE_UPGRADE;
          p_dev_data->SubState = NET_DEV_MGMT_FIRMWARE_LOAD_FF_TAIM2;
          break;

        case NET_DEV_MGMT_FIRMWARE_LOAD_FF_DATA:
          NetDev_MgmtExecuteCmdFirmwareUpgrade(p_if, NET_DEV_PART_FF_TADM, p_err);
          if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
            p_ctx->WaitResp = DEF_NO;
            p_ctx->MgmtCompleted = DEF_YES;
            p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
            break;
          }
          p_ctx->WaitResp = DEF_YES;
          p_ctx->MgmtCompleted = DEF_NO;
          p_dev_data->WaitRespType = NET_DEV_MGMT_FIRMWARE_UPGRADE;
          p_dev_data->SubState = NET_DEV_MGMT_FIRMWARE_LOAD_FF_DATA;
          break;

        default:
          RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
          break;
      }
      break;

    case NET_DEV_MGMT_FIRMWARE_GET_VERSION:
      NetDev_MgmtExecuteCmdCtrlQuery(p_if, NET_DEV_CTRL_CMD_GET_FIRMWARE, p_err);
      if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;
        p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
        break;
      }
      p_ctx->WaitResp = DEF_YES;
      p_ctx->MgmtCompleted = DEF_NO;
      p_dev_data->WaitRespType = NET_DEV_MGMT_FIRMWARE_GET_VERSION;
      break;

    case NET_DEV_MGMT_FIRMWARE_START:
      NetDev_MgmtExecuteCmdFirmwareStart(p_if, p_err);
      if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;
        p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
        break;
      }
      p_ctx->WaitResp = DEF_YES;
      p_ctx->MgmtCompleted = DEF_NO;
      p_dev_data->WaitRespType = NET_DEV_MGMT_FIRMWARE_START;
      break;

    case NET_DEV_MGMT_GET_HW_ADDR:
      NetDev_MgmtExecuteCmdCtrlQuery(p_if, NET_DEV_CTRL_CMD_GET_HW_ADDR, p_err);
      if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;
        p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
        break;
      }

      p_ctx->WaitResp = DEF_YES;
      p_ctx->MgmtCompleted = DEF_NO;
      p_dev_data->WaitRespType = NET_DEV_MGMT_GET_HW_ADDR;
      break;

#if 0                                                           // TODO_NET : Setting the MAC address is currently not supported.
    case NET_DEV_MGMT_SET_HW_ADDR:
      NetDev_MgmtExecuteCmdCtrlQuery(p_if, NET_DEV_CTRL_CMD_SET_HW_ADDR, p_err);
      if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;
        p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
        break;
      }

      p_ctx->WaitResp = DEF_YES;
      p_ctx->MgmtCompleted = DEF_NO;
      p_dev_data->WaitRespType = NET_DEV_MGMT_SET_HW_ADDR;
      break;
#endif

    case NET_DEV_MGMT_STACK_BYPASS:
      NetDev_MgmtExecuteCmdStackBypass(p_if, p_err);
      if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;
        p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
        break;
      }
      p_ctx->WaitResp = DEF_YES;
      p_ctx->MgmtCompleted = DEF_NO;
      p_dev_data->WaitRespType = NET_DEV_MGMT_STACK_BYPASS;
      break;

    case NET_IF_WIFI_CMD_SCAN:
      NetDev_MgmtExecuteCmdScan(p_if,
                                (NET_IF_WIFI_SCAN *)p_cmd_data,
                                p_err);
      if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;
        p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
        break;
      }

      p_ctx->WaitResp = DEF_YES;
      p_ctx->MgmtCompleted = DEF_NO;
      p_ctx->WaitRespTimeout_ms = NET_DEV_MGMT_RESP_SCAN_TIMEOUT_MS;
      p_dev_data->WaitRespType = NET_IF_WIFI_CMD_SCAN;
      break;

    case NET_IF_WIFI_CMD_CREATE:
      NetDev_MgmtExecuteCmdJoin(p_if,
                                (NET_IF_WIFI_AP_CFG *)p_cmd_data,
                                DEF_YES,
                                p_err);
      if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;
        p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
        break;
      }

      p_ctx->WaitResp = DEF_YES;
      p_ctx->MgmtCompleted = DEF_NO;
      p_dev_data->WaitRespType = NET_IF_WIFI_CMD_JOIN;
      break;

    case NET_IF_WIFI_CMD_JOIN:
      NetDev_MgmtExecuteCmdJoin(p_if,
                                (NET_IF_WIFI_AP_CFG *)p_cmd_data,
                                DEF_NO,
                                p_err);
      if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;
        p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
        break;
      }

      p_ctx->WaitResp = DEF_YES;
      p_ctx->MgmtCompleted = DEF_NO;
      p_dev_data->WaitRespType = NET_IF_WIFI_CMD_JOIN;
      break;

    case NET_IF_WIFI_CMD_LEAVE:
      NetDev_MgmtExecuteCmdCtrlQuery(p_if, NET_DEV_CTRL_CMD_LEAVE, p_err);
      if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;
        p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
        break;
      }

      p_ctx->WaitResp = DEF_YES;
      p_ctx->MgmtCompleted = DEF_NO;
      p_dev_data->WaitRespType = NET_IF_WIFI_CMD_LEAVE;
      p_dev_data->LinkStatus = NET_IF_LINK_DOWN;
      break;

    case NET_IF_IO_CTRL_LINK_STATE_GET:
    case NET_IF_IO_CTRL_LINK_STATE_GET_INFO:
      p_dev_data->LinkStatusQueryCtr++;
      if (p_dev_data->LinkStatusQueryCtr == 100) {
        NetDev_MgmtExecuteCmdCtrlQuery(p_if, NET_DEV_CTRL_CMD_GET_STATUS, p_err);
        if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
          p_ctx->WaitResp = DEF_NO;
          p_ctx->MgmtCompleted = DEF_YES;
          p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
          break;
        }

        p_ctx->WaitResp = DEF_YES;
        p_ctx->MgmtCompleted = DEF_NO;
        p_dev_data->WaitRespType = cmd;
        p_dev_data->LinkStatusQueryCtr = 0;
      } else {
        p_link_state = (NET_IF_LINK_STATE *)p_buf_rtn;
        *p_link_state = p_dev_data->LinkStatus;
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;
      }
      break;

    case NET_IF_IO_CTRL_LINK_STATE_UPDATE:
    default:
      p_ctx->WaitResp = DEF_NO;
      p_ctx->MgmtCompleted = DEF_YES;
      p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
      RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
      break;
  }

  return (0);
}

/****************************************************************************************************//**
 *                                           NetDev_MgmtProcessResp()
 *
 * @brief    After that the wireless manager has received the response, this function is called to
 *           analyze, set the state machine context and fill the return buffer.
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
  CPU_INT08U                  type;
  CPU_INT32U                  rtn;
  CPU_INT08U                  *p_frame;
  CPU_INT08U                  *p_dst;
  const void                  *p_src;
  CPU_INT16U                  len;
  NET_DEV_DATA                *p_dev_data;
  const NET_IF_WIFI_PART_INFO *p_part_info;
  NET_IF_LINK_STATE           *p_link_state;

  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;
  p_part_info = &NetWiFi_Info_RS9110N2x;

  type = p_buf_rxd[2];
  p_frame = p_buf_rxd + p_part_info->RxBufIxOffset;
  rtn = 0;

  switch (cmd) {
    case NET_DEV_MGMT_BOOT:
      if ((p_dev_data->WaitRespType == NET_DEV_MGMT_BOOT)
          && (type == NET_DEV_RESP_CARD_RDY)) {
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;
      } else {
        p_ctx->WaitResp = DEF_YES;
        p_ctx->MgmtCompleted = DEF_NO;
      }
      break;

    case NET_DEV_MGMT_GET_HW_ADDR:
      if ((p_dev_data->WaitRespType == NET_DEV_MGMT_GET_HW_ADDR)
          && (type == NET_DEV_RESP_CTRL)       ) {
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;

        RTOS_ASSERT_DBG_ERR_SET((p_buf_rtn != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, 0);

        p_dst = (CPU_INT08U *)p_buf_rtn;
        p_src = p_frame + 2;
        Mem_Copy(p_dst, p_src, buf_rtn_len_max);
        rtn = buf_rtn_len_max;
      } else {
        p_ctx->WaitResp = DEF_YES;
        p_ctx->MgmtCompleted = DEF_NO;
      }
      break;

    case NET_DEV_MGMT_FIRMWARE_GET_VERSION:
      if ((p_dev_data->WaitRespType == NET_DEV_MGMT_FIRMWARE_GET_VERSION)
          && (type == NET_DEV_RESP_CTRL)                ) {
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;

        RTOS_ASSERT_DBG_ERR_SET((p_buf_rtn != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, 0);

        p_dst = (CPU_INT08U *)p_buf_rtn;
        p_src = p_frame + 2;
        Mem_Copy(p_dst, p_src, buf_rtn_len_max);
        rtn = buf_rtn_len_max;
      } else {
        p_ctx->WaitResp = DEF_YES;
        p_ctx->MgmtCompleted = DEF_NO;
      }
      break;

    case NET_DEV_MGMT_FIRMWARE_START:
      if ((p_dev_data->WaitRespType == NET_DEV_MGMT_FIRMWARE_START)
          && (type == NET_DEV_RESP_INIT)          ) {
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;
      } else {
        p_ctx->WaitResp = DEF_YES;
        p_ctx->MgmtCompleted = DEF_NO;
      }
      break;

    case NET_IF_WIFI_CMD_SCAN:
      if ((p_dev_data->WaitRespType == NET_IF_WIFI_CMD_SCAN)
          && (type == NET_DEV_RESP_SCAN)   ) {
        len = buf_rxd_len - p_part_info->RxBufIxOffset;
        rtn = NetDev_MgmtProcessRespScan(p_if,
                                         p_frame,
                                         len,
                                         p_buf_rtn,
                                         buf_rtn_len_max,
                                         p_err);
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;
      }
      break;

    case NET_DEV_MGMT_SET_PWR_SAVE:
      if ((p_dev_data->WaitRespType == NET_DEV_MGMT_SET_PWR_SAVE)
          && (type == NET_DEV_RESP_WAKEUP)      ) {
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;
      }
      break;

    case NET_IF_WIFI_CMD_JOIN:
      if ((p_dev_data->WaitRespType == NET_IF_WIFI_CMD_JOIN)
          && (type == NET_DEV_RESP_JOIN)   ) {
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;

        if (p_frame[2] != DEF_CLR) {
          RTOS_ERR_SET(*p_err, RTOS_ERR_IO);
        } else {
          p_dev_data->LinkStatus = NET_IF_LINK_UP;
        }
      }
      break;

    case NET_IF_WIFI_CMD_LEAVE:
      if ((p_dev_data->WaitRespType == NET_IF_WIFI_CMD_LEAVE)
          && (type == NET_DEV_RESP_CTRL)    ) {
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;

        if (p_frame[2] != DEF_CLR) {
          RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
        }
      }
      break;

    case NET_DEV_MGMT_FIRMWARE_UPGRADE:
      switch (p_dev_data->SubState) {
        case NET_DEV_MGMT_FIRMWARE_LOAD_FF_TAIM1:
          if (type == NET_DEV_RESP_TAIM1) {
            p_dev_data->WaitRespType = NET_DEV_MGMT_FIRMWARE_UPGRADE;
            p_dev_data->SubState = NET_DEV_MGMT_FIRMWARE_LOAD_FF_TAIM2;
            p_ctx->WaitResp = DEF_NO;
            p_ctx->MgmtCompleted = DEF_NO;
          } else {
            p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
            p_dev_data->SubState = NET_DEV_MGMT_NONE;
            p_ctx->WaitResp = DEF_NO;
            p_ctx->MgmtCompleted = DEF_YES;
            RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
          }
          break;

        case NET_DEV_MGMT_FIRMWARE_LOAD_FF_TAIM2:
          if (type == NET_DEV_RESP_TAIM2) {
            p_dev_data->WaitRespType = NET_DEV_MGMT_FIRMWARE_UPGRADE;
            p_dev_data->SubState = NET_DEV_MGMT_FIRMWARE_LOAD_FF_DATA;
            p_ctx->WaitResp = DEF_NO;
            p_ctx->MgmtCompleted = DEF_NO;
          } else {
            p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
            p_dev_data->SubState = NET_DEV_MGMT_NONE;
            p_ctx->WaitResp = DEF_NO;
            p_ctx->MgmtCompleted = DEF_YES;
            RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
          }
          break;

        case NET_DEV_MGMT_FIRMWARE_LOAD_FF_DATA:
          p_dev_data->WaitRespType = NET_DEV_MGMT_NONE;
          p_dev_data->SubState = NET_DEV_MGMT_NONE;
          p_ctx->WaitResp = DEF_NO;
          p_ctx->MgmtCompleted = DEF_YES;
          if (type != NET_DEV_RESP_TADM) {
            RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
          }
          break;

        default:
          RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
          break;
      }
      break;

    case NET_IF_IO_CTRL_LINK_STATE_GET:
    case NET_IF_IO_CTRL_LINK_STATE_GET_INFO:
      if ((p_dev_data->WaitRespType == cmd)
          && (type == NET_DEV_RESP_CTRL)) {
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;
        p_link_state = (NET_IF_LINK_STATE *)p_buf_rtn;
        if (p_frame[2] != DEF_SET) {
          *p_link_state = NET_IF_LINK_UP;
        } else {
          *p_link_state = NET_IF_LINK_DOWN;
        }

        p_dev_data->LinkStatus = *p_link_state;
      }
      break;

    case NET_DEV_MGMT_STACK_BYPASS:
      if ((p_dev_data->WaitRespType == NET_DEV_MGMT_STACK_BYPASS)
          && (type == NET_DEV_RESP_STACK_BYPASS)) {
        p_ctx->WaitResp = DEF_NO;
        p_ctx->MgmtCompleted = DEF_YES;
      } else {
        p_ctx->WaitResp = DEF_YES;
        p_ctx->MgmtCompleted = DEF_NO;
      }
      break;

    default:
      RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
      break;
  }

  return (rtn);
}

/****************************************************************************************************//**
 *                                               NetDev_InitSPI()
 *
 * @brief    Initialize the SPI on the device.
 *
 * @param    p_if    Pointer to a network interface.
 *
 * @param    p_err   Pointer to variable that will receive the return error code from this function.
 *
 * @note     (1) The SPI lock must not be acquired before calling this function.
 *******************************************************************************************************/
static void NetDev_InitSPI(NET_IF   *p_if,
                           RTOS_ERR *p_err)
{
  NET_DEV_DATA   *p_dev_data;
  NET_DEV_CMD    wr;
  NET_DEV_ANSWER rd;

  //                                                               ----------- OBTAIN REFERENCE TO MGR/BSP ------------
  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;

  //                                                               ---------------- PREAPRE INIT DATA -----------------
  wr.C1 = NET_DEV_CMD_INIT_SPI;                                 // Init cmd.
  wr.C2 = 0;                                                    // Dummy data.

  //                                                               Wr and rd data from SPI.
  SPI_SlaveXfer(p_dev_data->SPI_Handle,
                (CPU_INT08U *)&rd,
                (CPU_INT08U *)&wr,
                2,
                NET_DEV_SPI_XFER_TIMEOUT_MS,
                p_err);
  if ((RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE)
      || (rd.A2 != NET_DEV_STATUS_SUCCESS)) {
    RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
    return;
  }
}

/****************************************************************************************************//**
 *                                               NetDev_RdIntReg()
 *
 * @brief    Read interrupt register on the device.
 *
 * @param    p_if    Pointer to a network interface.
 *
 * @param    p_err   Pointer to variable that will receive the return error code from this function.
 *
 * @return   Interrupt register value.
 *
 * @note     (1) Prior calling this function, the SPI bus must be configured correctly:
 *               - (a) The SPI  lock          MUST be acquired via 'p_dev_bsp->SPI_Lock()'.
 *               - (b) The Chip select        MUST be enabled  via 'p_dev_bsp->SPI_ChipSelEn()'.
 *               - (c) The SPI  configuration MUST be set      via 'p_dev_bsp->SPI_SetCfg()'.
 *******************************************************************************************************/
static CPU_INT08U NetDev_RdIntReg(NET_IF   *p_if,
                                  RTOS_ERR *p_err)
{
  NET_DEV_DATA   *p_dev_data;
  NET_DEV_CMD    wr;
  NET_DEV_ANSWER rd;
  CPU_BOOLEAN    done;

  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;

  wr.C1 = NET_DEV_CMD_RD_WR
          | NET_DEV_CMD_TRANSFER_LEN_2_BYTES;

  wr.C2 = NET_DEV_REG_INT;                                      // Interrupt register
  rd.A2 = 0;

  done = DEF_NO;
  while (done != DEF_YES) {
    SPI_SlaveXfer(p_dev_data->SPI_Handle,
                  (CPU_INT08U *)&rd,
                  (CPU_INT08U *)&wr,
                  2,
                  NET_DEV_SPI_XFER_TIMEOUT_MS,
                  p_err);
    if (rd.A2 == NET_DEV_STATUS_SUCCESS) {
      done = DEF_YES;
    } else if (rd.A2 == NET_DEV_STATUS_FAIL) {
      RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
      return (0);
    } else if (rd.A2 != NET_DEV_STATUS_BUSY) {
      done = DEF_NO;
    }
  }

  NetDev_RdStartToken(p_if, p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return (0);
  }

  wr.C3 = NET_DEV_MGMT_CMD_OCTET;
  wr.C4 = NET_DEV_MGMT_CMD_OCTET;
  SPI_SlaveXfer(p_dev_data->SPI_Handle,
                (CPU_INT08U *)&rd,
                (CPU_INT08U *)&wr.C3,
                2,
                NET_DEV_SPI_XFER_TIMEOUT_MS,
                p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return (0);
  }

  return (rd.A1);
}

/****************************************************************************************************//**
 *                                   NetDev_MgmtExecuteCmdFirmwareBoot()
 *
 * @brief    Send data on the device to boot firmware on the device.
 *
 * @param    p_if        Pointer to a network interface.
 *
 * @param    upgrade     Boot to upgrade the firmware:
 *                       DEF_NO      Load the current firmware (firmware already loaded on the device).
 *                       DEF_YES     Prepare for a firmware upgrade.
 *
 * @param    p_err       Pointer to variable that will receive the return error code from this function.
 *
 * @note     (1) Prior calling this function, the SPI bus must be configured correctly:
 *               - (a) The SPI  lock          MUST be acquired via 'p_dev_bsp->SPI_Lock()'.
 *               - (b) The Chip select        MUST be enabled  via 'p_dev_bsp->SPI_ChipSelEn()'.
 *               - (c) The SPI  configuration MUST be set      via 'p_dev_bsp->SPI_SetCfg()'.
 *
 * @note     (2) Once the boot process is completed and the device is ready to be used for upgrading or
 *               to initialize the MAC layer an response is sent to the host.
 *******************************************************************************************************/
static void NetDev_MgmtExecuteCmdFirmwareBoot(NET_IF      *p_if,
                                              CPU_BOOLEAN upgrade,
                                              RTOS_ERR    *p_err)
{
  NET_DEV_DATA *p_dev_data;
  CPU_INT08U   *p_firmware;
  CPU_INT32U   num_blocks;
  CPU_INT32U   addr;
  CPU_INT32U   data;
  CPU_INT32U   size;
  CPU_INT32U   cur_ix;
  CPU_INT32U   i;
  CPU_INT32U   ii;

  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;

  //                                                               ----------- SET DEVICE IN SOFT RST MODE ------------
  data = MEM_VAL_LITTLE_TO_HOST_32(NET_DEV_RST_SET);
  addr = MEM_VAL_LITTLE_TO_HOST_32(NET_DEV_RST_ADDR);

  NetDev_WrMem(p_if,
               addr,
               sizeof(data),
               (CPU_INT08U *)&data,
               p_dev_data->GlobalBufPtr,
               p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }

  //                                                               ------------------ LOAD FIRMWARE -------------------
  for (i = 0; i < 4; i++) {
    switch (i) {
      case 0:
        if (upgrade != DEF_YES) {
          p_firmware = (CPU_INT08U *)&NetDev_sbtaim1[0];
          size = sizeof(NetDev_sbtaim1);
          addr = MEM_VAL_LITTLE_TO_HOST_32(NET_DEV_SOFT_ADDR_SB_IM1);
        } else {
          p_firmware = (CPU_INT08U *)&NetDev_iutaim1[0];
          size = sizeof(NetDev_iutaim1);
          addr = MEM_VAL_LITTLE_TO_HOST_32(NET_DEV_SOFT_ADDR_IU_IM1);
        }
        break;

      case 1:
        if (upgrade != DEF_YES) {
          p_firmware = (CPU_INT08U *)&NetDev_sbtaim2[0];
          size = sizeof(NetDev_sbtaim2);
          addr = MEM_VAL_LITTLE_TO_HOST_32(NET_DEV_SOFT_ADDR_SB_IM2);
        } else {
          p_firmware = (CPU_INT08U *)&NetDev_iutaim2[0];
          size = sizeof(NetDev_iutaim2);
          addr = MEM_VAL_LITTLE_TO_HOST_32(NET_DEV_SOFT_ADDR_IU_IM2);
        }
        break;

      case 2:
        if (upgrade != DEF_YES) {
          p_firmware = (CPU_INT08U *)&NetDev_sbtadm1[0];
          size = sizeof(NetDev_sbtadm1);
          addr = MEM_VAL_LITTLE_TO_HOST_32(NET_DEV_SOFT_ADDR_SB_DM1);
        } else {
          p_firmware = (CPU_INT08U *)&NetDev_iutadm[0];
          size = sizeof(NetDev_iutadm);
          addr = MEM_VAL_LITTLE_TO_HOST_32(NET_DEV_SOFT_ADDR_IU_DM);
        }
        break;

      case 3:
        if (upgrade != DEF_YES) {
          p_firmware = (CPU_INT08U *)&NetDev_sbtadm2[0];
          size = sizeof(NetDev_sbtadm1);
          addr = MEM_VAL_LITTLE_TO_HOST_32(NET_DEV_SOFT_ADDR_SB_DM2);
        } else {
          size = 0u;
        }
        break;

      default:
        size = 0u;
        break;
    }

    cur_ix = 0;
    if (size  % 4) {
      size += 4 - (size % 4);
    }

    ii = 0;
    num_blocks = size / NET_DEV_SOFT_BLK_SIZE;
    for (ii = 0; ii < num_blocks; ii++) {
      NetDev_WrMem(p_if,
                   addr + cur_ix,
                   NET_DEV_SOFT_BLK_SIZE,
                   p_firmware + cur_ix,
                   p_dev_data->GlobalBufPtr,
                   p_err);
      if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        return;
      }
      cur_ix += NET_DEV_SOFT_BLK_SIZE;
    }

    if (size % NET_DEV_SOFT_BLK_SIZE) {                         // This should be 4 byte aligned
      NetDev_WrMem(p_if,
                   addr + cur_ix,
                   size % NET_DEV_SOFT_BLK_SIZE,
                   p_firmware + cur_ix,
                   p_dev_data->GlobalBufPtr,
                   p_err);
      if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        return;
      }
    }
  }

  //                                                               ----------- SET DEVICE IN SOFT RST MODE ------------
  data = MEM_VAL_LITTLE_TO_HOST_32(NET_DEV_RST_CLR);
  addr = MEM_VAL_LITTLE_TO_HOST_32(NET_DEV_RST_ADDR);
  NetDev_WrMem(p_if,
               addr,
               sizeof(data),
               (CPU_INT08U *)&data,
               p_dev_data->GlobalBufPtr,
               p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }
}

/****************************************************************************************************//**
 *                                   NetDev_MgmtExecuteCmdFirmwareUpgrade()
 *
 * @brief    Send new firmware for upgrading.
 *
 * @param    p_if    Pointer to a network interface.
 *
 * @param    part    Firmware part to send:
 *                       - NET_DEV_PART_FF_TAIM1
 *                       - NET_DEV_PART_FF_TAIM2
 *                       - NET_DEV_PART_FF_TADM
 *
 * @param    p_err   Pointer to variable that will receive the return error code from this function.
 *
 * @note     (1) Prior calling this function, the SPI bus must be configured correctly:
 *               - (a) The SPI  lock          MUST be acquired via 'p_dev_bsp->SPI_Lock()'.
 *               - (b) The Chip select        MUST be enabled  via 'p_dev_bsp->SPI_ChipSelEn()'.
 *               - (c) The SPI  configuration MUST be set      via 'p_dev_bsp->SPI_SetCfg()'.
 *
 * @note     (2) For each firmware part sent to the device a response will be sent back to host when the data
 *               is correctly loaded and ready to receive the next part.
 *******************************************************************************************************/
static void NetDev_MgmtExecuteCmdFirmwareUpgrade(NET_IF     *p_if,
                                                 CPU_INT08U part,
                                                 RTOS_ERR   *p_err)
{
  NET_DEV_DATA            *p_dev_data;
  NET_DEV_MGMT_DESC_FRAME mgmt;
  CPU_INT08U              *p_firmware;
  CPU_INT32U              num_blocks;
  CPU_INT32U              cur_ix;
  CPU_INT32U              addr;
  CPU_INT32U              ii;
  CPU_INT16U              size;
  CPU_INT16U              cmd;

  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;

  switch (part) {
    case NET_DEV_PART_FF_TAIM1:
      cmd = NET_DEV_DESC_UPGRADE_FF_TAIM1;
      size = sizeof(NetDev_fftaim1);
      p_firmware = (CPU_INT08U *)&NetDev_fftaim1[0];
      addr = NET_DEV_SOFT_ADDR_IU;
      break;

    case NET_DEV_PART_FF_TAIM2:
      cmd = NET_DEV_DESC_UPGRADE_FF_TAIM2;
      size = sizeof(NetDev_fftaim2);
      p_firmware = (CPU_INT08U *)&NetDev_fftaim2[0];
      addr = NET_DEV_SOFT_ADDR_IU;
      break;

    case NET_DEV_PART_FF_TADM:
      cmd = NET_DEV_DESC_UPGRADE_FF_DATA;
      size = sizeof(NetDev_ffdata);
      p_firmware = (CPU_INT08U *)&NetDev_ffdata[0];
      addr = NET_DEV_SOFT_ADDR_IU;
      break;

    default:
      RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
      return;
  }

  num_blocks = size / NET_DEV_SOFT_BLK_SIZE;
  cur_ix = 0;
  ii = 0;

  for (ii = 0; ii < num_blocks; ii++) {
    NetDev_WrMem(p_if,
                 addr + cur_ix,
                 NET_DEV_SOFT_BLK_SIZE,
                 p_firmware + cur_ix,
                 p_dev_data->GlobalBufPtr,
                 p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
      return;
    }
    KAL_Dly(1);
    cur_ix += NET_DEV_SOFT_BLK_SIZE;
  }

  if (size % NET_DEV_SOFT_BLK_SIZE) {                           // This should be 4 byte aligned
    NetDev_WrMem(p_if,
                 addr + cur_ix,
                 size % NET_DEV_SOFT_BLK_SIZE,
                 p_firmware + cur_ix,
                 p_dev_data->GlobalBufPtr,
                 p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
      return;
    }
  }

  Mem_Clr(&mgmt, sizeof(mgmt));
  mgmt.Desc.Word1 = size;
  NetDev_WrMgmtCmd(p_if, cmd, DEF_NO, &mgmt, p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }
}

/****************************************************************************************************//**
 *                                       NetDev_MgmtExecuteCmdCtrlQuery()
 *
 * @brief    Send ctrl command to get some management informations.
 *
 * @param    p_if    Pointer to a network interface.
 *
 * @param    cmd     Control query command.
 *
 * @param    p_err   Pointer to variable that will receive the return error code from this function.
 *
 * @note     (1) Prior calling this function, the SPI bus must be configured correctly:
 *               - (a) The SPI  lock          MUST be acquired via 'p_dev_bsp->SPI_Lock()'.
 *               - (b) The Chip select        MUST be enabled  via 'p_dev_bsp->SPI_ChipSelEn()'.
 *               - (c) The SPI  configuration MUST be set      via 'p_dev_bsp->SPI_SetCfg()'.
 *
 * @note     (2) The firmware version is loaded by the device and then it's returned through a response.
 *******************************************************************************************************/
static void NetDev_MgmtExecuteCmdCtrlQuery(NET_IF     *p_if,
                                           CPU_INT08U cmd,
                                           RTOS_ERR   *p_err)
{
  NET_DEV_MGMT_FRAME_CTRL ctrl;
  NET_DEV_DATA            *p_dev_data;

  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;

  Mem_Clr(&ctrl, sizeof(ctrl));

  ctrl.Desc[0] = sizeof(ctrl.SubType);
  ctrl.Desc[4] = NET_DEV_MGMT_QUERY_OCTET;
  ctrl.Desc[14] = NET_DEV_FRAME_DATA;
  ctrl.SubType = cmd;

  NetDev_WrData(p_if,
                NET_DEV_Q_DATA,
                sizeof(ctrl.Desc),
                (CPU_INT08U *)&ctrl.Desc,
                p_dev_data->GlobalBufPtr,
                p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }

  NetDev_WrData(p_if,
                NET_DEV_Q_DATA,
                sizeof(ctrl.SubType),
                (CPU_INT08U *)&ctrl.SubType,
                p_dev_data->GlobalBufPtr,
                p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }
}

/****************************************************************************************************//**
 *                                   NetDev_MgmtExecuteCmdFirmwareStart()
 *
 * @brief    Send command to start the device firmware.
 *
 * @param    p_if    Pointer to a network interface.
 *
 * @param    p_err   Pointer to variable that will receive the return error code from this function.
 *
 * @note     (1) Prior calling this function, the SPI bus must be configured correctly:
 *               - (a) The SPI  lock          MUST be acquired via 'p_dev_bsp->SPI_Lock()'.
 *               - (b) The Chip select        MUST be enabled  via 'p_dev_bsp->SPI_ChipSelEn()'.
 *               - (c) The SPI  configuration MUST be set      via 'p_dev_bsp->SPI_SetCfg()'.
 *
 * @note     (2) Once the firmware is started, the device send a response to signal that the device is ready to
 *               receive and transmit.
 *******************************************************************************************************/
static void NetDev_MgmtExecuteCmdFirmwareStart(NET_IF   *p_if,
                                               RTOS_ERR *p_err)
{
  NET_DEV_MGMT_DESC_FRAME mgmt;

  Mem_Clr(&mgmt, sizeof(mgmt));

  NetDev_WrMgmtCmd(p_if, NET_DEV_DESC_INIT, DEF_NO, &mgmt, p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }
}

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
 *               - (a) The SPI  lock          MUST be acquired via 'p_dev_bsp->SPI_Lock()'.
 *               - (b) The Chip select        MUST be enabled  via 'p_dev_bsp->SPI_ChipSelEn()'.
 *               - (c) The SPI  configuration MUST be set      via 'p_dev_bsp->SPI_SetCfg()'.
 *
 * @note     (2) The scan is completed the device and then results are returned through a response.
 *******************************************************************************************************/
static void NetDev_MgmtExecuteCmdScan(NET_IF           *p_if,
                                      NET_IF_WIFI_SCAN *p_scan,
                                      RTOS_ERR         *p_err)
{
  NET_DEV_MGMT_DESC_FRAME mgmt;

  Mem_Clr(&mgmt, sizeof(mgmt));

  mgmt.Frame.Scan.Channel = p_scan->Ch;
  mgmt.Frame.Scan.SSID = p_scan->SSID;

  NetDev_WrMgmtCmd(p_if, NET_DEV_DESC_SCAN, DEF_YES, &mgmt, p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
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
 * @param    buf_len     Length of the access point buffer in octets.
 *
 * @param    p_err       Pointer to variable  that will receive the return error code from this function.
 *
 * @return   Number of octets written in the access point buffer.
 *******************************************************************************************************/
static CPU_INT32U NetDev_MgmtProcessRespScan(NET_IF     *p_if,
                                             CPU_INT08U *p_frame,
                                             CPU_INT16U frame_len,
                                             CPU_INT08U *p_ap_buf,
                                             CPU_INT16U buf_len,
                                             RTOS_ERR   *p_err)
{
  CPU_INT08U             i;
  CPU_INT08U             ap_ctn;
  CPU_INT08U             ap_rtn_max;
  NET_IF_WIFI_AP         *p_ap;
  NET_DEV_MGMT_SCAN_AP   *p_scan_ap;
  NET_DEV_MGMT_SCAN_RESP *p_scan_response;

  PP_UNUSED_PARAM(p_if);
  PP_UNUSED_PARAM(frame_len);

  RTOS_ASSERT_DBG_ERR_SET((p_ap_buf != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, 0);

  p_ap = (NET_IF_WIFI_AP *)p_ap_buf;
  p_scan_response = (NET_DEV_MGMT_SCAN_RESP *)p_frame;
  ap_rtn_max = buf_len / sizeof(NET_IF_WIFI_AP);
  ap_ctn = p_scan_response->ScanCnt;

  for (i = 0; i < ap_ctn; i++) {
    p_scan_ap = (NET_DEV_MGMT_SCAN_AP *)&p_scan_response->APs[i];
    Str_Copy_N((CPU_CHAR *)&p_ap->SSID,
               (CPU_CHAR *)&p_scan_ap->SSID,
               NET_IF_WIFI_STR_LEN_MAX_SSID);
    p_ap->SignalStrength = p_scan_ap->RSSID;
    p_ap->Ch = p_scan_ap->Ch;
    switch (p_scan_ap->SecMode) {
      case 0:
        p_ap->SecurityType = NET_IF_WIFI_SECURITY_OPEN;
        break;

      case 1:
        p_ap->SecurityType = NET_IF_WIFI_SECURITY_WPA;
        break;

      case 2:
        p_ap->SecurityType = NET_IF_WIFI_SECURITY_WPA2;
        break;

      case 3:
        p_ap->SecurityType = NET_IF_WIFI_SECURITY_WEP;
        break;

      default:
        RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
        return (ap_rtn_max);
    }

    p_ap->NetType = NET_IF_WIFI_NET_TYPE_INFRASTRUCTURE;
    p_ap++;

    if (i == ap_rtn_max - 1) {
      break;
    }
  }

  if (i == ap_rtn_max - 1) {
    return ((ap_rtn_max) * sizeof(NET_IF_WIFI_AP));
  } else {
    return ((ap_ctn) * sizeof(NET_IF_WIFI_AP));
  }
}

/****************************************************************************************************//**
 *                                       NetDev_MgmtExecuteCmdJoin()
 *
 * @brief    Send command to join a wireless network already scanned.
 *
 * @param    p_if    Pointer to a network interface.
 *
 * @param    p_join  Pointer to a access point to join.
 *
 * @param    p_err   Pointer to variable  that will receive the return error code from this function.
 *
 * @note     (1) Prior calling this function, the SPI bus must be configured correctly:
 *               - (a) The SPI  lock          MUST be acquired via 'p_dev_bsp->SPI_Lock()'.
 *               - (b) The Chip select        MUST be enabled  via 'p_dev_bsp->SPI_ChipSelEn()'.
 *               - (c) The SPI  configuration MUST be set      via 'p_dev_bsp->SPI_SetCfg()'.
 *******************************************************************************************************/
static void NetDev_MgmtExecuteCmdJoin(NET_IF             *p_if,
                                      NET_IF_WIFI_AP_CFG *p_join,
                                      CPU_BOOLEAN        create_adhoc,
                                      RTOS_ERR           *p_err)
{
  NET_DEV_MGMT_DESC_FRAME mgmt;
  NET_DEV_MGMT_FRAME_JOIN *p_frame;

  Mem_Clr(&mgmt, sizeof(mgmt));

  p_frame = &mgmt.Frame.Join;

  switch (p_join->NetType) {
    case NET_IF_WIFI_NET_TYPE_INFRASTRUCTURE:
      p_frame->NetworkType = NET_DEV_JOIN_NET_INFRASTRUCT;
      break;

    case NET_IF_WIFI_NET_TYPE_ADHOC:
      p_frame->NetworkType = NET_DEV_JOIN_NET_ADHOC;
      break;

    case NET_IF_WIFI_NET_TYPE_UNKNOWN:
    default:
      RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
      return;
  }

  switch (p_join->DataRate) {
    case NET_IF_WIFI_DATA_RATE_AUTO:
      p_frame->DataRate = NET_DEV_JOIN_DATA_RATE_AUTO;
      break;

    case NET_IF_WIFI_DATA_RATE_1_MBPS:
      p_frame->DataRate = NET_DEV_JOIN_DATA_RATE_1_MBPS;
      break;

    case NET_IF_WIFI_DATA_RATE_2_MBPS:
      p_frame->DataRate = NET_DEV_JOIN_DATA_RATE_2_MBPS;
      break;

    case NET_IF_WIFI_DATA_RATE_5_5_MBPS:
      p_frame->DataRate = NET_DEV_JOIN_DATA_RATE_5_5_MBPS;
      break;

    case NET_IF_WIFI_DATA_RATE_6_MBPS:
      p_frame->DataRate = NET_DEV_JOIN_DATA_RATE_6_MBPS;
      break;

    case NET_IF_WIFI_DATA_RATE_9_MBPS:
      p_frame->DataRate = NET_DEV_JOIN_DATA_RATE_9_MBPS;
      break;

    case NET_IF_WIFI_DATA_RATE_11_MBPS:
      p_frame->DataRate = NET_DEV_JOIN_DATA_RATE_11_MBPS;
      break;

    case NET_IF_WIFI_DATA_RATE_12_MBPS:
      p_frame->DataRate = NET_DEV_JOIN_DATA_RATE_12_MBPS;
      break;

    case NET_IF_WIFI_DATA_RATE_18_MBPS:
      p_frame->DataRate = NET_DEV_JOIN_DATA_RATE_18_MBPS;
      break;

    case NET_IF_WIFI_DATA_RATE_24_MBPS:
      p_frame->DataRate = NET_DEV_JOIN_DATA_RATE_24_MBPS;
      break;

    case NET_IF_WIFI_DATA_RATE_36_MBPS:
      p_frame->DataRate = NET_DEV_JOIN_DATA_RATE_36_MBPS;
      break;

    case NET_IF_WIFI_DATA_RATE_48_MBPS:
      p_frame->DataRate = NET_DEV_JOIN_DATA_RATE_48_MBPS;
      break;

    case NET_IF_WIFI_DATA_RATE_54_MBPS:
      p_frame->DataRate = NET_DEV_JOIN_DATA_RATE_54_MBPS;
      break;

    case NET_IF_WIFI_DATA_RATE_MCS0:
      p_frame->DataRate = NET_DEV_JOIN_DATA_RATE_MCS0;
      break;

    case NET_IF_WIFI_DATA_RATE_MCS1:
      p_frame->DataRate = NET_DEV_JOIN_DATA_RATE_MCS1;
      break;

    case NET_IF_WIFI_DATA_RATE_MCS2:
      p_frame->DataRate = NET_DEV_JOIN_DATA_RATE_MCS2;
      break;

    case NET_IF_WIFI_DATA_RATE_MCS3:
      p_frame->DataRate = NET_DEV_JOIN_DATA_RATE_MCS3;
      break;

    case NET_IF_WIFI_DATA_RATE_MCS4:
      p_frame->DataRate = NET_DEV_JOIN_DATA_RATE_MCS4;
      break;

    case NET_IF_WIFI_DATA_RATE_MCS5:
      p_frame->DataRate = NET_DEV_JOIN_DATA_RATE_MCS5;
      break;

    case NET_IF_WIFI_DATA_RATE_MCS6:
      p_frame->DataRate = NET_DEV_JOIN_DATA_RATE_MCS6;
      break;

    case NET_IF_WIFI_DATA_RATE_MCS7:
      p_frame->DataRate = NET_DEV_JOIN_DATA_RATE_MCS7;
      break;

    default:
      RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
      break;
  }

  switch (p_join->PwrLevel) {
    case NET_IF_WIFI_PWR_LEVEL_LO:
      p_frame->PwrLevel = NET_DEV_JOIN_PWR_LEVEL_LO;
      break;

    case NET_IF_WIFI_PWR_LEVEL_MED:
      p_frame->PwrLevel = NET_DEV_JOIN_PWR_LEVEL_MED;
      break;

    case NET_IF_WIFI_PWR_LEVEL_HI:
      p_frame->PwrLevel = NET_DEV_JOIN_PWR_LEVEL_HI;
      break;

    default:
      RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
      return;
  }

  switch (p_join->SecurityType) {
    case NET_IF_WIFI_SECURITY_OPEN:
      p_frame->SecType = NET_DEV_JOIN_SECURITY_OPEN;
      break;

    case NET_IF_WIFI_SECURITY_WPA:
      p_frame->SecType = NET_DEV_JOIN_SECURITY_WPA;
      break;

    case NET_IF_WIFI_SECURITY_WPA2:
      p_frame->SecType = NET_DEV_JOIN_SECURITY_WPA2;
      break;

    case NET_IF_WIFI_SECURITY_WEP:
      p_frame->SecType = NET_DEV_JOIN_SECURITY_WEP;
      break;

    default:
      RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
      return;
  }

  if (create_adhoc == DEF_NO) {
    p_frame->Mode = NET_DEV_IBSS_MODE_JOIN;
  } else {
    if (p_join->NetType != NET_IF_WIFI_NET_TYPE_ADHOC) {
      RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
      return;
    }

    p_frame->Mode = NET_DEV_IBSS_MODE_CREATE;
    switch (p_join->Ch) {
      case NET_IF_WIFI_CH_1:
      case NET_IF_WIFI_CH_2:
      case NET_IF_WIFI_CH_3:
      case NET_IF_WIFI_CH_4:
      case NET_IF_WIFI_CH_5:
      case NET_IF_WIFI_CH_6:
      case NET_IF_WIFI_CH_7:
      case NET_IF_WIFI_CH_8:
      case NET_IF_WIFI_CH_9:
      case NET_IF_WIFI_CH_10:
      case NET_IF_WIFI_CH_11:
      case NET_IF_WIFI_CH_12:
      case NET_IF_WIFI_CH_13:
      case NET_IF_WIFI_CH_14:
        p_frame->Ch = p_join->Ch;
        break;

      case NET_IF_WIFI_CH_ALL:
      default:
        RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
        return;
    }
  }

  Mem_Copy(&p_frame->SSID, &p_join->SSID, sizeof(p_frame->SSID));
  Mem_Copy(&p_frame->PSK, &p_join->PSK, sizeof(p_frame->PSK));

  NetDev_WrMgmtCmd(p_if, NET_DEV_DESC_JOIN, DEF_YES, &mgmt, p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }
}

/****************************************************************************************************//**
 *                                   NetDev_MgmtExecuteCmdStackBypass()
 *
 * @brief    Send command to the device to Bypass the TCPIP stack of the module.
 *
 * @param    p_if    Pointer to a network interface.
 *
 * @param    p_err   Pointer to variable  that will receive the return error code from this function.
 *
 * @note     (1) Prior calling this function, the SPI bus must be configured correctly:
 *               - (a) The SPI  lock          MUST be acquired via 'p_dev_bsp->SPI_Lock()'.
 *               - (b) The Chip select        MUST be enabled  via 'p_dev_bsp->SPI_ChipSelEn()'.
 *               - (c) The SPI  configuration MUST be set      via 'p_dev_bsp->SPI_SetCfg()'.
 *
 * @note     (2) The scan is completed the device and then results are returned through a response.
 *******************************************************************************************************/
static void NetDev_MgmtExecuteCmdStackBypass(NET_IF   *p_if,
                                             RTOS_ERR *p_err)
{
  NET_DEV_MGMT_DESC_FRAME mgmt;

  Mem_Clr(&mgmt, sizeof(mgmt));

  mgmt.Frame.Stack_Bypass.Mode = 0x15;

  NetDev_WrMgmtCmd(p_if, NET_DEV_DESC_TCPIP_BYPASS, DEF_YES, &mgmt, p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }
}

/****************************************************************************************************//**
 *                                           NetDev_WrMgmtCmd()
 *
 * @brief    Write management command on the device
 *
 * @param    p_if        Pointer to a network interface.
 *
 * @param    mgmt_cmd    Management command value.
 *
 * @param    wr_frame    Frame data must be written.
 *
 * @param    p_mgmt      Pointer to management frame to write.
 *
 * @param    p_err       Pointer to variable  that will receive the return error code from this function.
 *
 * @note     (1) Prior calling this function, the SPI bus must be configured correctly:
 *               - (a) The SPI  lock          MUST be acquired via 'p_dev_bsp->SPI_Lock()'.
 *               - (b) The Chip select        MUST be enabled  via 'p_dev_bsp->SPI_ChipSelEn()'.
 *               - (c) The SPI  configuration MUST be set      via 'p_dev_bsp->SPI_SetCfg()'.
 *******************************************************************************************************/
static void NetDev_WrMgmtCmd(NET_IF                  *p_if,
                             CPU_INT16U              mgmt_cmd,
                             CPU_BOOLEAN             wr_frame,
                             NET_DEV_MGMT_DESC_FRAME *p_mgmt,
                             RTOS_ERR                *p_err)
{
  NET_DEV_DATA *p_dev_data;

  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;

  if (wr_frame == DEF_YES) {
    p_mgmt->Desc.Word0 |= MEM_VAL_LITTLE_TO_HOST_16(mgmt_cmd | sizeof(p_mgmt->Frame));
  } else {
    p_mgmt->Desc.Word0 |= MEM_VAL_LITTLE_TO_HOST_16(mgmt_cmd);
  }

  p_mgmt->Desc.Word7 |= MEM_VAL_LITTLE_TO_HOST_16(NET_DEV_FRAME_MGMT);

  NetDev_WrData(p_if,
                NET_DEV_Q_MGMT,
                sizeof(p_mgmt->Desc),
                (CPU_INT08U *)&p_mgmt->Desc,
                p_dev_data->GlobalBufPtr,
                p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }

  if (wr_frame == DEF_YES) {
    NetDev_WrData(p_if,
                  NET_DEV_Q_MGMT,
                  sizeof(p_mgmt->Frame),
                  (CPU_INT08U *)&p_mgmt->Frame,
                  p_dev_data->GlobalBufPtr,
                  p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
      return;
    }
  }
}

/****************************************************************************************************//**
 *                                               NetDev_WrMem()
 *
 * @brief    Write data into the memory section of the module.
 *
 * @param    p_if        Pointer to a network interface.
 *
 * @param    addr        Memory address to start writing on.
 *
 * @param    p_wr_data   Pointer to the data to write in memory section.
 *
 * @param    p_rd_data   Pointer to buffer to receice data read.
 *
 * @param    p_err       Pointer to variable  that will receive the return error code from this function.
 *
 * @note     (1) Prior calling this function, the SPI bus must be configured correctly:
 *               - (a) The SPI  lock          MUST be acquired via 'p_dev_bsp->SPI_Lock()'.
 *               - (b) The Chip select        MUST be enabled  via 'p_dev_bsp->SPI_ChipSelEn()'.
 *               - (c) The SPI  configuration MUST be set      via 'p_dev_bsp->SPI_SetCfg()'.
 *******************************************************************************************************/
static void NetDev_WrMem(NET_IF     *p_if,
                         CPU_INT32U addr,
                         CPU_INT16U data_len,
                         CPU_INT08U *p_wr_data,
                         CPU_INT08U *p_rd_data,
                         RTOS_ERR   *p_err)
{
  NET_DEV_DATA   *p_dev_data;
  NET_DEV_CMD    wr;
  NET_DEV_ANSWER rd;
  CPU_BOOLEAN    done;

  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;

  SPI_SlaveSel(p_dev_data->SPI_Handle, 0, 0, p_err);
  RTOS_ASSERT_DBG(RTOS_ERR_CODE_GET(*p_err) == RTOS_ERR_NONE, RTOS_ERR_FAIL,; );

  //                                                               ----------- SET DEVICE IN SOFT RST MODE ------------
  wr.C1 = NET_DEV_CMD_RD_WR                                     // SPI Cmd  = 0x74
          | NET_DEV_CMD_WR
          | NET_DEV_CMD_AHB_BUS_ACCESS
          | NET_DEV_CMD_TRANSFER_LEN_16_BITS;
  wr.C2 = 0;                                                    // Dummy Data
  wr.C3 = NET_DEV_LO_BYTE(data_len);                            // High Byte
  wr.C4 = NET_DEV_HI_BYTE(data_len);                            // Low Byte

  done = DEF_NO;
  while (done != DEF_YES) {
    SPI_SlaveXfer(p_dev_data->SPI_Handle,                       // Send C1 and C2
                  (CPU_INT08U *)&rd,
                  (CPU_INT08U *)&wr,
                  2,
                  NET_DEV_SPI_XFER_TIMEOUT_MS,
                  p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
      goto exit_desel;
    } else if (rd.A2 == NET_DEV_STATUS_SUCCESS) {
      done = DEF_YES;
    } else if (rd.A2 == NET_DEV_STATUS_BUSY) {
      done = DEF_NO;
    } else {
      RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
      goto exit_desel;
    }
  }

  SPI_SlaveXfer(p_dev_data->SPI_Handle,                         // Send C3 and C4
                (CPU_INT08U *)&rd,
                (CPU_INT08U *)&wr.C3,
                2,
                NET_DEV_SPI_XFER_TIMEOUT_MS,
                p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    goto exit_desel;
  } else if (rd.A2 != NET_DEV_STATUS_SUCCESS) {
    RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
    goto exit_desel;
  }

  SPI_SlaveXfer(p_dev_data->SPI_Handle,
                (CPU_INT08U *)&rd,
                (CPU_INT08U *)&addr,
                4,
                NET_DEV_SPI_XFER_TIMEOUT_MS,
                p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    goto exit_desel;
  } else if (rd.A2 != NET_DEV_STATUS_SUCCESS) {
    RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
    goto exit_desel;
  }

  SPI_SlaveXfer(p_dev_data->SPI_Handle,
                (CPU_INT08U *)p_rd_data,
                (CPU_INT08U *)p_wr_data,
                data_len,
                NET_DEV_SPI_XFER_TIMEOUT_MS,
                p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    goto exit_desel;
  } else if (p_rd_data[1] != NET_DEV_STATUS_SUCCESS) {
    RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
    goto exit_desel;
  }

exit_desel:
  {
    RTOS_ERR local_err;

    SPI_SlaveDesel(p_dev_data->SPI_Handle, &local_err);
    RTOS_ASSERT_DBG(RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE, RTOS_ERR_FAIL,; );
  }
}

/****************************************************************************************************//**
 *                                               NetDev_WrData()
 *
 * @brief    Write data to data section of the device.
 *
 * @param    p_if        Pointer to a network interface.
 *
 * @param    addr        Write register address.
 *
 * @param    data_len    Data length to write and read.
 *
 * @param    p_wr_data   Pointer to the data to write in memory section.
 *
 * @param    p_rd_data   Pointer to buffer to receice data read.
 *
 * @param    p_err       Pointer to variable  that will receive the return error code from this function.
 *
 * @note     (1) Prior calling this function, the SPI bus must be configured correctly:
 *               - (a) The SPI  lock          MUST be acquired via 'p_dev_bsp->SPI_Lock()'.
 *               - (b) The Chip select        MUST be enabled  via 'p_dev_bsp->SPI_ChipSelEn()'.
 *               - (c) The SPI  configuration MUST be set      via 'p_dev_bsp->SPI_SetCfg()'.
 *******************************************************************************************************/
static void NetDev_WrData(NET_IF     *p_if,
                          CPU_INT08U addr,
                          CPU_INT16U data_len,
                          CPU_INT08U *p_wr_data,
                          CPU_INT08U *p_rd_data,
                          RTOS_ERR   *p_err)
{
  NET_DEV_DATA   *p_dev_data;
  NET_DEV_CMD    wr;
  NET_DEV_ANSWER rd;
  CPU_BOOLEAN    done;

  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;

  wr.C1 = NET_DEV_CMD_RD_WR
          | NET_DEV_CMD_WR
          | NET_DEV_CMD_AHB_BUS_ACCESS
          | NET_DEV_CMD_AHB_SLAVE
          | NET_DEV_CMD_TRANSFER_LEN_16_BITS;

  wr.C2 = addr & 0x3F;
  wr.C3 = NET_DEV_LO_BYTE(data_len);
  wr.C4 = NET_DEV_HI_BYTE(data_len);
  done = DEF_NO;

  while (done != DEF_YES) {
    SPI_SlaveXfer(p_dev_data->SPI_Handle,                       // Send C1 and C2
                  (CPU_INT08U *)&rd,
                  (CPU_INT08U *)&wr,
                  2,
                  NET_DEV_SPI_XFER_TIMEOUT_MS,
                  p_err);
    if (rd.A2 == NET_DEV_STATUS_SUCCESS) {
      done = DEF_YES;
    } else if (rd.A2 == NET_DEV_STATUS_BUSY) {
      done = DEF_NO;
    } else {
      RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
      return;
    }
  }

  SPI_SlaveXfer(p_dev_data->SPI_Handle,                         // Send C3 and C4
                (CPU_INT08U *)&rd,
                (CPU_INT08U *)&wr.C3,
                2,
                NET_DEV_SPI_XFER_TIMEOUT_MS,
                p_err);
  if (rd.A2 != NET_DEV_STATUS_SUCCESS) {
    RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
    return;
  }

  //                                                               Wr & rd data
  SPI_SlaveXfer(p_dev_data->SPI_Handle,
                (CPU_INT08U *)p_rd_data,
                (CPU_INT08U *)p_wr_data,
                data_len,
                NET_DEV_SPI_XFER_TIMEOUT_MS,
                p_err);
  if (p_rd_data[1] != NET_DEV_STATUS_SUCCESS) {
    RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
    return;
  }
}

/****************************************************************************************************//**
 *                                               NetDev_RdData()
 *
 * @brief    Prepare the device for a read command and read data from a queue.
 *
 * @param    p_if        Pointer to a network interface.
 *
 * @param    p_buf       Pointer to a network buffer that will receive the data read.
 *
 * @param    p_size      Pointer to a variable that will receive the length of the data read.
 *
 * @param    p_queue     Pointer to a variable that will receive the from which queue the data is read.
 *
 * @param    p_type      Pointer to a variable that will receive the data type.
 *
 * @param    p_err       Pointer to variable  that will receive the return error code from this function.
 *
 * @note     (1) Prior calling this function, the SPI bus must be configured correctly:
 *               - (a) The SPI  lock          MUST be acquired via 'p_dev_bsp->SPI_Lock()'.
 *               - (b) The Chip select        MUST be enabled  via 'p_dev_bsp->SPI_ChipSelEn()'.
 *               - (c) The SPI  configuration MUST be set      via 'p_dev_bsp->SPI_SetCfg()'.
 *******************************************************************************************************/
static void NetDev_RdData(NET_IF     *p_if,
                          CPU_INT08U *p_buf,
                          CPU_INT16U *p_size,
                          CPU_INT08U *p_queue,
                          CPU_INT08U *p_type,
                          RTOS_ERR   *p_err)
{
  NET_DEV_DATA                *p_dev_data;
  const NET_IF_WIFI_PART_INFO *p_part_info;
  CPU_INT08U                  buf_wr[NET_DEV_DESC_SIZE];
  CPU_INT08U                  *p_data;
  CPU_INT32U                  queue;
  CPU_INT16U                  len;
  CPU_INT16U                  len_cur;
  CPU_INT16U                  pkts;
  CPU_INT16U                  frag;
  CPU_INT16U                  pkt_len;
  CPU_INT08U                  i;
  CPU_BOOLEAN                 flag;

  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;
  p_part_info = &NetWiFi_Info_RS9110N2x;

  len = 0;
  flag = DEF_CLR;
  p_data = p_buf + p_part_info->RxBufIxOffset;

  NetDev_Rd(p_if,
            0,
            buf_wr,
            p_data,
            sizeof(buf_wr),
            p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }

  queue = p_data[14] & NET_DEV_Q_MASK;
  switch (queue) {
    case NET_DEV_Q_LMAC:
      *p_queue = NET_DEV_Q_LMAC;
      if (p_data[1] & 0x01) {
        len = MEM_VAL_HOST_TO_LITTLE_16(*(CPU_INT16U *)&p_data[4]);
      } else {
        len = p_data[0];
      }
      break;

    case NET_DEV_Q_MGMT:
      *p_queue = NET_DEV_Q_MGMT;
      *p_size = 60;
      *p_type = p_data[1];
      if ((p_data[0] & 0xFF) > 0) {
        len = MEM_VAL_HOST_TO_LITTLE_16(*(CPU_INT16U *)&p_data[8]);
      } else {
        len = p_data[0];
      }
      break;

    default:
      *p_queue = NET_DEV_Q_DATA;
      *p_type = p_data[5];

      if (p_data[15] & NET_DEV_DESC_Q_MASK) {
        pkts = p_data[15] & NET_DEV_DESC_Q_MASK;
        len_cur = 0;

        for (i = 0; i < pkts; i++) {
          len_cur = MEM_VAL_HOST_TO_LITTLE_16(*(CPU_INT16U *)&p_data[i * 2] & 0xFFF);
          len += len_cur;
          frag = len_cur % 4;
          if (frag) {
            len += (4 - frag);
          }
        }
      } else {
        len = ( ((p_data[1] & 0x0F) << 8)  + (p_data[0]));
        if (len > NET_DEV_RX_BUF_LEN_MAX) {
          len = NET_DEV_RX_BUF_LEN_MAX;
        }

        pkt_len = len;
        if (len % 4) {
          len += (4 - len % 4);
        }

        flag = DEF_SET;
      }
  }

  if (flag == DEF_CLR) {
    frag = len % 4;
    if (frag) {
      len += (4 - frag);
    }
  }

  if (len > 0) {
    NetDev_Rd(p_if,
              0,
              p_dev_data->GlobalBufPtr,
              p_data,
              len,
              p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
      return;
    }

    *p_size = pkt_len;
  } else {
    *p_size = sizeof(buf_wr);
  }
}

/****************************************************************************************************//**
 *                                               NetDev_Rd()
 *
 * @brief    Read on the device.
 *
 * @param    p_if        Pointer to a network interface.
 *
 * @param    addr        Read register address.
 *
 * @param    p_wr_data   Pointer to the data to write in memory section.
 *
 * @param    p_rd_data   Pointer to buffer to receice data read.
 *
 * @param    data_len    Data length to write and read.
 *
 * @param    p_err       Pointer to variable  that will receive the return error code from this function.
 *
 * @note     (1) Prior calling this function, the SPI bus must be configured correctly:
 *               - (a) The SPI  lock          MUST be acquired via 'p_dev_bsp->SPI_Lock()'.
 *               - (b) The Chip select        MUST be enabled  via 'p_dev_bsp->SPI_ChipSelEn()'.
 *               - (c) The SPI  configuration MUST be set      via 'p_dev_bsp->SPI_SetCfg()'.
 *******************************************************************************************************/
static void NetDev_Rd(NET_IF     *p_if,
                      CPU_INT32U addr,
                      CPU_INT08U *p_data_wr,
                      CPU_INT08U *p_data_rd,
                      CPU_INT16U data_len,
                      RTOS_ERR   *p_err)
{
  NET_DEV_DATA   *p_dev_data;
  NET_DEV_CMD    wr;
  NET_DEV_ANSWER rd;

  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;

  wr.C1 = NET_DEV_CMD_RD_WR
          | NET_DEV_CMD_AHB_BUS_ACCESS
          | NET_DEV_CMD_AHB_SLAVE
          | NET_DEV_CMD_TRANSFER_LEN_16_BITS;
  wr.C2 = addr;
  wr.C3 = NET_DEV_LO_BYTE(data_len);
  wr.C4 = NET_DEV_HI_BYTE(data_len);

  SPI_SlaveXfer(p_dev_data->SPI_Handle,
                (CPU_INT08U *)&rd,
                (CPU_INT08U *)&wr,
                2,
                NET_DEV_SPI_XFER_TIMEOUT_MS,
                p_err);
  if ((RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE)
      || (rd.A2 != NET_DEV_STATUS_SUCCESS)) {
    RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
    return;
  }

  SPI_SlaveXfer(p_dev_data->SPI_Handle,
                (CPU_INT08U *)&rd,
                (CPU_INT08U *)&wr.C3,
                2,
                NET_DEV_SPI_XFER_TIMEOUT_MS,
                p_err);
  if ((RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE)
      || (rd.A2 != NET_DEV_STATUS_SUCCESS)) {
    RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
    return;
  }

  NetDev_RdStartToken(p_if, p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }

  SPI_SlaveXfer(p_dev_data->SPI_Handle,
                (CPU_INT08U *)p_data_rd,
                (CPU_INT08U *)p_data_wr,
                data_len,
                NET_DEV_SPI_XFER_TIMEOUT_MS,
                p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    return;
  }
}

/****************************************************************************************************//**
 *                                           NetDev_RdStartToken()
 *
 * @brief    Write on the device until the start token is read.
 *
 * @param    p_if    Pointer to a network interface.
 *
 * @param    p_err   Pointer to variable  that will receive the return error code from this function :
 *
 * @note     (1) Prior calling this function, the SPI bus must be configured correctly:
 *               - (a) The SPI  lock          MUST be acquired via 'p_dev_bsp->SPI_Lock()'.
 *               - (b) The Chip select        MUST be enabled  via 'p_dev_bsp->SPI_ChipSelEn()'.
 *               - (c) The SPI  configuration MUST be set      via 'p_dev_bsp->SPI_SetCfg()'.
 *******************************************************************************************************/
static void NetDev_RdStartToken(NET_IF   *p_if,
                                RTOS_ERR *p_err)
{
  NET_DEV_DATA *p_dev_data;
  CPU_INT32U   ctr;
  CPU_INT08U   wr;
  CPU_INT08U   rd;
  CPU_BOOLEAN  done;

  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;

  ctr = 0;
  wr = 0;
  rd = 0;
  done = DEF_NO;
  while (done == DEF_NO) {
    SPI_SlaveXfer(p_dev_data->SPI_Handle,                       // Send C3 and C4
                  (CPU_INT08U *)&rd,
                  (CPU_INT08U *)&wr,
                  1,
                  NET_DEV_SPI_XFER_TIMEOUT_MS,
                  p_err);
    if (rd == NET_DEV_TOKEN_START) {
      done = DEF_YES;
    } else {
      ctr++;
      if (ctr >= NET_DEV_TOKEN_MAX_RETRY) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
        return;
      }
    }
  }
}

/********************************************************************************************************
 ********************************************************************************************************
 *                                       DEPENDENCIES & AVAIL CHECK(S)
 ********************************************************************************************************
 *******************************************************************************************************/

#endif // RTOS_MODULE_NET_IF_WIFI_AVAIL
