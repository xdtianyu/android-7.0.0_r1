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

// A PoloWireAdapter implementation that uses protocol buffers for transmitting
// messages.

#include "polo/wire/protobuf/protobufwireadapter.h"

#include <glog/logging.h>
#include <algorithm>
#include <set>
#include <string>
#include <vector>
#include "polo/util/poloutil.h"

namespace polo {
namespace wire {
namespace protobuf {

ProtobufWireAdapter::ProtobufWireAdapter(PoloWireInterface* interface)
    : PoloWireAdapter(interface), read_state_(kNone) {
}

void ProtobufWireAdapter::GetNextMessage() {
  if (read_state_ != kNone) {
    LOG(ERROR) << "Invalid state: GetNextMessage called during a read";
    listener()->OnError(pairing::kErrorProtocol);
    return;
  }

  // Read the 4 byte preable which contains the length of the next message.
  read_state_ = kPreamble;
  interface()->Receive(4);
}

void ProtobufWireAdapter::SendConfigurationMessage(
    const pairing::message::ConfigurationMessage& message) {
  Configuration configuration;

  configuration.mutable_encoding()->set_symbol_length(
      message.encoding().symbol_length());

  configuration.mutable_encoding()->set_type(
      EncodingTypeToProto(message.encoding().encoding_type()));

  configuration.set_client_role(RoleToProto(message.client_role()));

  SendMessagePayload(OuterMessage_MessageType_MESSAGE_TYPE_CONFIGURATION,
                     configuration.SerializeAsString());
}

void ProtobufWireAdapter::SendConfigurationAckMessage(
    const pairing::message::ConfigurationAckMessage& message) {
  ConfigurationAck ack;

  SendMessagePayload(OuterMessage_MessageType_MESSAGE_TYPE_CONFIGURATION_ACK,
                     ack.SerializeAsString());
}

void ProtobufWireAdapter::SendOptionsMessage(
    const pairing::message::OptionsMessage& message) {
  LOG(INFO) << "Sending " << message.ToString();

  Options options;
  encoding::EncodingOption::EncodingSet::const_iterator iter;
  for (iter = message.input_encodings().begin();
       iter != message.input_encodings().end();
       iter++) {
    encoding::EncodingOption option = *iter;
    Options_Encoding* encoding = options.add_input_encodings();
    encoding->set_symbol_length(option.symbol_length());
    encoding->set_type(EncodingTypeToProto(option.encoding_type()));
  }

  for (iter = message.output_encodings().begin();
       iter != message.output_encodings().end();
       iter++) {
    encoding::EncodingOption option = *iter;
    Options_Encoding* encoding = options.add_output_encodings();
    encoding->set_symbol_length(option.symbol_length());
    encoding->set_type(EncodingTypeToProto(option.encoding_type()));
  }

  options.set_preferred_role(RoleToProto(message.protocol_role_preference()));

  SendMessagePayload(OuterMessage_MessageType_MESSAGE_TYPE_OPTIONS,
                     options.SerializeAsString());
}

void ProtobufWireAdapter::SendPairingRequestMessage(
    const pairing::message::PairingRequestMessage& message) {
  LOG(INFO) << "Sending " << message.ToString();

  PairingRequest request;
  request.set_service_name(message.service_name());

  if (message.has_client_name()) {
    request.set_client_name(message.client_name());
  }

  SendMessagePayload(OuterMessage_MessageType_MESSAGE_TYPE_PAIRING_REQUEST,
                     request.SerializeAsString());
}

void ProtobufWireAdapter::SendPairingRequestAckMessage(
    const pairing::message::PairingRequestAckMessage& message) {
  LOG(INFO) << "Sending " << message.ToString();

  PairingRequestAck ack;

  if (message.has_server_name()) {
    ack.set_server_name(message.server_name());
  }

  SendMessagePayload(OuterMessage_MessageType_MESSAGE_TYPE_PAIRING_REQUEST_ACK,
                     ack.SerializeAsString());
}

void ProtobufWireAdapter::SendSecretMessage(
    const pairing::message::SecretMessage& message) {
  LOG(INFO) << "Sending " << message.ToString();

  Secret secret;
  secret.set_secret(&message.secret()[0], message.secret().size());

  SendMessagePayload(OuterMessage_MessageType_MESSAGE_TYPE_SECRET,
                     secret.SerializeAsString());
}

void ProtobufWireAdapter::SendSecretAckMessage(
    const pairing::message::SecretAckMessage& message) {
  LOG(INFO) << "Sending " << message.ToString();

  SecretAck ack;
  ack.set_secret(&message.secret()[0], message.secret().size());

  SendMessagePayload(OuterMessage_MessageType_MESSAGE_TYPE_SECRET_ACK,
                     ack.SerializeAsString());
}

void ProtobufWireAdapter::OnBytesReceived(
    const std::vector<uint8_t>& data) {
  if (read_state_ == kMessage) {
    // We were waiting for a message, so parse the message and reset the read
    // state.
    read_state_ = kNone;
    ParseMessage(data);
  } else if (read_state_ == kPreamble && data.size() == 4) {
    // If we were waiting for the preamble and we received the expected 4 bytes,
    // then wait for the rest of the message now that we know the size.
    read_state_ = kMessage;
    uint32_t message_length = util::PoloUtil::BigEndianBytesToInt(&data[0]);
    interface()->Receive(message_length);
  } else {
    LOG(ERROR) << "Unexpected state: " << read_state_
        << " bytes: " << data.size();
    listener()->OnError(pairing::kErrorProtocol);
  }
}

void ProtobufWireAdapter::ParseMessage(const std::vector<uint8_t>& data) {
  OuterMessage outer;

  std::string string(reinterpret_cast<const char*>(&data[0]), data.size());
  if (!outer.ParseFromString(string)) {
    LOG(ERROR) << "Error parsing outer message";
    listener()->OnError(pairing::kErrorProtocol);
    return;
  }

  if (outer.status() != OuterMessage_Status_STATUS_OK) {
    LOG(ERROR) << "Got error message: " << outer.status();
    pairing::PoloError error = pairing::kErrorProtocol;
    switch (outer.status()) {
      case OuterMessage_Status_STATUS_BAD_CONFIGURATION:
        error = pairing::kErrorBadConfiguration;
        break;
      case OuterMessage_Status_STATUS_BAD_SECRET:
        error = pairing::kErrorInvalidChallengeResponse;
        break;
    }
    listener()->OnError(error);
    return;
  }

  LOG(INFO) << "Parsing message type: " << outer.type();

  switch (outer.type()) {
    case OuterMessage_MessageType_MESSAGE_TYPE_CONFIGURATION:
      ParseConfigurationMessage(outer.payload());
      break;
    case OuterMessage_MessageType_MESSAGE_TYPE_CONFIGURATION_ACK:
      ParseConfigurationAckMessage(outer.payload());
      break;
    case OuterMessage_MessageType_MESSAGE_TYPE_OPTIONS:
      ParseOptionsMessage(outer.payload());
      break;
    case OuterMessage_MessageType_MESSAGE_TYPE_PAIRING_REQUEST:
      ParsePairingRequestMessage(outer.payload());
      break;
    case OuterMessage_MessageType_MESSAGE_TYPE_PAIRING_REQUEST_ACK:
      ParsePairingRequestAckMessage(outer.payload());
      break;
    case OuterMessage_MessageType_MESSAGE_TYPE_SECRET:
      ParseSecretMessage(outer.payload());
      break;
    case OuterMessage_MessageType_MESSAGE_TYPE_SECRET_ACK:
      ParseSecretAckMessage(outer.payload());
      break;
    default:
      LOG(ERROR) << "Unknown message type " << outer.type();
      listener()->OnError(pairing::kErrorProtocol);
      return;
  }
}

void ProtobufWireAdapter::ParseConfigurationMessage(
    const std::string& payload) {
  Configuration configuration;
  if (!configuration.ParseFromString(payload)) {
    LOG(ERROR) << "Invalid ConfigurationMessage";
    listener()->OnError(pairing::kErrorProtocol);
    return;
  }

  encoding::EncodingOption encoding(
      EncodingTypeFromProto(configuration.encoding().type()),
      configuration.encoding().symbol_length());
  pairing::message::OptionsMessage::ProtocolRole role =
      RoleFromProto(configuration.client_role());

  pairing::message::ConfigurationMessage message(encoding, role);
  listener()->OnConfigurationMessage(message);
}

void ProtobufWireAdapter::ParseConfigurationAckMessage(
    const std::string& payload) {
  ConfigurationAck ack;
  if (!ack.ParseFromString(payload)) {
    LOG(ERROR) << "Invalid ConfigurationAckMessage";
    listener()->OnError(pairing::kErrorProtocol);
    return;
  }

  pairing::message::ConfigurationAckMessage message;
  listener()->OnConfigurationAckMessage(message);
}

void ProtobufWireAdapter::ParseOptionsMessage(const std::string& payload) {
  Options options;
  if (!options.ParseFromString(payload)) {
    LOG(ERROR) << "Invalid OptionsMessage";
    listener()->OnError(pairing::kErrorProtocol);
    return;
  }

  pairing::message::OptionsMessage message;

  for (int i = 0; i < options.input_encodings().size(); i++) {
    const Options_Encoding& encoding = options.input_encodings(i);

    encoding::EncodingOption option(EncodingTypeFromProto(encoding.type()),
                                    encoding.symbol_length());
    message.AddInputEncoding(option);
  }

  for (int i = 0; i < options.output_encodings().size(); i++) {
    const Options_Encoding& encoding = options.output_encodings(i);

    encoding::EncodingOption option(EncodingTypeFromProto(encoding.type()),
                                    encoding.symbol_length());
    message.AddOutputEncoding(option);
  }

  message.set_protocol_role_preference(
      RoleFromProto(options.preferred_role()));

  listener()->OnOptionsMessage(message);
}

void ProtobufWireAdapter::ParsePairingRequestMessage(
    const std::string& payload) {
  PairingRequest request;
  if (!request.ParseFromString(payload)) {
    LOG(ERROR) << "Invalid PairingRequestMessage";
    listener()->OnError(pairing::kErrorProtocol);
    return;
  }

  if (request.has_client_name()) {
    pairing::message::PairingRequestMessage message(request.service_name(),
                                  request.client_name());
    listener()->OnPairingRequestMessage(message);
  } else {
    pairing::message::PairingRequestMessage message(request.service_name());
    listener()->OnPairingRequestMessage(message);
  }
}

void ProtobufWireAdapter::ParsePairingRequestAckMessage(
    const std::string& payload) {
  PairingRequestAck ack;
  if (!ack.ParseFromString(payload)) {
    LOG(ERROR) << "Invalid PairingRequestAckMessage";
    listener()->OnError(pairing::kErrorProtocol);
    return;
  }

  if (ack.has_server_name()) {
    pairing::message::PairingRequestAckMessage message(ack.server_name());
    listener()->OnPairingRequestAckMessage(message);
  } else {
    pairing::message::PairingRequestAckMessage message;
    listener()->OnPairingRequestAckMessage(message);
  }
}

void ProtobufWireAdapter::ParseSecretMessage(const std::string& payload) {
  Secret secret;
  if (!secret.ParseFromString(payload)) {
    LOG(ERROR) << "Invalid SecretMessage";
    listener()->OnError(pairing::kErrorProtocol);
    return;
  }

  const std::vector<uint8_t> secret_bytes(secret.secret().begin(),
                                                secret.secret().end());

  pairing::message::SecretMessage message(secret_bytes);
  listener()->OnSecretMessage(message);
}

void ProtobufWireAdapter::ParseSecretAckMessage(const std::string& payload) {
  SecretAck ack;
  if (!ack.ParseFromString(payload)) {
    LOG(ERROR) << "Invalid SecretAckMessage";
    listener()->OnError(pairing::kErrorProtocol);
    return;
  }

  std::vector<uint8_t> secret_bytes(ack.secret().begin(),
                                          ack.secret().end());

  pairing::message::SecretAckMessage message(secret_bytes);
  listener()->OnSecretAckMessage(message);
}

void ProtobufWireAdapter::OnError() {
  LOG(ERROR) << "OnError";
  listener()->OnError(pairing::kErrorNetwork);
}

void ProtobufWireAdapter::SendErrorMessage(pairing::PoloError error) {
  OuterMessage outer;
  outer.set_protocol_version(1);

  OuterMessage_Status status;
  switch (error) {
    case pairing::kErrorBadConfiguration:
      status = OuterMessage_Status_STATUS_BAD_CONFIGURATION;
      break;
    case pairing::kErrorInvalidChallengeResponse:
      status = OuterMessage_Status_STATUS_BAD_SECRET;
      break;
    default:
      status = OuterMessage_Status_STATUS_ERROR;
  }

  outer.set_status(status);

  SendOuterMessage(outer);
}

void ProtobufWireAdapter::SendMessagePayload(OuterMessage_MessageType type,
                                             const std::string& payload) {
  // Create the outer message which specifies the message type and payload data.
  OuterMessage outer;
  outer.set_type(type);
  outer.set_payload(payload);
  outer.set_protocol_version(1);
  outer.set_status(OuterMessage_Status_STATUS_OK);

  SendOuterMessage(outer);
}

void ProtobufWireAdapter::SendOuterMessage(const OuterMessage& message) {
  // Send the message as a string, prepended with a 4 byte preamble containing
  // the length of the message in bytes.
  std::string outer_string = message.SerializeAsString();

  uint8_t* size_bytes;
  util::PoloUtil::IntToBigEndianBytes(outer_string.length(), size_bytes);

  std::vector<uint8_t> data(outer_string.length() + 4);

  std::vector<uint8_t>::iterator iter = data.begin();
  std::copy(size_bytes, size_bytes + 4, iter);
  std::copy(outer_string.begin(), outer_string.end(), iter + 4);
  delete[] size_bytes;

  interface()->Send(data);
}

Options_Encoding_EncodingType ProtobufWireAdapter::EncodingTypeToProto(
    encoding::EncodingOption::EncodingType type) {
  switch (type) {
    case encoding::EncodingOption::kAlphaNumeric:
      return Options_Encoding_EncodingType_ENCODING_TYPE_ALPHANUMERIC;
    case encoding::EncodingOption::kHexadecimal:
      return Options_Encoding_EncodingType_ENCODING_TYPE_HEXADECIMAL;
    case encoding::EncodingOption::kNumeric:
      return Options_Encoding_EncodingType_ENCODING_TYPE_NUMERIC;
    case encoding::EncodingOption::kQRCode:
      return Options_Encoding_EncodingType_ENCODING_TYPE_QRCODE;
    default:
      return Options_Encoding_EncodingType_ENCODING_TYPE_UNKNOWN;
  }
}

encoding::EncodingOption::EncodingType
    ProtobufWireAdapter::EncodingTypeFromProto(
        Options_Encoding_EncodingType type) {
  switch (type) {
    case Options_Encoding_EncodingType_ENCODING_TYPE_ALPHANUMERIC:
      return encoding::EncodingOption::kAlphaNumeric;
    case Options_Encoding_EncodingType_ENCODING_TYPE_HEXADECIMAL:
      return encoding::EncodingOption::kHexadecimal;
    case Options_Encoding_EncodingType_ENCODING_TYPE_NUMERIC:
      return encoding::EncodingOption::kNumeric;
    case Options_Encoding_EncodingType_ENCODING_TYPE_QRCODE:
      return encoding::EncodingOption::kQRCode;
    default:
      return encoding::EncodingOption::kUnknown;
  }
}

Options_RoleType ProtobufWireAdapter::RoleToProto(
    pairing::message::OptionsMessage::ProtocolRole role) {
  switch (role) {
    case pairing::message::OptionsMessage::kInputDevice:
      return Options_RoleType_ROLE_TYPE_INPUT;
    case pairing::message::OptionsMessage::kDisplayDevice:
      return Options_RoleType_ROLE_TYPE_OUTPUT;
    default:
      return Options_RoleType_ROLE_TYPE_UNKNOWN;
  }
}

pairing::message::OptionsMessage::ProtocolRole
    ProtobufWireAdapter::RoleFromProto(Options_RoleType role) {
  switch (role) {
    case Options_RoleType_ROLE_TYPE_INPUT:
      return pairing::message::OptionsMessage::kInputDevice;
    case Options_RoleType_ROLE_TYPE_OUTPUT:
      return pairing::message::OptionsMessage::kDisplayDevice;
    default:
      return pairing::message::OptionsMessage::kUnknown;
  }
}

}  // namespace protobuf
}  // namespace wire
}  // namespace polo
