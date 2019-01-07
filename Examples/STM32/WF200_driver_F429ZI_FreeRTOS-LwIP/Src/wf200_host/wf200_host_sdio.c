/*
 * EVALUATION AND USE OF THIS SOFTWARE IS SUBJECT TO THE TERMS AND
 * CONDITIONS OF THE CONTROLLING LICENSE AGREEMENT FOUND AT LICENSE.md
 * IN THIS SDK. IF YOU DO NOT AGREE TO THE LICENSE TERMS AND CONDITIONS,
 * PLEASE RETURN ALL SOURCE FILES TO SILICON LABORATORIES.
 * (c) Copyright 2018, Silicon Laboratories Inc.  All rights reserved.
 */

/*
 *  Bus low level operations that dependent on the underlying physical bus : sdio implementation
 */

#include "wf200_host_sdio.h"

extern MMC_HandleTypeDef hmmc;
extern DMA_HandleTypeDef hdma_sdio_rx;
extern DMA_HandleTypeDef hdma_sdio_tx;

sl_status_t wf200_host_sdio_enable_high_speed_mode( void ){
  MMC_InitTypeDef Init;
    
  Init.ClockEdge           = SDIO_CLOCK_EDGE_RISING;
  Init.ClockBypass         = SDIO_CLOCK_BYPASS_DISABLE; //enable to reach 50MHz
  Init.ClockPowerSave      = SDIO_CLOCK_POWER_SAVE_DISABLE;
  Init.BusWide             = SDIO_BUS_WIDE_4B;
  Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
  Init.ClockDiv            = 2; //12.5MHz speed
  SDIO_Init(SDIO, Init);
  return SL_SUCCESS;
}

sl_status_t wf200_host_bus_init( void )
{
  uint32_t errorstate = SDMMC_ERROR_NONE;

  __HAL_MMC_ENABLE_IT(&hmmc, SDIO_IT_SDIOIT);
  SDIO->DCTRL |= SDIO_DCTRL_SDIOEN;
  hmmc.Context = MMC_CONTEXT_IT;
  
  /* CMD0: GO_IDLE_STATE */
  errorstate = SDMMC_CmdGoIdleState(SDIO);
  if(errorstate != SDMMC_ERROR_NONE)
  {
    return SL_ERROR;
  }
  HAL_Delay(1); //Mandatory because Ineo reply to cmd0, TODO: maybe wait for command received
  /* CMD8 */
  errorstate = SDMMC_CmdOperCond(SDIO);
  if(errorstate != SDMMC_ERROR_NONE)
  {  
    return SL_ERROR;
  }
  /* CMD3 */
  errorstate = SDMMC_CmdSetRelAdd(SDIO, NULL);
  if(errorstate != SDMMC_ERROR_NONE)
  {  
    return SL_ERROR;
  }
  /* CMD7 */
  errorstate = SDMMC_CmdSelDesel(SDIO, 0x00010000);
  if(errorstate != SDMMC_ERROR_NONE)
  {  
    return SL_ERROR;
  }
  
  return SL_SUCCESS;
}

/* Command 52 */

static uint32_t SDMMC_GetCmdResp(SDIO_TypeDef *SDIOx)
{
  /* 8 is the number of required instructions cycles for the below loop statement.
  The SDMMC_CMDTIMEOUT is expressed in ms */
  register uint32_t count = SDIO_CMDTIMEOUT * (SystemCoreClock / 8U /1000U);
  
  do
  {
    if (count-- == 0U)
    {
      return SDMMC_ERROR_TIMEOUT;
    }
    
  }while(!__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT));
    
  if (__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CTIMEOUT))
  {
    __SDIO_CLEAR_FLAG(SDIOx, SDIO_FLAG_CTIMEOUT);
    
    return SDMMC_ERROR_CMD_RSP_TIMEOUT;
  }
  else if (__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CCRCFAIL))
  {
    __SDIO_CLEAR_FLAG(SDIOx, SDIO_FLAG_CCRCFAIL);
    
    return SDMMC_ERROR_CMD_CRC_FAIL;
  }
  else
  {
    /* No error flag set */
    /* Clear all the static flags */
    __SDIO_CLEAR_FLAG(SDIOx, SDIO_STATIC_FLAGS);
  }

  return SDMMC_ERROR_NONE;
}

