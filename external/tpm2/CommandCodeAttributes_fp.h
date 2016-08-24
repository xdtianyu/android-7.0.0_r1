/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_COMMANDCODEATTRIBUTES_FP_H
#define __TPM2_COMMANDCODEATTRIBUTES_FP_H

AUTH_ROLE CommandAuthRole(TPM_CC commandCode,  // IN: command code
                          UINT32 handleIndex   // IN: handle index (zero based)
                          );
TPMI_YES_NO CommandCapGetCCList(
    TPM_CC commandCode,  // IN: start command code
    UINT32 count,  // IN: maximum count for number of entries in 'commandList'
    TPML_CCA *commandList  // OUT: list of TPMA_CC
    );
BOOL CommandIsImplemented(TPM_CC commandCode  // IN: command code
                          );
int DecryptSize(TPM_CC commandCode  // IN: commandCode
                );
int EncryptSize(TPM_CC commandCode  // IN: commandCode
                );
BOOL IsReadOperation(TPM_CC command  // IN: Command to check
                     );
BOOL IsSessionAllowed(TPM_CC commandCode  // IN: the command to be checked
                      );
BOOL IsWriteOperation(TPM_CC command  // IN: Command to check
                      );

#endif  // __TPM2_COMMANDCODEATTRIBUTES_FP_H
