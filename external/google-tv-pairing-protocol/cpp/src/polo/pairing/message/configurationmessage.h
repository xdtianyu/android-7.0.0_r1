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

#ifndef POLO_PAIRING_MESSAGE_CONFIGURATIONMESSAGE_H_
#define POLO_PAIRING_MESSAGE_CONFIGURATIONMESSAGE_H_

#include <string>
#include "polo/encoding/encodingoption.h"
#include "polo/pairing/message/optionsmessage.h"
#include "polo/pairing/message/polomessage.h"

namespace polo {
namespace pairing {
namespace message {

// A message that contains Polo configuration options.
class ConfigurationMessage : public PoloMessage {
 public:
  // Creates a configuration message with the given encoding and role.
  // @param encoding the configured encoding options
  // @param client_role the client role
  ConfigurationMessage(const encoding::EncodingOption &encoding,
                       OptionsMessage::ProtocolRole client_role);

  // Gets the configured encoding options.
  const ::polo::encoding::EncodingOption& encoding() const;

  // Gets the client role.
  OptionsMessage::ProtocolRole client_role() const;

  // @override
  virtual std::string ToString() const;

  // Computes the best configuration given the local and peer options. This
  // performs a negotiation of the local and peer options and selects the most
  // complex common input and output encodings and a local role.
  // @param local_options the local options
  // @param peer_options the peer options
  static ConfigurationMessage* GetBestConfiguration(
      const OptionsMessage& local_options,
      const OptionsMessage& peer_options);

 private:
  encoding::EncodingOption encoding_;
  OptionsMessage::ProtocolRole client_role_;

  DISALLOW_COPY_AND_ASSIGN(ConfigurationMessage);
};

}  // namespace message
}  // namespace pairing
}  // namespace polo

#endif  // POLO_PAIRING_MESSAGE_CONFIGURATIONMESSAGE_H_
