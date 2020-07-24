/***************************************************************************//**
 * @brief bluetooth_app.h
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

#ifndef BLUETOOTH_APP_H_
#define BLUETOOTH_APP_H_

#include "gecko_configuration.h"

// Advertisement data
#define UINT16_TO_BYTES(n)        ((uint8_t) (n)), ((uint8_t)((n) >> 8))
#define UINT16_TO_BYTE0(n)        ((uint8_t) (n))
#define UINT16_TO_BYTE1(n)        ((uint8_t) ((n) >> 8))

#define BLE_STATE_ADVERTISING     (1<<0)
#define BLE_STATE_CONNECTED       (1<<1)

#ifdef __cplusplus
extern "C" {
#endif

int  bluetooth_app_init(gecko_configuration_t *pconfig);
void bluetooth_app_process(void);
void bluetooth_app_request_send_indication(void);
void bluetooth_app_start_advertising(void);
void bluetooth_app_stop_advertising(void);
void bluetooth_app_disconnect_master(void);
uint8_t bluetooth_app_get_ble_state(void);
void bluetooth_app_get_own_mac(bd_addr *mac);
int  bluetooth_app_get_master_mac(bd_addr *mac);
void bluetooth_app_get_own_name(char *name, int name_size);
int  bluetooth_app_get_master_name(char *name, int name_size);

#ifdef __cplusplus
}
#endif

#endif
