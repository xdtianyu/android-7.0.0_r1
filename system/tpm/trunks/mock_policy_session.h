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

#ifndef TRUNKS_MOCK_POLICY_SESSION_H_
#define TRUNKS_MOCK_POLICY_SESSION_H_

#include <string>
#include <vector>

#include <gmock/gmock.h>

#include "trunks/policy_session.h"

namespace trunks {

class MockPolicySession : public PolicySession {
 public:
  MockPolicySession();
  ~MockPolicySession() override;

  MOCK_METHOD0(GetDelegate, AuthorizationDelegate*());
  MOCK_METHOD3(StartBoundSession, TPM_RC(
      TPMI_DH_ENTITY bind_entity,
      const std::string& bind_authorization_value,
      bool enable_encryption));
  MOCK_METHOD1(StartUnboundSession, TPM_RC(bool enable_encryption));
  MOCK_METHOD1(GetDigest, TPM_RC(std::string*));
  MOCK_METHOD1(PolicyOR, TPM_RC(const std::vector<std::string>&));
  MOCK_METHOD2(PolicyPCR, TPM_RC(uint32_t, const std::string&));
  MOCK_METHOD1(PolicyCommandCode, TPM_RC(TPM_CC));
  MOCK_METHOD0(PolicyAuthValue, TPM_RC());
  MOCK_METHOD1(SetEntityAuthorizationValue, void(const std::string&));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockPolicySession);
};

}  // namespace trunks

#endif  // TRUNKS_MOCK_POLICY_SESSION_H_
