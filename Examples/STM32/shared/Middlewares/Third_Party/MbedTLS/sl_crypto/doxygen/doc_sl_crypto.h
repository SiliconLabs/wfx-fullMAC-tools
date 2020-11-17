/***************************************************************************//**
 * @file
 * @brief Overview of plugins for hardware accelerated cryptography
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: APACHE-2.0
 *
 * This software is subject to an open source license and is distributed by
 * Silicon Laboratories Inc. pursuant to the terms of the Apache License,
 * Version 2.0 available at https://www.apache.org/licenses/LICENSE-2.0.
 * Such terms and conditions may be further supplemented by the Silicon Labs
 * Master Software License Agreement (MSLA) available at www.silabs.com and its
 * sections applicable to open source software.
 *
 ******************************************************************************/

/**
@defgroup sl_crypto Silicon Labs Cryptography Hardware Acceleration Plugins
@brief Overview of plugins for hardware accelerated cryptography
@{

@section Introduction
The mbedtls/sl_crypto folder includes alternative implementations (plugins)
from Silicon Labs for selected mbed TLS library functions. The plugins
use the AES, RADIOAES, CRYPTO and SE hardware peripherals to accelerate low-level
cryptographic primitives. Available acceleration hardware depends on the target device.

The plugins support sharing of cryptography hardware in multi-threaded applications,
as well as a reduced overhead configuration for optimal performance in single-threaded applications.
Multi-threaded support is provided by the @ref sl_crypto_threading module.


@subsection sl_aes_plugin AES Peripheral

A plugin for the AES peripheral is provided for classic EFM32 and EZR32 MCUs (Series-0).

Define MBEDTLS_AES_C and MBEDTLS_AES_ALT to enable this plugin.

For more details, see accelerated implementation file aes_aes.c.


@subsection sl_crypto_plugin CRYPTO Peripheral

The Series-1 devices incorporate the CRYPTO peripheral for cryptographic hardware acceleration.
The plugins using the CRYPTO peripheral support multi-threaded applications by implementing mbed TLS
threading primitives and are located in mbedtls/sl_crypto/src/crypto_*.
These implementations are replacing the corresponding software implementations in mbedtls/include/mbedtls/.

@li <b>crypto_aes.c:</b> acceleration enabled by MBEDTLS_AES_C / MBEDTLS_AES_ALT.
@li <b>crypto_ecp.c:</b> ECC point multiplication acceleration (secp192r1, secp224r1 and secp256r1) enabled by MBEDTLS_ECP_C / MBEDTLS_ECP_ALT.
@li <b>crypto_sha.c:</b> SHA-1 and SHA-256 acceleration enabled by MBEDTLS_SHA1_C / MBEDTLS_SHA1_ALT or MBEDTLS_SHA256_C / MBEDTLS_SHA256_ALT.


@subsection sl_se_crypto_plugin Secure Element Peripheral

The Series-2 devices incorporate the SE peripheral for cryptographic hardware acceleration.
The plugins using the SE peripheral support multi-threaded applications by implementing mbed TLS
threading primitives and are located in mbedtls/sl_crypto/src/se_*.
These implementations are replacing the corresponding software implementations in mbedtls/include/mbedtls/.

@li <b>se_aes.c:</b> acceleration enabled by MBEDTLS_AES_C / MBEDTLS_AES_ALT.
@li <b>se_ccm.c:</b> acceleration enabled by MBEDTLS_AES_C / MBEDTLS_CCM_ALT.
@li <b>se_cmac.c:</b> acceleration enabled by MBEDTLS_AES_C / MBEDTLS_CMAC_C / MBEDTLS_CMAC_ALT.
@li <b>se_ecp.c:</b> acceleration enabled by MBEDTLS_ECP_C / MBEDTLS_ECDH_GEN_PUBLIC_ALT, MBEDTLS_ECDH_COMPUTE_SHARED_ALT, MBEDTLS_ECDSA_GENKEY_ALT, MBEDTLS_ECDSA_VERIFY_ALT or MBEDTLS_ECDSA_SIGN_ALT.
@li <b>se_jpake.c:</b> acceleration enabled by MBEDTLS_ECJPAKE_C / MBEDTLS_ECJPAKE_ALT.
@li <b>se_sha.c:</b> acceleration enabled by MBEDTLS_SHA1_C / MBEDTLS_SHA1_ALT, MBEDTLS_SHA256_C / MBEDTLS_SHA256_ALT or MBEDTLS_SHA512_C / MBEDTLS_SHA512_ALT.
@li <b>se_trng.c:</b> acceleration enabled by MBEDTLS_TRNG_C / MBEDTLS_ENTROPY_HARDWARE_ALT.


@subsection sl_se_cryptoacc_plugin CRYPTOACC Peripheral

The EFR32xG22 devices incorporate the CRYPTOACC peripheral for cryptographic hardware acceleration.
The plugins using the CRYPTOACC peripheral support multi-threaded applications by implementing mbed TLS
threading primitives and are located in mbedtls/sl_crypto/src/cryptoacc_*.
These implementations are replacing the corresponding software implementations in mbedtls/include/mbedtls/.

@li <b>cryptoacc_aes.c:</b> acceleration enabled by MBEDTLS_AES_C / MBEDTLS_AES_ALT.
@li <b>cryptoacc_ccm.c:</b> acceleration enabled by MBEDTLS_AES_C / MBEDTLS_CCM_ALT.
@li <b>cryptoacc_cmac.c:</b> acceleration enabled by MBEDTLS_AES_C / MBEDTLS_CMAC_C / MBEDTLS_CMAC_ALT.
@li <b>cryptoacc_ecp.c:</b> acceleration enabled by MBEDTLS_ECP_C / MBEDTLS_ECDH_GEN_PUBLIC_ALT, MBEDTLS_ECDH_COMPUTE_SHARED_ALT, MBEDTLS_ECDSA_GENKEY_ALT, MBEDTLS_ECDSA_VERIFY_ALT or MBEDTLS_ECDSA_SIGN_ALT.
@li <b>cryptoacc_gcm.c:</b> acceleration enabled by MBEDTLS_ECJPAKE_C / MBEDTLS_ECJPAKE_ALT.
@li <b>cryptoacc_sha.c:</b> acceleration enabled by MBEDTLS_SHA1_C / MBEDTLS_SHA1_ALT, MBEDTLS_SHA256_C / MBEDTLS_SHA256_ALT or MBEDTLS_SHA512_C / MBEDTLS_SHA512_ALT.
@li <b>cryptoacc_trng.c:</b> acceleration enabled by MBEDTLS_TRNG_C / MBEDTLS_ENTROPY_HARDWARE_ALT.

@} end group sl_crypto */
