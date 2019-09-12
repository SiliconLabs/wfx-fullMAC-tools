/**
 * @file
 * @brief Defines the DSA algorithms (see NIST.FIPS.186-4.pdf,
 *        Section 4 - The Digital Signature Algorithm)
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */


#ifndef SX_DSA_ALG_H
#define SX_DSA_ALG_H

#include <stdint.h>
#include "cryptolib_types.h"
#include "sx_hash.h"
#include "sx_rng.h"

/**
 * @brief Generates DSA key pair (public and private)
 * @param  generator Generator to use for key generation
 * @param  p      p modulo
 * @param  q      q modulo
 * @param  pub    Public key (output)
 * @param  priv   Private key (output)
 * @param  rng    Used random number generator
 * @return        Returns CRYPTOLIB_SUCCESS if success
 *                Returns CRYPTOLIB_UNSUPPORTED_ERR if p/q length is higher
 *                than DSA_MAX_SIZE_P/Q
 */
uint32_t dsa_generate_keypair(
      block_t generator,
      block_t p,
      block_t q,
      block_t pub,
      block_t priv,
      struct sx_rng rng);

/**
 * @brief Generates DSA private key
 * @param  q      q modulo
 * @param  priv   Private key (output)
 * @param  rng    Used random number generator
 * @return        Returns CRYPTOLIB_SUCCESS if success
 *                Returns CRYPTOLIB_UNSUPPORTED_ERR if q length is higher
 *                than DSA_MAX_SIZE_Q
 */
uint32_t dsa_generate_private_key(
      block_t q,
      block_t priv,
      struct sx_rng rng);

/**
 * @brief Generates DSA public key with a given private
 *
 * @param  generator Generator to use for key generation
 * @param  p      p modulo
 * @param  q      q modulo
 * @param  pub    Public key (output)
 * @param  priv   Private key
 * @return        Returns CRYPTOLIB_SUCCESS if success
 *                Returns CRYPTOLIB_UNSUPPORTED_ERR if p/q length is higher
 *                than DSA_MAX_SIZE_P/Q
 */
uint32_t dsa_generate_public_key(
      block_t generator,
      block_t p,
      block_t q,
      block_t pub,
      block_t priv);

/**
 * @brief Generates DSA signature of a message
 *
 * @param  algo_hash hash function to use
 * @param  p         p modulo
 * @param  q         q modulo
 * @param  generator Generator of the key \p priv
 * @param  priv      Private key
 * @param  message   Message
 * @param  signature DSA signature of \p message (output)
 * @param  rng       Used random number generator
 * @return           CRYPTOLIB_SUCCESS if success
 *                   Returns CRYPTOLIB_UNSUPPORTED_ERR if p/q length is higher
 *                   than DSA_MAX_SIZE_P/Q
 */
uint32_t dsa_generate_signature(
      sx_hash_fct_t algo_hash,
      block_t p,
      block_t q,
      block_t generator,
      block_t priv,
      block_t message,
      block_t signature,
      struct sx_rng rng);

/**
 * @brief Verifies a given signature corresponds to a given message
 *
 * @param  algo_hash hash function to use
 * @param  generator generator of the key
 * @param  p         p modulo
 * @param  q         q modulo
 * @param  pub       Public key
 * @param  signature DSA signature to check
 * @param  message   Message to check
 * @return           CRYPTOLIB_SUCCESS if signature matches
 *                   CRYPTOLIB_CRYPTO_ERR if signature or error occurs
 *                   Returns CRYPTOLIB_UNSUPPORTED_ERR if p/q length is higher
 *                   than DSA_MAX_SIZE_P/Q
 */
uint32_t dsa_verify_signature(
      sx_hash_fct_t algo_hash,
      block_t p,
      block_t q,
      block_t generator,
      block_t pub,
      block_t message,
      block_t signature);


#endif
