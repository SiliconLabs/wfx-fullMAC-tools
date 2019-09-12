/**
 * @file
 * @brief Defines the procedures to make operations with
 *          the BA411E AES
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */


#include <stddef.h>
#include "sx_aes.h"
#include "cryptolib_def.h"
#include "cryptodma_internal.h"
#include "sx_memcpy.h"
#include "sx_memcmp.h"
#include "ba411e_config.h"
#include "sx_math.h"
#include "sx_trng.h"
#include <stdbool.h>
#include "sx_errors.h"

uint8_t aes_hw_key1;
uint8_t aes_hw_key2;


uint8_t zeroes[16] = {0x0};

/**
 * @brief AES set mode function
 *    Set the configuration of the mode register to the required value.
 *
 * @param aes_fct mode of operation for AES. See ::sx_aes_fct_t.
 * @param dir encrypt or decrypt. See ::sx_aes_mode_t.
 * @param ctx none, save or load context. See ::sx_aes_ctx_t.
 * @param key input key. Expect len to be 16, 24 or 32.
 * @param mode_out pointer to the mode register value. A 32-bit word will be written.
 */
static uint32_t sw_aes_setmode(sx_aes_fct_t aes_fct, sx_aes_mode_t dir, sx_aes_ctx_t ctx, block_t key, uint32_t *mode_out)
{
   *mode_out = 0;
   // AES mode of operation
    switch(aes_fct){
       #if AES_ECB_ENABLED
        case ECB:
            *mode_out  |= AES_MODEID_ECB;
            break;
        #endif
        #if AES_CBC_ENABLED
        case CBC:
            *mode_out  |= AES_MODEID_CBC;
            break;
        #endif
        #if AES_CTR_ENABLED
        case CTR:
            *mode_out  |= AES_MODEID_CTR;
            break;
        #endif
        #if AES_CFB_ENABLED
        case CFB:
            *mode_out  |= AES_MODEID_CFB;
            break;
        #endif
        #if AES_OFB_ENABLED
        case OFB:
            *mode_out  |= AES_MODEID_OFB;
            break;
        #endif
        #if AES_CCM_ENABLED
        case CCM:
            *mode_out  |= AES_MODEID_CCM;
            break;
        #endif
        #if AES_GCM_ENABLED
        case GCM:
            *mode_out  |= AES_MODEID_GCM;
            break;
        #endif
        #if AES_XTS_ENABLED
        case XTS:
            *mode_out  |= AES_MODEID_XTS;
            break;
        #endif
        #if AES_CMAC_ENABLED
        case CMAC:
            *mode_out  |= AES_MODEID_CMA;
            break;
        #endif
        default: // Should not arrive, checked in sx_aes_validate_input
            return CRYPTOLIB_UNSUPPORTED_ERR;
    }

    // context mode
    switch(ctx){
        case CTX_WHOLE:
            *mode_out  |= AES_MODEID_NO_CX;
            break;
        case CTX_BEGIN:
            *mode_out  |= AES_MODEID_CX_SAVE;
            break;
        case CTX_END:
            *mode_out  |= AES_MODEID_CX_LOAD;
            break;
        case CTX_MIDDLE:
            *mode_out  |= AES_MODEID_CX_SAVE;
            *mode_out  |= AES_MODEID_CX_LOAD;
            break;
        default: // Should not arrive, checked in sx_aes_validate_input
            return CRYPTOLIB_UNSUPPORTED_ERR;
    }

    // direction
    switch(dir){
        case ENC:
            *mode_out  |= AES_MODEID_ENCRYPT;
            break;
        case DEC:
            *mode_out  |= AES_MODEID_DECRYPT;
            break;
        case DEC_WITH_TAG:
            *mode_out  |= AES_MODEID_DECRYPT;
            break;
        default: // Should not arrive, checked in sx_aes_validate_input
            return CRYPTOLIB_UNSUPPORTED_ERR;
    }

    // keysize
    switch(key.len){
        #if AES_128_ENABLED
        case 16:
            *mode_out  |= AES_MODEID_AES128;
            break;
        #endif
        #if AES_192_ENABLED
        case 24:
            *mode_out  |= AES_MODEID_AES192;
            break;
        #endif
        #if AES_256_ENABLED
        case 32:
            *mode_out  |= AES_MODEID_AES256;
            break;
        #endif
        default: // Should not arrive, checked in sx_aes_validate_input
            return CRYPTOLIB_UNSUPPORTED_ERR;
    }

    // hardware keys
    if (key.addr==&aes_hw_key1) {
        *mode_out  |= AES_MODEID_KEY1;
    } else if (key.addr==&aes_hw_key2) {
        *mode_out  |= AES_MODEID_KEY2;
    }
    return CRYPTOLIB_SUCCESS;
}

