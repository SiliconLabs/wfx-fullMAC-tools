/**
 * @file
 * @brief Defines the procedures to check BA414EP integrity
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */

#include "sx_pk_interface.h"
#include "cryptolib_def.h"
#include "sx_memcpy.h"
#include "sx_memcmp.h"
#include "ba414ep_config.h"
#include "sx_hash.h"
#include "sx_errors.h"

uint32_t sx_pk_ucode_integrity_check(void)
{
   BA414EP_ucode_t *ucode = (BA414EP_ucode_t *) ADDR_BA414EP_UCODE;

   uint32_t wl;
   block_t blk_i;
   block_t blk_h;
   uint8_t digest[SHA256_DIGESTSIZE];

   // get BA414EP microcode length in words
   // note: ucode word are 16-bit values stored on 32-bit words)
   wl = ucode->info.ucodesize;
   if(wl > BA414EP_UCODE_MAX_LENGTH)   // check wl is valid
      return CRYPTOLIB_INVALID_SIGN_ERR;

   // hash BA414EP microcode content
   blk_i = block_t_convert((uint8_t*) ADDR_BA414EP_UCODE, 2*wl);
   blk_h = block_t_convert(digest, sizeof(digest));

   if(sx_hash_blk(e_SHA256, blk_i, blk_h))
      return CRYPTOLIB_CRYPTO_ERR;

   // check that reference value match computed value
   if (memcmp_time_cst((uint8_t*)&ucode->content[wl], digest, 4)){
      return CRYPTOLIB_INVALID_SIGN_ERR;
   } else {
      return CRYPTOLIB_SUCCESS;
   }
}


void sx_pk_set_rng(struct sx_rng rng)
{
#if (PK_CM_ENABLED) == 1
   ba414ep_set_rng(rng);
#else
   (void)rng;
   CRYPTOLIB_ASSERT(0, "No counter-measures activated");
#endif
}

uint32_t sx_pk_clear_memory()
{
   ba414ep_set_command(
         BA414EP_OPTYPE_CLEAR_MEM,
         0,
         BA414EP_LITTLEEND,
         BA414EP_SELCUR_NO_ACCELERATOR);
   return ba414ep_start_wait_status() ? CRYPTOLIB_CRYPTO_ERR : CRYPTOLIB_SUCCESS;
}
