/**************************************************************************//**
 * Copyright 2018, Silicon Laboratories Inc.
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

#ifndef SL_WFX_HOST_API_H
#define SL_WFX_HOST_API_H

#include "sl_wfx.h"
#include "wfm_api.h"

/******************************************************
*                   Enumerations
******************************************************/

typedef enum {
  SL_WFX_TX_FRAME_BUFFER,
  SL_WFX_RX_FRAME_BUFFER,
  SL_WFX_CONTROL_BUFFER,
  SL_WFX_SCAN_RESULT_BUFFER,
} sl_wfx_buffer_type_t;

typedef enum {
  SL_WFX_BUS_WRITE = (1 << 0),
  SL_WFX_BUS_READ  = (1 << 1),
  SL_WFX_BUS_WRITE_AND_READ = SL_WFX_BUS_WRITE | SL_WFX_BUS_READ,
} sl_wfx_host_bus_tranfer_type_t;

/******************************************************
*                    Variables
******************************************************/

extern sl_wfx_context_t *sl_wfx_context;

/******************************************************
*               Function Declarations
******************************************************/
/* Initialization phase*/
sl_status_t sl_wfx_host_init(void);
sl_status_t sl_wfx_host_get_firmware_data(const uint8_t **data, uint32_t data_size);
sl_status_t sl_wfx_host_get_firmware_size(uint32_t *firmware_size);
sl_status_t sl_wfx_host_get_pds_data(const char **pds_data, uint16_t index);
sl_status_t sl_wfx_host_get_pds_size(uint16_t *pds_size);
sl_status_t sl_wfx_host_deinit(void);
/* GPIO interface */
sl_status_t sl_wfx_host_reset_chip(void);
sl_status_t sl_wfx_host_set_wake_up_pin(uint8_t state);
sl_status_t sl_wfx_host_wait_for_wake_up(void);
sl_status_t sl_wfx_host_hold_in_reset(void);
/* Event management */
sl_status_t sl_wfx_host_wait_for_confirmation(uint32_t timeout_ms, void **event_payload_out);
sl_status_t sl_wfx_host_wait(uint32_t wait_ms);
sl_status_t sl_wfx_host_post_event(sl_wfx_frame_type_t frame_type,
                                   sl_wfx_generic_message_t *event_payload);
/* Memory management */
sl_status_t sl_wfx_host_allocate_buffer(void **buffer,
                                        sl_wfx_buffer_type_t type,
                                        uint32_t buffer_size,
                                        uint32_t wait_duration_ms);
sl_status_t sl_wfx_host_free_buffer(void *buffer, sl_wfx_buffer_type_t type);
/* Frame hook */
sl_status_t sl_wfx_host_transmit_frame(void *frame, uint32_t frame_len);
/* WF200 host bus API */
sl_status_t sl_wfx_host_init_bus(void);
sl_status_t sl_wfx_host_deinit_bus(void);
sl_status_t sl_wfx_host_enable_platform_interrupt(void);
sl_status_t sl_wfx_host_disable_platform_interrupt(void);
/* WF200 host SPI bus API */
sl_status_t sl_wfx_host_spi_cs_assert(void);
sl_status_t sl_wfx_host_spi_cs_deassert(void);
sl_status_t sl_wfx_host_spi_transfer_no_cs_assert(sl_wfx_host_bus_tranfer_type_t type,
                                                  uint8_t *header,
                                                  uint16_t header_length,
                                                  uint8_t *buffer,
                                                  uint16_t buffer_length);
/* WF200 host SDIO bus API */
sl_status_t sl_wfx_host_sdio_transfer_cmd52(sl_wfx_host_bus_tranfer_type_t type,
                                            uint8_t function,
                                            uint32_t address,
                                            uint8_t *buffer);
sl_status_t sl_wfx_host_sdio_transfer_cmd53(sl_wfx_host_bus_tranfer_type_t type,
                                            uint8_t function,
                                            uint32_t address,
                                            uint8_t *buffer,
                                            uint16_t buffer_length);
sl_status_t sl_wfx_host_sdio_enable_high_speed_mode(void);

#endif // SL_WFX_HOST_API_H
