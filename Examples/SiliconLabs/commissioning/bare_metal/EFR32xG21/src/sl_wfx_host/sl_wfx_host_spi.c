/***************************************************************************//**
 * @file
 * @brief WFX SPI interface driver implementation
 *******************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
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
#ifdef  SL_WFX_USE_SPI

#include "sl_wfx.h"
#include "sl_wfx_host_api.h"
#include "sl_wfx_host_cfg.h"

//#include "em_device.h"
//#include "em_chip.h"
#include "em_gpio.h"
#include "em_usart.h"
#include "em_cmu.h"
#include "em_ldma.h"
#include "em_bus.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define WFX_USART           SL_WFX_HOST_CFG_SPI_USART
#define WFX_USART_IDX       SL_WFX_HOST_CFG_SPI_USART_IDX
#define WFX_USART_CLK       SL_WFX_HOST_CFG_SPI_USART_CLK
#define WFX_USART_PORT      SL_WFX_HOST_CFG_SPI_USART_PORT
#define WFX_USART_CS_PIN    SL_WFX_HOST_CFG_SPI_USART_CS_PIN
#define WFX_USART_TX_PIN    SL_WFX_HOST_CFG_SPI_USART_TX_PIN
#define WFX_USART_RX_PIN    SL_WFX_HOST_CFG_SPI_USART_RX_PIN
#define WFX_USART_CLK_PIN   SL_WFX_HOST_CFG_SPI_USART_CLK_PIN

static bool spi_enabled = false;

/**************************************************************************//**
 * Initialize SPI peripheral
 *****************************************************************************/
sl_status_t sl_wfx_host_init_bus(void)
{
  // Initialize and enable the USART
  USART_InitSync_TypeDef usartInit = USART_INITSYNC_DEFAULT;

  spi_enabled = true;
  usartInit.baudrate = 36000000u;
  usartInit.msbf = true;
  CMU_ClockEnable(cmuClock_PCLK, true);
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(WFX_USART_CLK, true);

  GPIO_PinModeSet(WFX_USART_PORT, WFX_USART_TX_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(WFX_USART_PORT, WFX_USART_RX_PIN, gpioModeInput, 0);
  GPIO_PinModeSet(WFX_USART_PORT, WFX_USART_CLK_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(WFX_USART_PORT, WFX_USART_CS_PIN, gpioModePushPull, 1);

  GPIO->USARTROUTE[WFX_USART_IDX].CLKROUTE =  (WFX_USART_PORT << _GPIO_USART_CLKROUTE_PORT_SHIFT)
                                            | (WFX_USART_CLK_PIN << _GPIO_USART_CLKROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[WFX_USART_IDX].TXROUTE =  (WFX_USART_PORT << _GPIO_USART_TXROUTE_PORT_SHIFT)
                                           | (WFX_USART_TX_PIN << _GPIO_USART_TXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[WFX_USART_IDX].RXROUTE =  (WFX_USART_PORT << _GPIO_USART_RXROUTE_PORT_SHIFT)
                                           | (WFX_USART_RX_PIN << _GPIO_USART_RXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[WFX_USART_IDX].ROUTEEN =  GPIO_USART_ROUTEEN_CLKPEN
                                           | GPIO_USART_ROUTEEN_RXPEN
                                           | GPIO_USART_ROUTEEN_TXPEN;


  USART_InitSync(WFX_USART, &usartInit);
  WFX_USART->CTRL |= (1u << _USART_CTRL_SMSDELAY_SHIFT);
  WFX_USART->CMD = USART_CMD_CLEARRX | USART_CMD_CLEARTX;

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * De-initialize SPI peripheral and DMAs
 *****************************************************************************/
sl_status_t sl_wfx_host_deinit_bus(void)
{
  USART_Reset(WFX_USART);
  return SL_STATUS_OK;
}


/**************************************************************************//**
 * Assert chip select.
 *****************************************************************************/
sl_status_t sl_wfx_host_spi_cs_assert()
{
  GPIO_PinOutClear(WFX_USART_PORT, WFX_USART_CS_PIN);
  return SL_STATUS_OK;
}

/**************************************************************************//**
 * De-assert chip select.
 *****************************************************************************/
sl_status_t sl_wfx_host_spi_cs_deassert()
{
  GPIO_PinOutSet(WFX_USART_PORT, WFX_USART_CS_PIN);
  return SL_STATUS_OK;
}

/**************************************************************************//**
 * WFX SPI transfer implementation
 *****************************************************************************/
sl_status_t sl_wfx_host_spi_transfer_no_cs_assert(sl_wfx_host_bus_transfer_type_t type,
                                                  uint8_t *header,
                                                  uint16_t header_length,
                                                  uint8_t *buffer,
                                                  uint16_t buffer_length)
{
  const bool is_read = (type == SL_WFX_BUS_READ);

  while (!(WFX_USART->STATUS & USART_STATUS_TXBL)) {
  }
  WFX_USART->CMD = USART_CMD_CLEARRX | USART_CMD_CLEARTX;

  if (header_length > 0) {
    for (uint8_t *buffer_ptr = header; header_length > 0; --header_length, ++buffer_ptr) {
      WFX_USART->TXDATA = (uint32_t)(*buffer_ptr);

      while (!(WFX_USART->STATUS & USART_STATUS_TXC)) {
      }
    }
    while (!(WFX_USART->STATUS & USART_STATUS_TXBL)) {
    }
  }
  if (buffer_length > 0) {
    WFX_USART->CMD = USART_CMD_CLEARRX | USART_CMD_CLEARTX;

    for (uint8_t *buffer_ptr = buffer; buffer_length > 0; --buffer_length, ++buffer_ptr) {
      WFX_USART->TXDATA = (uint32_t)(*buffer_ptr);

      while (!(WFX_USART->STATUS & USART_STATUS_TXC)) {
      }
      if (is_read) {
        *buffer_ptr = (uint8_t)WFX_USART->RXDATA;
      }
    }
    while (!(WFX_USART->STATUS & USART_STATUS_TXBL)) {
    }
  }

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * Enable WFX interrupt
 *****************************************************************************/
sl_status_t sl_wfx_host_enable_platform_interrupt(void)
{
  GPIO_ExtIntConfig(SL_WFX_HOST_CFG_SPI_WIRQPORT,
                    SL_WFX_HOST_CFG_SPI_WIRQPIN,
                    SL_WFX_HOST_CFG_SPI_IRQ,
                    true,
                    false,
                    true);
  return SL_STATUS_OK;
}

/**************************************************************************//**
 * Disable WFX interrupt
 *****************************************************************************/
sl_status_t sl_wfx_host_disable_platform_interrupt(void)
{
  GPIO_IntDisable(1 << SL_WFX_HOST_CFG_SPI_IRQ);
  return SL_STATUS_OK;
}

#endif
