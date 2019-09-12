#include "sx_derive_key_alg.h"
#include "cryptolib_def.h"
#include "sx_memcpy.h"
#include "sx_hash.h"
#include "sx_errors.h"

/* Converting a value to a big endian octet string */
static void uint32_to_octet_string(uint32_t value, uint8_t buffer[4])
{
   uint8_t i;
   for (i = 0; i < 4; i++) {
      buffer[i] = (uint8_t)((value >> (8 * (3 - i))) & 0xFF);
   }
}


uint32_t sx_derive_key_hkdf(sx_hash_fct_t hash_fct, block_t ikm, block_t salt, block_t info_in, uint32_t length, block_t okm)
{
   uint8_t prkBuff[MAX_DIGESTSIZE]; //Contains the extracted key
   uint8_t tmpBuff[MAX_DIGESTSIZE]; //Contains the t blocks between iterations
   uint8_t infoBuff[DERIV_MAX_INFO_SIZE]; //Contains the info (used for every loop)
   uint8_t cntBuff = 0;
   block_t output = okm;

   uint32_t status;
   block_t prk, t, bytes, info;
   uint32_t blkSize = sx_hash_get_digest_size(hash_fct);

   if (info_in.len > DERIV_MAX_INFO_SIZE)
      /* There is not enough memory allocated to support "caching" of the
       * param info in memory. */
      return CRYPTOLIB_UNSUPPORTED_ERR;

   prk = block_t_convert(prkBuff, MAX_DIGESTSIZE);
   t = block_t_convert(tmpBuff, 0);

   bytes = block_t_convert(&cntBuff, sizeof(cntBuff));
   info = block_t_convert(infoBuff, DERIV_MAX_INFO_SIZE);


   block_t data[3];
   data[0] = ikm;
   status = sx_hmac_array_blk(hash_fct, salt, data, 1, prk);
   if (status != CRYPTOLIB_SUCCESS)
      return status;
   prk.len = blkSize;

   //Copy info_in
   memcpy_blk(info, info_in, info_in.len);
   info.len = info_in.len;

   uint32_t i = 0;
   for (i = 0; i < length; i+=blkSize) {
      cntBuff++;

      data[0] = t;
      data[1] = info;
      data[2] = bytes;

      t.len = blkSize;
      status = sx_hmac_array_blk(hash_fct, prk, data, 3, t);
      if (status != CRYPTOLIB_SUCCESS)
         return status;

      block_t_adapt_len(&output);    // Round-up needed for Internal DMA targetting a FIFO
      memcpy_blk(output, t, blkSize);
      output.len -= blkSize;
      if (!(output.flags & BLOCK_S_CONST_ADDR)) output.addr += blkSize;
   }

   return status;
}


static void xorbuf(block_t dk, block_t u)
{
   uint32_t i = 0;
   // FIXME Quid if unaligned on 32b ?
   for (i = 0; i < u.len; i += 4) {
      *((uint32_t*)(dk.addr + i)) = *((uint32_t*)(dk.addr + i))^(*(uint32_t*)(u.addr + i));
   }
}

