/**
 * @file
 * @brief Defines the procedures to make operations with
 *          the BA413 hash function
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */


#include "sx_hash.h"
#include <string.h>
#include <stddef.h>
#include "cryptolib_def.h"
#include "cryptodma_internal.h"
#include "sx_memcpy.h"
#include "ba413_config.h"
#include "sx_errors.h"


/* Internal functions */

#define OP_FULL_HASH 1
#define OP_FULL_HMAC 2
#define OP_PART_HASH 3

/**
 * @brief internal function for hash operation
 * @param hash_fct hash function to use. See ::sx_hash_fct_t.
 * @param extra_in input K for OP_FULL_HMAC, or state for OP_PART_HASH, unused for OP_FULL_HASH
 * @param operation_type define type of operation to perform
 * @param data_in array of input data to process
 * @param entries length of array \p data_in
 * @param data_out output digest or state
 * @return ::CRYPTOLIB_SUCCESS when execution was successful
 */
static uint32_t sx_hash_internal(sx_hash_fct_t hash_fct,
                                 block_t extra_in,
                                 uint8_t operation_type,
                                 block_t data_in[],
                                 const unsigned int entries,
                                 block_t data_out)
{
   uint32_t total_len;
   uint32_t ign_bytes;
   uint32_t extra_in_tag;
   struct ba413_regs_s info;
   struct dma_sg_descr_s desc_in[SX_HASH_ARRAY_MAX_ENTRIES + 2];
   struct dma_sg_descr_s desc_out[2];
   struct dma_sg_descr_s *current_desc;
   uint32_t genlen, outlen;
   block_t extra = extra_in;

   CRYPTOLIB_ASSERT(entries <= SX_HASH_ARRAY_MAX_ENTRIES, "Too many entries in data array");

   switch (hash_fct) {
   case e_MD5:
      if (!MD5_ENABLED)
         return CRYPTOLIB_UNSUPPORTED_ERR;
      info.config = BA413_CONF_MODE_MD5;
      break;
   case e_SHA1:
      if (!SHA1_ENABLED)
         return CRYPTOLIB_UNSUPPORTED_ERR;
      info.config = BA413_CONF_MODE_SHA1;
      break;
   case e_SHA224:
      if (!SHA224_ENABLED)
         return CRYPTOLIB_UNSUPPORTED_ERR;
      info.config = BA413_CONF_MODE_SHA224;
      break;
   case e_SHA256:
      if (!SHA256_ENABLED)
         return CRYPTOLIB_UNSUPPORTED_ERR;
      info.config = BA413_CONF_MODE_SHA256;
      break;
   case e_SHA384:
      if (!SHA384_ENABLED)
         return CRYPTOLIB_UNSUPPORTED_ERR;
      info.config = BA413_CONF_MODE_SHA384;
      break;
   case e_SHA512:
      if (!SHA512_ENABLED)
         return CRYPTOLIB_UNSUPPORTED_ERR;
      info.config = BA413_CONF_MODE_SHA512;
      break;
   case e_SM3:
      if (!SM3_ENABLED)
         return CRYPTOLIB_UNSUPPORTED_ERR;
      info.config = BA413_CONF_MODE_SM3;
      break;
   default:
      return CRYPTOLIB_INVALID_PARAM;
   }

   // Could be 4B for padding but due to both enabled stack protection and
   // -Werror, it has to be 8B minimum.
   uint8_t discarded_buf[8] = {0};
   switch (operation_type) {
   // complete hash operation: no need to load extra info, enable padding in
   // hardware, output will be digest
   case OP_FULL_HASH:
      info.config |= BA413_CONF_HWPAD | BA413_CONF_FINAL;
      extra_in.len = 0;
      extra_in_tag = DMA_SG_ENGINESELECT_BA413;
      genlen = sx_hash_get_digest_size(hash_fct);
      break;

   // complete hmac operation: need to load extra info K, enable padding in
   // hardware, output will be digest
   case OP_FULL_HMAC:
   {
      uint32_t extra_ign_bytes;
      if (extra_in.len) {
         extra_ign_bytes = -extra_in.len&0x3;
      } else {
         extra_ign_bytes = 4;
         // In case of null key (which may come from a FIFO), we provide a local
         // buffer to the DMA with appropriate extra_ign_bytes
         extra = block_t_convert(discarded_buf, extra_ign_bytes);
      }
      info.config |= BA413_CONF_HMAC | BA413_CONF_HWPAD | BA413_CONF_FINAL;
      extra_in_tag = DMA_SG_TAG_DATATYPE_HASHKEY | DMA_SG_ENGINESELECT_BA413 |
                     DMA_SG_TAG_ISDATA | DMA_SG_TAG_ISLAST |
                     DMA_SG_TAG_SETINVALIDBYTES(extra_ign_bytes);
      genlen = sx_hash_get_digest_size(hash_fct);
      break;
   }

   // partial hash operation: need to load initial state, don't enable padding in
   // hardware, output will be state
   case OP_PART_HASH:
   default:
      extra_in_tag = DMA_SG_TAG_DATATYPE_HASHINIT | DMA_SG_ENGINESELECT_BA413 |
                     DMA_SG_TAG_ISDATA | DMA_SG_TAG_ISLAST;
      genlen = sx_hash_get_state_size(hash_fct);
   }

   // configuration (to register)
   current_desc = &desc_in[0];
   current_desc = write_desc(
         current_desc,
         &info,
         sizeof(info),
         DMA_AXI_DESCR_REALIGN,
         DMA_SG_ENGINESELECT_BA413 | DMA_SG_TAG_ISCONFIG);

   // K or init value
   current_desc = write_desc_always_blk(
         current_desc,
         &extra,
         DMA_AXI_DESCR_REALIGN,
         extra_in_tag);

   // datas
   uint32_t i = 0;
   total_len = 0;
   for (i = 0; i < entries; i++) {
      total_len += data_in[i].len;
      current_desc = write_desc_blk(
            current_desc,
            &data_in[i],
            0,
            DMA_SG_ENGINESELECT_BA413 | DMA_SG_TAG_ISDATA | DMA_SG_TAG_DATATYPE_HASHMSG);
   }

   if (total_len == 0) {
      ign_bytes = 4;
      /* DMA_SG_TAG_ISDATA must be present for each part of data, even for
       * an empty message. That's why in case of HMAC without data (empty
       * message), we artificially build a descriptor with zero data length
       * and insert it before the last descriptor.
       * To summarize, for HMAC, there are 2 descriptors tagged with IS_DATA,
       * one for K and the second which could be without data associated in
       * case of empty message.
       */
      if (operation_type == OP_FULL_HMAC) {
         current_desc = write_desc_always(
               current_desc,
               NULL,
               0,
               0,
               DMA_SG_ENGINESELECT_BA413 | DMA_SG_TAG_ISDATA | DMA_SG_TAG_DATATYPE_HASHMSG);
      }
   } else if (total_len & 0x3) // not word aligned
      ign_bytes = (4 - (total_len & 0x3));
   else
      ign_bytes = 0;

   // last descriptor
   --current_desc;
   set_last_desc(current_desc);
   set_desc_invalid_bytes(current_desc, ign_bytes);

   // ouput digest or state
   outlen = SX_MIN(genlen, data_out.len);
   current_desc = &desc_out[0];
   current_desc = write_desc( // Can we speculate on outlen value ?
         desc_out,
         data_out.addr,
         outlen,
         data_out.flags,
         0);

   // discard unused
   write_desc_always(
         current_desc,
         NULL,
         (genlen - outlen),
         0,
         0);
   set_last_desc(current_desc);

   // launch cryptodma operation
   cryptodma_run_sg(desc_in, desc_out);
   // if error occurs, hardfault should be trigger


   return CRYPTOLIB_SUCCESS;
}

