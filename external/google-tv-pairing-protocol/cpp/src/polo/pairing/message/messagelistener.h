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

#ifndef POLO_PAIRING_MESSAGE_MESSAGELISTENER_H_
#define POLO_PAIRING_MESSAGE_MESSAGELISTENER_H_

#include "polo/pairing/poloerror.h"
#include "polo/pairing/message/configurationackmessage.h"
#include "polo/pairing/message/configurationmessage.h"
#include "polo/pairing/message/optionsmessage.h"
#include "polo/pairing/message/pairingrequestackmessage.h"
#include "polo/pairing/message/pairingrequestmessage.h"
#include "polo/pairing/message/secretackmessage.h"
#include "polo/pairing/message/secretmessage.h"

namespace polo {
namespace pairing {
namespace message {

// A listener interface for Polo messages.
class MessageListener {
 public:
  virtual ~MessageListener() {}

  // Handles a message containing the peer's configuration.
  virtual void OnConfigurationMessage(
      const ConfigurationMessage& message) = 0;

  // Handles an acknowledgment that the peer received the local configuration.
  virtual void OnConfigurationAckMessage(
      const ConfigurationAckMessage& message) = 0;

  // Handles a message containing the peer's supported pairing options.
  virtual void OnOptionsMessage(const OptionsMessage& message) = 0;

  // Handles a message from the peer requesting a pairing session.
  virtual void OnPairingRequestMessage(
      const PairingRequestMessage& message) = 0;

  // Handles a message from the peer acknowledging a pairing request.
  virtual void OnPairingRequestAckMessage(
      const PairingRequestAckMessage& message) = 0;

  // Handles a challenge message from the peer.
  virtual void OnSecretMessage(const SecretMessage& message) = 0;

  // Handles an challenge response from the peer.
  virtual void OnSecretAckMessage(const SecretAckMessage& message) = 0;

  // Handles a Polo error.
  virtual void OnError(PoloError error) = 0;
};

}  // namespace message
}  // namespace pairing
}  // namespace polo

#endif  // POLO_PAIRING_MESSAGE_MESSAGELISTENER_H_
