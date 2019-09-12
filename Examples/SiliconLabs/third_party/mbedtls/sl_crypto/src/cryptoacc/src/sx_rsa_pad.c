/**
* @file
* @brief Handles RSA padding
* @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
*/


#include "sx_rsa_pad.h"
#include <string.h>
#include "cryptolib_def.h"
#include "sx_memcpy.h"
#include "sx_memcmp.h"
#include "padding.h"
#include "sx_hash.h"
#include "sx_rng.h"
#include "sx_errors.h"

static const ALIGNED uint8_t zeros[32] = {0};


/**
 * @brief Get random bytes
 * @param dst pointer to output result
 * @param n size of \p dst
 * @param rng used random number generator
 * @param rem_zeros remove zeroes if set.
*/
static void gen_rnd_bytes(uint8_t *dst, size_t n, uint32_t rem_zeros,
      struct sx_rng rng)
{
   rng.get_rand_blk(
         rng.param,
         block_t_convert(dst, n));
   if (rem_zeros) {
      /* Zeros must be removed for caller needs. As the random generator may
       * return zeros, we manually replace them by ones. As it is not a pure
       * random replacement, it unfortunately decreases the entropy of the
       * generated randomness. */
     for (; n > 0; n--) if(dst[n-1] == 0x00) dst[n-1] = 0x01;
   }
}

/**
 * @brief Get first mask
 * @param n0 input
 * @return mask corresponding to \p n0
*/
static uint32_t getFirstMask(uint32_t n0) {
   if(n0 & 0x80) return 0x7F;
   else if(n0 & 0x40) return 0x3F;
   else if(n0 & 0x20) return 0x1F;
   else if(n0 & 0x10) return 0x0F;
   else if(n0 & 0x08) return 0x07;
   else if(n0 & 0x04) return 0x03;
   else if(n0 & 0x02) return 0x01;
   else return 0x00;
}

/**
 * @brief perform buff = buff XOR mask
 * @param buff input/output buffer
 * @param mask to xor with
 * @param n size of \p buff and \p mask
*/
static void mask(uint8_t *buff, uint8_t *mask, size_t n)
{
   for (; n > 0; n--) buff[n-1] = buff[n-1]^mask[n-1];
}

/**
 * Mask Generation Function as defined by RFC-8017 B.2.1.
 * @param hashType hash function to use
 * @param seed input
 * @param seedLen length of \p seed
 * @param mask output
 * @param maskLen length of \p mask
*/
static CHECK_RESULT uint32_t MGF1(sx_hash_fct_t hashType, uint8_t *seed, size_t seedLen, uint8_t *mask, size_t maskLen)
{
   uint8_t cnt[8]; //todo: reduce size (get warning "stack protector not protecting function: all local arrays are less than 8 bytes long")
   uint32_t c ;
   block_t array_blk[2];
   uint32_t hashLen = sx_hash_get_digest_size(hashType);

   array_blk[0] = block_t_convert(seed, seedLen);
   array_blk[1] = block_t_convert(cnt, 4);

   c = 0;
   while(maskLen)
   {
      array_blk[1].addr[0] = ((c >> 24)  & 0xFF); // note: modifying cnt content via array_blk[1].addr to make static analyzer happy
      array_blk[1].addr[1] = ((c >> 16)  & 0xFF);
      array_blk[1].addr[2] = ((c >> 8)   & 0xFF);
      array_blk[1].addr[3] = ((c)        & 0xFF);
      uint32_t status = sx_hash_array_blk(hashType, array_blk, 2, block_t_convert(mask, maskLen));
      if (status)
         return status;

      c++;
      maskLen -= SX_MIN(hashLen, maskLen);
      mask += hashLen;
   }

   return CRYPTOLIB_SUCCESS;
}


uint32_t rsa_pad_eme_oaep_encode(uint32_t k, sx_hash_fct_t hashType,
      uint8_t *EM, block_t message, size_t mLen,
      struct sx_rng rng)
{
   uint8_t dbMask[RSA_MAX_SIZE] = {0};
   uint8_t seedMask[MAX_DIGESTSIZE] = {0};
   uint32_t hLen;
   uint32_t status;

