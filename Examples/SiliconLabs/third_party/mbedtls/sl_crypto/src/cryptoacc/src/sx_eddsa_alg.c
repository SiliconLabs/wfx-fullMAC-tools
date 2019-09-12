/**
 * @file
 * @brief Implements the procedures to make EdDSA operations with
 *        the BA414EP public key engine
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */


#include "sx_eddsa_alg.h"
#include <string.h>
#include "cryptolib_def.h"
#include "sx_memcpy.h"
#include "ba414ep_config.h"
#include "sx_hash.h"
#include "sx_rng.h"
#include "sx_errors.h"
#include "sx_sha3.h"
#include <stdbool.h>

#define DOM4_ED448_STR "SigEd448"


/* Curve Ed25519 */
static const uint8_t ecc_ed25519_params[ED25519_KEY_SIZE * 6] =
{
      /* q = */
      0xED, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F,
      /* l = */
      0xED, 0xD3, 0xF5, 0x5C, 0x1A, 0x63, 0x12, 0x58,
      0xD6, 0x9C, 0xF7, 0xA2, 0xDE, 0xF9, 0xDE, 0x14,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10,
      /* Bx = */
      0x1A, 0xD5, 0x25, 0x8F, 0x60, 0x2D, 0x56, 0xC9,
      0xB2, 0xA7, 0x25, 0x95, 0x60, 0xC7, 0x2C, 0x69,
      0x5C, 0xDC, 0xD6, 0xFD, 0x31, 0xE2, 0xA4, 0xC0,
      0xFE, 0x53, 0x6E, 0xCD, 0xD3, 0x36, 0x69, 0x21,
      /* By = */
      0x58, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
      0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
      0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
      0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
      /* d = */
      0xA3, 0x78, 0x59, 0x13, 0xCA, 0x4D, 0xEB, 0x75,
      0xAB, 0xD8, 0x41, 0x41, 0x4D, 0x0A, 0x70, 0x00,
      0x98, 0xE8, 0x79, 0x77, 0x79, 0x40, 0xC7, 0x8C,
      0x73, 0xFE, 0x6F, 0x2B, 0xEE, 0x6C, 0x03, 0x52,
      /*  I = */
      0xB0, 0xA0, 0x0E, 0x4A, 0x27, 0x1B, 0xEE, 0xC4,
      0x78, 0xE4, 0x2F, 0xAD, 0x06, 0x18, 0x43, 0x2F,
      0xA7, 0xD7, 0xFB, 0x3D, 0x99, 0x00, 0x4D, 0x2B,
      0x0B, 0xDF, 0xC1, 0x4F, 0x80, 0x24, 0x83, 0x2B
};

/* Curve Ed448 */
static const uint8_t ecc_ed448_params[ED448_KEY_SIZE * 5] =
{
/* q = */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0x00,
/* l = */
      0xf3, 0x44, 0x58, 0xab, 0x92, 0xc2, 0x78, 0x23,
      0x55, 0x8f, 0xc5, 0x8d, 0x72, 0xc2, 0x6c, 0x21,
      0x90, 0x36, 0xd6, 0xae, 0x49, 0xdb, 0x4e, 0xc4,
      0xe9, 0x23, 0xca, 0x7c, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f,
      0x00,
/* Bx = */
      0x5e, 0xc0, 0x0c, 0xc7, 0x2b, 0xa8, 0x26, 0x26,
      0x8e, 0x93, 0x00, 0x8b, 0xe1, 0x80, 0x3b, 0x43,
      0x11, 0x65, 0xb6, 0x2a, 0xf7, 0x1a, 0xae, 0x12,
      0x64, 0xa4, 0xd3, 0xa3, 0x24, 0xe3, 0x6d, 0xea,
      0x67, 0x17, 0x0f, 0x47, 0x70, 0x65, 0x14, 0x9e,
      0xda, 0x36, 0xbf, 0x22, 0xa6, 0x15, 0x1d, 0x22,
      0xed, 0x0d, 0xed, 0x6b, 0xc6, 0x70, 0x19, 0x4f,
      0x00,
/* By = */
      0x14, 0xfa, 0x30, 0xf2, 0x5b, 0x79, 0x08, 0x98,
      0xad, 0xc8, 0xd7, 0x4e, 0x2c, 0x13, 0xbd, 0xfd,
      0xc4, 0x39, 0x7c, 0xe6, 0x1c, 0xff, 0xd3, 0x3a,
      0xd7, 0xc2, 0xa0, 0x05, 0x1e, 0x9c, 0x78, 0x87,
      0x40, 0x98, 0xa3, 0x6c, 0x73, 0x73, 0xea, 0x4b,
      0x62, 0xc7, 0xc9, 0x56, 0x37, 0x20, 0x76, 0x88,
      0x24, 0xbc, 0xb6, 0x6e, 0x71, 0x46, 0x3f, 0x69,
      0x00,
/* d = */
      0x56, 0x67, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0x00
};

