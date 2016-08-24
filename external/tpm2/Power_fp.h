/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_POWER_FP_H
#define __TPM2_POWER_FP_H

BOOL TPMIsStarted(void);
void TPMInit(void);
void TPMRegisterStartup(void);

#endif  // __TPM2_POWER_FP_H
