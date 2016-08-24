/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_PROPERTYCAP_FP_H
#define __TPM2_PROPERTYCAP_FP_H

TPMI_YES_NO TPMCapGetProperties(
    TPM_PT property,  // IN: the starting TPM property
    UINT32 count,     // IN: maximum number of returned propertie
    TPML_TAGGED_TPM_PROPERTY *propertyList  // OUT: property list
    );

#endif  // __TPM2_PROPERTYCAP_FP_H
