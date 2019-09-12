/**
 * @file
 * @brief Defines the ECKCDSA algorithms
 *
 * (See http://www.kisa.or.kr/public/library/report_View.jsp?regno=018906)
 *
 * Default format for ECC is big array of uint8_t (big endian) for both
 * private and public key. Public key is represented as the concatenation
 * of x and y coordinate for all but Montgomery and EdDSA.
 *
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */


#ifndef SX_ECKCDSA_ALG_H
#define SX_ECKCDSA_ALG_H

#include <stdint.h>
#include "cryptolib_def.h"
#include "cryptolib_types.h"
#include "sx_hash.h"
#include "sx_rng.h"

/**
 * @brief Generates an ECKCDSA private key
 * @param  n           the order coming from the curve parameters
 * @param  priv        private key used to generate
 * @param  rng         used random number generator
 * @return CRYPTOLIB_SUCCESS if no error
 *
 * @note By design, as the modulus is always coprime with the \c priv,
 *       \c priv is always invertible and the public key exists.
 */
uint32_t eckcdsa_generate_private_key(
      block_t n,
      block_t priv,
      struct sx_rng rng);

/**
 * @brief Generates an ECKCDSA public key
 * @param  domain      curve parameters
 * @param  pub         public key to recompute
 * @param  priv        private key used to compute the public key
 * @param  size        size of one element (expressed in bytes)
 * @param  curve_flags curve acceleration parameters
 * @return CRYPTOLIB_SUCCESS if no error
 */
uint32_t eckcdsa_generate_public_key(
      block_t domain,
      block_t pub,
      block_t priv,
      uint32_t size,
      uint32_t curve_flags);

/**
 * @brief Generates an ECKCDSA key pair (public and private)
 * @param  domain      curve parameters
 * @param  pub         public key to be generated
 * @param  priv        private key to be generated
 * @param  size        size of one element (expressed in bytes)
 * @param  curve_flags curve acceleration parameters
 * @param  rng         used random number generator
 * @return CRYPTOLIB_SUCCESS if no error
 */
uint32_t eckcdsa_generate_keypair(
      block_t domain,
      block_t pub,
      block_t priv,
      uint32_t size,
      uint32_t curve_flags,
      struct sx_rng rng);

/**
 * @brief Generates an ECKDSA signature of a message
 * @param  algo_hash   hash function to use (see ::sx_hash_fct_t)
 * @param  domain      domain to use
 * @param  pub         public key
 * @param  priv        private key
 * @param  message     message to sign
 * @param  signature   ECKCDSA signature of message
 * @param  size        size of one element (expressed in bytes)
 * @param  curve_flags curve acceleration parameters
 * @param  rng         used random number generator
 * @return CRYPTOLIB_SUCCESS if no error
 */
uint32_t eckcdsa_generate_signature(
      sx_hash_fct_t algo_hash,
      block_t domain,
      block_t pub,
      block_t priv,
      block_t message,
      block_t signature,
      uint32_t size,
      uint32_t curve_flags,
      struct sx_rng rng);

/**
 * @brief Verifies a given signature corresponds to a given message
 * @param  algo_hash   hash function to use (see ::sx_hash_fct_t)
 * @param  domain      domain of the key
 * @param  pub         public key
 * @param  message     message to verify against signature
 * @param  signature   ECKCDSA signature to check
 * @param  size        size of one element (expressed in bytes)
 * @param  curve_flags curve acceleration parameters
 * @return CRYPTOLIB_SUCCESS if no error
 */
uint32_t eckcdsa_verify_signature(
      sx_hash_fct_t algo_hash,
      block_t domain,
      block_t pub,
      block_t message,
      block_t signature,
      uint32_t size,
      uint32_t curve_flags);

#endif