uint32_t sx_derive_key_pbkdf2(sx_hash_fct_t hash_fct, block_t password, block_t salt,
   uint32_t iterations, uint32_t dkLen, block_t dk)
{
   uint8_t uTmpBuff[MAX_BLOCKSIZE];
   uint8_t dkBuff[MAX_BLOCKSIZE]; //Contains the output during computations
   uint8_t saltBuff[DERIV_MAX_SALT_SIZE];

   uint8_t itmp[4] = {0}; //Contains i, the increment at each step
   block_t data[2];

   block_t uTmp, dkTmp, bytes, salt2;
   uint32_t hLen = sx_hash_get_digest_size(hash_fct);
   uint32_t status = 0;
   block_t output = dk;

   uTmp = block_t_convert(uTmpBuff, hLen);
   dkTmp = block_t_convert(dkBuff, hLen);
   bytes = block_t_convert(itmp, sizeof(itmp));

   // Copy salt to buffer
   if (salt.len > DERIV_MAX_SALT_SIZE)
      /* There is not enough memory allocated to support "caching" of the
       * param info in memory. */
      return CRYPTOLIB_UNSUPPORTED_ERR;

   salt2 = block_t_convert(saltBuff, salt.len);
   memcpy_blk(salt2, salt, salt.len);
   salt = salt2;

   uint32_t currentLen = 0;
   uint32_t i = 1;
   while (currentLen < dkLen) {
      data[0] = salt;

      uint32_to_octet_string(i, itmp);
      data[1] = bytes;

      //First Block (U_1)
      status = sx_hmac_array_blk(hash_fct, password, data, 2, uTmp);
      if (status != CRYPTOLIB_SUCCESS)
         return status;
      memcpy_blk(dkTmp, uTmp, hLen);

      //U_j
      uint32_t j;
      for (j = 1; j < iterations; j++) {
         data[0] = uTmp;
         status = sx_hmac_array_blk(hash_fct, password, data, 1, uTmp);
         if (status != CRYPTOLIB_SUCCESS)
            return status;
         xorbuf(dkTmp, uTmp);
      }
      currentLen += hLen;

      block_t_adapt_len(&output);    // Round-up needed for Internal DMA targetting a FIFO
      memcpy_blk(output, dkTmp, hLen);
      output.len -= hLen;
      if(!(output.flags & BLOCK_S_CONST_ADDR)) output.addr += hLen;
      i++;
   }



   return status;
}

uint32_t sx_derive_key_kdf2(const sx_hash_fct_t hash_fct, block_t secret,
      block_t param, block_t derived_key)
{
   uint32_t digest_size = sx_hash_get_digest_size(hash_fct);
   uint8_t digest_buffer[MAX_DIGESTSIZE];
   block_t digest = block_t_convert(digest_buffer, digest_size);
   uint8_t param_buffer[DERIV_MAX_INFO_SIZE];
   block_t param_copy = block_t_convert(param_buffer, param.len);
   uint8_t counter_buffer[4];
   block_t data[3];
   uint32_t error = CRYPTOLIB_SUCCESS;
   uint32_t copy_length = 0;

   if ((secret.flags & BLOCK_S_CONST_ADDR) && (derived_key.len > digest_size)) {
      return CRYPTOLIB_UNSUPPORTED_ERR;
   }

   data[0] = secret;
   data[1] = block_t_convert(counter_buffer, sizeof(counter_buffer));
   if ((param.flags & BLOCK_S_CONST_ADDR) && (derived_key.len > digest_size)) {
      if (param.len > DERIV_MAX_INFO_SIZE) {
         /* There is not enough memory allocated to support "caching" of the
          * param info in memory. */
         return CRYPTOLIB_UNSUPPORTED_ERR;
      } else {
         memcpy_blk(param_copy, param, param.len);
         data[2] = param_copy;
      }
   } else {
      data[2] = param;
   }

   uint32_t remaining_length = derived_key.len;
   uint32_t counter = 1;
   while (remaining_length > 0) {
      /* Convert counter to bit string of length 32 bit using I2BSP. */
      uint32_to_octet_string(counter, counter_buffer);

      if (digest_size <= remaining_length) {
         error = sx_hash_array_blk(hash_fct, data, 3, derived_key);
         if (error != CRYPTOLIB_SUCCESS) {
            return error;
         }
         copy_length = digest_size;
      } else {
         error = sx_hash_array_blk(hash_fct, data, 3, digest);
         if (error != CRYPTOLIB_SUCCESS) {
            return error;
         }
         copy_length = remaining_length;
         memcpy_blk(derived_key, digest, copy_length);
      }

      if (!(derived_key.flags & BLOCK_S_CONST_ADDR)) {
         derived_key.addr += copy_length;
      }

      remaining_length -= copy_length;
      counter++;
   }

   return CRYPTOLIB_SUCCESS;
}