/**
 * @brief internal function for SHA2 message padding
 * @param hash_fct hash function to use. See ::sx_hash_fct_t.
 * @param data_len length of the input message to be padded
 * @param total_len length of all data hashed including the data hashed before using update and including data_len
 * @param padding output for padding, length must be equal or bigger than padding length
 * @return padsize length of the padding to be added
 */
static uint32_t sx_hash_pad(sx_hash_fct_t hash_fct, uint32_t data_len, size_t total_len, block_t padding)
{
   uint32_t block_size = sx_hash_get_block_size(hash_fct);
   uint32_t length_field_size = (hash_fct >= e_SHA384) ? 16 : 8;
   uint32_t padsize;

   /* As block_size is a power of 2, the modulo computation has been converted
    * into a binary and operation. This avoids a division operation which can
    * be very slow on some chips. */
   padsize = block_size - (data_len & (block_size-1));
   if (padsize < (length_field_size + 1))
      padsize += block_size;
   if (padding.len < padsize)
      return 0;
   /* first bit of padding should be 1 */
   padding.addr[0] = 0x80;
   /* followed by zeros until the length field. */
   for (size_t i = 1; i < padsize; i++)
      padding.addr[i] = 0;
   /* write number of bits at end of padding in big endian order */
   size_t start = length_field_size;
   if (start > sizeof(total_len))
      start = sizeof(total_len);
   for (size_t i = start; i; i--) {
      padding.addr[padsize - i - 1] = (total_len >> (i * 8 - 3)) & 0xFF;
   }
   padding.addr[padsize - 1] = (total_len & 0x1F) << 3;

   return padsize;
}

