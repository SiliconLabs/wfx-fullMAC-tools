/**
 * @file
 * @brief Provides a family of memcpy functions accelerated by the cryptoDMA
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */

#ifndef CRYPTODMA_H
#define CRYPTODMA_H

#include <stdint.h>
#include "cryptolib_types.h"

/**
 * Copy length bytes from the src block_t to the dest block_t
 *
 * This function speeds up the data transfer by using the internal DMA
 * @param src block_t to the source data to transfer
 * @param dest block_t to the destination location
 * @param length the length in bytes to transfer (from src to dest)
 */
void memcpy_blk(block_t dest, block_t src, uint32_t length);

/**
 * Copy length bytes from src to the block_t defined by dest
 *
 * This function speeds up the data transfer by using the internal DMA
 * @param  dest   block_t to the destination location
 * @param  src    input buffer
 * @param  length length of the data to copy
 */
void memcpy_blkOut(block_t dest, const volatile void * src, uint32_t length);

/**
 * Copy length bytes from the src block_t to the dest buffer
 *
 * This function speeds up the data transfer by using the internal DMA
 * @param  dest   output buffer
 * @param  src    input block_t
 * @param  length length of the data to copy
 */
void memcpy_blkIn(volatile void * dest, block_t src, uint32_t length);

/**
 * Copy length bytes from the src array to the dest array
 *
 * This function speeds up the data transfer by using the internal DMA
 * @param  dest   output array
 * @param  src    input array
 * @param  length length of the data to copy
 */
void memcpy_array(volatile void * dest, const volatile void * src, uint32_t length);

#endif
