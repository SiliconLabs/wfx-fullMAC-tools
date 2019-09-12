/**
 * @file
 * @brief Miller-Rabin primality test function
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */


#ifndef MR_H
#define MR_H

#include <stdint.h>
#include <stdbool.h>
#include "compiler_extentions.h"
#include "sx_rng.h"
#include "sx_errors.h"

/**
 * @brief perform Miller-Rabin primality test
 * @param number number that is a prime candidate
 * @param size byte size of \p number \pre should be <= PRIME_MAX_SIZE
 * @param mr_rounds number of rounds to be used for Miller-Rabin test
 * @param is_probably_prime true if \p number is probably prime
 * @param rng random number generator to use
 * @return cryptolib_status with \c status:
 *        \li ::CRYPTOLIB_SUCCESS when the function was able to determine
 *         the primality of \c number
 *        \li Other value if the result can not be used
 */
uint32_t miller_rabin(uint8_t *number, uint32_t size, uint32_t mr_rounds,
                      bool *is_probably_prime, struct sx_rng rng);
#endif
