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

#include "polo/pairing/serverpairingsession.h"

#include <glog/logging.h>
#include "polo/pairing/polochallengeresponse.h"
#include "polo/util/poloutil.h"

namespace polo {
namespace pairing {

ServerPairingSession::ServerPairingSession(wire::PoloWireAdapter *wire,
                                           PairingContext *context,
                                           PoloChallengeResponse* challenge,
                                           const std::string& server_name)
    : PairingSession(wire, context, challenge),
      server_name_(server_name) {
}

ServerPairingSession::~ServerPairingSession() {
}

void ServerPairingSession::DoInitializationPhase() {
  LOG(INFO) << "Waiting for PairingRequest...";
  wire()->GetNextMessage();
}

void ServerPairingSession::DoConfigurationPhase() {
  LOG(INFO) << "Waiting for Configuration...";
  wire()->GetNextMessage();
}

void ServerPairingSession::OnPairingRequestMessage(
    const message::PairingRequestMessage& message) {
  set_service_name(message.service_name());

  if (message.has_client_name()) {
    set_peer_name(message.client_name());
  }

  message::PairingRequestAckMessage ack(service_name());
  wire()->SendPairingRequestAckMessage(ack);

  LOG(INFO) << "Waiting for Options...";
  wire()->GetNextMessage();
}

void ServerPairingSession::OnOptionsMessage(
    const message::OptionsMessage& message) {
  // The client is responsible for negotiating a valid configuration, so just
  // send the server options.
  wire()->SendOptionsMessage(local_options());

  DoConfigurationPhase();
}

void ServerPairingSession::OnConfigurationMessage(
    const message::ConfigurationMessage& message) {
  if (!SetConfiguration(message)) {
    wire()->SendErrorMessage(kErrorBadConfiguration);
    listener()->OnError(kErrorBadConfiguration);
    return;
  }

  const encoding::EncodingOption& encoding = message.encoding();

  if (GetLocalRole() == message::OptionsMessage::kDisplayDevice) {
    if (!local_options().SupportsOutputEncoding(encoding)) {
      LOG(ERROR) << "Unsupported output encoding requested: "
          << encoding.encoding_type();
      wire()->SendErrorMessage(kErrorBadConfiguration);
      listener()->OnError(kErrorBadConfiguration);
      return;
    }
  } else {
    if (!local_options().SupportsInputEncoding(encoding)) {
      LOG(ERROR) << "Unsupported input encoding requested: "
          << encoding.encoding_type();
      wire()->SendErrorMessage(kErrorBadConfiguration);
      listener()->OnError(kErrorBadConfiguration);
      return;
    }
  }

  message::ConfigurationAckMessage ack;
  wire()->SendConfigurationAckMessage(ack);

  DoPairingPhase();
}

void ServerPairingSession::OnConfigurationAckMessage(
    const message::ConfigurationAckMessage& message) {
  LOG(ERROR) << "Received unexpected ConfigurationAckMessage";
  wire()->SendErrorMessage(kErrorProtocol);
  listener()->OnError(kErrorProtocol);
}

void ServerPairingSession::OnPairingRequestAckMessage(
    const message::PairingRequestAckMessage& message) {
  LOG(ERROR) << "Received unexpected PairingRequestAckMessage";
  wire()->SendErrorMessage(kErrorProtocol);
  listener()->OnError(kErrorProtocol);
}

}  // namespace pairing
}  // namespace polo
