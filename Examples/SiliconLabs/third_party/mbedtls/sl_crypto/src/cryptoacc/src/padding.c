#include "padding.h"
#include <string.h>
#include "sx_memcpy.h"


void pad_zeros(uint8_t *EM, size_t emLen, uint8_t *hash, size_t hashLen)
{
   memset(EM, 0x00, emLen - hashLen);
   memcpy_array(EM + emLen - hashLen, hash, hashLen);
}

void pad_zeros_blk(block_t out, block_t in)
{
   memset(out.addr, 0x00, out.len - in.len);
   if(!(out.flags & BLOCK_S_CONST_ADDR)) out.addr += (out.len - in.len);
   memcpy_blk(out, in, in.len);
}
