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

#ifndef TV_GTVREMOTE_TESTS_POLO_PAIRING_MOCKS_H_
#define TV_GTVREMOTE_TESTS_POLO_PAIRING_MOCKS_H_

#include <gmock/gmock.h>
#include <polo/pairing/polochallengeresponse.h>

namespace polo {
namespace pairing {

// A mock PoloChallengeResponse.
class MockChallengeResponse : public PoloChallengeResponse {
 public:
  MockChallengeResponse() : PoloChallengeResponse(NULL, NULL) {}
  MOCK_CONST_METHOD1(GetAlpha, Alpha*(const Nonce& nonce));
  MOCK_CONST_METHOD1(GetGamma, Gamma*(const Nonce& nonce));
  MOCK_CONST_METHOD1(ExtractNonce, Nonce*(const Gamma& gamma));
  MOCK_CONST_METHOD1(CheckGamma, bool(const Gamma& gammma));
};

// A mock PairingListener.
class MockPairingListener : public PairingListener {
 public:
  MOCK_METHOD0(OnSessionCreated, void());
  MOCK_METHOD0(OnSessionEnded, void());
  MOCK_METHOD1(OnPerformOutputDeviceRole, void(const Gamma& gamma));
  MOCK_METHOD0(OnPerformInputDeviceRole, void());
  MOCK_METHOD0(OnPairingSuccess, void());
  MOCK_METHOD0(OnPairingCancelled, void());
  MOCK_METHOD1(OnError, void(PoloError error));
};

}  // namespace pairing
}  // namespace polo

#endif  // TV_GTVREMOTE_TESTS_POLO_PAIRING_MOCKS_H_
