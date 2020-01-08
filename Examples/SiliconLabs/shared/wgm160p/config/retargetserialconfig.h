/***************************************************************************//**
 * @file
 * @brief Provide stdio retargeting configuration parameters.
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

#ifndef __SILICON_LABS_RETARGETSERIALCONFIG_H__
#define __SILICON_LABS_RETARGETSERIALCONFIG_H__

#include "bsp.h"

/***************************************************************************//**
 *
 * When retargeting serial output the user can choose which peripheral
 * to use as the serial output device. This choice is made by configuring
 * one or more of the following defines: RETARGET_USART4, RETARGET_LEUART0,
 * RETARGET_VCOM.
 *
 * This table shows the supported configurations and the resulting serial
 * output device.
 *
 * +----------------------------------------------------------------------+
 * | Defines                            | Serial Output (Locations)       |
 * |----------------------------------------------------------------------+
 * | None                               | USART4  (Rx #4, Tx #4)          |
 * | RETARGET_USART4                    | USART4  (Rx #4, Tx #4)          |
 * | RETARGET_LEUART0                   | LEUART0 (Rx #2, Tx #2)          |
 * | RETARGET_VCOM                      | VCOM using USART4               |
 * | RETARGET_USART4 and RETARGET_VCOM  | VCOM using USART4               |
 * | RETARGET_LEUART0 and RETARGET_VCOM | Not supported by EFM32GG11.     |
 * +----------------------------------------------------------------------+
 *
 * Note that the default configuration is the same as RETARGET_USART4.
 *
 ******************************************************************************/
#define RETARGET_IRQ_NAME    USART0_RX_IRQHandler         /* UART IRQ Handler */
  #define RETARGET_CLK         cmuClock_USART0              /* HFPER Clock */
  #define RETARGET_IRQn        USART0_RX_IRQn               /* IRQ number */
  #define RETARGET_UART        USART0                       /* UART instance */
  #define RETARGET_TX          USART_Tx                     /* Set TX to USART_Tx */
  #define RETARGET_RX          USART_Rx                     /* Set RX to USART_Rx */
  #define RETARGET_TX_LOCATION _USART_ROUTELOC0_TXLOC_LOC1  /* Location of of USART TX pin */
  #define RETARGET_RX_LOCATION _USART_ROUTELOC0_RXLOC_LOC1  /* Location of of USART RX pin */
  #define RETARGET_TXPORT      gpioPortE                    /* UART transmission port */
  #define RETARGET_TXPIN       7                            /* UART transmission pin */
  #define RETARGET_RXPORT      gpioPortE                    /* UART reception port */
  #define RETARGET_RXPIN       6                            /* UART reception pin */
  #define RETARGET_USART       1                            /* Includes em_usart.h */



#if defined(RETARGET_VCOM)
  #define RETARGET_PERIPHERAL_ENABLE() \
  ;
#endif


#endif
