/**
 * @file
 * @brief Defines derive key functions
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */


#ifndef SX_DERIVE_KEY_ALG_H
#define SX_DERIVE_KEY_ALG_H

#include <stdint.h>
#include "compiler_extentions.h"
#include "cryptolib_types.h"
#include "sx_hash.h"

/**
 * @brief HKDF derivation key function as defined by RFC 5869
 *
 * @param  hash_fct function to use for HMAC (see ::sx_hash_fct_t)
 * @param  ikm      input key to be derived
 * @param  salt     derivation salt
 * @param  info_in  derivation info (application-dependant and optional)
 * @param  length   desired length of the derived key
 * @param  okm      derived key (output key)
 * @return          CRYPTOLIB_SUCCESS if success
 */
uint32_t sx_derive_key_hkdf(
      sx_hash_fct_t hash_fct,
      block_t ikm,
      block_t salt,
      block_t info_in,
      uint32_t length,
      block_t okm) CHECK_RESULT;

/**
 * @brief PBKDF2 key derivation function as defined by RFC 8018
 *
 * @param  hash_fct       hash function to use (see ::sx_hash_fct_t)
 * @param  password       password to derive
 * @param  salt           derivation salt
 * @param  iterations     desired number of iterations
 * @param  dkLen          desired length of the derived key
 * @param  dk             derived key (output key)
 * @return                CRYPTOLIB_SUCCESS on success
 */
uint32_t sx_derive_key_pbkdf2(
      sx_hash_fct_t hash_fct,
      block_t password,
      block_t salt,
      uint32_t iterations,
      uint32_t dkLen, block_t dk) CHECK_RESULT;

/**
 * @brief KDF2 key derivation function as defined by IEEE 1363a-2004 and ANSI-X9.63-KDF.
 * @param  hash_fct     hash function to use (see ::sx_hash_fct_t)
 * @param  secret       shared secret string
 * @param  param        shared parameter string
 * @param  derived_key  derived key (output key)
 * @return              CRYPTOLIB_SUCCESS on success
 */
uint32_t sx_derive_key_kdf2(sx_hash_fct_t hash_fct, block_t secret, block_t param, block_t derived_key) CHECK_RESULT;

#endif
