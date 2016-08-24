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

#ifndef TPM_MANAGER_SERVER_OPENSSL_CRYPTO_UTIL_H_
#define TPM_MANAGER_SERVER_OPENSSL_CRYPTO_UTIL_H_

#include <string>

#include <base/compiler_specific.h>
#include <base/macros.h>

namespace tpm_manager {

// This class is used to provide a mockable interface for openssl calls.
class OpensslCryptoUtil {
 public:
  OpensslCryptoUtil() = default;
  virtual ~OpensslCryptoUtil() = default;

  // This method sets the out argument |random_data| to a string with at
  // least |num_bytes| of random data and returns true on success.
  virtual bool GetRandomBytes(size_t num_bytes,
                              std::string* random_data) WARN_UNUSED_RESULT = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(OpensslCryptoUtil);
};

}  // namespace tpm_manager

#endif  // TPM_MANAGER_SERVER_OPENSSL_CRYPTO_UTIL_H_
