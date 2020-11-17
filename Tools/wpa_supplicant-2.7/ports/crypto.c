#include "ports/includes.h"
#include "utils/common.h"
#include "crypto/crypto.h"
#include "mbedtls/bignum.h"
#include "mbedtls/ecp.h"
#include "mbedtls/md.h"

#define SL_IANA_IKE_GROUP_NIST_P256 19

/**
 * HMAC
 */

int hmac_sha256_vector(
    const u8* key,
    size_t key_len,
    size_t num_elem,
    const u8* addr[],
    const size_t* len,
    u8* mac)
{
    mbedtls_md_context_t md_ctx;
    int index = 0;
    int ret;

    mbedtls_md_init(&md_ctx);
    ret = mbedtls_md_setup(&md_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    if (!ret) {
        ret = mbedtls_md_hmac_starts(&md_ctx, key, key_len);
        while(!ret && (index < num_elem)) {
            ret = mbedtls_md_hmac_update(&md_ctx, addr[index], len[index]);
            index++;
        }
        if (!ret) {
            ret = mbedtls_md_hmac_finish(&md_ctx, mac);
        }
    }
    mbedtls_md_free(&md_ctx);

    return ret;
}

int hmac_sha256(
    const u8* key,
    size_t key_len,
    const u8* data,
    size_t data_len,
    u8* mac)
{
    return mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), key, key_len, data, data_len, mac);
}

/**
 * FCC DH groups
 */

const struct dh_group* dh_groups_get(
    int id)
{
    /* FCC groups not supported */

    return NULL;
}

/**
 * crypto_bignum
 */

struct crypto_bignum* crypto_bignum_init(
    void)
{
    mbedtls_mpi* mpi;

    mpi = (mbedtls_mpi*)malloc(sizeof(mbedtls_mpi));
    if (mpi) {
        mbedtls_mpi_init(mpi);
    }

    return (struct crypto_bignum*)mpi;
}

struct crypto_bignum* crypto_bignum_init_set(
    const u8* buf,
    size_t len)
{
    mbedtls_mpi* mpi;
    int ret;

    mpi = (mbedtls_mpi*)crypto_bignum_init();
    if (mpi) {
        ret = mbedtls_mpi_read_binary(mpi, buf, len);
        if (ret) {
            crypto_bignum_deinit((struct crypto_bignum*)mpi, false);
            mpi = NULL;
        }
    }

    return (struct crypto_bignum*)mpi;
}

void crypto_bignum_deinit(
    struct crypto_bignum* n,
    int clear)
{
    mbedtls_mpi* mpi = (mbedtls_mpi*)n;

    if (mpi) {
        mbedtls_mpi_free(mpi);
        free(mpi);
    }
}

int crypto_bignum_bits(
    const struct crypto_bignum* a)
{
    mbedtls_mpi* mpi = (mbedtls_mpi*)a;
    int ret = -1;

    if (mpi) {
        ret = mbedtls_mpi_bitlen(mpi);
    }

    return ret;
}

int crypto_bignum_is_zero(
    const struct crypto_bignum* a)
{
    mbedtls_mpi* mpi = (mbedtls_mpi*)a;
    int ret = -1;

    if (mpi) {
        ret = !mbedtls_mpi_cmp_int(mpi, 0);
    }

    return ret;
}

int crypto_bignum_is_one(
    const struct crypto_bignum* a)
{
    mbedtls_mpi* mpi = (mbedtls_mpi*)a;
    int ret = -1;

    if (mpi) {
        ret = !mbedtls_mpi_cmp_int(mpi, 1);
    }

    return ret;
}

int crypto_bignum_cmp(
    const struct crypto_bignum*a,
    const struct crypto_bignum* b)
{
    mbedtls_mpi* mpi_a = (mbedtls_mpi*)a;
    mbedtls_mpi* mpi_b = (mbedtls_mpi*)b;
    int ret = -2;

    if (mpi_a && mpi_b) {
        ret = mbedtls_mpi_cmp_mpi(mpi_a, mpi_b);
    }

    return ret;
}