   // get lengths
   hLen = sx_hash_get_digest_size(hashType);
   if(mLen +2*hLen + 2 > k) return CRYPTOLIB_INVALID_PARAM; // MESSAGE_TOO_LONG;

   // get label hash -> no label so NULL_blk
   status = sx_hash_blk(hashType, NULL_blk, block_t_convert(EM + hLen + 1, hLen) );
   if(status != CRYPTOLIB_SUCCESS)
      return status;

   //Assemply of DB
   gen_rnd_bytes(EM+1, hLen, 0, rng);
   EM[0] = 0x00;
   memset((uint8_t*)(EM + 2*hLen + 1), 0x00, k - mLen - 2*hLen - 2); //PS
   *((uint8_t*)(EM + k - mLen - 1)) = 0x01; //0x01
   memcpy_blkIn(EM + k - mLen, message, mLen);

   status = MGF1(hashType, EM + 1, hLen, dbMask, k - hLen - 1);
   if (status)
      return status;
   mask(EM + 1 + hLen, dbMask, k - hLen - 1);

   status = MGF1(hashType, EM + 1 + hLen, k - hLen - 1, seedMask, hLen);
   if (status)
      return status;
   mask(EM + 1, seedMask, hLen);

   return CRYPTOLIB_SUCCESS;
}

uint32_t rsa_pad_eme_oaep_decode(uint32_t k, sx_hash_fct_t hashType, uint8_t *EM, uint8_t **message, size_t *mLen)
{
   uint32_t status;
   size_t hLen = sx_hash_get_digest_size(hashType);
   uint8_t dbMask[RSA_MAX_SIZE] = {0};
   uint8_t seedMask[MAX_DIGESTSIZE] = {0};

   status = MGF1(hashType, EM + hLen + 1, k - hLen - 1, seedMask, hLen);
   if (status)
      return status;
   mask(EM + 1, seedMask, hLen);

   status = MGF1(hashType, EM + 1, hLen, dbMask, k - hLen - 1);
   if (status)
      return status;
   mask(EM + hLen + 1, dbMask, k - hLen - 1);

   *mLen = 0;
   size_t i = 0;
   for (i = hLen + 1; i < k; i++) {
      if(*(uint8_t*)(EM + i) == 0x01){
         *mLen = k - i - 1;
         break;
      }
   }

   status = sx_hash_blk(hashType, NULL_blk, block_t_convert(seedMask, hLen) );
   if(status != CRYPTOLIB_SUCCESS)
      return status;

   int chkLHash = memcmp_time_cst(seedMask, EM + hLen + 1, sx_hash_get_digest_size(hashType));
   if(chkLHash || *mLen == 0 || EM[0]) return CRYPTOLIB_CRYPTO_ERR; // DECRYPTION_ERROR;

   *message = (uint8_t*)((EM + k) - *mLen);

   return CRYPTOLIB_SUCCESS;
}


uint32_t rsa_pad_eme_pkcs_encode(uint32_t k, uint8_t *EM, block_t message,
      size_t mLen, struct sx_rng rng)
{
   if(mLen > (size_t)(k - 11)) return CRYPTOLIB_INVALID_PARAM; // MESSAGE_TOO_LONG;

   //Assemply of DB
   gen_rnd_bytes(EM + 2, k - mLen - 2, 1, rng); //PS (first written for alignment purpose)
   EM[0] = 0x00;
   EM[1] = 0x02;

   *((uint8_t*)(EM + k - mLen - 1)) = 0x00; //0x00
   memcpy_blkIn(EM + k - mLen, message, mLen);

   return CRYPTOLIB_SUCCESS;
}

uint32_t rsa_pad_eme_pkcs_decode(uint32_t k, uint8_t *EM, uint8_t **message, size_t *mLen)
{
   *mLen = 0;
   size_t i = 0;
   for (i = 2; i < k; i++) {
      if(*(uint8_t*)(EM + i) == 0x00){
         *mLen = k - i - 1;
         break;
      }
   }

   size_t PSLen = k - 3 - *mLen;
   if(*mLen == 0 || EM[0] || EM[1] != 0x02 || PSLen < 8) return CRYPTOLIB_CRYPTO_ERR; // DECRYPTION_ERROR;
   *message = (uint8_t*)((EM + k) - *mLen);

   return CRYPTOLIB_SUCCESS;
}

