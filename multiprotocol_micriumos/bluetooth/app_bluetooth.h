/***************************************************************************//**
 * @file app_bluetooth.h
 * @brief Bluetooth app user-defined types & public functions. 
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
#ifndef APP_BLUETOOTH_H
#define APP_BLUETOOTH_H

#include "interface.h"
#include "sl_bt_api.h"
#include "sl_simple_timer.h"
/**************************************************************************//**
 * DEFINES.
 *****************************************************************************/
#define LIGHT_STATE_GATTDB                        gattdb_light_state
#define TRIGGER_SOURCE_GATTDB                     gattdb_trigger_source
#define SOURCE_ADDRESS_GATTDB                     gattdb_source_address
#define DEV_NAME_GATDB                            gattdb_device_name

//FIXME issue with inferior values
#define INDICATION_TIMER_PERIOD_MSEC              1200    
#define INDICATION_TIMER_TIMEOUT_MSEC             1000

#define BLE_GATT_SERVICE_GENERIC_ACCESS_UUID      0x1800
#define BLE_GATT_CHARACTERISTIC_DEVICE_NAME_UUID  0x2A00
#define BLE_MASTER_NAME_MAX_LEN                   15
#define BLE_MASTER_CONNECTION                     1

#define BLE_STATE_ADVERTISING                     (1<<0) 
#define BLE_STATE_CONNECTED                       (1<<1)
#define MAX_BUF_LEN                               128 

// Device name length (NULL-terminated string)
#define DEVNAME_LEN                                8        
extern char boot_message[MAX_BUF_LEN];

/**************************************************************************//**
 * User-defined types
 *****************************************************************************/
typedef enum {
  get_gatt_service_handle = 0,
  get_gatt_name_characteristic_handle,
  get_gatt_name_characteristic_value,
} ble_name_retrieval_step_t;

typedef enum {
  ble_periodic_indication_timer     = 0,
  ble_indication_timeout_timer      = 1,
} ble_timer_t;

typedef struct {
  uint8_t handle;
  bd_addr address;
  bool    inUse;
  uint8_t indicated;
  char    name[BLE_MASTER_NAME_MAX_LEN+1];
} ble_conn_t;

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @brief: Pulic bluetooth functions
 ******************************************************************************/
void bluetooth_app_request_send_indication(void);
uint8_t bluetooth_app_get_ble_state (void);
void bluetooth_app_get_own_mac(bd_addr *mac);
int  bluetooth_app_get_master_mac(bd_addr *mac);
void bluetooth_app_get_own_name(char *name, int name_size);
int  bluetooth_app_get_master_name(char *name, int name_size);
void bluetooth_app_disconnect_master(void);
void bluetooth_app_start_advertising(void);
void bluetooth_app_stop_advertising(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_BLUETOOTH_H */ 
