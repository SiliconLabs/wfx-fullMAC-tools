/***************************************************************************//**
 * @file
 * @brief Bluetooth advertisement
 *******************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement.
 * The software is governed by the sections of the MSLA applicable to Micrium
 * Software.
 *
 ******************************************************************************/

#include <stdio.h>

/* Bluetooth stack headers */
#include "bg_types.h"
#include "native_gecko.h"
#include "gatt_db.h"

#include "bluetooth_app.h"
#include "bluetooth_advertisement.h"

static responseData_t responseData = {
  2,  /* length (incl type) */
  0x01, /* type */
  0x04 | 0x02, /* Flags: LE General Discoverable Mode, BR/EDR is disabled. */
  DEVNAME_LEN + 1,        // length of local name (incl type)
  0x08,               // shortened local name
  { 'D', 'M', '0', '0', ':', '0', '0' },
  UUID_LEN + 1,           // length of UUID data (incl type)
  0x06,               // incomplete list of service UUID's
  // custom service UUID for silabs light in little-endian format
  { 0x13, 0x87, 0x37, 0x25, 0x42, 0xb0, 0xc3, 0xbf, 0x78, 0x40, 0x83, 0xb5, 0xe4, 0x96, 0xf5, 0x63 }
};

// iBeacon structure and data
static iBeaconData_t iBeaconData =
{
  /* Flag bits - See Bluetooth 4.0 Core Specification , Volume 3, Appendix C, 18.1 for more details on flags. */
  2,  /* length  */
  0x01, /* type */
  0x04 | 0x02, /* Flags: LE General Discoverable Mode, BR/EDR is disabled. */

  /* Manufacturer specific data */
  26,  /* length of field*/
  0xFF, /* type of field */

  /* The first two data octets shall contain a company identifier code from
   * the Assigned Numbers - Company Identifiers document */
  { UINT16_TO_BYTES(0x004C) },

  /* Beacon type */
  /* 0x0215 is iBeacon */
  { UINT16_TO_BYTE1(0x0215), UINT16_TO_BYTE0(0x0215) },

  /* 128 bit / 16 byte UUID - generated specially for the DMP Demo */
  { 0x00, 0x47, 0xe7, 0x0a, 0x5d, 0xc1, 0x47, 0x25, 0x87, 0x99, 0x83, 0x05, 0x44, 0xae, 0x04, 0xf6 },

  /* Beacon major number */
  { UINT16_TO_BYTE1(IBEACON_MAJOR_NUM), UINT16_TO_BYTE0(IBEACON_MAJOR_NUM) },

  /* Beacon minor number  - not used for this application*/
  { UINT16_TO_BYTE1(0), UINT16_TO_BYTE0(0) },

  /* The Beacon's measured RSSI at 1 meter distance in dBm */
  /* 0xC3 is -61dBm */
  // TBD: check?
  0xC3
};

static eddystoneData_t eddystoneData = {
  /* Flag bits - See Bluetooth 4.0 Core Specification , Volume 3, Appendix C, 18.1 for more details on flags. */
  2,  /* length  */
  0x01, /* type */
  0x04 | 0x02, /* Flags: LE General Discoverable Mode, BR/EDR is disabled. */
  /* Service field length */
  0x03,
  /* Service field type */
  0x03,
  /* 16-bit Eddystone UUID */
  { UINT16_TO_BYTES(0xFEAA) },
  /* Eddystone-TLM Frame length */
  0x10,
  /* Service Data data type value */
  0x16,
  /* 16-bit Eddystone UUID */
  { UINT16_TO_BYTES(0xFEAA) },
  /* Eddystone-URL Frame type */
  0x10,
  /* Tx power */
  0x00,
  /* URL prefix - standard */
  0x00,
  /* URL */
  { 's', 'i', 'l', 'a', 'b', 's', '.', 'c', 'o', 'm' }
};

static int startAdvertisement(uint8_t adv_handle,
                              uint8_t data_size,
                              uint8_t *data,
                              uint8_t connect_mode,
                              bool enable_scan_report)
{
  uint16_t ret;

  gecko_cmd_le_gap_bt5_set_adv_data(adv_handle, 0, data_size, data);
  gecko_cmd_le_gap_set_advertise_timing(adv_handle, 160, 160, 0, 0);
  gecko_cmd_le_gap_set_advertise_report_scan_request(adv_handle, enable_scan_report);
  gecko_cmd_le_gap_set_advertise_configuration(adv_handle, 1);
  ret = *(uint16_t *)gecko_cmd_le_gap_start_advertising(adv_handle, le_gap_user_data, connect_mode);

  return ret;
}

int enableBleAdvertisements(char *device_name, uint8_t nb_adv_sets, bool connectable)
{
  uint16_t ret;
  uint8_t connect_mode = le_gap_connectable_scannable;

  if (!connectable) {
    connect_mode = le_gap_scannable_non_connectable;
  }

  // set transmit power to 0 dBm.
  gecko_cmd_system_set_tx_power(0);

  // Copy to the local GATT database - this will be used by the BLE stack
  // to put the local device name into the advertisements, but only if we are
  // using default advertisements
  gecko_cmd_gatt_server_write_attribute_value(gattdb_device_name,
                                              0,
                                              strlen(device_name),
                                              (uint8_t *)device_name);

  // Copy the shortened device name to the response data, overwriting
  // the default device name which is set at compile time
  strncpy((char *)responseData.shortName, device_name, DEVNAME_LEN);

  ret = startAdvertisement(0,
                           sizeof(responseData),
                           (uint8_t*)&responseData,
                           connect_mode,
                           true);

  if ((ret == 0) && (nb_adv_sets > 1)) {
    ret = startAdvertisement(1,
                             sizeof(iBeaconData),
                             (uint8_t*)&iBeaconData,
                             le_gap_scannable_non_connectable,
                             false);

    if ((ret == 0) && (nb_adv_sets >= 3)) {
      ret = startAdvertisement(2,
                               sizeof(eddystoneData),
                               (uint8_t*)&eddystoneData,
                               le_gap_scannable_non_connectable,
                               false);
    }
  }

  return ret;
}

int disableBleAdvertisements(void)
{
  struct gecko_msg_le_gap_stop_advertising_rsp_t *resp;

  resp = gecko_cmd_le_gap_stop_advertising(0);
  gecko_cmd_le_gap_stop_advertising(1);
  gecko_cmd_le_gap_stop_advertising(2);

  return resp->result > 0;
}
