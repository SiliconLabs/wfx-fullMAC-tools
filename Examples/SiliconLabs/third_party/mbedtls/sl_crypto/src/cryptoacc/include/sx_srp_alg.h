/**
* @brief Handles the algorithmic operations for Secure Remote Password (SRP)
* @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
* @file
*
* The SRP protocol is described by the RFC2945
* (https://tools.ietf.org/html/rfc2945).
*
* The following information is known by the host and the user:
* N - large prime number
* g - generator of the multiplicative group
* size - size of the operands (N, g)
* hash function
*
* 0. On the user side, we have the user  + ":"+ password (usr:pwd)
*    On the host side:
*     - Generate salt
*     - Compute verifier v = srp_gen_verifier(N, g, salt, usr:pwd, hash).
*     - Host stores user, salt and verifier in its database.
* 1. User generates ephemeral key: (A, a) = srp_gen_pub(N, g).
*    Sends the user name (usr) and A to the host.
* 2. Host generates ephemeral key based on the stored verifier:
*    (B, b) = srp_host_gen_pub(N, g, v, hash).
*    Sends salt and B to the user.
* 3. User: session key = srp_user_gen_key(N, g, A, B, usr:pwd, salt, a, hash).
* 4. Host: session key = srp_host_gen_key(N, A, v, b, B, hash).
*/

#ifndef SX_SRP_ALG_H
#define SX_SRP_ALG_H

#include <stdint.h>
#include "compiler_extentions.h"
#include "cryptolib_types.h"
#include "sx_hash.h"
#include "sx_rng.h"

/**
* Generate pub key (by the host/server)
* @param  N         Domain of the key
* @param  g         Generator for the key
* @param  v         Verifier generated previously using srp_gen_verifier()
* @param  B         Output: public key B = g^b mod N
* @param  b         Output: private key
* @param  hash_fct  Hash function to use
* @param  SRP6a     Enables SRP-6a
* @param  size      Size of the operands
* @param  rng       Random number generator to use
* @return ::CRYPTOLIB_SUCCESS if no error was encountered
*         ::CRYPTOLIB_UNSUPPORTED_ERR if \c size is bigger than \c SRP_MAX_KEY_SIZE
*/
uint32_t srp_host_gen_pub(block_t N, block_t g, block_t v, block_t B, block_t b,
      sx_hash_fct_t hash_fct, uint32_t SRP6a, uint32_t size,
      struct sx_rng rng) CHECK_RESULT;

/**
* Generation of session key (by the host/server)
* @param  N         Domain of the key
* @param  A         Public key of the user
* @param  v         Verifier (generated previously)
* @param  B         Public key of the host
* @param  b         Private key of the host
* @param  K         Output: Session key
* @param  hash_fct  Hash function to use
* @param  size      Size of the operands
* @return ::CRYPTOLIB_SUCCESS if no error was encountered
*         ::CRYPTOLIB_UNSUPPORTED_ERR if \c size is bigger than \c SRP_MAX_KEY_SIZE
*/
uint32_t srp_host_gen_key(block_t N, block_t A, block_t v, block_t b, block_t B,
      block_t K, sx_hash_fct_t hash_fct, uint32_t size) CHECK_RESULT;

/**
* Generation of session key (by the user/client)
* @param  N         Domain of the key
* @param  g         Generator for the key
* @param  A         Public key of the client
* @param  B         Public key of the host
* @param  usrpwd    The concatenation of user name, a colon (:) and the password
* @param  s         Salt
* @param  a         User private key
* @param  K         Output: Session key
* @param  hash_fct  Hash function to use
* @param  SRP6a     Enables SRP-6a
* @param  size      Size of the operands
* @return ::CRYPTOLIB_SUCCESS if no error
*         ::CRYPTOLIB_UNSUPPORTED_ERR if \c size is bigger than \c SRP_MAX_KEY_SIZE
*/
uint32_t srp_user_gen_key(block_t N, block_t g, block_t A, block_t B,
   block_t usrpwd, block_t s, block_t a, block_t K, sx_hash_fct_t hash_fct,
   uint32_t SRP6a, uint32_t size) CHECK_RESULT;

/**
 * Generates verifier
 * @param  N         Domain of the key
 * @param  g         Generator for the key
 * @param  usrpwd    The concatenation of user name, a colon (:) and the
 *                   password
 * @param  s         Salt
 * @param  v         Output: verifier
 * @param  hash_fct  Hash function to use
 * @param  size      Size of the operands
 * @return ::CRYPTOLIB_SUCCESS if no error
*          ::CRYPTOLIB_UNSUPPORTED_ERR if \c size is bigger than \c SRP_MAX_KEY_SIZE
 */
uint32_t srp_gen_verifier(block_t N, block_t g,
   block_t usrpwd, block_t s,
   block_t v, sx_hash_fct_t hash_fct, uint32_t size) CHECK_RESULT;

/**
 * Generates public key (by the user/client)
 * @param  N    Domain of the key
 * @param  g    Generator for the key
 * @param  a    Output: private key
 * @param  A    Output: public key
 * @param  size Size of the operands
 * @param  rng  Random number generator to use
 * @return ::CRYPTOLIB_SUCCESS if no error
*          ::CRYPTOLIB_UNSUPPORTED_ERR if \c size is bigger than \c SRP_MAX_KEY_SIZE
 */
uint32_t srp_gen_pub(block_t N, block_t g, block_t a, block_t A, uint32_t size,
      struct sx_rng rng) CHECK_RESULT;

#endif
