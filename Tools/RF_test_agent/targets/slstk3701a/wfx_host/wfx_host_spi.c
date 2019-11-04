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
 * SPI interface implementation: EFM32GG11 + Bare Metal
 *****************************************************************************/

#include "em_gpio.h"
#include "em_usart.h"
#include "spidrv.h"
#include "sl_wfx.h"

#include "wfx_host_cfg.h"

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */
static SPIDRV_HandleData_t spiHandleData;
static SPIDRV_Handle_t spiHandle = &spiHandleData;
/** @endcond */

sl_status_t sl_wfx_host_init_bus (void)
{
  Ecode_t       result;
  sl_status_t	  status = SL_ERROR;
  SPIDRV_Init_t initData = SPIDRV_MASTER_USART0;

  // Adapt the default configuration
  initData.bitRate = 42000000;
  initData.portLocationCs = _USART_ROUTELOC0_CSLOC_LOC0;
  initData.portLocationClk = _USART_ROUTELOC0_CLKLOC_LOC0;
  initData.csControl = spidrvCsControlApplication;

  // Init SPI interface
  result = SPIDRV_Init(spiHandle, &initData);
  if (result == ECODE_OK) {
  	status = SL_SUCCESS;
  }

  // Enable a higher speed on the link
  USART0->CTRL |= (1<<_USART_CTRL_SMSDELAY_SHIFT);

  return status;
}

sl_status_t sl_wfx_host_deinit_bus (void)
{
  Ecode_t		result;
  sl_status_t	status = SL_ERROR;

  // Init SPI interface
  result = SPIDRV_DeInit(spiHandle);
  if (result == ECODE_OK) {
  	status = SL_SUCCESS;
  }

  return status;
}

sl_status_t sl_wfx_host_spi_cs_assert (void)
{
  GPIO_PinModeSet(WFX_HOST_CFG_SPI_USART_PORT, WFX_HOST_CFG_SPI_USART_CS_PIN,
		          gpioModePushPull, 0);

  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_spi_cs_deassert(void)
{
  GPIO_PinModeSet(WFX_HOST_CFG_SPI_USART_PORT, WFX_HOST_CFG_SPI_USART_CS_PIN,
  		          gpioModePushPull, 1);

  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_spi_transfer_no_cs_assert (sl_wfx_host_bus_tranfer_type_t type,
                                                   uint8_t *header,
                                                   uint16_t header_length,
                                                   uint8_t *buffer,
                                                   uint16_t buffer_length)
{
  sl_status_t    result  = SL_ERROR;
  const bool     is_read = ( type == SL_WFX_BUS_READ );

  SPIDRV_MTransmitB(spiHandle, header, header_length);
  if (is_read) {
    if (SPIDRV_MReceiveB(spiHandle, buffer, buffer_length) == ECODE_OK) {
      result = SL_SUCCESS;
    }
  } else {
    if (SPIDRV_MTransmitB(spiHandle, buffer, buffer_length) == ECODE_OK) {
      result = SL_SUCCESS;
    }
  }

  return result;
}

sl_status_t sl_wfx_host_enable_platform_interrupt( void )
{
  GPIO_IntEnable(1<<WFX_HOST_CFG_SPI_IRQ);
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_disable_platform_interrupt( void )
{
  GPIO_IntDisable(1<<WFX_HOST_CFG_SPI_IRQ);
  return SL_SUCCESS;
}
