/***************************************************************************//**
 * @file
 * @brief User Interface core logic for demo.
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

#include <LCD/mp-bitmaps.h>
#include "interface.h"

#if !(BRD4187X || BRD4180_81_X) || defined(SL_CATALOG_DMD_MEMLCD_PRESENT)

#include "em_types.h"
#include "glib.h"
#include "dmd/dmd.h"
#include <string.h>
#include <stdio.h>
#include "mp-ui.h"

#define PROT_ZIGBEE     (1)
#define PROT_RAIL       (2)
#define PROT_CONNECT    (3)
#define PROT_BLUETOOTH  (4)
#define PROT_THREAD     (5)

#define PROT1           (PROT_WIFI)

#define PROT2           (PROT_BLUETOOTH)

#ifndef DEVICE_TYPE
#define DEVICE_TYPE "Multiprotocol"
#endif

#define helpmenu_line1_light  "      **HELP**       "
#define helpmenu_line2_light  " PB0 - Toggle Light  "
#define helpmenu_line3_light  " PB1 - NWK Control   "
#define helpmenu_line4_light  " No NWK : Form NWK   "
#define helpmenu_line5_light  " NWK    : Permit join"
#define helpmenu_line6_light  " Press>5s: Leave NWK "

#define helpmenu_line1_switch "      **HELP**       "
#define helpmenu_line2_switch " PB0 - Toggle Light  "
#define helpmenu_line3_switch " PB1 - NWK Control   "
#define helpmenu_line4_switch " No NWK:Join NWK     "
#define helpmenu_line5_switch " Press>5s: Leave NWK "
#define helpmenu_line6_switch "                     "

#define helpmenu_line1_no_nwk "      **HELP**       "
#define helpmenu_line2_no_nwk " PB0 - Toggle Light  "
#define helpmenu_line3_no_nwk " PB1 - Toggle Light  "
#define helpmenu_line4_no_nwk "                     "
#define helpmenu_line5_no_nwk "                     "
#define helpmenu_line6_no_nwk "                     "

#define SILICONLABS_X_POSITION          ((glibContext.pDisplayGeometry->xSize - SILICONLABS_BITMAP_WIDTH) / 2)
#define SILICONLABS_Y_POSITION          0
#define LIGHT_BITMAP_WIDTH              64
#define LIGHT_BITMAP_HEIGHT             64
#define LIGHT_X_POSITION                ((glibContext.pDisplayGeometry->xSize - LIGHT_BITMAP_WIDTH) / 2)
#define LIGHT_Y_POSITION                (glibContext.pDisplayGeometry->ySize - LIGHT_BITMAP_HEIGHT - 5)
#define PROT1_ID_X_POSITION             1
#define PROT2_ID_X_POSITION             79


#define PROT1_BITMAP_WIDTH          WIFI_BITMAP_WIDTH
#define PROT1_BITMAP_HEIGHT         WIFI_BITMAP_HEIGHT
#define PROT1_X_POSITION            8
#define PROT1_Y_POSITION            (LIGHT_Y_POSITION + (LIGHT_Y_POSITION / 2)) + 2
#define PROT1_BITMAP                (wifiBitmap)
#define PROT1_BITMAP_CONN           (wifiConnectedBitmap)

#define PROT2_BITMAP_WIDTH          BLUETOOTH_BITMAP_WIDTH
#define PROT2_BITMAP_HEIGHT         BLUETOOTH_BITMAP_HEIGHT
#define PROT2_X_POSITION            104
#define PROT2_Y_POSITION            (LIGHT_Y_POSITION + (LIGHT_Y_POSITION / 2))
#define PROT2_BITMAP                (bluetoothBitmap)
#define PROT2_BITMAP_CONN           (bluetoothConnectedBitmap)


/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/
static GLIB_Context_t glibContext;          /* Global glib context */

static const uint8_t siliconlabsBitmap[] = { SILABS_BITMAP };
static const uint8_t lightOnBitMap[] = { LIGHT_ON_BITMAP };
static const uint8_t lightOffBitMap[] = { LIGHT_OFF_BITMAP };

