/***************************************************************************//**
 * @file
 * @brief hal-config.h
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

#ifndef HAL_CONFIG_H
#define HAL_CONFIG_H

#include "board_features.h"
#include "hal-config-board.h"
#include "hal-config-app-common.h"

#ifndef HAL_VCOM_ENABLE
#define HAL_VCOM_ENABLE                   (1)
#endif
#ifndef HAL_I2CSENSOR_ENABLE
#define HAL_I2CSENSOR_ENABLE              (0)
#endif
#ifndef HAL_SPIDISPLAY_ENABLE
#define HAL_SPIDISPLAY_ENABLE             (1)
#endif
#define HAL_SPIDISPLAY_EXTCOMIN_CALLBACK
#if defined(FEATURE_IOEXPANDER)
#define HAL_SPIDISPLAY_EXTMODE_EXTCOMIN               (0)
#else
#define HAL_SPIDISPLAY_EXTMODE_EXTCOMIN               (1)
#endif
#define HAL_SPIDISPLAY_EXTMODE_SPI                    (0)
#define HAL_SPIDISPLAY_EXTCOMIN_USE_PRS               (0)
#define HAL_SPIDISPLAY_EXTCOMIN_USE_CALLBACK          (0)
#define HAL_SPIDISPLAY_FREQUENCY                      (1000000)

#endif
