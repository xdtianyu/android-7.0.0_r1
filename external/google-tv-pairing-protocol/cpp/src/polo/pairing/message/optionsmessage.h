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

#ifndef POLO_PAIRING_MESSAGE_OPTIONSMESSAGE_H_
#define POLO_PAIRING_MESSAGE_OPTIONSMESSAGE_H_

#include <set>
#include <string>
#include "polo/encoding/encodingoption.h"
#include "polo/pairing/message/polomessage.h"

namespace polo {
namespace pairing {
namespace message {

// A message containing the Polo pairing options.
class OptionsMessage : public PoloMessage {
 public:
  // The device role. The display device will be responsible for displaying
  // a secret code, and the user will enter the secret on the input device.
  enum ProtocolRole {
    kUnknown = 0,
    kInputDevice = 1,
    kDisplayDevice = 2,
  };

  // Creates an empty options message. The supported encodings and protocol
  // role preference should be set before sending this message.
  OptionsMessage();

  // Adds a supported input encoding.
  void AddInputEncoding(const encoding::EncodingOption& encoding);

  // Adds a supported output encoding.
  void AddOutputEncoding(const encoding::EncodingOption& encoding);

  // Determines whether the given input encoding is supported.
  bool SupportsInputEncoding(
      const encoding::EncodingOption& encoding) const;

  // Determines whether the given output encoding is supported.
  bool SupportsOutputEncoding(
      const encoding::EncodingOption& encoding) const;

  // Sets the protocol role preference.
  void set_protocol_role_preference(ProtocolRole preference);

  // Gets the protocol role preference.
  ProtocolRole protocol_role_preference() const;

  // Gets the set of supported input encodings.
  const encoding::EncodingOption::EncodingSet& input_encodings() const;

  // Gets the set of supported output encodings.
  const encoding::EncodingOption::EncodingSet& output_encodings() const;

  // @override
  virtual std::string ToString() const;

 private:
  ProtocolRole protocol_role_preference_;
  encoding::EncodingOption::EncodingSet input_encodings_;
  encoding::EncodingOption::EncodingSet output_encodings_;

  DISALLOW_COPY_AND_ASSIGN(OptionsMessage);
};

}  // namespace message
}  // namespace pairing
}  // namespace polo

#endif  // POLO_PAIRING_MESSAGE_OPTIONSMESSAGE_H_
