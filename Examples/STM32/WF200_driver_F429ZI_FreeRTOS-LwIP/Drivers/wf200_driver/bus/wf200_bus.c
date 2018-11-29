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
 * \file wf200_bus.c
 * \brief Common bus operations that are independent of the underlying physical bus
 *
 */

#include "wf200.h"
#include "wf200_bus.h"
#include "wf200_host_api.h"

#define MAX_RETRIES 3
#define CONFIG_PREFECH_BIT (1 << 13)

sl_status_t wf200_reg_read_16( wf200_register_address_t address, uint16_t* value_out )
{
    uint8_t tmp[4];
    sl_status_t result = wf200_reg_read(  address, tmp, sizeof( tmp ) );

    *value_out = UNPACK_16BIT_LITTLE_ENDIAN( tmp );

    return result;
}

sl_status_t wf200_reg_write_16( wf200_register_address_t address, uint16_t value_in )
{
    uint8_t tmp[4];

    PACK_16BIT_LITTLE_ENDIAN( tmp, value_in );
    tmp[2] = 0;
    tmp[3] = 0;

    return wf200_reg_write(  address, tmp, sizeof( tmp ) );
}

sl_status_t wf200_reg_read_32( wf200_register_address_t address, uint32_t* value_out )
{
    uint8_t tmp[4];
    sl_status_t result = wf200_reg_read( address, tmp, sizeof( tmp ) );

    *value_out = UNPACK_32BIT_LITTLE_ENDIAN( tmp );
    return result;
}

sl_status_t wf200_reg_write_32( wf200_register_address_t address, uint32_t value_in )
{
    uint8_t tmp[4];

    PACK_32BIT_LITTLE_ENDIAN( tmp, value_in );

    return wf200_reg_write( address, tmp, sizeof( tmp ) );
}

sl_status_t wf200_data_read( void* buffer, uint32_t length )
{
    sl_status_t result;

    result = wf200_reg_read(  WF200_IN_OUT_QUEUE_REG_ID, buffer, length );

    return result;
}

sl_status_t wf200_data_write( const void* buffer, uint32_t length )
{
    sl_status_t result;

    result = wf200_reg_write(  WF200_IN_OUT_QUEUE_REG_ID, buffer, length );

    return result;
}

sl_status_t wf200_apb_write( uint32_t address, const void* buffer, uint32_t length )
{
    sl_status_t result;
    if ( length / 2 >= 0x1000 )
    {
        return SL_BAD_ARG;
    }

    result = wf200_reg_write_32( WF200_SRAM_BASE_ADDR_REG_ID, address );

    if ( result == SL_SUCCESS )
    {
        result = wf200_reg_write(  WF200_SRAM_DPORT_REG_ID, buffer, length );
    }

    return result;
}

sl_status_t wf200_apb_write_32( uint32_t address, uint32_t value_in )
{
    sl_status_t result;

    result = wf200_reg_write_32( WF200_SRAM_BASE_ADDR_REG_ID, address );

    if ( result == SL_SUCCESS )
    {
        result = wf200_reg_write_32( WF200_SRAM_DPORT_REG_ID, value_in );
    }

    return result;
}

sl_status_t wf200_apb_read_32( uint32_t address, uint32_t* value_out )
{
    uint32_t value32;
    sl_status_t result;

    // write address
    result = wf200_reg_write_32( WF200_SRAM_BASE_ADDR_REG_ID, address );

    // set the "prefetch" bit
    result = wf200_reg_read_32( WF200_CONFIG_REG_ID, &value32 );
    result = wf200_reg_write_32( WF200_CONFIG_REG_ID, value32 | CONFIG_PREFECH_BIT );

    // and wait for the prefetch bit to clear
    for ( uint32_t i = 0; i < 20; i++ )
    {
        result = wf200_reg_read_32( WF200_CONFIG_REG_ID, &value32 );
        if ( ( value32 & CONFIG_PREFECH_BIT ) == 0 )
        {
            break;
        }

        wf200_host_wait( 1 );
    }

    // and data is ready
    result = wf200_reg_read_32( WF200_SRAM_DPORT_REG_ID, value_out );

    return result;
}
