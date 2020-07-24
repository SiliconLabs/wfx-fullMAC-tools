/***************************************************************************//**
 * @file app.c
 * @brief Silicon Labs Empty Example Project
 *
 * This example demonstrates the bare minimum needed for a Blue Gecko C application
 * that allows Over-the-Air Device Firmware Upgrading (OTA DFU). The application
 * starts advertising after boot and restarts advertising after a connection is closed.
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

#include <stdio.h>
#include <stdbool.h>

/* Bluetooth stack headers */
#include "bg_types.h"
#include "native_gecko.h"
#include "gatt_db.h"

#include "interface.h"
#include "bluetooth_advertisement.h"
#include "bluetooth_app.h"

#define LIGHT_STATE_GATTDB                        gattdb_light_state
#define TRIGGER_SOURCE_GATTDB                     gattdb_trigger_source
#define SOURCE_ADDRESS_GATTDB                     gattdb_source_address

// Timer Frequency used.
#define TIMER_CLK_FREQ                            ((uint32_t)32768)
// Convert msec to timer ticks.
#define TIMER_MS_2_TIMERTICK(ms)                  ((TIMER_CLK_FREQ * ms) / 1000)
#define TIMER_S_2_TIMERTICK(s)                    (TIMER_CLK_FREQ * s)

#define INDICATION_TIMER_PERIOD_MSEC              1200    //FIXME issue with inferior values
#define INDICATION_TIMER_TIMEOUT_MSEC             1000

#define BLE_GATT_SERVICE_GENERIC_ACCESS_UUID      0x1800
#define BLE_GATT_CHARACTERISTIC_DEVICE_NAME_UUID  0x2A00
#define BLE_MASTER_CONNECTION                     1
#define BLE_MASTER_NAME_MAX_LEN                   15

typedef enum {
  get_gatt_service_handle = 0,
  get_gatt_name_characteristic_handle,
  get_gatt_name_characteristic_value,
} ble_name_retrieval_step_t;

typedef enum {
  bleTimerIndicatePeriod    = 1,
  bleTimerIndicateTimeout   = 2,
} BleTimer_t;

typedef struct {
  uint8_t handle;
  bd_addr address;
  bool    inUse;
  uint8_t indicated;
  char    name[BLE_MASTER_NAME_MAX_LEN+1];
} BleConn_t;

static BleConn_t ble_conn[MAX_CONNECTIONS];

static bd_addr ble_own_addr = {0};

static gecko_configuration_t *ble_config = NULL;

static enum gatt_client_config_flag light_state_gatt_flag = gatt_disable;
static enum gatt_client_config_flag trigger_source_gatt_flag = gatt_disable;
static enum gatt_client_config_flag source_address_gatt_flag = gatt_disable;

static bool ble_indication_pending = false;
static bool ble_indication_ongoing = false;
static uint8_t ble_nb_connected = 0;
static uint8_t ble_state = 0;
static char ble_own_name[DEVNAME_LEN] = {0};
static uint32_t gatt_service_handle = 0;
static uint16_t gatt_name_characteristic_handle = 0;
static ble_name_retrieval_step_t name_retrieval_step = get_gatt_service_handle;

/* Print stack version and local Bluetooth address as boot message */
static void bootMessage(struct gecko_msg_system_boot_evt_t *bootevt, bd_addr *local_addr)
{
  int i;

  printf("stack version: %u.%u.%u\r\n", bootevt->major, bootevt->minor, bootevt->patch);

  printf("local BT device address: ");
  for (i = 0; i < 5; i++) {
    printf("%02X:", local_addr->addr[5 - i]);
  }
  printf("%02X\r\n", local_addr->addr[0]);
}

// return false on error
static bool bleConnAdd(uint8_t handle, bd_addr* address)
{
  for (uint8_t i = 0; i < ble_config->bluetooth.max_connections; i++) {
    if (ble_conn[i].inUse == false) {
      ble_conn[i].handle = handle;
      memcpy((void*)&ble_conn[i].address, (void*)address, sizeof(ble_conn[i].address));
      ble_conn[i].inUse = true;
      return true;
    }
  }
  return false;
}

