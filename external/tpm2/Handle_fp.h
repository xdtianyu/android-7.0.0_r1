/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_HANDLE_FP_H
#define __TPM2_HANDLE_FP_H

TPM_HT HandleGetType(TPM_HANDLE handle  // IN: a handle to be checked
                     );
TPMI_YES_NO PermanentCapGetHandles(
    TPM_HANDLE handle,       // IN: start handle
    UINT32 count,            // IN: count of returned handle
    TPML_HANDLE *handleList  // OUT: list of handle
    );

#endif  // __TPM2_HANDLE_FP_H
