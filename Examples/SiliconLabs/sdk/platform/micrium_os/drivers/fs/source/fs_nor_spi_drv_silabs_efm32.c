/***************************************************************************//**
 * @file
 * @brief File System - EFM32-EFR32 SPI Controller Driver
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
 * @note    (1) This driver does NOT support:
 *             - (a) Dual I/O data communication, that is commands 2READ and DREAD
 *             - (b) Interrupt-based data communication
 *
 *          (2) This driver works with the USART peripheral in SPI mode of microcontrollers EFM32 and
 *              EFR32. The driver has been run on the following MCUs: EFM32GG390, EFR32MG1P.
 *******************************************************************************************************/

/********************************************************************************************************
 ********************************************************************************************************
 *                                       DEPENDENCIES & AVAIL CHECK(S)
 ********************************************************************************************************
 *******************************************************************************************************/

#include <rtos_description.h>

#if (defined(RTOS_MODULE_FS_STORAGE_NOR_AVAIL))

#if (!defined(RTOS_MODULE_FS_AVAIL))

#error NOR module requires File System Storage module. Make sure it is part of your project and that \
  RTOS_MODULE_FS_AVAIL is defined in rtos_description.h.

#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                               INCLUDE FILES
 ********************************************************************************************************
 *******************************************************************************************************/

#include  <cpu/include/cpu.h>
#include  <common/include/lib_mem.h>
#include  <common/include/lib_utils.h>
#include  <common/source/kal/kal_priv.h>
#include  <common/source/logging/logging_priv.h>

#include  <drivers/fs/include/fs_nor_quad_spi_drv.h>
#include  <fs/include/fs_nor_quad_spi.h>
#include  <fs/include/fs_nor.h>

/********************************************************************************************************
 ********************************************************************************************************
 *                                               LOCAL DEFINES
 ********************************************************************************************************
 *******************************************************************************************************/

#define  LOG_DFLT_CH                            (FS, DRV, NOR)
#define  RTOS_MODULE_CUR                        RTOS_CFG_MODULE_FS

//                                                                 Transfer mode.
#define  FS_SPI_XFER_MODE_SINGLE_IO                   0u
#define  FS_SPI_XFER_MODE_DUAL_IO                     1u
#define  FS_SPI_XFER_MODE_QUAD_IO                     2u

#define  FS_SPI_EN_DELAY_MIN_mS                      10u

/********************************************************************************************************
 *                                        REGISTER BIT DEFINES
 *******************************************************************************************************/

//                                                                 -------------- USART CONTROL REGISTER --------------
#define  FS_USART_REG_CTRL_AUTOTX                 DEF_BIT_29
#define  FS_USART_REG_CTRL_MSBF                   DEF_BIT_10
#define  FS_USART_REG_CTRL_CLKPHA                 DEF_BIT_09
#define  FS_USART_REG_CTRL_CLKPOL                 DEF_BIT_08
#define  FS_USART_REG_CTRL_SYNC                   DEF_BIT_00

//                                                                 ----------- USART FRAME FORMAT REGISTER ------------
#define  FS_USART_REG_FRAME_STOPBITS_ONEANDHALF   DEF_BIT_MASK(2u, 12u)
#define  FS_USART_REG_FRAME_STOPBITS_ONE          DEF_BIT_MASK(1u, 12u)
#define  FS_USART_REG_FRAME_STOPBITS_HALF         DEF_BIT_MASK(0u, 12u)
#define  FS_USART_REG_FRAME_PARITY_ODD            DEF_BIT_MASK(3u, 8u)
#define  FS_USART_REG_FRAME_PARITY_EVEN           DEF_BIT_MASK(2u, 8u)
#define  FS_USART_REG_FRAME_PARITY_NONE           DEF_BIT_MASK(0u, 8u)
#define  FS_USART_REG_FRAME_DATABITS_8            DEF_BIT_MASK(5u, 0u)
#define  FS_USART_REG_FRAME_DATABITS_7            DEF_BIT_MASK(4u, 0u)
#define  FS_USART_REG_FRAME_DATABITS_6            DEF_BIT_MASK(3u, 0u)
#define  FS_USART_REG_FRAME_DATABITS_5            DEF_BIT_MASK(2u, 0u)
#define  FS_USART_REG_FRAME_DATABITS_4            DEF_BIT_MASK(1u, 0u)

