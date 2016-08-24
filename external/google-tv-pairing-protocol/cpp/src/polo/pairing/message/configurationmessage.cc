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

#include "polo/pairing/message/configurationmessage.h"

#include <algorithm>
#include <sstream>
#include <string>

namespace polo {
namespace pairing {
namespace message {

ConfigurationMessage::ConfigurationMessage(
    const encoding::EncodingOption& encoding,
    message::OptionsMessage::ProtocolRole client_role)
    : PoloMessage(PoloMessage::kConfiguration),
      encoding_(encoding),
      client_role_(client_role) {
}

const encoding::EncodingOption& ConfigurationMessage::encoding() const {
  return encoding_;
}

OptionsMessage::ProtocolRole ConfigurationMessage::client_role() const {
  return client_role_;
}

std::string ConfigurationMessage::ToString() const {
  std::ostringstream ss;
  ss << "[ConfigurationMessage encoding=" << encoding_.ToString()
      << ", client_role=" << client_role_ << "]";
  return ss.str();
}

ConfigurationMessage* ConfigurationMessage::GetBestConfiguration(
    const OptionsMessage &local_options,
    const OptionsMessage &peer_options) {
  // Compute the intersection of the available encodings. The sets use the
  // 'less' comparator so they are ordered using EncodingOption's less-than
  // operator.
  encoding::EncodingOption::EncodingSet common_outputs;
  std::insert_iterator<encoding::EncodingOption::EncodingSet> outputs_inserter(
      common_outputs,
      common_outputs.begin());
  set_intersection(local_options.output_encodings().begin(),
                   local_options.output_encodings().end(),
                   peer_options.output_encodings().begin(),
                   peer_options.output_encodings().end(),
                   outputs_inserter,
                   encoding::EncodingOption::EncodingOptionComparator());

  encoding::EncodingOption::EncodingSet common_inputs;
  std::insert_iterator<encoding::EncodingOption::EncodingSet> inputs_inserter(
      common_inputs,
      common_inputs.begin());
  set_intersection(local_options.input_encodings().begin(),
                   local_options.input_encodings().end(),
                   peer_options.input_encodings().begin(),
                   peer_options.input_encodings().end(),
                   inputs_inserter,
                   encoding::EncodingOption::EncodingOptionComparator());

  if (common_outputs.size() == 0 || common_inputs.size() == 0) {
    return NULL;
  }

  const encoding::EncodingOption* best_input = NULL;
  const encoding::EncodingOption* best_output = NULL;

  // Use the last option as the best encoding since that one will be the most
  // complex based on the sorting order.
  if (common_outputs.size() > 0) {
    best_output = &*(--common_outputs.end());
  }

  if (common_inputs.size() > 0) {
    best_input = &*(--common_inputs.end());
  }

  if (local_options.protocol_role_preference()
      == OptionsMessage::kDisplayDevice) {
    if (best_input) {
      return new ConfigurationMessage(*best_input,
                                      OptionsMessage::kDisplayDevice);
    } else {
      return new ConfigurationMessage(*best_output,
                                      OptionsMessage::kInputDevice);
    }
  } else {
    if (best_output) {
      return new ConfigurationMessage(*best_output,
                                      OptionsMessage::kInputDevice);
    } else {
      return new ConfigurationMessage(*best_input,
                                      OptionsMessage::kDisplayDevice);
    }
  }
}

}  // namespace message
}  // namespace pairing
}  // namespace polo
