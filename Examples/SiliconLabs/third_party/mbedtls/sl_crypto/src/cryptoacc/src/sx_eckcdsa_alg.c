/**
 * @file
 * @brief Defines the ECKCDSA algorithms
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */

#include "sx_eckcdsa_alg.h"
#include "cryptolib_def.h"
#include "sx_memcpy.h"
#include "sx_memcmp.h"
#include "ba414ep_config.h"
#include "sx_hash.h"
#include "sx_rng.h"
#include "sx_errors.h"


uint32_t eckcdsa_generate_private_key(block_t n, block_t priv, struct sx_rng rng)
{
   if (n.len > ECC_MAX_KEY_SIZE)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   return sx_rng_get_rand_lt_n_blk(priv, n, rng);
}

uint32_t eckcdsa_generate_public_key(
      block_t domain,
      block_t pub,
      block_t priv,
      uint32_t size,
      uint32_t pk_flags)
{
   if (size > ECC_MAX_KEY_SIZE)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   // Only domain of 6 parameters are supported (Weierstrass prime or binary)
   if (pub.len != 2 * size || priv.len != size || domain.len != 6 * size)
      return CRYPTOLIB_INVALID_PARAM;

   ba414ep_set_command(BA414EP_OPTYPE_ECKCDSA_PK_GEN, size, BA414EP_BIGEND, pk_flags);
   ba414ep_load_curve(domain, size, BA414EP_BIGEND, 1);

   mem2CryptoRAM_rev(priv, priv.len, BA414EP_MEMLOC_6);
   if (ba414ep_start_wait_status())
      return CRYPTOLIB_CRYPTO_ERR;

   CryptoRAM2mem_rev(pub, size, BA414EP_MEMLOC_8);
   pub.addr += size;
   CryptoRAM2mem_rev(pub, size, BA414EP_MEMLOC_9);

   return CRYPTOLIB_SUCCESS;
}

uint32_t eckcdsa_generate_keypair(
      block_t domain,
      block_t pub,
      block_t priv,
      uint32_t size,
      uint32_t pk_flags,
      struct sx_rng rng)
{
   block_t n = block_t_convert(domain.addr + size, size);
   uint32_t status = eckcdsa_generate_private_key(n, priv, rng);
   if (status)
      return status;
   return eckcdsa_generate_public_key(domain, pub, priv, size, pk_flags);
}

uint32_t eckcdsa_generate_signature(
      sx_hash_fct_t algo_hash,
      block_t domain,
      block_t pub_key,
      block_t priv_key,
      block_t message,
      block_t signature,
      uint32_t size,
      uint32_t curve_flags,
      struct sx_rng rng)
{
   if (size > ECC_MAX_KEY_SIZE)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   if (domain.len != 6 * size ||
         pub_key.len != 2 * size ||
         priv_key.len != size ||
         signature.len != 2 * size)
      return CRYPTOLIB_INVALID_PARAM;

   // H(Cq || M) is not dependent of k, compute it out of the loop

   // Extract Qx, Qy
   block_t puKeyX = block_t_convert(pub_key.addr, size);
   block_t puKeyY = block_t_convert(pub_key.addr + size, size);

   // Move private key to BA414EP_MEMLOC_6
   ba414ep_set_command(BA414EP_OPTYPE_ECC_POINT_MULT, size,
                      BA414EP_BIGEND, BA414EP_SELCUR_NO_ACCELERATOR);
   mem2CryptoRAM_rev(priv_key, size, BA414EP_MEMLOC_6);

