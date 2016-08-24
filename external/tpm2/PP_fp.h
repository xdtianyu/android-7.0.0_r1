/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_PP_FP_H
#define __TPM2_PP_FP_H

TPMI_YES_NO PhysicalPresenceCapGetCCList(
    TPM_CC commandCode,   // IN: start command code
    UINT32 count,         // IN: count of returned TPM_CC
    TPML_CC *commandList  // OUT: list of TPM_CC
    );
void PhysicalPresenceCommandClear(TPM_CC commandCode  // IN: command code
                                  );
void PhysicalPresenceCommandSet(TPM_CC commandCode  // IN: command code
                                );
BOOL PhysicalPresenceIsRequired(TPM_CC commandCode  // IN: command code
                                );
void PhysicalPresencePreInstall_Init(void);

#endif  // __TPM2_PP_FP_H