int crypto_bignum_mulmod(
    const struct crypto_bignum* a,
    const struct crypto_bignum* b,
    const struct crypto_bignum* c,
    struct crypto_bignum* d)
{
    mbedtls_mpi* mpi_a = (mbedtls_mpi*)a;
    mbedtls_mpi* mpi_b = (mbedtls_mpi*)b;
    mbedtls_mpi* mpi_c = (mbedtls_mpi*)c;
    mbedtls_mpi* mpi_d = (mbedtls_mpi*)d;
    int ret = -1;

    if (mpi_a && mpi_b && mpi_c && mpi_d)
    {
        ret = mbedtls_mpi_mul_mpi(mpi_d, mpi_a, mpi_b);
        if (!ret) {
            ret = mbedtls_mpi_mod_mpi(mpi_d, mpi_d, mpi_c);
        }
    }

    return ret;
}

int crypto_bignum_legendre(
    const struct crypto_bignum* a,
    const struct crypto_bignum* p)
{
    mbedtls_mpi mpi_exp;
    mbedtls_mpi mpi_result;
    mbedtls_mpi* mpi_a = (mbedtls_mpi*)a;
    mbedtls_mpi* mpi_p = (mbedtls_mpi*)p;
    int ret;
    int result = -2;


    mbedtls_mpi_init(&mpi_exp);
    mbedtls_mpi_init(&mpi_result);
    ret = mbedtls_mpi_sub_int(&mpi_exp, mpi_p, 1);
    if (!ret) {
        ret = mbedtls_mpi_shift_r(&mpi_exp, 1);
    }
    if (!ret) {
        ret = mbedtls_mpi_exp_mod(&mpi_result, mpi_a, &mpi_exp, mpi_p, NULL);
    }
    if (!ret) {
        if (!mbedtls_mpi_cmp_int(&mpi_result, 1)) {
            result = 1;
        } else if (!mbedtls_mpi_cmp_int(&mpi_result, 0)) {
            result = 0;
        } else {
            result = -1;
        }
    }

    mbedtls_mpi_free(&mpi_result);
    mbedtls_mpi_free(&mpi_exp);

    return result;
}

int crypto_bignum_sub(
    const struct crypto_bignum* a,
    const struct crypto_bignum* b,
    struct crypto_bignum* c)
{
    mbedtls_mpi* mpi_a = (mbedtls_mpi*)a;
    mbedtls_mpi* mpi_b = (mbedtls_mpi*)b;
    mbedtls_mpi* mpi_c = (mbedtls_mpi*)c;
    int ret = -1;

    if (mpi_a && mpi_b && mpi_c) {
        ret = mbedtls_mpi_sub_mpi(mpi_c, mpi_a, mpi_b);
    }

    return ret;
}

int crypto_bignum_div(
    const struct crypto_bignum* a,
    const struct crypto_bignum* b,
    struct crypto_bignum* c)
{
    mbedtls_mpi* mpi_a = (mbedtls_mpi*)a;
    mbedtls_mpi* mpi_b = (mbedtls_mpi*)b;
    mbedtls_mpi* mpi_c = (mbedtls_mpi*)c;
    int ret = -1;

    if (mpi_a && mpi_b && mpi_c) {
        ret = mbedtls_mpi_div_mpi(mpi_c, NULL, mpi_a, mpi_b);
    }

    return ret;
}

int crypto_bignum_exptmod(
    const struct crypto_bignum* a,
    const struct crypto_bignum* b,
    const struct crypto_bignum* c,
    struct crypto_bignum* d)
{
    mbedtls_mpi* mpi_a = (mbedtls_mpi*)a;
    mbedtls_mpi* mpi_b = (mbedtls_mpi*)b;
    mbedtls_mpi* mpi_c = (mbedtls_mpi*)c;
    mbedtls_mpi* mpi_d = (mbedtls_mpi*)d;
    int ret = -1;

    if (mpi_a && mpi_b && mpi_c && mpi_d) {
        ret = mbedtls_mpi_exp_mod(mpi_d, mpi_a, mpi_b, mpi_c, NULL);
    }

    return ret;
}

