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

#ifndef TRUNKS_PASSWORD_AUTHORIZATION_DELEGATE_H_
#define TRUNKS_PASSWORD_AUTHORIZATION_DELEGATE_H_

#include <string>

#include <base/gtest_prod_util.h>

#include "trunks/authorization_delegate.h"
#include "trunks/tpm_generated.h"
#include "trunks/trunks_export.h"

namespace trunks {

// PasswdAuthorizationDelegate is an implementation of the AuthorizationDelegate
// interface. This delegate is used for password based authorization. Upon
// initialization of this delegate, we feed in the plaintext password. This
// password is then used to authorize the commands issued with this delegate.
// This delegate performs no parameter encryption.
class TRUNKS_EXPORT PasswordAuthorizationDelegate
    : public AuthorizationDelegate {
 public:
  explicit PasswordAuthorizationDelegate(const std::string& password);
  ~PasswordAuthorizationDelegate() override;
  // AuthorizationDelegate methods.
  bool GetCommandAuthorization(const std::string& command_hash,
                               bool is_command_parameter_encryption_possible,
                               bool is_response_parameter_encryption_possible,
                               std::string* authorization) override;
  bool CheckResponseAuthorization(const std::string& response_hash,
                                  const std::string& authorization) override;
  bool EncryptCommandParameter(std::string* parameter) override;
  bool DecryptResponseParameter(std::string* parameter) override;

 protected:
  FRIEND_TEST(PasswordAuthorizationDelegateTest, NullInitialization);

 private:
  TPM2B_AUTH password_;

  DISALLOW_COPY_AND_ASSIGN(PasswordAuthorizationDelegate);
};

}  // namespace trunks

#endif  // TRUNKS_PASSWORD_AUTHORIZATION_DELEGATE_H_
