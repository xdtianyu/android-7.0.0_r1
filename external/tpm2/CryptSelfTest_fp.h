/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_CRYPTSELFTEST_FP_H
#define __TPM2_CRYPTSELFTEST_FP_H

TPM_RC CryptIncrementalSelfTest(
    TPML_ALG *toTest,   // IN: list of algorithms to be tested
    TPML_ALG *toDoList  // OUT: list of algorithms needing test
    );
void CryptInitializeToTest(void);
TPM_RC CryptSelfTest(TPMI_YES_NO fullTest  // IN: if full test is required
                     );
TPM_RC CryptTestAlgorithm(TPM_ALG_ID alg, ALGORITHM_VECTOR *toTest);

#endif  // __TPM2_CRYPTSELFTEST_FP_H
