//
// Copyright (C) 2014 The Android Open Source Project
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

#ifndef TRUNKS_AUTHORIZATION_DELEGATE_H_
#define TRUNKS_AUTHORIZATION_DELEGATE_H_

#include <string>

#include <base/macros.h>

namespace trunks {

// AuthorizationDelegate is an interface passed to TPM commands. The delegate
// takes care of providing the authorization data for commands and verifying
// authorization data for responses. It also handles parameter encryption for
// commands and parameter decryption for responses.
class AuthorizationDelegate {
 public:
  AuthorizationDelegate() {}
  virtual ~AuthorizationDelegate() {}

  // Provides authorization data for a command which has a cpHash value of
  // |command_hash|. The availability of encryption for the command is indicated
  // by |is_*_parameter_encryption_possible|. On success, |authorization| is
  // populated with the exact octets for the Authorization Area of the command.
  // Returns true on success.
  virtual bool GetCommandAuthorization(
      const std::string& command_hash,
      bool is_command_parameter_encryption_possible,
      bool is_response_parameter_encryption_possible,
      std::string* authorization) = 0;

  // Checks authorization data for a response which has a rpHash value of
  // |response_hash|. The exact octets from the Authorization Area of the
  // response are given in |authorization|. Returns true iff the authorization
  // is valid.
  virtual bool CheckResponseAuthorization(const std::string& response_hash,
                                          const std::string& authorization) = 0;

  // Encrypts |parameter| if encryption is enabled. Returns true on success.
  virtual bool EncryptCommandParameter(std::string* parameter) = 0;

  // Decrypts |parameter| if encryption is enabled. Returns true on success.
  virtual bool DecryptResponseParameter(std::string* parameter) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(AuthorizationDelegate);
};

}  // namespace trunks

#endif  // TRUNKS_AUTHORIZATION_DELEGATE_H_