/**
 * @brief AES generate CCM header
 *    Create the header of CCM data based on lengths, nonce & aad2
 *
 * @param nonce      The Nonce
 * @param aad_len    The length of additional data to authentify
 * @param data_len   The length of the data (plaintext or cipher) in bytes
 * @param tag_len    The length of the MAC in bytes
 * @param header     Pointer to the output header block
 * @return CRYPTOLIB_SUCCESS if execution was successful
 */
#if AES_CCM_ENABLED
static uint32_t generate_ccm_header(block_t nonce, uint32_t aad_len, uint32_t data_len, uint32_t tag_len, block_t *header)
{
   uint32_t flags;
   uint32_t m, l;
   uint32_t i;

   /* RFC3610 paragraph 2.2 defines the formatting of the first block.
    * Thee first block contains:
    *  byte  [0]           the flags byte (see below)
    *  bytes [1,1+nonce.len]   nonce
    *  bytes [2+nonce.len, 16] message length
    *
    *  The flags byte has the following bit fields:
    *    [6:7] = 0
    *    [3:5] = authentication tag size, encoded as (tag_len-2)/2
    *              only multiples of 2 between 2 and 16 are allowed.
    *    [0:2] = length field size minus 1. Is the same as (15 - nonce.len - 1)
    *         between 2 and 8.
    **/

   // Verify input parameters
   if ((tag_len & 1) || (tag_len == 2) || (tag_len > 16) ||
      (nonce.len < 7) || (nonce.len > 13)) {
      return CRYPTOLIB_INVALID_PARAM;
   }

   m = (tag_len > 0) ? (tag_len-2)/2 : 0; /* authentication tag size */
   l = 15 - nonce.len - 1;
   flags = (aad_len > 0) ? (1 << 6) : 0;
   flags |= (m & 0x7) << 3;
   flags |= (l & 0x7);
   header->addr[0] = flags;

   if (l < 4 && data_len >= (1UL << (l * 8))) {
      /* message too long to encode the size in the CCM header */
      return CRYPTOLIB_INVALID_PARAM;
   }

   memcpy_blkIn(&header->addr[1], nonce, nonce.len);

   /* append message length in big endian format */
   for (i=nonce.len+1; i < 16; i++) {
      if (i<14) {
         header->addr[i] = 0;
      } else {
         header->addr[i] = (data_len >> (l * 8)) & 0xff;
      }
      l--;
   }

   /* if there's additional authentication data (or aad2),
    * encode the size:
    *
    * 0 < aad_len < 0xFF00     => 2 bytes in big endian format.
    * 0xFF00 < aad_len < 2^32  => 0xff, 0xfe, and four bytes in big endian format.
    * eSecure currently does not support sizes bigger than 2^32.
    */
   if (aad_len > 0UL) {
      if (aad_len < 0xFF00) {
         header->addr[16] = aad_len >> 8;
         header->addr[17] = aad_len & 0xff;
         header->len = 18;
      } else {
         // TODO: Test following case
         header->addr[16] = 0xFF;
         header->addr[17] = 0xFE;
         header->addr[18] = aad_len >> 24;
         header->addr[19] = (aad_len >> 16) & 0xff;
         header->addr[20] = (aad_len >> 8) & 0xff;
         header->addr[21] = aad_len & 0xff;
         header->len = 22;
      }
   } else {
      header->len = 16;
   }

   // SX_PRINT_ARRAY(header->addr, header->len)
   return CRYPTOLIB_SUCCESS;
}
#endif

