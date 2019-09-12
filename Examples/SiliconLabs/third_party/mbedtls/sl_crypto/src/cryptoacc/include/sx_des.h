/**
 * @brief Defines the procedures to make operations with the BA412 DES function
 * @copyright Copyright (c) 2017-2018 Silex Insight. All Rights reserved
 * @file
 */

#ifndef SX_DES_H
#define SX_DES_H

#include <stdint.h>
#include "compiler_extentions.h"
#include "cryptolib_types.h"

#include "sx_aes.h"

#define DES_BLOCK_SIZE        8
/** @brief BA412 Size for IV */
#define DES_IV_SIZE           DES_BLOCK_SIZE

/**
 * @brief Triple DES (3DES) generic function
 *        Encrypt or decrypt using 3DES \p input to \p output,
 *        using \p fct mode of operation.
 *
 * \note As DES is considered unsafe, only 3DES is supported
 *
 * @param fct cipher block function to use.
 *            Only ::ECB, ::CBC and ::CBCMAC from ::sx_aes_fct_t are supported.
 * @param dir encrypt or decrypt. See ::sx_aes_mode_t.
 * @param key DES secret key.
 * @param iv  initialization vector. Used for \p fct ::CBC.
 * @param data_in input data (plaintext or ciphertext).
 * @param data_out output data (ciphertext or plaintext).
 *
 * @return CRYPTOLIB_SUCCESS when execution was successful
 */
uint32_t sx_des_blk(sx_aes_fct_t fct,
               sx_aes_mode_t dir,
               block_t key,
               block_t iv,
               block_t data_in,
               block_t data_out) CHECK_RESULT;

#endif //SX_DES_H