//                                                                 -------------- USART COMMAND REGISTER --------------
#define  FS_USART_REG_CMD_MASTERDIS               DEF_BIT_05
#define  FS_USART_REG_CMD_MASTEREN                DEF_BIT_04
#define  FS_USART_REG_CMD_TXDIS                   DEF_BIT_03
#define  FS_USART_REG_CMD_TXEN                    DEF_BIT_02
#define  FS_USART_REG_CMD_RXDIS                   DEF_BIT_01
#define  FS_USART_REG_CMD_RXEN                    DEF_BIT_00

//                                                                 -------------- USART STATUS REGISTER ---------------
#define  FS_USART_REG_STATUS_RXFULLRIGHT          DEF_BIT_12
#define  FS_USART_REG_STATUS_RXDATAVRIGHT         DEF_BIT_11
#define  FS_USART_REG_STATUS_TXBSRIGHT            DEF_BIT_10
#define  FS_USART_REG_STATUS_TXBDRIGHT            DEF_BIT_09
#define  FS_USART_REG_STATUS_RXFULL               DEF_BIT_08
#define  FS_USART_REG_STATUS_RXDATAV              DEF_BIT_07
#define  FS_USART_REG_STATUS_TXBL                 DEF_BIT_06
#define  FS_USART_REG_STATUS_TXC                  DEF_BIT_05
#define  FS_USART_REG_STATUS_TXTRI                DEF_BIT_04
#define  FS_USART_REG_STATUS_RXBLOCK              DEF_BIT_03
#define  FS_USART_REG_STATUS_MASTER               DEF_BIT_02
#define  FS_USART_REG_STATUS_TXENS                DEF_BIT_01
#define  FS_USART_REG_STATUS_RXENS                DEF_BIT_00

//                                                                 ----------- USART CLOCK CONTROL REGISTER -----------
#if defined(_EFM_DEVICE)
#define  FS_USART_REG_CLKDIV_DIV_MASK             DEF_BIT_FIELD(15u, 6u)
#elif defined(_EFR_DEVICE)
#define  FS_USART_REG_CLKDIV_DIV_MASK             DEF_BIT_FIELD(20u, 3u)
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                             DATA TYPES
 ********************************************************************************************************
 *******************************************************************************************************/

/********************************************************************************************************
 *                                             REGISTERS
 *******************************************************************************************************/