int crypto_bignum_to_bin(
    const struct crypto_bignum* a,
    u8* buf,
    size_t buflen,
    size_t padlen)
{
    mbedtls_mpi* mpi_a = (mbedtls_mpi*)a;
    int ret;
    int result = -1;

    if (mpi_a) {
        if (padlen) {
            buflen = padlen;
        }
        ret = mbedtls_mpi_write_binary(mpi_a, buf, buflen);
        if(!ret)
        {
            result = buflen;
        }
    }

    return result;
}

int crypto_bignum_inverse(
    const struct crypto_bignum* a,
    const struct crypto_bignum* b,
    struct crypto_bignum* c)
{
    mbedtls_mpi* mpi_a = (mbedtls_mpi*)a;
    mbedtls_mpi* mpi_b = (mbedtls_mpi*)b;
    mbedtls_mpi* mpi_c = (mbedtls_mpi*)c;
    int ret = -1;

    if (mpi_a && mpi_b && mpi_c) {
        ret = mbedtls_mpi_inv_mod(mpi_c, mpi_a, mpi_b);
    }

    return ret;
}

int crypto_bignum_add(
    const struct crypto_bignum* a,
    const struct crypto_bignum* b,
    struct crypto_bignum* c)
{
    mbedtls_mpi* mpi_a = (mbedtls_mpi*)a;
    mbedtls_mpi* mpi_b = (mbedtls_mpi*)b;
    mbedtls_mpi* mpi_c = (mbedtls_mpi*)c;
    int ret = -1;

    if (mpi_a && mpi_b && mpi_c) {
        ret = mbedtls_mpi_add_mpi(mpi_c, mpi_a, mpi_b);
    }

    return ret;
}

int crypto_bignum_mod(
    const struct crypto_bignum* a,
    const struct crypto_bignum* b,
    struct crypto_bignum* c)
{
    mbedtls_mpi* mpi_a = (mbedtls_mpi*)a;
    mbedtls_mpi* mpi_b = (mbedtls_mpi*)b;
    mbedtls_mpi* mpi_c = (mbedtls_mpi*)c;
    int ret = -1;

    if (mpi_a && mpi_b && mpi_c) {
        ret = mbedtls_mpi_mod_mpi(mpi_c, mpi_a, mpi_b);
    }

    return ret;
}

/**
 * crypto_ec
 */

struct crypto_ec* crypto_ec_init(
    int group)
{
    mbedtls_ecp_group* ec;
    int ret;

    /* Only NIST p256 aka secp256r1 supported */
    if (group != SL_IANA_IKE_GROUP_NIST_P256) {
        return NULL;
    }

    ec = (mbedtls_ecp_group*)malloc(sizeof(mbedtls_ecp_group));
    if (ec) {
        mbedtls_ecp_group_init(ec);
        ret = mbedtls_ecp_group_load(ec, MBEDTLS_ECP_DP_SECP256R1);
        if (ret)
        {
            crypto_ec_deinit((struct crypto_ec*)ec);
            ec = NULL;
        }
    }

    return (struct crypto_ec*)ec;
}

void crypto_ec_deinit(
    struct crypto_ec* e)
{
    mbedtls_ecp_group* ec = (mbedtls_ecp_group*)e;

    if (ec) {
        mbedtls_ecp_group_free(ec);
        free(ec);
    }
}

size_t crypto_ec_prime_len_bits(
    struct crypto_ec* e)
{
    mbedtls_ecp_group* ec = (mbedtls_ecp_group*)e;
    int ret = -1;

    if (ec) {
        ret = ec->pbits;
    }

    return ret;
}

size_t crypto_ec_prime_len(
    struct crypto_ec* e)
{
    mbedtls_ecp_group* ec = (mbedtls_ecp_group*)e;
    int ret = -1;

    if (ec) {
        ret = (ec->pbits + 7) / 8;
    }

    return ret;
}

const struct crypto_bignum* crypto_ec_get_prime(
    struct crypto_ec* e)
{
    mbedtls_ecp_group* ec = (mbedtls_ecp_group*)e;

    if (ec) {
        return (const struct crypto_bignum*)&ec->P;
    }

    return NULL;
}

