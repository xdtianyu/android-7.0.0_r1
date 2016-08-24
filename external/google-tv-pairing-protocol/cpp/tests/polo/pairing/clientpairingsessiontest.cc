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

// Tests for ClientPairingSession.

#include <gtest/gtest.h>
#include <polo/pairing/clientpairingsession.h>
#include "polo/pairing/mocks.h"
#include "polo/wire/mocks.h"

using ::testing::InSequence;
using ::testing::Mock;
using ::testing::Return;
using ::testing::StrictMock;
using ::testing::_;

namespace polo {
namespace pairing {

class TestClientPairingSession : public ClientPairingSession {
 public:
  TestClientPairingSession(wire::PoloWireAdapter* wire,
                          PairingContext* context,
                          PoloChallengeResponse* challenge)
      : ClientPairingSession(wire, context, challenge, "service1", "client1") {
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

MATCHER_P2(PairingRequestEq, service_name, client_name, "") {
  return arg.service_name() == service_name
      && arg.client_name() == client_name;
}

class ClientPairingSessionTest : public ::testing::Test {
 protected:
  ClientPairingSessionTest()
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

    EXPECT_CALL(wire_, SendPairingRequestMessage(
        PairingRequestEq("service1", "client1")));

    EXPECT_CALL(wire_, GetNextMessage());

    session_.DoPair(&listener_);
  }

  StrictMock<wire::MockWireInterface> interface_;
  StrictMock<wire::MockWireAdapter> wire_;
  StrictMock<MockChallengeResponse> challenge_;
  PairingContext context_;
  StrictMock<MockPairingListener> listener_;
  StrictMock<TestClientPairingSession> session_;
};

TEST_F(ClientPairingSessionTest, DoInitializationPhase) {
  InitSession();
}

TEST_F(ClientPairingSessionTest, DoConfigurationPhase) {
  InitSession();
  InSequence sequence;
  encoding::EncodingOption encoding(encoding::EncodingOption::kHexadecimal, 8);
  message::ConfigurationMessage configuration(encoding,
      message::OptionsMessage::kInputDevice);
  session_.TestSetConfiguration(configuration);

  EXPECT_CALL(wire_, SendConfigurationMessage(_));
  EXPECT_CALL(wire_, GetNextMessage());

  session_.TestDoConfigurationPhase();
}

TEST_F(ClientPairingSessionTest, OnPairingRequestAckMessage) {
  InitSession();
  InSequence sequence;
  EXPECT_CALL(wire_, SendOptionsMessage(_));
  EXPECT_CALL(wire_, GetNextMessage());

  message::PairingRequestAckMessage message;
  session_.OnPairingRequestAckMessage(message);
}

TEST_F(ClientPairingSessionTest, OnOptionsMessage) {
  encoding::EncodingOption encoding(encoding::EncodingOption::kHexadecimal, 8);
  session_.AddInputEncoding(encoding);
  session_.AddOutputEncoding(encoding);

  InitSession();

  InSequence sequence;

  EXPECT_CALL(wire_, SendConfigurationMessage(_));
  EXPECT_CALL(wire_, GetNextMessage());

  message::OptionsMessage message;
  message.AddInputEncoding(encoding);
  message.AddOutputEncoding(encoding);
  message.set_protocol_role_preference(message::OptionsMessage::kInputDevice);
  session_.OnOptionsMessage(message);
}

TEST_F(ClientPairingSessionTest, OnConfigurationAckMessage) {
  InitSession();
  InSequence sequence;
  encoding::EncodingOption encoding(encoding::EncodingOption::kHexadecimal, 8);
  message::ConfigurationMessage configuration(encoding,
      message::OptionsMessage::kInputDevice);
  session_.TestSetConfiguration(configuration);

  EXPECT_CALL(listener_, OnPerformInputDeviceRole());

  message::ConfigurationAckMessage message;
  session_.OnConfigurationAckMessage(message);
}

}  // namespace pairing
}  // namespace polo