//                                                                 ---------------- USART REGISTERS MAP ---------------
typedef  struct  usart_reg {
  CPU_REG32 CTRL;                                               // Control Register
  CPU_REG32 FRAME;                                              // USART Frame Format Register
  CPU_REG32 TRIGCTRL;                                           // USART Trigger Control register
  CPU_REG32 CMD;                                                // Command Register
  CPU_REG32 STATUS;                                             // USART Status Register
  CPU_REG32 CLKDIV;                                             // Clock Control Register
  CPU_REG32 RXDATAX;                                            // RX Buffer Data Extended Register
  CPU_REG32 RXDATA;                                             // RX Buffer Data Register
  CPU_REG32 RXDOUBLEX;                                          // RX Buffer Double Data Extended Register
  CPU_REG32 RXDOUBLE;                                           // RX FIFO Double Data Register
  CPU_REG32 RXDATAXP;                                           // RX Buffer Data Extended Peek Register
  CPU_REG32 RXDOUBLEXP;                                         // RX Buffer Double Data Extended Peek Register
  CPU_REG32 TXDATAX;                                            // TX Buffer Data Extended Register
  CPU_REG32 TXDATA;                                             // TX Buffer Data Register
  CPU_REG32 TXDOUBLEX;                                          // TX Buffer Double Data Extended Register
  CPU_REG32 TXDOUBLE;                                           // TX Buffer Double Data Register
  CPU_REG32 IF;                                                 // Interrupt Flag Register
  CPU_REG32 IFS;                                                // Interrupt Flag Set Register
  CPU_REG32 IFC;                                                // Interrupt Flag Clear Register
  CPU_REG32 IEN;                                                // Interrupt Enable Register
  CPU_REG32 IRCTRL;                                             // IrDA Control Register
#if defined(_EFM_DEVICE)
  CPU_REG32 ROUTE;                                              // I/O Routing Register
#elif defined(_EFR_DEVICE)
  CPU_REG32 RESERVED0[1];                                       // Reserved for future use
#endif
  CPU_REG32 INPUT;                                              // USART Input Register
  CPU_REG32 I2SCTRL;                                            // I2S Control Register
#if defined(_EFR_DEVICE)
  CPU_REG32 TIMING;                                             // Timing Register
  CPU_REG32 CTRLX;                                              // Control Register Extended
  CPU_REG32 TIMECMP0;                                           // Used to generate interrupts and various delays
  CPU_REG32 TIMECMP1;                                           // Used to generate interrupts and various delays
  CPU_REG32 TIMECMP2;                                           // Used to generate interrupts and various delays
  CPU_REG32 ROUTEPEN;                                           // I/O Routing Pin Enable Register
  CPU_REG32 ROUTELOC0;                                          // I/O Routing Location Register
  CPU_REG32 ROUTELOC1;                                          // I/O Routing Location Register
#endif
} USART_REG;

//                                                                 ------------------- DRIVER DATA --------------------
typedef  struct spi_data {
  FS_NOR_QUAD_SPI_CTRLR_INFO *SPI_CtrlrInfoPtr;                 // Pointer to SPI controller info.
  CPU_INT08U                 ChipSelID;                         // Chip select ID to which flash device is connected.
} SPI_DATA;

/********************************************************************************************************
 ********************************************************************************************************
 *                                      LOCAL FUNCTION PROTOTYPES
 ********************************************************************************************************
 *******************************************************************************************************/

/********************************************************************************************************
 *                                DRIVER INTERFACE FUNCTION PROTOTYPES
 *******************************************************************************************************/

static void *FS_NOR_SPI_Add(const FS_NOR_QUAD_SPI_CTRLR_INFO *p_ctrlr_info,
                            const FS_NOR_QUAD_SPI_SLAVE_INFO *p_slave_info,
                            MEM_SEG                          *p_seg,
                            RTOS_ERR                         *p_err);

static void FS_NOR_SPI_Start(void     *p_drv_data,
                             RTOS_ERR *p_err);

static void FS_NOR_SPI_Stop(void     *p_drv_data,
                            RTOS_ERR *p_err);

static void FS_NOR_SPI_ClkSet(void       *p_drv_data,
                              CPU_INT32U clk,
                              RTOS_ERR   *p_err);

static void FS_NOR_SPI_CmdSend(void                           *p_drv_data,
                               const FS_NOR_QUAD_SPI_CMD_DESC *p_cmd,
                               CPU_INT08U                     addr_tbl[],
                               CPU_INT08U                     inter_data[],
                               CPU_INT08U                     inter_cycles,
                               void                           *p_xfer_data,
                               CPU_INT32U                     xfer_size,
                               RTOS_ERR                       *p_err);

static CPU_SIZE_T FS_NOR_SPI_AlignReqGet(void     *p_drv_data,
                                         RTOS_ERR *p_err);

/********************************************************************************************************
 *                                       LOCAL FUNCTION PROTOTYPES
 *******************************************************************************************************/

static CPU_INT08U FS_NOR_SPI_Transfer(USART_REG  *p_reg,
                                      CPU_INT08U data);

