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

#ifdef EFR32MG12P432F1024GL125
#define SL_WFX_HOST_CFG_RESET_PORT          gpioPortD
#define SL_WFX_HOST_CFG_RESET_PIN           10

#define SL_WFX_HOST_CFG_SPI_USART_PORT      gpioPortA
#define SL_WFX_HOST_CFG_SPI_USART_CS_PIN    9
#define SL_WFX_HOST_CFG_SPI_USART_TX_PIN    6
#define SL_WFX_HOST_CFG_SPI_USART_RX_PIN    7
#define SL_WFX_HOST_CFG_SPI_USART_CLK_PIN   8

#define SL_WFX_HOST_CFG_SPI_USART           USART2
#define SL_WFX_HOST_CFG_SPI_USART_IDX       2
#define SL_WFX_HOST_CFG_SPI_USART_CLK       cmuClock_USART2

#define SL_WFX_HOST_CFG_WUP_PORT            gpioPortD
#define SL_WFX_HOST_CFG_WUP_PIN             8

#ifdef  SL_WFX_USE_SPI
#define SL_WFX_HOST_CFG_SPI_WIRQPORT        gpioPortB                    /* SPI IRQ port*/
#define SL_WFX_HOST_CFG_SPI_WIRQPIN         6                            /* SPI IRQ pin */
#define SL_WFX_HOST_CFG_SPI_IRQ             5
#else
#error "SPI only interface available and SL_WFX_USE_SPI not defined"
#endif
#endif