const struct crypto_bignum* crypto_ec_get_order(
    struct crypto_ec* e)
{
    mbedtls_ecp_group* ec = (mbedtls_ecp_group*)e;

    if (ec) {
        return (const struct crypto_bignum*)&ec->N;
    }

    return NULL;
}

/**
 * crypto_ec_point
 */

struct crypto_ec_point* crypto_ec_point_init(
    struct crypto_ec* e)
{
    mbedtls_ecp_point* ecp;

    ecp = (mbedtls_ecp_point*)malloc(sizeof(mbedtls_ecp_point));
    if (ecp) {
        mbedtls_ecp_point_init(ecp);
    }

    return (struct crypto_ec_point*)ecp;
}

void crypto_ec_point_deinit(
    struct crypto_ec_point* p,
    int clear)
{
    mbedtls_ecp_point* ecp = (mbedtls_ecp_point*)p;

    if (ecp) {
        mbedtls_ecp_point_free(ecp);
        free(ecp);
    }
}

int crypto_ec_point_mul(
    struct crypto_ec* e,
    const struct crypto_ec_point* p,
    const struct crypto_bignum* b,
    struct crypto_ec_point* res)
{
    mbedtls_ecp_group* ec = (mbedtls_ecp_group*)e;
    mbedtls_ecp_point* ecp_p = (mbedtls_ecp_point*)p;
    mbedtls_mpi* mpi_b = (mbedtls_mpi*)b;
    mbedtls_ecp_point* ecp_res = (mbedtls_ecp_point*)res;
    int ret = -1;

    if (ec && ecp_p && mpi_b && ecp_res) {
        ret = mbedtls_ecp_mul(ec, ecp_res, mpi_b, ecp_p, NULL, NULL);
    }

    return ret;
}

int crypto_ec_point_invert(
    struct crypto_ec* e,
    struct crypto_ec_point* p)
{
    mbedtls_ecp_group* ec = (mbedtls_ecp_group*)e;
    mbedtls_ecp_point* ecp = (mbedtls_ecp_point*)p;
    int ret = -1;

    if (ec && ecp) {
        ret = mbedtls_mpi_sub_mpi(&ecp->Y, &ec->P, &ecp->Y);
    }

    return ret;
}

int crypto_ec_point_solve_y_coord(
    struct crypto_ec* e,
    struct crypto_ec_point* p,
    const struct crypto_bignum* x,
    int y_bit)
{
    /**
     * This code is based on https://github.com/mwarning/mbedtls_ecp_compression committed
     * to Public Domain.
     */

    mbedtls_ecp_group* ec = (mbedtls_ecp_group*)e;
    mbedtls_ecp_point* ecp_p = (mbedtls_ecp_point*)p;
    mbedtls_mpi* mpi_x = (mbedtls_mpi*)x;
    mbedtls_mpi mpi_r;
    mbedtls_mpi mpi_n;
    int ret = -1;

    if (ec && ecp_p && mpi_x) {
        mbedtls_mpi_init(&mpi_r);
        mbedtls_mpi_init(&mpi_n);

        /* r = x^2 */
        ret = mbedtls_mpi_mul_mpi(&mpi_r, mpi_x, mpi_x);
        if (!ret) {
            /* r = x^2 + a */
            ret = mbedtls_mpi_add_mpi(&mpi_r, &mpi_r, &ec->A);
        }
        if (!ret) {
            /* r = x^3 + ax */
            ret = mbedtls_mpi_mul_mpi(&mpi_r, &mpi_r, mpi_x);
        }
        if (!ret) {
            /* r = x^3 + ax + b */
            ret = mbedtls_mpi_add_mpi(&mpi_r, &mpi_r, &ec->B);
        }
        if (!ret) {
            /* n = P + 1 */
            ret = mbedtls_mpi_add_int(&mpi_n, &ec->P, 1);
        }
        if (!ret) {
            /* n = (P + 1) / 4 */
            ret = mbedtls_mpi_shift_r(&mpi_n, 2);
        }
        if (!ret) {
            /* r = r ^ ((P + 1) / 4) mod P) */
            ret = mbedtls_mpi_exp_mod(&mpi_r, &mpi_r, &mpi_n, &ec->P, NULL);
        }
        if (!ret) {
            if (mbedtls_mpi_get_bit(&mpi_r, 0) != y_bit) {
                ret = mbedtls_mpi_sub_mpi(&mpi_r, &ec->P, &mpi_r);
            }
        }
        if (!ret) {
            ret = mbedtls_mpi_copy(&ecp_p->Y, &mpi_r);
        }
        if (!ret) {
            ret = mbedtls_mpi_copy(&ecp_p->X, mpi_x);
        }
        if (!ret) {
            ret = mbedtls_mpi_lset(&ecp_p->Z, 1);
        }

        mbedtls_mpi_free(&mpi_n);
        mbedtls_mpi_free(&mpi_r);
    }

    return ret;
}

