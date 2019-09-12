/**
 * @file
 * @brief Defines RSA algorithmic functions
 *
 * Reference: Public-Key Cryptography Standards (PKCS) #1: RSA Cryptography
 * Specifications Version 2.1 (https://tools.ietf.org/html/rfc3447)
 *
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */


#ifndef SX_RSA_ALG_H
#define SX_RSA_ALG_H


#include <stdint.h>
#include "compiler_extentions.h"
#include "cryptolib_def.h"
#include "sx_hash.h"
#include "sx_rng.h"

/** @brief Enumeration of possible padding types */
typedef enum rsa_pad_types {
   ESEC_RSA_PADDING_NONE      = 0x0,/**< No padding */
   ESEC_RSA_PADDING_OAEP      = 0x1,/**< Optimal Asymmetric Encryption Padding */
   ESEC_RSA_PADDING_EME_PKCS  = 0x2,/**< EME-PKCS padding */
   ESEC_RSA_PADDING_EMSA_PKCS = 0x3,/**< EMSA-PKCS padding */
   ESEC_RSA_PADDING_PSS       = 0x4 /**< EMSA-PSS padding */
} rsa_pad_types;

/* XXX Hash/salt presence is directly dependant of the padding type.
 * Is it possible to abstract these both field ? For instance:
 * struct rsa_variant {
 *    rsa_pad_types padding;
 *    sx_hash_fct_t hash;
 *    size_t salt_len;
 * };
 *
 * it may really ease the logic of handling padding/hash with a nice functions array:
 * handle_padding[rsa_variant->padding](message, n, rsa_variant);
 */

/**
 * @brief Generate the modulus, the private key and the lambda for a given (P, Q)
 * @param p Big integer prime P
 * @param q Big integer prime Q
 * @param exponent Exponent of the RSA (also public key)
 * @param n Ouptut big integer modulus
 * @param private_key Output private key associated to the \c exponent
 * @param lambda Output lcm(P-1, Q-1), used when countermeasures are enabled
 *               May also be NULL_blk when not expected by caller
 * @param size Size of a RSA element (expressed in bytes)
 *
 * @return ::CRYPTOLIB_SUCCESS if able to compute the different elements
 *         ::BA414EP_STS_NINV_MASK if the public exponent is non-invertible
 */
uint32_t rsa_generate_private_key_from_p_q(
      const block_t p,
      const block_t q,
      const block_t exponent,
      block_t n,
      block_t private_key,
      block_t lambda,
      const uint32_t size);

/**
 * @brief Generate the dp, dq and q_inv for a given (P, Q, d) when the Chinese
 *        Reminder Theorem (CRT) is used
 * @param p Big integer prime P
 * @param q Big integer prime Q
 * @param d Private key (ie: generated through ::rsa_generate_private_key_from_p_q)
 * @param dp Output big integer equals to \f[ d mod P-1 \f]
 * @param dq Output big integer equals to \f[ d mod Q-1 \f]
 * @param q_inv Output first CRT coefficient
 * @param size Size of a RSA element (expressed in bytes)
 *
 * @return ::CRYPTOLIB_SUCCESS if able to compute the different elements
 *         ::CRYPTOLIB_UNSUPPORTED_ERR if size is bigger than RSA_MAX_SIZE
 */
uint32_t rsa_generate_crt_private_key_from_p_q(
      const block_t p,
      const block_t q,
      const block_t d,
      block_t dp,
      block_t dq,
      block_t q_inv,
      const uint32_t size);

/**
 * @brief Generate all the elements of a RSA key (without CRT) for a given
 *        (exponent, size)
 * @param exponent Input public exponent (public key of RSA key pair)
 * @param modulus Ouptut modulus involved in RSA operation
 * @param d Output private key, generated for the specified \c exponent
 * @param lambda Ouptut lcm(P-1, Q-1), usefull when counter-measures are used.
 *                 May be NULL_blk if not requested (must be kept secret)
 * @param size Size of a RSA element (expressed in bytes)
 * @param rng The random number generator to use
 *
 * @return ::CRYPTOLIB_SUCCESS if able to compute the different elements
 *         ::CRYPTOLIB_UNSUPPORTED_ERR if size is bigger than RSA_MAX_SIZE
 */
uint32_t rsa_generate_private_key(
      const block_t exponent,
      block_t modulus,
      block_t d,
      block_t lambda,
      const size_t size,
      struct sx_rng rng);

/**
 * @brief Generate all the elements of a RSA key (with CRT) for a given
 *        (exponent, size)
 * @param exponent Input exponent and also public key of a RSA key pair
 * @param modulus Ouptut modulus involved in RSA operation
 * @param p Big integer prime P (must be kept secret)
 * @param q Big integer prime Q (must be kept secret)
 * @param dp Output big integer equals to \f[ d mod P-1 \f] (part of secret key)
 * @param dq Output big integer equals to \f[ d mod Q-1 \f] (part of secret key)
 * @param q_inv First CRT coefficient (must be kept secret)
 * @param size Size of a RSA element (expressed in bytes)
 * @param rng The random number generator to use
 *
 * @return ::CRYPTOLIB_SUCCESS if able to compute the different elements
 *         ::CRYPTOLIB_UNSUPPORTED_ERR if size is bigger than RSA_MAX_SIZE
 */
uint32_t rsa_generate_crt_private_key(
      const block_t exponent,
      block_t modulus,
      block_t p,
      block_t q,
      block_t dp,
      block_t dq,
      block_t q_inv,
      const size_t size,
      struct sx_rng rng);

