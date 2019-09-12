/**
 * @file
 * @brief Declares the constants and functions to make operations
 *          with the BA418 hash function
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */


#include "sx_sha3.h"
#include <stdint.h>
#include <string.h>
#include "cryptodma_internal.h"
#include "sx_memcpy.h"
#include "sx_errors.h"
#include "ba418_config.h"

/**
 * @brief internal function for sha3 parameter validation
 * @param sha3_hash_fct sha3 hash function to use. See ::sx_sha3_fct_t.
 * @param sha3_hash_operation define type of operation to perform
 * @param state block containing intermediate  state during context switching
 * @param data_out block containing intermediate  state during context switching
 * @param digest_size the size og the output in case of shake modes
 * @param msg_len specifies the message length
 * @return ::CRYPTOLIB_SUCCESS when paramters are valid
           ::CRYPTOLIB_INVALID_PARAM when paramters are not valid
 */
static uint32_t sx_sha3_validateInput(sx_sha3_fct_t sha3_hash_fct,
                                      sx_sha3_op_t sha3_hash_operation,
                                      block_t state,
                                      block_t data_out,
                                      size_t digest_size,
                                      size_t msg_len)
{
   size_t block_size;
   size_t output_size;
   output_size = sx_sha3_get_digest_size(sha3_hash_fct);
   block_size = output_size * 2;
   block_size = SHA3_STATE_SIZE - block_size;
   if (msg_len % block_size != 0)
      return CRYPTOLIB_INVALID_PARAM;
   if ((sha3_hash_operation == e_SHA3_OP_UPDATE || sha3_hash_operation == e_SHA3_OP_FINAL)
      && state.len != SHA3_STATE_SIZE)
      return CRYPTOLIB_INVALID_PARAM;
   size_t expected_size;
   if (sha3_hash_operation == e_SHA3_OP_BEGIN || sha3_hash_operation == e_SHA3_OP_UPDATE){
      expected_size = SHA3_STATE_SIZE;
   } else {
      if (sha3_hash_fct != e_SHAKE128 && sha3_hash_fct != e_SHAKE256)
         expected_size = output_size;
      else
         expected_size = digest_size;
   }
   if (data_out.len < expected_size)
      return CRYPTOLIB_INVALID_PARAM;
   return CRYPTOLIB_SUCCESS;
}

/**
 * @brief internal function for sha3 hash operation
 * @param sha3_hash_fct sha3 hash function to use. See ::sx_sha3_fct_t.
 * @param sha3_hash_operation define type of operation to perform
 * @param state intermediate state in case of context switching
 * @param data_in block of input data to process
 * @param padding padding of the message length will be 0 in case of msg is correctly padded
 * @param data_out output digest or state
 * @param digest_size in case of doing SHAKE128 or SHAKE256 , ignored otherwise
 * @return ::CRYPTOLIB_SUCCESS when execution was successful
 */
