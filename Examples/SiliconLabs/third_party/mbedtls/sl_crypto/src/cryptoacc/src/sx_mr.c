/**
 * @file
 * @brief Miller-Rabin primality test implementation
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */


#include "sx_mr.h"
#include "ba414ep_config.h"
#include "sx_errors.h"
#include "sx_math.h"

/** @brief Executes a single round of the Miller-Rabin primality test
 *  @param rand random number with 1 < rand < number-1
 *  @param number number that is a prime candidate
 *  @param size byte size of \p number
 *  @param is_probably_prime true if \p number is probably prime
 *  @return CRYPTOLIB_SUCCESS if no errors occurred
 */
static uint32_t miller_rabin_round(uint8_t *rand, uint8_t *number,
                                   uint32_t size, bool *is_probably_prime)
{
   /* Execute Miller-Rabin round in BA414EP */
   ba414ep_set_command(BA414EP_OPTYPE_MILLER_RABIN, size, BA414EP_BIGEND,
                       BA414EP_SELCUR_NO_ACCELERATOR);
   mem2CryptoRAM_rev(block_t_convert(rand, size), size, BA414EP_MEMLOC_6);
   mem2CryptoRAM_rev(block_t_convert(number, size), size, BA414EP_MEMLOC_0);

   uint32_t status = ba414ep_start_wait_status();
   if (status == BA414EP_STS_PRIM_MASK || status == BA414EP_STS_NNV_MASK) {
      *is_probably_prime = false;
      return CRYPTOLIB_SUCCESS;
   } else if (status) {
      return CRYPTOLIB_CRYPTO_ERR;
   }

   *is_probably_prime = true;
   return CRYPTOLIB_SUCCESS;
}

uint32_t miller_rabin(uint8_t *number, uint32_t size, uint32_t mr_rounds,
                      bool *is_probably_prime, struct sx_rng rng)
{
   if (size > PRIME_MAX_SIZE || size == 0 || mr_rounds == 0)
      return CRYPTOLIB_INVALID_PARAM;

   uint8_t rand[PRIME_MAX_SIZE];
   for (uint32_t i = 0; i < mr_rounds; i++) {
      /* Generate a random number with 1 < rand < number-1 */
      if (sx_math_array_incr(number, size, -2) < 0)
         return CRYPTOLIB_INVALID_PARAM;

      uint32_t status = sx_rng_get_rand_lt_n_blk( // 0 < rand < number-2
            block_t_convert(rand, size), block_t_convert(number, size), rng);
      if (status)
         return status;
      sx_math_array_incr(rand, size, 1); // 1 < rand < number-1
      sx_math_array_incr(number, size, 2); // restore prime candidate
      /* Execute Miller-Rabin round through BA414EP */
      status = miller_rabin_round(rand, number, size, is_probably_prime);
      if (status)
         return status;
      /* Return if the prime candidate is composite */
      if (*is_probably_prime == false)
         return CRYPTOLIB_SUCCESS;
   }

   return CRYPTOLIB_SUCCESS;
}
