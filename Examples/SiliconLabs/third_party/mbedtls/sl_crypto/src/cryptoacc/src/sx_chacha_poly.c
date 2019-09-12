/**
 * @file
 * @brief Defines the procedures to make operations with
 *          the BA417 ChachaPoly function
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */


#include "sx_chacha_poly.h"
#include "cryptolib_def.h"
#include "cryptodma_internal.h"
#include "sx_memcpy.h"
#include "sx_memcmp.h"
#include "ba417_config.h"
#include "sx_math.h"
#include "sx_errors.h"

/**
 * @brief Set the command word of the ChaCha20 & Poly1305.
 * @param algo      The Chacha-Poly algo
 * @param op        The Chacha operation (encrypt or decrypt)
 * @param mode_out  A pointer to the output command word
 * @return          CRYPTOLIB_SUCCESS if execution was successful,
 *                  CRYPTOLIB_UNSUPPORTED_ERR if the algo or op is invalid
 */
static int sw_chapol_setmode(sx_chacha_poly_algo algo,
                             sx_chacha_op_t op, uint32_t *mode_out)
{
   *mode_out = 0;
   // ChaCha - Poly algo
   switch (algo) {
   case SX_CHACHA_POLY:
      *mode_out |= BA417_CONFIG_CHACHA_POLY;
      break;
   case SX_CHACHA_ONLY:
      *mode_out |= BA417_CONFIG_CHACHA_ONLY;
      break;
   case SX_POLY_CHACHAKEY:
      *mode_out |= BA417_CONFIG_POLY_CHACHAKEY;
      break;
   case SX_POLY_ONLY:
      *mode_out |= BA417_CONFIG_POLY_ONLY;
      break;
   default:
      return CRYPTOLIB_UNSUPPORTED_ERR;
   }

   // ChaCha operation
   switch (op) {
   case CHAENC:
      *mode_out |= BA417_CONFIG_ENCRYPT;
      break;
   case CHADEC:
      *mode_out |= BA417_CONFIG_DECRYPT;
      break;
   default:
      return CRYPTOLIB_UNSUPPORTED_ERR;
   }

   return CRYPTOLIB_SUCCESS;
}

