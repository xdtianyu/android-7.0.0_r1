/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_CONTEXT_SPT_FP_H
#define __TPM2_CONTEXT_SPT_FP_H
void ComputeContextIntegrity(TPMS_CONTEXT *contextBlob,  // IN: context blob
                             TPM2B_DIGEST *integrity     // OUT: integrity
                             );
void ComputeContextProtectionKey(
    TPMS_CONTEXT *contextBlob,  // IN: context blob
    TPM2B_SYM_KEY *symKey,      // OUT: the symmetric key
    TPM2B_IV *iv                // OUT: the IV.
    );
void SequenceDataImportExport(
    OBJECT *object,        // IN: the object containing the sequence data
    OBJECT *exportObject,  // IN/OUT: the object structure that will get the
                           // exported hash state
    IMPORT_EXPORT direction);

#endif  // __TPM2_CONTEXT_SPT_FP_H
