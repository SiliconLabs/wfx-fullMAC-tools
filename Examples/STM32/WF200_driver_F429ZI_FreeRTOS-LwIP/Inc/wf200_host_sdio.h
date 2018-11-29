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

#pragma once
#include "stm32f4xx_hal.h"
#include "wf200_host_api.h"

#define SDIO_BLOCK_SIZE  0x200
#define SDIO_BLOCK_MODE_THRESHOLD     0x200

/* SDIO CMD53 argument */
#define SDIO_CMD53_WRITE                       ( 1 << 31 )
#define SDIO_CMD53_FUNCTION( function )        ( ( ( function ) & 0x7 ) << 28 )
#define SDIO_CMD53_BLOCK_MODE                  ( 1 << 27 )
#define SDIO_CMD53_OPMODE_INCREASING_ADDRESS   ( 1 << 26 )
#define SDIO_CMD53_ADDRESS( address )          ( ( ( address ) & 0x1ffff ) << 9 )
#define SDIO_CMD53_COUNT( count )              ( ( count ) & 0x1ff )

#define SDIO_CMD53_IS_BLOCK_MODE( arg )        ( ( ( arg ) & SDIO_CMD53_BLOCK_MODE ) != 0 )
#define SDIO_CMD53_GET_COUNT( arg )            ( SDIO_CMD53_COUNT( arg ) )

static uint32_t            rx_buffer_id;
static uint32_t            tx_buffer_id;
