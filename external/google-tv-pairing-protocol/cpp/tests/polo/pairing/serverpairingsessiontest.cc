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

// Tests for ServerPairingSession.

#include <gtest/gtest.h>
#include <polo/pairing/serverpairingsession.h>
#include "polo/pairing/mocks.h"
#include "polo/wire/mocks.h"

using ::testing::InSequence;
using ::testing::Mock;
using ::testing::Return;
using ::testing::StrictMock;
using ::testing::_;

namespace polo {
namespace pairing {

class TestServerPairingSession : public ServerPairingSession {
 public:
  TestServerPairingSession(wire::PoloWireAdapter* wire,
                          PairingContext* context,
                          PoloChallengeResponse* challenge)
      : ServerPairingSession(wire, context, challenge, "server1") {
  }

  void TestDoInitializationPhase() {
    DoInitializationPhase();
  }

  void TestDoConfigurationPhase() {
    DoConfigurationPhase();
  }

  bool TestSetConfiguration(const message::ConfigurationMessage& config) {
    return SetConfiguration(config);
  }
};

class ServerPairingSessionTest : public ::testing::Test {
 protected:
  ServerPairingSessionTest()
      : interface_(),
        wire_(&interface_),
        challenge_(),
        context_(NULL, NULL, true),
        session_(&wire_, &context_, &challenge_) {
  }

  virtual void SetUp() {
  }

  virtual void TearDown() {
  }

  void InitSession() {
    InSequence sequence;

    EXPECT_CALL(listener_, OnSessionCreated());
    EXPECT_CALL(wire_, GetNextMessage());

    session_.DoPair(&listener_);
  }

  StrictMock<wire::MockWireInterface> interface_;
  StrictMock<wire::MockWireAdapter> wire_;
  StrictMock<MockChallengeResponse> challenge_;
  PairingContext context_;
  StrictMock<MockPairingListener> listener_;
  StrictMock<TestServerPairingSession> session_;
};

TEST_F(ServerPairingSessionTest, DoInitializationPhase) {
  InitSession();
}

TEST_F(ServerPairingSessionTest, DoConfigurationPhase) {
  InitSession();
  InSequence sequence;
  EXPECT_CALL(wire_, GetNextMessage());

  session_.TestDoInitializationPhase();
}

TEST_F(ServerPairingSessionTest, OnPairingRequestMessage) {
  InitSession();
  InSequence sequence;
  EXPECT_CALL(wire_, SendPairingRequestAckMessage(_));
  EXPECT_CALL(wire_, GetNextMessage());

  message::PairingRequestMessage message("service1");
  session_.OnPairingRequestMessage(message);
}

TEST_F(ServerPairingSessionTest, OnOptionsMessage) {
  InitSession();
  InSequence sequence;
  EXPECT_CALL(wire_, SendOptionsMessage(_));
  EXPECT_CALL(wire_, GetNextMessage());

  message::OptionsMessage message;
  session_.OnOptionsMessage(message);
}

TEST_F(ServerPairingSessionTest, OnConfigurationMessage) {
  encoding::EncodingOption encoding(encoding::EncodingOption::kHexadecimal, 8);
  session_.AddInputEncoding(encoding);
  session_.AddOutputEncoding(encoding);

  InitSession();

  InSequence sequence;
  EXPECT_CALL(wire_, SendConfigurationAckMessage(_));

  EXPECT_CALL(challenge_, GetGamma(_)).WillOnce(Return(new Gamma(5, 0x5)));
  EXPECT_CALL(listener_, OnPerformOutputDeviceRole(Gamma(5, 0x5)));
  EXPECT_CALL(wire_, GetNextMessage());

  message::ConfigurationMessage message(encoding,
      message::OptionsMessage::kInputDevice);
  session_.OnConfigurationMessage(message);
}

}  // namespace pairing
}  // namespace polo
