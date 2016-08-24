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

#ifndef POLO_PAIRING_MESSAGE_POLOMESSAGE_H_
#define POLO_PAIRING_MESSAGE_POLOMESSAGE_H_

#include <string>
#include "polo/util/macros.h"

namespace polo {
namespace pairing {
namespace message {

// An abstract polo message.
class PoloMessage {
 public:
  // The possible polo message types.
  enum PoloMessageType {
    kUnknown = 0,
    kPairingRequest = 10,
    kPairingRequestAck = 11,
    kOptions = 20,
    kConfiguration = 30,
    kConfigurationAck = 31,
    kSecret = 40,
    kSecretAck = 41,
  };

  // Creates a new polo message of the given message type. Subclasses should
  // provided their corresponding message type.
  // @param message_type the type of this message
  explicit PoloMessage(PoloMessageType message_type);

  virtual ~PoloMessage() {}

  // Gets the type of this message.
  PoloMessageType message_type() const;

  // Gets a string representation of this message.
  virtual std::string ToString() const = 0;

 private:
  PoloMessageType message_type_;

  DISALLOW_COPY_AND_ASSIGN(PoloMessage);
};

}  // namespace message
}  // namespace pairing
}  // namespace polo

#endif  // POLO_PAIRING_MESSAGE_POLOMESSAGE_H_
