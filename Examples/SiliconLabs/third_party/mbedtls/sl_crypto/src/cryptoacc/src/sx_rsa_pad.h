/**
 * @file
 * @brief Handles RSA padding
 *
 * These functions are implemented using the reference document from RSA
 * Laboratories:
 * PKCS #1 v2.2: RSA Cryptography Standard, sections 7, 8 and 9
 *
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */


#ifndef SX_RSA_PAD_H
#define SX_RSA_PAD_H

#include <stdint.h>
#include <stddef.h>
#include "compiler_extentions.h"
#include "sx_hash.h"
#include "sx_rng.h"

/**
 * Encode a message using OAEP
 * @param  k        RSA parameter length in bytes
 * @param  hashType The hash function to use (default is SHA1)
 * @param  EM       Pointer to the encoded message (EM) buffer
 * @param  message  Block containing address of the message to encode
 * @param  mLen     Length of the message to be encoded (bytes)
 * @param  rng      Random number generator to use
 * @return          CRYPTOLIB_SUCCESS when no error occurs
 */
uint32_t rsa_pad_eme_oaep_encode(uint32_t k, sx_hash_fct_t hashType,
      uint8_t *EM, block_t message, size_t mLen,
      struct sx_rng rng) CHECK_RESULT;

/**
 * Decode a message using OEAP
 * @param  k        RSA parameter length in bytes
 * @param  hashType the hash function to use (default is SHA1)
 * @param  EM       Pointer to the encoded message (EM) buffer
 * @param  message  Output: Pointer to the decoded message. The decoding is
 *                  done in place within the limits of the EM buffer.
 * @param  mLen     Output: Length of the decoded message
 * @return          CRYPTOLIB_SUCCESS when no error occurs
 */
uint32_t rsa_pad_eme_oaep_decode(uint32_t k, sx_hash_fct_t hashType,
      uint8_t *EM, uint8_t **message, size_t *mLen) CHECK_RESULT;

/**
 * Encode message using PKCS
 * @param  k        RSA parameter length in bytes
 * @param  EM       Pointer to the encoded message (EM) buffer
 * @param  message  Block containing address of the message to encode
 * @param  mLen     Len of the message to be encoded (bytes)
 * @param  rng      random number generator to use
 * @return          CRYPTOLIB_SUCCESS when no error occurs
 */
uint32_t rsa_pad_eme_pkcs_encode(uint32_t k, uint8_t *EM, block_t message,
      size_t mLen, struct sx_rng rng) CHECK_RESULT;

/**
 * Decodes encoded message using PKCS. message will point to the decoded message in EM
 * @param  k        RSA param length in bytes
 * @param  EM       Pointer to the encoded message (EM) buffer
 * @param  message  Output: Pointer to the decoded message in EM. The decoding
 *                  is done in place within the limits of the EM buffer.
 * @param  mLen     Output: Length of the decoded message (bytes)
 * @return          CRYPTOLIB_SUCCESS when no error occurs
 */
uint32_t rsa_pad_eme_pkcs_decode(uint32_t k, uint8_t *EM, uint8_t **message,
      size_t *mLen) CHECK_RESULT;

/**
 * Encode hash using PKCS
 * @param  emLen     Length of encoded message (EM) buffer (RSA parameter length
 *                   in bytes)
 * @param  hash_type Hash function used for hashing \p hash (also used for the
 *                   PKCS algorithm)
 * @param  EM        Output: Pointer to the encoded message buffer
 * @param  hash      Input: Hash to encode
 * @return           CRYPTOLIB_SUCCESS if no error occurs
 */
uint32_t rsa_pad_emsa_pkcs_encode(uint32_t emLen, sx_hash_fct_t hash_type,
      uint8_t *EM, uint8_t *hash) CHECK_RESULT;


/**
 * Encode hash using PSS. This function uses a salt length equal to the hash
 * digest length.
 * @param  emLen     Length of encoded message (EM) buffer (RSA parameter length
 *                   in bytes)
 * @param  hashType  Hash function used for hashing \p hash (also used for PKCS
 *                   algorithm)
 * @param  EM        Output: Pointer to the encoded message buffer
 * @param  hash      Input: Hash to encode
 * @param  n0        MSB of the modulus (for masking in order to match the
 *                   modulus size)
 * @param  sLen      Intended length of the salt
 * @param  rng       Random number generator to use
 * @return           CRYPTOLIB_SUCCESS if no error occurs
 */
uint32_t rsa_pad_emsa_pss_encode(uint32_t emLen, sx_hash_fct_t hashType,
      uint8_t *EM, uint8_t *hash, uint32_t n0, size_t sLen,
      struct sx_rng rng) CHECK_RESULT;

/**
 * Decode encoded message using PSS and compares hash to the decoded hash
 * @param  emLen     Length of encoded message (EM) buffer (RSA parameter length
 *                   in bytes)
 * @param  hashType  Hash function used for hasing \p hash
 * @param  EM        Input: Pointer to the encoded message buffer
 * @param  hash      Hash to compare with
 * @param  sLen      Intended length of the salt
 * @param  n0        MSB of the modulus (for masking in order to match the
 *                   modulus size)
 * @return           CRYPTOLIB_SUCCESS if no error occurs and hash is valid
 */
uint32_t rsa_pad_emsa_pss_decode(uint32_t emLen, sx_hash_fct_t hashType,
                                 uint8_t *EM, uint8_t *hash, uint32_t sLen,
                                 uint32_t n0) CHECK_RESULT;

/**
 * Pads the hash of \p hashLen to \p EM of \p emLen. MSBs are set to 0.
 *
 * \warning There should not be overlapping between \p EM and \p hash !
 * @param EM      Destination buffer (pointer)
 * @param emLen   Length of the destination buffer (bytes)
 * @param hash    Input to pad
 * @param hashLen Length of the input
 */
void rsa_pad_zeros(uint8_t *EM, size_t emLen, uint8_t *hash, size_t hashLen);

#endif
