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
 * \file wf200_bus_sdio.c
 * \brief Bus low level operations that dependent on the underlying physical bus: SDIO implementation
 *
 */

#include "wf200_bus.h"
#include "wf200_host_api.h"
#include "firmware/wf200_registers.h"
#include "wf200_configuration.h"
#include <stddef.h>
#include <string.h>

#ifndef WF200_SDIO_BLOCK_MODE_THRESHOLD
#define WF200_SDIO_BLOCK_MODE_THRESHOLD     0x200
#endif

// From the SDIO specification
#define SDIO_FBR_FN1_BLOCK_SIZE_LSB_REGISTER  ( 0x110 ) /* Function 1 16-bit block size LSB */
#define SDIO_FBR_FN1_BLOCK_SIZE_MSB_REGISTER  ( 0x111 ) /* Function 1 16-bit block size MSB */

static uint32_t            rx_buffer_id;
static uint32_t            tx_buffer_id;


sl_status_t wf200_reg_read( wf200_register_address_t address, void* buffer, uint32_t length )
{
    sl_status_t result;
    uint32_t buffer_id = 0;
    uint32_t reg_address;
    uint16_t control_register;
    uint32_t current_transfer_size = ( length >= WF200_SDIO_BLOCK_MODE_THRESHOLD ) ? ROUND_UP( length, WF200_SDIO_BLOCK_SIZE ) : length;

    if ( address == WF200_IN_OUT_QUEUE_REG_ID )
    {
        buffer_id = rx_buffer_id++;
        if ( rx_buffer_id > 4 )
        {
            rx_buffer_id = 1;
        }
    }

    reg_address = ( buffer_id << 7 ) | ( address << 2 );

    result = wf200_host_sdio_transfer_cmd53( WF200_BUS_READ, 1, reg_address, buffer, current_transfer_size );
    if ( address == WF200_IN_OUT_QUEUE_REG_ID )
    {
      /*In block mode, the piggy_back value is at the end of the block. Append it at the end of the frame instead.*/
      if( length > WF200_SDIO_BLOCK_MODE_THRESHOLD)
      {
        memcpy( (uint8_t*)((uint8_t*)buffer + length - 2), (uint8_t*)((uint8_t*)buffer + current_transfer_size - 2), 2);
      }
      /* If the piggy-back value is null, acknowledge the received frame with a dummy read*/
      control_register = UNPACK_16BIT_LITTLE_ENDIAN(((uint8_t*)buffer) + length - 2);
      if((control_register & WF200_CONT_NEXT_LEN_MASK) == 0){
        wf200_reg_read_32( WF200_CONFIG_REG_ID, NULL );
      }
    }
    return result;
}

sl_status_t wf200_reg_write( wf200_register_address_t address, const void* buffer, uint32_t length )
{
    uint32_t buffer_id = 0;
    uint32_t reg_address;
    uint32_t current_transfer_size = ( length >= WF200_SDIO_BLOCK_MODE_THRESHOLD ) ? ROUND_UP( length, WF200_SDIO_BLOCK_SIZE ) : length;

    if ( address == WF200_IN_OUT_QUEUE_REG_ID )
    {
        buffer_id = tx_buffer_id++;
        if ( tx_buffer_id > 31 )
        {
            tx_buffer_id = 0;
        }
    }

    reg_address = ( buffer_id << 7 ) | ( address << 2 );

    return wf200_host_sdio_transfer_cmd53( WF200_BUS_WRITE, 1, reg_address, (void*)buffer, current_transfer_size );
}


sl_status_t wf200_init_bus( void )
{
    sl_status_t result;
    uint32_t    value32;
    uint8_t     value_u8;

    rx_buffer_id = 1;
    tx_buffer_id = 0;

    wf200_host_reset_chip( );
    
    result = wf200_host_init_bus();
    ERROR_CHECK(result);

    result = wf200_host_sdio_transfer_cmd52( WF200_BUS_READ, 0, 2, &value_u8 );
    ERROR_CHECK( result );

    // Enables Function 1
    value_u8 |= ( 1 << 1 );
    result = wf200_host_sdio_transfer_cmd52( WF200_BUS_WRITE, 0, 2, &value_u8 );
    ERROR_CHECK( result );

    // Enables Master and Function 1 interrupts
    result = wf200_host_sdio_transfer_cmd52( WF200_BUS_READ, 0, 4, &value_u8 );
    ERROR_CHECK( result );
    value_u8 |= 0x1 | ( 1 << 1 );
    result = wf200_host_sdio_transfer_cmd52( WF200_BUS_WRITE, 0, 4, &value_u8 );
    ERROR_CHECK( result );

    // Set bus width to 4-bit
    result = wf200_host_sdio_transfer_cmd52( WF200_BUS_READ, 0, 7, &value_u8 );
    ERROR_CHECK( result );
    value_u8 = ( value_u8 & 0xFC ) | 0x2;
    result = wf200_host_sdio_transfer_cmd52( WF200_BUS_WRITE, 0, 7, &value_u8 );
    ERROR_CHECK( result );

    // Switch to HS mode
    result = wf200_host_sdio_transfer_cmd52( WF200_BUS_READ, 0, 0x13, &value_u8 );
    ERROR_CHECK( result );
    value_u8 |= 0x2; // Set EHS to 1
    result = wf200_host_sdio_transfer_cmd52( WF200_BUS_WRITE, 0, 0x13, &value_u8 );
    ERROR_CHECK(result);

    // Enabled SDIO high speed mode
    wf200_host_sdio_enable_high_speed_mode( );

    // Set function 1 block size
    value_u8 = WF200_SDIO_BLOCK_SIZE & 0xff;
    result = wf200_host_sdio_transfer_cmd52( WF200_BUS_WRITE, 0, SDIO_FBR_FN1_BLOCK_SIZE_LSB_REGISTER, &value_u8 );
    ERROR_CHECK( result );

    value_u8 = ( WF200_SDIO_BLOCK_SIZE >> 8 ) & 0xff;
    result = wf200_host_sdio_transfer_cmd52( WF200_BUS_WRITE, 0, SDIO_FBR_FN1_BLOCK_SIZE_MSB_REGISTER, &value_u8 );
    ERROR_CHECK( result );

    result = wf200_reg_read_32( WF200_CONFIG_REG_ID, &value32);
    ERROR_CHECK(result);

    if ( value32 == 0 || value32 == 0xFFFFFFFF )
    {
        result = SL_ERROR;
        ERROR_CHECK( result );
    }

error_handler:
    return result;
}

sl_status_t wf200_deinit_bus( void )
{
    return wf200_host_deinit_bus();
}
