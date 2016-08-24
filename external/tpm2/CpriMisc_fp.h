/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_CPRIMISC_FP_H
#define __TPM2_CPRIMISC_FP_H

BIGNUM *BnFrom2B(BIGNUM *out,     // OUT: The BIGNUM
                 const TPM2B *in  // IN: the TPM2B to copy
                 );
BOOL BnTo2B(TPM2B *outVal,  // OUT: place for the result
            BIGNUM *inVal,  // IN: number to convert
            UINT16 size     // IN: size of the output.
            );
void Copy2B(TPM2B *out,  // OUT: The TPM2B to receive the copy
            TPM2B *in    // IN: the TPM2B to copy
            );

#endif  // __TPM2_CPRIMISC_FP_H