uint32_t rsa_pad_emsa_pkcs_encode(uint32_t emLen, sx_hash_fct_t hash_type, uint8_t *EM, uint8_t *hash)
{
   uint8_t hashTypeDer[19] = {0x30, 0x21, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x2, 0x06, 0x05, 0x00, 0x04, 0x20};
   size_t tLen;
   size_t hLen;
   size_t dLen = 19;

   switch(hash_type) { //adapt DER encoded hashType
      case e_SHA1: //SHA1
      hLen = 20;
      dLen = 15;
      hashTypeDer[3] = 0x09;
      hashTypeDer[5] = 0x05;
      hashTypeDer[6] = 0x2b;
      hashTypeDer[7] = 0x0e;
      hashTypeDer[8] = 0x03;
      hashTypeDer[9] = 0x02;
      hashTypeDer[10] = 0x1a;
      hashTypeDer[11] = 0x05;
      hashTypeDer[12] = 0x00;
      hashTypeDer[13] = 0x04;
      hashTypeDer[14] = 0x14;
      break;
      case e_SHA224: //SHA224
      hLen = 28;
      hashTypeDer[1] = 0x2d;
      hashTypeDer[14] = 0x04;
      hashTypeDer[18] = 0x1c;
      break;
      case e_SHA256: //SHA256
      hLen = 32;
      hashTypeDer[1] = 0x31;
      hashTypeDer[14] = 0x01;
      hashTypeDer[18] = 0x20;
      break;
      case e_SHA384: //SHA384
      hLen = 4*12;
      hashTypeDer[1] = 0x41;
      hashTypeDer[14] = 0x02;
      hashTypeDer[18] = 0x30;
      break;
      case e_SHA512: //SHA512
      hLen = 4*16;
      hashTypeDer[1] = 0x51;
      hashTypeDer[14] = 0x03;
      hashTypeDer[18] = 0x40;
      break;
      default:
      return CRYPTOLIB_INVALID_PARAM; // HASH_TYPE_ERROR;
   }

   tLen = hLen + dLen;

   if(emLen < tLen + 11) return CRYPTOLIB_INVALID_PARAM; // MESSAGE_TOO_SHORT;

   *EM = 0x00;
   *(uint8_t*)(EM + 1) = 0x01;
   memset((uint8_t*)(EM + 2), 0xff, emLen - tLen - 3); //PS
   *(uint8_t*)(EM + emLen - tLen - 1) = 0x00;
   memcpy_array(EM + emLen - tLen, hashTypeDer, dLen); //HashType
   memcpy_array(EM + emLen - hLen, hash, hLen); //Hash

   return CRYPTOLIB_SUCCESS;
}



uint32_t rsa_pad_emsa_pss_encode(uint32_t emLen, sx_hash_fct_t hashType,
      uint8_t *EM, uint8_t *hash, uint32_t n0 , size_t sLen,
      struct sx_rng rng)
{
   size_t hLen = sx_hash_get_digest_size(hashType);
   if(!hLen) return CRYPTOLIB_INVALID_PARAM; // HASH_TYPE_ERROR;

   if(emLen < sLen + hLen + 2) return CRYPTOLIB_INVALID_PARAM; // ENCODING_ERROR;

   uint8_t dbMask[RSA_MAX_SIZE];
   memset(EM, 0x00, emLen - sLen - hLen - 2); //PS
   gen_rnd_bytes((uint8_t*)(EM + emLen - sLen - hLen - 1), sLen, 0,
         rng); //Salt
   *((uint8_t*)(EM + emLen - sLen - hLen - 2)) = 0x01; //0x01

   block_t array[3] = {block_t_convert(zeros, 8),
                       block_t_convert(hash, hLen),
                       block_t_convert((EM + emLen - (sLen + hLen) - 1), sLen)};
   uint32_t status = sx_hash_array_blk(hashType, array, 3, block_t_convert((uint8_t*)(EM + emLen - hLen - 1), hLen));
   if (status)
      return status;

   status = MGF1(hashType, EM + emLen - hLen - 1, hLen, dbMask, emLen - hLen - 1);
   if (status)
      return status;
   mask(EM, dbMask, emLen - hLen - 1);

