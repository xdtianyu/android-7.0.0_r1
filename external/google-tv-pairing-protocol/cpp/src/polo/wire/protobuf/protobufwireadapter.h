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

#ifndef POLO_WIRE_PROTOBUF_PROTOBUFWIREADAPTER_H_
#define POLO_WIRE_PROTOBUF_PROTOBUFWIREADAPTER_H_

#include <string>
#include <vector>

#include "polo/wire/polowireadapter.h"
#include "polo/wire/protobuf/polo.pb.h"

namespace polo {
namespace wire {
namespace protobuf {

// Polo wire adapter that transmits Polo messages using protocol buffers.
class ProtobufWireAdapter : public PoloWireAdapter {
 public:
  // Creates a new protocol buffer adapter on the given interface
  // @param interface the wire interface used to send and receive data
  explicit ProtobufWireAdapter(PoloWireInterface* interface);
  virtual ~ProtobufWireAdapter() {}

  /** @override */
  virtual void GetNextMessage();

  /** @override */
  virtual void SendConfigurationMessage(
      const pairing::message::ConfigurationMessage& message);

  /** @override */
  virtual void SendConfigurationAckMessage(
      const pairing::message::ConfigurationAckMessage& message);

  /** @override */
  virtual void SendOptionsMessage(
      const pairing::message::OptionsMessage& message);

  /** @override */
  virtual void SendPairingRequestMessage(
      const pairing::message::PairingRequestMessage& message);

  /** @override */
  virtual void SendPairingRequestAckMessage(
      const pairing::message::PairingRequestAckMessage& message);

  /** @override */
  virtual void SendSecretMessage(
      const pairing::message::SecretMessage& message);

  /** @override */
  virtual void SendSecretAckMessage(
      const pairing::message::SecretAckMessage& message);

  /** @override */
  virtual void SendErrorMessage(pairing::PoloError error);

  /** @override */
  virtual void OnBytesReceived(const std::vector<uint8_t>& data);

  /** @override */
  virtual void OnError();

 private:
  // The current read state.
  enum ReadState {
    // There is no read operation in progress.
    kNone,

    // Waiting to read the message preamble which is 4 bytes representing
    // the size of the next message.
    kPreamble,

    // Waiting to read the message.
    kMessage,
  };

  // Sends a message with the given type and payload. The payload should be
  // the serialized string representation of a protobuf message of the given
  // type.
  void SendMessagePayload(OuterMessage_MessageType type,
                          const std::string& payload);

  // Sends the given outer message.
  void SendOuterMessage(const OuterMessage& message);

  // Parses a received protobuf message.
  void ParseMessage(const std::vector<uint8_t>& data);

  // Parses a configuration message from a serialized protobuf string.
  void ParseConfigurationMessage(const std::string& payload);

  // Parses a configuration ack message from a serialized protobuf string.
  void ParseConfigurationAckMessage(const std::string& payload);

  // Parses an options messages from a serialized protobuf string.
  void ParseOptionsMessage(const std::string& payload);

  // Parses a pairing request message from a serialized protobuf string.
  void ParsePairingRequestMessage(const std::string& payload);

  // Parses a pairing request ack message from a serialized protobuf string.
  void ParsePairingRequestAckMessage(const std::string& payload);

  // Parses a secret message from a serialized protobuf string.
  void ParseSecretMessage(const std::string& payload);

  // Parses a secret ack message from a serialized protobuf string.
  void ParseSecretAckMessage(const std::string& payload);

  // Converts an encoding type from the internal representation to the protobuf
  // representation.
  Options_Encoding_EncodingType EncodingTypeToProto(
      encoding::EncodingOption::EncodingType type);

  // Converts an encoding type from the protobuf representation to the internal
  // representation.
  encoding::EncodingOption::EncodingType EncodingTypeFromProto(
      Options_Encoding_EncodingType type);

  // Converts a role type from the internal representation to the protobuf
  // representation.
  Options_RoleType RoleToProto(
      pairing::message::OptionsMessage::ProtocolRole role);

  // Converts a role type from the protobuf representation to the internal
  // representation.
  pairing::message::OptionsMessage::ProtocolRole RoleFromProto(
      Options_RoleType role);

  ReadState read_state_;

  DISALLOW_COPY_AND_ASSIGN(ProtobufWireAdapter);
};

}  // namespace protobuf
}  // namespace wire
}  // namespace polo

#endif  // POLO_WIRE_PROTOBUF_PROTOBUFWIREADAPTER_H_
