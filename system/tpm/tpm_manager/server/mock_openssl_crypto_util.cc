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

#include "tpm_manager/server/mock_openssl_crypto_util.h"

using testing::_;
using testing::Invoke;

namespace tpm_manager {

MockOpensslCryptoUtil::MockOpensslCryptoUtil() {
  ON_CALL(*this, GetRandomBytes(_, _))
      .WillByDefault(Invoke(this, &MockOpensslCryptoUtil::FakeGetRandomBytes));
}

MockOpensslCryptoUtil::~MockOpensslCryptoUtil() {}

bool MockOpensslCryptoUtil::FakeGetRandomBytes(size_t num_bytes,
                                               std::string* random_data) {
  random_data->assign(num_bytes, 'a');
  return true;
}

}  // namespace tpm_manager