struct crypto_bignum* crypto_ec_point_compute_y_sqr(
    struct crypto_ec* e,
    const struct crypto_bignum* x)
{
    mbedtls_ecp_group* ec = (mbedtls_ecp_group*)e;
    mbedtls_mpi* mpi_x = (mbedtls_mpi*)x;
    mbedtls_mpi* mpi_r = NULL;
    int ret = -1;

    if (ec && mpi_x) {
        mpi_r = (mbedtls_mpi*)crypto_bignum_init();
        if (mpi_r) {
            ret = mbedtls_mpi_mul_mpi(mpi_r, mpi_x, mpi_x);
            if (!ret) {
                ret = mbedtls_mpi_add_mpi(mpi_r, mpi_r, &ec->A);
            }
            if (!ret) {
                ret = mbedtls_mpi_mul_mpi(mpi_r, mpi_r, mpi_x);
            }
            if (!ret) {
                ret = mbedtls_mpi_add_mpi(mpi_r, mpi_r, &ec->B);
            }
            if (!ret) {
                ret = mbedtls_mpi_mod_mpi(mpi_r, mpi_r, &ec->P);
            }
            if (ret) {
                crypto_bignum_deinit((struct crypto_bignum*)mpi_r, false);
                mpi_r = NULL;
            }
        }
    }

    return (struct crypto_bignum*)mpi_r;
}

int crypto_ec_point_is_on_curve(
    struct crypto_ec* e,
    const struct crypto_ec_point* p)
{
    mbedtls_ecp_group* ec = (mbedtls_ecp_group*)e;
    mbedtls_ecp_point* ecp = (mbedtls_ecp_point*)p;
    int ret = -1;

    if (ec && ecp) {
        ret = !mbedtls_ecp_check_pubkey(ec, ecp);
    }

    return ret;
}

int crypto_ec_point_is_at_infinity(
    struct crypto_ec* e,
    const struct crypto_ec_point* p)
{
    mbedtls_ecp_group* ec = (mbedtls_ecp_group*)e;
    mbedtls_ecp_point* ecp = (mbedtls_ecp_point*)p;
    int ret = -1;

    if (ecp) {
        ret = mbedtls_ecp_is_zero(ecp);
    }

    return ret;
}

int crypto_ec_point_add(
    struct crypto_ec* e,
    const struct crypto_ec_point* a,
    const struct crypto_ec_point* b,
    struct crypto_ec_point* c)
{
    mbedtls_ecp_group* ec = (mbedtls_ecp_group*)e;
    mbedtls_ecp_point* ecp_a = (mbedtls_ecp_point*)a;
    mbedtls_ecp_point* ecp_b = (mbedtls_ecp_point*)b;
    mbedtls_ecp_point* ecp_c = (mbedtls_ecp_point*)c;
    mbedtls_mpi mpi_one;
    int ret = -1;

    if (ec && ecp_a && ecp_b && ecp_c) {
        mbedtls_mpi_init(&mpi_one);

        ret = mbedtls_mpi_lset(&mpi_one, 1);
        if (!ret) {
            ret = mbedtls_ecp_muladd(ec, ecp_c, &mpi_one, ecp_a, &mpi_one, ecp_b);
        }

        mbedtls_mpi_free(&mpi_one);
    }

    return ret;
}