/**
 * @brief Get aes padding length (realign on aes block size)
 * @param input_length input length
 * @return length of padding
 */
static uint32_t get_aes_pad_len(uint32_t input_length)
{
   return (16-input_length)&15;
}

/**
 * @brief Build descriptors and call cryptoDMA for AES operation
 * @param config value for cfg mode register
 * @param key AES key
 * @param xtskey XTS key.
 * @param iv initialization vector
 * @param datain input data (plaintext or ciphertext)
 * @param dataout output data (ciphertext or plaintext)
 * @param aad1 additional authenticated data part #1
 * @param aad2 additional authenticated data part #2
 * @param tag_in authentication tag input for ::CCM
 * @param tag_out authentication tag input for ::CCM, ::GCM & ::CMAC
 * @param ctx_ptr AES context output
 * @param lenAlenC_blk lenA|lenC block for ::GCM mode
 * @return CRYPTOLIB_SUCCESS when execution was successful
 */
static uint32_t sx_aes_build_descr(  block_t *config,
                              block_t *key,
                              block_t *xtskey,
                              block_t *iv,
                              block_t *datain,
                              block_t *dataout,
                              block_t *aad1,
                              block_t *aad2,
                              block_t *tag_in,
                              block_t *tag_out,
                              block_t *ctx_ptr,
                              block_t *lenAlenC_blk)
{
   struct dma_sg_descr_s desc_to[9];  //could be reduces as no use case where all inputs are used, but safer like this
   struct dma_sg_descr_s desc_fr[6];
   struct dma_sg_descr_s *d;  // pointer to current descriptor
   block_t keyb = *key;
   block_t datainb = *datain;
   // input padding
   uint32_t aad_zeropad_len      = get_aes_pad_len(aad1->len + aad2->len);
   uint32_t datain_zeropad_len   = get_aes_pad_len(datain->len);
   uint32_t tagin_zeropad_len    = get_aes_pad_len(tag_in->len);

   // output discards
   block_t aads_discard      = block_t_convert(NULL, aad1->len + aad2->len + aad_zeropad_len);
   block_t dataout_discard   = block_t_convert(NULL, get_aes_pad_len(dataout->len));
   block_t tagout_discard    = block_t_convert(NULL, get_aes_pad_len(tag_out->len));

   // no input provided, -> empty input = 1 block of zero padding (i.e. for CMAC)
   if (!datain->len && !tag_in->len && !lenAlenC_blk->len && !aad1->len &&  !aad2->len ) {
      datain_zeropad_len = 16;
      datainb = block_t_convert(zeroes, datain_zeropad_len);
   }
   //do not transfer hardware keys
   if(keyb.addr == &aes_hw_key1 || keyb.addr == &aes_hw_key2)
      keyb.len = 0;

   // fetcher descriptors
   d = desc_to;

   // Config
   d = write_desc_blk(
         d,
         config,
         DMA_AXI_DESCR_REALIGN,
         DMA_SG_ENGINESELECT_BA411E | DMA_SG_TAG_ISCONFIG |
         DMA_SG_TAG_SETCFGOFFSET(AES_OFFSET_CFG));

   // Symmetric key
   d = write_desc_blk(
         d,
         &keyb,
         DMA_AXI_DESCR_REALIGN,
         DMA_SG_ENGINESELECT_BA411E | DMA_SG_TAG_ISCONFIG |
         DMA_SG_TAG_SETCFGOFFSET(AES_OFFSET_KEY));

   // IV or context (if existing)
   d = write_desc_blk(
         d,
         iv,
         DMA_AXI_DESCR_REALIGN,
         DMA_SG_ENGINESELECT_BA411E | DMA_SG_TAG_ISCONFIG |
         DMA_SG_TAG_SETCFGOFFSET(AES_OFFSET_IV));

   // XTS key (if existing)
   d = write_desc_blk(
         d,
         xtskey,
         DMA_AXI_DESCR_REALIGN,
         DMA_SG_ENGINESELECT_BA411E | DMA_SG_TAG_ISCONFIG |
         DMA_SG_TAG_SETCFGOFFSET(AES_OFFSET_KEY2));

   // authentification data (if existing)
   d = write_desc_blk(
         d,
         aad1,
         0,
         DMA_SG_ENGINESELECT_BA411E | DMA_SG_TAG_ISDATA |
         DMA_SG_TAG_DATATYPE_AESHEADER);
   d = write_desc_blk(
         d,
         aad2,
         DMA_AXI_DESCR_REALIGN,
         DMA_SG_ENGINESELECT_BA411E | DMA_SG_TAG_ISDATA |
         DMA_SG_TAG_DATATYPE_AESHEADER |
         DMA_SG_TAG_SETINVALIDBYTES(aad_zeropad_len));

   // Input data (if existing)
   d = write_desc_blk(
         d,
         &datainb,
         DMA_AXI_DESCR_REALIGN,
         DMA_SG_ENGINESELECT_BA411E | DMA_SG_TAG_ISDATA |
         DMA_SG_TAG_DATATYPE_AESPAYLOAD |
         DMA_SG_TAG_SETINVALIDBYTES(datain_zeropad_len));

   // Input tag (if existing)
   d = write_desc_blk(
         d,
         tag_in,
         DMA_AXI_DESCR_REALIGN,
         DMA_SG_ENGINESELECT_BA411E | DMA_SG_TAG_ISDATA |
         DMA_SG_TAG_DATATYPE_AESPAYLOAD |
         DMA_SG_TAG_SETINVALIDBYTES(tagin_zeropad_len));
   d = write_desc_blk(
         d,
         lenAlenC_blk,
         0,
         DMA_SG_ENGINESELECT_BA411E | DMA_SG_TAG_ISDATA |
         DMA_SG_TAG_DATATYPE_AESPAYLOAD | DMA_SG_TAG_DATATYPE_AESPAYLOAD);

   set_last_desc(d - 1);

   // pusher descriptors
   d = desc_fr;

   // discard output aad and associated padding
   d = write_desc_blk(d, &aads_discard, 0, 0);

   // Output data
   d = write_desc_blk(d, dataout, 0, 0);
   d = write_desc_blk(d, &dataout_discard, 0, 0);

   // Output tag (if existing)
   d = write_desc_blk(d, tag_out, 0, 0);
   d = write_desc_blk(d, &tagout_discard, 0, 0);

   // Output context (if existing)
   d = write_desc_blk(d, ctx_ptr, 0, 0);

   set_last_desc(d - 1);

   // launch cryptodma
   cryptodma_run_sg(desc_to, desc_fr);

   return CRYPTOLIB_SUCCESS;
}


