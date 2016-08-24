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

#ifndef TPM_MANAGER_SERVER_TPM_NVRAM_IMPL_H_
#define TPM_MANAGER_SERVER_TPM_NVRAM_IMPL_H_

#include "tpm_manager/server/tpm_nvram.h"

#include <stdint.h>

#include <string>

#include <base/macros.h>
#include <trousers/scoped_tss_type.h>
#include <trousers/tss.h>

#include "tpm_manager/server/tpm_connection.h"

namespace tpm_manager {

class LocalDataStore;

class TpmNvramImpl : public TpmNvram {
 public:
  TpmNvramImpl(LocalDataStore* local_data_store);
  ~TpmNvramImpl() override = default;

  // TpmNvram methods.
  bool DefineNvram(uint32_t index, size_t length) override;
  bool DestroyNvram(uint32_t index) override;
  bool WriteNvram(uint32_t index, const std::string& data) override;
  bool ReadNvram(uint32_t index, std::string* data) override;
  bool IsNvramDefined(uint32_t index, bool* defined) override;
  bool IsNvramLocked(uint32_t index, bool* locked) override;
  bool GetNvramSize(uint32_t index, size_t* size) override;

 private:
  // This method creates and initializes the nvram object associated with
  // |handle| at |index|. Returns true on success, else false.
  bool InitializeNvramHandle(trousers::ScopedTssNvStore* nv_handle,
                             uint32_t index);

  // This method injects a tpm policy with the owner password. Returns true
  // on success.
  bool SetOwnerPolicy(trousers::ScopedTssNvStore* nv_handle);

  // This method sets up the composite pcr provided by |pcr_handle| with the
  // value of PCR0 at locality 1. Returns true on success.
  bool SetCompositePcr0(trousers::ScopedTssPcrs* pcr_handle);

  // This method gets the owner password stored on disk and returns it via the
  // out argument |owner_password|. Returns true if we were able to read a
  // non empty owner_password off disk, else false.
  bool GetOwnerPassword(std::string* owner_password);

  LocalDataStore* local_data_store_;
  TpmConnection tpm_connection_;

  DISALLOW_COPY_AND_ASSIGN(TpmNvramImpl);
};

}  // namespace tpm_manager

#endif  // TPM_MANAGER_SERVER_TPM_NVRAM_IMPL_H_
