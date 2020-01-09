/***************************************************************************//**
 * @file
 * @brief WFX host configuration and pinout
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

#ifdef EFM32GG11B820F2048GL192
#define WFX_HOST_CFG_RESET_PORT      gpioPortC
#define WFX_HOST_CFG_RESET_PIN       4

#define WFX_HOST_CFG_SPI_USART_PORT      gpioPortE
#define WFX_HOST_CFG_SPI_USART_CS_PIN    13
#define WFX_HOST_CFG_SPI_USART_TX_PIN    10
#define WFX_HOST_CFG_SPI_USART_RX_PIN    11
#define WFX_HOST_CFG_SPI_USART_CLK_PIN   12

#define WFX_HOST_CFG_SPI_RX_LOC_NBR      0
#define WFX_HOST_CFG_SPI_TX_LOC_NBR      0
#define WFX_HOST_CFG_SPI_CLK_LOC_NBR     0

#define WFX_HOST_CFG_SPI_USART           USART0
#define WFX_HOST_CFG_SPI_USART_CLK       cmuClock_USART0
#define WFX_HOST_CFG_SPI_RX_DMA_SIGNAL   dmadrvPeripheralSignal_USART0_RXDATAV
#define WFX_HOST_CFG_SPI_TX_DMA_SIGNAL   dmadrvPeripheralSignal_USART0_TXBL

#define WFX_HOST_CFG_WUP_PORT        gpioPortA
#define WFX_HOST_CFG_WUP_PIN         12
#ifdef  SL_WFX_USE_SPI
#define WFX_HOST_CFG_SPI_WIRQPORT    gpioPortE                    /* SPI IRQ port*/
#define WFX_HOST_CFG_SPI_WIRQPIN     8                            /* SPI IRQ pin */
#define WFX_HOST_CFG_SPI_IRQ         10
#endif
#ifdef SLEEP_ENABLED
#ifdef SL_WFX_USE_SDIO
#error "This is an invalid combination for GG11 STK"
#endif
#endif
#endif

#ifdef EFM32GG11B820F2048GM64
#define WFX_HOST_CFG_RESET_PORT      gpioPortF
#define WFX_HOST_CFG_RESET_PIN       12

#define WFX_HOST_CFG_WUP_PORT        gpioPortE
#define WFX_HOST_CFG_WUP_PIN         4
#define WFX_HOST_CFG_WIRQPORT        gpioPortA                    /* WIRQ port*/
#define WFX_HOST_CFG_WIRQPIN         8                            /* WIRQ pin */
#define WFX_HOST_CFG_IRQ             10
#define LP_CLK_PORT             gpioPortA
#define LP_CLK_PIN              12
#endif

#define FRAME_RX_EVENT          DEF_BIT_00

#define MAX_RX_QUEUE_SIZE       4
