/**
 * @file
 * @brief Header for key generation DH MODP
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */


#ifndef SX_DH_KEYGEN_ALG
#define SX_DH_KEYGEN_ALG

#include <stdint.h>
#include "compiler_extentions.h"
#include "cryptolib_types.h"
#include "sx_rng.h"

/**
 * @brief  Generates Diffie Hellman private key
 * @param  mod Modulus part of a Diffie Hellman key
 * @param  key Output Diffie Hellman private key
 * @param  rng Used random number generator
 * @return CRYPTOLIB_SUCCESS if no error
 */
uint32_t dh_genkey(block_t mod, block_t key, struct sx_rng rng) CHECK_RESULT;

#endif
