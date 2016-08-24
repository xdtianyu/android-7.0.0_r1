/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_ALGORITHMCAP_FP_H
#define __TPM2_ALGORITHMCAP_FP_H

LIB_EXPORT void AlgorithmGetImplementedVector(
    ALGORITHM_VECTOR *implemented  // OUT: the implemented bits are SET
    );

TPMI_YES_NO AlgorithmCapGetImplemented(
    TPM_ALG_ID algID,           // IN: the starting algorithm ID
    UINT32 count,               // IN: count of returned algorithms
    TPML_ALG_PROPERTY *algList  // OUT: algorithm list
    );

#endif  // __TPM2_ALGORITHMCAP_FP_H
