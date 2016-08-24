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

#ifndef TRUNKS_TRUNKS_FACTORY_FOR_TEST_H_
#define TRUNKS_TRUNKS_FACTORY_FOR_TEST_H_

#include "trunks/trunks_factory.h"

#include <string>

#include <base/macros.h>
#include <base/memory/scoped_ptr.h>

#include "trunks/password_authorization_delegate.h"
#include "trunks/trunks_export.h"

namespace trunks {

class AuthorizationDelegate;
class MockBlobParser;
class MockHmacSession;
class MockPolicySession;
class MockSessionManager;
class MockTpm;
class MockTpmState;
class MockTpmUtility;
class HmacSession;
class PasswordAuthorizationDelegate;
class PolicySession;
class SessionManager;
class Tpm;
class TpmState;
class TpmUtility;

// A factory implementation for testing. Custom instances can be injected. If no
// instance has been injected, a default mock instance will be used. Objects for
// which ownership is passed to the caller are instantiated as forwarders which
// simply forward calls to the current instance set for the class.
//
// Example usage:
//   TrunksFactoryForTest factory;
//   MockTpmState mock_tpm_state;
//   factory.set_tpm_state(mock_tpm_state);
//   // Set expectations on mock_tpm_state...
class TRUNKS_EXPORT TrunksFactoryForTest : public TrunksFactory {
 public:
  TrunksFactoryForTest();
  ~TrunksFactoryForTest() override;

  // TrunksFactory methods.
  Tpm* GetTpm() const override;
  scoped_ptr<TpmState> GetTpmState() const override;
  scoped_ptr<TpmUtility> GetTpmUtility() const override;
  scoped_ptr<AuthorizationDelegate> GetPasswordAuthorization(
      const std::string& password) const override;
  scoped_ptr<SessionManager> GetSessionManager() const override;
  scoped_ptr<HmacSession> GetHmacSession() const override;
  scoped_ptr<PolicySession> GetPolicySession() const override;
  scoped_ptr<PolicySession> GetTrialSession() const override;
  scoped_ptr<BlobParser> GetBlobParser() const override;

  // Mutators to inject custom mocks.
  void set_tpm(Tpm* tpm) {
    tpm_ = tpm;
  }

  void set_tpm_state(TpmState* tpm_state) {
    tpm_state_ = tpm_state;
  }

  void set_tpm_utility(TpmUtility* tpm_utility) {
    tpm_utility_ = tpm_utility;
  }

  void set_password_authorization_delegate(AuthorizationDelegate* delegate) {
    password_authorization_delegate_ = delegate;
  }

  void set_session_manager(SessionManager* session_manager) {
    session_manager_ = session_manager;
  }

  void set_hmac_session(HmacSession* hmac_session) {
    hmac_session_ = hmac_session;
  }

  void set_policy_session(PolicySession* policy_session) {
    policy_session_ = policy_session;
  }

  void set_blob_parser(BlobParser* blob_parser) {
    blob_parser_ = blob_parser;
  }

 private:
  scoped_ptr<MockTpm> default_tpm_;
  Tpm* tpm_;
  scoped_ptr<MockTpmState> default_tpm_state_;
  TpmState* tpm_state_;
  scoped_ptr<MockTpmUtility> default_tpm_utility_;
  TpmUtility* tpm_utility_;
  scoped_ptr<PasswordAuthorizationDelegate> default_authorization_delegate_;
  AuthorizationDelegate* password_authorization_delegate_;
  scoped_ptr<MockSessionManager> default_session_manager_;
  SessionManager* session_manager_;
  scoped_ptr<MockHmacSession> default_hmac_session_;
  HmacSession* hmac_session_;
  scoped_ptr<MockPolicySession> default_policy_session_;
  PolicySession* policy_session_;
  scoped_ptr<MockBlobParser> default_blob_parser_;
  BlobParser* blob_parser_;

  DISALLOW_COPY_AND_ASSIGN(TrunksFactoryForTest);
};

}  // namespace trunks

#endif  // TRUNKS_TRUNKS_FACTORY_FOR_TEST_H_
