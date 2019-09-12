/**
 * @file
 * @brief Implements the procedures to make RSA operations with
 *          the BA414EP pub key
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */

#include <string.h>

#include "sx_rsa_alg.h"
#include "cryptolib_def.h"
#include "sx_memcpy.h"
#include "ba414ep_config.h"
#include "padding.h"
#include "sx_hash.h"
#include "sx_errors.h"
#include "sx_rsa_pad.h"
#include "sx_primitives.h"
#include "sx_prime_alg.h"
#include "sx_math.h"


/**
 * @brief Generate both P and Q for a given length
 * @param p  Output P (big prime number of \c size /2 bytes)
 * @param Q  Output Q (big prime number of \c size /2 bytes)
 * @param size Size of the RSA element (expressed in bytes)
 * @param rng The random number generator to use
 *
 * return ::CRYPTOLIB_SUCCESS if both P and Q are successfully generated
 *        ::CRYPTOLIB_UNSUPPORTED_ERR if size is bigger than RSA_MAX_SIZE
 */
static uint32_t rsa_generate_p_q(
      block_t p,
      block_t q,
      const size_t size,
      struct sx_rng rng)
{
#if (PRIME_GEN_ENABLED == 1)
   CRYPTOLIB_ASSERT_NM(!(p.flags & BLOCK_S_CONST_ADDR));
   CRYPTOLIB_ASSERT_NM(!(q.flags & BLOCK_S_CONST_ADDR));

   if (size > RSA_MAX_SIZE)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   if (size != 2 * p.len || size != 2 * q.len)
      return CRYPTOLIB_INVALID_PARAM;

   uint8_t result[RSA_MAX_SIZE] = {0};
   uint8_t pBuff[RSA_MAX_SIZE] = {0};
   uint8_t qBuff[RSA_MAX_SIZE] = {0};

   bool continue_searching = true;
   // Find proper prime numbers
   while(continue_searching) {
      // 1. Generate two random numbers
      rng.get_rand_blk(rng.param, block_t_convert(pBuff + size/2, size/2));
      rng.get_rand_blk(rng.param, block_t_convert(qBuff + size/2, size/2));

      // 2. Force lsb to '1' and force msb to '1'
      pBuff[size-1] |= 0x1;
      qBuff[size-1] |= 0x1;
      pBuff[size/2] |= 0x80;
      qBuff[size/2] |= 0x80;

      // 3. Calculate modulus result = r1 * r2
      uint32_t status = multiplication(pBuff, qBuff, result, size);
      if (status)
         return status;

      status = !(result[0] & 0x80); //MSB == 1 (ensure right size in bits)
      if (!status) {
         // Converge to prime numbers.
         // FIXME Overflows of p/qBuff are undetected
         status = converge_to_prime(pBuff + size/2, size/2, 10, rng);
         status |= converge_to_prime(qBuff + size/2, size/2, 10, rng);
         if (!status) {
            status = multiplication(pBuff, qBuff, result, size);
            if (status)
               return status;

            status = !(result[0] & 0x80);
         } else {
            return status;
         }

         continue_searching = !!status; // If no status, primes are found
      }
   }

   memcpy_blkOut(p, pBuff + size/2, size/2);
   memcpy_blkOut(q, qBuff + size/2, size/2);
   return CRYPTOLIB_SUCCESS;
#else
   (void)p;
   (void)q;
   (void)size;
   (void)rng;
   return CRYPTOLIB_UNSUPPORTED_ERR;
#endif
}

uint32_t rsa_generate_private_key_from_p_q(
      const block_t p,
      const block_t q,
      const block_t exponent,
      block_t n,
      block_t private_key,
      block_t lambda,
      const uint32_t size)
{
   if (size > RSA_MAX_SIZE)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   if (size != 2*p.len ||
         size != 2*q.len ||
         exponent.len > size ||
         n.len != size ||
         private_key.len != size ||
         (lambda.len && lambda.len != size))
      return CRYPTOLIB_INVALID_PARAM;

   ba414ep_set_command(
         BA414EP_OPTYPE_RSA_PK_GEN,
         size,
         BA414EP_BIGEND,
         BA414EP_SELCUR_NO_ACCELERATOR);

   mem2CryptoRAM_rev(p, size, BA414EP_MEMLOC_2);
   mem2CryptoRAM_rev(q, size, BA414EP_MEMLOC_3);
   mem2CryptoRAM_rev(exponent, size, BA414EP_MEMLOC_8);

   const uint32_t error = ba414ep_start_wait_status();
   if (error == BA414EP_STS_NINV_MASK)
      return BA414EP_STS_NINV_MASK;
   if (error)
      return CRYPTOLIB_CRYPTO_ERR;

   CryptoRAM2mem_rev(n, size, BA414EP_MEMLOC_0);
   if (lambda.len == size)
      CryptoRAM2mem_rev(lambda, size, BA414EP_MEMLOC_1);
   CryptoRAM2mem_rev(private_key, size, BA414EP_MEMLOC_6);

   return CRYPTOLIB_SUCCESS;
}

