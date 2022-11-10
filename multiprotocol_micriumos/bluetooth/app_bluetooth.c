/***************************************************************************//**
 * @file app_bluetooth.c 
 * @brief Core bluetooth application logic.
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
#include <stdbool.h>
#include <stdio.h>
#include "em_common.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"
#include "app_bluetooth.h"

#if !(BRD4187X || BRD4180_81_X)
#include "app_assert.h"
#endif
char boot_message[MAX_BUF_LEN];

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

static sl_bt_gatt_client_config_flag_t light_state_gatt_flag = sl_bt_gatt_disable;
static sl_bt_gatt_client_config_flag_t trigger_source_gatt_flag = sl_bt_gatt_disable;
static sl_bt_gatt_client_config_flag_t source_address_gatt_flag = sl_bt_gatt_disable;

static bool ble_indication_pending = false;
static bool ble_indication_ongoing = false;

static uint8_t ble_nb_connected = 0;
static uint8_t ble_state = 0;

static ble_conn_t ble_conn[SL_BT_CONFIG_MAX_CONNECTIONS];
static bd_addr ble_own_addr = {0};
static char ble_own_name[DEVNAME_LEN] = {0};

static uint32_t gatt_service_handle = 0;
static uint16_t gatt_name_characteristic_handle = 0;
static ble_name_retrieval_step_t name_retrieval_step = get_gatt_service_handle;

// Timers for periodic & timeout indication
static sl_simple_timer_t app_bt_timers[2]; // periodic & timeout timer handles
static ble_timer_t g_timer_types[2] = {ble_periodic_indication_timer, ble_indication_timeout_timer};
static void app_single_timer_cb(sl_simple_timer_t *handle, void *data);

/* Print stack version and local Bluetooth address as boot message */
static void get_ble_boot_msg(sl_bt_msg_t *bootevt, bd_addr *local_addr)
{
  snprintf(boot_message, MAX_BUF_LEN, "BLE stack version: %u.%u.%u\r\n"
          "Local BT device address: %02X:%02X:%02X:%02X:%02X:%02X",
          bootevt->data.evt_system_boot.major,
          bootevt->data.evt_system_boot.minor,
          bootevt->data.evt_system_boot.patch,
          local_addr->addr[5],
          local_addr->addr[4],
          local_addr->addr[3],
          local_addr->addr[2],
          local_addr->addr[1],
          local_addr->addr[0]);
}

/***************************************************************************//**
 * @brief
 *    This function adds ble handle & address to the managed array. Used for
 *    multiple connected devices
 * @param[in]
 *    + handle: BLE handle
 *    + address: BLE address
 * @param[out] None
 * @return
 *    true if success
 *    false if fail
 ******************************************************************************/
static bool add_ble_conn(uint8_t handle, bd_addr* address)
{
  for (uint8_t i = 0; i < SL_BT_CONFIG_MAX_CONNECTIONS; i++) {
    if (ble_conn[i].inUse == false) {
      ble_conn[i].handle = handle;
      memcpy((void*)&ble_conn[i].address, 
              (void*)address, 
              sizeof(ble_conn[i].address));
      ble_conn[i].inUse = true;
      return true;
    }
  }
  return false;
}

/***************************************************************************//**
 * @brief
 *    This function removes BLE handle & address to the managed array. Used for
 *    multiple connected devices
 * @param[in]
 *    + handle: BLE handle 
 * @param[out] None
 * @return
 *    true if success
 *    false if fail
 ******************************************************************************/
static bool remove_ble_conn(uint8_t handle)
{
  for (uint8_t i = 0; i < SL_BT_CONFIG_MAX_CONNECTIONS; i++) {
    if (ble_conn[i].handle == handle) {
      ble_conn[i].handle = 0;
      memset((void*)&ble_conn[i].address.addr, 
              0, 
              sizeof(ble_conn[i].address.addr));
      ble_conn[i].inUse = false;
      return true;
    }
  }
  return false;
}

/***************************************************************************//**
 * @brief
 *    This function returns the connection pointer corresponding to the BLE handle
 * @param[in]
 *    + handle: BLE handle 
 * @param[out] None
 * @return
 *    ble_conn_t* if success
 *    NULL otherwise
 ******************************************************************************/
