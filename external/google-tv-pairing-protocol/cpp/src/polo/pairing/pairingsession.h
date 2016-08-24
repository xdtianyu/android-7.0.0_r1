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

#ifndef POLO_PAIRING_PAIRINGSESSION_H_
#define POLO_PAIRING_PAIRINGSESSION_H_

#include <string>

#include "polo/encoding/secretencoder.h"
#include "polo/pairing/pairingcontext.h"
#include "polo/pairing/pairinglistener.h"
#include "polo/pairing/message/messagelistener.h"
#include "polo/wire/polowireadapter.h"

namespace polo {
namespace pairing {

class PairingSession : public message::MessageListener {
 public:
  // The state of the Polo pairing session.
  enum ProtocolState {
    // The Polo session has not yet been initialized.
    kUninitialized,

    // The session is initializing.
    kInitializing,

    // The configuration options are being negotiated with the peer.
    kConfiguring,

    // The local device is being paired with the peer.
    kPairing,

    // Waiting for the secret challenge messsage or response from the peer.
    kWaitingForSecret,

    // The pairing completely successfully.
    kSuccess,

    // There was an error pairing.
    kFailure,
  };

  // Creates a new pairing session. The given wire adapter will be used for
  // sending and receiving protocol messages. The given context contains the
  // local and peer SSL certificates from the establishment of the SSL
  // connection. No ownership is taken of the given pointers.
  PairingSession(wire::PoloWireAdapter* wire,
                 PairingContext* context,
                 PoloChallengeResponse* challenge);

  virtual ~PairingSession();

  // Adds a supported input encoding. This must be called before the session is
  // started.
  void AddInputEncoding(const encoding::EncodingOption& encoding);

  // Adds a supported output encoding. This must be called before the session is
  // started.
  void AddOutputEncoding(const encoding::EncodingOption& encoding);

  // Starts the pairing session. The given listener will be invoked during the
  // pairing session.
  void DoPair(PairingListener* listener);

  // Sets the secret entered by the user. This must be invoked when, and only
  // when, OnPerformInputDeviceRole has been called on the listener.
  // @return Whether the secret was successfully set. If the given secret is
  //         invalid or fails the local check, this will return false.
  bool SetSecret(const Gamma& secret);

  // Gets the encoder used for encoding and decoding the secret challenge. This
  // should only be invoked after OnPerformInputDeviceRole or
  // OnPerformOutputDeviceRole has been called on the listener.
  const encoding::SecretEncoder* encoder() const { return encoder_; }

 protected:
  // Starts the pairing process.
  void DoPairingPhase();

  // Performs the initialization phase of the pairing process.
  virtual void DoInitializationPhase() = 0;

  // Performs the configuration phase of the pairing process.
  virtual void DoConfigurationPhase() = 0;

  // Sets the configuration once it has been negotiated. This must be called
  // by implementations during the configuration phase. Returns true if the
  // configuration was valid and false otherwise. If the configuration was
  // invalid the pairing process can not continue.
  bool SetConfiguration(const message::ConfigurationMessage& message);

  const message::ConfigurationMessage* configuration() const {
    return configuration_;
  }

  // @override
  virtual void OnSecretMessage(const message::SecretMessage& message);

  // @override
  virtual void OnSecretAckMessage(const message::SecretAckMessage& message);

  // @override
  virtual void OnError(pairing::PoloError error);

  // Determines whether this device is acting as the input device.
  bool IsInputDevice() const;

  // Gets the local device role or kUnknown if the configuration has not been
  // established yet.
  message::OptionsMessage::ProtocolRole GetLocalRole() const;

  // Set the current protocol state.
  void set_state(ProtocolState state);

  // Gets the current state of the pairing process.
  ProtocolState state() const { return state_; }

  // Sets the service name.
  void set_service_name(const std::string& service_name) {
    service_name_.assign(service_name);
  }

  // Sets the peer name.
  void set_peer_name(const std::string& peer_name) {
    peer_name_.assign(peer_name);
  }

  // Gets the service name.
  std::string service_name() const { return service_name_; }

  // Gets the peer name.
  std::string peer_name() const { return peer_name_; }

  // Gets the local options.
  const message::OptionsMessage& local_options() const {
    return local_options_;
  }

  // Gets the wire adapter used to send and receive Polo messages.
  wire::PoloWireAdapter* wire() const { return wire_; }

  // Gets the listener that will be notified of pairing events.
  PairingListener* listener() const { return listener_; }

  // Gets the challenge response.
  const PoloChallengeResponse& challenge() const { return *challenge_; }

  // Gets the nonce value.
  const Nonce* nonce() const { return nonce_; }

 private:
  // Performs pairing as the input device.
  void DoInputPairing();

  // Performs pairing as the output device.
  void DoOutputPairing();

  // Determines whether the given encoding option is valid.
  bool IsValidEncodingOption(const encoding::EncodingOption& option) const;

  // Verifies that the given secret is correct.
  bool VerifySecret(const Alpha& secret) const;

  enum {
    // Whether to verify the secret ack. This is not currently required since
    // the ack means the peer already verified the secret.
    kVerifySecretAck = false,

    // The time to wait for a secret.
    kSecretPollTimeoutMs = 500
  };

  ProtocolState state_;
  wire::PoloWireAdapter* wire_;
  PairingContext* context_;
  message::OptionsMessage local_options_;
  PoloChallengeResponse* challenge_;
  PairingListener* listener_;
  message::ConfigurationMessage* configuration_;
  encoding::SecretEncoder* encoder_;
  Nonce* nonce_;
  Gamma* secret_;
  std::string service_name_;
  std::string peer_name_;
};

}  // namespace pairing
}  // namespace polo

#endif  // POLO_PAIRING_PAIRINGSESSION_H_
