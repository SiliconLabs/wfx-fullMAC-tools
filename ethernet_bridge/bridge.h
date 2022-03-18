/***************************************************************************//**
 * @file  bridge.h
 * @brief LwIP task and related functions header file
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
#ifndef BRIDGE_H
#define BRIDGE_H

#if BRIDGE_DEBUG
#include  <stdio.h>
#define  LOG_TRACE(...)                      printf(__VA_ARGS__)
#else 
#define LOG_TRACE(...)                      (void)0
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "sl_wfx_cmd_api.h"
#include "sl_status.h"

/**************************************************************************//**
 * sl_wfx_host_received_frame_callback()
 * @brief: This function is called by FMAC driver to forward the received frames
 * to ethernet controller
 *****************************************************************************/
void sl_wfx_host_received_frame_callback(sl_wfx_received_ind_t* rx_buffer);

/**************************************************************************//**
 * low_level_output_ethernet()
 * @brief: This function is called in the ethernet driver to forward the frames
 * to FMAC driver
 *****************************************************************************/
sl_status_t low_level_output_ethernet(uint8_t *data, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif //BRIDGE_H
