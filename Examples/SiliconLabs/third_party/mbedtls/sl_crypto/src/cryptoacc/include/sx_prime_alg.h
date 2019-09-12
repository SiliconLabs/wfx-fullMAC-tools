/**
 * @file
 * @brief Defines the procedures for prime generation
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */


#ifndef SX_PRIME_ALG_H
#define SX_PRIME_ALG_H

#include <stdint.h>
#include <stdbool.h>
#include "compiler_extentions.h"
#include "cryptolib_def.h"
#include "sx_rng.h"
#include "sx_errors.h"

/**
 * @brief Converge from a given number to a prime number using the Miller-Rabin
 * algorithm
 * @details This algorithm tries to converge to a prime number from a given
 * random number. First it tries to find a small divider, then a larger divider.
 * If neither is found, the Miller-Rabin algorithm is applied. If this also
 * confirms the number is probably prime, the number is assumed prime. If the
 * given number is not prime, it is incremented (in steps of 2) and checked again.
 * The procedure is repeated until a prime is found.
 * @param number pointer to a given random number
 * @param nr_of_bytes byte size of \p number
 * @param mr_rounds number of rounds for the Miller-Rabin algorithm
 * @param rng random number generator to use with the Miller-Rabin algorithm
 * @return cryptolib_status with \c status:
 *         ::CRYPTOLIB_SUCCESS when no errors has occurred during the calculation
 */
uint32_t converge_to_prime(uint8_t *number, uint32_t nr_of_bytes,
                           uint32_t mr_rounds, struct sx_rng rng);

/**
 * @brief Generate a prime starting from a random number using the Miller-Rabin
 * algorithm
 * @param prime pointer to the resulting prime
 * @param bitsize the size of the prime in bits
 * @param mr_rounds number of rounds for the Miller-Rabin algorithm
 * @param rng random number generator to be used by the Miller-Rabin algorithm
 * @return CRYPTOLIB_SUCCESS if no errors occurred
 */
uint32_t generate_prime(uint8_t *prime, uint32_t bitsize, uint32_t mr_rounds,
                        struct sx_rng rng) CHECK_RESULT;

/**
 * @brief Test primality of a given number using the Miller-Rabin algorithm
 * @param number pointer to the number to check
 * @param nr_of_bytes byte size of \p number
 * @param mr_rounds number of rounds for the Miller-Rabin algorithm
 * @param is_probably_prime true if \p number is probably prime
 * @param rng random number generator to be used by the Miller-Rabin algorithm
 * @return cryptolib_status with \c status:
 *         ::CRYPTOLIB_SUCCESS when \c is_prime can determine the primality
 */

uint32_t is_prime(uint8_t *number, uint32_t nr_of_bytes, uint32_t mr_rounds,
                  bool *is_probably_prime, struct sx_rng rng);

#endif