static const uint8_t wifiBitmap[] = { WIFI_BITMAP };
static const uint8_t wifiConnectedBitmap[] = { WIFI_CONNECTED_BITMAP };

static const uint8_t bluetoothBitmap[] = { BLUETOOTH_BITMAP };
static const uint8_t bluetoothConnectedBitmap[] = { BLUETOOTH_CONNECTED_BITMAP };

/*******************************************************************************
 **************************   LOCAL FUNCTIONS   ********************************
 ******************************************************************************/ \
static void mpUIDisplayLogo(void)
{
  GLIB_drawBitmap(&glibContext,
                  SILICONLABS_X_POSITION,
                  SILICONLABS_Y_POSITION,
                  SILICONLABS_BITMAP_WIDTH,
                  SILICONLABS_BITMAP_HEIGHT,
                  siliconlabsBitmap);
}

static void mpUIDisplayAppName(uint8_t* device)
{
  char appName[20];
  sprintf(appName, "Demo %s", (const char*)device);

  GLIB_drawStringOnLine(&glibContext, appName, 5, GLIB_ALIGN_CENTER, 0, 0, true);
}

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/
void mpUIInit(void)
{
  EMSTATUS status;

  /* Initialize the DMD module for the DISPLAY device driver. */
  status = DMD_init(0);
  if (DMD_OK != status) {
    while (1)
      ;
  }

  status = GLIB_contextInit(&glibContext);
  if (GLIB_OK != status) {
    while (1)
      ;
  }

  glibContext.backgroundColor = White;
  glibContext.foregroundColor = Black;

  /* Use Narrow font */
  GLIB_setFont(&glibContext, (GLIB_Font_t *)&GLIB_FontNarrow6x8);
  GLIB_clear(&glibContext);
  DMD_updateDisplay();
}

void mpUIDisplayHeader(uint8_t* name)
{
  mpUIDisplayLogo();
  mpUIDisplayAppName(name);
  DMD_updateDisplay();
}

void mpUIDisplayHelp(bool networkForming)
{
  GLIB_clear(&glibContext);
  uint8_t y_position = SILICONLABS_BITMAP_HEIGHT + 20;
  if (!networkForming) {
    GLIB_drawString(&glibContext, helpmenu_line1_no_nwk,
                    strlen(helpmenu_line1_no_nwk) + 1, 2, y_position, 0);
    GLIB_drawString(&glibContext, helpmenu_line2_no_nwk,
                    strlen(helpmenu_line2_no_nwk) + 1, 2, y_position + 10, 0);
    GLIB_drawString(&glibContext, helpmenu_line3_no_nwk,
                    strlen(helpmenu_line3_no_nwk) + 1, 2, y_position + 20, 0);
    GLIB_drawString(&glibContext, helpmenu_line4_no_nwk,
                    strlen(helpmenu_line4_no_nwk) + 1, 2, y_position + 30, 0);
    GLIB_drawString(&glibContext, helpmenu_line5_no_nwk,
                    strlen(helpmenu_line5_no_nwk) + 1, 2, y_position + 40, 0);
    GLIB_drawString(&glibContext, helpmenu_line6_no_nwk,
                    strlen(helpmenu_line6_no_nwk) + 1, 2, y_position + 50, 0);
  } else if (!strcmp(DEVICE_TYPE, "Light")) {
    GLIB_drawString(&glibContext, helpmenu_line1_light,
                    strlen(helpmenu_line1_light) + 1, 2, y_position, 0);
    GLIB_drawString(&glibContext, helpmenu_line2_light,
                    strlen(helpmenu_line2_light) + 1, 2, y_position + 10, 0);
    GLIB_drawString(&glibContext, helpmenu_line3_light,
                    strlen(helpmenu_line3_light) + 1, 2, y_position + 20, 0);
    GLIB_drawString(&glibContext, helpmenu_line4_light,
                    strlen(helpmenu_line4_light) + 1, 2, y_position + 30, 0);
    GLIB_drawString(&glibContext, helpmenu_line5_light,
                    strlen(helpmenu_line5_light) + 1, 2, y_position + 40, 0);
    GLIB_drawString(&glibContext, helpmenu_line6_light,
                    strlen(helpmenu_line6_light) + 1, 2, y_position + 50, 0);
  } else {
    GLIB_drawString(&glibContext, helpmenu_line1_switch,
                    strlen(helpmenu_line1_switch) + 1, 2, y_position, 0);
    GLIB_drawString(&glibContext, helpmenu_line2_switch,
                    strlen(helpmenu_line2_switch) + 1, 2, y_position + 10, 0);
    GLIB_drawString(&glibContext, helpmenu_line3_switch,
                    strlen(helpmenu_line3_switch) + 1, 2, y_position + 20, 0);
    GLIB_drawString(&glibContext, helpmenu_line4_switch,
                    strlen(helpmenu_line4_switch) + 1, 2, y_position + 30, 0);
    GLIB_drawString(&glibContext, helpmenu_line5_switch,
                    strlen(helpmenu_line5_switch) + 1, 2, y_position + 40, 0);
    GLIB_drawString(&glibContext, helpmenu_line6_switch,
                    strlen(helpmenu_line6_switch) + 1, 2, y_position + 50, 0);
  }
  DMD_updateDisplay();
}

