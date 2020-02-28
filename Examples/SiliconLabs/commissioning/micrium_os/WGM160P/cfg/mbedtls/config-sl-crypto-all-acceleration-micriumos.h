
#ifndef MBEDTLS_CONFIG_SL_CRYPTO_ALL_ACCELERATION_MICRIUMOS_H
#define MBEDTLS_CONFIG_SL_CRYPTO_ALL_ACCELERATION_MICRIUMOS_H

/* Include the default mbed TLS config file */
#include "mbedtls/config.h"

/* Add Micrium OS support */
#define MBEDTLS_THREADING_ALT
#define MBEDTLS_THREADING_C
#define MBEDTLS_MICRIUM
#undef MBEDTLS_NET_C

#include "config-sl-crypto-all-acceleration.h"

#endif //MBEDTLS_CONFIG_SL_CRYPTO_ALL_ACCELERATION_MICRIUMOS_H
