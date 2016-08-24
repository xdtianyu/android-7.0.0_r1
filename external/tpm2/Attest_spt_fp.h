/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_ATTEST_SPT_FP_H
#define __TPM2_ATTEST_SPT_FP_H
TPM_RC FillInAttestInfo(
    TPMI_DH_OBJECT signHandle,  //   IN: handle of signing object
    TPMT_SIG_SCHEME *scheme,    //   IN/OUT: scheme to be used for signing
    TPM2B_DATA *data,           //   IN: qualifying data
    TPMS_ATTEST *attest         //   OUT: attest structure
    );
TPM_RC SignAttestInfo(
    TPMI_DH_OBJECT signHandle,   //   IN: handle of sign object
    TPMT_SIG_SCHEME *scheme,     //   IN: sign scheme
    TPMS_ATTEST *certifyInfo,    //   IN: the data to be signed
    TPM2B_DATA *qualifyingData,  //   IN: extra data for the signing proce
    TPM2B_ATTEST *attest,        //   OUT: marshaled attest blob to be signed
    TPMT_SIGNATURE *signature    //   OUT: signature
    );
#endif  // __TPM2_ATTEST_SPT_FP_H
