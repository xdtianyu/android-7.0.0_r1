/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_POLICY_SPT_FP_H_
#define __TPM2_POLICY_SPT_FP_H_

void PolicyContextUpdate(
    TPM_CC commandCode,    //   IN:   command code
    TPM2B_NAME *name,      //   IN:   name of entity
    TPM2B_NONCE *ref,      //   IN:   the reference data
    TPM2B_DIGEST *cpHash,  //   IN:   the cpHash (optional)
    UINT64 policyTimeout,
    SESSION *session  // IN/OUT: policy session to be updated
    );
TPM_RC PolicyParameterChecks(SESSION *session, UINT64 authTimeout,
                             TPM2B_DIGEST *cpHashA, TPM2B_NONCE *nonce,
                             TPM_RC nonceParameterNumber,
                             TPM_RC cpHashParameterNumber,
                             TPM_RC expirationParameterNumber);

#endif  // __TPM2_POLICY_SPT_FP_H_
