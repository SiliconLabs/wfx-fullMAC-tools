/**
 * @file
 * @brief Defines the procedures to make operations with
 *          the BA411 AES function
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */


#ifndef SX_AES_H
#define SX_AES_H

#include <stdint.h>
#include "compiler_extentions.h"
#include "cryptolib_types.h"


/** @brief Size for IV in GCM mode */
#define AES_IV_GCM_SIZE       12
/** @brief Size for IV in all modes except GCM */
#define AES_IV_SIZE           16
/** @brief Size for Context in GCM and CCM modes */
#define AES_CTX_xCM_SIZE      32
/** @brief Size for Context in all modes except GCM and CCM */
#define AES_CTX_SIZE          16

#define AES_MAX_SIZE (256/8)

/**
 * @brief Dummy variables to use hardware keys Key1 and Key2
 *
 * @note They are declared \c extern for internal reasons, user should \e not
 *       use them because they may disappear in future release.
 */
extern uint8_t aes_hw_key2;
extern uint8_t aes_hw_key1;

/**
 * @brief First Hardware Key (of 128bits)
 *
 * Two secret hardware keys may be wired directly into to AES module, preventing
 * the CPU to read them back. This block_t provides an abstraction to pass them
 * as input (user should not use them for anything else as input) in the same way
 * that user AES keys.
 */
#define AES_KEY1_128 block_t_convert(&aes_hw_key1, 128/8)
/** @brief First Hardware Key of 256b (for description, see ::AES_KEY1_128) */
#define AES_KEY1_256 block_t_convert(&aes_hw_key1, 256/8)
/** @brief Second Hardware Key of 128b (for description, see ::AES_KEY1_128) */
#define AES_KEY2_128 block_t_convert(&aes_hw_key2, 128/8)
/** @brief Second Hardware Key of 256b (for description, see ::AES_KEY1_128) */
#define AES_KEY2_256 block_t_convert(&aes_hw_key2, 256/8)

/**
* @brief Enumeration of possible mode of operation for AES algorithm.
*/
//TODO: replace with generic block cipher enum
typedef enum sx_aes_fct_e
{
    ECB  = 1,              /**< Electronic Codebook */
    CBC  = 2,              /**< Cipher Block Chaining */
    CTR  = 3,              /**< Counter Feedback */
    CFB  = 4,              /**< Cipher Feedback */
    OFB  = 5,              /**< Output Feedback */
    CCM  = 6,              /**< Counter with CBC-MAC Mode */
    GCM  = 7,              /**< Galois/Counter Mode */
    XTS  = 8,              /**< XEX-based tweaked-codebook mode with ciphertext stealing */
    CMAC  = 9,             /**< CMAC mode */
    CBCMAC = 10,           /**< CBC-MAC mode (only used by DES)*/
} sx_aes_fct_t;


/**
* @brief Enumeration of possible mode for AES algorithm.
*/
//TODO: replace with generic block cipher enum
typedef enum sx_aes_mode_e
{
    ENC = 1,            /**< Encrypt */
    DEC = 2,            /**< Decrypt but does not return any tag */
    DEC_WITH_TAG = 3    /**< Decrypt and return the output tag in authenticated Encryption modes */
} sx_aes_mode_t;


/**
* @brief Enumeration of possible context states for AES algorithm.
*/
typedef enum sx_aes_ctx_e
{
    CTX_WHOLE = 0,            /**< No context switching (whole message) */
    CTX_BEGIN = 1,            /**< Save context (operation is not final) */
    CTX_END = 2,              /**< Load context (operation is not initial) */
    CTX_MIDDLE = 3            /**< Save & load context (operation is not initial & not final) */
} sx_aes_ctx_t;


/**
 * @brief AES generic function to handle any AES mode of operation.
 *        Encrypt or decrypt \p input to \p output, using \p aes_fct mode of operation.
 *
 * This function will handle AES operation depending of the mode used, for both
 * encryption/decryption and authentication.
 * It also handles the different cases of context switching.
 *
 * @param aes_fct       mode of operation for AES. See ::sx_aes_fct_t
 * @param dir           encrypt or decrypt. See ::sx_aes_mode_t
 * @param ctx           current context status. See ::sx_aes_ctx_t
 * @param data_in       block_t to input data (plaintext or ciphertext)
 * @param data_out      block_t to output data (ciphertext or plaintext)
 * @param key           block_t to the AES key
 * @param xtskey        block_t to the XTS key used in ::XTS
 * @param iv            block_t to initialization vector (16 bytes expected).
 *                      \n Used for ::CBC, ::CTR, ::CFB, ::OFB, ::XTS and ::GCM
 * @param aad           block_t to additional authenticated data. Used for ::GCM
 * @param tag           block_t to the authentication tag (16 bytes expected).
 *                      Used for ::GCM, ::CCM and ::CMAC
 * @param ctx_ptr       block_t to the AES context state after current completion
 *                      (expected size depends of the current \p aes_fct)
 * @param nonce_len_blk block_t to
 *                      - \f$ len(aad) | len(ciphertext) \f$ in ::GCM
 *                      - a nonce in ::CCM
 *
 * @return CRYPTOLIB_SUCCESS when execution was successful,
 *         CRYPTOLIB_INVALID_SIGN_ERR when computed tag does
 *         not match input tag (i.e. message corruption)
 */
uint32_t sx_aes_blk( sx_aes_fct_t aes_fct,
               sx_aes_mode_t dir,
               sx_aes_ctx_t ctx,
               block_t key,
               block_t xtskey,
               block_t iv,
               block_t data_in,
               block_t data_out,
               block_t aad,
               block_t tag,
               block_t ctx_ptr,
               block_t nonce_len_blk) CHECK_RESULT;

/**
 * @brief Reload random used in the AES counter-measures.
 *
 * When enabled in HW, counter-measures are available for the AES
 * (See the Technical Report: "Secure and Efficient Masking of AES - A Mission
 * Impossible?", June 2004)
 *
 * \warning It is under the user responsibility to call it after system boot
 *          (not automatically called).
 *
 * @param value new random value used to reload counter-measures
 */
void sx_aes_load_mask(uint32_t value);

#endif
