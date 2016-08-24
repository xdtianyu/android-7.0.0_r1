// Copyright 2012 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Tests for PairingSession.

#include <gtest/gtest.h>
#include <polo/pairing/clientpairingsession.h>
#include "polo/pairing/mocks.h"
#include "polo/wire/mocks.h"

using ::testing::Const;
using ::testing::InSequence;
using ::testing::Mock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::StrictMock;
using ::testing::_;

namespace polo {
namespace pairing {

class TestPairingSession : public PairingSession {
 public:
  TestPairingSession(wire::PoloWireAdapter* wire,
                     PairingContext* context,
                     PoloChallengeResponse* challenge)
      : PairingSession(wire, context, challenge) {
  }

  void TestDoPairingPhase() {
    DoPairingPhase();
  }

  bool TestSetConfiguration(const message::ConfigurationMessage& message) {
    return SetConfiguration(message);
  }

  const message::ConfigurationMessage* GetConfiguration() {
    return configuration();
  }

  const message::OptionsMessage& GetLocalOptions() {
    return local_options();
  }

  void TestOnSecretMessage(const message::SecretMessage& message) {
    OnSecretMessage(message);
  }

  void TestOnSecretAckmessage(const message::SecretAckMessage& message) {
    OnSecretAckMessage(message);
  }

  MOCK_METHOD0(DoInitializationPhase, void());
  MOCK_METHOD0(DoConfigurationPhase, void());
  MOCK_METHOD1(OnConfigurationMessage,
               void(const pairing::message::ConfigurationMessage& message));
  MOCK_METHOD1(OnConfigurationAckMessage,
               void(const pairing::message::ConfigurationAckMessage& message));
  MOCK_METHOD1(OnOptionsMessage,
               void(const pairing::message::OptionsMessage& message));
  MOCK_METHOD1(OnPairingRequestMessage,
               void(const pairing::message::PairingRequestMessage& message));
  MOCK_METHOD1(OnPairingRequestAckMessage,
               void(const pairing::message::PairingRequestAckMessage& message));
};

class PairingSessionTest : public ::testing::Test {
 protected:
  PairingSessionTest()
      : interface_(),
        wire_(&interface_),
        challenge_(),
        context_(NULL, NULL, false),
        session_(&wire_, &context_, &challenge_) {
  }

  virtual void SetUp() {
  }

  virtual void TearDown() {
  }

  void InitSession() {
    InSequence sequence;

    EXPECT_CALL(listener_, OnSessionCreated());
    EXPECT_CALL(session_, DoInitializationPhase());

    session_.DoPair(&listener_);
  }

