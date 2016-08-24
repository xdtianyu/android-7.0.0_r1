/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_UNIQUE_FP_H_
#define __TPM2_UNIQUE_FP_H_

LIB_EXPORT uint32_t
_plat__GetUnique(uint32_t which,   // authorities (0) or details
                 uint32_t bSize,   // size of the buffer
                 unsigned char *b  // output buffer
                 );

#endif  // __TPM2_UNIQUE_FP_H_
