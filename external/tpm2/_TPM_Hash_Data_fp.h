/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2__TPM_HASH_DATA_FP_H
#define __TPM2__TPM_HASH_DATA_FP_H

void _TPM_Hash_Data(UINT32 dataSize,  // IN: size of data to be extend
                    BYTE *data        // IN: data buffer
                    );

#endif  // __TPM2__TPM_HASH_DATA_FP_H
