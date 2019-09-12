/**
 * @brief Defines the procedures to make operations with the BA412 DES function
 * @copyright Copyright (c) 2017-2018 Silex Insight. All Rights reserved
 * @file
 */

#include <stddef.h>
#include "sx_des.h"
#include "cryptolib_def.h"
#include "cryptodma_internal.h"
#include "sx_memcmp.h"
#include "ba412_config.h"
#include "sx_errors.h"

static uint8_t zeroes[DES_BLOCK_SIZE-1] = {0x0};

/**
 * @brief DES set mode function
 *    Set the configuration of the mode register to the required value.
 *
 * @param fct mode of operation for DES. See ::sx_aes_fct_t.
 * @param dir encrypt or decrypt. See ::sx_aes_mode_t.
 * @param mode_out pointer to the mode register value. A 32-bit word will be written.
 */
static uint32_t sw_des_setmode(sx_aes_fct_t fct, sx_aes_mode_t dir, uint32_t *mode_out)
{
   *mode_out = 0;
   // DES mode of operation
   switch(fct){
      case ECB:
         *mode_out  |= DES_MODEID_ECB;
         break;
      case CBC:
         *mode_out  |= DES_MODEID_CBC;
         break;
      case CBCMAC:
         *mode_out  |= DES_MODEID_MAC;
         break;
      default:
         return CRYPTOLIB_UNSUPPORTED_ERR;
   }

   // direction
   switch(dir){
      case ENC:
         *mode_out  |= DES_MODEID_ENCRYPT;
         break;
      case DEC:
         *mode_out  |= DES_MODEID_DECRYPT;
         break;
      default:
         return CRYPTOLIB_UNSUPPORTED_ERR;
   }

   //Hardcode Triple DES
   *mode_out |= DES_MODEID_TDES;

   return CRYPTOLIB_SUCCESS;
}

/**
 * @brief Get des padding length (realign on des block size)
 * @param input_length input length
 * @return length of padding
 */
static uint32_t get_des_pad_len(uint32_t input_length)
{
   return (DES_BLOCK_SIZE-input_length)&(DES_BLOCK_SIZE-1);
}

/**
 * @brief Build descriptors and call cryptoDMA for DES operation
 * @param config value for cfg mode register
 * @param key DES key
 * @param iv initialization vector
 * @param datain input data (plaintext or ciphertext)
 * @param dataout output data (ciphertext or plaintext)
 * @return CRYPTOLIB_SUCCESS when execution was successful
 */
static uint32_t sx_des_build_descr(  block_t *config,
                              block_t *key,
                              block_t *iv,
                              block_t *datain,
                              block_t *dataout)
{
   struct dma_sg_descr_s desc_to[12];  //could be reduces as no use case where all inputs are used, but safer like this
   struct dma_sg_descr_s desc_from[6];
   struct dma_sg_descr_s *d;  // pointer to current descriptor
   block_t keyb = *key;

   // input padding
   block_t datain_zeropad = block_t_convert(zeroes, get_des_pad_len(datain->len));

   // output discards
   block_t dataout_discard = block_t_convert(NULL, get_des_pad_len(dataout->len));

   // fetcher descriptors
   d = desc_to;
   d = write_desc_blk(
         d,
         config,
         DMA_AXI_DESCR_REALIGN,
         DMA_SG_ENGINESELECT_BA412 | DMA_SG_TAG_ISCONFIG |
         DMA_SG_TAG_SETCFGOFFSET(DES_OFFSET_CFG));
   d = write_desc_blk(
         d,
         &keyb,
         DMA_AXI_DESCR_REALIGN,
         DMA_SG_ENGINESELECT_BA412 | DMA_SG_TAG_ISCONFIG |
         DMA_SG_TAG_SETCFGOFFSET(DES_OFFSET_KEY));
   d = write_desc_blk(
         d,
         iv,
         DMA_AXI_DESCR_REALIGN,
         DMA_SG_ENGINESELECT_BA412 | DMA_SG_TAG_ISCONFIG |
         DMA_SG_TAG_SETCFGOFFSET(DES_OFFSET_IV));
   d = write_desc_blk(
         d,
         datain,
         0,
         DMA_SG_ENGINESELECT_BA412 | DMA_SG_TAG_ISDATA |
         DMA_SG_TAG_DATATYPE_AESPAYLOAD);

   d = write_desc_blk(
         d,
         &datain_zeropad,
         DMA_AXI_DESCR_REALIGN,
         DMA_SG_ENGINESELECT_BA412 | DMA_SG_TAG_ISDATA |
         DMA_SG_TAG_DATATYPE_AESPAYLOAD |
         DMA_SG_TAG_SETINVALIDBYTES(datain_zeropad.len));
   set_last_desc(d-1);

   // pusher descriptors
   d = desc_from;

   d = write_desc_blk(d, dataout, 0, 0);
   d = write_desc_blk(d, &dataout_discard, 0, 0);
   set_last_desc(d-1);

   // launch cryptodma
   cryptodma_run_sg(desc_to, desc_from);

   return CRYPTOLIB_SUCCESS;
}

#define DES_VALIDATE_INPUTS 0

#if DES_VALIDATE_INPUTS
static void sx_des_validate_input( sx_aes_fct_t fct,
                  sx_aes_mode_t dir,
                  block_t key,
                  block_t iv,
                  block_t data_in,
                  block_t data_out)
{
   // validate functions
   SX_ASSERT((fct == ECB) || (fct == CBC) || (fct == CBCMAC), "Unsupported DES mode");

   // validate direction (enc/dec)
   if (fct == CBCMAC)
      SX_ASSERT(dir == ENC, "Only encode supported by CBC-MAC");

   // validate IV input length
   uint32_t expected_iv_len = 0;
   if ((fct == CBC) || (fct == CBCMAC))
      expected_iv_len = DES_BLOCK_SIZE;
   if (fct == ECB)
      expected_iv_len = 0;
   SX_ASSERT(iv.len == expected_iv_len, "Invalid IV length");

   // validate data input/output length
   if (fct == CBCMAC)
      SX_ASSERT(data_out.len == DES_BLOCK_SIZE, "DES CBC-MAC ouputs exactly one 64-bit block");
   else
      SX_ASSERT(data_in.len >= data_out.len, "data in and data out lengths are different");
}
#endif //DES_VALIDATE_INPUTS

uint32_t sx_des_blk(sx_aes_fct_t fct,
               sx_aes_mode_t dir,
               block_t key,
               block_t iv,
               block_t data_in,
               block_t data_out)
{
#if DES_VALIDATE_INPUTS
   sx_des_validate_input(fct, dir, key, iv, data_in, data_out);
#endif //DES_VALIDATE_INPUTS

   /* Step 1: get value of config register, will also check fct, dir, ctx values and key length*/
   uint32_t ret;
   uint32_t config;
   block_t  config_blk = block_t_convert(&config, sizeof(config));
   ret = sw_des_setmode(fct, dir, &config);
   if(ret != CRYPTOLIB_SUCCESS)
      return ret;

   /* Step 2: pre-processing */

   // 2.3 handle alignment for data out
   if(data_out.flags & DMA_AXI_DESCR_CONST_ADDR)
      data_out.len = roundup_32(data_out.len);

   /* Step 3: Build descriptors and call cryptoDMA */
   ret = sx_des_build_descr(&config_blk, &key, &iv, &data_in, &data_out);
   if (ret != CRYPTOLIB_SUCCESS)
      return ret;

   return CRYPTOLIB_SUCCESS;
}
