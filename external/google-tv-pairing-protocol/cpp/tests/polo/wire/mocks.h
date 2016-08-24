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

#ifndef TV_GTVREMOTE_TESTS_POLO_WIRE_MOCKS_H_
#define TV_GTVREMOTE_TESTS_POLO_WIRE_MOCKS_H_

#include <gmock/gmock.h>
#include <polo/wire/polowireinterface.h>
#include <polo/wire/polowireadapter.h>
#include <vector>

namespace polo {
namespace wire {

// A mock PoloWireInterface.
class MockWireInterface : public PoloWireInterface {
 public:
  MOCK_METHOD1(Send, void(const std::vector<uint8_t>& data));
  MOCK_METHOD1(Receive, void(size_t num_bytes));
};

// A mock PoloWireAdapter.
class MockWireAdapter : public PoloWireAdapter {
 public:
  explicit MockWireAdapter(PoloWireInterface* interface)
      : PoloWireAdapter(interface) {
  }

  MOCK_METHOD0(GetNextMessage, void());
  MOCK_METHOD1(SendConfigurationMessage,
               void(const pairing::message::ConfigurationMessage& message));
  MOCK_METHOD1(SendConfigurationAckMessage,
               void(const pairing::message::ConfigurationAckMessage& message));
  MOCK_METHOD1(SendOptionsMessage,
               void(const pairing::message::OptionsMessage& message));
  MOCK_METHOD1(SendPairingRequestMessage,
               void(const pairing::message::PairingRequestMessage& message));
  MOCK_METHOD1(SendPairingRequestAckMessage,
               void(const pairing::message::PairingRequestAckMessage& message));
  MOCK_METHOD1(SendSecretMessage,
               void(const pairing::message::SecretMessage& message));
  MOCK_METHOD1(SendSecretAckMessage,
               void(const pairing::message::SecretAckMessage& message));
  MOCK_METHOD1(SendErrorMessage, void(pairing::PoloError error));
  MOCK_METHOD1(OnBytesReceived, void(const std::vector<uint8_t>& data));
  MOCK_METHOD0(OnError, void());
};

}  // namespace wire
}  // namespace polo


#endif  // TV_GTVREMOTE_TESTS_POLO_WIRE_MOCKS_H_
