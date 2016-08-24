/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_NV_SPT_FP_H
#define __TPM2_NV_SPT_FP_H

TPM_RC NvReadAccessChecks(
    TPM_HANDLE authHandle,  // IN: the handle that provided the authorization
    TPM_HANDLE nvHandle     // IN: the handle of the NV index to be written
    );
TPM_RC NvWriteAccessChecks(
    TPM_HANDLE authHandle,  // IN: the handle that provided the authorization
    TPM_HANDLE nvHandle     // IN: the handle of the NV index to be written
    );

#endif  // __TPM2_NV_SPT_FP_H