static const uint8_t* ecc_eddsa_params[] = {
      ecc_ed25519_params,
      ecc_ed448_params
};

static const size_t ecc_eddsa_param_size[] = {
      sizeof(ecc_ed25519_params),
      sizeof(ecc_ed448_params)
};

static const size_t eddsa_key_size[] = {
      ED25519_KEY_SIZE,
      ED448_KEY_SIZE
};

static const size_t eddsa_sign_size[] = {
      ED25519_SIGN_SIZE,
      ED448_SIGN_SIZE
};

static const uint32_t eddsa_hw_flags[] = {
      BA414EP_SELCUR_ACCEL_ED25519,
      BA414EP_SELCUR_NO_ACCELERATOR | (1 << BA414EP_CMD_EDWARDS_LSB)
};

/**
 *  Computes H(x) = SHAKE256(dom4(phflag,context)||x, 114), as specified
 * by the RFC8032, chapter 5.2, needed for Ed448.
 * @param[in] phflag  Prehash flag (0 or 1); must be 0 for Ed448
 * @param[in] context Maximum 255 bytes shared by signer and verifier
 * @param[in] x Array of data to be concatenated (matching x in the above
 *                    formula)
 * @param[in] num_items number of elements of the x array, must be smaller or
 *                      equal to SHA3_MAX_BLOCK_ARRAY_ELEMS - 4
 * @param[out] digest Result of the applied hashing
 * @return CRYPTOLIB_SUCCESS when no issues were incountered
 */
static uint32_t ed448_hash(uint8_t phflag,
      block_t context,
      block_t x[],
      size_t num_items,
      block_t digest)
{
   block_t array[SHA3_MAX_BLOCK_ARRAY_ELEMS];

   CRYPTOLIB_ASSERT_NM((num_items + 4) <= SHA3_MAX_BLOCK_ARRAY_ELEMS);

   array[0] = block_t_convert(DOM4_ED448_STR, sizeof(DOM4_ED448_STR) - 1);
   array[1] = block_t_convert(&phflag, 1);

   uint8_t context_len = context.len;
   array[2] = block_t_convert(&context_len, 1);
   array[3] = context;

   for (size_t i = 0; i < num_items; i++) {
      array[4 + i] = x[i];
   }

   return sx_sha3_hash_array_blk(e_SHAKE256, array, num_items + 4, digest,
         ED448_SIGN_SIZE);
}

/**
 * @brief Load curve parameters into pk engine
 * @param[in] curve select the curve for which to load the parameters
 * @return CRYPTOLIB_SUCCESS
 */
static uint32_t eddsa_load_parameters(enum sx_eddsa_curve curve) {
   ba414ep_load_curve(
         block_t_convert(ecc_eddsa_params[curve], ecc_eddsa_param_size[curve]),
         eddsa_key_size[curve],
         BA414EP_LITTLEEND,
         1);
   return CRYPTOLIB_SUCCESS;
}

/**
 * @brief Hash and clamp the secret key
 * @param[in] curve select the curve for which to load the parameters
 * @param[in] sk pointer to secret key
 * @param[out] result pointer to the result
 * @param[in] result_size the size of \p buffer
 * @return CRYPTOLIB_SUCCESS
 */