   // Build hash of Cq || m with Cq = Qx || Qy with padding/truncation
   const uint32_t hashBlkSize = sx_hash_get_block_size(algo_hash);
   const uint32_t dgstSize    = sx_hash_get_digest_size(algo_hash);
   block_t blocks[4];
   uint32_t zero = 0; // use to pad, must be 4bytes
   size_t msgIdx = 0;
   if (hashBlkSize != 0) { // if dgst_sr is valid
      if (hashBlkSize <= size) { // Cq based only on Qx
         blocks[0] = block_t_convert(puKeyX.addr, hashBlkSize);
         msgIdx = 1;
      } else if (hashBlkSize <= 2 * size) { // Cq based only on Qx, Qy
         blocks[0] = puKeyX;
         blocks[1] = block_t_convert(puKeyY.addr, hashBlkSize - size);
         msgIdx = 2;
      } else {
         blocks[0] = puKeyX;
         blocks[1] = puKeyY;
         blocks[2].addr  = (uint8_t*)&zero;
         blocks[2].len   = hashBlkSize - 2 * size;
         blocks[2].flags = BLOCK_S_CONST_ADDR; // tricky, do not allocate a buffer to fill with zero
         msgIdx = 3;// Cq padded with 0
      }
   } else
      return CRYPTOLIB_INVALID_PARAM;

   blocks[msgIdx] = message;
   block_t hashBlk = block_t_convert(BA414EP_ADDR_MEMLOC(BA414EP_MEMLOC_12, dgstSize), dgstSize);
   uint32_t status = sx_hash_array_blk(algo_hash, blocks, msgIdx + 1, hashBlk);
   if (status)
      return status;

   // Loop on k, Wx, r and s generation until s != 0
   uint32_t retries = 5;
   block_t n = block_t_convert(domain.addr + size, size);
   do {
      // Generate k at BA414EP_MEMLOC_7
      uint8_t kBuff[ECC_MAX_KEY_SIZE] = {0};
      block_t kBlk = block_t_convert(kBuff, size);
      status = sx_rng_get_rand_lt_n_blk(kBlk, n, rng);
      if (status)
         return status;
      mem2CryptoRAM_rev(kBlk, kBlk.len, BA414EP_MEMLOC_7);

      // Set command to enable byte-swap (point multiplication for W)
      // Generate Wx at BA414EP_MEMLOC_9
      ba414ep_set_command(BA414EP_OPTYPE_ECC_POINT_MULT, size, BA414EP_BIGEND, curve_flags);
      ba414ep_set_config(BA414EP_MEMLOC_2, BA414EP_MEMLOC_7, BA414EP_MEMLOC_9, BA414EP_MEMLOC_0);

      // Load Gx and Gy and start BA414EP
      ba414ep_load_curve(domain, size, BA414EP_BIGEND, 1);
      if (ba414ep_start_wait_status())
         return CRYPTOLIB_CRYPTO_ERR;

      // compute r = H(Wx) at BA414EP_MEMLOC_10
      block_t wxBlk = block_t_convert(BA414EP_ADDR_MEMLOC(BA414EP_MEMLOC_9, size), size);
      hashBlk = block_t_convert(BA414EP_ADDR_MEMLOC(BA414EP_MEMLOC_10, dgstSize), dgstSize);
      status = sx_hash_blk(algo_hash, wxBlk, hashBlk);
      if (status)
         return status;

      // Generate s at BA414EP_MEMLOC_11 and loop if s == 0
      ba414ep_set_command(BA414EP_OPTYPE_ECKCDSA_SIGN_GEN, size, BA414EP_BIGEND, curve_flags);
      status = ba414ep_start_wait_status();
      if(status != CRYPTOLIB_INVALID_SIGN_ERR && status != CRYPTOLIB_SUCCESS)
         return CRYPTOLIB_CRYPTO_ERR;
   } while (status == CRYPTOLIB_INVALID_SIGN_ERR && --retries);

   if(status == CRYPTOLIB_INVALID_SIGN_ERR)
      return CRYPTOLIB_CRYPTO_ERR;

   // Push r and s to the dma.
   CryptoRAM2point_rev(signature, size, BA414EP_MEMLOC_10);

   return CRYPTOLIB_SUCCESS;
}

