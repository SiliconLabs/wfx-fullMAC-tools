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

#include "em_gpio.h"
#include "em_usart.h"
#include "em_cmu.h"
#include "em_ldma.h"
#include "em_bus.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define USART           SL_WFX_HOST_CFG_SPI_USART
#define USART_CLK       SL_WFX_HOST_CFG_SPI_USART_CLK
#define USART_PORT      SL_WFX_HOST_CFG_SPI_USART_PORT
#define USART_CS_PIN    SL_WFX_HOST_CFG_SPI_USART_CS_PIN
#define USART_TX_PIN    SL_WFX_HOST_CFG_SPI_USART_TX_PIN
#define USART_RX_PIN    SL_WFX_HOST_CFG_SPI_USART_RX_PIN
#define USART_CLK_PIN   SL_WFX_HOST_CFG_SPI_USART_CLK_PIN

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
  CMU_ClockEnable(cmuClock_HFPER, true);
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(USART_CLK, true);
  USART_InitSync(USART, &usartInit);
  USART->CTRL |= (1u << _USART_CTRL_SMSDELAY_SHIFT);
  USART->ROUTELOC0 = (USART->ROUTELOC0
                      & ~(_USART_ROUTELOC0_TXLOC_MASK
                          | _USART_ROUTELOC0_RXLOC_MASK
                          | _USART_ROUTELOC0_CLKLOC_MASK))
                     | (SL_WFX_HOST_CFG_SPI_TX_LOC_NBR  << _USART_ROUTELOC0_TXLOC_SHIFT)
                     | (SL_WFX_HOST_CFG_SPI_RX_LOC_NBR  << _USART_ROUTELOC0_RXLOC_SHIFT)
                     | (SL_WFX_HOST_CFG_SPI_CLK_LOC_NBR << _USART_ROUTELOC0_CLKLOC_SHIFT);

  USART->ROUTEPEN = USART_ROUTEPEN_TXPEN
                    | USART_ROUTEPEN_RXPEN
                    | USART_ROUTEPEN_CLKPEN;
  GPIO_DriveStrengthSet(USART_PORT, gpioDriveStrengthStrongAlternateStrong);
  GPIO_PinModeSet(USART_PORT, USART_TX_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(USART_PORT, USART_RX_PIN, gpioModeInput, 0);
  GPIO_PinModeSet(USART_PORT, USART_CLK_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(USART_PORT, USART_CS_PIN, gpioModePushPull, 1);
  USART->CMD = USART_CMD_CLEARRX | USART_CMD_CLEARTX;
  return SL_STATUS_OK;
}

/**************************************************************************//**
 * De-initialize SPI peripheral and DMAs
 *****************************************************************************/
sl_status_t sl_wfx_host_deinit_bus(void)
{
  USART_Reset(USART);
  return SL_STATUS_OK;
}


/**************************************************************************//**
 * Assert chip select.
 *****************************************************************************/
sl_status_t sl_wfx_host_spi_cs_assert()
{
  GPIO_PinOutClear(USART_PORT, USART_CS_PIN);
  return SL_STATUS_OK;
}

/**************************************************************************//**
 * De-assert chip select.
 *****************************************************************************/
sl_status_t sl_wfx_host_spi_cs_deassert()
{
  GPIO_PinOutSet(USART_PORT, USART_CS_PIN);
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

  while (!(USART->STATUS & USART_STATUS_TXBL)) {
  }
  USART->CMD = USART_CMD_CLEARRX | USART_CMD_CLEARTX;

  if (header_length > 0) {
    for (uint8_t *buffer_ptr = header; header_length > 0; --header_length, ++buffer_ptr) {
      USART->TXDATA = (uint32_t)(*buffer_ptr);

      while (!(USART->STATUS & USART_STATUS_TXC)) {
      }
    }
    while (!(USART->STATUS & USART_STATUS_TXBL)) {
    }
  }
  if (buffer_length > 0) {
    USART->CMD = USART_CMD_CLEARRX | USART_CMD_CLEARTX;

    for (uint8_t *buffer_ptr = buffer; buffer_length > 0; --buffer_length, ++buffer_ptr) {
      USART->TXDATA = (uint32_t)(*buffer_ptr);

      while (!(USART->STATUS & USART_STATUS_TXC)) {
      }
      if (is_read) {
        *buffer_ptr = (uint8_t)USART->RXDATA;
      }
    }
    while (!(USART->STATUS & USART_STATUS_TXBL)) {
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
