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

#ifndef TPM_MANAGER_SERVER_TPM2_STATUS_IMPL_H_
#define TPM_MANAGER_SERVER_TPM2_STATUS_IMPL_H_

#include "tpm_manager/server/tpm_status.h"

#include <memory>

#include <base/macros.h>
#include <trunks/tpm_state.h>
#include <trunks/trunks_factory.h>

namespace tpm_manager {

class Tpm2StatusImpl : public TpmStatus {
 public:
  Tpm2StatusImpl();
  // Does not take ownership of |factory|.
  explicit Tpm2StatusImpl(trunks::TrunksFactory* factory);
  ~Tpm2StatusImpl() override = default;

  // TpmState methods.
  bool IsTpmEnabled() override;
  bool IsTpmOwned() override;
  bool GetDictionaryAttackInfo(int* counter,
                               int* threshold,
                               bool* lockout,
                               int* seconds_remaining) override;

 private:
  // Refreshes the Tpm state information. Can be called as many times as needed
  // to refresh the cached information in this class. Return true if the
  // refresh operation succeeded.
  bool Refresh();

  bool initialized_{false};
  bool is_owned_{false};
  std::unique_ptr<trunks::TrunksFactory> default_trunks_factory_;
  trunks::TrunksFactory* trunks_factory_;
  scoped_ptr<trunks::TpmState> trunks_tpm_state_;

  DISALLOW_COPY_AND_ASSIGN(Tpm2StatusImpl);
};

}  // namespace tpm_manager

#endif  // TPM_MANAGER_SERVER_TPM2_STATUS_IMPL_H_