static bool bleConnRemove(uint8_t handle)
{
  for (uint8_t i = 0; i < ble_config->bluetooth.max_connections; i++) {
    if (ble_conn[i].handle == handle) {
      ble_conn[i].handle = 0;
      memset((void*)&ble_conn[i].address.addr, 0, sizeof(ble_conn[i].address.addr));
      ble_conn[i].inUse = false;
      return true;
    }
  }
  return false;
}

static BleConn_t* bleConnGet(uint8_t handle)
{
  for (uint8_t i = 0; i < ble_config->bluetooth.max_connections; i++) {
    if (ble_conn[i].handle == handle) {
      return &ble_conn[i];
    }
  }
  return NULL;
}

static int bluetooth_app_send_indication(uint16_t ble_characteristic)
{
  int res = -1;

  switch (ble_characteristic) {

    case LIGHT_STATE_GATTDB:
    {
      // stop light indication confirmation timer
      gecko_cmd_hardware_set_soft_timer(0, bleTimerIndicateTimeout, false);

      if (light_state_gatt_flag == gatt_indication) {
        uint8_t light_state = interface_light_get_state();

        // start timer for light indication confirmation
        gecko_cmd_hardware_set_soft_timer(TIMER_MS_2_TIMERTICK(INDICATION_TIMER_TIMEOUT_MSEC),
                                          bleTimerIndicateTimeout,
                                          true);

        /* Send notification/indication data */
        gecko_cmd_gatt_server_send_characteristic_notification(ble_conn[0].handle,
                                                               LIGHT_STATE_GATTDB,
                                                               sizeof(light_state),
                                                               (uint8_t*)&light_state);
        res = 0;
      }
      break;
    }

    case TRIGGER_SOURCE_GATTDB:
    {
      // stop light indication confirmation timer
      gecko_cmd_hardware_set_soft_timer(0, bleTimerIndicateTimeout, false);

      if (trigger_source_gatt_flag == gatt_indication) {
        uint8_t trigger = (uint8_t)interface_light_get_trigger();

        // start timer for trigger source indication confirmation
        gecko_cmd_hardware_set_soft_timer(TIMER_MS_2_TIMERTICK(INDICATION_TIMER_TIMEOUT_MSEC),
                                          bleTimerIndicateTimeout,
                                          true);

        /* Send notification/indication data */
        gecko_cmd_gatt_server_send_characteristic_notification(ble_conn[0].handle,
                                                               TRIGGER_SOURCE_GATTDB,
                                                               sizeof(trigger),
                                                               &trigger);
        res = 0;
      }
      break;
    }

    case SOURCE_ADDRESS_GATTDB:
    {
      // stop light indication confirmation timer
      gecko_cmd_hardware_set_soft_timer(0, bleTimerIndicateTimeout, false);

      if (source_address_gatt_flag == gatt_indication) {
        // start timer for source address indication confirmation
        gecko_cmd_hardware_set_soft_timer(TIMER_MS_2_TIMERTICK(INDICATION_TIMER_TIMEOUT_MSEC),
                                          bleTimerIndicateTimeout,
                                          true);

        uint8_t addr[8] = {0};
        interface_mac_t mac;

        // Retrieve the MAC address of the source which last triggered the light
        interface_light_get_mac_trigger(&mac);
        memcpy(addr, (uint8_t *)&mac, sizeof(mac));

        /* Send notification/indication data */
        gecko_cmd_gatt_server_send_characteristic_notification(ble_conn[0].handle,
                                                               SOURCE_ADDRESS_GATTDB,
                                                               sizeof(addr),
                                                               addr);
        res = 0;
      }
      break;
    }
  }

  return res;
}


