/**
 * @file
 * @brief Declares the constants and functions to make operations
 *          with the BA418 hash function
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */


#ifndef SX_SHA3_H
#define SX_SHA3_H

#include "compiler_extentions.h"
#include <stddef.h> /* for size_t */
#include "cryptolib_types.h"


/** @brief Size of SHAKE128 digest in bytes */
#define SHAKE128_DIGESTSIZE 16
/** @brief Size of SHA3_224 digest in bytes */
#define SHA3_224_DIGESTSIZE 28
/** @brief Size of SHA3_256 digest in bytes */
#define SHA3_256_DIGESTSIZE 32
/** @brief Size of SHA3_384 digest in bytes */
#define SHA3_384_DIGESTSIZE 48
/** @brief Size of SHA3_512 digest in bytes */
#define SHA3_512_DIGESTSIZE 64
/** @brief Size of the biggest block in SHA3 in bytes */
#define SHA3_MAX_BLOCK_SIZE 168
/** @brief Size of the state in SHA3 in bytes */
#define SHA3_STATE_SIZE 200
/** @brief byte to be added at the beginning while padding in sha mode  */
#define SHA_MODE_PREFIX 0x1F
/** @brief byte to be added at the end while padding in sha mode  */
#define SHA_MODE_SUFFIX 0x80
/** @brief byte to be added at the end in sha mode if msg needs 1 byte padding */
#define SHA_MODE_COMPLEMENT 0x9F
/** @brief byte to be added at the beging while padding in shake mode  */
#define SHAKE_MODE_PREFIX 0x06
/** @brief byte to be added at the end while padding in shake mode  */
#define SHAKE_MODE_SUFFIX 0x80
/** @brief byte to be added at the end in shake mode if msg needs 1 byte padding */
#define SHAKE_MODE_COMPLEMENT 0x86
/** @brief Maximum number of elements in the array for sx_sha3_hash_array_blk() */
#define SHA3_MAX_BLOCK_ARRAY_ELEMS 10


/**
* @brief Enumeration of the supported SHA3 Hash capacities
*/
typedef enum sx_sha3_hash_fct_e
{
   e_SHA3_224 = 0x07,
   e_SHA3_256 = 0x08,
   e_SHA3_384 = 0x09,
   e_SHA3_512 = 0x0A,
   e_SHAKE128 = 0x01,
   e_SHAKE256 = 0x02
} sx_sha3_fct_t;

/**
 * @brief Get digest size in bytes for the given \p sha3_hash_fct
 * @param sha3_hash_fct hash function. See ::sx_sha3_fct_t.
 * @return digest size in bytes, or 0 if invalid \p sha3_hash_fct
 */
uint32_t sx_sha3_get_digest_size(sx_sha3_fct_t sha3_hash_fct) CHECK_RESULT;

/**
 * @brief Compute sha3 hash digest of the content of \p data_in and write the result in \p data_out.
 * @param sha3_hash_fct hash function to use. See ::sx_sha3_fct_t.
 * @param data_in array of input data to process
 * @param data_out output digest
 * @param digest_size size of the output in case of using SHAKE , ignored otherwise
 * @return ::CRYPTOLIB_SUCCESS if execution was successful
 */
uint32_t sx_sha3_hash_blk(sx_sha3_fct_t sha3_hash_fct, block_t data_in, block_t data_out, size_t digest_size) CHECK_RESULT;

/**
 * @brief Compute the sha3 intermediate state of the content of \p data_in and write the result in \p data_out.
 * @param sha3_hash_fct hash function to use. See ::sx_sha3_fct_t.
 * @param data_in array of input data to process
 * @param state_out output a state to be used afterward in update or finish, length is always SHA3_STATE_SIZE
 * @return ::CRYPTOLIB_SUCCESS if execution was successful
 */
uint32_t sx_sha3_hash_begin_blk(sx_sha3_fct_t sha3_hash_fct, block_t data_in, block_t state_out)CHECK_RESULT;
/**
 * @brief Load previous context in \p state and then compute the sha3 intermediate state of the content of \p data_in and write the result in \p data_out.
 * @param sha3_hash_fct hash function to use. See ::sx_sha3_fct_t.
 * @param data_in array of input data to process , input length should always be a multiple of the algorith capacity
 * @param state the context of a previous operation , length must always be equal to SHA3_STATE_SIZE
 * @param state_out output the updated intermediate state , length is always SHA3_STATE_SIZE
 * @return ::CRYPTOLIB_SUCCESS if execution was successful
 */
uint32_t sx_sha3_hash_update_blk(sx_sha3_fct_t sha3_hash_fct, block_t data_in, block_t state, block_t state_out)CHECK_RESULT;

/**
 * @brief Compute sha3 hash digest of the content of \p data_in using the state output of previous operations and write the result in \p data_out.
 * @param sha3_hash_fct hash function to use. See ::sx_sha3_fct_t.
 * @param data_in array of input data to process , data length can be any length as padding is done here
 * @param state the context of a previous operation , length must always be equal to SHA3_STATE_SIZE
 * @param data_out output hash digest
 * @param digest_size size of the output in case of using SHAKE , ignored otherwise
 * @return ::CRYPTOLIB_SUCCESS if execution was successful
 */
uint32_t sx_sha3_hash_finish_blk(sx_sha3_fct_t sha3_hash_fct, block_t data_in, block_t state, block_t data_out, size_t digest_size)CHECK_RESULT;

/**
 * @brief Compute sha3 hash digest of the concatenated content of the data blocks in the \p data_in array.
 * @param sha3_hash_fct hash function to use. See ::sx_sha3_fct_t.
 * @param data_in array of input data to process, data length can be any length as padding will be applied
 * @param entries number of elements of the data_in array, must not exceed SHA3_MAX_BLOCK_ARRAY_ELEMS
 * @param data_out output hash digest
 * @param digest_size size of the output in case of using SHAKE , ignored otherwise
 * @return ::CRYPTOLIB_SUCCESS if execution was successful
 */
uint32_t sx_sha3_hash_array_blk(sx_sha3_fct_t sha3_hash_fct,
                           block_t data_in[],
                           const unsigned int entries,
                           block_t data_out,
                           size_t digest_size);


#endif
