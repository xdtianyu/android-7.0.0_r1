/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_TPMFAIL_FP_H
#define __TPM2_TPMFAIL_FP_H

void TpmFailureMode(
    unsigned int inRequestSize,     //   IN: command buffer size
    unsigned char *inRequest,       //   IN: command buffer
    unsigned int *outResponseSize,  //   OUT: response buffer size
    unsigned char **outResponse     //   OUT: response buffer
    );

#endif  // __TPM2_TPMFAIL_FP_H