int crypto_ec_point_cmp(
    const struct crypto_ec* e,
    const struct crypto_ec_point* a,
    const struct crypto_ec_point* b)
{
    mbedtls_ecp_group* ec = (mbedtls_ecp_group*)e;
    mbedtls_ecp_point* ecp_a = (mbedtls_ecp_point*)a;
    mbedtls_ecp_point* ecp_b = (mbedtls_ecp_point*)b;
    int ret = -1;

    if (ecp_a && ecp_b) {
        ret = mbedtls_ecp_point_cmp(ecp_a, ecp_b);
    }

    return ret;
}

int crypto_ec_point_to_bin(
    struct crypto_ec* e,
    const struct crypto_ec_point* point,
    u8* x,
    u8* y)

{
    mbedtls_ecp_group* ec = (mbedtls_ecp_group*)e;
    mbedtls_ecp_point* ecp = (mbedtls_ecp_point*)point;
    size_t len;
    int ret = -1;

    if (ec && ecp) {
        len = crypto_ec_prime_len(e);
        ret = 0;
        if (!ret && x) {
            ret = mbedtls_mpi_write_binary(&ecp->X, x, len);
        }
        if (!ret && y) {
            ret = mbedtls_mpi_write_binary(&ecp->Y, y, len);
        }
    }

    return ret;
}

struct crypto_ec_point* crypto_ec_point_from_bin(
    struct crypto_ec* e,
    const u8* val)
{
    mbedtls_ecp_group* ec = (mbedtls_ecp_group*)e;
    mbedtls_ecp_point* ecp = NULL;
    size_t len;
    int ret;

    if (ec) {
        ecp = (mbedtls_ecp_point*)crypto_ec_point_init(e);
        if (ecp) {
            len = crypto_ec_prime_len(e);
            ret = mbedtls_mpi_read_binary(&ecp->X, val, len);
            if (!ret) {
                ret = mbedtls_mpi_read_binary(&ecp->Y, val + len, len);
            }
            if (!ret) {
                ret = mbedtls_mpi_lset(&ecp->Z, 1);
            }
            if (ret) {
                crypto_ec_point_deinit((struct crypto_ec_point*)ecp, false);
                ecp = NULL;
            }
        }
    }

    return (struct crypto_ec_point*)ecp;
}



#if 0
int md4_vector(size_t num_elem, const u8 *addr[], const size_t *len, u8 *mac)
{
    return -1;
}

int md5_vector(size_t num_elem, const u8 *addr[], const size_t *len, u8 *mac)
{
    return -1;
}

int sha1_vector(size_t num_elem, const u8 *addr[], const size_t *len, u8 *mac)
{
    return -1;
}

int __must_check fips186_2_prf(const u8 *seed, size_t seed_len, u8 *x, size_t xlen)
{
    return -1;
}

int sha256_vector(size_t num_elem, const u8 *addr[], const size_t *len, u8 *mac)
{
    return -1;
}

int sha384_vector(size_t num_elem, const u8 *addr[], const size_t *len, u8 *mac)
{
    return -1;
}

int sha512_vector(size_t num_elem, const u8 *addr[], const size_t *len, u8 *mac)
{
    return -1;
}

int des_encrypt(const u8 *clear, const u8 *key, u8 *cypher)
{
    return -1;
}

void * aes_encrypt_init(const u8 *key, size_t len)
{
    return NULL;
}

int aes_encrypt(void *ctx, const u8 *plain, u8 *crypt)
{
    return -1;
}

void aes_encrypt_deinit(void *ctx)
{
}

void * aes_decrypt_init(const u8 *key, size_t len)
{
    return NULL;
}

int aes_decrypt(void *ctx, const u8 *crypt, u8 *plain)
{
    return -1;
}

void aes_decrypt_deinit(void *ctx)
{
}

struct crypto_hash * crypto_hash_init(enum crypto_hash_alg alg, const u8 *key, size_t key_len)
{
    return NULL;
}

void crypto_hash_update(struct crypto_hash *ctx, const u8 *data, size_t len)
{
}

int crypto_hash_finish(struct crypto_hash *ctx, u8 *hash, size_t *len)
{
    return -1;
}

