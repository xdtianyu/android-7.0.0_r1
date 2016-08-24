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

#ifndef TPM_MANAGER_SERVER_OPENSSL_CRYPTO_UTIL_IMPL_H_
#define TPM_MANAGER_SERVER_OPENSSL_CRYPTO_UTIL_IMPL_H_

#include <string>

#include <base/compiler_specific.h>
#include <base/macros.h>

#include "tpm_manager/server/openssl_crypto_util.h"

namespace tpm_manager {

// OpensslCryptoUtilImpl is the default implementation of the
// OpensslCryptoUtil interface.
// Example usage:
// OpensslCryptoUtilImpl util;
// std::string random_bytes;
// bool result = util.GetRandomBytes(5, &random_bytes);
class OpensslCryptoUtilImpl : public OpensslCryptoUtil {
 public:
  OpensslCryptoUtilImpl() = default;
  ~OpensslCryptoUtilImpl() override = default;

  // OpensslCryptoUtil methods.
  bool GetRandomBytes(size_t num_bytes,
                      std::string* random_data) override WARN_UNUSED_RESULT;

 private:
  DISALLOW_COPY_AND_ASSIGN(OpensslCryptoUtilImpl);
};

}  // namespace tpm_manager

#endif  // TPM_MANAGER_SERVER_OPENSSL_CRYPTO_UTIL_IMPL_H_
