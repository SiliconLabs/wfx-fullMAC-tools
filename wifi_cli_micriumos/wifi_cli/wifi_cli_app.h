/**************************************************************************//**
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
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
#ifndef WIFI_CLI_APP_H
#define WIFI_CLI_APP_H

#include <string.h>
#include <stdio.h>
#include "os.h"
#include "wifi_cli_cmd_registration.h"
#include "wifi_cli_lwip.h"
#include "wifi_cli_params.h"
#include "sl_wfx_host.h"
#include "app_wifi_events.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @brief: Initialize the Wi-Fi CLI app.
 ******************************************************************************/
 void cli_app_init(void);

#ifdef __cplusplus
}
#endif

#endif  /* WIFI_CLI_APP_H */
