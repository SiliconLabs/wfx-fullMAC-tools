/**
 * @file
 * @brief Regroup functions of the BA414EP which are not directly cryptographic
 *        (like encryption, signature, ...)
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */

#ifndef SX_PK_INTERFACE_H
#define SX_PK_INTERFACE_H

#include <stdint.h>
#include "compiler_extentions.h"
#include "sx_rng.h"
/**
 * @brief Check BA414EP ROM integrity
 * @return CRYPTOLIB_SUCCESS when no error
 */
uint32_t sx_pk_ucode_integrity_check(void) CHECK_RESULT;

/**
 * @brief Set the random number generator to be used for the PK countermeasures.
 * @param rng a is a ::sx_rng structure which can generate random outputs by calling
 *              ::sx_rng.get_rand_blk
 */
void sx_pk_set_rng(struct sx_rng rng);

/**
 * @brief Run a PK routine to clear the whole BA414EP RAM memory
 * @return CRYPTOLIB_SUCCESS if no error
 */
uint32_t sx_pk_clear_memory();

#endif /* SX_PK_INTERFACE_H */

