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

#ifndef TPM_MANAGER_SERVER_TPM_STATUS_IMPL_H_
#define TPM_MANAGER_SERVER_TPM_STATUS_IMPL_H_

#include "tpm_manager/server/tpm_status.h"

#include <memory>
#include <string>

#include <base/macros.h>
#include <trousers/tss.h>
#include <trousers/trousers.h>  // NOLINT(build/include_alpha)

#include <tpm_manager/server/tpm_connection.h>

namespace tpm_manager {

class TpmStatusImpl : public TpmStatus {
 public:
  TpmStatusImpl() = default;
  ~TpmStatusImpl() override = default;

  // TpmState methods.
  bool IsTpmEnabled() override;
  bool IsTpmOwned() override;
  bool GetDictionaryAttackInfo(int* counter,
                               int* threshold,
                               bool* lockout,
                               int* seconds_remaining) override;

 private:
  // This method refreshes the |is_owned_| and |is_enabled_| status of the
  // Tpm. It can be called multiple times.
  void RefreshOwnedEnabledInfo();
  // This method wraps calls to Tspi_TPM_GetCapability. |data| is set to
  // the raw capability data. If the optional out argument |tpm_result| is
  // provided, it is set to the result of the |Tspi_TPM_GetCapability| call.
  bool GetCapability(uint32_t capability,
                     uint32_t sub_capability,
                     std::string* data,
                     TSS_RESULT* tpm_result);

  TpmConnection tpm_connection_;
  bool is_enabled_{false};
  bool is_owned_{false};
  bool is_enable_initialized_{false};

  DISALLOW_COPY_AND_ASSIGN(TpmStatusImpl);
};

}  // namespace tpm_manager

#endif  // TPM_MANAGER_SERVER_TPM_STATUS_IMPL_H_
