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

#ifdef EFR32MG21A020F1024IM32
#define SL_WFX_HOST_CFG_RESET_PORT          gpioPortD
#define SL_WFX_HOST_CFG_RESET_PIN           2

#define SL_WFX_HOST_CFG_SPI_USART_PORT      gpioPortC
#define SL_WFX_HOST_CFG_SPI_USART_CS_PIN    3
#define SL_WFX_HOST_CFG_SPI_USART_TX_PIN    0
#define SL_WFX_HOST_CFG_SPI_USART_RX_PIN    1
#define SL_WFX_HOST_CFG_SPI_USART_CLK_PIN   2

#define SL_WFX_HOST_CFG_SPI_USART           USART2
#define SL_WFX_HOST_CFG_SPI_USART_IDX       2
#define SL_WFX_HOST_CFG_SPI_USART_CLK       cmuClock_USART2

#define SL_WFX_HOST_CFG_WUP_PORT            gpioPortD
#define SL_WFX_HOST_CFG_WUP_PIN             4

#ifdef  SL_WFX_USE_SPI
#define SL_WFX_HOST_CFG_SPI_WIRQPORT        gpioPortC                    /* SPI IRQ port*/
#define SL_WFX_HOST_CFG_SPI_WIRQPIN         5                            /* SPI IRQ pin */
#define SL_WFX_HOST_CFG_SPI_IRQ             5
#else
#error "SPI only interface available and SL_WFX_USE_SPI not defined"
#endif
#endif
