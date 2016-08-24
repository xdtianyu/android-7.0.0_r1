/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_PCR_FP_H
#define __TPM2_PCR_FP_H

TPM_RC PCRAllocate(TPML_PCR_SELECTION *allocate,  //   IN: required allocation
                   UINT32 *maxPCR,        //   OUT: Maximum number of PCR
                   UINT32 *sizeNeeded,    //   OUT: required space
                   UINT32 *sizeAvailable  //   OUT: available space
                   );
BOOL PCRBelongsAuthGroup(TPMI_DH_PCR handle,  // IN: handle of PCR
                         UINT32 *groupIndex   // OUT: group index if PCR belongs
                                              // a group that allows authValue.
                                              // If PCR does not belong to an
                                              // auth group, the value in this
                                              // parameter is invalid
                         );
BOOL PCRBelongsPolicyGroup(TPMI_DH_PCR handle,  // IN: handle of PCR
                           UINT32 *groupIndex   // OUT: group index if PCR
                                                // belongs a group that allows
                                               // policy. If PCR does not belong
                                               // to a policy group, the value
                                               // in this parameter is invalid
                           );
TPMI_YES_NO PCRCapGetAllocation(
    UINT32 count,                     // IN: count of return
    TPML_PCR_SELECTION *pcrSelection  // OUT: PCR allocation list
    );
void PCRChanged(TPM_HANDLE pcrHandle  // IN: the handle of the PCR that changed.
                );
void PCRComputeCurrentDigest(
    TPMI_ALG_HASH hashAlg,  // IN: hash algorithm to compute digest
    TPML_PCR_SELECTION *
        selection,        // IN/OUT: PCR selection (filtered on output)
    TPM2B_DIGEST *digest  // OUT: digest
    );
TPMI_ALG_HASH PCRGetAuthPolicy(TPMI_DH_PCR handle,   // IN: PCR handle
                               TPM2B_DIGEST *policy  // OUT: policy of PCR
                               );
TPMI_YES_NO PCRCapGetHandles(TPMI_DH_PCR handle,  // IN: start handle
                             UINT32 count,  // IN: count of returned handle
                             TPML_HANDLE *handleList  // OUT: list of handle
                             );
TPMI_YES_NO PCRCapGetProperties(
    TPM_PT_PCR property,              // IN: the starting PCR property
    UINT32 count,                     // IN: count of returned propertie
    TPML_TAGGED_PCR_PROPERTY *select  // OUT: PCR select
    );
void PCRGetAuthValue(TPMI_DH_PCR handle,  // IN: PCR handle
                     TPM2B_AUTH *auth     // OUT: authValue of PCR
                     );
void PCRExtend(TPMI_DH_PCR handle,  //   IN:    PCR handle to be extended
               TPMI_ALG_HASH hash,  //   IN:    hash algorithm of PCR
               UINT32 size,         //   IN:    size of data to be extended
               BYTE *data           //   IN:    data to be extended
               );
void PCRResetDynamics(void);
void PcrDrtm(
    const TPMI_DH_PCR pcrHandle,  // IN: the index of the PCR to be modified
    const TPMI_ALG_HASH hash,     // IN: the bank identifier
    const TPM2B_DIGEST *digest    // IN: the digest to modify the PCR
    );
BOOL PcrIsAllocated(UINT32 pcr,            // IN: The number of the PCR
                    TPMI_ALG_HASH hashAlg  // IN: The PCR algorithm
                    );
BOOL PCRIsExtendAllowed(TPMI_DH_PCR handle  // IN: PCR handle to be extended
                        );
BOOL PCRIsResetAllowed(TPMI_DH_PCR handle  // IN: PCR handle to be extended
                       );
BOOL PCRIsStateSaved(TPMI_DH_PCR handle  // IN: PCR handle to be extended
                     );
BOOL PCRPolicyIsAvailable(TPMI_DH_PCR handle  // IN: PCR handle
                          );
void PCRRead(
    TPML_PCR_SELECTION *
        selection,        // IN/OUT: PCR selection (filtered on output)
    TPML_DIGEST *digest,  // OUT: digest
    UINT32 *pcrCounter    // OUT: the current value of PCR generation number
    );
void PCRSetValue(TPM_HANDLE handle,  // IN: the handle of the PCR to set
                 INT8 initialValue   // IN: the value to set
                 );
void PCRSimStart(void);
void PCRStartup(STARTUP_TYPE type,  // IN: startup type
                BYTE locality       // IN: startup locality
                );
void PCRStateSave(TPM_SU type  // IN: startup type
                  );

#endif  // __TPM2_PCR_FP_H