static uint32_t eddsa_hash_and_clamp(enum sx_eddsa_curve curve,
      block_t sk, uint8_t *result, const size_t result_size)
{
   uint32_t status;

   size_t key_size = eddsa_key_size[curve];

   /* Hash the secret key into buffer */
   if (curve == SX_ED25519) {
      status = sx_hash_blk(e_SHA512, sk, block_t_convert(result, result_size));

      /* Clamp the LSB */
      result[key_size - 1] &= ~0x80; // Mask bit 255
      result[key_size - 1] |=  0x40; // Set bit 254
      result[0]  &= ~0x07; // Mask bits 2 to 0
   } else {
      status = sx_sha3_hash_blk(e_SHAKE256, sk,
            block_t_convert(result, ED448_SIGN_SIZE), ED448_SIGN_SIZE);
      result[0] &= ~(0x03);
      result[key_size - 1] = 0x00;
      result[key_size - 2] |= 0x80;
   }

   return status;
}

/**
 * @brief Use point coordinates compression for encoding
 * @param[in] size number of bytes needed to represent the x and y coordinates
 * @param[in] x pointer to coordinate x
 * @param[in, out] y pointer to coordinate y - encoded point output
 */
static void eddsa_encode_point(uint32_t size, const uint8_t *x, uint8_t *y)
{
   // Mask bit MSb
   y[size - 1] &= 0x7F;

   // Use LSb of x as MSb
   y[size- 1] |= (x[0] & 1)<<7;
}

/**
 * @brief decode an encoded point
 * @param[in] size number of bytes needed to represent an coordinate
 * @param[in, out] p pointer to the encoded point - y coordinate output
 * @return sign (lsb of x coordinate = x_0)
 */
static uint32_t eddsa_decode_point(uint32_t size, uint8_t *p)
{
   /* Get lsb of x for encoded point */
   uint32_t sign = ((p[size - 1]) >> 7) & 0x01; // Get MSb (sign)

   /* Get y coordinates for encoded point */
   p[size - 1] &= 0x7F; //Mask the MSb (sign bit)

   return sign;
}

uint32_t eddsa_generate_keypair(enum sx_eddsa_curve curve, block_t pk,
      block_t sk, struct sx_rng rng)
{
   uint32_t status = eddsa_generate_private_key(curve, sk, rng);
   if (status != CRYPTOLIB_SUCCESS)
      return status;
   return eddsa_generate_pubkey(curve, sk, pk);
}

uint32_t eddsa_generate_private_key(enum sx_eddsa_curve curve, block_t sk,
      struct sx_rng rng)
{
   if (curve != SX_ED25519 && curve != SX_ED448)
      return CRYPTOLIB_INVALID_PARAM;

   size_t key_size = eddsa_key_size[curve];
   if (key_size > ECC_MAX_KEY_SIZE)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   if (sk.len != key_size)
      return CRYPTOLIB_INVALID_PARAM;

   rng.get_rand_blk(rng.param, sk);
   return CRYPTOLIB_SUCCESS;
}

uint32_t eddsa_generate_pubkey(enum sx_eddsa_curve curve, block_t sk,
      block_t pk)
{
   uint8_t buffer[2 * EDDSA_MAX_KEY_SIZE];
   uint32_t status;

   if (curve != SX_ED25519 && curve != SX_ED448)
      return CRYPTOLIB_INVALID_PARAM;

   size_t key_size = eddsa_key_size[curve];
   if (key_size > ECC_MAX_KEY_SIZE)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   if (pk.len != key_size || sk.len != key_size)
      return CRYPTOLIB_INVALID_PARAM;

   // Hash and clamp the secret key
   status = eddsa_hash_and_clamp(curve, sk, buffer, 2 * key_size);
   if(status)
      return CRYPTOLIB_CRYPTO_ERR;

   /* Clear the MSB */
   memset(buffer + key_size, 0, key_size);

   // Set command to disable byte-swap
   ba414ep_set_command(BA414EP_OPTYPE_EDDSA_POINT_MULT, key_size,
         BA414EP_LITTLEEND, eddsa_hw_flags[curve]);

   // Load parameters
   eddsa_load_parameters(curve);

   /*  Set operands for EdDSA point multiplication */
#if (0)
   mem2CryptoRAM(block_t_convert(buffer, key_size),
         key_size, BA414EP_MEMLOC_8);
   mem2CryptoRAM(block_t_convert(buffer + key_size, key_size),
         key_size, BA414EP_MEMLOC_9);
#else
   point2CryptoRAM(block_t_convert(buffer, key_size * 2),
         key_size, BA414EP_MEMLOC_8);
#endif

   /* EDDSA Point Multiplication */
   if(ba414ep_start_wait_status())
      return CRYPTOLIB_CRYPTO_ERR;

#if (0)
   CryptoRAM2mem(block_t_convert(buffer, key_size),
         key_size, BA414EP_MEMLOC_10);
   CryptoRAM2mem(block_t_convert(buffer + key_size, key_size),
         key_size, BA414EP_MEMLOC_11);
#else
   CryptoRAM2point(block_t_convert(buffer, key_size * 2),
         key_size, BA414EP_MEMLOC_10);


#endif

   /* Encode point  */
   eddsa_encode_point(key_size, buffer, buffer + key_size);

   /* copy result to pk */
   memcpy_blkOut(pk, buffer + key_size, key_size);

   return CRYPTOLIB_SUCCESS;
}