static ble_conn_t* get_ble_conn(uint8_t handle)
{
  for (uint8_t i = 0; i < SL_BT_CONFIG_MAX_CONNECTIONS; i++) {
    if (ble_conn[i].handle == handle) {
      return &ble_conn[i];
    }
  }
  return NULL;
}

/***************************************************************************//**
 * @brief  Request send indication
 * @param[in]   None
 * @param[out]  None
 * @return      None
 ******************************************************************************/
void bluetooth_app_request_send_indication(void)
{
  if (ble_nb_connected > 0) {
    ble_indication_pending = true;
  }
}

/***************************************************************************//**
 * @brief
 *    This function sends indication of the characteristic to BLE client
 * @param[in]
 *    + ble_characteristic: GATT characteristic
 * @param[out] None
 * @return
 *    0 if success
 *    -1 otherwise
 ******************************************************************************/
static int bluetooth_app_send_indication(uint16_t ble_characteristic)
{
  int res = -1;

  switch (ble_characteristic) {

    case LIGHT_STATE_GATTDB:
    {
      // Stop light indication confirmation timer
      sl_simple_timer_stop(&app_bt_timers[ble_indication_timeout_timer]);

      if (light_state_gatt_flag == sl_bt_gatt_indication) {
        uint8_t light_state = interface_light_get_state();

        // Start timer for light indication confirmation
        sl_simple_timer_start(&app_bt_timers[ble_indication_timeout_timer],
                              INDICATION_TIMER_TIMEOUT_MSEC,
                              app_single_timer_cb,
                              &g_timer_types[ble_indication_timeout_timer],
                              false);

        /* Send notification/indication data */
        sl_bt_gatt_server_send_indication(ble_conn[0].handle,
                                          LIGHT_STATE_GATTDB,
                                          sizeof(light_state),
                                          (uint8_t*)&light_state);
        res = 0;
      }
      break;
    }

    case TRIGGER_SOURCE_GATTDB:
    {
      // Stop light indication confirmation timer
      sl_simple_timer_stop(&app_bt_timers[ble_indication_timeout_timer]);
      if (trigger_source_gatt_flag == sl_bt_gatt_indication) {
        uint8_t trigger = (uint8_t)interface_light_get_trigger();

        // Start timer for trigger source indication confirmation
        sl_simple_timer_start(&app_bt_timers[ble_indication_timeout_timer],
                              INDICATION_TIMER_TIMEOUT_MSEC,
                              app_single_timer_cb,
                              &g_timer_types[ble_indication_timeout_timer],
                              false);

        /* Send notification/indication data */
        sl_bt_gatt_server_send_indication(ble_conn[0].handle,
                                         TRIGGER_SOURCE_GATTDB,
                                         sizeof(trigger),
                                         &trigger);
        res = 0;
      }
      break;
    }

    case SOURCE_ADDRESS_GATTDB:
    {
      // Stop light indication confirmation timer
      sl_simple_timer_stop(&app_bt_timers[ble_indication_timeout_timer]);
      if (source_address_gatt_flag == sl_bt_gatt_indication) {
        // Start timer for source address indication confirmation
        sl_simple_timer_start(&app_bt_timers[ble_indication_timeout_timer],
                              INDICATION_TIMER_TIMEOUT_MSEC,
                              app_single_timer_cb,
                              &g_timer_types[ble_indication_timeout_timer],
                              false);

        uint8_t addr[8] = {0};
        interface_mac_t mac;

        // Retrieve the MAC address of the source which last triggered the light
        interface_light_get_mac_trigger(&mac);
        memcpy(addr, (uint8_t *)&mac, sizeof(mac));

        /* Send notification/indication data */
        sl_bt_gatt_server_send_indication(ble_conn[0].handle,
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

/***************************************************************************//**
 * @brief
 *    This function starts BLE advertisement
 * @param[in]   None
 * @param[out]  None
 * @return      None
 ******************************************************************************/
void bluetooth_app_start_advertising(void)
{
  sl_status_t sc;
  bool is_connectable = ble_nb_connected < SL_BT_CONFIG_MAX_CONNECTIONS;

  // Start general advertising and enable connections.
  sc = sl_bt_advertiser_start(advertising_set_handle,
                              sl_bt_advertiser_general_discoverable,
                              is_connectable ?
                              sl_bt_advertiser_connectable_scannable :
                              sl_bt_advertiser_scannable_non_connectable);
#if !(BRD4187X || BRD4180_81_X)
  app_assert_status(sc);
#endif

  if (sc == SL_STATUS_OK) {
    ble_state |= BLE_STATE_ADVERTISING;
  }
}

/***************************************************************************//**
 * @brief
 *    This function stops BLE advertisement
 * @param[in]   None
 * @param[out]  None
 * @return      None
 ******************************************************************************/
void bluetooth_app_stop_advertising(void)
{
  sl_status_t sc;

  sc = sl_bt_advertiser_stop(advertising_set_handle);
#if !(BRD4187X || BRD4180_81_X)
  app_assert_status(sc);  
#endif

  if (sc == SL_STATUS_OK) {
    ble_state &= ~BLE_STATE_ADVERTISING;
  }
}

/**************************************************************************//**
 * @brief Bluetooth app's timer callback
 * @param[in] handle timer handle
 * @param[in] data additional data
 *****************************************************************************/
static void app_single_timer_cb(sl_simple_timer_t *handle,
                                void *data)
{
  (void)handle;
  int res = -1;

  ble_timer_t *p_timer_type = data;
  int timer_type = (int) *p_timer_type;

  switch (timer_type) {

    /* Indication period reached */
    case ble_periodic_indication_timer:      
      if (!ble_indication_ongoing) {
        ble_indication_ongoing = true;

        // Start sending BLE indications
        res = bluetooth_app_send_indication(LIGHT_STATE_GATTDB);
        if (res != 0) {
          /** Error, no indications are sent as long as the remote GATT doesn't 
           * read the characteristics first. */
          ble_indication_ongoing = false;
        }
      } else {
        ble_indication_pending = true;
      }
      break;

      /* Indication timeout */
    case ble_indication_timeout_timer:
      ble_indication_ongoing = false;
      break;
   }
}

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application code here!                              //
  // This is called infinitely.                                              //
  // Do not call blocking functions from here!                               //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  bd_addr address;
  uint8_t address_type;
  uint8_t system_id[8];

  // Check if an indication is pending
  if (ble_indication_pending & !ble_indication_ongoing) {
    ble_indication_pending = false;
    ble_indication_ongoing = true;
    bluetooth_app_send_indication(LIGHT_STATE_GATTDB);
  }

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:

      // Extract unique ID from BT Address.
      sc = sl_bt_system_get_identity_address(&address, &address_type);
#if !(BRD4187X || BRD4180_81_X)
      app_assert_status(sc);
#endif

      // Pad and reverse unique ID to get System ID.
      system_id[0] = address.addr[5];
      system_id[1] = address.addr[4];
      system_id[2] = address.addr[3];
      system_id[3] = 0xFF;
      system_id[4] = 0xFE;
      system_id[5] = address.addr[2];
      system_id[6] = address.addr[1];
      system_id[7] = address.addr[0];

      sc = sl_bt_gatt_server_write_attribute_value(gattdb_system_id,
                                                   0,
                                                   sizeof(system_id),
                                                   system_id);
#if !(BRD4187X || BRD4180_81_X)
      app_assert_status(sc);
#endif

      memcpy((void *)&ble_own_addr, (void *)&address, sizeof(ble_own_addr));

      // Boot_messages
      get_ble_boot_msg(evt, &address);

      // Update the LCD display with the BLE Id
      interface_display_ble_id((uint8_t *)&ble_own_addr.addr);
      snprintf(ble_own_name, DEVNAME_LEN, "MP%02X%02X",
               ble_own_addr.addr[1], ble_own_addr.addr[0]);

      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
#if !(BRD4187X || BRD4180_81_X)
      app_assert_status(sc);
#endif

      // Change the default device name (MP Demo)
      sc = sl_bt_gatt_server_write_attribute_value(DEV_NAME_GATDB,
                                              0,
                                              strlen(ble_own_name),
                                              (const uint8_t *)ble_own_name);
#if !(BRD4187X || BRD4180_81_X)
      app_assert_status(sc);
#endif

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
                              advertising_set_handle,
                              160, // min. adv. interval (milliseconds * 1.6)
                              160, // max. adv. interval (milliseconds * 1.6)
                              0,   // adv. duration
                              0);  // max. num. adv. events
#if !(BRD4187X || BRD4180_81_X)
      app_assert_status(sc);
#endif
      
      // Start general advertising and enable connections.
      bluetooth_app_start_advertising();
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      ble_nb_connected++;
      ble_state |= BLE_STATE_CONNECTED;

      add_ble_conn(evt->data.evt_connection_opened.connection,
                 &evt->data.evt_connection_opened.address);

      printf("BLE: connection opened %d"
            " (MAC address %02X:%02X:%02X:%02X:%02X:%02X)\r\n",
             evt->data.evt_connection_opened.connection,
             evt->data.evt_connection_opened.address.addr[5],
             evt->data.evt_connection_opened.address.addr[4],
             evt->data.evt_connection_opened.address.addr[3],
             evt->data.evt_connection_opened.address.addr[2],
             evt->data.evt_connection_opened.address.addr[1],
             evt->data.evt_connection_opened.address.addr[0]);

      // Restart the advertising with scan response (scannable ability only) 
      // which is automatically stopped after a connection (not the beacons)
      bluetooth_app_start_advertising();

      if (ble_nb_connected == 1) {
          interface_display_ble_state(true);

          // Start timer for indication
          sl_simple_timer_start(&app_bt_timers[ble_periodic_indication_timer],
                                INDICATION_TIMER_PERIOD_MSEC,
                                app_single_timer_cb,
                                &g_timer_types[ble_periodic_indication_timer],
                                true);

          // Start the procedure to retrieve the remote device name
          uint16_t ga_service = BLE_GATT_SERVICE_GENERIC_ACCESS_UUID;
          sl_bt_gatt_discover_primary_services_by_uuid(BLE_MASTER_CONNECTION,
                                                        2,
                                                        (uint8_t *)&ga_service);
      }
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      ble_nb_connected--;
      remove_ble_conn(evt->data.evt_connection_closed.connection);
      printf("BLE: connection closed, reason: %#02x\r\n", 
              evt->data.evt_connection_closed.reason);

      if (ble_nb_connected == 0) {
        ble_state &= ~BLE_STATE_CONNECTED;
        interface_display_ble_state(false);

        /* No device connected, stop sending indications */
        sl_simple_timer_stop(&app_bt_timers[ble_periodic_indication_timer]);
        sl_simple_timer_stop(&app_bt_timers[ble_indication_timeout_timer]);
        ble_indication_ongoing = false;
        ble_indication_pending = false;
      }

      /** Restart after client has disconnected & 
       * advertising with connectable ability*/ 
      bluetooth_app_start_advertising();
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////
    /** This event indicates that a remote GATT client is attempting to write 
    * a value of an attribute in to the local GATT database, where the attribute
    * was defined in the GATT XML firmware configuration file to have 
    * type="user".
    * */
    case sl_bt_evt_gatt_server_user_write_request_id:
    {

      switch (evt->data.evt_gatt_server_user_write_request.characteristic) {
        case LIGHT_STATE_GATTDB: {
          ble_conn_t *p_conn = get_ble_conn(evt->data.evt_gatt_server_user_write_request.connection);
          if (p_conn != NULL) {
            interface_light_set_state(interface_light_trigger_src_bluetooth,
                                      &p_conn->address,
                                      evt->data.evt_gatt_server_user_write_request.value.data[0]);
            
            /* Send response to write request */
            sl_bt_gatt_server_send_user_write_response(evt->data.evt_gatt_server_user_write_request.connection,
                                                        LIGHT_STATE_GATTDB,
                                                        0);
          }
          break;
        }
      }
      
      break;
    }
      
    /** This event indicates that a remote GATT client is attempting to read 
     * a value of an attribute from the local GATT database, where the attribute 
     * was defined in the GATT XML firmware configuration file to have type="user". 
     * */
    case sl_bt_evt_gatt_server_user_read_request_id:
    {   
    
      uint16_t sent_len = 0;
      switch (evt->data.evt_gatt_server_user_read_request.characteristic) 
      {

        /* Light state read */
        case LIGHT_STATE_GATTDB:
        {
          uint8_t light_state = interface_light_get_state();
          

          /* Send response to read request */
          sl_bt_gatt_server_send_user_read_response(evt->data.evt_gatt_server_user_read_request.connection,
                                                    LIGHT_STATE_GATTDB,
                                                    0,
                                                    sizeof(light_state),
                                                    (uint8_t*)&light_state,
                                                    &sent_len);
          break;
        }

        /* Trigger source read */
        case TRIGGER_SOURCE_GATTDB:
        {
          uint8_t trigger = (uint8_t)interface_light_get_trigger();

          /* Send response to read request */
          sl_bt_gatt_server_send_user_read_response(evt->data.evt_gatt_server_user_read_request.connection,
                                                    TRIGGER_SOURCE_GATTDB,
                                                    0,
                                                    sizeof(trigger),
                                                    &trigger,
                                                    &sent_len);
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
          sl_bt_gatt_server_send_user_read_response(evt->data.evt_gatt_server_user_read_request.connection,
                                                    SOURCE_ADDRESS_GATTDB,
                                                    0,
                                                    sizeof(addr),
                                                    addr,
                                                    &sent_len);
          break;
        }
      }
      break;
    }

    /** This event indicates either that a local Client Characteristic
     * Configuration descriptor has been changed by the remote GATT client, or 
     * that a confirmation from the remote GATT client was received upon 
     * a successful reception of the indication.
     * */
    case sl_bt_evt_gatt_server_characteristic_status_id:
    {
      
      switch (evt->data.evt_gatt_server_characteristic_status.characteristic) {

        case LIGHT_STATE_GATTDB:
        {
          switch ((sl_bt_gatt_server_characteristic_status_flag_t)evt->data.evt_gatt_server_characteristic_status.status_flags) {

            // confirmation of indication received from remote GATT client
            case sl_bt_gatt_server_confirmation:
              // Notification received from the remote GAT, send the next indication
              bluetooth_app_send_indication(TRIGGER_SOURCE_GATTDB);
              break;

              // client characteristic configuration changed by remote GATT client
            case sl_bt_gatt_server_client_config:
              light_state_gatt_flag = (sl_bt_gatt_client_config_flag_t)evt->data.evt_gatt_server_characteristic_status.client_config_flags;
              break;
          }
          break;
        }

        case TRIGGER_SOURCE_GATTDB:
        {
          switch ((sl_bt_gatt_server_characteristic_status_flag_t)evt->data.evt_gatt_server_characteristic_status.status_flags) {

            // Confirmation of indication received from remote GATT client
            case sl_bt_gatt_server_confirmation:
              // Notification received from the remote GAT, send the next indication
              bluetooth_app_send_indication(SOURCE_ADDRESS_GATTDB);
              break;

              // Client characteristic configuration changed by remote GATT client
            case sl_bt_gatt_server_client_config:
              trigger_source_gatt_flag = (sl_bt_gatt_client_config_flag_t)evt->data.evt_gatt_server_characteristic_status.client_config_flags;
              break;
          }
          break;
        }

        case SOURCE_ADDRESS_GATTDB:
        {
          switch ((sl_bt_gatt_server_characteristic_status_flag_t)evt->data.evt_gatt_server_characteristic_status.status_flags) {
            // Confirmation of indication received from remote GATT client
            case sl_bt_gatt_server_confirmation:
              // All indications sent successfully, stop the timer
              sl_simple_timer_stop(&app_bt_timers[ble_indication_timeout_timer]);

              ble_indication_ongoing = false;
              break;

              // Client characteristic configuration changed by remote GATT client
            case sl_bt_gatt_server_client_config:
              source_address_gatt_flag = (sl_bt_gatt_client_config_flag_t)evt->data.evt_gatt_server_characteristic_status.client_config_flags;
              break;
          }
          break;
        }
      }
      break;
    }

    case sl_bt_evt_advertiser_scan_request_id:
      break;

    case sl_bt_evt_scanner_scan_report_id:
      break;

    case sl_bt_evt_gatt_service_id:
    {
      // Store the service handle
      gatt_service_handle = evt->data.evt_gatt_service.service;
      break;
    }

    case sl_bt_evt_gatt_characteristic_id:
    {
      // Store the handle of the Name Characteristic
      gatt_name_characteristic_handle = evt->data.evt_gatt_characteristic.characteristic;
      break;
    }

    case sl_bt_evt_gatt_characteristic_value_id:
    {
      // Name retrieved, update the connection information
      ble_conn_t *conn = get_ble_conn(BLE_MASTER_CONNECTION);
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
    case sl_bt_evt_gatt_procedure_completed_id:
    {
      if (evt->data.evt_gatt_procedure_completed.result == 0) {

        switch (name_retrieval_step) {
          case get_gatt_service_handle:
          {
            sl_status_t resp;

            // Discover the Name Characteristic of the service
            uint16_t uuid = BLE_GATT_CHARACTERISTIC_DEVICE_NAME_UUID;
            resp = sl_bt_gatt_discover_characteristics_by_uuid(BLE_MASTER_CONNECTION,
                                                             gatt_service_handle,
                                                             2,
                                                             (uint8_t*)&uuid);

            if (resp == SL_STATUS_OK) {
              // Operation success, go to the next step
              name_retrieval_step = get_gatt_name_characteristic_handle;
            }
            break;
          }

          case get_gatt_name_characteristic_handle:
          {
            sl_status_t resp;
            // Request the value of the Name characteristic
            resp = sl_bt_gatt_read_characteristic_value(BLE_MASTER_CONNECTION,
                                                gatt_name_characteristic_handle);

            if (resp == SL_STATUS_OK) {
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

    // Default event handler.
    default:
      break;
  }
}

/***************************************************************************//**
 * @brief
 *    This function returns BLE state of the device
 * @param[in]   None
 * @param[out]  None
 * @return      BLE_STATE_ADVERTISING | BLE_STATE_CONNECTED
 ******************************************************************************/
uint8_t bluetooth_app_get_ble_state(void)
{
  return ble_state;
}

/***************************************************************************//**
 * @brief
 *    This function returns BLE mac address of the device
 * @param[in]   None
 * @param[out]  mac Device BLE mac address
 * @return      None
 ******************************************************************************/
void bluetooth_app_get_own_mac(bd_addr *mac)
{
  if (mac != NULL) {
    memcpy(mac, &ble_own_addr, sizeof(bd_addr));
  }
}

/***************************************************************************//**
 * @brief
 *    This function returns BLE mac address of the connected device
 * @param[in]   None
 * @param[out]  mac Device BLE mac address of the connected device
 * @return      0 if success, otherwise -1
 ******************************************************************************/
int bluetooth_app_get_master_mac(bd_addr *mac)
{
  ble_conn_t *conn;
  int ret = -1;

  if (mac != NULL) {
    conn = get_ble_conn(BLE_MASTER_CONNECTION);
    if (conn != NULL) {
      memcpy(mac, &conn->address, sizeof(bd_addr));
      ret = 0;
    }
  }

  return ret;
}

/***************************************************************************//**
 * @brief
 *    This function returns BLE device name
 * @param[in]   name_size Size of output array
 * @param[out]  name Output array contains device name
 * @return      None
 * @note        Make sure the name_size is large enough for NULL terminated
 ******************************************************************************/
void bluetooth_app_get_own_name(char *name, int name_size)
{
  if (name != NULL) {
    strncpy(name, ble_own_name, name_size);
  }
}

/***************************************************************************//**
 * @brief
 *    This function returns the name of connected BLE device 
 * @param[in]   name_size Size of output array
 * @param[out]  name Output array contains connected device name
 * @return      0 if success otherwise -1
 * @note        Make sure the name_size is large enough for NULL terminated
 ******************************************************************************/
int bluetooth_app_get_master_name(char *name, int name_size)
{
  ble_conn_t *conn;
  int ret = -1;

  if (name != NULL) {
    conn = get_ble_conn(BLE_MASTER_CONNECTION);
    if (conn != NULL) {
      strncpy(name, conn->name, name_size);
      name[name_size - 1] = '\0';
      ret = 0;
    }
  }
  return ret;
}

/***************************************************************************//**
 * @brief
 *    This function disconnect the connected BLE device 
 * @param[in]   None
 * @param[out]  None
 * @return      None
 ******************************************************************************/
void bluetooth_app_disconnect_master(void) 
{
  ble_conn_t *conn;
  conn = get_ble_conn(BLE_MASTER_CONNECTION);

  if (conn->inUse) {
      sl_bt_connection_close(conn->handle);
  }
}
