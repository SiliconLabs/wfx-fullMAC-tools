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
 * \file wf200_registers.h
 * \brief Register definitions for the WF200 chip
 *
 */

#pragma once

#include <stdint.h>

#define SYS_BASE_ADDR_SILICON       (0)
#define PAC_BASE_ADDRESS_SILICON    (SYS_BASE_ADDR_SILICON + 0x09000000)
#define PAC_SHARED_MEMORY_SILICON   (PAC_BASE_ADDRESS_SILICON)

#define WF200_APB(addr)        (PAC_SHARED_MEMORY_SILICON + (addr))


/* Download control area */
/* boot loader start address in SRAM */
#define DOWNLOAD_BOOT_LOADER_OFFSET (0x00000000)
/* 32K, 0x4000 to 0xDFFF */
//#define DOWNLOAD_FIFO_OFFSET        (0x00004000)
/* 32K */
#define DOWNLOAD_FIFO_SIZE      (0x00008000)
/* 128 bytes, 0xFF80 to 0xFFFF */

#define DOWNLOAD_CTRL_DATA_DWORDS   (32-6)


#define DOWNLOAD_CTRL_OFFSET        (0x0900C000)
#define DOWNLOAD_IMAGE_SIZE_REG     (DOWNLOAD_CTRL_OFFSET + 0)
#define DOWNLOAD_PUT_REG            (DOWNLOAD_CTRL_OFFSET + offsetof(struct download_cntl_t, put))
#define DOWNLOAD_TRACE_PC_REG       (DOWNLOAD_CTRL_OFFSET + offsetof(struct download_cntl_t, trace_pc))
#define DOWNLOAD_GET_REG            (DOWNLOAD_CTRL_OFFSET + offsetof(struct download_cntl_t, get))
#define DOWNLOAD_STATUS_REG         (DOWNLOAD_CTRL_OFFSET + offsetof(struct download_cntl_t, status))
#define DOWNLOAD_DEBUG_DATA_REG     (DOWNLOAD_CTRL_OFFSET + offsetof(struct download_cntl_t, debug_data))
#define DOWNLOAD_DEBUG_DATA_LEN     (108)

#define DOWNLOAD_BLOCK_SIZE     (1024)

#define ADDR_DWL_CTRL_AREA              0x0900C000
#define FW_SIGNATURE_SIZE               64
#define FW_HASH_SIZE                    8
#define ADDR_DWL_CTRL_AREA_IMAGE_SIZE   (ADDR_DWL_CTRL_AREA + 0)
#define ADDR_DWL_CTRL_AREA_PUT          (ADDR_DWL_CTRL_AREA + 4)
#define ADDR_DWL_CTRL_AREA_GET          (ADDR_DWL_CTRL_AREA + 8)
#define ADDR_DWL_CTRL_AREA_HOST_STATUS  (ADDR_DWL_CTRL_AREA + 12)
#define ADDR_DWL_CTRL_AREA_NCP_STATUS   (ADDR_DWL_CTRL_AREA + 16)
#define ADDR_DWL_CTRL_AREA_SIGNATURE    (ADDR_DWL_CTRL_AREA + 20)
#define ADDR_DWL_CTRL_AREA_FW_HASH      (ADDR_DWL_CTRL_AREA_SIGNATURE + FW_SIGNATURE_SIZE)
#define ADDR_DWL_CTRL_AREA_FW_VERSION   (ADDR_DWL_CTRL_AREA_FW_HASH + FW_HASH_SIZE)

#define HOST_STATE_UNDEF                0xFFFFFFFF
#define HOST_STATE_NOT_READY            0x12345678
#define HOST_STATE_READY                0x87654321
#define HOST_STATE_HOST_INFO_READ       0xA753BD99
#define HOST_STATE_UPLOAD_PENDING       0xABCDDCBA
#define HOST_STATE_UPLOAD_COMPLETE      0xD4C64A99
#define HOST_STATE_OK_TO_JUMP           0x174FC882

#define NCP_STATE_UNDEF                 0xFFFFFFFF
#define NCP_STATE_NOT_READY             0x12345678
#define NCP_STATE_INFO_READY            0xBD53EF99
#define NCP_STATE_READY                 0x87654321
#define NCP_STATE_DOWNLOAD_PENDING      0xABCDDCBA
#define NCP_STATE_DOWNLOAD_COMPLETE     0xCAFEFECA
#define NCP_STATE_AUTH_OK               0xD4C64A99
#define NCP_STATE_AUTH_FAIL             0x174FC882
#define NCP_STATE_PUB_KEY_RDY           0x7AB41D19

#define ADDR_DOWNLOAD_FIFO_BASE         0x09004000
#define ADDR_DOWNLOAD_FIFO_END          0x0900C000
#define ADDR_SHARED_RAM_DEBUG_AREA      0x09002000

#define BIT(n) (1 << (n))

/* WBF - Control register bit set */
/* next o/p length, bit 11 to 0 */
#define WF200_CONT_NEXT_LEN_MASK  (0x0FFF)
#define WF200_CONT_WUP_BIT        (BIT(12))
#define WF200_CONT_RDY_BIT        (BIT(13))
#define WF200_CONT_IRQ_ENABLE     (BIT(14))
#define WF200_CONT_RDY_ENABLE     (BIT(15))
#define WF200_CONT_IRQ_RDY_ENABLE (BIT(14)|BIT(15))

/* SPI Config register bit set */
/*TODO update these bits definitions : word_mode are now in 8 and 9*/
#define WF200_CONFIG_FRAME_BIT    (BIT(2))
#define WF200_CONFIG_WORD_MODE_BITS   (BIT(8)|BIT(9))
#define WF200_CONFIG_WORD_MODE_1  (BIT(8))
#define WF200_CONFIG_WORD_MODE_2  (BIT(9))
#define WF200_CONFIG_ERROR_0_BIT  (BIT(5))
#define WF200_CONFIG_ERROR_1_BIT  (BIT(6))
#define WF200_CONFIG_ERROR_2_BIT  (BIT(7))
/*  */
#define WF200_CONFIG_CSN_FRAME_BIT    (BIT(7))
#define WF200_CONFIG_ERROR_3_BIT  (BIT(8))
#define WF200_CONFIG_ERROR_4_BIT  (BIT(9))
/* QueueM */
#define WF200_CONFIG_ACCESS_MODE_BIT  (BIT(10))
/* AHB bus */
#define WF200_CONFIG_AHB_PRFETCH_BIT  (BIT(11))
#define WF200_CONFIG_CPU_CLK_DIS_BIT  (BIT(12))
/* APB bus */
#define WF200_CONFIG_PRFETCH_BIT  (BIT(13))
/* CPU reset */
#define WF200_CONFIG_CPU_RESET_BIT    (BIT(14))
#define WF200_CONFIG_CLEAR_INT_BIT    (BIT(15))

/* For WF200 the IRQ Enable and Ready Bits are in CONFIG register */
#define WF200_CONF_IRQ_ENABLE     (BIT(16))
#define WF200_CONF_RDY_ENABLE     (BIT(17))
#define WF200_CONF_IRQ_RDY_ENABLE (BIT(16)|BIT(17))

#define FW_VERSION_VALUE        0x00000001