/***************************************************************************//**
 * @file
 * @brief WiFi related functions
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

#include "lwip_bm.h"
#include "wifi_app.h"

void wifi_app_gpio_setup(void)
{
  // Configure WF200 reset pin.
  GPIO_PinModeSet(SL_WFX_HOST_CFG_RESET_PORT, SL_WFX_HOST_CFG_RESET_PIN, gpioModePushPull, 0);
  // Configure WF200 WUP pin.
  GPIO_PinModeSet(SL_WFX_HOST_CFG_WUP_PORT, SL_WFX_HOST_CFG_WUP_PIN, gpioModePushPull, 0);
  // GPIO used as IRQ.
  GPIO_PinModeSet(SL_WFX_HOST_CFG_SPI_WIRQPORT, SL_WFX_HOST_CFG_SPI_WIRQPIN, gpioModeInputPull, 0);
}

int wifi_app_init(void)
{
  wifi_app_gpio_setup();
  return lwip_bm_init();
}

void wifi_app_process(void)
{
  sl_wfx_process();
  lwip_bm_process();
}
