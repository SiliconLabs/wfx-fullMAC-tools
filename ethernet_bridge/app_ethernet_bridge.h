/***************************************************************************//**
 * @file  app_ethernet_bridge.h
 * @brief Ethernet bridge application.
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
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
#ifndef APP_ETHERNET_BRIDGE_H
#define APP_ETHERNET_BRIDGE_H

#include <bsp_os.h>
#include <cpu/include/cpu.h>
#include <kernel/include/os.h>
#include <kernel/include/os_trace.h>
#include <common/include/common.h>
#include <common/include/lib_def.h>
#include <common/include/rtos_utils.h>
#include <common/include/toolchains.h>

#include <stdio.h>
#include <string.h>
#include <common/include/auth.h>
#include <common/include/shell.h>
#include <mbedtls/threading.h>

#include <net/include/net_if_ether.h>
#include <net/source/tcpip/net_if_priv.h>

#include "em_cmu.h"
#include "em_emu.h"
#include "em_chip.h"
#include "io.h"

#include "sl_wfx_task.h"
#include "sl_wfx_host.h"
#include "sl_wfx.h"

/*******************************************************************************
 *                        Wi-Fi SOFTAP CONFIGURATION                           *
 *******************************************************************************/
///< wifi ssid for soft ap mode
#define SOFTAP_SSID_DEFAULT     "silabs_bridge"
///< wifi password for soft ap mode
#define SOFTAP_PASSKEY_DEFAULT  "changeme"
///< wifi security for soft ap mode:
/// WFM_SECURITY_MODE_OPEN/WFM_SECURITY_MODE_WEP/WFM_SECURITY_MODE_WPA2_WPA1_PSK
#define SOFTAP_SECURITY_DEFAULT WFM_SECURITY_MODE_WPA2_PSK
///< wifi channel for soft ap
#define SOFTAP_CHANNEL_DEFAULT  6

/**************************************************************************//**
 * Ethernet Wi-Fi Bridge Application Initialization.
 *****************************************************************************/
void  app_ethernet_bridge_init(void);

/**************************************************************************//**
 * Start Soft-AP Task.
 *****************************************************************************/
void wifi_start_softap(void);

#endif // APP_ETHERNET_BRIDGE_H