uint32_t sx_chacha_poly(sx_chacha_poly_algo algo,
                        sx_chacha_op_t op,
                        block_t din,
                        block_t dout,
                        block_t key,
                        block_t iv,
                        block_t nonce,
                        block_t aad,
                        block_t tag)
{
   uint32_t ret;
   unsigned int datapad_len;
   unsigned int aadpad_len;
   struct dma_sg_descr_s desc_to[6];
   struct dma_sg_descr_s desc_fr[4];
   uint8_t lenAlenC[16];
   uint8_t zeroes[16] = {0x00};
   uint8_t one[SX_CHACHA_IV_SIZE] = {0x00, 0x00, 0x00, 0x01};
   uint8_t tag_out[SX_POLY_TAG_SIZE];
   struct ba417_regs_s chapol_params;

   // FETCHER
   // Populate param structure
   ret = sw_chapol_setmode(algo, op, (uint32_t*) &chapol_params.config);
   if (ret)
      return ret;

   // Validate input size
   if ((!din.len && algo != SX_CHACHA_POLY) ||
         (!din.len && !aad.len && algo == SX_CHACHA_POLY))
      return CRYPTOLIB_INVALID_PARAM;

   // Copy key
   if ((key.len != SX_CHACHA_POLY_KEY_SIZE))
      return CRYPTOLIB_INVALID_PARAM;
   memcpy_blkIn(chapol_params.key, key, key.len);

   // Copy IV
   if (algo == SX_CHACHA_ONLY && iv.len != SX_CHACHA_IV_SIZE)
      return CRYPTOLIB_INVALID_PARAM;
   else if (algo != SX_CHACHA_ONLY && iv.len != 0)
      return CRYPTOLIB_INVALID_PARAM;

   switch(algo) {
   case SX_CHACHA_ONLY:
      /* IV can be any value (https://tools.ietf.org/html/rfc7539#section-2.4) */
      break;
   case SX_CHACHA_POLY:
      /* IV is set to one (https://tools.ietf.org/html/rfc7539#section-2.4)*/
      iv = block_t_convert(one, SX_CHACHA_IV_SIZE);
      break;
   case SX_POLY_ONLY:
      /* Not IV expected when computing only the Poly1305 MAC */
      break;
   case SX_POLY_CHACHAKEY:
      /* IV is zeroed (https://tools.ietf.org/html/rfc7539#section-2.6) */
      iv = block_t_convert(zeroes, SX_CHACHA_IV_SIZE);
      break;
   default:
      return CRYPTOLIB_INVALID_PARAM;
   }

   if (algo != SX_POLY_ONLY)
      memcpy_blkIn(chapol_params.iv, iv, SX_CHACHA_IV_SIZE);

   // Copy nonce
   if (nonce.len != SX_CHACHA_NONCE_SIZE && algo != SX_POLY_ONLY)
      return CRYPTOLIB_INVALID_PARAM;
   memcpy_blkIn(chapol_params.nonce, nonce, nonce.len);

   datapad_len = (16 - din.len) & 15;
   aadpad_len = (16 - aad.len) & 15;

   // configuration descriptor
   struct dma_sg_descr_s *d = &desc_to[0];
   d = write_desc(
         d,
         &chapol_params,
         sizeof(chapol_params),
         DMA_AXI_DESCR_REALIGN,
         DMA_SG_ENGINESELECT_BA417 | DMA_SG_TAG_ISCONFIG);

   // aad input descriptor
   d = write_desc_blk(
         d,
         &aad,
         0,
         DMA_SG_ENGINESELECT_BA417 | DMA_SG_TAG_ISDATA |
         DMA_SG_TAG_DATATYPE_AESHEADER);

   d = write_desc(
         d,
         zeroes,
         aadpad_len,
         0,
         DMA_SG_ENGINESELECT_BA417 | DMA_SG_TAG_ISDATA |
         DMA_SG_TAG_DATATYPE_AESHEADER | DMA_SG_TAG_SETINVALIDBYTES(aadpad_len));

   // data input descriptor
   d = write_desc_blk(
         d,
         &din,
         0,
         DMA_SG_ENGINESELECT_BA417 | DMA_SG_TAG_ISDATA |
         DMA_SG_TAG_DATATYPE_AESPAYLOAD);

   d = write_desc(
         d,
         zeroes,
         datapad_len,
         0,
         DMA_SG_ENGINESELECT_BA417 | DMA_SG_TAG_ISDATA |
         DMA_SG_TAG_DATATYPE_AESPAYLOAD |
         DMA_SG_TAG_SETINVALIDBYTES(datapad_len));

   realign_desc(d - 1);

   // lenAlenC block
   if (algo == SX_CHACHA_POLY) {
      sx_math_u64_to_u8array(aad.len, &lenAlenC[0], sx_little_endian);
      sx_math_u64_to_u8array(din.len, &lenAlenC[8], sx_little_endian);

      d = write_desc(
            d,
            lenAlenC,
            sizeof(lenAlenC),
            DMA_AXI_DESCR_REALIGN,
            DMA_SG_ENGINESELECT_BA417 | DMA_SG_TAG_ISDATA |
            DMA_SG_TAG_DATATYPE_AESPAYLOAD);
   }

   set_last_desc(d - 1);

   // Pusher
   // Reset the descriptor pointer to first descriptor and iterate again
   // over the possible data to receive
   d = &desc_fr[0];

   // For ChaCha (only or combined with Poly)
   if (algo < SX_POLY_CHACHAKEY) {
      // discard aad and aad padding
      d = write_desc(
            d,
            dout.addr,
            (aad.len + aadpad_len),
            DMA_AXI_DESCR_DISCARD | DMA_AXI_DESCR_REALIGN,
            0);

      // data output descriptor
      d = write_desc(
            d,
            dout.addr,
            roundup_32(din.len), // Round-up to 32 bits to avoid issue with half-word writes
            dout.flags,
            0);

      // discard data padding
      d = write_desc(
            d,
            dout.addr,
            (datapad_len & ~3),
            DMA_AXI_DESCR_DISCARD,
            0);
   }

   // tag block (for all but chacha cipher only)
   if (op == CHADEC && algo != SX_CHACHA_ONLY) {
      d = write_desc(
            d,
            tag_out,
            sizeof(tag_out),
            DMA_AXI_DESCR_REALIGN,
            0);
   }

   // (for all but chacha cipher only)
   if (op == CHAENC && algo != SX_CHACHA_ONLY) {
      if (tag.len != SX_POLY_TAG_SIZE)
         return CRYPTOLIB_INVALID_PARAM;

      d = write_desc_blk(
            d,
            &tag,
            DMA_AXI_DESCR_REALIGN,
            0);
   }

   set_last_desc(d - 1);

   // Run
   cryptodma_run_sg(desc_to, desc_fr);

   // Check MAC
   if (op == CHADEC && algo != SX_CHACHA_ONLY) {
      uint32_t error = memcmp32_blk_time_cst(
            tag,
            block_t_convert(tag_out, sizeof(tag_out)),
            sizeof(tag_out));
      if (error) {
         return CRYPTOLIB_INVALID_SIGN_ERR;
      }
   }
   return CRYPTOLIB_SUCCESS;
}
