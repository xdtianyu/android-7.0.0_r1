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

#include "polo/pairing/message/pairingrequestmessage.h"

#include <sstream>
#include <string>

namespace polo {
namespace pairing {
namespace message {

PairingRequestMessage::PairingRequestMessage(
    const std::string& service_name)
    : PoloMessage(PoloMessage::kPairingRequest),
      service_name_(service_name),
      client_name_("") {
}

PairingRequestMessage::PairingRequestMessage(const std::string& service_name,
                                             const std::string& client_name)
    : PoloMessage(PoloMessage::kPairingRequest),
      service_name_(service_name),
      client_name_(client_name) {
}

std::string PairingRequestMessage::service_name() const {
  return service_name_;
}

std::string PairingRequestMessage::client_name() const {
  return client_name_;
}

bool PairingRequestMessage::has_client_name() const {
  return client_name_.length() > 0;
}

std::string PairingRequestMessage::ToString() const {
  std::ostringstream ss;
  ss << "[PairingRequestMessage service_name=" << service_name_
      << ", client_name=" << client_name_ << "]";
  return ss.str();
}

}  // namespace message
}  // namespace pairing
}  // namespace polo
