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

#ifndef TPM_MANAGER_SERVER_LOCAL_DATA_STORE_H_
#define TPM_MANAGER_SERVER_LOCAL_DATA_STORE_H_

#include "tpm_manager/common/local_data.pb.h"

namespace tpm_manager {

// LocalDataStore is an interface class that provides access to read and write
// local system data.
class LocalDataStore {
 public:
  LocalDataStore() = default;
  virtual ~LocalDataStore() = default;

  // Reads local |data| from persistent storage. If no local data exists, the
  // output is an empty protobuf and the method succeeds. Returns true on
  // success.
  virtual bool Read(LocalData* data) = 0;

  // Writes local |data| to persistent storage. Returns true on success.
  virtual bool Write(const LocalData& data) = 0;
};

}  // namespace tpm_manager

#endif  // TPM_MANAGER_SERVER_LOCAL_DATA_STORE_H_
