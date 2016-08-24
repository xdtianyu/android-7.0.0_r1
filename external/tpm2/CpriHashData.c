// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "CryptoEngine.h"

  const HASH_INFO    g_hashData[HASH_COUNT + 1] = {
#ifdef TPM_ALG_SHA1
   {TPM_ALG_SHA1,     SHA1_DIGEST_SIZE,   SHA1_BLOCK_SIZE,
    SHA1_DER_SIZE,    {SHA1_DER}},
#endif
#ifdef TPM_ALG_SHA256
   {TPM_ALG_SHA256,     SHA256_DIGEST_SIZE,   SHA256_BLOCK_SIZE,
    SHA256_DER_SIZE,    {SHA256_DER}},
#endif
#ifdef TPM_ALG_SHA384
   {TPM_ALG_SHA384,     SHA384_DIGEST_SIZE,   SHA384_BLOCK_SIZE,
    SHA384_DER_SIZE,    {SHA384_DER}},
#endif
#ifdef TPM_ALG_SM3_256
   {TPM_ALG_SM3_256,     SM3_256_DIGEST_SIZE,   SM3_256_BLOCK_SIZE,
    SM3_256_DER_SIZE,    {SM3_256_DER}},
#endif
#ifdef TPM_ALG_SHA512
   {TPM_ALG_SHA512,     SHA512_DIGEST_SIZE,   SHA512_BLOCK_SIZE,
    SHA512_DER_SIZE,    {SHA512_DER}},
#endif
      {TPM_ALG_NULL,0,0,0,{0}}
  };
