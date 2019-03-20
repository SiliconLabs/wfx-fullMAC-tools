/*
* Copyright 2018, Silicon Laboratories Inc.  All rights reserved.
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
*/

/**
 * \file wf200_host_api.h
 * \brief wf200_host_api.h contains the definitions of the functions to be implemented by the host MCU.
 *
 */

#ifndef __WF200_HOST_API_H
#define __WF200_HOST_API_H

#include "sl_status.h"
#include "wf200.h"
#include <stdint.h>

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    WF200_TX_FRAME_BUFFER,
    WF200_RX_FRAME_BUFFER,
    WF200_CONTROL_BUFFER,
    WF200_SCAN_RESULT_BUFFER,
} wf200_buffer_type_t;

typedef enum
{
    WF200_BUS_WRITE = (1 << 0),
    WF200_BUS_READ  = (1 << 1),
    WF200_BUS_WRITE_AND_READ = WF200_BUS_WRITE | WF200_BUS_READ,
} wf200_host_bus_tranfer_type_t;

/******************************************************
 *                    Variables
 ******************************************************/

extern wf200_context_t* wf200_context;

/******************************************************
 *               Function Declarations
 ******************************************************/
/* Initialization phase*/
sl_status_t wf200_host_init( void );
sl_status_t wf200_host_get_firmware_data( const uint8_t** data, uint32_t data_size );
sl_status_t wf200_host_get_firmware_size( uint32_t* firmware_size );
sl_status_t wf200_host_deinit( void );
/* GPIO interface */
sl_status_t wf200_host_reset_chip( void );
sl_status_t wf200_host_set_wake_up_pin( uint8_t state );
sl_status_t wf200_host_wait_for_wake_up( void );
sl_status_t wf200_host_hold_in_reset( void );
/* Event management */
sl_status_t wf200_host_wait_for_confirmation( uint32_t timeout, void** event_payload_out );
sl_status_t wf200_host_wait( uint32_t wait_time );
sl_status_t wf200_host_post_event( wf200_frame_type_t frame_type, uint32_t event_id, void* event_payload, uint32_t event_payload_length );
/* Memory management */
sl_status_t wf200_host_allocate_buffer( wf200_buffer_t** buffer, wf200_buffer_type_t type, uint32_t buffer_size, uint32_t wait_duration );
sl_status_t wf200_host_free_buffer( wf200_buffer_t* buffer, wf200_buffer_type_t type );
/* Frame hook */
sl_status_t wf200_host_transmit_frame( wf200_buffer_t* frame );
/* WF200 host bus API */
sl_status_t wf200_host_init_bus( void );
sl_status_t wf200_host_deinit_bus( void );
sl_status_t wf200_host_enable_platform_interrupt( void );
sl_status_t wf200_host_disable_platform_interrupt( void );
/* WF200 host SPI bus API */
sl_status_t wf200_host_spi_cs_assert( void );
sl_status_t wf200_host_spi_cs_deassert(void );
sl_status_t wf200_host_spi_transfer_no_cs_assert( wf200_host_bus_tranfer_type_t type, uint8_t* buffer, uint16_t buffer_length );
/* WF200 host SDIO bus API */
sl_status_t wf200_host_sdio_transfer_cmd52( wf200_host_bus_tranfer_type_t type, uint8_t function, uint32_t address, uint8_t* buffer );
sl_status_t wf200_host_sdio_transfer_cmd53( wf200_host_bus_tranfer_type_t type, uint8_t function, uint32_t address, uint8_t* buffer, uint16_t buffer_length );
sl_status_t wf200_host_sdio_enable_high_speed_mode( void );

#endif // __WF200_HOST_API_H
