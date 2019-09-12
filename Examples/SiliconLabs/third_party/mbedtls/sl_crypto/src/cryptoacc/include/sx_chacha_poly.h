/**
 * @file
 * @brief Defines the procedures to make operations with
 *        the BA417 ChachaPoly function
 *        See https://tools.ietf.org/html/rfc7539
 *
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */


#ifndef SX_CHAPOL_H
#define SX_CHAPOL_H

#include <stdint.h>
#include "compiler_extentions.h"
#include "cryptolib_types.h"

/** @brief Size for ChaCha Nonce (expressed in bytes) */
#define SX_CHACHA_NONCE_SIZE     12

/** @brief Size for ChaCha Key (expressed in bytes) */
#define SX_CHACHA_POLY_KEY_SIZE 32

/** @brief Size for IV (expressed in bytes) */
#define SX_CHACHA_IV_SIZE 4

/** @brief Size for the tag (expressed in bytes) */
#define SX_POLY_TAG_SIZE 16


/**
* @brief Enumeration of possible algorithm for ChaCha and Poly.
*/
typedef enum sx_chacha_poly_algo
{
    SX_CHACHA_POLY = 0,    /**< ChaCha20 and Poly1305 (AEAD)*/
    SX_CHACHA_ONLY = 1,    /**< ChaCha20 (encryption/decryption only)*/
    SX_POLY_CHACHAKEY = 2, /**< Poly1305 using \p key as input for ChaCha20
                                to generate the \e key_r and \e key_s internally */
    SX_POLY_ONLY = 3,     /**< Poly1305 using \p key as the concatenation
                               of \e key_r and \e key_s */
} sx_chacha_poly_algo;


/**
* @brief Enumeration of possible operations for ChaCha algorithm.
*/
typedef enum sx_chacha_op_e
{
    CHAENC = 1,  /**< Encrypt and/or generate tag when Poly1305 is enabled */
    CHADEC = 2   /**< Decrypt and/or verify tag when Poly1305 is enabled */
} sx_chacha_op_t;



/**
 * @brief ChaCha20 Poly1305 (see RFC 7539) generic function to handle any ChaCha and Poly
 * valid combination
 *
 * @param algo  Algorithm used, see ::sx_chacha_poly_algo
 * @param op    encrypt or decrypt. See ::sx_chacha_op_t
 * @param din   block_t of input data (plaintext or ciphertext).
 *              Size of din must be 1 byte minimum.
 * @param dout  block_t of output data (ciphertext or plaintext).
 *              Not used for ::SX_POLY_CHACHAKEY and ::SX_POLY_ONLY
 * @param key   block_t of the ChaCha key (32 bytes expected)
 * @param iv    block_t of initialization vector (4 bytes will be read)
 * @param nonce block_t to the nonce to use for encryption/decryption
 * @param aad   block_t of additional authenticated data
 * @param tag   block_t of the authentication tag (16 bytes will be written).
 *              Not used for ::SX_CHACHA_POLY
 *
 * @return CRYPTOLIB_SUCCESS when execution was successful, CRYPTOLIB_INVALID_SIGN_ERR when computed tag does not match input tag (i.e. message corruption)
 */
uint32_t sx_chacha_poly(sx_chacha_poly_algo algo,
                        sx_chacha_op_t op,
                        block_t din,
                        block_t dout,
                        block_t key,
                        block_t iv,
                        block_t nonce,
                        block_t aad,
                        block_t tag
                        ) CHECK_RESULT;
#endif
