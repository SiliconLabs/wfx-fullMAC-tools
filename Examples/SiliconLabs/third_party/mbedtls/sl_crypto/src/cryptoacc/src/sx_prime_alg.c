/**
 * @file
 * @brief Implements the procedures for prime generation
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */


#include "sx_prime_alg.h"
#include <string.h>
#include "sx_memcpy.h"
#include "sx_primitives.h"
#include "sx_mr.h"
#include "sx_math.h"
#include "sx_errors.h"
#include "ba414ep_config.h"

#if (PRIME_GEN_ENABLED)

static const ALIGNED uint8_t  first_odd_primes[8] = {3, 5, 7, 11, 13, 17, 19, 23};
static const         uint32_t prod_first_8_odd_primes = 111546435;
static const ALIGNED uint8_t  prod_first_130_odd_primes[128] =
                              {0x0a, 0x89, 0x99, 0x92, 0x50, 0x8f, 0xd5, 0x92,
                               0xb9, 0xda, 0xfd, 0x74, 0xdd, 0x54, 0x3e, 0xf7,
                               0x6e, 0x2d, 0x9f, 0x50, 0x33, 0x68, 0x19, 0xf2,
                               0x28, 0x33, 0xfb, 0x3b, 0x07, 0xb1, 0xf0, 0xc9,
                               0x0b, 0x3e, 0x4f, 0x1a, 0xcd, 0x73, 0x80, 0x18,
                               0xf9, 0x15, 0x42, 0x07, 0x0a, 0x92, 0x99, 0x8f,
                               0x1d, 0xd8, 0x70, 0x32, 0xb7, 0x05, 0x16, 0x70,
                               0xac, 0x8c, 0xfa, 0x03, 0x33, 0x56, 0x52, 0x24,
                               0x0c, 0x3e, 0xae, 0x8e, 0x5c, 0x8e, 0x31, 0xf6,
                               0x91, 0x04, 0xd3, 0xef, 0xc3, 0xe0, 0xcd, 0x17,
                               0x08, 0xcf, 0xbc, 0x23, 0x00, 0x15, 0xa4, 0x65,
                               0xfc, 0x0f, 0xc8, 0xb6, 0xb0, 0xf3, 0x61, 0xe0,
                               0x16, 0x07, 0xfb, 0x81, 0xd3, 0x86, 0x57, 0x78,
                               0xff, 0x11, 0xf0, 0xb5, 0x3d, 0x56, 0x03, 0x74,
                               0xc7, 0xdf, 0x5d, 0xe0, 0x46, 0x94, 0x12, 0x32,
                               0xbd, 0xa4, 0xb5, 0x47, 0x06, 0x39, 0xd7, 0x87};

/**
 * Compute the modulo of a number when divided by the product of the first 8 odd primes
 * @param number number to divide
 * @param nr_of_bytes byte size of \p number \pre should be <= PRIME_MAX_SIZE
 * @param mod modulo that was computed
 * @return CRYPTOLIB_SUCCESS if successful
 */
static uint32_t get_prime_modulo(uint8_t *number,
                                 uint32_t nr_of_bytes,
                                 uint32_t *mod)
{
   uint8_t tmp[PRIME_MAX_SIZE];

   if (nr_of_bytes > PRIME_MAX_SIZE)
      return CRYPTOLIB_INVALID_PARAM;

   memset(tmp, 0, nr_of_bytes - sizeof(prod_first_8_odd_primes));
   tmp[nr_of_bytes - 4] = (prod_first_8_odd_primes & 0xFF000000) >> 24;
   tmp[nr_of_bytes - 3] = (prod_first_8_odd_primes & 0x00FF0000) >> 16;
   tmp[nr_of_bytes - 2] = (prod_first_8_odd_primes & 0x0000FF00) >> 8;
   tmp[nr_of_bytes - 1] = (prod_first_8_odd_primes & 0x000000FF);

   uint32_t status = modular_reduction(number, tmp, tmp, nr_of_bytes);
   if (status)
      return status;

   *mod = (tmp[nr_of_bytes - 4] << 24) | (tmp[nr_of_bytes - 3] << 16) |
          (tmp[nr_of_bytes - 2] << 8 ) | (tmp[nr_of_bytes - 1]);

   return status;
}

/**
 * Check if a number is co-prime with the product of the first 130 odd prime numbers
 * @param number number to check
 * @param nr_of_bytes byte size of \p number \pre should be <= PRIME_MAX_SIZE
 * @param coprime true if co-prime
 * @return CRYPTOLIB_SUCCESS if successful
 */
