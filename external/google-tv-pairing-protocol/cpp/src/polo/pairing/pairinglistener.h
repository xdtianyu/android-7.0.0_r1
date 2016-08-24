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

#ifndef POLO_PAIRING_PAIRINGLISTENER_H_
#define POLO_PAIRING_PAIRINGLISTENER_H_

#include <string>
#include "polo/pairing/polochallengeresponse.h"
#include "polo/pairing/poloerror.h"

namespace polo {
namespace pairing {

class PairingSession;

// Listener for a pairing session.
class PairingListener {
 public:
  virtual ~PairingListener() {}

  // Invoked when a pairing session has been created.
  virtual void OnSessionCreated() = 0;

  // Invoked when a pairing session has ended.
  virtual void OnSessionEnded() = 0;

  // Invoked when the output device role should be performed. The client should
  // display a decoded secret based on the given gamma value.
  virtual void OnPerformOutputDeviceRole(const Gamma& gamma) = 0;

  // Invoked when the input device role should be performed. The client should
  // prompt the user to enter the secret displayed on the output device.
  virtual void OnPerformInputDeviceRole() = 0;

  // Invoked when pairing has completed successfully.
  virtual void OnPairingSuccess() = 0;

  // Invoked when the pairing operation has been cancelled.
  virtual void OnPairingCancelled() = 0;

  // Invoked if there was an error during the pairing process.
  virtual void OnError(PoloError error) = 0;
};

}  // namespace pairing
}  // namespace polo

#endif  // POLO_PAIRING_PAIRINGLISTENER_H_
