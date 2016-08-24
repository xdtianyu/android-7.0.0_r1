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

#include "trunks/mock_tpm_state.h"

#include <gmock/gmock.h>

using testing::Return;

namespace trunks {

MockTpmState::MockTpmState() {
  ON_CALL(*this, IsOwnerPasswordSet()).WillByDefault(Return(true));
  ON_CALL(*this, IsEndorsementPasswordSet()).WillByDefault(Return(true));
  ON_CALL(*this, IsLockoutPasswordSet()).WillByDefault(Return(true));
  ON_CALL(*this, IsOwned()).WillByDefault(Return(true));
  ON_CALL(*this, IsPlatformHierarchyEnabled()).WillByDefault(Return(true));
  ON_CALL(*this, IsStorageHierarchyEnabled()).WillByDefault(Return(true));
  ON_CALL(*this, IsEndorsementHierarchyEnabled()).WillByDefault(Return(true));
  ON_CALL(*this, IsEnabled()).WillByDefault(Return(true));
  ON_CALL(*this, WasShutdownOrderly()).WillByDefault(Return(true));
  ON_CALL(*this, IsRSASupported()).WillByDefault(Return(true));
  ON_CALL(*this, IsECCSupported()).WillByDefault(Return(true));
  ON_CALL(*this, GetLockoutCounter()).WillByDefault(Return(0));
  ON_CALL(*this, GetLockoutThreshold()).WillByDefault(Return(0));
  ON_CALL(*this, GetLockoutInterval()).WillByDefault(Return(0));
  ON_CALL(*this, GetLockoutRecovery()).WillByDefault(Return(0));
}

MockTpmState::~MockTpmState() {}

}  // namespace trunks