uint32_t rsa_generate_private_key(
      const block_t exponent,
      block_t modulus,
      block_t d,
      block_t lambda,
      const size_t size,
      struct sx_rng rng)
{
   if (size > RSA_MAX_SIZE)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   // Both exponent, modulus, d and lambda are
   // checked in rsa_generate_private_key_from_p_q

   uint8_t p_buf[RSA_MAX_SIZE / 2] = {0};
   uint8_t q_buf[RSA_MAX_SIZE / 2] = {0};
   block_t p = block_t_convert(p_buf, size / 2);
   block_t q = block_t_convert(q_buf, size / 2);

   uint32_t status = CRYPTOLIB_SUCCESS;
   do {
      status = rsa_generate_p_q(p, q, size, rng);
      if (status)
         break;
      status = rsa_generate_private_key_from_p_q(
            p,
            q,
            exponent,
            modulus,
            d,
            lambda,
            size);
   } while (status == BA414EP_STS_NINV_MASK);
   return status;
}

uint32_t rsa_generate_crt_private_key_from_p_q(
      const block_t p,
      const block_t q,
      const block_t d,
      block_t dp,
      block_t dq,
      block_t q_inv,
      const uint32_t size)
{
   if (size > RSA_MAX_SIZE)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   if (2*p.len != size ||
         2*q.len != size ||
         d.len != size ||
         2*dp.len != size ||
         2*dq.len != size ||
         2*q_inv.len != size)
      return CRYPTOLIB_INVALID_PARAM;

   ba414ep_set_command(
         BA414EP_OPTYPE_RSA_CRT_GEN,
         size,
         BA414EP_BIGEND,
         BA414EP_SELCUR_NO_ACCELERATOR);

   mem2CryptoRAM_rev(p, size, BA414EP_MEMLOC_2);
   mem2CryptoRAM_rev(q, size, BA414EP_MEMLOC_3);
   mem2CryptoRAM_rev(d, size, BA414EP_MEMLOC_6);

   if (ba414ep_start_wait_status())
      return CRYPTOLIB_CRYPTO_ERR;

   // PK is configured to work with size bits but for dp,dq and q_inv,
   // we only expect size/2
   CryptoRAM2mem_rev(dp, size/2, BA414EP_MEMLOC_10);
   CryptoRAM2mem_rev(dq, size/2, BA414EP_MEMLOC_11);
   CryptoRAM2mem_rev(q_inv, size/2, BA414EP_MEMLOC_12);

   return CRYPTOLIB_SUCCESS;
}

uint32_t rsa_generate_crt_private_key(
      const block_t exponent,
      block_t modulus,
      block_t p,
      block_t q,
      block_t dp,
      block_t dq,
      block_t q_inv,
      const size_t size,
      struct sx_rng rng)
{
   if (size > RSA_MAX_SIZE)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   // Both exponent, modulus, dp, dq and q_inv are
   // checked in rsa_generate_private_key_from_p_q
   if (2*p.len != size || 2*q.len != size)
      return CRYPTOLIB_INVALID_PARAM;

   uint8_t d_buf[RSA_MAX_SIZE] = {0};
   block_t d = block_t_convert(d_buf, size);

   uint32_t status = CRYPTOLIB_SUCCESS;
   do {
      status = rsa_generate_p_q(p, q, size, rng);
      if (status)
         break;
      status = rsa_generate_private_key_from_p_q(
            p,
            q,
            exponent,
            modulus,
            d,
            NULL_blk,
            size);
   } while (status == BA414EP_STS_NINV_MASK);
   if (status)
      return status;

