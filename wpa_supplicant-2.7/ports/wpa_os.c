#include "ports/includes.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "wpa_os.h"

static bool os_random_init = false;
static mbedtls_entropy_context os_entropy_ctx;
static mbedtls_ctr_drbg_context os_drbg_ctx;

/*
 * Structure returned by gettimeofday(2) system call, and used in other calls.
 */
struct timeval {
  _TIME_T_    tv_sec;   /* seconds */
  __suseconds_t tv_usec;  /* and microseconds */
};

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

int os_get_time(struct os_time *t)
{
	int res;
	struct timeval tv;

	res = gettimeofday(&tv, NULL);
	t->sec = tv.tv_sec;
	t->usec = tv.tv_usec;

	return res;
}

void mbedtls_platform_zeroize( void *buf, size_t len )
{
    MBEDTLS_INTERNAL_VALIDATE( len == 0 || buf != NULL );
    volatile unsigned char *p = buf; 
    while( len-- ) *p++ = 0;
}
