//
// Copyright (C) 2014 The Android Open Source Project
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

#ifndef TRUNKS_MOCK_TPM_STATE_H_
#define TRUNKS_MOCK_TPM_STATE_H_

#include <gmock/gmock.h>

#include "trunks/tpm_state.h"

namespace trunks {

class MockTpmState : public TpmState {
 public:
  MockTpmState();
  ~MockTpmState() override;

  MOCK_METHOD0(Initialize, TPM_RC());
  MOCK_METHOD0(IsOwnerPasswordSet, bool());
  MOCK_METHOD0(IsEndorsementPasswordSet, bool());
  MOCK_METHOD0(IsLockoutPasswordSet, bool());
  MOCK_METHOD0(IsOwned, bool());
  MOCK_METHOD0(IsInLockout, bool());
  MOCK_METHOD0(IsPlatformHierarchyEnabled, bool());
  MOCK_METHOD0(IsStorageHierarchyEnabled, bool());
  MOCK_METHOD0(IsEndorsementHierarchyEnabled, bool());
  MOCK_METHOD0(IsEnabled, bool());
  MOCK_METHOD0(WasShutdownOrderly, bool());
  MOCK_METHOD0(IsRSASupported, bool());
  MOCK_METHOD0(IsECCSupported, bool());
  MOCK_METHOD0(GetLockoutCounter, uint32_t());
  MOCK_METHOD0(GetLockoutThreshold, uint32_t());
  MOCK_METHOD0(GetLockoutInterval, uint32_t());
  MOCK_METHOD0(GetLockoutRecovery, uint32_t());
};

}  // namespace trunks

#endif  // TRUNKS_MOCK_TPM_STATE_H_