int bluetooth_app_init(gecko_configuration_t *pconfig)
{
  int res;

  // Adapt the default configuration to the application
#if DISABLE_SLEEP > 0
  pconfig->sleep.flags = 0;
#endif

  pconfig->mbedtls.flags = GECKO_MBEDTLS_FLAGS_NO_MBEDTLS_DEVICE_INIT,
  pconfig->mbedtls.dev_number = 0,

  /* Initialize stack */
  res = gecko_stack_init(pconfig);
  gecko_bgapi_class_system_init();
  gecko_bgapi_class_le_gap_init();
  gecko_bgapi_class_le_connection_init();
  gecko_bgapi_class_gatt_init();
  gecko_bgapi_class_gatt_server_init();
  gecko_bgapi_class_hardware_init();

  ble_config = pconfig;

  return res;
}

void bluetooth_app_process(void)
{
  struct gecko_cmd_packet* evt;
  int res;

  // Check if an indication is pending
  if (ble_indication_pending & !ble_indication_ongoing) {
    ble_indication_pending = false;
    ble_indication_ongoing = true;
    bluetooth_app_send_indication(LIGHT_STATE_GATTDB);
  }

  // Treat BLE events
  do {
    evt = gecko_peek_event();
    if (evt != NULL) {
      /* Handle events */
      switch (BGLIB_MSG_ID(evt->header)) {

        /* This boot event is generated when the system boots up after reset.
         * Do not call any stack commands before receiving the boot event. */
        case gecko_evt_system_boot_id:
        {
          struct gecko_msg_system_get_bt_address_rsp_t *addr = gecko_cmd_system_get_bt_address();
          memcpy((void *)&ble_own_addr, (void *)&addr->address, sizeof(ble_own_addr));
          bootMessage(&(evt->data.evt_system_boot), &ble_own_addr);

          // Update the LCD display with the BLE Id
          interface_display_ble_id((uint8_t *)&ble_own_addr.addr);

          printf("BLE: boot event - start advertising\r\n");
          // Initialize the advertising name and start advertising
          snprintf(ble_own_name, DEVNAME_LEN, "MP%02X%02X", ble_own_addr.addr[1], ble_own_addr.addr[0]);
          bluetooth_app_start_advertising();
          break;
        }

        case gecko_evt_le_connection_opened_id:
        {
          ble_nb_connected++;
          ble_state |= BLE_STATE_CONNECTED;

          bleConnAdd(evt->data.evt_le_connection_opened.connection,
                     &evt->data.evt_le_connection_opened.address);

          printf("BLE: connection opened %d (%02X%02X%02X%02X%02X%02X)\r\n",
                 evt->data.evt_le_connection_opened.connection,
                 evt->data.evt_le_connection_opened.address.addr[5],
                 evt->data.evt_le_connection_opened.address.addr[4],
                 evt->data.evt_le_connection_opened.address.addr[3],
                 evt->data.evt_le_connection_opened.address.addr[2],
                 evt->data.evt_le_connection_opened.address.addr[1],
                 evt->data.evt_le_connection_opened.address.addr[0]);

          // Restart the advertising with scan response (scannable ability only) which is
          // automatically stopped after a connection (not the beacons)
          bluetooth_app_start_advertising();

          if (ble_nb_connected == 1) {
            interface_display_ble_state(true);

            // Start the timer managing the sending of indications
            gecko_cmd_hardware_set_soft_timer(TIMER_MS_2_TIMERTICK(INDICATION_TIMER_PERIOD_MSEC),
                                              bleTimerIndicatePeriod,
                                              false);

            // Start the procedure to retrieve the remote device name
            uint16_t ga_service = BLE_GATT_SERVICE_GENERIC_ACCESS_UUID;
            gecko_cmd_gatt_discover_primary_services_by_uuid(BLE_MASTER_CONNECTION,
                                                             2,
                                                             (uint8_t *)&ga_service);
          }
          break;
        }

        case gecko_evt_le_connection_closed_id:
        {
          ble_nb_connected--;
          bleConnRemove(evt->data.evt_le_connection_closed.connection);
          printf("BLE: connection closed, reason: %#02x\r\n", evt->data.evt_le_connection_closed.reason);

          if (ble_nb_connected == 0) {
            ble_state &= ~BLE_STATE_CONNECTED;
            interface_display_ble_state(false);

            /* No device connected, stop sending indications */
            gecko_cmd_hardware_set_soft_timer(0, bleTimerIndicatePeriod, false);
            gecko_cmd_hardware_set_soft_timer(0, bleTimerIndicateTimeout, false);
            ble_indication_ongoing = false;
            ble_indication_pending = false;

            // Restart the advertising with connectable ability
            bluetooth_app_start_advertising();
          }
          break;
        }

        /* This event indicates that a remote GATT client is attempting to write a value of an
         * attribute in to the local GATT database, where the attribute was defined in the GATT
         * XML firmware configuration file to have type="user".  */
        case gecko_evt_gatt_server_user_write_request_id:
        {
          switch (evt->data.evt_gatt_server_user_write_request.characteristic) {

            case LIGHT_STATE_GATTDB:
            {
              BleConn_t *connPoi = bleConnGet(evt->data.evt_gatt_server_user_write_request.connection);
              if (connPoi != NULL) {
                interface_light_set_state(interface_light_trigger_src_bluetooth,
                                          &connPoi->address,
                                          evt->data.evt_gatt_server_user_write_request.value.data[0]);

                /* Send response to write request */
                gecko_cmd_gatt_server_send_user_write_response(evt->data.evt_gatt_server_user_write_request.connection,
                                                               LIGHT_STATE_GATTDB,
                                                               0);
              }
              break;
            }
          }
          break;
        }

        /* This event indicates that a remote GATT client is attempting to read a value of an
         *  attribute from the local GATT database, where the attribute was defined in the GATT
         *  XML firmware configuration file to have type="user". */
        case gecko_evt_gatt_server_user_read_request_id:
        {
          switch (evt->data.evt_gatt_server_user_read_request.characteristic) {

            /* Light state read */
            case LIGHT_STATE_GATTDB:
            {
              uint8_t light_state = interface_light_get_state();

              /* Send response to read request */
              gecko_cmd_gatt_server_send_user_read_response(evt->data.evt_gatt_server_user_read_request.connection,
                                                            LIGHT_STATE_GATTDB,
                                                            0,
                                                            sizeof(light_state),
                                                            (uint8_t*)&light_state);
              break;
            }

            /* Trigger source read */
            case TRIGGER_SOURCE_GATTDB:
            {
              uint8_t trigger = (uint8_t)interface_light_get_trigger();

              /* Send response to read request */
              gecko_cmd_gatt_server_send_user_read_response(evt->data.evt_gatt_server_user_read_request.connection,
                                                            TRIGGER_SOURCE_GATTDB,
                                                            0,
                                                            sizeof(trigger),
                                                            &trigger);
              break;
            }

            /* Source address read */
            case SOURCE_ADDRESS_GATTDB:
            {
              uint8_t addr[8] = {0};
              interface_mac_t mac;

              // Retrieve the MAC address of the source which last triggered the light
              interface_light_get_mac_trigger(&mac);
              memcpy(addr, (uint8_t *)&mac, sizeof(mac));

              /* Send response to read request */
              gecko_cmd_gatt_server_send_user_read_response(evt->data.evt_gatt_server_user_read_request.connection,
                                                            SOURCE_ADDRESS_GATTDB,
                                                            0,
                                                            sizeof(addr),
                                                            addr);
              break;
            }
          }
          break;
        }

        /* This event indicates either that a local Client Characteristic Configuration descriptor
         * has been changed by the remote GATT client, or that a confirmation from the remote GATT
         * client was received upon a successful reception of the indication. */
        case gecko_evt_gatt_server_characteristic_status_id:
        {
          switch (evt->data.evt_gatt_server_characteristic_status.characteristic) {

            case LIGHT_STATE_GATTDB:
            {
              switch ((enum gatt_server_characteristic_status_flag)evt->data.evt_gatt_server_characteristic_status.status_flags) {

                // confirmation of indication received from remote GATT client
                case gatt_server_confirmation:
                  // Notification received from the remote GAT, send the next indication
                  bluetooth_app_send_indication(TRIGGER_SOURCE_GATTDB);
                  break;

                  // client characteristic configuration changed by remote GATT client
                case gatt_server_client_config:
                  light_state_gatt_flag = (enum gatt_client_config_flag)evt->data.evt_gatt_server_characteristic_status.client_config_flags;
                  break;
              }
              break;
            }

            case TRIGGER_SOURCE_GATTDB:
            {
              switch ((enum gatt_server_characteristic_status_flag)evt->data.evt_gatt_server_characteristic_status.status_flags) {

                // confirmation of indication received from remote GATT client
                case gatt_server_confirmation:
                  // Notification received from the remote GAT, send the next indication
                  bluetooth_app_send_indication(SOURCE_ADDRESS_GATTDB);
                  break;

                  // client characteristic configuration changed by remote GATT client
                case gatt_server_client_config:
                  trigger_source_gatt_flag = (enum gatt_client_config_flag)evt->data.evt_gatt_server_characteristic_status.client_config_flags;
                  break;
              }
              break;
            }

            case SOURCE_ADDRESS_GATTDB:
            {
              switch ((enum gatt_server_characteristic_status_flag)evt->data.evt_gatt_server_characteristic_status.status_flags) {
                // confirmation of indication received from remote GATT client
                case gatt_server_confirmation:
                  // All indications sent successfully, stop the timer
                  gecko_cmd_hardware_set_soft_timer(0, bleTimerIndicateTimeout, false);

                  ble_indication_ongoing = false;
                  break;

                  // client characteristic configuration changed by remote GATT client
                case gatt_server_client_config:
                  source_address_gatt_flag = (enum gatt_client_config_flag)evt->data.evt_gatt_server_characteristic_status.client_config_flags;
                  break;
              }
              break;
            }
          }
          break;
        }

        case gecko_evt_le_gap_scan_request_id:
          break;

        case gecko_evt_le_gap_scan_response_id:
          break;

        case gecko_evt_gatt_service_id:
        {
          // Store the service handle
          gatt_service_handle = evt->data.evt_gatt_service.service;
          break;
        }

        case gecko_evt_gatt_characteristic_id:
        {
          // Store the handle of the Name Characteristic
          gatt_name_characteristic_handle = evt->data.evt_gatt_characteristic.characteristic;
          break;
        }

        case gecko_evt_gatt_characteristic_value_id:
        {
          // Name retrieved, update the connection information
          BleConn_t *conn = bleConnGet(BLE_MASTER_CONNECTION);
          if (conn != NULL) {
            uint8_t min_len = evt->data.evt_gatt_characteristic_value.value.len < BLE_MASTER_NAME_MAX_LEN ?
                              evt->data.evt_gatt_characteristic_value.value.len : BLE_MASTER_NAME_MAX_LEN;
            memcpy(conn->name,
                   evt->data.evt_gatt_characteristic_value.value.data,
                   min_len);
            conn->name[min_len] = '\0';
          }
          break;
        }

        case gecko_evt_gatt_procedure_completed_id:
        {
          if (evt->data.evt_gatt_procedure_completed.result == 0) {

            switch (name_retrieval_step) {
              case get_gatt_service_handle:
              {
                struct gecko_msg_gatt_discover_characteristics_by_uuid_rsp_t *resp;

                // Discover the Name Characteristic of the service
                uint16_t uuid = BLE_GATT_CHARACTERISTIC_DEVICE_NAME_UUID;
                resp = gecko_cmd_gatt_discover_characteristics_by_uuid(BLE_MASTER_CONNECTION,
                                                                       gatt_service_handle,
                                                                       2,
                                                                       (uint8_t*)&uuid);

                if (resp->result == 0) {
                  // Operation success, go to the next step
                  name_retrieval_step = get_gatt_name_characteristic_handle;
                }
                break;
              }

              case get_gatt_name_characteristic_handle:
              {
                struct gecko_msg_gatt_read_characteristic_value_rsp_t *resp;

                // Request the value of the Name characteristic
                resp = gecko_cmd_gatt_read_characteristic_value(BLE_MASTER_CONNECTION,
                                                                gatt_name_characteristic_handle);

                if (resp->result == 0) {
                  // Operation success, go to the next step
                  name_retrieval_step = get_gatt_name_characteristic_value;
                }
                break;
              }

              case get_gatt_name_characteristic_value:
                // Nothing to do here, the value has already been stored
                name_retrieval_step = get_gatt_service_handle;
                break;
            }
          }

          break;
        }

          /* Software Timer event */
        case gecko_evt_hardware_soft_timer_id:
        {
          switch (evt->data.evt_hardware_soft_timer.handle) {

            /* Indication period reached */
            case bleTimerIndicatePeriod:
              if (!ble_indication_ongoing) {
                ble_indication_ongoing = true;

                // Start sending BLE indications
                res = bluetooth_app_send_indication(LIGHT_STATE_GATTDB);
                if (res != 0) {
                  // Error, no indications are sent as long as the remote GATT doesn't read the characteristics first.
                  ble_indication_ongoing = false;
                }
              } else {
                ble_indication_pending = true;
              }
              break;

              /* Indication timeout */
            case bleTimerIndicateTimeout:
              ble_indication_ongoing = false;
              printf("BLE: Indication timeout\r\n");
              break;
          }
          break;
        }

        default:
          break;
      }
    }
  } while (evt != NULL);
}