static uint32_t is_coprime(uint8_t *number, uint32_t nr_of_bytes, bool *coprime)
{
   uint8_t tmp[PRIME_MAX_SIZE]; // working buffer for intermediate steps
   const size_t prodsize = sizeof(prod_first_130_odd_primes);
   uint32_t status = 0;

   if (nr_of_bytes > PRIME_MAX_SIZE)
      return CRYPTOLIB_INVALID_PARAM;

   /* All inputs to a PK primitive operation (modular reduction, inversion...)
    * should have the same byte size.
    */

   /* If the number is shorter than 1024 bits, pad it with leading zeros before
    * proceeding to the modular inversion that checks co-primality.
    */
   if (nr_of_bytes < prodsize) {
      memset(tmp, 0, prodsize - nr_of_bytes);
      memcpy_array(tmp + prodsize - nr_of_bytes, number, nr_of_bytes);
      number = tmp;
   }
   /* If the number is larger than 1024 bits, reduce it to its modulo when
    * divided by prod_first_130_odd_primes. Rather than padding the product,
    * we reduce the number itself. This is done for performance: doing a modular
    * reduction + modular inversion on 1024 bits is faster than a modular
    * inversion on larger operands.
    */
   else if (nr_of_bytes > prodsize) {
      memset(tmp, 0, nr_of_bytes - prodsize);
      memcpy_array(tmp + nr_of_bytes - prodsize, prod_first_130_odd_primes,
                   prodsize);
      status = modular_reduction(number, tmp, tmp, nr_of_bytes);
      if (status)
         return CRYPTOLIB_CRYPTO_ERR;
      number = tmp + nr_of_bytes - prodsize;
   }

   /* Check that the greatest common divisor between the number and
    * prod_first_130_odd_primes is 1. */
   status = modular_inversion(number, (uint8_t*)prod_first_130_odd_primes,
                              tmp, prodsize);
   *coprime = (status ==  CRYPTOLIB_SUCCESS);

   return CRYPTOLIB_SUCCESS;
}

uint32_t is_prime(uint8_t *number, uint32_t nr_of_bytes, uint32_t mr_rounds,
                  bool *is_probably_prime, struct sx_rng rng)
{
   uint32_t status = miller_rabin(number, nr_of_bytes, mr_rounds,
                                  is_probably_prime, rng);
   return status;
}

uint32_t converge_to_prime(uint8_t *number, uint32_t nr_of_bytes,
                           uint32_t mr_rounds, struct sx_rng rng)
{
   uint32_t mod = 0;
   uint8_t incr = 0;

   /* Make the number odd */
   *(number + nr_of_bytes - 1) |= 0x01;

   /* Compute the number's modulo when divided by the product of the first 8 odd
    * prime numbers (on 32 bits)
    */
   uint32_t status = get_prime_modulo(number, nr_of_bytes, &mod);
   if (status)
      return status;

    /* Test candidates for primality until a prime is found
     * candidate = number + incr (number is odd)
     * mod = candidate % prod_first_8_odd_primes
     */
   while(1) {
      bool possibly_prime = true;

      /* Step 1:
       * Sieve based on the modulo: check if it is a multiple of 1 of the first
       * 8 odd prime numbers. If that is the case the number can't be prime.
       */
      for (int i = 0; i < sizeof(first_odd_primes); i++) {
         if ((mod % first_odd_primes[i]) == 0) {
            possibly_prime = false;
            break;
         }
      }

      if(possibly_prime) {
         /* Add a small increment (incr) to the number (constant time) with
          * incr <=40 (because we will always find a number that is not a
          * multiple of the 8 first primes within 20 tries - empirically tested)
          */
         sx_math_array_incr(number, nr_of_bytes, incr);
         incr = 0;

         /* Step 2:
          * Check coprimality between the number and the product of the first
          * 130 odd prime numbers (on 1024 bits)
          */
         bool coprime = false;
         status = is_coprime(number, nr_of_bytes, &coprime);
         if (status)
            return status;

         if (!coprime)
            possibly_prime = false;
      }

      if(possibly_prime) {
         /* Step 3:
          * Perform Miller-Rabin primality test on the number
          */
         bool prime = false;
         status = miller_rabin(number, nr_of_bytes, mr_rounds, &prime, rng);
         if (status)
            return status;

         /* If the number is prime, stop searching */
         if (prime) {
            break;
         }
      }

      /* If the number is not prime, try again, incrementing the number and the
       * corresponding modulo by 2.
       */
      incr += 2;
      mod += 2;
      if (mod >= prod_first_8_odd_primes) {
         mod -= prod_first_8_odd_primes;
      }
   }

   return status;
}

uint32_t generate_prime(uint8_t *prime, uint32_t bitsize, uint32_t mr_rounds,
                        struct sx_rng rng)
{
   uint32_t byte_length = (bitsize + 7) / 8;
   uint32_t unaligned_bits = bitsize % 8;

   rng.get_rand_blk(rng.param, block_t_convert(prime, byte_length));
   /* Force most significant bit to always have a random starting point of
    * maximal length. Shift right to reduce the bit size in case the requested
    * prime is not aligned on 8 bit.
    */
   prime[0] |= 0x80;
   if(unaligned_bits)
        prime[0] >>= 8 - unaligned_bits;

   uint32_t status = converge_to_prime(prime, byte_length, mr_rounds, rng);
   return status;
}

#endif
