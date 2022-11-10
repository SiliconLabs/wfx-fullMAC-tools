/***************************************************************************//**
 * @file
 * @brief User Interface for mp.
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
#ifndef MP_UI_H
#define MP_UI_H

/**************************************************************************//**
* DEMO UI uses the underlying DMD interface and the GLIB and exposes several
* wrapper functions to application. These functions are used to display
* different bitmaps for the mp.
*
******************************************************************************/

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

typedef enum {
  MP_UI_PROTOCOL1,
  MP_UI_PROTOCOL2
} mpUIProtocol;

typedef enum {
  MP_UI_LIGHT_OFF,
  MP_UI_LIGHT_ON
} mpUILightState_t;

typedef enum {
  MP_UI_DIRECTION_PROT1,
  MP_UI_DIRECTION_PROT2,
  MP_UI_DIRECTION_SWITCH,
  MP_UI_DIRECTION_INVALID
} mpUILightDirection_t;

typedef enum {
  MP_UI_NO_NETWORK,
  MP_UI_SCANNING,
  MP_UI_JOINING,
  MP_UI_FORMING,
  MP_UI_NETWORK_UP,
  MP_UI_STATE_UNKNOWN
} mpUIZigBeeNetworkState_t;

/*******************************************************************************
 ******************************   PROTOTYPES   *********************************
 ******************************************************************************/

/**************************************************************************//**
 * @brief
 *   Initilize the GLIB and DMD interfaces.
 *
 * @param[in] void
 *
 * @return
 *      void
 *****************************************************************************/
void mpUIInit(void);

/**************************************************************************//**
 * @brief
 *   Update the display with Silicon Labs logo and application name.
 *
 * @param[in] name name of the current application.
 *
 * @return
 *      void
 *****************************************************************************/
void mpUIDisplayHeader(uint8_t* name);

/**************************************************************************//**
 * @brief
 *   Update the display with Help menu.
 *
 * @param[in] networkForming whether UI displays network forming related components.
 *
 * @return
 *      void
 *****************************************************************************/
void mpUIDisplayHelp(bool networkForming);

/**************************************************************************//**
 * @brief
 *   Update the display with light bulb image.
 *
 * @param[in] on status of light bulb
 *
 * @return
 *      void
 *****************************************************************************/
void mpUIDisplayLight(bool on);

/**************************************************************************//**
 * @brief
 *   Update the display to show if the bluetooth is connected to the mobile device.
 *
 * @param[in] bool, true if the Light is connected to mobile device, false otherwise.
 *
 * @return
 *      void
 *****************************************************************************/
void mpUIDisplayProtocol(mpUIProtocol protocol, bool isConnected);

/**************************************************************************//**
 * @brief
 *   Update the display to show which interface toggled the light.
 *
 * @param[in] DEMO_UI_DIRECTION_BLUETOOTH or DEMO_UI_DIRECTION_ZIGBEE
 *
 * @return
 *      void
 *****************************************************************************/
void mpUIDisplayDirection(mpUILightDirection_t direction);

/**************************************************************************//**
 * @brief
 *   Update the display to clear signs used to indicate light toggle source.
 *
 * @param[in] DEMO_UI_DIRECTION_BLUETOOTH or DEMO_UI_DIRECTION_ZIGBEE
 *
 * @return
 *      void
 *****************************************************************************/
void mpUIClearDirection(mpUILightDirection_t direction);

/**************************************************************************//**
 * @brief
 *   Update the display to show the device id.
 *
 * @param[in] protocol - DEMO_UI_PROTOCOL1 or DEMO_UI_PROTOCOL2 (left or right)
 * @param[in] id - id to show, max 9 byte long string.
 *
 * @return
 *      void
 *****************************************************************************/
void mpUIDisplayId(mpUIProtocol protocol, uint8_t* id);

/**************************************************************************//**
 * @brief
 *   Clear the Lcd screen and display the main screen.
 *
 * @param[in] name - application name
 * @param[in] showPROT1 - show protocol 1 related icon.
 * @param[in] showPROT2 - show protocol 2 related icon.
 *
 * @return
 *      void
 *****************************************************************************/
void mpUIClearMainScreen(uint8_t* name, bool showPROT1, bool showPROT2);

/**************************************************************************//**
 * @brief
 *   Update the current operating channel.
 *
 * @param[in] channel
 *
 * @return
 *      void
 *****************************************************************************/
void mpUIDisplayChan(uint8_t channel);

#endif //MP_UI_H