   // dq, dp and q_inv are updated only in case of success
   status = rsa_generate_crt_private_key_from_p_q(p, q, d, dp, dq, q_inv, size);
   if (status)
      return status;
   return CRYPTOLIB_SUCCESS;
}

uint32_t rsa_encrypt(
      const rsa_pad_types padding,
      const block_t message,
      const block_t n,
      const block_t public_expo,
      block_t cipher,
      const sx_hash_fct_t hash,
      struct sx_rng rng)
{
   uint32_t status = CRYPTOLIB_SUCCESS;
   const uint32_t size = n.len;

   // Inputs checks
   CRYPTOLIB_ASSERT_NM(!(n.flags & BLOCK_S_CONST_ADDR));
   CRYPTOLIB_ASSERT_NM(!(public_expo.flags & BLOCK_S_CONST_ADDR));

   if (padding != ESEC_RSA_PADDING_NONE && padding != ESEC_RSA_PADDING_OAEP &&
         padding != ESEC_RSA_PADDING_EME_PKCS)
      return CRYPTOLIB_INVALID_PARAM;
   if(size > RSA_MAX_SIZE)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   if (public_expo.len > size ||
         (message.len != size && padding == ESEC_RSA_PADDING_NONE) ||
         (message.len > size && padding != ESEC_RSA_PADDING_NONE) ||
         cipher.len != size)
      return CRYPTOLIB_INVALID_PARAM;

   // Set encryption command to issue
   ba414ep_set_command(
         BA414EP_OPTYPE_RSA_ENC,
         size,
         BA414EP_BIGEND,
         BA414EP_SELCUR_NO_ACCELERATOR);

   // Copy elements to CryptoRAM
   if (padding == ESEC_RSA_PADDING_NONE) {
      // As plaintext may come from a fifo, we've to copy it first before using it
      // multiple times (which is our case, we also want to check p < n). To avoid
      // extra copies we load it directly into the cryptoRAM and check it in place.
      mem2CryptoRAM_rev(message, size, BA414EP_MEMLOC_5);
      uint8_t *ba414ep_msg = BA414EP_ADDR_MEMLOC(BA414EP_MEMLOC_5, size);
      if (!sx_math_less_than(ba414ep_msg, n.addr, size, sx_big_endian))
         return CRYPTOLIB_INVALID_PARAM;
   } else {
      uint8_t EM[RSA_MAX_SIZE] = {0};
      if (padding == ESEC_RSA_PADDING_OAEP)
         status = rsa_pad_eme_oaep_encode(size, hash, EM, message, message.len, rng);
      else // ESEC_RSA_PADDING_EME_PKCS
         status = rsa_pad_eme_pkcs_encode(n.len, EM, message, message.len, rng);

      if(status)
         return status;
      mem2CryptoRAM_rev(block_t_convert(EM, size), size, BA414EP_MEMLOC_5);
   }

   mem2CryptoRAM_rev(n, size, BA414EP_MEMLOC_0);
   mem2CryptoRAM_rev(public_expo, size, BA414EP_MEMLOC_8);

   // Issue RSA encryption
   if (ba414ep_start_wait_status())
      return CRYPTOLIB_CRYPTO_ERR;

   // Read back data
   CryptoRAM2mem_rev(cipher, size, BA414EP_MEMLOC_4);
   return status;
}

/**
 * @brief Decode the padding associated to a RSA decryption and extract plaintext
 * @param padding Type of padding used
 * @param plain Pointer to the decoded data (block_t), both the data and the
 *        length will be updated
 * @param hash Type of hash used when OAEP padding is specified
 * @param size Size of a RSA element
 * @return ::CRYPTOLIB_SUCCESS when padding decoding is successful
 */
static uint32_t rsa_decrypt_padding(
      const rsa_pad_types padding,
      block_t *plain,
      const sx_hash_fct_t hash,
      const size_t size)
{
   uint32_t status = CRYPTOLIB_SUCCESS;

   // No input to checks here, already done in caller
   if(padding == ESEC_RSA_PADDING_NONE) {
      plain->len = size;
      CryptoRAM2mem_rev(*plain, size, BA414EP_MEMLOC_5);
   } else {
      uint8_t em[RSA_MAX_SIZE + 4];
      CryptoRAM2mem_rev(block_t_convert(em, size), size, BA414EP_MEMLOC_5);

      uint8_t *addr = NULL;
      if(padding == ESEC_RSA_PADDING_OAEP)
        status = rsa_pad_eme_oaep_decode(size, hash, em, &addr, (size_t*)&plain->len);
      else
        status = rsa_pad_eme_pkcs_decode(size, em, &addr, (size_t*)&plain->len);
      if (status)
         return status;
     memcpy_blkOut(*plain, addr, plain->len);
   }

   return status;
}

