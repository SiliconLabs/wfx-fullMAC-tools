/**
 * @file
 * @brief Generate DH MODP key
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */


#include "sx_dh_keygen_alg.h"
#include "cryptolib_def.h"
#include "sx_memcpy.h"
#include "sx_rng.h"
#include "sx_errors.h"


#if (DH_MODP_ENABLED)

uint32_t dh_genkey(block_t mod, block_t priv, struct sx_rng rng)
{
   if (mod.len > DH_MAX_KEY_SIZE)
      return CRYPTOLIB_UNSUPPORTED_ERR;
   return sx_rng_get_rand_lt_n_blk(priv, mod, rng);
}
#endif
