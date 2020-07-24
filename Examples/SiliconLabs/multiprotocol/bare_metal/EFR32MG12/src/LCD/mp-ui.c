/*****************************************************************************/
/**
 * @brief User Interface rendering for DMP demo
 * @version 0.0.1
 ******************************************************************************
 * @section License
 * <b>(C) Copyright 2015 Silicon Labs, www.silabs.com</b>
 *******************************************************************************
 *
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 *
 ******************************************************************************/

#include "em_types.h"
#include "glib.h"
#include "dmd/dmd.h"
#include "display.h"
#include <string.h>
#include <stdio.h>
#include "mp-ui.h"
#include "mp-bitmaps.h"

static GLIB_Context_t glibContext;          /* Global glib context */

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/

/*******************************************************************************
 **************************   LOCAL FUNCTIONS   ********************************
 ******************************************************************************/ \
  static void mpUiDisplayLogo(void)
{
  GLIB_drawBitmap(&glibContext,
                  SILICONLABS_X_POSITION,
                  SILICONLABS_Y_POSITION,
                  SILICONLABS_BITMAP_WIDTH,
                  SILICONLABS_BITMAP_HEIGHT,
                  siliconlabsBitmap);
}

static void mpUiDisplayAppName(void)
{
  char appName[20] = "Multiprotocol Demo";

  GLIB_drawString(&glibContext, appName,
                  strlen(appName) + 1, 10, SILICONLABS_BITMAP_HEIGHT + 2, 0);
}

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/
void mpUiInit(void)
{
  EMSTATUS status;

  /* Initialize the display module. */
  status = DISPLAY_Init();
  if (DISPLAY_EMSTATUS_OK != status) {
    while (1)
      ;
  }

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

void mpUiDisplayHeader(void)
{
  mpUiDisplayLogo();
  mpUiDisplayAppName();
  DMD_updateDisplay();
}

void mpUiDisplayLight(bool on)
{
  GLIB_drawBitmap(&glibContext,
                  LIGHT_X_POSITION,
                  LIGHT_Y_POSITION,
                  LIGHT_BITMAP_WIDTH,
                  LIGHT_BITMAP_HEIGHT,
                  (on ? lightOnBitMap : lightOffBitMap));
  DMD_updateDisplay();
}

void mpUiDisplayProtocol(MpUiProtocol protocol, bool isConnected)
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

void mpUiDisplayDirection(MpUiLightDirection_t direction)
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

void mpUiClearDirection(MpUiLightDirection_t direction)
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

void mpUiDisplayId(MpUiProtocol protocol, uint8_t* id)
{
  char tmpId[10] = { 0 };
  strncpy(tmpId, (char*)id, 9);
  if (strlen(tmpId)) {
    GLIB_drawString(&glibContext,
                    tmpId,
                    strlen(tmpId) + 1,
                    (protocol == MP_UI_PROTOCOL1
                     ? PROT1_ID_X_POSITION : 79),
                    glibContext.pDisplayGeometry->ySize - 10,
                    0);
  }
  DMD_updateDisplay();
}

void mpUiClearMainScreen(bool showPROT1, bool showPROT2)
{
  GLIB_clear(&glibContext);
  mpUiDisplayHeader();
  mpUiDisplayLight(false);
  if (showPROT1) {
    mpUiDisplayProtocol(MP_UI_PROTOCOL1, false);
    mpUiClearDirection(MP_UI_DIRECTION_PROT1);
  }

  if (showPROT2) {
    mpUiDisplayProtocol(MP_UI_PROTOCOL2, false);
    mpUiClearDirection(MP_UI_DIRECTION_PROT2);
  }
}
