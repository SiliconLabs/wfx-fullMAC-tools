/**
 * @file
 * @brief defines dma configuration functions
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */

#include <string.h>
#include "sx_memcpy.h"
#include "cryptodma_internal.h"
#include "cryptolib_def.h"

#ifndef BLK_MEMCPY_MIN_DMA_SIZE
#define BLK_MEMCPY_MIN_DMA_SIZE 0
#endif

void memcpy_blk(block_t dest, block_t src, uint32_t length)
{
   if (dest.flags & DMA_AXI_DESCR_DISCARD) {
      return;
   }
   if (dest.len < length) {
      length = dest.len;
   }
   if (!(src.flags & BLOCK_S_CONST_ADDR) && (src.len < length)) {
      length = src.len;
   }
   if (!length)
      return;
   if (length >= BLK_MEMCPY_MIN_DMA_SIZE) {
      cryptodma_config_direct(dest, src, length);
      cryptodma_start();
      cryptodma_wait();
      cryptodma_check_status();
   } else {
      if (src.flags & BLOCK_S_CONST_ADDR) {
         for (uint32_t i = 0; i < length; i+=4) {
            uint32_t v = *(volatile uint32_t*)src.addr;
            size_t len = (dest.len-i) < 4 ? (dest.len-i): 4;
            memcpy(dest.addr + i, &v, len);
         }
      } else {
         memcpy(dest.addr, src.addr, length);
      }
   }
}

void memcpy_blkOut(block_t dest, const volatile void * src, uint32_t length)
{
   block_t s = {(uint8_t*)src, length, BLOCK_S_INCR_ADDR};
   memcpy_blk(dest, s, length);
}

void memcpy_blkIn(volatile void * dest, block_t src, uint32_t length)
{
   block_t d = {(uint8_t*)dest, length, BLOCK_S_INCR_ADDR};
   memcpy_blk(d, src, length);
}

void memcpy_array(volatile void * dest, const volatile void * src, uint32_t length)
{
   block_t s = {(uint8_t*)src, length, BLOCK_S_INCR_ADDR};
   block_t d = {(uint8_t*)dest, length, BLOCK_S_INCR_ADDR};
   memcpy_blk(d, s, length);
}