void sx_aes_load_mask(uint32_t value)
{
   struct dma_sg_descr_s desc_to;
   struct dma_sg_descr_s desc_fr;

   // Fetcher descriptor to store random in AES
   write_desc(
         &desc_to,
         &value,
         sizeof(value),
         DMA_AXI_DESCR_REALIGN,
         DMA_SG_ENGINESELECT_BA411E | DMA_SG_TAG_ISCONFIG |
         DMA_SG_TAG_SETCFGOFFSET(AES_OFFSET_MASK));
   set_last_desc(&desc_to);

   // Dummy pusher descriptor
   write_desc_always(&desc_fr, NULL, 0, DMA_AXI_DESCR_REALIGN, 0);
   set_last_desc(&desc_fr);

   // RUN
   cryptodma_run_sg(&desc_to, &desc_fr);
}

/**
 * @brief Verify the AES key (and in case of ::XTS, the key2) length are valid.
 * @param fct mode of operation of AES. Used only for ::XTS.
 * @param len length of the key (first key in ::XTS)
 * @param xts_len length of the secondary key used in ::XTS
 * @return true if key(s) length is/are valid.
 */
static bool IsKeyLenValid(sx_aes_fct_t fct, size_t len, size_t xts_len)
{
   bool valid = len == 16 || len == 24 || len == 32;
   if (fct == XTS)
      return valid && len == xts_len;
   return valid && xts_len == 0;
}

