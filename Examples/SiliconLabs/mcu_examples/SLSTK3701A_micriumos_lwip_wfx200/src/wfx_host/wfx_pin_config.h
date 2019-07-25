/**************************************************************************//**
 * Copyright 2019, Silicon Laboratories Inc.
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
#define BSP_EXP_RESET_PORT      gpioPortC
#define BSP_EXP_RESET_PIN       4

#define BSP_EXP_WUP_PORT        gpioPortA
#define BSP_EXP_WUP_PIN         12
#ifdef  SL_WFX_USE_SPI
#define BSP_EXP_SPI_WIRQPORT    gpioPortE                    /* SPI IRQ port*/
#define BSP_EXP_SPI_WIRQPIN     8                            /* SPI IRQ pin */
#define BSP_EXP_SPI_IRQ         10
#endif
#ifdef SLEEP_ENABLED
#ifdef SL_WFX_USE_SDIO
#error "This is an invalid combination for GG11 STK"
#endif
#endif
#endif



#ifdef EFM32GG11B820F2048GM64
#define BSP_EXP_RESET_PORT      gpioPortF
#define BSP_EXP_RESET_PIN       12

#define BSP_EXP_WUP_PORT        gpioPortE
#define BSP_EXP_WUP_PIN         4
#define BSP_EXP_WIRQPORT        gpioPortA                    /* WIRQ port*/
#define BSP_EXP_WIRQPIN         8                            /* WIRQ pin */
#define BSP_EXP_IRQ             10
#define LP_CLK_PORT             gpioPortA
#define LP_CLK_PIN              12
#endif

#define FRAME_RX_EVENT          DEF_BIT_00

#define MAX_RX_QUEUE_SIZE       4


