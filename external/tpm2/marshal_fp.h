/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_MARSHAL_FP_H
#define __TPM2_MARSHAL_FP_H

UINT16 Common_Marshal(void *source, BYTE **buffer, INT32 *size,
                      UINT16 type_size);
TPM_RC Common_Unmarshal(void *source, BYTE **buffer, INT32 *size,
                        UINT16 type_size);

#define MARSHAL_WRAPPER(name)                                          \
  static inline UINT16 name##_Marshal(void *x, BYTE **y, INT32 *z) {   \
    return Common_Marshal(x, y, z, sizeof(name));                      \
  }                                                                    \
  static inline TPM_RC name##_Unmarshal(void *x, BYTE **y, INT32 *z) { \
    return Common_Unmarshal(x, y, z, sizeof(name));                    \
  }

MARSHAL_WRAPPER(SESSION)
MARSHAL_WRAPPER(TPM2B_AUTH)
MARSHAL_WRAPPER(TPM2B_DIGEST)
MARSHAL_WRAPPER(TPM2B_IV)
MARSHAL_WRAPPER(TPM2B_NONCE)
MARSHAL_WRAPPER(TPMA_LOCALITY)
MARSHAL_WRAPPER(TPMA_SESSION)
MARSHAL_WRAPPER(TPMI_SH_AUTH_SESSION)
MARSHAL_WRAPPER(TPMI_ST_COMMAND_TAG)
MARSHAL_WRAPPER(TPML_PCR_SELECTION)
MARSHAL_WRAPPER(TPMS_ATTEST)
MARSHAL_WRAPPER(TPMS_CREATION_DATA)
MARSHAL_WRAPPER(TPMS_ECC_POINT)
MARSHAL_WRAPPER(TPMS_NV_PUBLIC)
MARSHAL_WRAPPER(TPMS_TIME_INFO)
MARSHAL_WRAPPER(TPMT_PUBLIC)
MARSHAL_WRAPPER(TPMT_SENSITIVE)
MARSHAL_WRAPPER(TPM_CC)
MARSHAL_WRAPPER(TPM_GENERATED)
MARSHAL_WRAPPER(TPM_HANDLE)
MARSHAL_WRAPPER(TPM_RC)
MARSHAL_WRAPPER(TPM_ST)
MARSHAL_WRAPPER(UINT16)
MARSHAL_WRAPPER(UINT32)

UINT16 TPMU_PUBLIC_PARMS_Marshal(TPMU_PUBLIC_PARMS *x, BYTE **y, INT32 *z,
                                 TPMI_ALG_PUBLIC type);

#endif  // __TPM2_MARSHAL_FP_H