/********************************************************************************************************
 ********************************************************************************************************
 *                                           GLOBAL VARIABLES
 ********************************************************************************************************
 *******************************************************************************************************/

const FS_NOR_QUAD_SPI_DRV_API FS_NOR_SpiDrvAPI_Silabs_EFM32 = {
  .Add = FS_NOR_SPI_Add,
  .Start = FS_NOR_SPI_Start,
  .Stop = FS_NOR_SPI_Stop,
  .ClkSet = FS_NOR_SPI_ClkSet,
  .DTR_Set = DEF_NULL,
  .FlashSizeSet = DEF_NULL,
  .CmdSend = FS_NOR_SPI_CmdSend,
  .WaitWhileBusy = DEF_NULL,
  .AlignReqGet = FS_NOR_SPI_AlignReqGet,
  .XipBitSet = DEF_NULL,
  .XipCfg = DEF_NULL
};

/********************************************************************************************************
 ********************************************************************************************************
 *                                           LOCAL FUNCTIONS
 ********************************************************************************************************
 *******************************************************************************************************/

/****************************************************************************************************//**
 *                                          FS_NOR_SPI_Add()
 *
 * @brief   Add a SPI controller driver instance.
 *
 * @param   p_ctrlr_info    Pointer to a controller information structure.
 *
 * @param   p_slave_info    Pointer to a slave information structure.
 *
 * @param   p_seg           Pointer to a memory segment where to allocate the controller driver instance.
 *
 * @param   p_err           Error pointer.
 *
 * @return  Pointer to driver-specific data, if NO error(s).
 *          Null pointer,                    otherwise.
 *******************************************************************************************************/
