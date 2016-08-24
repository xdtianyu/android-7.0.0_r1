//
// Copyright (C) 2013 The Android Open Source Project
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

#ifndef SHILL_MOCK_PENDING_ACTIVATION_STORE_H_
#define SHILL_MOCK_PENDING_ACTIVATION_STORE_H_

#include <string>

#include <base/files/file_path.h>
#include <gmock/gmock.h>

#include "shill/pending_activation_store.h"

namespace shill {

class MockPendingActivationStore : public PendingActivationStore {
 public:
  MockPendingActivationStore();
  ~MockPendingActivationStore() override;

  MOCK_METHOD1(InitStorage, bool(const base::FilePath& storage_path));
  MOCK_CONST_METHOD2(GetActivationState,
                     State(IdentifierType type, const std::string& iccid));
  MOCK_METHOD3(SetActivationState,
               bool(IdentifierType type,
                    const std::string& iccid,
                    State state));
  MOCK_METHOD2(RemoveEntry,
               bool(IdentifierType type, const std::string& iccid));
};

}  // namespace shill

#endif  // SHILL_MOCK_PENDING_ACTIVATION_STORE_H_
