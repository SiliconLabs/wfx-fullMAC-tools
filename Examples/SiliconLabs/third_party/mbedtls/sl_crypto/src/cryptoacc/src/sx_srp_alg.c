/**
* @brief Handles the algorithmic operations for srp
* @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
* @file
*/


#include "sx_srp_alg.h"
#include "cryptolib_def.h"
#include "sx_memcpy.h"
#include "ba414ep_config.h"
#include "padding.h"
#include "sx_hash.h"
#include "sx_rng.h"
#include <string.h>
#include "sx_errors.h"


#if PK_CM_ENABLED && PK_CM_RANDPROJ_MOD
   #define SIZE_ADAPTATION (PK_CM_RAND_SIZE)
#else
   #define SIZE_ADAPTATION (0)
#endif


/**
 * @brief Generate \p out = hash( \p in1 | \p in2 )
 * @param out output digest buffer
 * @param in1 first part of message to hash
 * @param in2 second part message to hash
 * @param hash_fct function to use
 * @return length to \p out
*/
static uint32_t genHash2(block_t out, block_t in1, block_t in2, sx_hash_fct_t hash_fct)
{
   block_t array[2] = {in1, in2};
   uint32_t hlen = sx_hash_get_digest_size(hash_fct);
   if (sx_hash_array_blk(hash_fct, array, 2, block_t_convert(out.addr, hlen)))
      return 0;
   return hlen;
}


/** Compute the SRP k multiplier and copy it into crypto mem. */
static void srp_push_k_multiplier(block_t N, block_t g,
            sx_hash_fct_t hash_fct, uint32_t size,
            uint32_t SRP6a, uint32_t cryptomem_idx)
{
   uint8_t hashbuf[MAX_DIGESTSIZE];
   block_t k = BLOCK_T_CONV(hashbuf, MAX_DIGESTSIZE);

   if(SRP6a) {
      k.len = sx_hash_get_digest_size(hash_fct);
      /* hash( N | g) */
      genHash2(k, N, g, hash_fct);
   } else {
      k.len = 4;
      memset(hashbuf, 0, 4);
      hashbuf[3] = 3;
   }
   /* copy into crypto RAM */
   mem2CryptoRAM_rev(k, size + SIZE_ADAPTATION, cryptomem_idx);
}

/** Compute the SRP u factor and copy it into crypto mem. */
static void srp_push_u_factor(block_t A, block_t B, sx_hash_fct_t hash_fct,
   uint32_t size, uint32_t cryptomem_idx)
{
   uint8_t hashbuf[MAX_DIGESTSIZE];
   uint32_t hash_size = sx_hash_get_digest_size(hash_fct);
   block_t u = BLOCK_T_CONV(hashbuf, hash_size);

   /* hash(A | B) */
   genHash2(u, A, B, hash_fct);
   /* copy into crypto RAM */
   mem2CryptoRAM_rev(u, size + SIZE_ADAPTATION, cryptomem_idx);
}

/** Derive the SRP x user secret and copy it into crypto mem. */
static uint32_t srp_push_x_secret(block_t usrpwd, block_t s,
   sx_hash_fct_t hash_fct,
   uint32_t size, uint32_t cryptomem_idx)
{
   uint8_t hashbuf[MAX_DIGESTSIZE];
   uint32_t hash_size = sx_hash_get_digest_size(hash_fct);
   uint32_t status;
   block_t x = BLOCK_T_CONV(hashbuf, hash_size);

   /* intermediate x = hash( username | ":" | p ) */
   status = sx_hash_blk(hash_fct, usrpwd, x);
   if (status)
      return status;

   /* x = hash(s | x) */
   block_t array[2] = {s, x};
   status = sx_hash_array_blk(hash_fct, array, 2, x);

   /* copy into crypto RAM */
   mem2CryptoRAM_rev(x, size + SIZE_ADAPTATION, cryptomem_idx);
   return status;
}

uint32_t srp_host_gen_pub(block_t N, block_t g, block_t v, block_t B,
            block_t b, sx_hash_fct_t hash_fct, uint32_t SRP6a, uint32_t size,
            struct sx_rng rng)
{

   uint32_t status;
   uint32_t size_adapt = size + SIZE_ADAPTATION;
   ALIGNED uint8_t buf[SRP_MAX_KEY_SIZE] = {0};
   block_t tmp = BLOCK_T_CONV(buf, size);

   if (size > SRP_MAX_KEY_SIZE)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   if (N.len != size || g.len != size || B.len != size || b.len != size ||
         v.len != size)
      return CRYPTOLIB_INVALID_PARAM;

