/***************************************************************************//**
 * @file
 * @brief Functions and data related to PTI
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

#include "board_features.h"

#if defined(FEATURE_PTI_SUPPORT)

#include "pti.h"
#include "rail.h"
#if defined(HAL_CONFIG)
#include "bsphalconfig.h"
#else
#include "bspconfig.h"
#endif

uint8_t configEnablePti(void)
{
  RAIL_PtiConfig_t ptiConfig = RAIL_PTI_CONFIG;
  RAIL_Status_t status;

  status = RAIL_ConfigPti(RAIL_EFR32_HANDLE, &ptiConfig);

  if (RAIL_STATUS_NO_ERROR == status) {
    status = RAIL_EnablePti(RAIL_EFR32_HANDLE, true);
  }

  return (uint8_t)status;
}

#endif
