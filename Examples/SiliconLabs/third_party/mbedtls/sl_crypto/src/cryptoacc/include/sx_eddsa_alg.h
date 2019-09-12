/**
 * @file
 * @brief Defines the procedures to make EdDSA operations with the BA414EP
 * public key engine
 * Supported instances/curves: Ed25519, Ed448.
 * Reference: https://tools.ietf.org/html/rfc8032
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */

#ifndef SX_EDDSA_ALG_H
#define SX_EDDSA_ALG_H

#include <stdint.h>
#include "compiler_extentions.h"
#include "cryptolib_types.h"
#include "sx_rng.h"

/** Curve to be used by the EdDSA algorithms */
typedef enum sx_eddsa_curve {
   SX_ED25519, /**< Ed25519 curve */
   SX_ED448    /**< Ed448 curve */
} sx_eddsa_curve_t;

/** @brief Ed448 max context size in bytes */
#define EDDSA_MAX_CONTEXT_SIZE 255

/** @brief EdDSA key size in bytes */
#define ED25519_KEY_SIZE      32
#define ED448_KEY_SIZE        57
#define EDDSA_MAX_KEY_SIZE    ED448_KEY_SIZE

/** @brief EdDSA signature size in bytes */
#define ED25519_SIGN_SIZE     (2 * (ED25519_KEY_SIZE))
#define ED448_SIGN_SIZE       (2 * (ED448_KEY_SIZE))
#define EDDSA_MAX_SIGN_SIZE  ED448_SIGN_SIZE

/**
 * @brief Generates an EdDSA key pair (public and private)
 * @param[in]  curve Curve used by the EdDSA algorithm
 * @param[out] pk    Public key to be generated
 * @param[out] sk    Private key to be generated
 * @param[in]  rng   Used random number generator
 * @return CRYPTOLIB_SUCCESS if no error
 */
uint32_t eddsa_generate_keypair(
      enum sx_eddsa_curve curve,
      block_t pk,
      block_t sk,
      struct sx_rng rng) CHECK_RESULT;

/**
 * @brief Generates an EdDSA ECC public key based on a given private key.
 * @param[in]  curve Curve used by the EdDSA algorithm
 * @param[in]  sk    Private key for which public key is desired
 * @param[out] pk    Public key to be generated
 * @return  CRYPTOLIB_SUCCESS if no error
 */
uint32_t eddsa_generate_pubkey(
      enum sx_eddsa_curve curve,
      block_t sk,
      block_t pk) CHECK_RESULT;

/**
 * @brief Generates an EdDSA ECC private key.
 * @param[in]  curve Curve used by the EdDSA algorithm
 * @param[out] sk    Private key to be generated
 * @param[in]  rng   Used random number generator
 * @return  CRYPTOLIB_SUCCESS if no error
 */

uint32_t eddsa_generate_private_key(
      enum sx_eddsa_curve curve,
      block_t sk,
      struct sx_rng rng);

/**
 * @brief Generate an EdDSA signature of a message
 * @param[in]  curve     Curve used by the EdDSA algorithm
 * @param[in]  message   Message to sign
 * @param[in]  pub_key   EdDSA public key
 * @param[in]  priv_key  EdDSA private key
 * @param[in]  context   EdDSA context, maximum 255 bytes, NULL_blk for Ed25519
 * @param[out] signature EdDSA signature of message
 * @return  CRYPTOLIB_SUCCESS if no error
 */
uint32_t eddsa_generate_signature(
      enum sx_eddsa_curve curve,
      block_t message,
      block_t pub_key,
      block_t priv_key,
      block_t context,
      block_t signature) CHECK_RESULT;

/**
 * @brief Generate an EdDSA 25519 signature with counter measures of a message
 *
 * The problem and the countermeasure are described in the paper:
 * "Breaking Ed25519 in WolfSSL" (https://eprint.iacr.org/2017/985.pdf)
 * @param[in]  message   Message to sign
 * @param[in]  pub_key   EdDSA public key
 * @param[in]  priv_key  EdDSA private key
 * @param[out] signature EdDSA signature of message
 * @param[in]  rng       Random number generator to be used
 * @return  CRYPTOLIB_SUCCESS if no error
 */
uint32_t ed25519_generate_signature_with_cm(
      block_t message,
      block_t pub_key,
      block_t priv_key,
      block_t signature,
      struct sx_rng rng) CHECK_RESULT;

/**
 * @brief Verifies a given signature corresponds to a given message
 * @param[in] curve     Curve used by the EdDSA algorithm
 * @param[in] message   Message to verify against signature
 * @param[in] key       EdDSA public key
 * @param[in] context   EdDSA context, maximum 255 bytes, NULL_blk for Ed25519
 * @param[in] signature Signature to check
 * @return CRYPTOLIB_SUCCESS if successful
 *         CRYPTOLIB_INVALID_SIGN_ERR if signature verification failed
*/
uint32_t eddsa_verify_signature(
      enum sx_eddsa_curve curve,
      block_t message,
      block_t key,
      block_t context,
      block_t signature) CHECK_RESULT;

#endif
