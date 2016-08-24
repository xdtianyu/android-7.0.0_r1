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

#ifndef TRUNKS_POLICY_SESSION_IMPL_H_
#define TRUNKS_POLICY_SESSION_IMPL_H_

#include "trunks/policy_session.h"

#include <string>
#include <vector>

#include "trunks/hmac_authorization_delegate.h"
#include "trunks/session_manager.h"
#include "trunks/trunks_factory.h"

namespace trunks {

// This class implements the PolicySession interface. It is used for
// keeping track of the HmacAuthorizationDelegate used for commands, and to
// provide authorization for commands that need it. It can also be used to
// create custom policies to restrict the usage of keys.
// TrunksFactoryImpl factory;
// PolicySessionImpl session(factory);
// session.StartBoundSession(bind_entity, bind_authorization, true);
// session.PolicyPCR(pcr_index, pcr_value);
// factory.GetTpm()->RSA_EncrpytSync(_,_,_,_, session.GetDelegate());
// NOTE: StartBoundSession/StartUnboundSession should not be called before
// TPM Ownership is taken. This is because starting a session uses the
// SaltingKey, which is only created after ownership is taken.
class TRUNKS_EXPORT PolicySessionImpl: public PolicySession {
 public:
  explicit PolicySessionImpl(const TrunksFactory& factory);
  // |session_type| specifies what type of session this is. It can only
  // be TPM_SE_TRIAL or TPM_SE_POLICY. If other values are used,
  // StartBoundSession will return SAPI_RC_INVALID_SESSIONS.
  PolicySessionImpl(const TrunksFactory& factory, TPM_SE session_type);
  ~PolicySessionImpl() override;

  // PolicySession methods
  AuthorizationDelegate* GetDelegate() override;
  TPM_RC StartBoundSession(TPMI_DH_ENTITY bind_entity,
                           const std::string& bind_authorization_value,
                           bool enable_encryption) override;
  TPM_RC StartUnboundSession(bool enable_encryption) override;
  TPM_RC GetDigest(std::string* digest) override;
  TPM_RC PolicyOR(const std::vector<std::string>& digests) override;
  TPM_RC PolicyPCR(uint32_t pcr_index, const std::string& pcr_value) override;
  TPM_RC PolicyCommandCode(TPM_CC command_code) override;
  TPM_RC PolicyAuthValue() override;
  void SetEntityAuthorizationValue(const std::string& value) override;

 private:
  // This factory is only set in the constructor and is used to instantiate
  // The TPM class to forward commands to the TPM chip.
  const TrunksFactory& factory_;
  // This field determines if this session is of type TPM_SE_TRIAL or
  // TPM_SE_POLICY.
  TPM_SE session_type_;
  // This delegate is what provides authorization to commands. It is what is
  // returned when the GetDelegate method is called.
  HmacAuthorizationDelegate hmac_delegate_;
  // This object is used to manage the TPM session associated with this
  // AuthorizationSession.
  scoped_ptr<SessionManager> session_manager_;

  friend class PolicySessionTest;
  DISALLOW_COPY_AND_ASSIGN(PolicySessionImpl);
};

}  // namespace trunks

#endif  // TRUNKS_POLICY_SESSION_IMPL_H_