uint32_t rsa_decrypt(
      const rsa_pad_types padding,
      const block_t cipher,
      const block_t n,
      const block_t private_key,
      const block_t lambda,
      block_t *plain,
      const sx_hash_fct_t hash)
{
   // Inputs checks
   CRYPTOLIB_ASSERT_NM(!(n.flags & BLOCK_S_CONST_ADDR));
   CRYPTOLIB_ASSERT_NM(!(private_key.flags & BLOCK_S_CONST_ADDR));
   CRYPTOLIB_ASSERT_NM(!(lambda.flags & BLOCK_S_CONST_ADDR));

   const size_t size = n.len;
#if PK_CM_ENABLED
   const size_t pk_size = size + (PK_CM_RANDPROJ_MOD ? PK_CM_RAND_SIZE : 0);
#else
   const size_t pk_size = size;
#endif
   if (size > RSA_MAX_SIZE)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   if (cipher.len != size || private_key.len != size ||
         (lambda.len && lambda.len != size) ||
         plain->len < size) // Reserve enough place for output plaintext
      return CRYPTOLIB_INVALID_PARAM;
   if (padding != ESEC_RSA_PADDING_NONE && padding != ESEC_RSA_PADDING_OAEP &&
         padding != ESEC_RSA_PADDING_EME_PKCS)
      return CRYPTOLIB_INVALID_PARAM;

   // Set command to issue
   ba414ep_set_command(
         BA414EP_OPTYPE_RSA_DEC,
         pk_size,
         BA414EP_BIGEND,
         BA414EP_SELCUR_NO_ACCELERATOR);

   // Copy elements to CryptoRAM
#if PK_CM_ENABLED
   if (PK_CM_RANDKE_MOD)
      mem2CryptoRAM_rev(lambda, pk_size, BA414EP_MEMLOC_1);
#endif
   mem2CryptoRAM_rev(private_key, pk_size, BA414EP_MEMLOC_6);
   mem2CryptoRAM_rev(n, pk_size, BA414EP_MEMLOC_0);
   mem2CryptoRAM_rev(cipher, pk_size, BA414EP_MEMLOC_4);

   // As ciphertext may come from a fifo, we've to copy it first before using it
   // multiple times (which is our case, we also want to check c < n). To avoid
   // extra copies we load it directly into the cryptoRAM and check it in place.
   uint8_t *ba414ep_ciphertext = BA414EP_ADDR_MEMLOC(BA414EP_MEMLOC_4, size);
   uint8_t *ba414ep_n = BA414EP_ADDR_MEMLOC(BA414EP_MEMLOC_0, size);
   if (!sx_math_less_than(ba414ep_ciphertext, ba414ep_n, size, sx_big_endian))
      return CRYPTOLIB_INVALID_PARAM;

   // Issue RSA decryption
   uint32_t status = ba414ep_start_wait_status();
   if (status)
      return CRYPTOLIB_CRYPTO_ERR;

   // Read back data and handle padding
   return rsa_decrypt_padding(padding, plain, hash, size);
}

