/***************************************************************************//**
 * # License
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is Third Party Software licensed by Silicon Labs from a third party
 * and is governed by the sections of the MSLA applicable to Third Party
 * Software and the additional terms set forth below.
 *
 ******************************************************************************/
/*
 * Common configuration for Silicon Labs stacks in multiprotocol
 *
 */

#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

#include <stddef.h>
#include <stdint.h>
#include "em_device.h"

// bg_calloc and bg_free are implemented in bgcommon
void bg_free(void *pv);
void *bg_calloc(size_t num, size_t size);

#if !defined(NO_CRYPTO_ACCELERATION)
// Common acceleration
#define MBEDTLS_AES_ALT

#if defined(CRYPTO_COUNT) && (CRYPTO_COUNT > 0)
#define MBEDTLS_SHA1_ALT
#define MBEDTLS_SHA256_ALT

#define MBEDTLS_ECP_INTERNAL_ALT
#define ECP_SHORTWEIERSTRASS
#define MBEDTLS_ECP_ADD_MIXED_ALT
#define MBEDTLS_ECP_DOUBLE_JAC_ALT
#define MBEDTLS_ECP_NORMALIZE_JAC_MANY_ALT
#define MBEDTLS_ECP_NORMALIZE_JAC_ALT
#define MBEDTLS_ECP_RANDOMIZE_JAC_ALT
#define CRYPTO_DEVICE_PREEMPTION
#endif

#if defined(SEMAILBOX_PRESENT)
#include "em_se.h"

#if defined(SE_COMMAND_OPTION_HASH_SHA1)
#define MBEDTLS_SHA1_ALT
#define MBEDTLS_SHA1_PROCESS_ALT
#endif
#if defined(SE_COMMAND_OPTION_HASH_SHA256) || defined(SE_COMMAND_OPTION_HASH_SHA224)
#define MBEDTLS_SHA256_ALT
#define MBEDTLS_SHA256_PROCESS_ALT
#endif
#if defined(SE_COMMAND_OPTION_HASH_SHA512) || defined(SE_COMMAND_OPTION_HASH_SHA384)
#define MBEDTLS_SHA512_ALT
#define MBEDTLS_SHA512_PROCESS_ALT
#endif
#if defined(SE_COMMAND_CREATE_KEY)
#define MBEDTLS_ECDH_GEN_PUBLIC_ALT
#define MBEDTLS_ECDSA_GENKEY_ALT
#endif
#if defined(SE_COMMAND_DH)
#define MBEDTLS_ECDH_COMPUTE_SHARED_ALT
#endif
#if defined(SE_COMMAND_JPAKE_GEN_SESSIONKEY)
#define MBEDTLS_ECJPAKE_ALT
#endif
#if defined(SE_COMMAND_SIGNATURE_SIGN)
#define MBEDTLS_ECDSA_SIGN_ALT
#endif
#if defined(SE_COMMAND_SIGNATURE_VERIFY)
#define MBEDTLS_ECDSA_VERIFY_ALT
#endif

#if defined(SE_COMMAND_AES_CCM_ENCRYPT) && defined(SE_COMMAND_AES_CCM_DECRYPT)
#define MBEDTLS_CCM_ALT
#endif
#if defined(SE_COMMAND_AES_CMAC)
#define MBEDTLS_CMAC_ALT
#endif
#endif

// Threading for multiprotocol
#define MBEDTLS_THREADING_ALT
#define MBEDTLS_THREADING_C
#endif

/* mbed TLS modules */
#define MBEDTLS_CIPHER_C
#define MBEDTLS_ECP_C
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#define MBEDTLS_ECDH_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_CMAC_C
#define MBEDTLS_AES_C
#define MBEDTLS_CCM_C
#define MBEDTLS_CIPHER_MODE_CTR
#undef MBEDTLS_FS_IO
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_PLATFORM_CALLOC_MACRO  bg_calloc /**< Default allocator macro to use, can be undefined */
#define MBEDTLS_PLATFORM_FREE_MACRO    bg_free /**< Default free macro to use, can be undefined */

/* Save RAM at the expense of ROM */
#define MBEDTLS_AES_ROM_TABLES

/* Save RAM by adjusting to our exact needs */
#define MBEDTLS_ECP_MAX_BITS   256
#define MBEDTLS_MPI_MAX_SIZE    32 // 384 bits is 48 bytes

/*
   Set MBEDTLS_ECP_WINDOW_SIZE to configure
   ECC point multiplication window size, see ecp.h:
   2 = Save RAM at the expense of speed
   3 = Improve speed at the expense of RAM
   4 = Optimize speed at the expense of RAM
 */
#define MBEDTLS_ECP_WINDOW_SIZE        2
#define MBEDTLS_ECP_FIXED_POINT_OPTIM  0

/* Significant speed benefit at the expense of some ROM */
/* Do not define if only enabling curves that have HW support */
// #define MBEDTLS_ECP_NIST_OPTIM

/* Do not compile in SW ECP code if only enabling curves that have hardware support */
#define MBEDTLS_ECP_NO_FALLBACK

/*
 * You should adjust this to the exact number of sources you're using: default
 * is the "mbedtls_platform_entropy_poll" source, but you may want to add other ones.
 * Minimum is 2 for the entropy test suite.
 */
#define MBEDTLS_ENTROPY_MAX_SOURCES 2

#include "mbedtls/check_config.h"

#endif /* MBEDTLS_CONFIG_H */
