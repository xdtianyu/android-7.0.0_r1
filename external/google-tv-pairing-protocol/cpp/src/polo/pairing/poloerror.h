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

#ifndef POLO_PAIRING_POLOERROR_H_
#define POLO_PAIRING_POLOERROR_H_

namespace polo {
namespace pairing {

// Error codes for the pairing process.
enum PoloError {
  // Not an error.
  kOk                                = 0,

  // Network-layer error.
  kErrorNetwork                      = 1,

  // Polo pairing protocol error.
  kErrorProtocol                     = 2,

  // Invalid pairing configuration.
  kErrorBadConfiguration             = 3,

  // The user provided challenge response (secret) is incorrect.
  kErrorInvalidChallengeResponse     = 4
};

}  // namespace pairing
}  // namespace polo

#endif  // POLO_PAIRING_POLOERROR_H_