static void *FS_NOR_SPI_Add(const FS_NOR_QUAD_SPI_CTRLR_INFO *p_ctrlr_info,
                            const FS_NOR_QUAD_SPI_SLAVE_INFO *p_slave_info,
                            MEM_SEG                          *p_seg,
                            RTOS_ERR                         *p_err)
{
  SPI_DATA    *p_spi_drv_data = DEF_NULL;
  USART_REG   *p_reg = (USART_REG *)p_ctrlr_info->BaseAddr;
  CPU_INT32U  reg_val;
  CPU_BOOLEAN ok = DEF_FAIL;

  //                                                               Alloc drv private data.
  p_spi_drv_data = (SPI_DATA *)Mem_SegAlloc(DEF_NULL,
                                            p_seg,
                                            sizeof(SPI_DATA),
                                            p_err);
  if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
    RTOS_ERR_CONTEXT_REFRESH(*p_err);
    return (DEF_NULL);
  }
  Mem_Clr(p_spi_drv_data, sizeof(SPI_DATA));

  //                                                               Save SPI controller info for later usage by drv.
  p_spi_drv_data->SPI_CtrlrInfoPtr = (FS_NOR_QUAD_SPI_CTRLR_INFO *)p_ctrlr_info;

  //                                                               --------------------- BSP INIT ---------------------
  if (p_ctrlr_info->BspApiPtr->Init != DEF_NULL) {
    ok = p_ctrlr_info->BspApiPtr->Init(DEF_NULL,                // DEF_NULL = no intr used by this SPI drv.
                                       p_spi_drv_data);
    if (ok != DEF_OK) {
      RTOS_ERR_SET(*p_err, RTOS_ERR_IO);
      return (DEF_NULL);
    }
  }
  //                                                               Cfg clock(s) required by SPI ctrlr.
  if (p_ctrlr_info->BspApiPtr->ClkCfg != DEF_NULL) {
    ok = p_ctrlr_info->BspApiPtr->ClkCfg();
    if (ok != DEF_OK) {
      RTOS_ERR_SET(*p_err, RTOS_ERR_IO);
      return (DEF_NULL);
    }
  }
  //                                                               Cfg I/O needed by SPI ctrlr if necessary.
  if (p_ctrlr_info->BspApiPtr->IO_Cfg != DEF_NULL) {
    ok = p_ctrlr_info->BspApiPtr->IO_Cfg();
    if (ok != DEF_OK) {
      RTOS_ERR_SET(*p_err, RTOS_ERR_IO);
      return (DEF_NULL);
    }
  }
  //                                                               Cfg interrupt controller if necessary.
  if (p_ctrlr_info->BspApiPtr->IntCfg != DEF_NULL) {
    ok = p_ctrlr_info->BspApiPtr->IntCfg();
    if (ok != DEF_OK) {
      RTOS_ERR_SET(*p_err, RTOS_ERR_IO);
      return (DEF_NULL);
    }
  }

  reg_val = p_reg->CTRL;
  reg_val |= FS_USART_REG_CTRL_SYNC;                            // USART operates in synchronous mode
  switch (p_slave_info->Mode) {
    case FS_NOR_SPI_MODE_0:                                     // CLKPOL_IDLELOW, CLKPHA_SAMPLELEADING
      reg_val &= ~(FS_USART_REG_CTRL_CLKPHA | FS_USART_REG_CTRL_CLKPOL);
      break;

    case FS_NOR_SPI_MODE_3:                                     // CLKPOL_IDLEHIGH, CLKPHA_SAMPLETRAILING
      reg_val |= (FS_USART_REG_CTRL_CLKPHA | FS_USART_REG_CTRL_CLKPOL);
      break;

    case FS_NOR_SPI_MODE_1:
    case FS_NOR_SPI_MODE_2:
    default:
      RTOS_ERR_SET(*p_err, RTOS_ERR_INIT);
      return (DEF_NULL);
  }

  reg_val |= FS_USART_REG_CTRL_MSBF;                            // Data is sent with the most significant bit first.
  reg_val &= ~(FS_USART_REG_CTRL_AUTOTX);

  p_reg->CTRL = reg_val;

  p_reg->FRAME = (FS_USART_REG_FRAME_DATABITS_8                 // Each frame contains 8 data bits
                  | FS_USART_REG_FRAME_STOPBITS_ONE             // One stop bit is generated and verified.
                  | FS_USART_REG_FRAME_PARITY_NONE);            // Parity bits are not used.

  p_reg->CMD = FS_USART_REG_CMD_MASTEREN;                       // Enable Master mode.

  p_spi_drv_data->ChipSelID = p_slave_info->ChipSelID;

  return ((void *)p_spi_drv_data);
}

/****************************************************************************************************//**
 *                                         FS_NOR_SPI_ClkSet()
 *
 * @brief   Set serial clock frequency outputted by the SPI controller to the flash device.
 *
 * @param   p_drv_data    Pointer to driver-specific data.
 *
 * @param   clk           Serial clock frequency.
 *
 * @param   p_err         Error pointer.
 *******************************************************************************************************/
static void FS_NOR_SPI_ClkSet(void       *p_drv_data,
                              CPU_INT32U clk,
                              RTOS_ERR   *p_err)
{
  FS_NOR_QUAD_SPI_CTRLR_INFO *p_ctrlr_info = ((SPI_DATA *)p_drv_data)->SPI_CtrlrInfoPtr;
  USART_REG                  *p_reg = (USART_REG *)p_ctrlr_info->BaseAddr;
  CPU_INT32U                 per_clk = 0u;
  CPU_INT32U                 clk_div;

  if (p_ctrlr_info->BspApiPtr->ClkFreqGet != DEF_NULL) {
    per_clk = p_ctrlr_info->BspApiPtr->ClkFreqGet();
  }

  if (per_clk == 0u) {
    RTOS_ERR_SET(*p_err, RTOS_ERR_INIT);
    return;
  }

  clk_div = (per_clk - 1) / (2 * clk);
  clk_div = clk_div << 8;

  if (clk_div & ~FS_USART_REG_CLKDIV_DIV_MASK) {                // Check if clock divider is within limits
    RTOS_ERR_SET(*p_err, RTOS_ERR_IO_FATAL);
    return;
  }

  p_reg->CLKDIV = clk_div;
}

