/**************************************************************************//**
 * Copyright 2018, Silicon Laboratories Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

/**************************************************************************//**
 * SDIO interface implementation: STM32F4 + Bare Metal
 *****************************************************************************/

#include "sl_wfx.h"
#include "stm32f4xx_hal.h"

/* SDIO CMD53 argument */
#define SDIO_CMD53_WRITE                       ( 1 << 31 )
#define SDIO_CMD53_FUNCTION( function )        ( ( ( function ) & 0x7 ) << 28 )
#define SDIO_CMD53_BLOCK_MODE                  ( 1 << 27 )
#define SDIO_CMD53_OPMODE_INCREASING_ADDRESS   ( 1 << 26 )
#define SDIO_CMD53_ADDRESS( address )          ( ( ( address ) & 0x1ffff ) << 9 )
#define SDIO_CMD53_COUNT( count )              ( ( count ) & 0x1ff )

#define SDIO_CMD53_IS_BLOCK_MODE( arg )        ( ( ( arg ) & SDIO_CMD53_BLOCK_MODE ) != 0 )
#define SDIO_CMD53_GET_COUNT( arg )            ( SDIO_CMD53_COUNT( arg ) )

static void MX_SDIO_Init(void);
static void MX_SDIO_DeInit(void);
static uint32_t sdio_optimal_block_size( uint16_t buffer_size );
static uint32_t SDMMC_GetCmdResp(SDIO_TypeDef *SDIOx);
extern void HAL_SDIO_MspInit(void);

DMA_HandleTypeDef hdma_sdio_rx;
DMA_HandleTypeDef hdma_sdio_tx;
   
sl_status_t sl_wfx_host_sdio_enable_high_speed_mode( void )
{
  SDIO_InitTypeDef Init;
  Init.ClockEdge           = SDIO_CLOCK_EDGE_RISING;
  Init.ClockBypass         = SDIO_CLOCK_BYPASS_DISABLE;
  Init.ClockPowerSave      = SDIO_CLOCK_POWER_SAVE_DISABLE;
  Init.BusWide             = SDIO_BUS_WIDE_4B;
  Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
  Init.ClockDiv            = 2;

  SDIO_Init(SDIO, Init);
  SDIO_PowerState_ON(SDIO);
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_init_bus( void )
{
  uint32_t errorstate = SDMMC_ERROR_NONE;
  
  MX_SDIO_Init();
  
  /* CMD0: GO_IDLE_STATE */
  errorstate = SDMMC_CmdGoIdleState(SDIO);
  if(errorstate != SDMMC_ERROR_NONE)
  {
    return SL_ERROR;
  }
  sl_wfx_host_wait(1); //Mandatory because wf200 reply to cmd0,
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

sl_status_t sl_wfx_host_deinit_bus( void )
{
  MX_SDIO_DeInit();
  return SL_SUCCESS;
}

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
  else if(__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CMDREND))
  {
    __SDIO_CLEAR_FLAG(SDIOx, SDIO_FLAG_CMDREND);
  }
  return SDMMC_ERROR_NONE;
}