sl_status_t wf200_host_sdio_transfer_cmd52( wf200_host_bus_tranfer_type_t type, uint8_t function, uint32_t address, uint8_t* buffer ){
  uint32_t status;
  SDIO_CmdInitTypeDef command;
  while(hmmc.State != HAL_MMC_STATE_READY);
  if(type == WF200_BUS_WRITE){
    /*Argument to write on the SDIO bus*/
    command.Argument = 0x80000000 | // 0x80000000 for write, 0x00000000 for read
      (function << 28) | // function number
        (address << 9)
          | *buffer; // length
  }else{
    /*Argument to read on the SDIO bus*/
    command.Argument = 0x00000000 | // 0x80000000 for write, 0x00000000 for read
      (function << 28) | // function number
        (address << 9); // length
  }
  command.CmdIndex = SDMMC_CMD_SDMMC_RW_DIRECT;
  
  command.Response = SDIO_RESPONSE_SHORT;
  command.WaitForInterrupt = SDIO_WAIT_NO;
  command.CPSM     = SDIO_CPSM_ENABLE;
  SDIO_SendCommand(SDIO, &command);
  status = SDMMC_GetCmdResp(SDIO);
  if(status == SDMMC_ERROR_NONE)
  {
    uint8_t response_flags = (SDIO_GetResponse(SDIO, SDIO_RESP1) >> 8) & 0xFF;
    if(response_flags == 0x10)
    {
      *buffer = SDIO_GetResponse(SDIO, SDIO_RESP1) & 0xFF;
    }
    else
    {
      return SL_ERROR;
    }
  }
  return SL_SUCCESS;
}

/* Command 53 */

static sl_status_t sdio_io_write_extended( uint8_t function, uint32_t address, uint8_t* data, uint32_t data_length )
{
  uint32_t status;
  SDIO_CmdInitTypeDef command;
  SDIO_DataInitTypeDef config;
  
  if(hmmc.State == HAL_MMC_STATE_READY)
  {
    hmmc.ErrorCode = HAL_MMC_ERROR_NONE;
    hmmc.State = HAL_MMC_STATE_BUSY;
    
    /* Initialize data control register */
    hmmc.Instance->DCTRL = 0U;
    hmmc.Instance->DCTRL |= SDIO_DCTRL_SDIOEN;
    hmmc.pTxBuffPtr = (uint32_t *)data;
    hmmc.TxXferSize = data_length;
    
    /* Enable transfer interrupts */
    __HAL_MMC_CLEAR_FLAG(&hmmc, SDIO_STATIC_FLAGS); 
    __HAL_MMC_ENABLE_IT(&hmmc, (SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT | SDIO_IT_TXUNDERR | SDIO_IT_DATAEND | SDIO_FLAG_TXFIFOHE)); 
    
    command.CmdIndex = SDMMC_CMD_SDMMC_RW_EXTENDED;
    command.Argument = SDIO_CMD53_WRITE | SDIO_CMD53_FUNCTION( function ) | SDIO_CMD53_OPMODE_INCREASING_ADDRESS | SDIO_CMD53_ADDRESS( address );
    command.Response = SDIO_RESPONSE_SHORT;
    command.WaitForInterrupt = SDIO_WAIT_NO;
    command.CPSM     = SDIO_CPSM_ENABLE;
    if( data_length >= SDIO_BLOCK_MODE_THRESHOLD )
    {
      uint32_t block_count = ( data_length / SDIO_BLOCK_SIZE ) + ( ( ( data_length % SDIO_BLOCK_SIZE ) == 0 ) ? 0 : 1 );
      command.Argument |= SDIO_CMD53_BLOCK_MODE | SDIO_CMD53_COUNT( block_count );
      config.TransferMode  = SDIO_TRANSFER_MODE_BLOCK;
      config.DataBlockSize = SDIO_DATABLOCK_SIZE_512B;
    }
    else
    {
      command.Argument |= SDIO_CMD53_COUNT( data_length );
      config.TransferMode  = SDIO_TRANSFER_MODE_STREAM;
      config.DataBlockSize = SDIO_DATABLOCK_SIZE_64B;
      hmmc.Instance->DCTRL |= SDIO_DCTRL_SDIOEN;
    }
    SDIO_SendCommand(SDIO, &command);
    status = SDMMC_GetCmdResp(SDIO);
    if(status == SDMMC_ERROR_NONE)
    {
      uint8_t response_flags = (SDIO_GetResponse(SDIO, SDIO_RESP1) >> 8) & 0xFF;
      if(response_flags != 0x20)
      {
        return SL_ERROR;
      }
    }
    
    /* Configure the MMC DPSM (Data Path State Machine) */ 
    config.DataTimeOut   = SDMMC_DATATIMEOUT;
    config.DataLength    = data_length;
    config.TransferDir   = SDIO_TRANSFER_DIR_TO_CARD;
    config.DPSM          = SDIO_DPSM_ENABLE;
    SDIO_ConfigData(SDIO, &config);
    
    return SL_SUCCESS;
  }
  else
  {
    return SL_ERROR;
  }
}