void bluetooth_app_request_send_indication (void)
{
  if (ble_nb_connected > 0) {
    ble_indication_pending = true;
  }
}

void bluetooth_app_start_advertising (void)
{
  int res;

  res = enableBleAdvertisements(ble_own_name,
                                ble_config->bluetooth.max_advertisers,
                                ble_config->bluetooth.max_connections > ble_nb_connected);
  if (res == 0) {
    ble_state |= BLE_STATE_ADVERTISING;
  }
}

void bluetooth_app_stop_advertising (void)
{
  int res = 0;

  res = disableBleAdvertisements();
  if (res == 0) {
    ble_state &= ~BLE_STATE_ADVERTISING;
  }
}

void bluetooth_app_disconnect_master (void)
{
  gecko_cmd_le_connection_close(BLE_MASTER_CONNECTION);
}


uint8_t bluetooth_app_get_ble_state (void)
{
  return ble_state;
}

void bluetooth_app_get_own_mac (bd_addr *mac)
{
  if (mac != NULL) {
    memcpy(mac, &ble_own_addr, sizeof(bd_addr));
  }
}

int bluetooth_app_get_master_mac (bd_addr *mac)
{
  BleConn_t *conn;
  int ret = -1;

  if (mac != NULL) {
    conn = bleConnGet(BLE_MASTER_CONNECTION);
    if (conn != NULL) {
      memcpy(mac, &conn->address, sizeof(bd_addr));
      ret = 0;
    }
  }

  return ret;
}

void bluetooth_app_get_own_name (char *name, int name_size)
{
  if (name != NULL) {
    strncpy(name, ble_own_name, name_size);
  }
}

int bluetooth_app_get_master_name (char *name, int name_size)
{
  BleConn_t *conn;
  int ret = -1;

  if (name != NULL) {
    conn = bleConnGet(BLE_MASTER_CONNECTION);
    if (conn != NULL) {
      strncpy(name, conn->name, name_size);
      name[name_size-1] = '\0';
      ret = 0;
    }
  }

  return ret;
}
