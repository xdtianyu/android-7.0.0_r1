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

#include "attestation/common/mock_crypto_utility.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::WithArgs;

namespace {

bool FakeRandom(size_t num_bytes, std::string* output) {
  *output = std::string(num_bytes, 'A');
  return true;
}

bool CopyString(const std::string& s1, std::string* s2) {
  *s2 = s1;
  return true;
}

}  // namespace

namespace attestation {

MockCryptoUtility::MockCryptoUtility() {
  ON_CALL(*this, GetRandom(_, _)).WillByDefault(Invoke(FakeRandom));
  ON_CALL(*this, CreateSealedKey(_, _)).WillByDefault(Return(true));
  ON_CALL(*this, UnsealKey(_, _, _)).WillByDefault(Return(true));
  ON_CALL(*this, EncryptData(_, _, _, _))
      .WillByDefault(WithArgs<0, 3>(Invoke(CopyString)));
  ON_CALL(*this, DecryptData(_, _, _))
      .WillByDefault(WithArgs<0, 2>(Invoke(CopyString)));
  ON_CALL(*this, GetRSASubjectPublicKeyInfo(_, _))
      .WillByDefault(Invoke(CopyString));
}

MockCryptoUtility::~MockCryptoUtility() {}

}  // namespace attestation
