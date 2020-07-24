/****************************************************************************/
/**
 * @file mp-ui.h
 * @brief UI interface for Multiprotocol demo
 * @version 0.0.1
 ******************************************************************************
 * # License
 * <b>Copyright 2015 Silicon Labs, www.silabs.com</b>
 *******************************************************************************
 *
 * This file is licensensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 *
 ******************************************************************************/

#ifndef MP_UI_H
#define MP_UI_H

/******************************************************************************/
/**
 * @addtogroup UI Interface for Multiprotocol Demo
 * @{
 *
 * MP UI uses the underlying DMD interface and the GLIB and exposes several
 * wrapper functions to application. These functions are used to display
 * different bitmaps for the demo.
 *
 ******************************************************************************/

#define PROT_ZIGBEE     (1)
#define PROT_RAIL       (2)
#define PROT_CONNECT    (3)
#define PROT_BLUETOOTH  (4)
#define PROT_THREAD     (5)
#define PROT_WIFI       (6)

#define PROT1           (PROT_WIFI)
#define PROT2           (PROT_BLUETOOTH)

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

typedef enum {
  MP_UI_PROTOCOL1,
  MP_UI_PROTOCOL2
} MpUiProtocol;

typedef enum {
  MP_UI_LIGHT_OFF,
  MP_UI_LIGHT_ON
} MpUiLightState_t;

typedef enum {
  MP_UI_DIRECTION_PROT1,
  MP_UI_DIRECTION_PROT2,
  MP_UI_DIRECTION_SWITCH,
  MP_UI_DIRECTION_INVALID
} MpUiLightDirection_t;

/*******************************************************************************
 ******************************   PROTOTYPES   *********************************
 ******************************************************************************/

/**
 * @brief
 *   Initilize the GLIB and DMD interfaces.
 *
 * @param[in] void
 *
 * @return
 *      void
 */

void mpUiInit(void);

/**
 * @brief
 *   Update the display with Silicon Labs logo and application name.
 *
 * @param[in] void
 *
 * @return
 *      void
 */
void mpUiDisplayHeader(void);

/**
 * @brief
 *   Update the display with light bulb image.
 *
 * @param[in] on status of light bulb
 *
 * @return
 *      void
 */
void mpUiDisplayLight(bool on);

/**
 * @brief
 *   Update the display to show if the mobile device is connected via a protocol.
 *
 * @param[in] bool, true if the Light is connected to mobile device, false otherwise.
 *
 * @return
 *      void
 */
void mpUiDisplayProtocol(MpUiProtocol protocol, bool isConnected);

/**
 * @brief
 *   Update the display to show which interface toggled the light.
 *
 * @param[in] source of the Light toggling
 *
 * @return
 *      void
 */
void mpUiDisplayDirection(MpUiLightDirection_t direction);

/**
 * @brief
 *   Update the display to clear signs used to indicate light toggle source.
 *
 * @param[in] source to clear
 *
 * @return
 *      void
 */
void mpUiClearDirection(MpUiLightDirection_t direction);

/**
 * @brief
 *   Update the display to show the device id.
 *
 * @param[in] protocol - MP_UI_PROTOCOL1 or MP_UI_PROTOCOL2 (left or right)
 * @param[in] id - id to show, max 9 byte long string.
 *
 * @return
 *      void
 */
void mpUiDisplayId(MpUiProtocol protocol, uint8_t* id);

/**
 * @brief
 *   Clear the Lcd screen and display the main screen.
 *
 * @param[in] showPROT1 - show protocol 1 related icon.
 * @param[in] showPROT2 - show protocol 2 related icon.
 *
 * @return
 *      void
 */
void mpUiClearMainScreen(bool showPROT1, bool showPROT2);

#endif //MP_UI_H
