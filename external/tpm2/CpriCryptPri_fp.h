/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_CPRICRYPTPRI_FP_H
#define __TPM2_CPRICRYPTPRI_FP_H

LIB_EXPORT CRYPT_RESULT _cpri__InitCryptoUnits(FAIL_FUNCTION failFunction);
LIB_EXPORT BOOL _cpri__Startup(void);
LIB_EXPORT void _cpri__StopCryptoUnits(void);

#endif  // __TPM2_CPRICRYPTPRI_FP_H
