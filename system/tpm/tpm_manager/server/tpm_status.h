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

#ifndef TPM_MANAGER_SERVER_TPM_STATUS_H_
#define TPM_MANAGER_SERVER_TPM_STATUS_H_

namespace tpm_manager {

// TpmStatus is an interface class that reports status information for some kind
// of TPM device.
class TpmStatus {
 public:
  TpmStatus() = default;
  virtual ~TpmStatus() = default;

  // Returns true iff the TPM is enabled.
  virtual bool IsTpmEnabled() = 0;
  // Returns true iff the TPM has been owned.
  virtual bool IsTpmOwned() = 0;
  // Reports the current state of the TPM dictionary attack logic.
  virtual bool GetDictionaryAttackInfo(int* counter,
                                       int* threshold,
                                       bool* lockout,
                                       int* seconds_remaining) = 0;
};

}  // namespace tpm_manager

#endif  // TPM_MANAGER_SERVER_TPM_STATUS_H_
