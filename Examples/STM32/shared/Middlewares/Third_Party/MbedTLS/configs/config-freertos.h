
#ifndef MBEDTLS_CONFIG_FREERTOS_H
#define MBEDTLS_CONFIG_FREERTOS_H

/* Include the default mbed TLS config file */
#include "mbedtls/config.h"

/* Add FreeRTOS support */
#define MBEDTLS_THREADING_ALT
#define MBEDTLS_THREADING_C
#define MBEDTLS_FREERTOS
#undef MBEDTLS_NET_C
#undef MBEDTLS_TIMING_C
#undef MBEDTLS_FS_IO

#define MBEDTLS_NO_PLATFORM_ENTROPY

#if defined(HAL_RNG_MODULE_ENABLED)
#define MBEDTLS_TRNG_C
#define MBEDTLS_ENTROPY_HARDWARE_ALT
#endif

#endif //MBEDTLS_CONFIG_FREERTOS_H