struct crypto_cipher * crypto_cipher_init(enum crypto_cipher_alg alg, const u8 *iv, const u8 *key, size_t key_len)
{
    return NULL;
}

int __must_check crypto_cipher_encrypt(struct crypto_cipher *ctx, const u8 *plain, u8 *crypt, size_t len)
{
    return -1;
}

int __must_check crypto_cipher_decrypt(struct crypto_cipher *ctx, const u8 *crypt, u8 *plain, size_t len)
{
    return -1;
}

void crypto_cipher_deinit(struct crypto_cipher *ctx)
{
}

struct crypto_public_key * crypto_public_key_import(const u8 *key, size_t len)
{
    return NULL;
}

struct crypto_public_key *crypto_public_key_import_parts(const u8 *n, size_t n_len, const u8 *e, size_t e_len)
{
    return NULL;
}

struct crypto_private_key * crypto_private_key_import(const u8 *key, size_t len, const char *passwd)
{
    return NULL;
}

struct crypto_public_key * crypto_public_key_from_cert(const u8 *buf, size_t len)
{
    return NULL;
}

int __must_check crypto_public_key_encrypt_pkcs1_v15(struct crypto_public_key *key, const u8 *in, size_t inlen, u8 *out, size_t *outlen)
{
    return -1;
}

int __must_check crypto_private_key_decrypt_pkcs1_v15(struct crypto_private_key *key, const u8 *in, size_t inlen, u8 *out, size_t *outlen)
{
    return -1;
}

int __must_check crypto_private_key_sign_pkcs1(struct crypto_private_key *key, const u8 *in, size_t inlen, u8 *out, size_t *outlen)
{
    return -1;
}

void crypto_public_key_free(struct crypto_public_key *key)
{
}

void crypto_private_key_free(struct crypto_private_key *key)
{
}

int __must_check crypto_public_key_decrypt_pkcs1(struct crypto_public_key *key, const u8 *crypt, size_t crypt_len, u8 *plain, size_t *plain_len)
{
    return -1;
}

int crypto_dh_init(u8 generator, const u8 *prime, size_t prime_len, u8 *privkey, u8 *pubkey)
{
    return -1;
}

int crypto_dh_derive_secret(u8 generator, const u8 *prime, size_t prime_len, const u8 *privkey, size_t privkey_len, const u8 *pubkey, size_t pubkey_len, u8 *secret, size_t *len)
{
    return -1;
}

int __must_check crypto_global_init(void)
{
}

void crypto_global_deinit(void)
{
}

int __must_check crypto_mod_exp(const u8 *base, size_t base_len, const u8 *power, size_t power_len, const u8 *modulus, size_t modulus_len, u8 *result, size_t *result_len)
{
    return -1;
}

int rc4_skip(const u8 *key, size_t keylen, size_t skip, u8 *data, size_t data_len)
{
    return -1;
}

int crypto_get_random(void *buf, size_t len)
{
    return -1;
}

int crypto_bignum_rand(struct crypto_bignum *r, const struct crypto_bignum *m)
{
    return -1;
}

int crypto_bignum_rshift(const struct crypto_bignum *a, int n, struct crypto_bignum *r)
{
    return -1;
}

int crypto_bignum_is_odd(const struct crypto_bignum *a)
{
    return -1;
}

int crypto_ec_cofactor(struct crypto_ec *e, struct crypto_bignum *cofactor)
{
    return -1;
}

size_t crypto_ec_order_len(struct crypto_ec *e)
{
    return 0;
}

int crypto_ec_point_x(struct crypto_ec *e, const struct crypto_ec_point *p, struct crypto_bignum *x)
{
    return -1;
}

struct crypto_ecdh * crypto_ecdh_init(int group)
{
    return NULL;
}

struct wpabuf * crypto_ecdh_get_pubkey(struct crypto_ecdh *ecdh, int inc_y)
{
    return NULL;
}

struct wpabuf * crypto_ecdh_set_peerkey(struct crypto_ecdh *ecdh, int inc_y, const u8 *key, size_t len)
{
    return NULL;
}

void crypto_ecdh_deinit(struct crypto_ecdh *ecdh)
{
}
#endif