/****************************************************************************************************//**
 *                                         FS_NOR_SPI_Start()
 *
 * @brief   Start controller operation.
 *
 * @param   p_drv_data      Pointer to driver-specific data.
 *
 * @param   p_err           Error pointer.
 *******************************************************************************************************/
static void FS_NOR_SPI_Start(void     *p_drv_data,
                             RTOS_ERR *p_err)
{
  FS_NOR_QUAD_SPI_CTRLR_INFO *p_ctrlr_info = ((SPI_DATA *)p_drv_data)->SPI_CtrlrInfoPtr;
  USART_REG                  *p_reg = (USART_REG *)p_ctrlr_info->BaseAddr;

  PP_UNUSED_PARAM(p_err);

  p_reg->CMD = (FS_USART_REG_CMD_RXEN                           // Set to activate data reception.
                | FS_USART_REG_CMD_TXEN);                       // Set to enable data transmission.

  KAL_Dly(FS_SPI_EN_DELAY_MIN_mS);                              // Wait before issuing 1st cmd after SPI ctrlr start.
}

/****************************************************************************************************//**
 *                                          FS_NOR_SPI_Stop()
 *
 * @brief   Stop controller operation.
 *
 * @param   p_drv_data     Pointer to driver-specific data.
 *
 * @param   p_err          Error pointer.
 *******************************************************************************************************/
static void FS_NOR_SPI_Stop(void     *p_drv_data,
                            RTOS_ERR *p_err)
{
  FS_NOR_QUAD_SPI_CTRLR_INFO *p_ctrlr_info = ((SPI_DATA *)p_drv_data)->SPI_CtrlrInfoPtr;
  USART_REG                  *p_reg = (USART_REG *)p_ctrlr_info->BaseAddr;

  PP_UNUSED_PARAM(p_err);

  p_reg->CMD = (FS_USART_REG_CMD_RXDIS                          // Set to disable data reception.
                | FS_USART_REG_CMD_TXDIS);                      // Set to disable data transmission.
}

/****************************************************************************************************//**
 *                                        FS_NOR_SPI_CmdSend()
 *
 * @brief   Send a command.
 *
 * @param   p_drv_data      Pointer to driver-specific data.
 *
 * @param   p_cmd           Pointer to a command descriptor.
 *
 * @param   addr_tbl        Source / Destination address table.
 *
 * @param   inter_data      Inter data table.
 *
 * @param   inter_cycles    Inter cycle cnt.
 *
 * @param   p_xfer_data     Pointer to a buffer that contains data to be written or that receives
 *                          data to be read.
 *
 * @param   xfer_size       Number of octets to be read / written.
 *
 * @param   p_err           Error pointer.
 *
 * @note    (1) The NOR command can have one of the following formats:
 *
 *              - (a) OPCODE (single operation such as reset, write enable)
 *              - (b) OPCODE + DATA (flash register read/write access)
 *              - (c) OPCODE + ADDRESS (erase operation)
 *              - (d) OPCODE + ADDRESS + DATA (memory read/write access in indirect/direct mode)
 *
 *******************************************************************************************************/
