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

#include "polo/pairing/clientpairingsession.h"

#include <glog/logging.h>
#include <string>

#include "polo/encoding/hexadecimalencoder.h"
#include "polo/pairing/polochallengeresponse.h"

namespace polo {
namespace pairing {

ClientPairingSession::ClientPairingSession(wire::PoloWireAdapter *wire,
                                           PairingContext *context,
                                           PoloChallengeResponse* challenge,
                                           const std::string &service_name,
                                           const std::string &client_name)
    : PairingSession(wire, context, challenge),
      service_name_(service_name),
      client_name_(client_name) {
}

ClientPairingSession::~ClientPairingSession() {
}

void ClientPairingSession::DoInitializationPhase() {
  if (!client_name_.empty()) {
    message::PairingRequestMessage message(service_name_, client_name_);
    wire()->SendPairingRequestMessage(message);
  } else {
    message::PairingRequestMessage message(service_name_);
    wire()->SendPairingRequestMessage(message);
  }

  LOG(INFO) << "Waiting for PairingRequestAck...";
  wire()->GetNextMessage();
}

void ClientPairingSession::DoConfigurationPhase() {
  const message::ConfigurationMessage* config = configuration();
  if (!config) {
    LOG(ERROR) << "No configuration";
    listener()->OnError(kErrorBadConfiguration);
    return;
  }

  wire()->SendConfigurationMessage(*config);
  wire()->GetNextMessage();

  LOG(INFO) << "Waiting for ConfigurationAck...";
}

void ClientPairingSession::OnPairingRequestAckMessage(
    const message::PairingRequestAckMessage& message) {
  LOG(INFO) << "Handle PairingRequestAckMessage " << message.ToString();

  if (message.has_server_name()) {
    set_peer_name(message.server_name());
  }

  wire()->SendOptionsMessage(local_options());
  wire()->GetNextMessage();
}

void ClientPairingSession::OnOptionsMessage(
    const message::OptionsMessage& message) {
  LOG(INFO) << "HandleOptionsMessage " << message.ToString();

  message::ConfigurationMessage* configuration =
      message::ConfigurationMessage::GetBestConfiguration(local_options(),
                                                          message);

  if (!configuration) {
    LOG(ERROR) << "No compatible configuration: "
        << local_options().ToString() << ", " << message.ToString();
    wire()->SendErrorMessage(kErrorBadConfiguration);
    listener()->OnError(kErrorBadConfiguration);
    return;
  }

  bool valid_configuration = SetConfiguration(*configuration);
  delete configuration;

  if (valid_configuration) {
    DoConfigurationPhase();
  } else {
    wire()->SendErrorMessage(kErrorBadConfiguration);
    listener()->OnError(kErrorBadConfiguration);
  }
}

void ClientPairingSession::OnConfigurationAckMessage(
    const message::ConfigurationAckMessage& message) {
  LOG(INFO) << "HandleConfigurationAckMessage " << message.ToString();

  DoPairingPhase();
}

void ClientPairingSession::OnConfigurationMessage(
    const message::ConfigurationMessage& message) {
  LOG(ERROR) << "Received unexpected ConfigurationMessage";
  wire()->SendErrorMessage(kErrorProtocol);
  listener()->OnError(kErrorProtocol);
}

void ClientPairingSession::OnPairingRequestMessage(
    const message::PairingRequestMessage& message) {
  LOG(ERROR) << "Received unexpected PairingRequestMessage";
  wire()->SendErrorMessage(kErrorProtocol);
  listener()->OnError(kErrorProtocol);
}

}  // namespace pairing
}  // namespace polo
