
#ifndef MBEDTLS_CONFIG_SECURE_MQTT_H
#define MBEDTLS_CONFIG_SECURE_MQTT_H

/* Include the default mbed TLS config file */
#include "mbedtls/config.h"

/* Add Micrium OS support */
#define MBEDTLS_THREADING_ALT
#define MBEDTLS_THREADING_C
#define MBEDTLS_MICRIUM
#undef MBEDTLS_NET_C

/* Exclude ECC using more than 256bits */
#undef MBEDTLS_ECP_DP_SECP384R1_ENABLED
#undef MBEDTLS_ECP_DP_SECP521R1_ENABLED
#undef MBEDTLS_ECP_DP_BP384R1_ENABLED
#undef MBEDTLS_ECP_DP_BP512R1_ENABLED

#include "config-sl-crypto-all-acceleration.h"

#endif //MBEDTLS_CONFIG_SECURE_MQTT_H