/**
 * @brief Verify that the size of the IV or the context passed to the BA411 is valid
 * @param fct mode of operation of AES, it determines with the context the expected length
 * @param ctx current state for the context, it determines with the mode the expected length
 * @param len length of the IV for full message (::CTX_WHOLE) or for beginning of message (::CTX_BEGIN), context for oters cases (::CTX_MIDDLE or ::CTX_END)
 * @return true if IV/context length is valid.
 */
static bool IsIVContextLenValid(sx_aes_fct_t fct, sx_aes_ctx_t ctx, size_t len)
{
   // Use a context switching, save it and add it later is mathematically equal
   // to the IV mechanism. So, at first iteration, it is a "true" IV which is
   // injected in the AES. For the following iteration the context replaces the
   // IV but works exactly the same way
   switch (fct)
   {
   case ECB:
      return !len;
   case CMAC:
      if (ctx == CTX_BEGIN || ctx == CTX_WHOLE)
         return !len;
      break;
   case CCM:
      if (ctx == CTX_BEGIN || ctx == CTX_WHOLE)
         return !len;
      return len == AES_CTX_xCM_SIZE;
   case GCM:
      if (ctx == CTX_BEGIN || ctx == CTX_WHOLE)
         return len == AES_IV_GCM_SIZE;
      return len == AES_CTX_xCM_SIZE;
   default:
      break;
   }

   return len == AES_IV_SIZE;
}

/**
 * @brief Verify that the next-context size to read from BA411 is valid
 * @param fct mode of operation of AES, it determines with the context the expected length
 * @param ctx current state for the context, it determines with the mode the expected length
 * @param len length of the next context
 * @return true if next context length is valid.
 */
static bool IsNextContextLenValid(sx_aes_fct_t fct, sx_aes_ctx_t ctx, size_t len)
{
   if (fct == ECB || ctx == CTX_END || ctx == CTX_WHOLE)
      return !len;
   else if (fct == GCM || fct == CCM)
      return len == AES_CTX_xCM_SIZE;
   return len == AES_CTX_SIZE;
}

/**
 * @brief Verify that the Nonce size is valid
 * @param fct mode of operation of AES, it determines with the context the expected length
 * @param ctx current state for the context, it determines with the mode the expected length
 * @param len length of the Nonce
 * @return true if Nonce length is valid.
 */
static bool IsNonceLenValid(sx_aes_fct_t fct, sx_aes_ctx_t ctx, size_t len)
{
   if (fct == GCM && (ctx == CTX_END /*|| ctx == CTX_WHOLE*/))
      return len == 16;
   else if (fct == CCM)
      return len >= 7 && len <= 13;
   return !len;
}

/**
 * @brief Verify that the data (payload) size is valid
 * @param fct mode of operation of AES, it determines with the context the expected length
 * @param ctx current state for the context, it determines with the mode the expected length
 * @param len length of the payload
 * @return true if payload length is valid.
 */
static bool IsPayloadLenValid(sx_aes_fct_t fct, sx_aes_ctx_t ctx, size_t len)
{
   // Context check
   if (ctx == CTX_BEGIN || ctx == CTX_MIDDLE) {
      if (fct != GCM)
         return len && !(len % 16);
   }
   else if (!len) {
      if (ctx == CTX_END && fct != CTR && fct != CMAC && fct != GCM)
      return false;
      else if (ctx == CTX_WHOLE && fct != CCM && fct != GCM && fct != CMAC)
         return false;
   }

   // Mode check
   if (fct == ECB || fct == OFB || fct == CFB)
      return !(len % 16) && len;
   else if (fct == CBC || fct == XTS) {
      if (ctx == CTX_WHOLE)
         return len >= 16;
      return len;
   }

   return true;
}

