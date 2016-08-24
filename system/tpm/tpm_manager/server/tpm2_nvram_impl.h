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

#ifndef TPM_MANAGER_SERVER_TPM2_NVRAM_IMPL_H_
#define TPM_MANAGER_SERVER_TPM2_NVRAM_IMPL_H_

#include "tpm_manager/server/tpm_nvram.h"

#include <memory>
#include <string>

#include <base/macros.h>
#include <base/memory/scoped_ptr.h>
#include <trunks/trunks_factory.h>

#include "tpm_manager/server/local_data_store.h"

namespace tpm_manager {

class Tpm2NvramImpl : public TpmNvram {
 public:
  // Does not take ownership of |local_data_store|.
  explicit Tpm2NvramImpl(LocalDataStore* local_data_store);
  // Does not take ownership of |local_data_store|, but takes ownership of
  // |factory|.
  Tpm2NvramImpl(std::unique_ptr<trunks::TrunksFactory> factory,
                LocalDataStore* local_data_store);
  ~Tpm2NvramImpl() override = default;

  // TpmNvram methods.
  bool DefineNvram(uint32_t index, size_t length) override;
  bool DestroyNvram(uint32_t index) override;
  bool WriteNvram(uint32_t index, const std::string& data) override;
  bool ReadNvram(uint32_t index, std::string* data) override;
  bool IsNvramDefined(uint32_t index, bool* defined) override;
  bool IsNvramLocked(uint32_t index, bool* locked) override;
  bool GetNvramSize(uint32_t index, size_t* size) override;

 private:
  // Initializes the connection to the Tpm2.0 and starts an authorization
  // session.
  // Note: there are no guarantees about the authorization value loaded into
  // |trunks_session_| at the end of this method.
  bool Initialize();

  // This method initializes and ensures that a valid owner password is
  // available. When this method returns, |owner_password_| will be loaded
  // into |trunks_session_|.
  bool InitializeWithOwnerPassword();

  std::unique_ptr<trunks::TrunksFactory> trunks_factory_;
  LocalDataStore* local_data_store_;
  bool initialized_;
  std::string owner_password_;
  scoped_ptr<trunks::HmacSession> trunks_session_;
  scoped_ptr<trunks::TpmUtility> trunks_utility_;

  friend class Tpm2NvramTest;
  DISALLOW_COPY_AND_ASSIGN(Tpm2NvramImpl);
};

}  // namespace tpm_manager

#endif  // TPM_MANAGER_SERVER_TPM2_NVRAM_IMPL_H_