/**
 * Check parameters for the EdDSA operations
 *
 * @param[in]  curve     Curve used by the EdDSA algorithm
 * @param[in]  pub_key   EdDSA public key
 * @param[in]  priv_key  EdDSA private key
 * @param[in]  context   EdDSA context, maximum 255 bytes, NULL_blk for Ed25519
 * @param[in] signature EdDSA signature of message
 * @return CRYPTOLIB_SUCCESS when no issues were encountered
 *         CRYPTOLIB_INVALID_PARAM otherwise
 */
static uint32_t eddsa_check_params(
      enum sx_eddsa_curve curve,
      block_t pub_key,
      block_t priv_key,
      block_t context,
      block_t signature)
{
   if (curve != SX_ED25519 && curve != SX_ED448)
      return CRYPTOLIB_INVALID_PARAM;

   size_t key_size = eddsa_key_size[curve];
   size_t sign_size = eddsa_sign_size[curve];

   if (key_size > ECC_MAX_KEY_SIZE)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   if (pub_key.len != key_size || priv_key.len != key_size || signature.len !=
         sign_size)
      return CRYPTOLIB_INVALID_PARAM;

   if (curve == SX_ED25519) {
      if (context.len != 0)
         return CRYPTOLIB_INVALID_PARAM;
   } else {
      if (context.len > EDDSA_MAX_CONTEXT_SIZE)
         return CRYPTOLIB_INVALID_PARAM;
   }

   return CRYPTOLIB_SUCCESS;
}

/**
 * @brief Generate an EdDSA signature of a message
 * @param[in]  curve     Curve used by the EdDSA algorithm
 * @param[in]  message   Message to sign
 * @param[in]  pub_key   EdDSA public key
 * @param[in]  priv_key  EdDSA private key
 * @param[in]  context   EdDSA context, maximum 255 bytes, NULL_blk for Ed25519
 * @param[out] signature EdDSA signature of message
 * @param[in]  use_cm    When true, use the countermeasures
 * @param[in]  rng       Random number generator to use when use_cm == true,
 *                       ignored otherwise
 * @return  CRYPTOLIB_SUCCESS if no error
 */
static uint32_t eddsa_generate_signature_internal(
      enum sx_eddsa_curve curve,
      block_t message,
      block_t pub_key,
      block_t priv_key,
      block_t context,
      block_t signature,
      bool use_cm,
      struct sx_rng *rng)
{
   uint32_t status = CRYPTOLIB_SUCCESS;
   uint8_t buffer[4 * EDDSA_MAX_KEY_SIZE];
   uint8_t random_data[SHA512_BLOCKSIZE - ED25519_KEY_SIZE];
   block_t random_blk = block_t_convert(random_data, sizeof(random_data));
   block_t array[4];

   status = eddsa_check_params(curve, pub_key, priv_key,
         context, signature);
   if (status != CRYPTOLIB_SUCCESS)
      return status;

