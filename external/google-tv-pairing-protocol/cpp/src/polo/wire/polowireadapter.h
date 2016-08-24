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

#ifndef POLO_WIRE_POLOWIREADAPTER_H_
#define POLO_WIRE_POLOWIREADAPTER_H_

#include "polo/wire/polowireinterface.h"

#include "polo/pairing/message/messagelistener.h"

namespace polo {
namespace wire {

// Abstract class for protocol adapters that send and receive Polo messages. The
// adapter is responsible for serializing and deserializing messages sent and
// received from the supplied PoloWireInterface.
class PoloWireAdapter : public PoloWireListener {
 public:
  // Creates a new adapter on the given interface. The interface should only
  // exist in the context of this adapter and will be freed when this adapter is
  // freed.
  // @param interface the interface used to send and receive data
  explicit PoloWireAdapter(PoloWireInterface* interface);
  virtual ~PoloWireAdapter() {}

  // Sets the listener that will receive incoming Polo messages. A listener
  // must be set before using this adapter.
  void set_listener(pairing::message::MessageListener* listener);

  // Gets the next message from the interface asynchronously. The listener
  // will be invoked when a message has been received. An error will occur if
  // this method is invoked again before a message or error is received by
  // the listener.
  virtual void GetNextMessage() = 0;

  // Sends a configuration message to the peer.
  virtual void SendConfigurationMessage(
      const pairing::message::ConfigurationMessage& message) = 0;

  // Sends a configuration acknowledgment to the peer.
  virtual void SendConfigurationAckMessage(
      const pairing::message::ConfigurationAckMessage& message) = 0;

  // Sends an options message to the peer.
  virtual void SendOptionsMessage(
      const pairing::message::OptionsMessage& message) = 0;

  // Sends a pairing request message to the peer.
  virtual void SendPairingRequestMessage(
      const pairing::message::PairingRequestMessage& message) = 0;

  // Sends a pairing request acknowledgment to the peer.
  virtual void SendPairingRequestAckMessage(
      const pairing::message::PairingRequestAckMessage& message) = 0;

  // Sends a secret message to the peer.
  virtual void SendSecretMessage(
      const pairing::message::SecretMessage& message) = 0;

  // Sends a secret acknowledgment to the peer.
  virtual void SendSecretAckMessage(
      const pairing::message::SecretAckMessage& message) = 0;

  // Sends an error message to the peer.
  virtual void SendErrorMessage(pairing::PoloError error) = 0;

 protected:
  // Gets the Polo wire interface used to send and receive data.
  PoloWireInterface* interface() { return interface_; }

  // Get the listener that will be notified of received Polo messages.
  pairing::message::MessageListener* listener() { return listener_; }

 private:
  PoloWireInterface* interface_;
  pairing::message::MessageListener* listener_;

  DISALLOW_COPY_AND_ASSIGN(PoloWireAdapter);
};

}  // namespace wire
}  // namespace polo

#endif  // POLO_WIRE_POLOWIREADAPTER_H_
