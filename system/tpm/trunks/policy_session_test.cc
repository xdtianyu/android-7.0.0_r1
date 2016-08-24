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

#include "trunks/policy_session_impl.h"

#include <crypto/sha2.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "trunks/error_codes.h"
#include "trunks/mock_session_manager.h"
#include "trunks/mock_tpm.h"
#include "trunks/tpm_generated.h"
#include "trunks/trunks_factory_for_test.h"

using testing::_;
using testing::NiceMock;
using testing::Return;
using testing::SaveArg;
using testing::SetArgPointee;

namespace trunks {

class PolicySessionTest : public testing::Test {
 public:
  PolicySessionTest() {}
  ~PolicySessionTest() override {}

  void SetUp() override {
    factory_.set_session_manager(&mock_session_manager_);
    factory_.set_tpm(&mock_tpm_);
  }

  HmacAuthorizationDelegate* GetHmacDelegate(PolicySessionImpl* session) {
    return &(session->hmac_delegate_);
  }

 protected:
  TrunksFactoryForTest factory_;
  NiceMock<MockSessionManager> mock_session_manager_;
  NiceMock<MockTpm> mock_tpm_;
};

TEST_F(PolicySessionTest, GetDelegateUninitialized) {
  PolicySessionImpl session(factory_);
  EXPECT_CALL(mock_session_manager_, GetSessionHandle())
      .WillRepeatedly(Return(kUninitializedHandle));
  EXPECT_EQ(nullptr, session.GetDelegate());
}

TEST_F(PolicySessionTest, GetDelegateSuccess) {
  PolicySessionImpl session(factory_);
  EXPECT_EQ(GetHmacDelegate(&session), session.GetDelegate());
}

TEST_F(PolicySessionTest, StartBoundSessionSuccess) {
  PolicySessionImpl session(factory_);
  EXPECT_EQ(TPM_RC_SUCCESS,
            session.StartBoundSession(TPM_RH_FIRST, "auth", true));
}

TEST_F(PolicySessionTest, StartBoundSessionFailure) {
  PolicySessionImpl session(factory_);
  TPM_HANDLE handle = TPM_RH_FIRST;
  EXPECT_CALL(mock_session_manager_, StartSession(TPM_SE_POLICY, handle,
                                                  _, true, _))
      .WillRepeatedly(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, session.StartBoundSession(handle, "auth", true));
}

TEST_F(PolicySessionTest, StartBoundSessionBadType) {
  PolicySessionImpl session(factory_, TPM_SE_HMAC);
  EXPECT_EQ(SAPI_RC_INVALID_SESSIONS,
            session.StartBoundSession(TPM_RH_FIRST, "auth", true));
}

TEST_F(PolicySessionTest, StartUnboundSessionSuccess) {
  PolicySessionImpl session(factory_);
  EXPECT_EQ(TPM_RC_SUCCESS, session.StartUnboundSession(true));
}

TEST_F(PolicySessionTest, StartUnboundSessionFailure) {
  PolicySessionImpl session(factory_);
  EXPECT_CALL(mock_session_manager_, StartSession(TPM_SE_POLICY, TPM_RH_NULL,
                                                  _, true, _))
      .WillRepeatedly(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, session.StartUnboundSession(true));
}

TEST_F(PolicySessionTest, GetDigestSuccess) {
  PolicySessionImpl session(factory_);
  std::string digest;
  TPM2B_DIGEST policy_digest;
  policy_digest.size = SHA256_DIGEST_SIZE;
  EXPECT_CALL(mock_tpm_, PolicyGetDigestSync(_, _, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(policy_digest),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_SUCCESS, session.GetDigest(&digest));
  EXPECT_EQ(static_cast<size_t>(SHA256_DIGEST_SIZE), digest.size());
}

TEST_F(PolicySessionTest, GetDigestFailure) {
  PolicySessionImpl session(factory_);
  std::string digest;
  EXPECT_CALL(mock_tpm_, PolicyGetDigestSync(_, _, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, session.GetDigest(&digest));
}

TEST_F(PolicySessionTest, PolicyORSuccess) {
  PolicySessionImpl session(factory_);
  std::vector<std::string> digests;
  digests.push_back("digest1");
  digests.push_back("digest2");
  digests.push_back("digest3");
  TPML_DIGEST tpm_digests;
  EXPECT_CALL(mock_tpm_, PolicyORSync(_, _, _, _))
      .WillOnce(DoAll(SaveArg<2>(&tpm_digests),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_SUCCESS, session.PolicyOR(digests));
  EXPECT_EQ(tpm_digests.count, digests.size());
  EXPECT_EQ(StringFrom_TPM2B_DIGEST(tpm_digests.digests[0]), digests[0]);
  EXPECT_EQ(StringFrom_TPM2B_DIGEST(tpm_digests.digests[1]), digests[1]);
  EXPECT_EQ(StringFrom_TPM2B_DIGEST(tpm_digests.digests[2]), digests[2]);
}

TEST_F(PolicySessionTest, PolicyORBadParam) {
  PolicySessionImpl session(factory_);
  std::vector<std::string> digests;
  // We use 9 here because the maximum number of digests allowed by the TPM
  // is 8. Therefore having 9 digests here should cause the code to fail.
  digests.resize(9);
  EXPECT_EQ(SAPI_RC_BAD_PARAMETER, session.PolicyOR(digests));
}

TEST_F(PolicySessionTest, PolicyORFailure) {
  PolicySessionImpl session(factory_);
  std::vector<std::string> digests;
  EXPECT_CALL(mock_tpm_, PolicyORSync(_, _, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, session.PolicyOR(digests));
}

TEST_F(PolicySessionTest, PolicyPCRSuccess) {
  PolicySessionImpl session(factory_);
  std::string pcr_digest("digest");
  int pcr_index = 1;
  TPML_PCR_SELECTION pcr_select;
  TPM2B_DIGEST pcr_value;
  EXPECT_CALL(mock_tpm_, PolicyPCRSync(_, _, _, _, _))
      .WillOnce(DoAll(SaveArg<2>(&pcr_value),
                      SaveArg<3>(&pcr_select),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_SUCCESS, session.PolicyPCR(pcr_index, pcr_digest));
  uint8_t pcr_select_index = pcr_index / 8;
  uint8_t pcr_select_byte = 1 << (pcr_index % 8);
  EXPECT_EQ(pcr_select.count, 1u);
  EXPECT_EQ(pcr_select.pcr_selections[0].hash, TPM_ALG_SHA256);
  EXPECT_EQ(pcr_select.pcr_selections[0].sizeof_select, PCR_SELECT_MIN);
  EXPECT_EQ(pcr_select.pcr_selections[0].pcr_select[pcr_select_index],
            pcr_select_byte);
  EXPECT_EQ(StringFrom_TPM2B_DIGEST(pcr_value),
            crypto::SHA256HashString(pcr_digest));
}

TEST_F(PolicySessionTest, PolicyPCRFailure) {
  PolicySessionImpl session(factory_);
  EXPECT_CALL(mock_tpm_, PolicyPCRSync(_, _, _, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, session.PolicyPCR(1, "pcr_digest"));
}

TEST_F(PolicySessionTest, PolicyPCRTrialWithNoDigest) {
  PolicySessionImpl session(factory_, TPM_SE_TRIAL);
  EXPECT_EQ(SAPI_RC_BAD_PARAMETER, session.PolicyPCR(1, ""));
}

TEST_F(PolicySessionTest, PolicyCommandCodeSuccess) {
  PolicySessionImpl session(factory_);
  TPM_CC command_code = TPM_CC_FIRST;
  EXPECT_CALL(mock_tpm_, PolicyCommandCodeSync(_, _, command_code, _))
      .WillOnce(Return(TPM_RC_SUCCESS));
  EXPECT_EQ(TPM_RC_SUCCESS, session.PolicyCommandCode(TPM_CC_FIRST));
}

TEST_F(PolicySessionTest, PolicyCommandCodeFailure) {
  PolicySessionImpl session(factory_);
  EXPECT_CALL(mock_tpm_, PolicyCommandCodeSync(_, _, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, session.PolicyCommandCode(TPM_CC_FIRST));
}

TEST_F(PolicySessionTest, PolicyAuthValueSuccess) {
  PolicySessionImpl session(factory_);
  EXPECT_CALL(mock_tpm_, PolicyAuthValueSync(_, _, _))
      .WillOnce(Return(TPM_RC_SUCCESS));
  EXPECT_EQ(TPM_RC_SUCCESS, session.PolicyAuthValue());
}

TEST_F(PolicySessionTest, PolicyAuthValueFailure) {
  PolicySessionImpl session(factory_);
  EXPECT_CALL(mock_tpm_, PolicyAuthValueSync(_, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, session.PolicyAuthValue());
}

TEST_F(PolicySessionTest, EntityAuthorizationForwardingTest) {
  PolicySessionImpl session(factory_);
  std::string test_auth("test_auth");
  session.SetEntityAuthorizationValue(test_auth);
  HmacAuthorizationDelegate* hmac_delegate = GetHmacDelegate(&session);
  std::string entity_auth = hmac_delegate->entity_authorization_value();
  EXPECT_EQ(0, test_auth.compare(entity_auth));
}

}  // namespace trunks