   size_t key_size = eddsa_key_size[curve];
   size_t sign_size = eddsa_sign_size[curve];

   // Hash and clamp the secret key
   status |= eddsa_hash_and_clamp(curve, priv_key, buffer, sign_size);

   if (status)
      return CRYPTOLIB_CRYPTO_ERR;

   // buffer content: hlsb, hmsb, -, -

   /* Hash last bytes of buffer with message */

   array[0]= block_t_convert(buffer + key_size, key_size);

   if (curve == SX_ED25519) {
      size_t index = 1;
      /* For countermeasures, we need to pad with random data until we reach
       * the block size.
       */
      if (use_cm) {
         rng->get_rand_blk(rng->param, random_blk);
         array[index++] = random_blk;
      }
      array[index++] = message;

      status = sx_hash_array_blk(e_SHA512, array, index,
            block_t_convert(buffer + key_size, key_size * 2));
   } else {
      array[1] = message;
      status = ed448_hash(0, context, array, 2,
            block_t_convert(buffer + key_size, key_size * 2));
   }
   if (status)
      return CRYPTOLIB_CRYPTO_ERR;

   // buffer content: hlsb, HLSB, HMSB, -

   // Set command to disable byte-swap
   ba414ep_set_command(BA414EP_OPTYPE_EDDSA_POINT_MULT, key_size,
         BA414EP_LITTLEEND, eddsa_hw_flags[curve]);

   // Load parameters
   eddsa_load_parameters(curve);

   /* add results to EdDSA parameters for point multiplication */
#if (0)
   mem2CryptoRAM(block_t_convert(buffer + key_size, key_size), key_size,
         BA414EP_MEMLOC_8);
   mem2CryptoRAM(block_t_convert(buffer + 2 * key_size, key_size), key_size,
         BA414EP_MEMLOC_9);
#else
   point2CryptoRAM(block_t_convert(buffer + key_size, 2 *key_size), key_size,
            BA414EP_MEMLOC_8);
#endif

   // Start BA414EP operation
   status = ba414ep_start_wait_status();
   if (status)
      return CRYPTOLIB_CRYPTO_ERR;

   /* Prepare buffers for point multiplication.
    * if CryptoRAM2point() is used than the order will be Rx, Ry in the
    * buffer and that will lead to more changes in the subsequent code,
    * therefore the two legacy calls for CryptoRAM2mem() remain.
    */
   CryptoRAM2mem(block_t_convert(buffer + 2 * key_size, key_size), key_size,
         BA414EP_MEMLOC_10);
   CryptoRAM2mem(block_t_convert(buffer + key_size, key_size), key_size,
         BA414EP_MEMLOC_11);

   // buffer content: hlsb, Ry, Rx, -

   /* Encode resulting point R(x,y) */
   eddsa_encode_point(key_size, buffer + 2 * key_size, buffer + key_size);

   // buffer content: hlsb, R, -, -

   /* Hash R, pk and message into temp */
   array[0]= block_t_convert(buffer + key_size, key_size);
   array[1]= pub_key;
   array[2]= message;
   if (curve == SX_ED25519) {
       status = sx_hash_array_blk(e_SHA512, array, 3,
             block_t_convert(buffer + 2 * key_size, 2 * key_size));
   } else {
       status = ed448_hash(0, context, array, 3,
             block_t_convert(buffer + 2 * key_size, 2 * key_size));
   }
   if (status)
      return CRYPTOLIB_CRYPTO_ERR;

   // buffer content: hlsb, R, HLSB, HMSB

   ba414ep_set_command(BA414EP_OPTYPE_EDDSA_SIGN_GEN, key_size,
         BA414EP_LITTLEEND, eddsa_hw_flags[curve]);

   /* Prepare EdDSA parameters for signature generation */
   mem2CryptoRAM(block_t_convert(buffer + 2 * key_size, key_size), key_size,
         BA414EP_MEMLOC_6);
   mem2CryptoRAM(block_t_convert(buffer + 3 * key_size, key_size), key_size,
         BA414EP_MEMLOC_7);
   mem2CryptoRAM(block_t_convert(buffer, key_size), key_size,
         BA414EP_MEMLOC_11);