static uint32_t sx_sha3_hash_internal(sx_sha3_fct_t sha3_hash_fct,
                                       sx_sha3_op_t sha3_hash_operation,
                                       block_t state,
                                       block_t data_in[],
                                       unsigned int entries,
                                       block_t padding,
                                       block_t data_out,
                                       size_t digest_size)
{
   if (entries > SHA3_MAX_BLOCK_ARRAY_ELEMS)
      return CRYPTOLIB_INVALID_PARAM;

   uint32_t total_len = 0;
   for (unsigned int i = 0; i < entries; i++)
      total_len += data_in[i].len;

   uint32_t isValidparameters = sx_sha3_validateInput(sha3_hash_fct,
         sha3_hash_operation, state, data_out, digest_size,
         total_len + padding.len);
   if (isValidparameters != CRYPTOLIB_SUCCESS)
      return CRYPTOLIB_INVALID_PARAM;

   /* Two extra elements are needed: configuration as first elements and padding as last. */
   struct dma_sg_descr_s desc_from[SHA3_MAX_BLOCK_ARRAY_ELEMS + 2];
   struct dma_sg_descr_s desc_to;
   struct dma_sg_descr_s *desc_curr;
   struct ba418_regs_s ba418;

   switch (sha3_hash_fct)
   {
      case e_SHA3_224:
         ba418.config = BA418_CONF_MODE_SHA3_224;
         break;
      case e_SHA3_256:
         ba418.config = BA418_CONF_MODE_SHA3_256;
         break;
      case e_SHA3_384:
         ba418.config = BA418_CONF_MODE_SHA3_384;
         break;
      case e_SHA3_512:
         ba418.config = BA418_CONF_MODE_SHA3_512;
         break;
      case e_SHAKE128:
         ba418.config = BA418_CONF_MODE_SHAKE_128;
         break;
      case e_SHAKE256:
         ba418.config = BA418_CONF_MODE_SHAKE_256;
         break;
   }
   if (sha3_hash_operation == e_SHA3_OP_UPDATE || sha3_hash_operation == e_SHA3_OP_BEGIN)
      ba418.config |= BA418_CONF_OUTPUT_STATE;

   if (sha3_hash_fct == e_SHAKE128 || sha3_hash_fct == e_SHAKE256){
      ba418.config |= (digest_size << 8);
   } else {
      digest_size = sx_sha3_get_digest_size(sha3_hash_fct);
   }

   // Config
   desc_curr = &desc_from[0];
   desc_curr = write_desc(
         desc_curr,
         &ba418,
         sizeof(ba418),
         DMA_AXI_DESCR_REALIGN,
         DMA_SG_ENGINESELECT_BA418 | DMA_SG_TAG_ISCONFIG);

   // Final or udpate state
   if (sha3_hash_operation == e_SHA3_OP_UPDATE || sha3_hash_operation == e_SHA3_OP_FINAL) {
      desc_curr = write_desc_blk(
            desc_curr,
            &state,
            DMA_AXI_DESCR_REALIGN,
            DMA_SG_ENGINESELECT_BA418 | DMA_SG_TAG_DATATYPE_HASHINIT | DMA_SG_TAG_ISLAST);
   }

   // Current part of data to hash
   for (unsigned int i = 0; i < entries; i++) {
      desc_curr = write_desc_blk(
            desc_curr,
            &data_in[i],
            0,
            DMA_SG_ENGINESELECT_BA418 | DMA_SG_TAG_ISDATA | DMA_SG_TAG_DATATYPE_HASHMSG);
   }

   // Additional padding (if any)
   desc_curr = write_desc_blk(
         desc_curr,
         &padding,
         DMA_AXI_DESCR_REALIGN,
         DMA_SG_ENGINESELECT_BA418);

   set_last_desc(desc_curr - 1);

   // Output digest
   if (sha3_hash_operation == e_SHA3_OP_FINAL || sha3_hash_operation == e_SHA3_OP_WHOLE) {
      data_out.len = digest_size;
      /* For the DMA FIFO, ensure that we have a multiple of 4 bytes. */
      if(data_out.flags & DMA_AXI_DESCR_CONST_ADDR)
         data_out.len = roundup_32(data_out.len);
   } else {
      data_out.len = SHA3_STATE_SIZE;
   }

   write_desc_blk(&desc_to, &data_out, 0, 0);
   set_last_desc(&desc_to);

   cryptodma_run_sg(&desc_from[0], &desc_to);


   return CRYPTOLIB_SUCCESS;
}


uint32_t sx_sha3_get_digest_size(sx_sha3_fct_t sha3_hash_fct)
{
   switch(sha3_hash_fct){
      case e_SHAKE128:
         return SHAKE128_DIGESTSIZE;
      case e_SHA3_224:
         return SHA3_224_DIGESTSIZE;
      case e_SHAKE256:
      case e_SHA3_256:
         return SHA3_256_DIGESTSIZE;
      case e_SHA3_384:
         return SHA3_384_DIGESTSIZE;
      case e_SHA3_512:
         return SHA3_512_DIGESTSIZE;
      default:
         return 0;
   }
}

/**
 * @brief perform SHA3 padding and write the output to \p padded_msg
 * according to standard sha3 padding scheme described here :
 * https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.202.pdf (section B2)
 * @param sha3_hash_fct hash function to use. See ::sx_sha3_fct_t.
 * @param msg_len length of the message being padded
 * @param padded_msg output of the padding to be appended to the message
 */
