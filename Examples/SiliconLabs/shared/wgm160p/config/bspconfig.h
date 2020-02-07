/***************************************************************************//**
 * @file
 * @brief Provide BSP (board support package) configuration parameters.
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#ifndef SILICON_LABS_BSPCONFIG_H
#define SILICON_LABS_BSPCONFIG_H

#define BSP_STK
//#define BSP_STK_BRD2204A

#define BSP_BCC_USART         USART0
#define BSP_BCC_CLK           cmuClock_USART0
#define BSP_BCC_TX_LOCATION   USART_ROUTELOC0_TXLOC_LOC1
#define BSP_BCC_RX_LOCATION   USART_ROUTELOC0_RXLOC_LOC1
#define BSP_BCC_TXPORT        gpioPortE
#define BSP_BCC_TXPIN         7
#define BSP_BCC_RXPORT        gpioPortE
#define BSP_BCC_RXPIN         6
//#define BSP_BCC_ENABLE_PORT   gpioPortE
//#define BSP_BCC_ENABLE_PIN    1                 /* VCOM_ENABLE */

#define BSP_DISP_ENABLE_PORT  gpioPortE
#define BSP_DISP_ENABLE_PIN   5                /* MemLCD display enable */

#define BSP_GPIO_LEDS
#define BSP_NO_OF_LEDS        2
#define BSP_GPIO_LED0_PORT      gpioPortA
#define BSP_GPIO_LED0_PIN       4
#define BSP_GPIO_LED1_PORT      gpioPortA
#define BSP_GPIO_LED1_PIN       5
#define BSP_GPIO_LEDARRAY_INIT { { gpioPortA, 4 }, { gpioPortA, 5 }}

#define BSP_GPIO_BUTTONS
#define BSP_NO_OF_BUTTONS       2
#define BSP_GPIO_PB0_PORT       gpioPortD
#define BSP_GPIO_PB0_PIN        6
#define BSP_GPIO_PB1_PORT       gpioPortD
#define BSP_GPIO_PB1_PIN        8

#define BSP_GPIO_BUTTONARRAY_INIT { { BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN },{ BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN } }

#define BSP_INIT_DEFAULT        0

#define BSP_LFXO_CTUNE          70U
#define BSP_HFXO_CTUNE          132U

#define EMU_DCDCINIT_DEFAULT_WGM160P                                                         \
  {                                                                                  \
    emuPowerConfig_DcdcToDvdd,   /* DCDC to DVDD. */                                 \
    emuDcdcMode_LowNoise,        /* Low-noise mode in EM0. */                        \
    1800,                        /* Nominal output voltage for DVDD mode, 1.8V.  */  \
    5,                           /* Nominal EM0/1 load current of less than 5mA. */  \
    10,                          /* Nominal EM2/3/4 load current less than 10uA.  */ \
    200,                         /* Maximum average current of 200mA
                                    (assume strong battery or other power source). */ \
    emuDcdcAnaPeripheralPower_AVDD,/* Select AVDD as analog power supply). */         \
    emuDcdcLnHighEfficiency,     /* Use high-efficiency mode. */                      \
    emuDcdcLnCompCtrl_4u7F,      /* 4.7uF DCDC capacitor. */                          \
  }


#if !defined(EMU_DCDCINIT_STK_DEFAULT)
/* Use emlib defaults */
#define EMU_DCDCINIT_STK_DEFAULT          EMU_DCDCINIT_DEFAULT_WGM160P
#endif



#if !defined(CMU_HFXOINIT_STK_DEFAULT)
#define CMU_HFXOINIT_STK_DEFAULT                   \
  {                                                \
    _CMU_HFXOSTARTUPCTRL_CTUNE_DEFAULT,            \
    BSP_HFXO_CTUNE,                                \
    _CMU_HFXOSTARTUPCTRL_IBTRIMXOCORE_DEFAULT,     \
    _CMU_HFXOSTEADYSTATECTRL_IBTRIMXOCORE_DEFAULT, \
    _CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_DEFAULT,   \
    _CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_DEFAULT,    \
    _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_DEFAULT,   \
    cmuOscMode_Crystal,                            \
  }
#endif

#define BSP_BCP_VERSION 2
#include "bsp_bcp.h"

#endif /* SILICON_LABS_BSPCONFIG_H */
