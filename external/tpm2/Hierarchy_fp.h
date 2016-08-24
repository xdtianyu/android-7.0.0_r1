/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_HIERARCHY_FP_H
#define __TPM2_HIERARCHY_FP_H

TPM2B_SEED *HierarchyGetPrimarySeed(
    TPMI_RH_HIERARCHY hierarchy  // IN: hierarchy
    );
TPM2B_AUTH *HierarchyGetProof(
    TPMI_RH_HIERARCHY hierarchy  // IN: hierarchy constant
    );
BOOL HierarchyIsEnabled(TPMI_RH_HIERARCHY hierarchy  // IN: hierarchy
                        );
void HierarchyPreInstall_Init(void);
void HierarchyStartup(STARTUP_TYPE type  // IN: start up type
                      );

#endif  // __TPM2_HIERARCHY_FP_H
