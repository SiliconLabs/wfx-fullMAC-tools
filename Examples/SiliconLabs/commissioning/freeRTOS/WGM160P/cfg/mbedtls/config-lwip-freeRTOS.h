
#ifndef MBEDTLS_CONFIG_LWIP_FREERTOS_H
#define MBEDTLS_CONFIG_LWIP_FREERTOS_H

/* Include the default mbed TLS config file */
#include "mbedtls/config.h"


#define MBEDTLS_THREADING_ALT
#define MBEDTLS_THREADING_C
#define MBEDTLS_FREERTOS
#undef MBEDTLS_NET_C

#include "config-sl-crypto-all-acceleration.h"

#endif //MBEDTLS_CONFIG_LWIP_FREERTOS_H
