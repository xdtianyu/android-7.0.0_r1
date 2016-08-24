/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_DA_FP_H
#define __TPM2_DA_FP_H

void DAPreInstall_Init(void);
void DARegisterFailure(TPM_HANDLE handle  // IN: handle for failure
                       );
void DASelfHeal(void);
void DAStartup(STARTUP_TYPE type  // IN: startup type
               );

#endif  // __TPM2_DA_FP_H
