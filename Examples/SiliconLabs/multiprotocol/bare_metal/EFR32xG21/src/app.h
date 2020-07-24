/***************************************************************************//**
 * @brief app.h
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

#ifndef APP_H_
#define APP_H_

#include "gecko_configuration.h"

/* Set this value to 1 if you want to disable deep sleep completely */
#define DISABLE_SLEEP 1

#ifdef __cplusplus
extern "C" {
#endif

void appMain(gecko_configuration_t *pconfig);

#ifdef __cplusplus
}
#endif

#endif
