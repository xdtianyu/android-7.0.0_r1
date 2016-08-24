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

#include "polo/pairing/message/secretackmessage.h"

#include <sstream>
#include "polo/util/poloutil.h"

namespace polo {
namespace pairing {
namespace message {

SecretAckMessage::SecretAckMessage(const std::vector<uint8_t>& secret)
    : PoloMessage(PoloMessage::kSecretAck),
      secret_(secret) {
}

const std::vector<uint8_t>& SecretAckMessage::secret() const {
  return secret_;
}

std::string SecretAckMessage::ToString() const {
  std::ostringstream ss;
  ss << "[SecretAckMessage secret="
      << util::PoloUtil::BytesToHexString(&secret_[0], secret_.size())
      << "]";
  return ss.str();
}

}  // namespace message
}  // namespace pairing
}  // namespace polo
