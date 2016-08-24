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

// Tests for ProtobufWireAdapter.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <polo/util/poloutil.h>
#include <polo/wire/protobuf/protobufwireadapter.h>
#include "polo/wire/mocks.h"

using ::testing::InSequence;
using ::testing::Mock;
using ::testing::Return;
using ::testing::StrictMock;

namespace polo {
namespace wire {
namespace protobuf {

// A mock MessageListener.
class MockMessageListener : public pairing::message::MessageListener {
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

  MOCK_METHOD1(OnSecretMessage,
               void(const pairing::message::SecretMessage& message));

  MOCK_METHOD1(OnSecretAckMessage,
               void(const pairing::message::SecretAckMessage& message));

  MOCK_METHOD1(OnError, void(pairing::PoloError error));
};

// Test fixture for ProtobufWireAdapter tests.
class ProtobufWireAdapterTest : public ::testing::Test {
 public:
  ProtobufWireAdapterTest() : interface_(), adapter_(&interface_) {}

 protected:
  virtual void SetUp() {
    adapter_.set_listener(&listener_);
  }

  // Expects that a call to GetNextMessage will be made which triggers a read
  // for the 4 byte preamble.
  void ExpectGetPreamble() {
    EXPECT_CALL(interface_, Receive(4));
    adapter_.GetNextMessage();
  }

  // Expects that a call to GetNextMessage will be made, and the preamble will
  // be read containing the given message size. This will trigger another read
  // for the full message.
  void ExpectReadPreamble(uint32_t message_size) {
    ExpectGetPreamble();

    unsigned char* size_bytes;
    util::PoloUtil::IntToBigEndianBytes(message_size, size_bytes);
    EXPECT_CALL(interface_, Receive(message_size));

    adapter_.OnBytesReceived(
        std::vector<uint8_t>(size_bytes, size_bytes + 4));
  }

  // Expects that the given OuterMessage will be sent over the interface.
  void ExpectSend(const OuterMessage& message) {
    std::string outer_string = message.SerializeAsString();

    unsigned char* size_bytes;
    util::PoloUtil::IntToBigEndianBytes(outer_string.length(), size_bytes);

    std::vector<unsigned char> data(outer_string.length() + 4);
    unsigned char* buffer = &data[0];
    memcpy(buffer, size_bytes, 4);
    memcpy((buffer + 4), &outer_string[0], outer_string.length());

    EXPECT_CALL(interface_, Send(data));
  }

