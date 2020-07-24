/***************************************************************************//**
 * @brief interface.h
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

#ifndef INTERFACE_H_
#define INTERFACE_H_

#include <stdbool.h>
#include "bg_types.h"

typedef bd_addr interface_mac_t;

typedef enum {
  interface_light_trigger_src_button     = 0,
  interface_light_trigger_src_bluetooth  = 1,
  interface_light_trigger_src_wifi       = 2,
} interface_light_trigger_src_t;

#ifdef __cplusplus
extern "C" {
#endif

void interface_init(void);

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
interface_light_trigger_src_t interface_light_get_trigger(void);
void interface_light_get_mac_trigger(interface_mac_t *mac);

void interface_display_ble_state(bool connected);
void interface_display_wifi_state(bool connected);
void interface_display_ble_id(uint8_t *id);
void interface_display_wifi_id(uint8_t *id);

#ifdef __cplusplus
}
#endif

#endif /* INTERFACE_H_ */
