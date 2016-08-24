/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_MATHFUNCTIONS_FP_H
#define __TPM2_MATHFUNCTIONS_FP_H

LIB_EXPORT int _math__Comp(const UINT32 aSize,  //   IN:   size of a
                           const BYTE *a,       //   IN:   a buffer
                           const UINT32 bSize,  //   IN:   size of b
                           const BYTE *b        //   IN:   b buffer
                           );
LIB_EXPORT CRYPT_RESULT _math__Div(const TPM2B *n,  //   IN: numerator
                                   const TPM2B *d,  //   IN: denominator
                                   TPM2B *q,        //   OUT: quotient
                                   TPM2B *r         //   OUT: remainder
                                   );
LIB_EXPORT BOOL _math__IsPrime(const UINT32 prime);
LIB_EXPORT CRYPT_RESULT
_math__ModExp(UINT32 cSize,        //   IN: size of the result
              BYTE *c,             //   OUT: results buffer
              const UINT32 mSize,  //   IN: size of number to be exponentiated
              const BYTE *m,       //   IN: number to be exponentiated
              const UINT32 eSize,  //   IN: size of power
              const BYTE *e,       //   IN: power
              const UINT32 nSize,  //   IN: modulus size
              const BYTE *n        //   IN: modulu
              );
LIB_EXPORT UINT16 _math__Normalize2B(TPM2B *b  // IN/OUT: number to normalize
                                     );
LIB_EXPORT int _math__sub(const UINT32 aSize,  //   IN: size   of a
                          const BYTE *a,       //   IN: a
                          const UINT32 bSize,  //   IN: size   of b
                          const BYTE *b,       //   IN: b
                          UINT16 *cSize,  //   OUT: set   to MAX(aSize, bSize)
                          BYTE *c         //   OUT: the   difference
                          );
LIB_EXPORT int _math__uComp(const UINT32 aSize,  // IN: size of a
                            const BYTE *a,       // IN: a
                            const UINT32 bSize,  // IN: size of b
                            const BYTE *b        // IN: b
                            );

#endif  // __TPM2_MATHFUNCTIONS_FP_H
