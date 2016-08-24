/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_COMMANDAUDIT_FP_H
#define __TPM2_COMMANDAUDIT_FP_H

TPMI_YES_NO CommandAuditCapGetCCList(
    TPM_CC commandCode,   // IN: start command code
    UINT32 count,         // IN: count of returned TPM_CC
    TPML_CC *commandList  // OUT: list of TPM_CC
    );
BOOL CommandAuditClear(TPM_CC commandCode  // IN: command code
                       );
void CommandAuditGetDigest(TPM2B_DIGEST *digest  // OUT: command digest
                           );
BOOL CommandAuditIsRequired(TPM_CC commandCode  // IN: command code
                            );
void CommandAuditPreInstall_Init(void);
BOOL CommandAuditSet(TPM_CC commandCode  // IN: command code
                     );

#endif                                      // __TPM2_COMMANDAUDIT_FP_H
void CommandAuditStartup(STARTUP_TYPE type  // IN: start up type
                         );