/* Command 52 */
sl_status_t sl_wfx_host_sdio_transfer_cmd52( sl_wfx_host_bus_tranfer_type_t type, uint8_t function, uint32_t address, uint8_t* buffer ){
  uint32_t status;
  SDIO_CmdInitTypeDef command;
  if(type == SL_WFX_BUS_WRITE){
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
  command.CmdIndex         = SDMMC_CMD_SDMMC_RW_DIRECT; 
  command.Response         = SDIO_RESPONSE_SHORT;
  command.WaitForInterrupt = SDIO_WAIT_NO;
  command.CPSM             = SDIO_CPSM_ENABLE;
  SDIO_SendCommand(SDIO, &command);
  status = SDMMC_GetCmdResp(SDIO);
  
  if(status == SDMMC_ERROR_NONE)
  {
    uint32_t response_flags = SDIO_GetResponse(SDIO, SDIO_RESP1);
    if((response_flags & 0xFF00) == 0x1000)
    {
      *buffer = response_flags & 0xFF;
    }else{
      return SL_ERROR;
    }
  }else{
    return SL_ERROR;
  }
  
  return SL_SUCCESS;
}

/* Command 53 */
sl_status_t sl_wfx_host_sdio_read_cmd53( uint8_t function, uint32_t address, uint8_t* buffer, uint16_t buffer_length )
{    
  SDIO_CmdInitTypeDef command;
  SDIO_DataInitTypeDef config;
  uint32_t *tempbuff = (uint32_t *)buffer;
  SDIO->DCTRL = 0U;  
  
  if( buffer_length >= SL_WFX_SDIO_BLOCK_MODE_THRESHOLD )
  {
    uint32_t block_count = ( buffer_length / SL_WFX_SDIO_BLOCK_SIZE ) + ( ( ( buffer_length % SL_WFX_SDIO_BLOCK_SIZE ) == 0 ) ? 0 : 1 );
    command.Argument = SDIO_CMD53_BLOCK_MODE | SDIO_CMD53_COUNT( block_count );
    config.TransferMode  = SDIO_TRANSFER_MODE_BLOCK;
    config.DataBlockSize = sdio_optimal_block_size(SL_WFX_SDIO_BLOCK_SIZE);
  }
  else
  {
    command.Argument = SDIO_CMD53_COUNT( buffer_length );
    config.TransferMode  = SDIO_TRANSFER_MODE_STREAM;
    config.DataBlockSize = sdio_optimal_block_size(buffer_length);
  }
  command.Argument |= SDIO_CMD53_FUNCTION( function ) | SDIO_CMD53_OPMODE_INCREASING_ADDRESS | SDIO_CMD53_ADDRESS( address );
  config.TransferDir   = SDIO_TRANSFER_DIR_TO_SDIO;
  
  SDIO->DCTRL |= SDIO_DCTRL_SDIOEN;
  /* Prepare Data */
  config.DataTimeOut   = SDMMC_DATATIMEOUT;
  config.DataLength    = buffer_length;
  config.DPSM          = SDIO_DPSM_ENABLE;
  SDIO_ConfigData(SDIO, &config);
  
  /* Prepare SDIO command */ 
  command.CmdIndex         = SDMMC_CMD_SDMMC_RW_EXTENDED;
  command.Response         = SDIO_RESPONSE_SHORT;
  command.WaitForInterrupt = SDIO_WAIT_NO;
  command.CPSM             = SDIO_CPSM_ENABLE;
  SDIO_SendCommand(SDIO, &command);
  SDMMC_GetCmdResp(SDIO);
  uint32_t response_flags = SDIO_GetResponse(SDIO, SDIO_RESP1);
  if(((response_flags>> 8) & 0xFF) != 0x20)
  {
    return SL_ERROR;
  }
  
  while(!__SDIO_GET_FLAG(SDIO, SDIO_FLAG_RXOVERR | SDIO_FLAG_DCRCFAIL | SDIO_FLAG_DTIMEOUT | SDIO_FLAG_DATAEND))
  {
    if(__SDIO_GET_FLAG(SDIO, SDIO_FLAG_RXFIFOHF))
    {
      /* Read data from SDIO Rx FIFO */
      for(uint32_t count = 0U; count < 8U; count++)
      {
        *(tempbuff + count) = SDIO_ReadFIFO(SDIO);
      }
      tempbuff += 8U;
    }
  }

  while ((__SDIO_GET_FLAG(SDIO, SDIO_FLAG_RXDAVL)))
  {
    *tempbuff = SDIO_ReadFIFO(SDIO);
    tempbuff++;
  }
  
  /* Clear all the static flags */
  __SDIO_CLEAR_FLAG(SDIO, SDIO_STATIC_FLAGS);
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_sdio_write_cmd53( uint8_t function, uint32_t address, uint8_t* buffer, uint16_t buffer_length )
{
  SDIO_CmdInitTypeDef command;
  SDIO_DataInitTypeDef config;
  uint32_t *tempbuff = (uint32_t *)buffer;
  SDIO->DCTRL = 0U;  

  if( buffer_length >= SL_WFX_SDIO_BLOCK_MODE_THRESHOLD )
  {
    uint32_t block_count = ( buffer_length / SL_WFX_SDIO_BLOCK_SIZE ) + ( ( ( buffer_length % SL_WFX_SDIO_BLOCK_SIZE ) == 0 ) ? 0 : 1 );
    command.Argument = SDIO_CMD53_BLOCK_MODE | SDIO_CMD53_COUNT( block_count );
    config.TransferMode  = SDIO_TRANSFER_MODE_BLOCK;
    config.DataBlockSize = sdio_optimal_block_size(SL_WFX_SDIO_BLOCK_SIZE);
  }
  else
  {
    command.Argument = SDIO_CMD53_COUNT( buffer_length );
    config.TransferMode  = SDIO_TRANSFER_MODE_STREAM;
    config.DataBlockSize = sdio_optimal_block_size(buffer_length);
  }
  command.Argument |= SDIO_CMD53_WRITE | SDIO_CMD53_FUNCTION( function ) | SDIO_CMD53_OPMODE_INCREASING_ADDRESS | SDIO_CMD53_ADDRESS( address );
  config.TransferDir   = SDIO_TRANSFER_DIR_TO_CARD;
  
  SDIO->DCTRL |= SDIO_DCTRL_SDIOEN;
  
  /* Prepare SDIO command */ 
  command.CmdIndex         = SDMMC_CMD_SDMMC_RW_EXTENDED;
  command.Response         = SDIO_RESPONSE_SHORT;
  command.WaitForInterrupt = SDIO_WAIT_NO;
  command.CPSM             = SDIO_CPSM_ENABLE;
  SDIO_SendCommand(SDIO, &command);
  SDMMC_GetCmdResp(SDIO);
  uint32_t response_flags = SDIO_GetResponse(SDIO, SDIO_RESP1);
  if(((response_flags>> 8) & 0xFF) != 0x20)
  {
    return SL_ERROR;
  }
  
  /* Prepare Data */
  config.DataTimeOut   = SDMMC_DATATIMEOUT;
  config.DataLength    = buffer_length;
  config.DPSM          = SDIO_DPSM_ENABLE;
  SDIO_ConfigData(SDIO, &config);
  
  while(!__SDIO_GET_FLAG(SDIO, SDIO_FLAG_DATAEND))
  {
    if(__SDIO_GET_FLAG(SDIO, SDIO_FLAG_TXFIFOHE))
    {
      /* Write data to SDIO Tx FIFO */
      for(uint32_t count = 0U; count < 8U; count++)
      {
        SDIO_WriteFIFO(SDIO, (tempbuff + count));
      }
      tempbuff += 8U;
    }
  }
  
  /* Clear all the static flags */
  __SDIO_CLEAR_FLAG(SDIO, SDIO_STATIC_FLAGS);
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_sdio_transfer_cmd53(sl_wfx_host_bus_tranfer_type_t type, uint8_t function, uint32_t address, uint8_t* buffer, uint16_t buffer_length)
{    
  sl_status_t    result  = SL_ERROR;
  
  if(type == SL_WFX_BUS_WRITE)
  {
    result = sl_wfx_host_sdio_write_cmd53(function, address, buffer, buffer_length);
  }else{
    result = sl_wfx_host_sdio_read_cmd53(function, address, buffer, buffer_length);
  }
  
  return result;
}

sl_status_t sl_wfx_host_enable_platform_interrupt( void )
{
  __SDIO_ENABLE_IT(SDIO, SDIO_IT_SDIOIT);
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_disable_platform_interrupt( void )
{
  __SDIO_DISABLE_IT(SDIO, SDIO_IT_SDIOIT);
  return SL_SUCCESS;
}

static uint32_t sdio_optimal_block_size( uint16_t buffer_size )
{
    if ( buffer_size > (uint16_t) 2048 )
        return SDIO_DATABLOCK_SIZE_4096B;
    if ( buffer_size > (uint16_t) 1024 )
        return SDIO_DATABLOCK_SIZE_2048B;
    if ( buffer_size > (uint16_t) 512 )
        return SDIO_DATABLOCK_SIZE_1024B;
    if ( buffer_size > (uint16_t) 256 )
        return SDIO_DATABLOCK_SIZE_512B;
    if ( buffer_size > (uint16_t) 128 )
        return SDIO_DATABLOCK_SIZE_256B;
    if ( buffer_size > (uint16_t) 64 )
        return SDIO_DATABLOCK_SIZE_128B;
    if ( buffer_size > (uint16_t) 32 )
        return SDIO_DATABLOCK_SIZE_64B;
    if ( buffer_size > (uint16_t) 16 )
        return SDIO_DATABLOCK_SIZE_32B;
    if ( buffer_size > (uint16_t) 8 )
        return SDIO_DATABLOCK_SIZE_16B;
    if ( buffer_size > (uint16_t) 4 )
        return SDIO_DATABLOCK_SIZE_8B;
    if ( buffer_size > (uint16_t) 2 )
        return SDIO_DATABLOCK_SIZE_4B;
    return SDIO_DATABLOCK_SIZE_4B;
}

/* SDIO init function */
static void MX_SDIO_Init(void)
{
  SDIO_InitTypeDef Init;
  
  HAL_SDIO_MspInit();
  Init.ClockEdge           = SDIO_CLOCK_EDGE_RISING;
  Init.ClockBypass         = SDIO_CLOCK_BYPASS_DISABLE;
  Init.ClockPowerSave      = SDIO_CLOCK_POWER_SAVE_DISABLE;
  Init.BusWide             = SDIO_BUS_WIDE_1B;
  Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
  Init.ClockDiv            = SDIO_INIT_CLK_DIV;
  SDIO_Init(SDIO, Init);
  __SDIO_DISABLE(SDIO);
  SDIO_PowerState_ON(SDIO);
  __SDIO_ENABLE(SDIO);
}

/* SDIO deinit function */
static void MX_SDIO_DeInit(void)
{
  /* Peripheral clock disable */
  __HAL_RCC_SDIO_CLK_DISABLE();

  HAL_GPIO_DeInit(GPIOC, GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11 
                  |GPIO_PIN_12);
  
  HAL_GPIO_DeInit(GPIOD, GPIO_PIN_2);
  
  /* SDIO interrupt DeInit */
  HAL_NVIC_DisableIRQ(SDIO_IRQn);
}
