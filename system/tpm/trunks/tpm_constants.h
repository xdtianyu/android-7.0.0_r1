//
// Copyright (C) 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef TRUNKS_TPM_CONSTANTS_H_
#define TRUNKS_TPM_CONSTANTS_H_

#include "trunks/tpm_generated.h"

namespace trunks {

// TPM Object Attributes.
const TPMA_OBJECT kFixedTPM = 1U << 1;
const TPMA_OBJECT kFixedParent = 1U << 4;
const TPMA_OBJECT kSensitiveDataOrigin = 1U << 5;
const TPMA_OBJECT kUserWithAuth = 1U << 6;
const TPMA_OBJECT kAdminWithPolicy = 1U << 7;
const TPMA_OBJECT kNoDA = 1U << 10;
const TPMA_OBJECT kRestricted = 1U << 16;
const TPMA_OBJECT kDecrypt = 1U << 17;
const TPMA_OBJECT kSign = 1U << 18;

// TPM NV Index Attributes, defined in TPM Spec Part 2 section 13.2.
const TPMA_NV TPMA_NV_OWNERWRITE = 1U << 1;
const TPMA_NV TPMA_NV_WRITELOCKED = 1U << 11;
const TPMA_NV TPMA_NV_WRITEDEFINE = 1U << 13;
const TPMA_NV TPMA_NV_AUTHREAD = 1U << 18;
const TPMA_NV TPMA_NV_NO_DA = 1U << 25;
const TPMA_NV TPMA_NV_WRITTEN = 1U << 29;

}  // namespace trunks

#endif  // TRUNKS_TPM_CONSTANTS_H_
