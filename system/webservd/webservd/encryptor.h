// Copyright 2015 The Android Open Source Project
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

#ifndef WEBSERVER_WEBSERVD_ENCRYPTOR_H_
#define WEBSERVER_WEBSERVD_ENCRYPTOR_H_

#include <memory>
#include <string>

namespace webservd {

// An abstract class to perform authenticated encryption.
class Encryptor {
 public:
  virtual ~Encryptor() = default;

  // Encrypts and authenticates the given |plaintext| and emits the
  // |ciphertext|. Returns true on success.
  virtual bool EncryptWithAuthentication(const std::string& plaintext,
                                         std::string* ciphertext) = 0;

  // Decrypts and authenticates the given |ciphertext| and emits the
  // |plaintext|. Returns true on success.
  virtual bool DecryptWithAuthentication(const std::string& ciphertext,
                                         std::string* plaintext) = 0;

  // A factory method to be exported by the default Encryptor implementation
  // for a given platform. Like a constructor, this method should not perform
  // any significant initialization work. The caller assumes ownership of the
  // pointer.
  static std::unique_ptr<Encryptor> CreateDefaultEncryptor();
};

}  // namespace webservd

#endif  // WEBSERVER_WEBSERVD_ENCRYPTOR_H_
