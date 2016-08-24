// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#ifndef _OSSL_CRYPTO_ENGINE_H
#define _OSSL_CRYPTO_ENGINE_H
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/ec.h>
#include <openssl/rand.h>
#include <openssl/bn.h>
#define     CRYPTO_ENGINE
#include "CryptoEngine.h"
#include "CpriMisc_fp.h"
#define MAX_ECC_PARAMETER_BYTES 32
#define MAX_2B_BYTES MAX((MAX_RSA_KEY_BYTES * ALG_RSA),                              \
                         MAX((MAX_ECC_PARAMETER_BYTES * ALG_ECC),                   \
                             MAX_DIGEST_SIZE))
#define assert2Bsize(a) pAssert((a).size <= sizeof((a).buffer))
#ifdef TPM_ALG_RSA
#   ifdef   RSA_KEY_SIEVE
#       include     "RsaKeySieve.h"
#       include     "RsaKeySieve_fp.h"
#   endif
#   include    "CpriRSA_fp.h"
#endif

#ifdef OPENSSL_IS_BORINGSSL
// libtpm2 reads internal EVP_MD state (e.g. ctx_size). The boringssl headers
// don't expose this type so define it here.
struct env_md_st {
  /* type contains a NID identifing the digest function. (For example,
   * NID_md5.) */
  int type;

  /* md_size contains the size, in bytes, of the resulting digest. */
  unsigned md_size;

  /* flags contains the OR of |EVP_MD_FLAG_*| values. */
  uint32_t flags;

  /* init initialises the state in |ctx->md_data|. */
  void (*init)(EVP_MD_CTX *ctx);

  /* update hashes |len| bytes of |data| into the state in |ctx->md_data|. */
  void (*update)(EVP_MD_CTX *ctx, const void *data, size_t count);

  /* final completes the hash and writes |md_size| bytes of digest to |out|. */
  void (*final)(EVP_MD_CTX *ctx, uint8_t *out);

  /* block_size contains the hash's native block size. */
  unsigned block_size;

  /* ctx_size contains the size, in bytes, of the state of the hash function. */
  unsigned ctx_size;
};
#endif

//
//     This is a structure to hold the parameters for the version of KDFa() used by the CryptoEngine(). This
//     structure allows the state to be passed between multiple functions that use the same pseudo-random
//     sequence.
//
typedef struct {
   CPRI_HASH_STATE          iPadCtx;
   CPRI_HASH_STATE          oPadCtx;
   TPM2B                   *extra;
   UINT32                  *outer;
   TPM_ALG_ID               hashAlg;
   UINT16                   keySizeInBits;
} KDFa_CONTEXT;
#endif // _OSSL_CRYPTO_ENGINE_H
