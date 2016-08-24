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

#include "polo/pairing/message/optionsmessage.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <sstream>

namespace polo {
namespace pairing {
namespace message {

OptionsMessage::OptionsMessage()
    : PoloMessage(PoloMessage::kOptions),
      protocol_role_preference_(kUnknown) {
}

void OptionsMessage::set_protocol_role_preference(
    OptionsMessage::ProtocolRole preference) {
  protocol_role_preference_ = preference;
}

OptionsMessage::ProtocolRole OptionsMessage::protocol_role_preference() const {
  return protocol_role_preference_;
}

void OptionsMessage::AddInputEncoding(
    const encoding::EncodingOption& encoding) {
  input_encodings_.insert(encoding);
}

void OptionsMessage::AddOutputEncoding(
    const encoding::EncodingOption& encoding) {
  output_encodings_.insert(encoding);
}

bool OptionsMessage::SupportsInputEncoding(
    const encoding::EncodingOption& encoding) const {
  return std::find_if(input_encodings_.begin(), input_encodings_.end(),
      encoding::EncodingOption::EncodingOptionPredicate(encoding))
      != input_encodings_.end();
}

bool OptionsMessage::SupportsOutputEncoding(
    const encoding::EncodingOption& encoding) const {
  return std::find_if(output_encodings_.begin(), output_encodings_.end(),
      encoding::EncodingOption::EncodingOptionPredicate(encoding))
      != output_encodings_.end();
}

const encoding::EncodingOption::EncodingSet&
    OptionsMessage::input_encodings() const {
  return input_encodings_;
}

const encoding::EncodingOption::EncodingSet&
    OptionsMessage::output_encodings() const {
  return output_encodings_;
}

std::string OptionsMessage::ToString() const {
  std::ostringstream ss;
  ss << "[OptionsMessage inputs=";
  encoding::EncodingOption::EncodingSet::const_iterator iter
      = input_encodings_.begin();
  while (iter != input_encodings_.end()) {
    ss << iter->ToString();
    if (++iter != input_encodings_.end()) {
      ss << ",";
    }
  }

  ss << ", outputs=";
  iter = output_encodings_.begin();
  while (iter != output_encodings_.end()) {
    ss << iter->ToString() << ",";
    if (++iter != output_encodings_.end()) {
      ss << ",";
    }
  }

  ss << ", pref=" << protocol_role_preference_ << "]";
  return ss.str();
}

}  // namespace message
}  // namespace pairing
}  // namespace polo

