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

#include "polo/pairing/message/polomessage.h"

namespace polo {
namespace pairing {
namespace message {

PoloMessage::PoloMessage(PoloMessage::PoloMessageType message_type)
    : message_type_(message_type) {
}

PoloMessage::PoloMessageType PoloMessage::message_type() const {
  return message_type_;
}

}  // namespace message
}  // namespace pairing
}  // namespace polo
