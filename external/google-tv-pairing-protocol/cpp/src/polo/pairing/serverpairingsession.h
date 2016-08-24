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

#ifndef POLO_PAIRING_SERVERPAIRINGSESSION_H_
#define POLO_PAIRING_SERVERPAIRINGSESSION_H_

#include <string>

#include "polo/pairing/pairingsession.h"
#include "polo/pairing/pairinglistener.h"

namespace polo {
namespace pairing {

// A Polo server pairing session. This handles the logic for sending and
// receiving Polo messages during a pairing session.
class ServerPairingSession : public PairingSession {
 public:
  // Creates a new server pairing session.
  // @param wire the wire adapter used to send and receive Polo messages
  // @param context the Polo pairing context
  // @param challenge the challenge response
  // @param server_name the server name
  ServerPairingSession(wire::PoloWireAdapter *wire,
                       PairingContext *context,
                       PoloChallengeResponse* challenge,
                       const std::string &server_name);

  ~ServerPairingSession();

  // @override
  virtual void OnPairingRequestMessage(
      const message::PairingRequestMessage& message);

  // @override
  virtual void OnOptionsMessage(
      const message::OptionsMessage& message);

  // @override
  virtual void OnConfigurationMessage(
      const message::ConfigurationMessage& message);

  // @override
  virtual void OnConfigurationAckMessage(
      const message::ConfigurationAckMessage& message);

  // @override
  virtual void OnPairingRequestAckMessage(
      const message::PairingRequestAckMessage& message);

 protected:
  // @override
  virtual void DoInitializationPhase();

  // @override
  virtual void DoConfigurationPhase();

 private:
  std::string server_name_;
};

}  // namespace pairing
}  // namespace polo

#endif  // POLO_PAIRING_SERVERPAIRINGSESSION_H_