   // Set byte_swap
   ba414ep_set_command(BA414EP_OPTYPE_SRP_SERVER_PK, size_adapt, BA414EP_BIGEND,
                      BA414EP_SELCUR_NO_ACCELERATOR);

   // Location 0 -> N
   mem2CryptoRAM_rev(N, size_adapt, BA414EP_MEMLOC_0);

   // Location 3 -> g

   mem2CryptoRAM_rev(g, size_adapt, BA414EP_MEMLOC_3);

   // Location 10 -> v

   mem2CryptoRAM_rev(v, size_adapt, BA414EP_MEMLOC_10);

   // Location 7:  k mulitplier

   srp_push_k_multiplier(N, g, hash_fct, size, SRP6a, BA414EP_MEMLOC_7);

   //Generates priv key
   status = sx_rng_get_rand_lt_n_blk(tmp, N, rng);
   if (status)
      return status;

   // Location 12 -> k
   mem2CryptoRAM_rev(tmp, size_adapt, BA414EP_MEMLOC_12);

   //Output of the pub key
   memcpy_blk(b, tmp, size); //To the output FIFO


   /* Start BA414EP */
   status = ba414ep_start_wait_status();
   if (status)
      return CRYPTOLIB_CRYPTO_ERR;

   // Output of the pub key
   CryptoRAM2mem_rev(B, size, BA414EP_MEMLOC_5);
   return CRYPTOLIB_SUCCESS;
}


uint32_t srp_host_gen_key(block_t N, block_t A, block_t v, block_t b,
                          block_t B, block_t K, sx_hash_fct_t hash_fct,
                          uint32_t size)
{
   uint32_t status;
   uint32_t size_adapt = size + SIZE_ADAPTATION;
   ALIGNED uint8_t buf[SRP_MAX_KEY_SIZE] = {0};
   block_t tmp = BLOCK_T_CONV(buf, SRP_MAX_KEY_SIZE);

   if (size > SRP_MAX_KEY_SIZE)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   if (N.len != size || A.len != size || b.len != size || B.len != size ||
         v.len != size || K.len != sx_hash_get_digest_size(hash_fct))
      return CRYPTOLIB_INVALID_PARAM;

   // Set byte_swap
   ba414ep_set_command(BA414EP_OPTYPE_SRP_SERVER_KEY, size_adapt, BA414EP_BIGEND,
                      BA414EP_SELCUR_NO_ACCELERATOR);

   // Location 10 -> v
   mem2CryptoRAM_rev(v, size_adapt, BA414EP_MEMLOC_10);
   // Location 12-> b
   mem2CryptoRAM_rev(b, size_adapt, BA414EP_MEMLOC_12);
   // Location 0 -> N
   mem2CryptoRAM_rev(N, size_adapt, BA414EP_MEMLOC_0);
   // Location 2-> A
   mem2CryptoRAM_rev(A, size_adapt, BA414EP_MEMLOC_2);

   //Generates u
   srp_push_u_factor(A, B, hash_fct, size, BA414EP_MEMLOC_8);

   /* Start BA414EP */
   status = ba414ep_start_wait_status();
   if (status)
      return CRYPTOLIB_CRYPTO_ERR;

   // Output of the pub key
   tmp.len = size;
   CryptoRAM2mem_rev(tmp, tmp.len, BA414EP_MEMLOC_11);

   //Hash S to create K
   return sx_hash_blk(hash_fct, tmp, K);
}


