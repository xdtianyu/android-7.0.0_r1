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

#include "attestation/server/mock_key_store.h"

using ::testing::_;
using ::testing::Return;

namespace attestation {

MockKeyStore::MockKeyStore() {
  ON_CALL(*this, Read(_, _, _)).WillByDefault(Return(true));
  ON_CALL(*this, Write(_, _, _)).WillByDefault(Return(true));
  ON_CALL(*this, Delete(_, _)).WillByDefault(Return(true));
  ON_CALL(*this, DeleteByPrefix(_, _)).WillByDefault(Return(true));
  ON_CALL(*this, Register(_, _, _, _, _, _, _)).WillByDefault(Return(true));
  ON_CALL(*this, RegisterCertificate(_, _)).WillByDefault(Return(true));
}

MockKeyStore::~MockKeyStore() {}

}  // namespace attestation
