/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __SOURCE_ENTITY_FP_H
#define __SOURCE_ENTITY_FP_H

TPMI_ALG_HASH EntityGetAuthPolicy(
    TPMI_DH_ENTITY handle,    // IN: handle of entity
    TPM2B_DIGEST *authPolicy  // OUT: authPolicy of the entity
    );
UINT16 EntityGetAuthValue(TPMI_DH_ENTITY handle,  // IN: handle of entity
                          AUTH_VALUE *auth  // OUT: authValue of the entity
                          );
TPMI_RH_HIERARCHY EntityGetHierarchy(
    TPMI_DH_ENTITY handle  // IN :handle of entity
    );
TPM_RC EntityGetLoadStatus(TPM_HANDLE *handle,  // IN/OUT: handle of the entity
                           TPM_CC commandCode   // IN: the commmandCode
                           );
UINT16 EntityGetName(TPMI_DH_ENTITY handle,  // IN: handle of entity
                     NAME *name              // OUT: name of entity
                     );

#endif  // __SOURCE_ENTITY_FP_H