static void sx_sha3_pad(sx_sha3_fct_t sha3_hash_fct, size_t msg_len , block_t *padded_msg)
{
   uint8_t prefixByte , suffixByte , complementByte;
   if (sha3_hash_fct == e_SHAKE256 || sha3_hash_fct == e_SHAKE128){
      prefixByte = SHA_MODE_PREFIX;
      suffixByte = SHA_MODE_SUFFIX;
      complementByte = SHA_MODE_COMPLEMENT;
   } else {
      prefixByte = SHAKE_MODE_PREFIX;
      suffixByte = SHAKE_MODE_SUFFIX;
      complementByte = SHAKE_MODE_COMPLEMENT;
   }
   uint32_t r = sx_sha3_get_digest_size(sha3_hash_fct);
   r *= 2;
   r = SHA3_STATE_SIZE - r;
   uint32_t q = 0;
   int i = 0;
   q = r - (msg_len % r);
   if (q == 1)
      padded_msg->addr[i++] = complementByte;
   else if (q >= 2) {
      padded_msg->addr[i++] = prefixByte;
      memset (&padded_msg->addr[i], 0x00 , q-2);
      i+= q-2;
      padded_msg->addr[i++]=suffixByte;
   }
   padded_msg->len = i;
}

uint32_t sx_sha3_hash_blk(sx_sha3_fct_t sha3_hash_fct, block_t data_in, block_t data_out, size_t digest_size)
{
   uint8_t padded_msg[SHA3_MAX_BLOCK_SIZE+4];
   block_t padding_blck;
   padding_blck = block_t_convert(padded_msg,0);
   sx_sha3_pad(sha3_hash_fct, data_in.len, &padding_blck);
   return sx_sha3_hash_internal(sha3_hash_fct, e_SHA3_OP_WHOLE, block_t_convert(NULL, 0), &data_in, 1, padding_blck, data_out, digest_size);
}

uint32_t sx_sha3_hash_begin_blk(sx_sha3_fct_t sha3_hash_fct, block_t data_in, block_t state_out)
{
   return sx_sha3_hash_internal(sha3_hash_fct, e_SHA3_OP_BEGIN, block_t_convert(NULL, 0), &data_in, 1, block_t_convert(NULL, 0), state_out, 0);
}

uint32_t sx_sha3_hash_update_blk(sx_sha3_fct_t sha3_hash_fct, block_t data_in, block_t state, block_t state_out)
{
   return sx_sha3_hash_internal(sha3_hash_fct, e_SHA3_OP_UPDATE, state, &data_in, 1, block_t_convert(NULL, 0), state_out, 0);
}

uint32_t sx_sha3_hash_finish_blk(sx_sha3_fct_t sha3_hash_fct, block_t data_in, block_t state, block_t data_out, size_t digest_size)
{
   uint8_t padded_msg[SHA3_MAX_BLOCK_SIZE+4];
   block_t padding_blck;
   padding_blck = block_t_convert(padded_msg,0);
   sx_sha3_pad(sha3_hash_fct, data_in.len, &padding_blck);
   return sx_sha3_hash_internal(sha3_hash_fct, e_SHA3_OP_FINAL, state, &data_in, 1, padding_blck, data_out, digest_size);
}


uint32_t sx_sha3_hash_array_blk(sx_sha3_fct_t sha3_hash_fct,
                           block_t data_in[],
                           const unsigned int entries,
                           block_t data_out,
                           size_t digest_size)
{
   uint8_t padded_msg[SHA3_MAX_BLOCK_SIZE+4];
   block_t padding_blck;
   padding_blck = block_t_convert(padded_msg,0);

   uint32_t total_len = 0;
   for (unsigned int i = 0; i < entries; i++)
      total_len += data_in[i].len;

   sx_sha3_pad(sha3_hash_fct, total_len, &padding_blck);
   return sx_sha3_hash_internal(sha3_hash_fct,
         e_SHA3_OP_WHOLE,
         block_t_convert(NULL, 0),
         data_in,
         entries,
         padding_blck,
         data_out,
         digest_size);
}