static void FS_NOR_SPI_CmdSend(void                           *p_drv_data,
                               const FS_NOR_QUAD_SPI_CMD_DESC *p_cmd,
                               CPU_INT08U                     addr_tbl[],
                               CPU_INT08U                     inter_data[],
                               CPU_INT08U                     inter_cycles,
                               void                           *p_xfer_data,
                               CPU_INT32U                     xfer_size,
                               RTOS_ERR                       *p_err)
{
  SPI_DATA                   *p_spi_drv_data = (SPI_DATA *)p_drv_data;
  FS_NOR_QUAD_SPI_CTRLR_INFO *p_ctrlr_info = p_spi_drv_data->SPI_CtrlrInfoPtr;
  USART_REG                  *p_reg = (USART_REG *)p_ctrlr_info->BaseAddr;
  CPU_INT08U                 *p_buffer = (CPU_INT08U *)p_xfer_data;
  CPU_INT32U                 index;

  if (p_cmd->Form.MultiIO_Quad == DEF_YES) {
    RTOS_ERR_SET(*p_err, RTOS_ERR_IO);
    return;
  }

  if (p_ctrlr_info->BspApiPtr->ChipSelEn != DEF_NULL) {
    p_ctrlr_info->BspApiPtr->ChipSelEn(p_spi_drv_data->ChipSelID);
  }

  FS_NOR_SPI_Transfer(p_reg, p_cmd->Opcode);                    // Send OPCODE

  if (p_cmd->Form.HasAddr == DEF_TRUE) {                        // Check if Flash address must be sent
    FS_NOR_SPI_Transfer(p_reg, addr_tbl[2u]);
    FS_NOR_SPI_Transfer(p_reg, addr_tbl[1u]);
    FS_NOR_SPI_Transfer(p_reg, addr_tbl[0u]);

    if (inter_cycles != DEF_NULL) {                             // Send dummy byte before FREAD begins.
      FS_NOR_SPI_Transfer(p_reg, inter_data[0u]);
    }
  }

  for (index = 0; index < xfer_size; index++) {
    if (p_cmd->Form.IsWr == DEF_YES) {                          // Write DATA
      FS_NOR_SPI_Transfer(p_reg, *p_buffer);
    } else {                                                    // Read DATA
      *p_buffer = FS_NOR_SPI_Transfer(p_reg, 0xFFu);
    }
    p_buffer++;
  }

  if (p_ctrlr_info->BspApiPtr->ChipSelDis != DEF_NULL) {
    p_ctrlr_info->BspApiPtr->ChipSelDis(p_spi_drv_data->ChipSelID);
  }
}

/****************************************************************************************************//**
 *                                     FS_NOR_SPI_AlignReqGet()
 *
 * @brief   Get buffer alignment requirement of the controller.
 *
 * @param   p_drv_data      Pointer to driver-specific data.
 *
 * @param   p_err           Error pointer.
 *
 * @return  buffer alignment requirement in octets.
 *
 *******************************************************************************************************/
static CPU_SIZE_T FS_NOR_SPI_AlignReqGet(void     *p_drv_data,
                                         RTOS_ERR *p_err)
{
  SPI_DATA *p_spi_drv_data = (SPI_DATA *)p_drv_data;

  PP_UNUSED_PARAM(p_err);

  return (p_spi_drv_data->SPI_CtrlrInfoPtr->AlignReq);
}

/********************************************************************************************************
 ********************************************************************************************************
 *                                       LOCAL FUNCTION PROTOTYPES
 ********************************************************************************************************
 *******************************************************************************************************/

/****************************************************************************************************//**
 *                                     FS_NOR_SPI_Transfer()
 *
 * @brief   Get buffer alignment requirement of the controller.
 *
 * @param   p_reg      Pointer to peripheral register block.
 *
 * @param   data       Data to transmit.
 *
 * @return  Data received.
 *******************************************************************************************************/
static CPU_INT08U FS_NOR_SPI_Transfer(USART_REG  *p_reg,
                                      CPU_INT08U data)
{
  while (DEF_BIT_IS_CLR(p_reg->STATUS, FS_USART_REG_STATUS_TXBL)) {
    ;
  }
  p_reg->TXDATA = (CPU_INT32U)data;

  while (DEF_BIT_IS_CLR(p_reg->STATUS, FS_USART_REG_STATUS_TXC)) {
    ;
  }

  return ((CPU_INT08U)p_reg->RXDATA);
}

/********************************************************************************************************
 ********************************************************************************************************
 *                                   DEPENDENCIES & AVAIL CHECK(S) END
 ********************************************************************************************************
 *******************************************************************************************************/

#endif // RTOS_MODULE_FS_NOR_AVAIL