uint32_t rsa_decrypt_crt(
      const rsa_pad_types padding,
      const block_t cipher,
      const block_t n,
      const block_t p,
      const block_t q,
      const block_t dp,
      const block_t dq,
      const block_t q_inv,
      block_t *plain,
      const sx_hash_fct_t hash)
{
   // Inputs checks
   CRYPTOLIB_ASSERT_NM(!(n.flags & BLOCK_S_CONST_ADDR));
   CRYPTOLIB_ASSERT_NM(!(p.flags & BLOCK_S_CONST_ADDR));
   CRYPTOLIB_ASSERT_NM(!(q.flags & BLOCK_S_CONST_ADDR));
   CRYPTOLIB_ASSERT_NM(!(dp.flags & BLOCK_S_CONST_ADDR));
   CRYPTOLIB_ASSERT_NM(!(dq.flags & BLOCK_S_CONST_ADDR));
   CRYPTOLIB_ASSERT_NM(!(q_inv.flags & BLOCK_S_CONST_ADDR));

   const size_t size = n.len;
#if PK_CM_ENABLED
   const size_t pk_size = size + (PK_CM_RANDPROJ_MOD ? 2*PK_CM_RAND_SIZE : 0);
#else
   const size_t pk_size = size;
#endif
   if (size > RSA_MAX_SIZE)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   if (cipher.len != size || 2*p.len != size || 2*q.len != size ||
         2*dp.len != size || 2*dq.len != size || 2*q_inv.len != size ||
         plain->len > size)
      return CRYPTOLIB_INVALID_PARAM;
   if (padding != ESEC_RSA_PADDING_NONE && padding != ESEC_RSA_PADDING_OAEP &&
         padding != ESEC_RSA_PADDING_EME_PKCS)
      return CRYPTOLIB_INVALID_PARAM;

   // Set command to issue
   ba414ep_set_command(
         BA414EP_OPTYPE_RSA_CRT_DEC,
         pk_size,
         BA414EP_BIGEND,
         BA414EP_SELCUR_NO_ACCELERATOR);

   // Copy elements to CryptoRAM
   mem2CryptoRAM_rev(cipher, pk_size, BA414EP_MEMLOC_4);
   mem2CryptoRAM_rev(p, pk_size, BA414EP_MEMLOC_2);
   mem2CryptoRAM_rev(q, pk_size, BA414EP_MEMLOC_3);
   mem2CryptoRAM_rev(dp, pk_size, BA414EP_MEMLOC_10);
   mem2CryptoRAM_rev(dq, pk_size, BA414EP_MEMLOC_11);
   mem2CryptoRAM_rev(q_inv, pk_size, BA414EP_MEMLOC_12);

   // As ciphertext may come from a fifo, we've to copy it first before using it
   // multiple times (which is our case, we also want to check c < n). To avoid
   // extra copies we load it directly into the cryptoRAM and check it in place.
   uint8_t *ba414ep_ciphertext = BA414EP_ADDR_MEMLOC(BA414EP_MEMLOC_4, size);
   if (!sx_math_less_than(ba414ep_ciphertext, n.addr, size, sx_big_endian))
      return CRYPTOLIB_INVALID_PARAM;

   // Issue command
   uint32_t status = ba414ep_start_wait_status();
   if (status)
      return CRYPTOLIB_CRYPTO_ERR;

   // Read back data and handle padding
   return rsa_decrypt_padding(padding, plain, hash, size);
}

static uint32_t rsa_generate_signature_padding(
      const rsa_pad_types padding,
      const block_t message,
      const block_t n,
      uint8_t *em,
      const sx_hash_fct_t hash_algo,
      const size_t salt_length,
      struct sx_rng rng)
{
   const size_t size = n.len;
   const size_t hash_size = sx_hash_get_digest_size(hash_algo);
   uint8_t hash[MAX_DIGESTSIZE];
   block_t h = block_t_convert(hash, hash_size);

   uint32_t status = sx_hash_blk(hash_algo, message, h);
   if (status)
      return status;

   if (padding == ESEC_RSA_PADDING_NONE)
      rsa_pad_zeros(em, size, hash, hash_size);
   else if (padding == ESEC_RSA_PADDING_EMSA_PKCS)
         status = rsa_pad_emsa_pkcs_encode(size, hash_algo, em, hash);
   else // ESEC_RSA_PADDING_PSS
      status = rsa_pad_emsa_pss_encode(size, hash_algo, em, hash, n.addr[0],
            salt_length, rng);

   return status;
}



uint32_t rsa_generate_signature(
      const rsa_pad_types padding,
      const size_t salt_length,
      const block_t message,
      const block_t n,
      const block_t private_key,
      const block_t lambda,
      block_t signature,
      const sx_hash_fct_t hash,
      struct sx_rng rng)
{
   // Inputs checks
   CRYPTOLIB_ASSERT_NM(!(n.flags & BLOCK_S_CONST_ADDR));
   CRYPTOLIB_ASSERT_NM(!(private_key.flags & BLOCK_S_CONST_ADDR));
   CRYPTOLIB_ASSERT_NM(!(lambda.flags & BLOCK_S_CONST_ADDR));