/* Public functions: properties */
uint32_t sx_hash_get_digest_size(sx_hash_fct_t hash_fct)
{
   switch (hash_fct) {
   case e_MD5:
      return MD5_DIGESTSIZE;
   case e_SHA1:
      return SHA1_DIGESTSIZE;
   case e_SHA224:
      return SHA224_DIGESTSIZE;
   case e_SHA256:
      return SHA256_DIGESTSIZE;
   case e_SHA384:
      return SHA384_DIGESTSIZE;
   case e_SHA512:
      return SHA512_DIGESTSIZE;
   case e_SM3:
      return SM3_DIGESTSIZE;
   default:
      return 0;
   }
}

uint32_t sx_hash_get_block_size(sx_hash_fct_t hash_fct)
{
   switch (hash_fct) {
   case e_MD5:
      return MD5_BLOCKSIZE;
   case e_SHA1:
      return SHA1_BLOCKSIZE;
   case e_SHA224:
      return SHA224_BLOCKSIZE;
   case e_SHA256:
      return SHA256_BLOCKSIZE;
   case e_SHA384:
      return SHA384_BLOCKSIZE;
   case e_SHA512:
      return SHA512_BLOCKSIZE;
   case e_SM3:
      return SM3_BLOCKSIZE;
   default:
      return 0;
   }
}

uint32_t sx_hash_get_state_size(sx_hash_fct_t hash_fct)
{

   switch (hash_fct) {
   case e_MD5:
      return MD5_INITSIZE;
   case e_SHA1:
      return SHA1_INITSIZE;
   case e_SHA224:
      return SHA224_INITSIZE;
   case e_SHA256:
      return SHA256_INITSIZE;
   case e_SHA384:
      return SHA384_INITSIZE;
   case e_SHA512:
      return SHA512_INITSIZE;
   case e_SM3:
      return SM3_INITSIZE;
   default:
      return 0;
   }
}

/* Public functions: process arrays of blocks */
uint32_t sx_hash_array_blk(sx_hash_fct_t hash_fct,
                           block_t data_in[],
                           const unsigned int entries,
                           block_t data_out)
{
   return sx_hash_internal(hash_fct, block_t_convert(NULL, 0), OP_FULL_HASH,
                           data_in, entries, data_out);
}

#if HMAC_ENABLED
uint32_t sx_hmac_array_blk(sx_hash_fct_t hash_fct,
                           block_t key,
                           block_t data_in[],
                           const unsigned int entries,
                           block_t data_out)
{
   return sx_hash_internal(hash_fct, key, OP_FULL_HMAC,
                           data_in, entries, data_out);
}
#endif

/* Public functions: process  blocks */
uint32_t sx_hash_update_blk(sx_hash_fct_t hash_fct, block_t state, block_t data)
{
   return sx_hash_internal(hash_fct, state, OP_PART_HASH, &data, 1, state);
}

uint32_t sx_hash_finish_blk(sx_hash_fct_t hash_fct, block_t state, block_t data_in, block_t data_out, uint32_t total_len)
{
   uint8_t padding[MAX_BLOCKSIZE+16+1];
   block_t padding_blk = block_t_convert(padding, sizeof(padding));

   int padding_len = sx_hash_pad(hash_fct, data_in.len, total_len, padding_blk);
   padding_blk.len = padding_len;

   block_t input_padding[] = {data_in, padding_blk};

   return sx_hash_internal(hash_fct, state, OP_PART_HASH, input_padding, 2, data_out);
}
uint32_t sx_hash_blk(sx_hash_fct_t hash_fct, block_t data_in, block_t data_out)
{
   return sx_hash_array_blk(hash_fct, &data_in, 1, data_out);
}

#if HMAC_ENABLED
uint32_t sx_hmac_blk(sx_hash_fct_t hash_fct, block_t key, block_t data_in, block_t data_out)
{
   return sx_hmac_array_blk(hash_fct, key, &data_in, 1, data_out);
}
#endif