   /* Generate signature */
   if(ba414ep_start_wait_status())
      return CRYPTOLIB_CRYPTO_ERR;

   CryptoRAM2mem(block_t_convert(buffer + 2 * key_size, key_size), key_size,
         BA414EP_MEMLOC_10);

   // buffer content: -, R, S, -
   memcpy_blkOut(signature, buffer + key_size, key_size * 2);
   return CRYPTOLIB_SUCCESS;
}

uint32_t ed25519_generate_signature_with_cm(
      block_t message,
      block_t pub_key,
      block_t priv_key,
      block_t signature,
      struct sx_rng rng)
{
   return eddsa_generate_signature_internal(SX_ED25519, message, pub_key,
         priv_key, NULL_blk, signature, true, &rng);
}

uint32_t eddsa_generate_signature(
      enum sx_eddsa_curve curve,
      block_t message,
      block_t pub_key,
      block_t priv_key,
      block_t context,
      block_t signature)
{
   return eddsa_generate_signature_internal(curve, message, pub_key, priv_key,
         context, signature, false, NULL);
}

uint32_t eddsa_verify_signature(
      enum sx_eddsa_curve curve,
      block_t message,
      block_t key,
      block_t context,
      block_t signature)
{
   uint8_t digest[EDDSA_MAX_SIGN_SIZE];

   uint32_t PKsign;
   uint32_t Rsign;
   uint8_t sig_copy[EDDSA_MAX_SIGN_SIZE];
   uint8_t key_cpy[EDDSA_MAX_KEY_SIZE];
   uint32_t status;

   /* We use the same function for checking the parameters as the signing
   functions by sending the same key for private and public (only the lengths
   are checked). */
   status = eddsa_check_params(curve, key, key, context,
      signature);
   if (status != CRYPTOLIB_SUCCESS)
      return status;

   size_t sign_size = eddsa_sign_size[curve];
   size_t key_size = eddsa_key_size[curve];

   /* Copy the key and the signature for multiple use */
   memcpy_blkIn(key_cpy, key, key_size);
   memcpy_blkIn(sig_copy, signature, sign_size);

   /* Hash R, pk and message result is byte array digest */
   block_t array[3] = {
         block_t_convert(sig_copy, key_size),
         block_t_convert(key_cpy, key_size),
         message};

   if (curve == SX_ED25519) {
       status = sx_hash_array_blk(e_SHA512, array, 3,
         block_t_convert(digest, sign_size));
   } else {
       status = ed448_hash(0, context, array, 3,
         block_t_convert(digest, sign_size));
   }
   if(status)
      return CRYPTOLIB_CRYPTO_ERR;

   /* Decode points pk and R */
   PKsign = eddsa_decode_point(key_size, key_cpy);
   Rsign  = eddsa_decode_point(key_size, sig_copy);

   // Set command to disable byte-swap
   ba414ep_set_command(BA414EP_OPTYPE_EDDSA_SIGN_VERIF,
       key_size,
       BA414EP_LITTLEEND,
      (PKsign << BA414EP_CMD_FLAGA_LSB) | (Rsign << BA414EP_CMD_FLAGB_LSB) |
      eddsa_hw_flags[curve]);

   // Load curve parameters
   eddsa_load_parameters(curve);

   /* Prepare EdDSA parameters for signature verification */
   mem2CryptoRAM(block_t_convert(digest, key_size),
         key_size, BA414EP_MEMLOC_6);
   mem2CryptoRAM(block_t_convert(digest + key_size, key_size),
         key_size, BA414EP_MEMLOC_7);
   mem2CryptoRAM(block_t_convert(key_cpy, key_size), key_size,
         BA414EP_MEMLOC_9);
   mem2CryptoRAM(block_t_convert(sig_copy + key_size, key_size), key_size,
         BA414EP_MEMLOC_10); // S
   mem2CryptoRAM(block_t_convert(sig_copy, key_size), key_size,
         BA414EP_MEMLOC_11); // R

   if(ba414ep_start_wait_status())
       return CRYPTOLIB_INVALID_SIGN_ERR;

   return CRYPTOLIB_SUCCESS;
}
