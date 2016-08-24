// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/privet/security_manager.h"

#include <algorithm>
#include <cctype>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <base/bind.h>
#include <base/logging.h>
#include <base/rand_util.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/string_util.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <weave/provider/test/fake_task_runner.h>
#include <weave/provider/test/mock_config_store.h>

#include "src/config.h"
#include "src/data_encoding.h"
#include "src/privet/auth_manager.h"
#include "src/privet/mock_delegates.h"
#include "src/privet/openssl_utils.h"
#include "src/test/mock_clock.h"
#include "third_party/chromium/crypto/p224_spake.h"

using testing::_;
using testing::Eq;
using testing::Return;

namespace weave {
namespace privet {

namespace {

bool IsBase64Char(char c) {
  return isalnum(c) || (c == '+') || (c == '/') || (c == '=');
}

bool IsBase64(const std::string& text) {
  return !text.empty() &&
         !std::any_of(text.begin(), text.end(),
                      std::not1(std::ref(IsBase64Char)));
}

class MockPairingCallbacks {
 public:
  MOCK_METHOD3(OnPairingStart,
               void(const std::string& session_id,
                    PairingType pairing_type,
                    const std::vector<uint8_t>& code));
  MOCK_METHOD1(OnPairingEnd, void(const std::string& session_id));
};

}  // namespace

class SecurityManagerConfigStore : public provider::test::MockConfigStore {
 public:
  SecurityManagerConfigStore() {
    EXPECT_CALL(*this, LoadDefaults(_))
        .WillRepeatedly(testing::Invoke([](Settings* settings) {
          settings->embedded_code = "1234";
          settings->pairing_modes = {PairingType::kEmbeddedCode};
          settings->client_id = "TEST_CLIENT_ID";
          settings->client_secret = "TEST_CLIENT_SECRET";
          settings->api_key = "TEST_API_KEY";
          settings->oem_name = "TEST_OEM";
          settings->model_name = "TEST_MODEL";
          settings->model_id = "ABCDE";
          settings->name = "TEST_NAME";
          return true;
        }));
  }
};

class SecurityManagerTest : public testing::Test {
 protected:
  void SetUp() override {
    EXPECT_CALL(clock_, Now())
        .WillRepeatedly(Return(base::Time::FromTimeT(1410000000)));
  }

  void PairAndAuthenticate(std::string* fingerprint, std::string* signature) {
    std::string session_id;
    std::string device_commitment_base64;

    EXPECT_TRUE(security_.StartPairing(PairingType::kEmbeddedCode,
                                       CryptoType::kSpake_p224, &session_id,
                                       &device_commitment_base64, nullptr));
    EXPECT_FALSE(session_id.empty());
    EXPECT_FALSE(device_commitment_base64.empty());

    crypto::P224EncryptedKeyExchange spake{
        crypto::P224EncryptedKeyExchange::kPeerTypeClient, "1234"};

    std::string client_commitment_base64{Base64Encode(spake.GetNextMessage())};

    EXPECT_TRUE(security_.ConfirmPairing(session_id, client_commitment_base64,
                                         fingerprint, signature, nullptr));
    EXPECT_TRUE(IsBase64(*fingerprint));
    EXPECT_TRUE(IsBase64(*signature));

    std::vector<uint8_t> device_commitment;
    ASSERT_TRUE(Base64Decode(device_commitment_base64, &device_commitment));
    spake.ProcessMessage(
        std::string(device_commitment.begin(), device_commitment.end()));

    const std::string& key = spake.GetUnverifiedKey();
    std::vector<uint8_t> auth_code{
        HmacSha256(std::vector<uint8_t>{key.begin(), key.end()},
                   std::vector<uint8_t>{session_id.begin(), session_id.end()})};

    std::string auth_code_base64{Base64Encode(auth_code)};

    std::string token;
    AuthScope scope;
    base::TimeDelta ttl;
    EXPECT_TRUE(security_.CreateAccessToken(AuthType::kPairing,
                                            auth_code_base64, AuthScope::kOwner,
                                            &token, &scope, &ttl, nullptr));
    EXPECT_EQ(AuthScope::kOwner, scope);
    EXPECT_EQ(base::TimeDelta::FromHours(1), ttl);

    UserInfo info;
    EXPECT_TRUE(security_.ParseAccessToken(token, &info, nullptr));
    EXPECT_EQ(AuthScope::kOwner, info.scope());
  }