void mpUIDisplayLight(bool on)
{
  GLIB_drawBitmap(&glibContext,
                  LIGHT_X_POSITION,
                  LIGHT_Y_POSITION,
                  LIGHT_BITMAP_WIDTH,
                  LIGHT_BITMAP_HEIGHT,
                  (on ? lightOnBitMap : lightOffBitMap));
  DMD_updateDisplay();
}

void mpUIDisplayProtocol(mpUIProtocol protocol, bool isConnected)
{
  GLIB_drawBitmap(&glibContext,
                  (protocol == MP_UI_PROTOCOL1 ? PROT1_X_POSITION : PROT2_X_POSITION),
                  (protocol == MP_UI_PROTOCOL1 ? PROT1_Y_POSITION : PROT2_Y_POSITION),
                  (protocol == MP_UI_PROTOCOL1 ? PROT1_BITMAP_WIDTH : PROT2_BITMAP_WIDTH),
                  (protocol == MP_UI_PROTOCOL1 ? PROT1_BITMAP_HEIGHT : PROT2_BITMAP_HEIGHT),
                  (protocol == MP_UI_PROTOCOL1 ? (isConnected ? PROT1_BITMAP_CONN : PROT1_BITMAP)
                   : (isConnected ? PROT2_BITMAP_CONN : PROT2_BITMAP)));
  DMD_updateDisplay();
}

void mpUIDisplayDirection(mpUILightDirection_t direction)
{
  if (direction == MP_UI_DIRECTION_PROT1) {
    GLIB_drawLine(&glibContext,
                  (PROT1_X_POSITION + PROT1_BITMAP_WIDTH + 5),
                  (PROT1_Y_POSITION + PROT1_BITMAP_HEIGHT / 2),
                  (PROT1_X_POSITION + PROT1_BITMAP_WIDTH + 15),
                  (PROT1_Y_POSITION + PROT1_BITMAP_HEIGHT / 2));
    GLIB_drawLine(&glibContext,
                  (PROT1_X_POSITION + PROT1_BITMAP_WIDTH + 12),
                  (PROT1_Y_POSITION + (PROT1_BITMAP_HEIGHT / 2) - 3),
                  (PROT1_X_POSITION + PROT1_BITMAP_WIDTH + 15),
                  (PROT1_Y_POSITION + PROT1_BITMAP_HEIGHT / 2));
    GLIB_drawLine(&glibContext,
                  (PROT1_X_POSITION + PROT1_BITMAP_WIDTH + 12),
                  (PROT1_Y_POSITION + (PROT1_BITMAP_HEIGHT / 2) + 3),
                  (PROT1_X_POSITION + PROT1_BITMAP_WIDTH + 15),
                  (PROT1_Y_POSITION + (PROT1_BITMAP_HEIGHT / 2)));
  } else if (direction == MP_UI_DIRECTION_PROT2) {
    GLIB_drawLine(&glibContext,
                  (PROT2_X_POSITION - 5),
                  (PROT2_Y_POSITION + PROT2_BITMAP_HEIGHT / 2),
                  (PROT2_X_POSITION - 15),
                  (PROT2_Y_POSITION + PROT2_BITMAP_HEIGHT / 2));
    GLIB_drawLine(&glibContext,
                  ((PROT2_X_POSITION - 15) + 3),
                  ((PROT2_Y_POSITION + PROT2_BITMAP_HEIGHT / 2) - 3),
                  (PROT2_X_POSITION - 15),
                  (PROT2_Y_POSITION + PROT2_BITMAP_HEIGHT / 2));
    GLIB_drawLine(&glibContext,
                  ((PROT2_X_POSITION - 15) + 3),
                  ((PROT2_Y_POSITION + PROT2_BITMAP_HEIGHT / 2) + 3),
                  (PROT2_X_POSITION - 15),
                  (PROT2_Y_POSITION + PROT2_BITMAP_HEIGHT / 2));
  }
  DMD_updateDisplay();
}

