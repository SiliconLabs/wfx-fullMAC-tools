/**
 * @file
 * @brief Defines the DSA algorithms
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */

#include "sx_dsa_alg.h"
#include <string.h>
#include "cryptolib_def.h"
#include "sx_memcpy.h"
#include "ba414ep_config.h"
#include "sx_hash.h"
#include "sx_rng.h"
#include "sx_primitives.h"
#include "sx_prime_alg.h"
#include "sx_errors.h"

#if (DSA_ENABLED)

uint32_t dsa_generate_keypair(
      block_t generator,
      block_t p,
      block_t q,
      block_t pub,
      block_t priv,
      struct sx_rng rng)
{
   uint32_t status = dsa_generate_private_key(q, priv, rng);
   if (status)
      return status;
   //writes the pub to output
   return dsa_generate_public_key(generator, p, q, pub, priv);
}

uint32_t dsa_generate_private_key(
      block_t q,
      block_t priv,
      struct sx_rng rng)
{
   if (q.len > DSA_MAX_SIZE_Q)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   return sx_rng_get_rand_lt_n_blk(priv, q, rng);
}

uint32_t dsa_generate_public_key(
      block_t generator,
      block_t p,
      block_t q,
      block_t pub,
      block_t priv)
{
   if (p.len > DSA_MAX_SIZE_P || q.len > DSA_MAX_SIZE_Q)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   if (pub.len != p.len || generator.len != p.len || priv.len != q.len)
      return CRYPTOLIB_INVALID_PARAM;

   uint32_t size_adapt = p.len;

   #if PK_CM_ENABLED
      if (PK_CM_RANDPROJ_MOD) {
         size_adapt += PK_CM_RAND_SIZE;
      }
   #endif

   ba414ep_set_command(BA414EP_OPTYPE_DSA_KEY_GEN, size_adapt, BA414EP_BIGEND, BA414EP_SELCUR_NO_ACCELERATOR);

   mem2CryptoRAM_rev(p, size_adapt, BA414EP_MEMLOC_0);
   mem2CryptoRAM_rev(generator, size_adapt, BA414EP_MEMLOC_3);
   //TODO: fix blocksize?
   mem2CryptoRAM_rev(priv, size_adapt, BA414EP_MEMLOC_6);

   uint32_t error = ba414ep_start_wait_status();
   if (error)
      return CRYPTOLIB_CRYPTO_ERR;

   CryptoRAM2mem_rev(pub, pub.len, BA414EP_MEMLOC_8);

   return CRYPTOLIB_SUCCESS;
}


uint32_t dsa_generate_signature(
      sx_hash_fct_t hash_algo,
      block_t p,
      block_t q,
      block_t generator,
      block_t priv,
      block_t message,
      block_t signature,
      struct sx_rng rng)
{
   if (p.len > DSA_MAX_SIZE_P || q.len > DSA_MAX_SIZE_Q)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   if (generator.len != p.len || priv.len != q.len || signature.len != 2*q.len)
      return CRYPTOLIB_INVALID_PARAM;

   uint8_t kBuff[DSA_MAX_SIZE_P] = {0};
   uint8_t hashBuff[MAX_DIGESTSIZE] = {0};

   block_t kBlk = block_t_convert(kBuff, q.len);
   /* FIPS 186-3 4.6: leftmost bits of hash if q.len is smaller then hash len */
   uint32_t hashLen = SX_MIN(q.len, sx_hash_get_digest_size(hash_algo));
   block_t hashBlk = block_t_convert(hashBuff, hashLen);

   uint32_t size_adapt = p.len;

   #if PK_CM_ENABLED
      if (PK_CM_RANDPROJ_MOD) {
         size_adapt += PK_CM_RAND_SIZE;
      }
   #endif

   uint32_t error = sx_rng_get_rand_lt_n_blk(kBlk, q, rng);
   if (error)
      return error;

   error = sx_hash_blk(hash_algo, message, hashBlk);
   if (error)
      return CRYPTOLIB_CRYPTO_ERR;

   // Set command to enable byte-swap
   ba414ep_set_command(BA414EP_OPTYPE_DSA_SIGN_GEN, size_adapt, BA414EP_BIGEND, BA414EP_SELCUR_NO_ACCELERATOR);

   mem2CryptoRAM_rev(p, size_adapt, BA414EP_MEMLOC_0);
   mem2CryptoRAM_rev(q, size_adapt, BA414EP_MEMLOC_2);
   mem2CryptoRAM_rev(generator, size_adapt, BA414EP_MEMLOC_3);
   mem2CryptoRAM_rev(kBlk, size_adapt, BA414EP_MEMLOC_5);
   mem2CryptoRAM_rev(priv, size_adapt, BA414EP_MEMLOC_6);
   mem2CryptoRAM_rev(hashBlk, size_adapt, BA414EP_MEMLOC_12);

   error = ba414ep_start_wait_status();
   if (error)
      return CRYPTOLIB_CRYPTO_ERR;

   // Result pub key
   CryptoRAM2mem_rev(signature, q.len, BA414EP_MEMLOC_10);
   if (!(signature.flags & BLOCK_S_CONST_ADDR))
      signature.addr += q.len;
   CryptoRAM2mem_rev(signature, q.len, BA414EP_MEMLOC_11);

   return CRYPTOLIB_SUCCESS;
}


uint32_t dsa_verify_signature(
      sx_hash_fct_t hash_algo,
      block_t p,
      block_t q,
      block_t generator,
      block_t pub,
      block_t message,
      block_t signature)
{
   if (p.len > DSA_MAX_SIZE_P || q.len > DSA_MAX_SIZE_Q)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   if (pub.len != p.len || generator.len != p.len || signature.len != 2*q.len)
      return CRYPTOLIB_INVALID_PARAM;

   uint8_t hashBuff[MAX_DIGESTSIZE] = {0};
   /* FIPS 186-3 4.7: leftmost bits of hash if q.len is smaller then hash len */
   uint32_t hashLen = SX_MIN(q.len, sx_hash_get_digest_size(hash_algo));
   block_t hashBlk = block_t_convert(hashBuff, hashLen);

   uint32_t error = sx_hash_blk(hash_algo, message, hashBlk);
   if (error)
      return CRYPTOLIB_CRYPTO_ERR;

   // Set command to enable byte-swap
   uint32_t size = p.len;
   ba414ep_set_command(BA414EP_OPTYPE_DSA_SIGN_VERIF, size, BA414EP_BIGEND, BA414EP_SELCUR_NO_ACCELERATOR);

   mem2CryptoRAM_rev(p, size, BA414EP_MEMLOC_0);
   mem2CryptoRAM_rev(q, size, BA414EP_MEMLOC_2);
   mem2CryptoRAM_rev(generator, size, BA414EP_MEMLOC_3);
   mem2CryptoRAM_rev(pub, size, BA414EP_MEMLOC_8);

   signature.len = q.len; // Will be padded with zeros if smaller than q.len
   mem2CryptoRAM_rev(signature, size, BA414EP_MEMLOC_10);
   if (!(signature.flags & BLOCK_S_CONST_ADDR))
      signature.addr += q.len;

   mem2CryptoRAM_rev(signature, size, BA414EP_MEMLOC_11);
   mem2CryptoRAM_rev(hashBlk, size, BA414EP_MEMLOC_12);

   error = ba414ep_start_wait_status();
   if (error & BA414EP_STS_SINV_MASK) {
      return CRYPTOLIB_INVALID_SIGN_ERR;
   } else if (error) {
      return CRYPTOLIB_CRYPTO_ERR;
   }
   return CRYPTOLIB_SUCCESS;
}

#endif
