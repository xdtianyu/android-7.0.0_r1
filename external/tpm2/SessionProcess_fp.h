/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_SESSIONPROCESS_FP_H
#define __TPM2_SESSIONPROCESS_FP_H

void BuildResponseSession(
    TPM_ST tag,             //    IN: tag
    TPM_CC commandCode,     //    IN: commandCode
    UINT32 resHandleSize,   //    IN: size of response handle buffer
    UINT32 resParmSize,     //    IN: size of response parameter buffer
    UINT32 *resSessionSize  //    OUT: response session area
    );
TPM_RC CheckAuthNoSession(
    TPM_CC commandCode,     //   IN:   Command Code
    UINT32 handleNum,       //   IN:   number of handles in command
    TPM_HANDLE handles[],   //   IN:   array of handle
    BYTE *parmBufferStart,  //   IN:   start of parameter buffer
    UINT32 parmBufferSize   //   IN:   size of parameter buffer
    );
BOOL IsDAExempted(TPM_HANDLE handle  // IN: entity handle
                  );
TPM_RC ParseSessionBuffer(
    TPM_CC commandCode,        //   IN:   Command code
    UINT32 handleNum,          //   IN:   number of element in handle array
    TPM_HANDLE handles[],      //   IN:   array of handle
    BYTE *sessionBufferStart,  //   IN:   start of session buffer
    UINT32 sessionBufferSize,  //   IN:   size of session buffer
    BYTE *parmBufferStart,     //   IN:   start of parameter buffer
    UINT32 parmBufferSize      //   IN:   size of parameter buffer
    );

#endif  // __TPM2_SESSIONPROCESS_FP_H