void mpUIClearDirection(mpUILightDirection_t direction)
{
  GLIB_Rectangle_t rect;
  if (direction == MP_UI_DIRECTION_PROT1) {
    rect.xMin = (PROT1_X_POSITION + PROT1_BITMAP_WIDTH + 5);
    rect.yMin = (PROT1_Y_POSITION + (PROT1_BITMAP_HEIGHT / 2) - 3);
    rect.xMax = (PROT1_X_POSITION + PROT1_BITMAP_WIDTH + 15);
    rect.yMax = (PROT1_Y_POSITION + (PROT1_BITMAP_HEIGHT / 2) + 3);
  } else if (direction == MP_UI_DIRECTION_PROT2) {
    rect.xMin = (PROT2_X_POSITION - 15);
    rect.yMin = ((PROT2_Y_POSITION + PROT2_BITMAP_HEIGHT / 2) - 3);
    rect.xMax = (PROT2_X_POSITION - 5);
    rect.yMax = ((PROT2_Y_POSITION + PROT2_BITMAP_HEIGHT / 2) + 3);
  }
  GLIB_setClippingRegion(&glibContext, (const GLIB_Rectangle_t*)&rect);
  GLIB_clearRegion(&glibContext);
  GLIB_resetClippingRegion(&glibContext);
  GLIB_applyClippingRegion(&glibContext);
  DMD_updateDisplay();
}

void mpUIDisplayId(mpUIProtocol protocol, uint8_t* id)
{
  char tmpId[10] = { 0 };
  strncpy(tmpId, (char*)id, 9);
  if (strlen(tmpId)) {
    GLIB_drawString(&glibContext,
                    tmpId,
                    strlen(tmpId) + 1,
                    (protocol == MP_UI_PROTOCOL1
                     ? PROT1_ID_X_POSITION : PROT2_ID_X_POSITION),
                    glibContext.pDisplayGeometry->ySize - 10,
                    0);
  }
  DMD_updateDisplay();
}

void mpUIDisplayChan(uint8_t channel)
{
  uint8_t msg[9];
  sprintf((char *)msg, "CHAN:%d", channel);
  mpUIDisplayId(MP_UI_PROTOCOL1, msg);
}

void mpUIClearMainScreen(uint8_t* name, bool showPROT1, bool showPROT2)
{
  GLIB_clear(&glibContext);
  mpUIDisplayHeader(name);
  mpUIDisplayLight(false);
  if (showPROT1) {
    mpUIDisplayProtocol(MP_UI_PROTOCOL1, false);
    mpUIClearDirection(MP_UI_DIRECTION_PROT1);
  }

  if (showPROT2) {
    mpUIDisplayProtocol(MP_UI_PROTOCOL2, false);
    mpUIClearDirection(MP_UI_DIRECTION_PROT2);
  }
}

#endif /* BRD4187X */