  StrictMock<MockWireInterface> interface_;
  StrictMock<MockMessageListener> listener_;
  ProtobufWireAdapter adapter_;
};

// Verifies that a call to GetNextMessage will trigger a read for the 4 byte
// preamble.
TEST_F(ProtobufWireAdapterTest, GetNextMessage) {
  ExpectGetPreamble();
}

// Verifies that once the preamble is received, a read will be triggered for
// the full message.
TEST_F(ProtobufWireAdapterTest, OnBytesReceivedPreamble) {
  InSequence sequence;

  ExpectReadPreamble(0xAABBCCDD);
}

// Verifies that a ConfigurationMessage is successfully sent over the interface.
TEST_F(ProtobufWireAdapterTest, SendConfigurationMessage) {
  InSequence sequence;

  Configuration proto;
  proto.set_client_role(Options_RoleType_ROLE_TYPE_OUTPUT);
  proto.mutable_encoding()->set_type(
      Options_Encoding_EncodingType_ENCODING_TYPE_QRCODE);
  proto.mutable_encoding()->set_symbol_length(64);

  OuterMessage outer;
  outer.set_type(OuterMessage_MessageType_MESSAGE_TYPE_CONFIGURATION);
  outer.set_payload(proto.SerializeAsString());
  outer.set_protocol_version(1);
  outer.set_status(OuterMessage_Status_STATUS_OK);

  ExpectSend(outer);

  pairing::message::ConfigurationMessage message(
      encoding::EncodingOption(encoding::EncodingOption::kQRCode, 64),
      pairing::message::OptionsMessage::kDisplayDevice);
  adapter_.SendConfigurationMessage(message);
}

// Verifies that a ConfigurationAckMessage is successfully sent over the
// interface.
TEST_F(ProtobufWireAdapterTest, SendConfigurationAckMessage) {
  InSequence sequence;

  ConfigurationAck proto;

  OuterMessage outer;
  outer.set_type(OuterMessage_MessageType_MESSAGE_TYPE_CONFIGURATION_ACK);
  outer.set_payload(proto.SerializeAsString());
  outer.set_protocol_version(1);
  outer.set_status(OuterMessage_Status_STATUS_OK);

  ExpectSend(outer);

  pairing::message::ConfigurationAckMessage message;
  adapter_.SendConfigurationAckMessage(message);
}

// Verifies that an OptionsMessage is successfully sent over the interface.
TEST_F(ProtobufWireAdapterTest, SendOptionsMessage) {
  InSequence sequence;

  Options proto;
  proto.set_preferred_role(Options_RoleType_ROLE_TYPE_INPUT);
  Options_Encoding* encoding = proto.add_input_encodings();
  encoding->set_type(Options_Encoding_EncodingType_ENCODING_TYPE_NUMERIC);
  encoding->set_symbol_length(16);

  encoding = proto.add_input_encodings();
  encoding->set_type(Options_Encoding_EncodingType_ENCODING_TYPE_ALPHANUMERIC);
  encoding->set_symbol_length(32);

  encoding = proto.add_output_encodings();
  encoding->set_type(Options_Encoding_EncodingType_ENCODING_TYPE_HEXADECIMAL);
  encoding->set_symbol_length(128);

  encoding = proto.add_output_encodings();
  encoding->set_type(Options_Encoding_EncodingType_ENCODING_TYPE_QRCODE);
  encoding->set_symbol_length(512);

  OuterMessage outer;
  outer.set_type(OuterMessage_MessageType_MESSAGE_TYPE_OPTIONS);
  outer.set_payload(proto.SerializeAsString());
  outer.set_protocol_version(1);
  outer.set_status(OuterMessage_Status_STATUS_OK);

  ExpectSend(outer);

  pairing::message::OptionsMessage message;
  message.set_protocol_role_preference(
      pairing::message::OptionsMessage::kInputDevice);

  // Note, the input and output encoding sets are sorted by complexity, so these
  // should be in the same order as the encodings added to the proto above to
  // ensure the assert matches.
  message.AddInputEncoding(
      encoding::EncodingOption(encoding::EncodingOption::kNumeric, 16));
  message.AddInputEncoding(
      encoding::EncodingOption(encoding::EncodingOption::kAlphaNumeric, 32));
  message.AddOutputEncoding(
      encoding::EncodingOption(encoding::EncodingOption::kHexadecimal, 128));
  message.AddOutputEncoding(
      encoding::EncodingOption(encoding::EncodingOption::kQRCode, 512));

  adapter_.SendOptionsMessage(message);
}

// Verifies that a PairingRequestMessage is successfully sent over the
// interface.
TEST_F(ProtobufWireAdapterTest, SendPairingRequestMessage) {
  InSequence sequence;

  PairingRequest proto;
  proto.set_client_name("foo-client");
  proto.set_service_name("foo-service");

  OuterMessage outer;
  outer.set_type(OuterMessage_MessageType_MESSAGE_TYPE_PAIRING_REQUEST);
  outer.set_payload(proto.SerializeAsString());
  outer.set_protocol_version(1);
  outer.set_status(OuterMessage_Status_STATUS_OK);

  ExpectSend(outer);

  pairing::message::PairingRequestMessage message("foo-service", "foo-client");
  adapter_.SendPairingRequestMessage(message);
}

// Verifies that a SendPairingRequestAckMesssage is successfully sent over the
// interface.
TEST_F(ProtobufWireAdapterTest, SendPairingRequestAckMessage) {
  InSequence sequence;

  PairingRequestAck proto;
  proto.set_server_name("foo-server");

  OuterMessage outer;
  outer.set_type(OuterMessage_MessageType_MESSAGE_TYPE_PAIRING_REQUEST_ACK);
  outer.set_payload(proto.SerializeAsString());
  outer.set_protocol_version(1);
  outer.set_status(OuterMessage_Status_STATUS_OK);

  ExpectSend(outer);

  pairing::message::PairingRequestAckMessage message("foo-server");
  adapter_.SendPairingRequestAckMessage(message);
}

// Verifies that a SecretMessage is successfully sent over the interface.
TEST_F(ProtobufWireAdapterTest, SendSecretMessage) {
  InSequence sequence;

  std::vector<unsigned char> secret(4);
  secret[0] = 0xAA;
  secret[1] = 0xBB;
  secret[2] = 0xCC;
  secret[3] = 0xDD;

  Secret proto;
  proto.set_secret(&secret[0], secret.size());

  OuterMessage outer;
  outer.set_type(OuterMessage_MessageType_MESSAGE_TYPE_SECRET);
  outer.set_payload(proto.SerializeAsString());
  outer.set_protocol_version(1);
  outer.set_status(OuterMessage_Status_STATUS_OK);

  ExpectSend(outer);

  pairing::message::SecretMessage message(secret);
  adapter_.SendSecretMessage(message);
}

// Verifies that a SecretAckMessage is successfully sent over the interface.
TEST_F(ProtobufWireAdapterTest, SendSecretAckMessage) {
  InSequence sequence;

  std::vector<unsigned char> secret(4);
  secret[0] = 0xAA;
  secret[1] = 0xBB;
  secret[2] = 0xCC;
  secret[3] = 0xDD;

  SecretAck proto;
  proto.set_secret(&secret[0], secret.size());

  OuterMessage outer;
  outer.set_type(OuterMessage_MessageType_MESSAGE_TYPE_SECRET_ACK);
  outer.set_payload(proto.SerializeAsString());
  outer.set_protocol_version(1);
  outer.set_status(OuterMessage_Status_STATUS_OK);

  ExpectSend(outer);

  pairing::message::SecretAckMessage message(secret);
  adapter_.SendSecretAckMessage(message);
}

// Verifies that an ErrorMessage is successfully sent over the interface.
TEST_F(ProtobufWireAdapterTest, SendErrorMessage) {
  InSequence sequence;

  OuterMessage outer;
  outer.set_protocol_version(1);
  outer.set_status(OuterMessage_Status_STATUS_BAD_SECRET);

  ExpectSend(outer);

  adapter_.SendErrorMessage(pairing::kErrorInvalidChallengeResponse);
}

}  // namespace protobuf
}  // namespace wire
}  // namespace polo
