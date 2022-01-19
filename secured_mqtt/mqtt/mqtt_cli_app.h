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

#ifndef MQTT_CLI_APP_H
#define MQTT_CLI_APP_H

/***************************************************************************//**
 * Initialize example
 ******************************************************************************/
#include <kernel/include/os.h>
#include <stdio.h>
#include <stdlib.h>

#define MQTT_DEBUG_ENABLE     0

#if (MQTT_DEBUG_ENABLE == 1)
#define MQTT_DBG(s, args...)   printf("%s-%d: " s, __func__, __LINE__, ##args)
#else
#define MQTT_DBG(...)
#endif
/* MQTT events state*/
#define SL_WFX_EVENT_MQTT_CONNECT           (1 << 1)
#define SL_WFX_EVENT_MQTT_PUBLISH           (1 << 2)
#define SL_WFX_EVENT_MQTT_SAVE              (1 << 3)

extern OS_FLAG_GRP mqtt_cli_events;

void mqtt_cli_app_init(void);

#endif  // CLI_MICRIUMOS_H
