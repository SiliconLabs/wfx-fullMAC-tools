#include "ports/includes.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

static bool os_random_init = false;
static mbedtls_entropy_context os_entropy_ctx;
static mbedtls_ctr_drbg_context os_drbg_ctx;

void* os_zalloc(
    size_t size)
{
    void* n;

    n = malloc(size);
    if (n)
    {
        memset(n, 0, size);
    }

    return n;
}

int os_get_random(
    unsigned char* buf,
    size_t len)
{
    int ret;

    if (!os_random_init) {
        mbedtls_entropy_init(&os_entropy_ctx);
        mbedtls_ctr_drbg_init(&os_drbg_ctx);
        ret = mbedtls_ctr_drbg_seed(&os_drbg_ctx, mbedtls_entropy_func, &os_entropy_ctx, NULL, 0);
        if(ret)
        {
            mbedtls_entropy_free(&os_entropy_ctx);
            return -1;
        }

        os_random_init = true;
    }

    ret = mbedtls_ctr_drbg_random(&os_drbg_ctx, buf, len);
    if(ret)
    {
        return -1;
    }

    return 0;
}

int os_memcmp_const(const void *a, const void *b, size_t len)
{
    return memcmp(a, b, len);
}
