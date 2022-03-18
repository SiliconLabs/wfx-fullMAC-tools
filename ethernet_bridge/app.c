/***************************************************************************//**
 * @file
 * @brief Application entry point.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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
#include "em_common.h"
#include "em_cmu.h"

#include "sl_device_init_clocks.h"
#include "sl_iostream_init_instances.h"

#include "app.h"
#include "app_ethernet_bridge.h"

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
  
  /* Select HFXO as the clock source that is important to ethernet controller */
  CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
#ifdef SL_CATALOG_IOSTREAM_USART_PRESENT
  sl_iostream_init_instances();
#endif

  /* Ethernet-Wi-Fi bridge init */
  app_ethernet_bridge_init();
}
