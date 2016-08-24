/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_MANUFACTURE_FP_H
#define __TPM2_MANUFACTURE_FP_H

#include "bool.h"
#include "TpmBuildSwitches.h"

LIB_EXPORT int TPM_Manufacture(
    BOOL firstTime  // IN: indicates if this is the first call from main()
    );

#ifndef EMBEDDED_MODE
LIB_EXPORT int TPM_TearDown(
    void
    );
#endif
#endif  // __TPM2_MANUFACTURE_FP_H