uint32_t srp_user_gen_key(block_t N, block_t g, block_t A, block_t B,
                          block_t usrpwd, block_t s,
                          block_t a, block_t K,
                          sx_hash_fct_t hash_fct, uint32_t SRP6a,
                          uint32_t size)
{
   uint32_t status;
   uint32_t size_adapt = size + SIZE_ADAPTATION;
   ALIGNED uint8_t buf[SRP_MAX_KEY_SIZE] = {0};
   block_t tmp = BLOCK_T_CONV(buf, SRP_MAX_KEY_SIZE);

   if (size > SRP_MAX_KEY_SIZE)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   if (N.len != size || g.len != size || a.len != size || A.len != size ||
         B.len != size || K.len != sx_hash_get_digest_size(hash_fct))
      return CRYPTOLIB_INVALID_PARAM;

   // Set byte_swap
   ba414ep_set_command(BA414EP_OPTYPE_SRP_CLIENT_KEY, size_adapt, BA414EP_BIGEND,
                      BA414EP_SELCUR_NO_ACCELERATOR);

   g.len = size;
   a.len = size;
   N.len = size;

   // Location 0 -> N
   mem2CryptoRAM_rev(N, size_adapt, BA414EP_MEMLOC_0);
   // Location 3 -> g
   mem2CryptoRAM_rev(g, size_adapt, BA414EP_MEMLOC_3);
   // Location 5 -> B
   mem2CryptoRAM_rev(B, size_adapt, BA414EP_MEMLOC_5);

   // Location 6: Generate hash x
   status = srp_push_x_secret(usrpwd, s, hash_fct, size, BA414EP_MEMLOC_6);
   if (status)
      return status;

   // Location 4 -> a (From the parameters but need to keep the order of inputs)
   mem2CryptoRAM_rev(a, size_adapt, BA414EP_MEMLOC_4);

   //Generates hash k
   srp_push_k_multiplier(N, g, hash_fct, size, SRP6a, BA414EP_MEMLOC_7);

   // Location 8: Generate hash u
   srp_push_u_factor(A, B, hash_fct, size, BA414EP_MEMLOC_8);

   /* Start BA414EP */
   status = ba414ep_start_wait_status();
   if (status)
      return CRYPTOLIB_CRYPTO_ERR;

   // Output of the pub key
   tmp.len = size;
   CryptoRAM2mem_rev(tmp, tmp.len, BA414EP_MEMLOC_11);

   //Hash S to create K
   return sx_hash_blk(hash_fct, tmp, K);
}


uint32_t srp_gen_verifier(block_t N, block_t g, block_t s,
                          block_t usrpwd,
                          block_t v, sx_hash_fct_t hash_fct, uint32_t size)
{
   uint32_t status;
   uint32_t size_adapt = size + SIZE_ADAPTATION;

   if (size > SRP_MAX_KEY_SIZE)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   if (N.len != size || g.len != size || v.len != size)
      return CRYPTOLIB_INVALID_PARAM;

   // Set byte_swap
   ba414ep_set_command(BA414EP_OPTYPE_MOD_EXP, size_adapt, BA414EP_BIGEND,
                      BA414EP_SELCUR_NO_ACCELERATOR);

   // Set BA414EP config reg
   ba414ep_set_config(10, 11, 12, 0);

   // Location 0 -> N
   mem2CryptoRAM_rev(N, size_adapt, BA414EP_MEMLOC_0);
   // Location 10 -> g
   mem2CryptoRAM_rev(g, size_adapt, BA414EP_MEMLOC_10);
   // Location 11 -> x
   status = srp_push_x_secret(usrpwd, s, hash_fct, size, BA414EP_MEMLOC_11);
   if (status)
      return status;

   /* Start BA414EP */
   status = ba414ep_start_wait_status();
   if (status)
      return CRYPTOLIB_CRYPTO_ERR;

   // Output of the pub key
   CryptoRAM2mem_rev(v, size, BA414EP_MEMLOC_12);

   return CRYPTOLIB_SUCCESS;
}


uint32_t srp_gen_pub(block_t N, block_t g, block_t a, block_t A, uint32_t size,
      struct sx_rng rng)
{
   uint32_t status;
   uint32_t size_adapt = size + SIZE_ADAPTATION;
   ALIGNED uint8_t buf[SRP_MAX_KEY_SIZE] = {0};
   block_t tmp = BLOCK_T_CONV(buf, size);

   if (size > SRP_MAX_KEY_SIZE)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   if (N.len != size || g.len != size || a.len != size || A.len != size)
      return CRYPTOLIB_INVALID_PARAM;

   //Generates priv key
   status = sx_rng_get_rand_lt_n_blk(tmp, N, rng);
   if (status)
      return status;

   // Set byte_swap
   ba414ep_set_command(BA414EP_OPTYPE_MOD_EXP, size_adapt, BA414EP_BIGEND,
                      BA414EP_SELCUR_NO_ACCELERATOR);

   // Set BA414EP config reg
   ba414ep_set_config(10, 11, 12, 0);

   // Location 0 -> N
   mem2CryptoRAM_rev(N, size_adapt, BA414EP_MEMLOC_0);
   // Location 10 -> g
   mem2CryptoRAM_rev(g, size_adapt, BA414EP_MEMLOC_10);
   // Location 11 -> B
   mem2CryptoRAM_rev(tmp, size_adapt, BA414EP_MEMLOC_11);

   // Copy the tmp to the priv key (a)
   memcpy_blk(a, tmp, size);

   /* Start BA414EP */
   status = ba414ep_start_wait_status();
   if (status)
      return CRYPTOLIB_CRYPTO_ERR;

   // Output of the pub key
   CryptoRAM2mem_rev(A, size, BA414EP_MEMLOC_12);

   return CRYPTOLIB_SUCCESS;
}