  const base::Time time_ = base::Time::FromTimeT(1410000000);
  provider::test::FakeTaskRunner task_runner_;
  test::MockClock clock_;
  SecurityManagerConfigStore config_store_;
  Config config_{&config_store_};
  AuthManager auth_manager_{
      {22, 47, 23, 77, 42, 98, 96, 25, 83, 16, 9, 14, 91, 44, 15, 75, 60, 62,
       10, 18, 82, 35, 88, 100, 30, 45, 7, 46, 67, 84, 58, 85},
      {
          59, 47, 77, 247, 129, 187, 188, 158, 172, 105, 246, 93, 102, 83, 8,
          138, 176, 141, 37, 63, 223, 40, 153, 121, 134, 23, 120, 106, 24, 205,
          7, 135,
      },
      {22, 47, 23, 77, 42, 98, 96, 25, 83, 16, 9, 14, 91, 44, 15, 75, 60, 62,
       10, 18, 82, 35, 88, 100, 30, 45, 7, 46, 67, 84, 58, 85},
      &clock_};

  SecurityManager security_{&config_, &auth_manager_, &task_runner_};
};

TEST_F(SecurityManagerTest, AccessToken) {
  AuthScope scopes[] = {
      AuthScope::kViewer, AuthScope::kUser, AuthScope::kManager,
      AuthScope::kOwner,
  };
  for (size_t i = 1; i < 100; ++i) {
    const AuthScope requested_scope = scopes[i % arraysize(scopes)];
    std::string token;
    AuthScope scope;
    base::TimeDelta ttl;
    EXPECT_TRUE(security_.CreateAccessToken(AuthType::kAnonymous, "",
                                            requested_scope, &token, &scope,
                                            &ttl, nullptr));
    EXPECT_EQ(requested_scope, scope);
    EXPECT_EQ(base::TimeDelta::FromHours(1), ttl);

    UserInfo info;
    EXPECT_TRUE(security_.ParseAccessToken(token, &info, nullptr));
    EXPECT_EQ(requested_scope, info.scope());
    EXPECT_EQ(TestUserId{std::to_string(i)}, info.id());
  }
}

TEST_F(SecurityManagerTest, PairingNoSession) {
  std::string fingerprint;
  std::string signature;
  ErrorPtr error;
  ASSERT_FALSE(
      security_.ConfirmPairing("123", "345", &fingerprint, &signature, &error));
  EXPECT_EQ("unknownSession", error->GetCode());
}

TEST_F(SecurityManagerTest, Pairing) {
  std::vector<std::pair<std::string, std::string> > fingerprints(2);
  for (auto& it : fingerprints) {
    PairAndAuthenticate(&it.first, &it.second);
  }

  // Same certificate.
  EXPECT_EQ(fingerprints.front().first, fingerprints.back().first);

  // Signed with different secret.
  EXPECT_NE(fingerprints.front().second, fingerprints.back().second);
}

TEST_F(SecurityManagerTest, NotifiesListenersOfSessionStartAndEnd) {
  testing::StrictMock<MockPairingCallbacks> callbacks;
  security_.RegisterPairingListeners(
      base::Bind(&MockPairingCallbacks::OnPairingStart,
                 base::Unretained(&callbacks)),
      base::Bind(&MockPairingCallbacks::OnPairingEnd,
                 base::Unretained(&callbacks)));
  for (auto commitment_suffix :
       std::vector<std::string>{"", "invalid_commitment"}) {
    // StartPairing should notify us that a new session has begun.
    std::string session_id;
    std::string device_commitment;
    EXPECT_CALL(callbacks, OnPairingStart(_, PairingType::kEmbeddedCode, _));
    EXPECT_TRUE(security_.StartPairing(PairingType::kEmbeddedCode,
                                       CryptoType::kSpake_p224, &session_id,
                                       &device_commitment, nullptr));
    EXPECT_FALSE(session_id.empty());
    EXPECT_FALSE(device_commitment.empty());
    testing::Mock::VerifyAndClearExpectations(&callbacks);

    // ConfirmPairing should notify us that the session has ended.
    EXPECT_CALL(callbacks, OnPairingEnd(Eq(session_id)));
    crypto::P224EncryptedKeyExchange spake{
        crypto::P224EncryptedKeyExchange::kPeerTypeServer, "1234"};
    std::string client_commitment = Base64Encode(spake.GetNextMessage());
    std::string fingerprint, signature;
    // Regardless of whether the commitment is valid or not, we should get a
    // callback indicating that the pairing session is gone.
    security_.ConfirmPairing(session_id, client_commitment + commitment_suffix,
                             &fingerprint, &signature, nullptr);
    testing::Mock::VerifyAndClearExpectations(&callbacks);
  }
}

TEST_F(SecurityManagerTest, CancelPairing) {
  testing::StrictMock<MockPairingCallbacks> callbacks;
  security_.RegisterPairingListeners(
      base::Bind(&MockPairingCallbacks::OnPairingStart,
                 base::Unretained(&callbacks)),
      base::Bind(&MockPairingCallbacks::OnPairingEnd,
                 base::Unretained(&callbacks)));
  std::string session_id;
  std::string device_commitment;
  EXPECT_CALL(callbacks, OnPairingStart(_, PairingType::kEmbeddedCode, _));
  EXPECT_TRUE(security_.StartPairing(PairingType::kEmbeddedCode,
                                     CryptoType::kSpake_p224, &session_id,
                                     &device_commitment, nullptr));
  EXPECT_CALL(callbacks, OnPairingEnd(Eq(session_id)));
  EXPECT_TRUE(security_.CancelPairing(session_id, nullptr));
}

TEST_F(SecurityManagerTest, ThrottlePairing) {
  auto pair = [this]() {
    std::string session_id;
    std::string device_commitment;
    ErrorPtr error;
    bool result = security_.StartPairing(PairingType::kEmbeddedCode,
                                         CryptoType::kSpake_p224, &session_id,
                                         &device_commitment, &error);
    EXPECT_TRUE(result || error->GetCode() == "deviceBusy");
    return result;
  };

  EXPECT_TRUE(pair());
  EXPECT_TRUE(pair());
  EXPECT_TRUE(pair());
  EXPECT_FALSE(pair());
  EXPECT_GT(security_.block_pairing_until_, clock_.Now());
  EXPECT_LE(security_.block_pairing_until_,
            clock_.Now() + base::TimeDelta::FromMinutes(15));

  // Wait timeout.
  security_.block_pairing_until_ =
      clock_.Now() - base::TimeDelta::FromMinutes(1);

  // Allow exactly one attempt.
  EXPECT_TRUE(pair());
  EXPECT_FALSE(pair());

  // Wait timeout.
  security_.block_pairing_until_ =
      clock_.Now() - base::TimeDelta::FromMinutes(1);

  // Completely unblock by successfully pairing.
  std::string fingerprint;
  std::string signature;
  PairAndAuthenticate(&fingerprint, &signature);

  // Now we have 3 attempts again.
  EXPECT_TRUE(pair());
  EXPECT_TRUE(pair());
  EXPECT_TRUE(pair());
  EXPECT_FALSE(pair());
}

TEST_F(SecurityManagerTest, DontBlockForCanceledSessions) {
  for (int i = 0; i < 20; ++i) {
    std::string session_id;
    std::string device_commitment;
    EXPECT_TRUE(security_.StartPairing(PairingType::kEmbeddedCode,
                                       CryptoType::kSpake_p224, &session_id,
                                       &device_commitment, nullptr));
    EXPECT_TRUE(security_.CancelPairing(session_id, nullptr));
  }
}

}  // namespace privet
}  // namespace weave