   const size_t size = n.len;
#if PK_CM_ENABLED
   const size_t pk_size = size + (PK_CM_RANDPROJ_MOD ? PK_CM_RAND_SIZE : 0);
#else
   const size_t pk_size = size;
#endif

   if (size > RSA_MAX_SIZE)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   if (private_key.len != size || (lambda.len && lambda.len != size) ||
         signature.len != size)
      return CRYPTOLIB_INVALID_PARAM;
   if (padding != ESEC_RSA_PADDING_NONE && padding != ESEC_RSA_PADDING_EMSA_PKCS &&
         padding != ESEC_RSA_PADDING_PSS)
      return CRYPTOLIB_INVALID_PARAM;

   // Handle padding
   uint8_t em[RSA_MAX_SIZE];
   uint32_t status = rsa_generate_signature_padding(
         padding, message, n, em, hash, salt_length, rng);
   if (status)
      return status;
   block_t hash_blk = block_t_convert(em, size);

   // Set command to issue
   ba414ep_set_command(
         BA414EP_OPTYPE_RSA_SIGN_GEN,
         pk_size,
         BA414EP_BIGEND,
         BA414EP_SELCUR_NO_ACCELERATOR);

   // Copy elements to CryptoRAM
#if PK_CM_ENABLED
   if (PK_CM_RANDKE_MOD)
      mem2CryptoRAM_rev(lambda, pk_size, BA414EP_MEMLOC_1);
#endif
   mem2CryptoRAM_rev(n, pk_size, BA414EP_MEMLOC_0);
   mem2CryptoRAM_rev(private_key, pk_size, BA414EP_MEMLOC_6);
   mem2CryptoRAM_rev(hash_blk, pk_size, BA414EP_MEMLOC_12);

   // Issue RSA signature
   if (ba414ep_start_wait_status())
      return CRYPTOLIB_CRYPTO_ERR;

   // Fetch signature
   CryptoRAM2mem_rev(signature, signature.len, BA414EP_MEMLOC_11);
   return CRYPTOLIB_SUCCESS;
}

uint32_t rsa_generate_signature_crt(
      const rsa_pad_types padding,
      const size_t salt_length,
      const block_t message,
      const block_t n,
      const block_t p,
      const block_t q,
      const block_t dp,
      const block_t dq,
      const block_t q_inv,
      block_t signature,
      const sx_hash_fct_t hash,
      struct sx_rng rng)
{
   // Inputs checks
   CRYPTOLIB_ASSERT_NM(!(n.flags & BLOCK_S_CONST_ADDR));
   CRYPTOLIB_ASSERT_NM(!(p.flags & BLOCK_S_CONST_ADDR));
   CRYPTOLIB_ASSERT_NM(!(q.flags & BLOCK_S_CONST_ADDR));
   CRYPTOLIB_ASSERT_NM(!(dp.flags & BLOCK_S_CONST_ADDR));
   CRYPTOLIB_ASSERT_NM(!(dq.flags & BLOCK_S_CONST_ADDR));
   CRYPTOLIB_ASSERT_NM(!(q_inv.flags & BLOCK_S_CONST_ADDR));

   const size_t size = n.len;
#if PK_CM_ENABLED
   const size_t pk_size = size + (PK_CM_RANDPROJ_MOD ? PK_CM_RAND_SIZE : 0);
#else
   const size_t pk_size = size;
#endif
   if (size > RSA_MAX_SIZE)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   if (2*p.len != size || 2*q.len != size || 2*dp.len != size ||
         2*dq.len != size || 2*q_inv.len != size || signature.len != size)
      return CRYPTOLIB_INVALID_PARAM;
   if (padding != ESEC_RSA_PADDING_NONE && padding != ESEC_RSA_PADDING_EMSA_PKCS &&
         padding != ESEC_RSA_PADDING_PSS)
      return CRYPTOLIB_INVALID_PARAM;

   // Handle padding
   uint8_t em[RSA_MAX_SIZE];
   uint32_t status = rsa_generate_signature_padding(
         padding, message, n, em, hash, salt_length, rng);
   if (status)
      return status;
   block_t hash_blk = block_t_convert(em, size);

   // Set command to issue
   ba414ep_set_command(
         BA414EP_OPTYPE_RSA_CRT_DEC,
         pk_size,
         BA414EP_BIGEND,
         BA414EP_SELCUR_NO_ACCELERATOR);