  StrictMock<wire::MockWireInterface> interface_;
  StrictMock<wire::MockWireAdapter> wire_;
  StrictMock<MockChallengeResponse> challenge_;
  PairingContext context_;
  StrictMock<MockPairingListener> listener_;
  StrictMock<TestPairingSession> session_;
};

TEST_F(PairingSessionTest, DoPair) {
  // Test the base SetUp case which initializes the pairing session.
  InitSession();
}

TEST_F(PairingSessionTest, SetConfiguration) {
  encoding::EncodingOption encoding(encoding::EncodingOption::kHexadecimal, 8);
  message::ConfigurationMessage configuration(encoding,
      message::OptionsMessage::kInputDevice);
  session_.TestSetConfiguration(configuration);

  ASSERT_TRUE(session_.GetConfiguration());
  ASSERT_EQ(encoding::EncodingOption::kHexadecimal,
            session_.GetConfiguration()->encoding().encoding_type());
  ASSERT_EQ(8, session_.GetConfiguration()->encoding().symbol_length());
  ASSERT_EQ(message::OptionsMessage::kInputDevice,
            session_.GetConfiguration()->client_role());

  ASSERT_TRUE(session_.encoder());
  ASSERT_EQ(2, session_.encoder()->symbols_per_byte());
}

TEST_F(PairingSessionTest, DoPairingPhaseInputDevice) {
  InitSession();
  InSequence sequence;

  encoding::EncodingOption encoding(encoding::EncodingOption::kHexadecimal, 8);
  message::ConfigurationMessage configuration(encoding,
      message::OptionsMessage::kInputDevice);
  session_.TestSetConfiguration(configuration);

  EXPECT_CALL(listener_, OnPerformInputDeviceRole());

  session_.TestDoPairingPhase();
}

TEST_F(PairingSessionTest, DoPairingPhaseDisplayDevice) {
  InitSession();
  InSequence sequence;

  encoding::EncodingOption encoding(encoding::EncodingOption::kHexadecimal, 8);
  message::ConfigurationMessage configuration(encoding,
      message::OptionsMessage::kDisplayDevice);
  session_.TestSetConfiguration(configuration);

  EXPECT_CALL(challenge_, GetGamma(_)).WillOnce(Return(new Gamma(10, 0x5)));
  EXPECT_CALL(listener_, OnPerformOutputDeviceRole(Gamma(10, 0x5)));
  EXPECT_CALL(wire_, GetNextMessage());

  session_.TestDoPairingPhase();
}

TEST_F(PairingSessionTest, AddInputEncoding) {
  encoding::EncodingOption encoding(encoding::EncodingOption::kHexadecimal, 8);
  session_.AddInputEncoding(encoding);
  ASSERT_TRUE(session_.GetLocalOptions().SupportsInputEncoding(encoding));
}

TEST_F(PairingSessionTest, AddInputEncodingInvalidEncoding) {
  encoding::EncodingOption encoding(encoding::EncodingOption::kHexadecimal, 1);
    session_.AddInputEncoding(encoding);
  ASSERT_FALSE(session_.GetLocalOptions().SupportsInputEncoding(encoding));
}

TEST_F(PairingSessionTest, AddOutputEncoding) {
  encoding::EncodingOption encoding(encoding::EncodingOption::kHexadecimal, 8);
  session_.AddOutputEncoding(encoding);
  ASSERT_TRUE(session_.GetLocalOptions().SupportsOutputEncoding(encoding));
}

TEST_F(PairingSessionTest, AddOutputEncodingInvalidEncoding) {
  encoding::EncodingOption encoding(encoding::EncodingOption::kUnknown, 8);
  session_.AddOutputEncoding(encoding);
  ASSERT_FALSE(session_.GetLocalOptions().SupportsOutputEncoding(encoding));
}

TEST_F(PairingSessionTest, SetSecret) {
  InitSession();
  InSequence sequence;

  // Do the setup so the session is expecting the secret.
  encoding::EncodingOption encoding(encoding::EncodingOption::kHexadecimal, 8);
  message::ConfigurationMessage configuration(encoding,
      message::OptionsMessage::kInputDevice);
  session_.TestSetConfiguration(configuration);

  EXPECT_CALL(listener_, OnPerformInputDeviceRole());

  session_.TestDoPairingPhase();

  Gamma gamma(5, 0x1);
  Nonce nonce(5, 0x2);
  Alpha alpha(5, 0x3);

  EXPECT_CALL(challenge_, CheckGamma(gamma)).WillOnce(Return(true));
  EXPECT_CALL(challenge_, ExtractNonce(gamma))
      .WillOnce(Return(new Nonce(nonce)));
  EXPECT_CALL(challenge_, GetAlpha(nonce))
      .WillOnce(Return(new Alpha(alpha)));

  EXPECT_CALL(wire_, SendSecretMessage(_));
  EXPECT_CALL(wire_, GetNextMessage());

  session_.SetSecret(gamma);
}

TEST_F(PairingSessionTest, OnSecretMessage) {
  InitSession();
  InSequence sequence;

  // Do the setup to set the secret.
  encoding::EncodingOption encoding(encoding::EncodingOption::kHexadecimal, 8);
  message::ConfigurationMessage configuration(encoding,
      message::OptionsMessage::kInputDevice);
  session_.TestSetConfiguration(configuration);

  EXPECT_CALL(listener_, OnPerformInputDeviceRole());

  session_.TestDoPairingPhase();

  Gamma gamma(5, 0x1);
  Nonce nonce(5, 0x2);
  Alpha alpha(5, 0x3);

  EXPECT_CALL(challenge_, CheckGamma(gamma)).WillOnce(Return(true));
  EXPECT_CALL(challenge_, ExtractNonce(gamma))
      .WillOnce(Return(new Nonce(nonce)));
  EXPECT_CALL(challenge_, GetAlpha(nonce))
      .WillOnce(Return(new Alpha(alpha)));

  EXPECT_CALL(wire_, SendSecretMessage(_));
  EXPECT_CALL(wire_, GetNextMessage());

  session_.SetSecret(gamma);

  EXPECT_CALL(challenge_, GetAlpha(nonce))
      .WillOnce(Return(new Alpha(alpha)));

  EXPECT_CALL(challenge_, GetAlpha(nonce))
        .WillOnce(Return(new Alpha(alpha)));

  EXPECT_CALL(wire_, SendSecretAckMessage(_));
  EXPECT_CALL(listener_, OnPairingSuccess());

  message::SecretMessage message(alpha);
  session_.TestOnSecretMessage(message);
}

TEST_F(PairingSessionTest, OnSecretAckMessage) {
  EXPECT_CALL(listener_, OnPairingSuccess());

  Alpha alpha(5, 0x3);
  message::SecretAckMessage message(alpha);
  session_.TestOnSecretAckmessage(message);
}

}  // namespace pairing
}  // namespace polo
