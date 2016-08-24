/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_CPRIRNG_FP_H
#define __TPM2_CPRIRNG_FP_H

LIB_EXPORT CRYPT_RESULT
_cpri__DrbgGetPutState(GET_PUT direction, int bufferSize, BYTE *buffer);
LIB_EXPORT UINT16 _cpri__GenerateRandom(INT32 randomSize, BYTE *buffer);
LIB_EXPORT UINT16 _cpri__GenerateSeededRandom(
    INT32 randomSize,    //   IN: the size of the request
    BYTE *random,        //   OUT: receives the data
    TPM_ALG_ID hashAlg,  //   IN: used by KDF version but not here
    TPM2B *seed,         //   IN: the seed value
    const char *label,   //   IN: a label string (optional)
    TPM2B *partyU,       //   IN: other data (oprtional)
    TPM2B *partyV        //   IN: still more (optional)
    );
LIB_EXPORT CRYPT_RESULT _cpri__StirRandom(INT32 entropySize, BYTE *entropy);
LIB_EXPORT BOOL _cpri__RngStartup(void);

#endif  // __TPM2_CPRIRNG_FP_H
