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

#ifndef TPM_MANAGER_SERVER_LOCAL_DATA_STORE_IMPL_H_
#define TPM_MANAGER_SERVER_LOCAL_DATA_STORE_IMPL_H_

#include "tpm_manager/server/local_data_store.h"

#include <string>

#include <base/macros.h>

namespace tpm_manager {

class LocalDataStoreImpl : public LocalDataStore {
 public:
  LocalDataStoreImpl() = default;
  ~LocalDataStoreImpl() override = default;

  // LocalDataStore methods.
  bool Read(LocalData* data) override;
  bool Write(const LocalData& data) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(LocalDataStoreImpl);
};


}  // namespace tpm_manager


#endif  // TPM_MANAGER_SERVER_LOCAL_DATA_STORE_IMPL_H_