uint32_t eckcdsa_verify_signature(
      sx_hash_fct_t algo_hash,
      block_t domain,
      block_t pub,
      block_t message,
      block_t signature,
      uint32_t size,
      uint32_t curve_flags)
{
   if (size > ECC_MAX_KEY_SIZE)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   if (domain.len != 6 * size ||
         pub.len != 2 * size ||
         signature.len != 2 * size)
      return CRYPTOLIB_INVALID_PARAM;

   const size_t hashBlkSize = sx_hash_get_block_size(algo_hash);
   const size_t dgstSize = sx_hash_get_digest_size(algo_hash);

   // Push Qx,Qy to BA414EP_MEMLOC_8/9
   point2CryptoRAM_rev(pub, size, BA414EP_MEMLOC_8);

   block_t pubKeyX = block_t_convert(BA414EP_ADDR_MEMLOC(BA414EP_MEMLOC_8, size), size);
   block_t pubKeyY = block_t_convert(BA414EP_ADDR_MEMLOC(BA414EP_MEMLOC_9, size), size);

   // Build hash of Cq || m with Cq = Qx || Qy with padding/truncation
   block_t blocks[4];
   uint64_t zero = 0; // HARDWARE DEPENDENT !! this assumes that bus width is 64 ,if it's less then
   //it's still fine but if it's more then it won't work TODO: make it in a cleaner way to not depend on hardware

   size_t msgIdx = 0;
   if (hashBlkSize != 0) {
      if (hashBlkSize <= size) {
         blocks[0] = block_t_convert(pubKeyX.addr, hashBlkSize);
         msgIdx = 1;
      } else if (hashBlkSize <= 2 * size) {
         blocks[0] = pubKeyX;
         blocks[1] = block_t_convert(pubKeyY.addr, hashBlkSize - size);
         msgIdx = 2;
      } else {
         blocks[0] = pubKeyX;
         blocks[1] = pubKeyY;
         blocks[2].addr  = (uint8_t*)&zero;
         blocks[2].len   = hashBlkSize - 2 * size;
         blocks[2].flags = BLOCK_S_CONST_ADDR; // tricky, do not allocate a buffer to fill with zero
         msgIdx = 3;// Cq padded with 0
      }
   } else
      return CRYPTOLIB_INVALID_PARAM;
   blocks[msgIdx] = message;

   // generate H(Qx || Qy || m') at BA414EP_MEM_LOC_12
   ba414ep_set_command(BA414EP_OPTYPE_MULT, dgstSize, BA414EP_BIGEND, curve_flags); // ensure big endian
   block_t hashBlk = block_t_convert(BA414EP_ADDR_MEMLOC(BA414EP_MEMLOC_12, dgstSize), dgstSize);
   uint32_t status = sx_hash_array_blk(algo_hash, blocks, msgIdx + 1, hashBlk);
   if (status)
      return status;

   hashBlk = block_t_convert(BA414EP_ADDR_MEMLOC(BA414EP_MEMLOC_12, size), size);

   // Push r,s to BA414EP_MEMLOC_10/11
   point2CryptoRAM_rev(signature, size, BA414EP_MEMLOC_10);

   // Compute Wx' and Wy'
   ba414ep_set_command(BA414EP_OPTYPE_ECKCDSA_SIGN_VERIF, size, BA414EP_BIGEND, curve_flags);
   ba414ep_load_curve(domain, size, BA414EP_BIGEND, 1);
   if (ba414ep_start_wait_status())
      return CRYPTOLIB_CRYPTO_ERR;

   // Compute r' = H(Wx')
   uint8_t hashBuff[MAX_DIGESTSIZE] = {0};
   hashBlk = block_t_convert(hashBuff, dgstSize);
   block_t ba414ep_loc13 = block_t_convert(BA414EP_ADDR_MEMLOC(BA414EP_MEMLOC_13, size), size);

   status = sx_hash_blk(algo_hash, ba414ep_loc13, hashBlk);
   if (status)
      return status;

   // Generate r == r'
   block_t rBlk = block_t_convert(BA414EP_ADDR_MEMLOC(BA414EP_MEMLOC_10, size), size);
   uint32_t dgst_local_len = SX_MIN(dgstSize, size);
   status = memcmp_time_cst(
         rBlk.addr + rBlk.len - dgst_local_len,
         hashBlk.addr + hashBlk.len - dgst_local_len,
         dgst_local_len);

   return status ? CRYPTOLIB_INVALID_SIGN_ERR : CRYPTOLIB_SUCCESS;
}