/**
 * @brief RSA encryption
 * @param padding Padding to use to encode message
 * @param plaintext Plaintext message to be encrypted
 * @param n RSA modulus
 * @param exponent Public exponent involved in RSA encryption
 * @param ciphertext Output ciphertext, its size must be equal to size of n.
 * @param hash Hash algorithm used when OAEP padding is issued
 * @param rng The random number generator to use
 * @return ::CRYPTOLIB_SUCCESS if successfully encrypted
 *         ::CRYPTOLIB_UNSUPPORTED_ERR if size is bigger than RSA_MAX_SIZE
 */
uint32_t rsa_encrypt(
      const rsa_pad_types padding,
      const block_t plaintext,
      const block_t n,
      const block_t exponent,
      block_t ciphertext,
      const sx_hash_fct_t hash,
      struct sx_rng rng) CHECK_RESULT;

/**
 * @brief RSA decryption (without CRT)
 * @param padding Padding to use to decrypt the cipher
 * @param ciphertext Cipher message to be decrypted
 * @param n RSA modulus
 * @param private_key Private key involved in RSA decryption
 * @param lambda Lcm(P-1, Q-1), useful when counter-measures are used.
 *               May be NULL_blk if not requested.
 * @param plaintext Plaintext message, it must be set to accept enough data
 *              before calling ::rsa_decrypt. Length field will be updated.
 * @param hash Hash algorithm used when OAEP padding is issued
 * @return ::CRYPTOLIB_SUCCESS if successfully decrypted
 *         ::CRYPTOLIB_UNSUPPORTED_ERR if size is bigger than RSA_MAX_SIZE
 */
uint32_t rsa_decrypt(
      const rsa_pad_types padding,
      const block_t ciphertext,
      const block_t n,
      const block_t private_key,
      const block_t lambda,
      block_t *plaintext,
      const sx_hash_fct_t hash) CHECK_RESULT;

/**
 * @brief RSA decryption (with CRT)
 * @param padding Padding to use to decrypt the cipher
 * @param ciphertext Ciphertext message to be decrypted
 * @param n RSA modulus
 * @param p Big integer prime P
 * @param q Big integer prime Q
 * @param dp Big integer equals to \f[ d mod P-1 \f]
 * @param dq Big integer equals to \f[ d mod Q-1 \f]
 * @param q_inv First CRT coefficient
 * @param plaintext Plaintext message, it must be set to accept enough data
 *              before calling ::rsa_decrypt and the length field will be updated.
 * @param hash Hash algorithm used when OAEP padding is issued
 * @return ::CRYPTOLIB_SUCCESS if successfully decrypted
 *         ::CRYPTOLIB_UNSUPPORTED_ERR if size is bigger than RSA_MAX_SIZE
 */
uint32_t rsa_decrypt_crt(
      const rsa_pad_types padding,
      const block_t ciphertext,
      const block_t n,
      const block_t p,
      const block_t q,
      const block_t dp,
      const block_t dq,
      const block_t q_inv,
      block_t *plaintext,
      const sx_hash_fct_t hash) CHECK_RESULT;

/**
 * @brief RSA signature generation (without CRT)
 * @param padding Padding to use to sign the message
 * @param salt_length Expected length of the salt used when issuing PSS padding
 * @param message The message to sign
 * @param n RSA modulus
 * @param private_key RSA private key
 * @param lambda Lcm(P-1, Q-1), useful when counter-measures are used.
 *               May be NULL_blk if not requested.
 * @param signature Resulting signature
 * @param hash Hash algorithm used when generating signature
 * @param rng The random number generator to use
 * @return ::CRYPTOLIB_SUCCESS if successfully decrypted
 *         ::CRYPTOLIB_UNSUPPORTED_ERR if size is bigger than RSA_MAX_SIZE
 */
uint32_t rsa_generate_signature(
      const rsa_pad_types padding,
      const size_t salt_length,
      const block_t message,
      const block_t n,
      const block_t private_key,
      const block_t lambda,
      block_t signature,
      const sx_hash_fct_t hash,
      struct sx_rng rng) CHECK_RESULT;

/**
 * @brief RSA signature generation (with CRT)
 * @param padding Padding to use to sign the message
 * @param salt_length Expected length of the salt used when issuing PSS padding
 * @param message The message to sign
 * @param n RSA modulus
 * @param p Big integer prime P
 * @param q Big integer prime Q
 * @param dp Big integer equals to \f[ d mod P-1 \f]
 * @param dq Big integer equals to \f[ d mod Q-1 \f]
 * @param q_inv First CRT coefficient
 * @param signature Resulting signature
 * @param hash Hash algorithm used when generating signature
 * @param rng The random number generator to use
 * @return ::CRYPTOLIB_SUCCESS if successfully decrypted
 *         ::CRYPTOLIB_UNSUPPORTED_ERR if size is bigger than RSA_MAX_SIZE
 */
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
      struct sx_rng rng) CHECK_RESULT;

/**
 * @brief RSA Signature verification
 * @param padding Padding to use when verifying the message
 * @param salt_length Expected length of the salt used when issuing PSS padding
 * @param message The message of signature to verify
 * @param n RSA modulus
 * @param exponent RSA public exponent
 * @param signature Signature to verify
 * @param hash Hash algorithm used when generating signature
 * @return ::CRYPTOLIB_SUCCESS when signature successfully verified against message
 *         ::CRYPTOLIB_INVALID_SIGN_ERR when  signature does not match message
 *         ::CRYPTOLIB_UNSUPPORTED_ERR if size is bigger than RSA_MAX_SIZE
 */
uint32_t rsa_verify_signature(
      const rsa_pad_types padding,
      const size_t salt_length,
      const block_t message,
      const block_t n,
      const block_t exponent,
      const block_t signature,
      const sx_hash_fct_t hash) CHECK_RESULT;

#endif