   // Copy elements to CryptoRAM
      mem2CryptoRAM_rev(hash_blk, pk_size, BA414EP_MEMLOC_4);
      mem2CryptoRAM_rev(p, pk_size, BA414EP_MEMLOC_2);
      mem2CryptoRAM_rev(q, pk_size, BA414EP_MEMLOC_3);
      mem2CryptoRAM_rev(dp, pk_size, BA414EP_MEMLOC_10);
      mem2CryptoRAM_rev(dq, pk_size, BA414EP_MEMLOC_11);
      mem2CryptoRAM_rev(q_inv, pk_size, BA414EP_MEMLOC_12);

   // Issue RSA signature
   status = ba414ep_start_wait_status();
   if (status)
      return CRYPTOLIB_CRYPTO_ERR;

   // Fetch signature
   CryptoRAM2mem_rev(signature, signature.len, BA414EP_MEMLOC_5);
   return CRYPTOLIB_SUCCESS;
}


uint32_t rsa_verify_signature(
      const rsa_pad_types padding,
      const size_t salt_length,
      const block_t message,
      const block_t n,
      const block_t exponent,
      const block_t signature,
      const sx_hash_fct_t sha_type)
{
   // Inputs checks
   CRYPTOLIB_ASSERT_NM(!(n.flags & BLOCK_S_CONST_ADDR));
   CRYPTOLIB_ASSERT_NM(!(exponent.flags & BLOCK_S_CONST_ADDR));

   const size_t size = n.len;
   uint8_t EM[RSA_MAX_SIZE];
   uint8_t hash[MAX_DIGESTSIZE];
   block_t hash_blk = block_t_convert(hash, sx_hash_get_digest_size(sha_type));

   if (size > RSA_MAX_SIZE)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   if (exponent.len > size || signature.len != size)
      return CRYPTOLIB_INVALID_PARAM;
   if (padding != ESEC_RSA_PADDING_NONE && padding != ESEC_RSA_PADDING_EMSA_PKCS &&
         padding != ESEC_RSA_PADDING_PSS)
      return CRYPTOLIB_INVALID_PARAM;

   // Handle padding
   uint32_t status = sx_hash_blk(sha_type, message, hash_blk);
   if (status)
      return status;

   if(padding == ESEC_RSA_PADDING_EMSA_PKCS) {
     status = rsa_pad_emsa_pkcs_encode(n.len, sha_type, EM, hash_blk.addr);
     hash_blk = block_t_convert(EM, n.len);
   }
   else if(padding == ESEC_RSA_PADDING_NONE){ //No padding
     rsa_pad_zeros(EM, n.len, hash_blk.addr, sx_hash_get_digest_size(sha_type));
     hash_blk = block_t_convert(EM, n.len);
   }
   if(status)
      return status;

   // Prepare command to issue (depends of the padding)
   uint32_t cmd = BA414EP_OPTYPE_RSA_SIGN_VERIF;
   if (padding == ESEC_RSA_PADDING_PSS)
      cmd = BA414EP_OPTYPE_RSA_ENC;
   ba414ep_set_command(cmd, size, BA414EP_BIGEND, BA414EP_SELCUR_NO_ACCELERATOR);

   mem2CryptoRAM_rev(n, size, BA414EP_MEMLOC_0);
   mem2CryptoRAM_rev(exponent, size, BA414EP_MEMLOC_8);
   if(padding != ESEC_RSA_PADDING_PSS) {
      mem2CryptoRAM_rev(signature, signature.len, BA414EP_MEMLOC_11);
      mem2CryptoRAM_rev(hash_blk, hash_blk.len, BA414EP_MEMLOC_12);
   }
   else {
      mem2CryptoRAM_rev(signature, signature.len, BA414EP_MEMLOC_5);
   }

   // Issue command
   status = ba414ep_start_wait_status();
   if (status)
      return CRYPTOLIB_INVALID_SIGN_ERR;

   if(padding == ESEC_RSA_PADDING_PSS) {
      CryptoRAM2mem_rev(block_t_convert(EM, n.len), n.len, BA414EP_MEMLOC_4);
      status = rsa_pad_emsa_pss_decode(n.len, sha_type, EM, hash_blk.addr,
                                      salt_length, n.addr[0]);
   }

   return (status ? CRYPTOLIB_INVALID_SIGN_ERR : CRYPTOLIB_SUCCESS);
}
