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

#include "polo/pairing/pairingsession.h"

#include <glog/logging.h>
#include "polo/encoding/hexadecimalencoder.h"
#include "polo/util/poloutil.h"

namespace polo {
namespace pairing {

PairingSession::PairingSession(wire::PoloWireAdapter* wire,
                               PairingContext* context,
                               PoloChallengeResponse* challenge)
    : state_(kUninitialized),
      wire_(wire),
      context_(context),
      challenge_(challenge),
      configuration_(NULL),
      encoder_(NULL),
      nonce_(NULL),
      secret_(NULL) {
  wire_->set_listener(this);

  local_options_.set_protocol_role_preference(context->is_server() ?
      message::OptionsMessage::kDisplayDevice
      : message::OptionsMessage::kInputDevice);
}

PairingSession::~PairingSession() {
  if (configuration_) {
    delete configuration_;
  }

  if (encoder_) {
    delete encoder_;
  }

  if (nonce_) {
    delete nonce_;
  }

  if (secret_) {
    delete secret_;
  }
}

void PairingSession::AddInputEncoding(
    const encoding::EncodingOption& encoding) {
  if (state_ != kUninitialized) {
    LOG(ERROR) << "Attempt to add input encoding to active session";
    return;
  }

  if (!IsValidEncodingOption(encoding)) {
    LOG(ERROR) << "Invalid input encoding: " << encoding.ToString();
    return;
  }

  local_options_.AddInputEncoding(encoding);
}

void PairingSession::AddOutputEncoding(
    const encoding::EncodingOption& encoding) {
  if (state_ != kUninitialized) {
    LOG(ERROR) << "Attempt to add output encoding to active session";
    return;
  }

  if (!IsValidEncodingOption(encoding)) {
    LOG(ERROR) << "Invalid output encoding: " << encoding.ToString();
    return;
  }

  local_options_.AddOutputEncoding(encoding);
}

bool PairingSession::SetSecret(const Gamma& secret) {
  secret_ = new Gamma(secret);

  if (!IsInputDevice() || state_ != kWaitingForSecret) {
    LOG(ERROR) << "Invalid state: unexpected secret";
    return false;
  }

  if (!challenge().CheckGamma(secret)) {
    LOG(ERROR) << "Secret failed local check";
    return false;
  }

  nonce_ = challenge().ExtractNonce(secret);
  if (!nonce_) {
    LOG(ERROR) << "Failed to extract nonce";
    return false;
  }

  const Alpha* gen_alpha = challenge().GetAlpha(*nonce_);
  if (!gen_alpha) {
    LOG(ERROR) << "Failed to get alpha";
    return false;
  }

  message::SecretMessage secret_message(*gen_alpha);
  delete gen_alpha;

  wire_->SendSecretMessage(secret_message);

  LOG(INFO) << "Waiting for SecretAck...";
  wire_->GetNextMessage();

  return true;
}

void PairingSession::DoPair(PairingListener *listener) {
  listener_ = listener;
  listener_->OnSessionCreated();

  if (context_->is_server()) {
    LOG(INFO) << "Pairing started (SERVER mode)";
  } else {
    LOG(INFO) << "Pairing started (CLIENT mode)";
  }
  LOG(INFO) << "Local options: " << local_options_.ToString();

  set_state(kInitializing);
  DoInitializationPhase();
}

void PairingSession::DoPairingPhase() {
  if (IsInputDevice()) {
    DoInputPairing();
  } else {
    DoOutputPairing();
  }
}

void PairingSession::DoInputPairing() {
  set_state(kWaitingForSecret);
  listener_->OnPerformInputDeviceRole();
}

void PairingSession::DoOutputPairing() {
  size_t nonce_length = configuration_->encoding().symbol_length() / 2;
  size_t bytes_needed = nonce_length / encoder_->symbols_per_byte();

  uint8_t* random = util::PoloUtil::GenerateRandomBytes(bytes_needed);
  nonce_ = new Nonce(random, random + bytes_needed);
  delete[] random;

  const Gamma* gamma = challenge().GetGamma(*nonce_);
  if (!gamma) {
    LOG(ERROR) << "Failed to get gamma";
    wire()->SendErrorMessage(kErrorProtocol);
    listener()->OnError(kErrorProtocol);
    return;
  }

  listener_->OnPerformOutputDeviceRole(*gamma);
  delete gamma;

  set_state(kWaitingForSecret);

  LOG(INFO) << "Waiting for Secret...";
  wire_->GetNextMessage();
}

void PairingSession::set_state(ProtocolState state) {
  LOG(INFO) << "New state: " << state;
  state_ = state;
}

bool PairingSession::SetConfiguration(
    const message::ConfigurationMessage& message) {
  const encoding::EncodingOption& encoding = message.encoding();

  if (!IsValidEncodingOption(encoding)) {
    LOG(ERROR) << "Invalid configuration: " << encoding.ToString();
    return false;
  }

  if (encoder_) {
    delete encoder_;
    encoder_ = NULL;
  }

  switch (encoding.encoding_type()) {
    case encoding::EncodingOption::kHexadecimal:
      encoder_ = new encoding::HexadecimalEncoder();
      break;
    default:
      LOG(ERROR) << "Unsupported encoding type: "
          << encoding.encoding_type();
      return false;
  }

  if (configuration_) {
    delete configuration_;
  }
  configuration_ = new message::ConfigurationMessage(message.encoding(),
                                                     message.client_role());
  return true;
}

void PairingSession::OnSecretMessage(const message::SecretMessage& message) {
  if (state() != kWaitingForSecret) {
    LOG(ERROR) << "Invalid state: unexpected secret message";
    wire()->SendErrorMessage(kErrorProtocol);
    listener()->OnError(kErrorProtocol);
    return;
  }

  if (!VerifySecret(message.secret())) {
    wire()->SendErrorMessage(kErrorInvalidChallengeResponse);
    listener_->OnError(kErrorInvalidChallengeResponse);
    return;
  }

  const Alpha* alpha = challenge().GetAlpha(*nonce_);
  if (!alpha) {
    LOG(ERROR) << "Failed to get alpha";
    wire()->SendErrorMessage(kErrorProtocol);
    listener()->OnError(kErrorProtocol);
    return;
  }

  message::SecretAckMessage ack(*alpha);
  delete alpha;

  wire_->SendSecretAckMessage(ack);

  listener_->OnPairingSuccess();
}

void PairingSession::OnSecretAckMessage(
    const message::SecretAckMessage& message) {
  if (kVerifySecretAck && !VerifySecret(message.secret())) {
    wire()->SendErrorMessage(kErrorInvalidChallengeResponse);
    listener_->OnError(kErrorInvalidChallengeResponse);
    return;
  }

  listener_->OnPairingSuccess();
}

void PairingSession::OnError(pairing::PoloError error) {
  listener_->OnError(error);
}

bool PairingSession::VerifySecret(const Alpha& secret) const {
  if (!nonce_) {
    LOG(ERROR) << "Nonce not set";
    return false;
  }

  const Alpha* gen_alpha = challenge().GetAlpha(*nonce_);
  if (!gen_alpha) {
    LOG(ERROR) << "Failed to get alpha";
    return false;
  }

  bool valid = (secret == *gen_alpha);

  if (!valid) {
    LOG(ERROR) << "Inband secret did not match. Expected ["
        << util::PoloUtil::BytesToHexString(&(*gen_alpha)[0], gen_alpha->size())
        << "], got ["
        << util::PoloUtil::BytesToHexString(&secret[0], secret.size())
        << "]";
  }

  delete gen_alpha;
  return valid;
}

message::OptionsMessage::ProtocolRole PairingSession::GetLocalRole() const {
  if (!configuration_) {
    return message::OptionsMessage::kUnknown;
  }

  if (context_->is_client()) {
    return configuration_->client_role();
  } else {
    return configuration_->client_role() ==
        message::OptionsMessage::kDisplayDevice ?
            message::OptionsMessage::kInputDevice
            : message::OptionsMessage::kDisplayDevice;
  }
}

bool PairingSession::IsInputDevice() const {
  return GetLocalRole() == message::OptionsMessage::kInputDevice;
}

bool PairingSession::IsValidEncodingOption(
    const encoding::EncodingOption& option) const {
  // Legal values of GAMMALEN must be an even number of at least 2 bytes.
  return option.encoding_type() != encoding::EncodingOption::kUnknown
      && (option.symbol_length() % 2 == 0)
      && (option.symbol_length() >= 2);
}

}  // namespace pairing
}  // namespace polo
