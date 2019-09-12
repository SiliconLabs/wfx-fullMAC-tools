/**
 * @file
 * @brief Defines the procedures to make JPAKE operations with
 *        the BA414EP pub key.
 *        Full details may be found in RFC 8236 but only the NIST-P256
 *        curve is supported as defined in
 *        Thread, "Thread Commissioning", White Paper, July 2015
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */


#ifndef SX_JPAKE_ALG_H
#define SX_JPAKE_ALG_H

#include <stdint.h>
#include "compiler_extentions.h"
#include "cryptolib_types.h"
#include "sx_rng.h"

/**
 * @brief J-Pake protocol round 1 generate zero-knowledge proof
 * @details Alice selects two random numbers x1 and x2 and generates a zkp for both number
 * The input is an ID "my_id" and the output is X1 (=g^x1), X2 (=g^x2),
 * zkp1 (zero-knowledge proof for x1) and zkp2 (zero-knowledge proof for x2)
 * @param  user_id  Input id of the user
 * @param  x2       Input scalar x2
 * @param  zkp1     Output point and its zero knowledge proof 1
 * @param  zkp2     Output point and its zero knowledge proof 2
 * @param  rng      Used random number generator
 * @return CRYPTOLIB_SUCCESS if finished successfully
 * \note NIST-P256 with SHA256 is internally used and cant be changed to other
 *       curve and hash combination
 */
 uint32_t jpake_round1_generate_blk(
      block_t user_id,
      block_t x2,
      block_t zkp1,
      block_t zkp2,
      struct sx_rng rng) CHECK_RESULT;

/**
 * @brief J-Pake protocol round 1 verify the zero knowledge proof
 * @details Alice receives zkp's from Bob for x3 and x4. Alice needs to verify these zkp:
 * The input (from Bob) is: "his_id", X3 (=X1 from Bob), X4 (=X2 from Bob),
 * zkp1 (zero-knowledge proof for x1) and zkp2 (zero-knowledge proof for x2)
 * @param  my_user_id  Input id of the user
 * @param  his_user_id Input id of the other user
 * @param  zkp3        Input point X3 and its ZKP
 * @param  zkp4        Input point X4 and its ZKP
 * @return CRYPTOLIB_SUCCESS if zkp is correct
 *         CRYPTOLIB_INVALID_SIGN_ERR otherwise
 *
 * \note NIST-P256 with SHA256 is internally used and cant be changed to other
 *       curve and hash combination
 */
 uint32_t jpake_round1_verify_blk(
      block_t my_user_id,
      block_t his_user_id,
      block_t zkp3,
      block_t zkp4) CHECK_RESULT;

/**
 * @brief J-Pake round 2 generate
 * @details Input is a password. Output is A (g^((x1+x3+x4)*x2*s) and
 * zkp1 (zero-knowledge proof for x2s)
 * @param  user_id   Input id of the user
 * @param  pwd_blk   Input user password
 * @param  x2        Input scalar x2
 * @param  X1_in     Input point X1
 * @param  X3_in     Input point X3
 * @param  X4_in     Input point X4
 * @param  data_out  Output for A and its ZKP
 * @param  rng       Used random number generator
 * @return CRYPTOLIB_SUCCESS if finished successfully
 *
 * \note NIST-P256 with SHA256 is internally used and cant be changed to other
 *       curve and hash combination
 */
 uint32_t jpake_round2_generate_blk(
      block_t user_id,
      block_t pwd_blk,
      block_t x2,
      block_t X1_in,
      block_t X3_in,
      block_t X4_in,
      block_t data_out,
      struct sx_rng rng) CHECK_RESULT;

/**
 * @brief Verify J-pake round 2 data
 * @details input is B (g^((x1+x2+x3)*x4*s)) received from Bob and
 * zkp2 (zero-knowledge proof for x4s)
 * @param  user_id Input id of the user
 * @param  X1      Input point X1
 * @param  X2      Input point X2
 * @param  X3      Input point X3
 * @param  Bzkp    Input point B and zkp
 * @return CRYPTOLIB_SUCCESS if data is correct
 *         CRYPTOLIB_INVALID_SIGN_ERR otherwise
 *
 * \note NIST-P256 with SHA256 is internally used and cant be changed to other
 *       curve and hash combination
 */
 uint32_t jpake_round2_verify_blk(
       block_t user_id,
       block_t X1,
       block_t X2,
       block_t X3,
       block_t Bzkp) CHECK_RESULT;

