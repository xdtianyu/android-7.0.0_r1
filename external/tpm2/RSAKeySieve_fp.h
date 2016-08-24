/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_RSAKEYSIEVE_FP_H
#define __TPM2_RSAKEYSIEVE_FP_H

LIB_EXPORT CRYPT_RESULT _cpri__GenerateKeyRSA(
    TPM2B *n,              // OUT: The public modulus
    TPM2B *p,              // OUT: One of the prime factors of n
    UINT16 keySizeInBits,  // IN: Size of the public modulus in bits
    UINT32 e,              // IN: The public exponent
    TPM_ALG_ID
        hashAlg,  // IN: hash algorithm to use in the key generation process
    TPM2B *seed,  // IN: the seed to use
    const char *label,  // IN: A label for the generation process.
    TPM2B *extra,       // IN: Party 1 data for the KDF
    UINT32 *counter     // IN/OUT: Counter value to allow KDF iteration to be
                        // propagated across multiple routines
#ifdef RSA_DEBUG
    ,
    UINT16 primes,    // IN: number of primes to test
    UINT16 fieldSize  // IN: the field size to use
#endif
    );

#endif  // __TPM2_RSAKEYSIEVE_FP_H