/**
 * @brief Verify that the tag (MAC) size is valid
 * @param fct mode of operation of AES, it determines with the context the expected length
 * @param ctx current state for the context, it determines with the mode the expected length
 * @param len length of the payload
 * @return true if MAC length is valid.
 */
static bool IsTagLenValid(sx_aes_fct_t fct, sx_aes_ctx_t ctx, size_t len)
{
   if(fct == GCM || fct == CMAC) {
      if (len != 0 && (ctx == CTX_BEGIN || ctx == CTX_MIDDLE))
         return false;
      if (len != 16  && (ctx == CTX_END || ctx == CTX_WHOLE))
         return false;
   }

   //CCM tag E [0,4,6,8,10,12,14,16]
   // check tag is even and remove 2 from even serie
   else if (fct == CCM) {
      if ((len % 2 != 0) || (len == 2) /*|| (!len) */|| (len > 16))
         return false;
   } else if (len)
      return false;

   return true;
}

/**
 * @brief Verify the differents inputs of sx_aes_blk
 * @param dir specify the direction of aes, in encryption or in decryption
 * @param fct mode of operation of AES, it determines with the context the expected lengths
 * @param ctx current state for the context, it determines with the mode the expected lengths
 * @param key pointer to the AES key.
 * @param xtskey pointer to the XTS key.
 * @param iv pointer to initialization vector. Used for \p aes_fct ::CBC, ::CTR, ::CFB, ::OFB, ::XTS and ::GCM, mode of operation. 16 bytes will be read.
 * @param data_in pointer to input data (plaintext or ciphertext).
 * @param aad pointer to additional authenticated data. Used for \p aes_fct ::GCM mode of operation.
 * @param tag pointer to the authentication tag. Used for ::GCM mode of operation. 16 bytes will be written.
 * @param ctx_ptr pointer to the AES context (after operation)
 * @param nonce_len_blk pointer to the lenAlenC data (AES GCM context switching only) or the AES CCM Nonce
 * @return ::CRYPTOLIB_SUCCESS if inputs are all valid, otherwise ::CRYPTOLIB_INVALID_PARAM.
 */
static uint32_t sx_aes_validate_input(sx_aes_mode_t dir,
                  sx_aes_fct_t fct,
                  sx_aes_ctx_t ctx,
                  block_t key,
                  block_t xtskey,
                  block_t iv,
                  block_t data_in,
                  block_t aad,
                  block_t tag,
                  block_t ctx_ptr,
                  block_t nonce_len_blk)
{
   if (dir != ENC && dir != DEC && dir != DEC_WITH_TAG)
      return CRYPTOLIB_INVALID_PARAM;

   if (!IsKeyLenValid(fct, key.len, xtskey.len))
      return CRYPTOLIB_INVALID_PARAM;

   if (!IsIVContextLenValid(fct, ctx, iv.len))
      return CRYPTOLIB_INVALID_PARAM;

   if (!IsNextContextLenValid(fct, ctx, ctx_ptr.len))
      return CRYPTOLIB_INVALID_PARAM;

   if (!IsNonceLenValid(fct, ctx, nonce_len_blk.len))
      return CRYPTOLIB_INVALID_PARAM;

   if (fct != GCM && fct != CCM && aad.len != 0)
      return CRYPTOLIB_INVALID_PARAM;

   if (!IsPayloadLenValid(fct, ctx, data_in.len))
      return CRYPTOLIB_INVALID_PARAM;

   if (!IsTagLenValid(fct, ctx, tag.len))
      return CRYPTOLIB_INVALID_PARAM;

   return CRYPTOLIB_SUCCESS;
}