static sl_status_t sdio_io_read_extended( uint8_t function, uint32_t address, uint8_t* data, uint32_t data_length )
{
  uint32_t status;
  SDIO_CmdInitTypeDef command;
  SDIO_DataInitTypeDef config;
  
  if(hmmc.State == HAL_MMC_STATE_READY)
  {
    hmmc.ErrorCode = HAL_DMA_ERROR_NONE;
    hmmc.State = HAL_MMC_STATE_BUSY;
    
    /* Initialize data control register */
    hmmc.Instance->DCTRL = 0U;
    hmmc.Instance->DCTRL |= SDIO_DCTRL_SDIOEN;
    hmmc.pRxBuffPtr = (uint32_t *)data;
    hmmc.RxXferSize = data_length;
    
    __HAL_MMC_CLEAR_FLAG(&hmmc, SDIO_STATIC_FLAGS); 
    __HAL_MMC_ENABLE_IT(&hmmc, (SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT | SDIO_IT_RXOVERR | SDIO_IT_DATAEND | SDIO_IT_RXDAVL));
    
    command.CmdIndex = SDMMC_CMD_SDMMC_RW_EXTENDED;
    command.Argument = SDIO_CMD53_FUNCTION( function ) | SDIO_CMD53_OPMODE_INCREASING_ADDRESS | SDIO_CMD53_ADDRESS( address );
    command.Response = SDIO_RESPONSE_SHORT;
    command.WaitForInterrupt = SDIO_WAIT_NO;
    command.CPSM     = SDIO_CPSM_ENABLE;
    if( data_length >= SDIO_BLOCK_MODE_THRESHOLD )
    {
      uint32_t block_count = ( data_length / SDIO_BLOCK_SIZE ) + ( ( ( data_length % SDIO_BLOCK_SIZE ) == 0 ) ? 0 : 1 );
      command.Argument |= SDIO_CMD53_BLOCK_MODE | SDIO_CMD53_COUNT( block_count );
      config.TransferMode  = SDIO_TRANSFER_MODE_BLOCK;
      config.DataBlockSize = SDIO_DATABLOCK_SIZE_512B;
    }
    else
    {
      command.Argument |= SDIO_CMD53_COUNT( data_length );
      config.TransferMode  = SDIO_TRANSFER_MODE_STREAM;
      config.DataBlockSize = SDIO_DATABLOCK_SIZE_64B;
      SDIO->DCTRL |= SDIO_DCTRL_SDIOEN;
    }
    config.DataTimeOut   = SDMMC_DATATIMEOUT;
    config.DataLength    = data_length;
    config.TransferDir   = SDIO_TRANSFER_DIR_TO_SDIO;
    config.DPSM          = SDIO_DPSM_ENABLE;
    SDIO_ConfigData(SDIO, &config);
    
    SDIO_SendCommand(SDIO, &command);
    status = SDMMC_GetCmdResp(SDIO);
    if(status == SDMMC_ERROR_NONE)
    {
      uint8_t response_flags = (SDIO_GetResponse(SDIO, SDIO_RESP1) >> 8) & 0xFF;
      if(response_flags != 0x20)
      {
        return SL_ERROR;
      }
    }
    
    return SL_SUCCESS;
  }
  else
  {
    return SL_ERROR;
  }
}

sl_status_t wf200_host_sdio_transfer_cmd53( wf200_host_bus_tranfer_type_t type, uint8_t function, uint32_t address, uint8_t* buffer, uint16_t buffer_length ){    
  sl_status_t status;
  if(type == WF200_BUS_WRITE){
    status = sdio_io_write_extended(function, address, (uint8_t*)buffer, buffer_length);
  }else{
    status = sdio_io_read_extended(function, address, (uint8_t*)buffer, buffer_length);
  }
  /*Wait for SDIO rdy*/
  while(hmmc.State != HAL_MMC_STATE_READY);
  return status;
}

sl_status_t wf200_host_enable_platform_interrupt( void )
{
  //Done in main due to STM32CubeMX format (To be moved?)
  return SL_SUCCESS;
}


sl_status_t wf200_host_disable_platform_interrupt( void )
{
  //TODO: add deinit from CubeMX template
  return SL_SUCCESS;
}