/**
 * @brief J-pake generate session key
 * @details output key = hashed session key to be used by alice and bob
 * @param  pwd_blk     Input  user password
 * @param  x2_blk      Input  scalar x2
 * @param  X4_blk      Input  point X4
 * @param  B_blk       Input  point B
 * @param  session_key Output generated session key
 * @return CRYPTOLIB_SUCCESS if finished
 *
 * \note NIST-P256 with SHA256 is internally used and cant be changed to other
 *       curve and hash combination
 */
uint32_t jpake_generate_session_key_blk(
      block_t pwd_blk,
      block_t x2_blk,
      block_t X4_blk,
      block_t B_blk,
      block_t session_key) CHECK_RESULT;

/**
 * @brief Compute GB = X1 + X2 + X3
 * @param  X1 point X1
 * @param  X2 point X2
 * @param  X3 point X3
 * @param  GB output
 * @return CRYPTOLIB_SUCCESS on success
 *
 * \note NIST-P256 with SHA256 is internally used and cant be changed to other
 *       curve and hash combination
 */
uint32_t jpake_3point_add(
      block_t X2,
      block_t X3,
      block_t X1,
	   block_t GB);

/**
 * @brief Compute points and x2 * s for ECJPAKE round 2
 * @details Computes: GA = X1 + X2 + X3; x2s = x2 * s; A = x2s * GA
 * @param  x2       Big number x2 in big endian format
 * @param  X1_in    Point X1
 * @param  X3_in    Point X3
 * @param  X4_in    Point X4
 * @param  x2s_blk  Input password; output = x2 * s
 * @param  GA       Output point X1 + X2 + X3
 * @param  A        Output point x2s * GA, may be set to NULL_blk to be not returned
 * @return CRYPTOLIB_SUCCESS on success
 *
 * \note NIST-P256 with SHA256 is internally used and cant be changed to other
 *       curve and hash combination
 */
uint32_t jpake_round2_compute_points(
      block_t x2,
      block_t X1_in,
      block_t X3_in,
      block_t X4_in,
      block_t x2s_blk,
      block_t GA,
      block_t A);

/**
 * @brief Verify a ECKPAKE Zero Knowledge Proof
 * @param  G           Point X1 + X2 + X3
 * @param  Gloc		  CryptoRAM location where G is preloaded or ~0 if not.
 * @param  X_b         Point X
 * @param  V_b         Point V base of the ZKP
 * @param  z_b         Value in big endian part of ZKP
 * @param  his_id	User ID of the creator of the ZKP
 * @return CRYPTOLIB_SUCCESS on success
 *
 * \note NIST-P256 with SHA256 is internally used and cant be changed to other
 *       curve and hash combination
 */
uint32_t jpake_hash_verify_zkp(
      block_t G,
      uint32_t Gloc,
      block_t X_b,
      block_t V_b,
      block_t z_b,
      block_t his_id);

/**
 * @brief Compute ZKP from the 'user_id' and 2 key pairs (x, X) and (v, V)
 *
 * Steps in the computation:
 *  1. Compute h = hash(G, V, X, my_id)
 *  2. Compute r = v - x.h mod n
 *
 * The zkp is the point V and the value r.
 *
 * @param G      Point G
 * @param x      ZKP private key x big endian value.
 *               If empty block, it should be preloaded in location 11.
 * @param X      ZKP Point X from private key x.
 * @param v      ZKP private key v big endian value
 *               If empty block, it should be preloaded in location 8.
 * @param V      ZKP Point V from private key v.
 * @param my_id  User ID of the creator of the ZKP
 * @param r      Big endian value proof in the ZKP
 * @return CRYPTOLIB_SUCCESS on success
 *
 * \note NIST-P256 with SHA256 is internally used and cant be changed to other
 *       curve and hash combination
 */
uint32_t jpake_hash_create_zkp(
      block_t G,
      block_t x,
      block_t X,
      block_t v,
      block_t V,
      block_t my_id,
      block_t r);


/** @brief Size (in bytes) of an element curve (for a NIST-P256) */
#define JPAKE_CURVE_ELEM_SIZE   32
/** @brief Number of elements in a curve point */
#define JPAKE_POINT_NB_ELEM  2
/** @brief Number of elements requested for a ZKP verification */
#define JPAKE_ZKP_NB_ELEM 3
/** @brief Number of elements of both a point and its ZKP */
#define JPAKE_POINT_ZKP_NB_ELEM (JPAKE_POINT_NB_ELEM + JPAKE_ZKP_NB_ELEM)
/** @brief Size (in bytes) of an J-PAKE ID */
#define JPAKE_MAX_ID_SIZE 32

#endif
