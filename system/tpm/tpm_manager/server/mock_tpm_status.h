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

#ifndef TPM_MANAGER_SERVER_MOCK_TPM_STATUS_H_
#define TPM_MANAGER_SERVER_MOCK_TPM_STATUS_H_

#include "tpm_manager/server/tpm_status.h"

#include <gmock/gmock.h>

namespace tpm_manager {

class MockTpmStatus : public TpmStatus {
 public:
  MockTpmStatus();
  ~MockTpmStatus() override;

  MOCK_METHOD0(IsTpmEnabled, bool());
  MOCK_METHOD0(IsTpmOwned, bool());
  MOCK_METHOD4(GetDictionaryAttackInfo,
               bool(int* counter,
                    int* threshold,
                    bool* lockout,
                    int* seconds_remaining));
};

}  // namespace tpm_manager

#endif  // TPM_MANAGER_SERVER_MOCK_TPM_STATUS_H_
