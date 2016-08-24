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

#ifndef POLO_PAIRING_MESSAGE_PAIRINGREQUESTMESSAGE_H_
#define POLO_PAIRING_MESSAGE_PAIRINGREQUESTMESSAGE_H_

#include <string>

#include "polo/pairing/message/polomessage.h"

namespace polo {
namespace pairing {
namespace message {

// A message to request a polo pairing session.
class PairingRequestMessage : public PoloMessage {
 public:
  // Creates a pairing request message with the given service name and no client
  // name.
  // @param service_name the service name
  explicit PairingRequestMessage(const std::string& service_name);

  // Creates a pairing request message with the given service name and client
  // name.
  // @param service_name the service name
  // @param client_name the client name
  PairingRequestMessage(const std::string& service_name,
                        const std::string& client_name);

  // Gets the service name.
  std::string service_name() const;

  // Gets the client name.
  std::string client_name() const;

  // Gets whether there is a client name.
  bool has_client_name() const;

  // @override
  virtual std::string ToString() const;
 private:
  std::string service_name_;
  std::string client_name_;

  DISALLOW_COPY_AND_ASSIGN(PairingRequestMessage);
};

}  // namespace message
}  // namespace pairing
}  // namespace polo

#endif  // POLO_PAIRING_MESSAGE_PAIRINGREQUESTMESSAGE_H_
