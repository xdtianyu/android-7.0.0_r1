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

#include "tpm_manager/server/openssl_crypto_util_impl.h"

#include <base/logging.h>
#include <base/stl_util.h>
#include <openssl/rand.h>

namespace tpm_manager {

bool OpensslCryptoUtilImpl::GetRandomBytes(size_t num_bytes,
                                           std::string* random_data) {
  random_data->resize(num_bytes);
  unsigned char* random_buffer =
      reinterpret_cast<unsigned char*>(string_as_array(random_data));
  if (RAND_bytes(random_buffer, num_bytes) != 1) {
    LOG(ERROR) << "Error getting random bytes using Openssl.";
    random_data->clear();
    return false;
  }
  return true;
}

}  // namespace tpm_manager
