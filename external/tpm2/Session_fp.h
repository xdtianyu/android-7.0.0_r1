/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_SESSION_FP_H
#define __TPM2_SESSION_FP_H

UINT32 SessionCapGetActiveAvail(void);
UINT32 SessionCapGetActiveNumber(void);
TPMI_YES_NO SessionCapGetLoaded(TPMI_SH_POLICY handle,  // IN: start handle
                                UINT32 count,  // IN: count of returned handle
                                TPML_HANDLE *handleList  // OUT: list of handle
                                );
UINT32 SessionCapGetLoadedAvail(void);
UINT32 SessionCapGetLoadedNumber(void);
TPMI_YES_NO SessionCapGetSaved(TPMI_SH_HMAC handle,  // IN: start handle
                               UINT32 count,  // IN: count of returned handle
                               TPML_HANDLE *handleList  // OUT: list of handle
                               );
void SessionComputeBoundEntity(
    TPMI_DH_ENTITY entityHandle,  // IN: handle of entity
    TPM2B_NAME *bind              // OUT: binding value
    );
TPM_RC SessionContextLoad(
    SESSION *session,   // IN: session structure from saved context
    TPM_HANDLE *handle  // IN/OUT: session handle
    );
TPM_RC SessionContextSave(TPM_HANDLE handle,          // IN: session handle
                          CONTEXT_COUNTER *contextID  // OUT: assigned contextID
                          );
TPM_RC SessionCreate(TPM_SE sessionType,        //   IN: the session type
                     TPMI_ALG_HASH authHash,    //   IN: the hash algorithm
                     TPM2B_NONCE *nonceCaller,  //   IN: initial nonceCaller
                     TPMT_SYM_DEF *symmetric,   //   IN: the symmetric algorithm
                     TPMI_DH_ENTITY bind,       //   IN: the bind object
                     TPM2B_DATA *seed,          //   IN: seed data
                     TPM_HANDLE *sessionHandle  //   OUT: the session handle
                     );
void SessionFlush(TPM_HANDLE handle  // IN: loaded or saved session handle
                  );
SESSION *SessionGet(TPM_HANDLE handle  // IN: session handle
                    );
void SessionInitPolicyData(SESSION *session  // IN: session handle
                           );
BOOL SessionIsLoaded(TPM_HANDLE handle  // IN: session handle
                     );
BOOL SessionIsSaved(TPM_HANDLE handle  // IN: session handle
                    );
BOOL SessionPCRValueIsCurrent(TPMI_SH_POLICY handle  // IN: session handle
                              );
void SessionResetPolicyData(SESSION *session  // IN: the session to reset
                            );
void SessionStartup(STARTUP_TYPE type);

#endif  // __TPM2_SESSION_FP_H