uint32_t sx_aes_blk( sx_aes_fct_t aes_fct,
               sx_aes_mode_t dir,
               sx_aes_ctx_t ctx,
               block_t key,
               block_t xtskey,
               block_t iv,
               block_t data_in,
               block_t data_out,
               block_t aad,
               block_t tag,
               block_t ctx_ptr,
               block_t nonce_len_blk)
{
   /* Step 0: input validation */
   uint32_t ret = sx_aes_validate_input(dir, aes_fct, ctx, key, xtskey, iv, data_in,
         aad, tag, ctx_ptr, nonce_len_blk);
   if (ret)
      return ret;

   /* Step 1: get value of config register, will also check aes_fct, dir, ctx values and key length*/
   uint32_t config = 0;
   ret = sw_aes_setmode(aes_fct, dir, ctx, key, &config);
   if(ret != CRYPTOLIB_SUCCESS)
      return ret;

   /* Step 2: pre-processing for authenticated modes */
   // 2.1a Generate CCM header
   block_t header = NULL_blk;

   #if AES_CCM_ENABLED
   uint8_t header_generated[22]; // Maximum header size (excluding aad2 which will be transfered directly)
   if (aes_fct == CCM) {
      header = block_t_convert(header_generated, sizeof(header_generated));
      // Generate the header message
      ret = generate_ccm_header(nonce_len_blk, aad.len, data_in.len, tag.len, &header);
      if (ret != CRYPTOLIB_SUCCESS)
         return ret;
   }
   #endif

   // 2.1b Generate GCM lenAlenC block
   block_t lenAlenC_blk = NULL_blk;

   uint8_t lenAlenC[16];
   if ((aes_fct == GCM) && (ctx == CTX_WHOLE || ctx == CTX_END)) {
      if (ctx == CTX_WHOLE) {
         // build lenAlenC block as big endian byte array
         sx_math_u64_to_u8array(aad.len<<3, &lenAlenC[0], sx_big_endian);
         sx_math_u64_to_u8array(data_in.len<<3,   &lenAlenC[8], sx_big_endian);
         lenAlenC_blk = block_t_convert(lenAlenC, 16);
      } else {
         // With context switching, lenAlenC is to be provided by the host just after the data
         lenAlenC_blk = nonce_len_blk;
      }
   }

   // 2.2 prepare tag input and output depending on the modes and context
   block_t tag_in  = NULL_blk;
   block_t tag_out = NULL_blk;
   block_t tag_ref = NULL_blk;
   uint8_t tag_generated[16];
   if( (ctx == CTX_WHOLE || ctx == CTX_END) && tag.len && (aes_fct == CCM || aes_fct == GCM || aes_fct == CMAC) ) {
      if(dir == ENC) {
         //encrypt -> output MAC
         tag_out = tag;

      } else {
         // decrypt -> store output MAC
         tag_out = block_t_convert(tag_generated, tag.len);
         if(aes_fct == CCM) {
            // for CCM, MAC should be input of the ba411e core, and it returns zeroes if it's valid
            tag_in = tag;
            tag_ref = block_t_convert(zeroes, tag.len);
         } else {
            // for others, MAC is an output, that should be compare with reference tag
            tag_ref = tag;
         }

      }
   }

   // 2.3 handle alignment for data out
   if(data_out.flags & DMA_AXI_DESCR_CONST_ADDR)
      data_out.len = roundup_32(data_out.len);

   if(tag_out.flags & DMA_AXI_DESCR_CONST_ADDR)
      tag_out.len = roundup_32(tag_out.len);

   /* Step 3: Build descriptors and call cryptoDMA */
   block_t  config_blk = block_t_convert(&config, sizeof(config));
   ret = sx_aes_build_descr(&config_blk, &key, &xtskey, &iv, &data_in, &data_out, &header, &aad, &tag_in, &tag_out, &ctx_ptr, &lenAlenC_blk);
   if (ret != CRYPTOLIB_SUCCESS)
      return ret;

   /* Step 4: post-processing of MAC if necessary (authenticated modes in decrypt mode) */
   if( tag_ref.len ) {
      if (dir != DEC_WITH_TAG){
         uint8_t tag_cpy[16];
         memcpy_blkIn(tag_cpy, tag_ref, tag_ref.len);
         uint32_t error = memcmp_time_cst(tag_out.addr, tag_cpy, tag_ref.len);
         if (error)
            return CRYPTOLIB_INVALID_SIGN_ERR;
      } else {
         memcpy_blk(tag, tag_out, tag_out.len);
      }
   }

   return CRYPTOLIB_SUCCESS;
}
