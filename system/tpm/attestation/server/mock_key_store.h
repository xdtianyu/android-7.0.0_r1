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

#ifndef ATTESTATION_SERVER_MOCK_KEY_STORE_H_
#define ATTESTATION_SERVER_MOCK_KEY_STORE_H_

#include "attestation/server/key_store.h"

#include <string>

#include <base/macros.h>
#include <gmock/gmock.h>

namespace attestation {

class MockKeyStore : public KeyStore {
 public:
  MockKeyStore();
  virtual ~MockKeyStore();

  MOCK_METHOD3(Read, bool(const std::string& username,
                          const std::string& name,
                          std::string* key_data));
  MOCK_METHOD3(Write, bool(const std::string& username,
                           const std::string& name,
                           const std::string& key_data));
  MOCK_METHOD2(Delete, bool(const std::string& username,
                            const std::string& name));
  MOCK_METHOD2(DeleteByPrefix, bool(const std::string& username,
                                    const std::string& key_prefix));
  MOCK_METHOD7(Register, bool(const std::string& username,
                              const std::string& label,
                              KeyType key_type,
                              KeyUsage key_usage,
                              const std::string& private_key_blob,
                              const std::string& public_key_der,
                              const std::string& certificate));
  MOCK_METHOD2(RegisterCertificate, bool(const std::string& username,
                                         const std::string& certificate));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockKeyStore);
};

}  // namespace attestation

#endif  // ATTESTATION_SERVER_MOCK_KEY_STORE_H_