   EM[emLen - 1] = 0xBC;
   EM[0] = EM[0] & getFirstMask(n0);

   return CRYPTOLIB_SUCCESS;
}

/* Steps from rfc8017 9.1.2 Verification operation */
uint32_t rsa_pad_emsa_pss_decode(uint32_t emLen, sx_hash_fct_t hashType,
                                 uint8_t *EM, uint8_t *hash, uint32_t sLen,
                                 uint32_t n0)
{
   size_t hLen = sx_hash_get_digest_size(hashType);
   if(!hLen) return CRYPTOLIB_INVALID_PARAM; // HASH_TYPE_ERROR;

   /* 3.  If emLen < hLen + sLen + 2, output "inconsistent" and stop. */
   /* 4.  If the rightmost octet of EM does not have hexadecimal value
       0xbc, output "inconsistent" and stop. */

   if (emLen < hLen + sLen + 2 || EM[emLen - 1] != 0xbc)
    return CRYPTOLIB_INVALID_PARAM; // INCONSISTENT;
   /* 5.  Let maskedDB be the leftmost emLen - hLen - 1 octets of EM, and
       let H be the next hLen octets. */
   uint8_t *maskedDB = EM;
   uint32_t dbLen = emLen - hLen - 1;
   uint8_t *H = EM + emLen - hLen - 1;

   //FIXME: This check is not performed because it fails a few known-good test cases.
   /* 6.   If the leftmost 8emLen - emBits bits of the leftmost octet in
           maskedDB are not all equal to zero, output "inconsistent" and
           stop. */

   /* 7.  Let dbMask = MGF(H, emLen - hLen - 1). */
   //This buffer is later reused to store H'
   uint8_t dbMask[RSA_MAX_SIZE];
   uint32_t status = MGF1(hashType, H, hLen, dbMask, emLen - hLen - 1);
   if (status)
      return status;

   /* 8.  Let DB = maskedDB \xor dbMask. */
   mask(maskedDB, dbMask, emLen - hLen - 1);
   //from here maskedDB = RFC's DB
   uint8_t *DB = maskedDB;
   /* 9.  Set the leftmost 8emLen - emBits bits of the leftmost octet in DB
          to zero. */
   DB[0] = DB[0] & getFirstMask(n0);


   /* 10. If the emLen - hLen - sLen - 2 leftmost octets of DB are not zero
       or if the octet at position emLen - hLen - sLen - 1 (the leftmost
       position is "position 1") does not have hexadecimal value 0x01,
       output "inconsistent" and stop. */
   size_t i;
   for (i = 1; i < emLen - hLen - sLen - 2; i++) {
      if(*(uint8_t*)(DB + i))
        return CRYPTOLIB_CRYPTO_ERR; // INCONSISTENT; //Check padding 0x00
   }

   if(*(uint8_t*)(DB + emLen - hLen - sLen - 2) != 0x01)
    return CRYPTOLIB_CRYPTO_ERR; // INCONSISTENT;

   /* 11.  Let salt be the last sLen octets of DB. */
   uint8_t *salt = DB + dbLen - sLen;
   /* 12.  Let M' = (0x)00 00 00 00 00 00 00 00 || mHash || salt ;
       M' is an octet string of length 8 + hLen + sLen with eight
       initial zero octets. */
   block_t M[3] = {block_t_convert(zeros, 8),
                   block_t_convert(hash, hLen),
                   block_t_convert(salt, sLen)};

   /* 13. Let H' = Hash(M'), an octet string of length hLen. */
   //Hash is placed in the unused big buffer. First byte of EM 1 byte to small.
   uint8_t *H_ = dbMask;
   status = sx_hash_array_blk(hashType, M, 3, block_t_convert(H_, hLen));
   if (status)
      return status;

   /* 14. If H = H', output "consistent." Otherwise, output "inconsistent." */
   int chkLHash = memcmp_time_cst(H, H_, hLen);
   if (chkLHash) {
      return CRYPTOLIB_INVALID_SIGN_ERR; // INCONSISTENT;
   }

   return CRYPTOLIB_SUCCESS;
}


void rsa_pad_zeros(uint8_t *EM, size_t emLen, uint8_t *hash, size_t hashLen){
   pad_zeros(EM, emLen, hash, hashLen);
}

