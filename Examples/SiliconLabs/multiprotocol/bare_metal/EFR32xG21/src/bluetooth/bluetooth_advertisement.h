/***************************************************************************//**
 * @brief bluetooth_beacon.h
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

#ifndef BLUETOOTH_ADVERTISEMENT_H_
#define BLUETOOTH_ADVERTISEMENT_H_

#define DEVNAME_LEN 8        /**< Device name length (incl term null). */
#define UUID_LEN 16          /**< 128-bit UUID. */

#define IBEACON_MAJOR_NUM 0x0200 /**< 16-bit major number. */

// We put the device name into a scan response packet,
// since it isn't included in the 'standard' beacons -
// I've included the flags, since certain apps seem to expect them
typedef struct {
  uint8_t flagsLen;          /**< Length of the Flags field. */
  uint8_t flagsType;         /**< Type of the Flags field. */
  uint8_t flags;             /**< Flags field. */
  uint8_t shortNameLen;      /**< Length of Shortened Local Name. */
  uint8_t shortNameType;     /**< Shortened Local Name. */
  uint8_t shortName[DEVNAME_LEN]; /**< Shortened Local Name. */
  uint8_t uuidLength;        /**< Length of UUID. */
  uint8_t uuidType;          /**< Type of UUID. */
  uint8_t uuid[UUID_LEN];    /**< 128-bit UUID. */
} responseData_t;

typedef struct {
  uint8_t flagsLen;          /* Length of the Flags field. */
  uint8_t flagsType;         /* Type of the Flags field. */
  uint8_t flags;             /* Flags field. */
  uint8_t mandataLen;        /* Length of the Manufacturer Data field. */
  uint8_t mandataType;       /* Type of the Manufacturer Data field. */
  uint8_t compId[2];         /* Company ID field. */
  uint8_t beacType[2];       /* Beacon Type field. */
  uint8_t uuid[16];          /* 128-bit Universally Unique Identifier (UUID). The UUID is an identifier for the company using the beacon*/
  uint8_t majNum[2];         /* Beacon major number. Used to group related beacons. */
  uint8_t minNum[2];         /* Beacon minor number. Used to specify individual beacons within a group.*/
  uint8_t txPower;           /* The Beacon's measured RSSI at 1 meter distance in dBm. See the iBeacon specification for measurement guidelines. */
} iBeaconData_t;

typedef struct {
  uint8_t flagsLen;          /**< Length of the Flags field. */
  uint8_t flagsType;         /**< Type of the Flags field. */
  uint8_t flags;             /**< Flags field. */
  uint8_t serLen;            /**< Length of Complete list of 16-bit Service UUIDs. */
  uint8_t serType;           /**< Complete list of 16-bit Service UUIDs. */
  uint8_t serviceList[2];    /**< Complete list of 16-bit Service UUIDs. */
  uint8_t serDataLength;     /**< Length of Service Data. */
  uint8_t serDataType;       /**< Type of Service Data. */
  uint8_t uuid[2];           /**< 16-bit Eddystone UUID. */
  uint8_t frameType;         /**< Frame type. */
  uint8_t txPower;           /**< The Beacon's measured RSSI at 0 meter distance in dBm. */
  uint8_t urlPrefix;         /**< URL prefix type. */
  uint8_t url[10];           /**< URL. */
} eddystoneData_t;

#ifdef __cplusplus
extern "C" {
#endif

int enableBleAdvertisements(char *device_name, uint8_t nb_adv_sets, bool connectable);
int disableBleAdvertisements(void);

#ifdef __cplusplus
}
#endif

#endif
