/***************************************************************************//**
 * @file  interface.h
 * @brief Provides public functions for interactions with light/LED states & 
 *        update to the LCD screen.
 *******************************************************************************
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
#ifndef INTERFACE_H
#define INTERFACE_H

#include <stdbool.h>
#include <stddef.h>
#include "sl_bt_types.h" 
#include "sl_component_catalog.h"

#define BRD4187X (defined(EFR32MG24B020F1536IM48)     \
                  || defined(EFR32MG24A020F1536GM48)  \ 
                  || defined(EFR32MG24B220F1536IM48))

#define BRD4180_81_X (defined(EFR32MG21A020F1024IM32) \
                      || defined(EFR32MG21A010F1024IM32))
                  
typedef bd_addr interface_mac_t;

typedef enum {
  interface_light_trigger_src_button     = 0,
  interface_light_trigger_src_bluetooth  = 1,
  interface_light_trigger_src_wifi       = 2,
} interface_light_trigger_src_t;

#ifdef __cplusplus
extern "C" {
#endif

void interface_light_off(interface_light_trigger_src_t trigger,
                         interface_mac_t *mac);

void interface_light_on(interface_light_trigger_src_t trigger,
                        interface_mac_t *mac);
void interface_light_toggle(interface_light_trigger_src_t trigger,
                             interface_mac_t *mac);
void interface_light_set_state(interface_light_trigger_src_t trigger,
                               interface_mac_t *mac,
                               uint8_t new_led_state);
uint8_t interface_light_get_state(void);
void interface_light_get_mac_trigger (interface_mac_t *mac);
interface_light_trigger_src_t interface_light_get_trigger (void);

void interface_display_ble_state(bool connected);
void interface_display_wifi_state(bool connected);
void interface_display_ble_id(uint8_t *id);
void interface_display_wifi_id(uint8_t *id);
void interface_init(void);

#ifdef __cplusplus
}
#endif

#endif /* INTERFACE_H */
