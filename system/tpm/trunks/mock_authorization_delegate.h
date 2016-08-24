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

#ifndef TRUNKS_MOCK_AUTHORIZATION_DELEGATE_H_
#define TRUNKS_MOCK_AUTHORIZATION_DELEGATE_H_

#include <string>

#include <base/macros.h>
#include <gmock/gmock.h>

#include "trunks/authorization_delegate.h"

namespace trunks {

class MockAuthorizationDelegate : public AuthorizationDelegate {
 public:
  MockAuthorizationDelegate();
  ~MockAuthorizationDelegate() override;

  MOCK_METHOD4(GetCommandAuthorization, bool(const std::string&,
                                             bool,
                                             bool,
                                             std::string*));
  MOCK_METHOD2(CheckResponseAuthorization, bool(const std::string&,
                                                const std::string&));
  MOCK_METHOD1(EncryptCommandParameter, bool(std::string*));
  MOCK_METHOD1(DecryptResponseParameter, bool(std::string*));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockAuthorizationDelegate);
};

}  // namespace trunks

#endif  // TRUNKS_MOCK_AUTHORIZATION_DELEGATE_H_
