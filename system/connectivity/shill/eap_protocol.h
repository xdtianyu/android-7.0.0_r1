//
// Copyright (C) 2013 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef SHILL_EAP_PROTOCOL_H_
#define SHILL_EAP_PROTOCOL_H_

#include <base/compiler_specific.h>

namespace shill {

namespace eap_protocol {

struct ALIGNAS(1) Ieee8021xHdr {
  uint8_t version;
  uint8_t type;
  uint16_t length;
};

enum IeeeEapolVersion {
  kIeee8021xEapolVersion1 = 1,
  kIeee8021xEapolVersion2 = 2
};

enum IeeeEapolType {
  kIIeee8021xTypeEapPacket = 0,
  kIIeee8021xTypeEapolStart = 1,
  kIIeee8021xTypeEapolLogoff = 2,
  kIIeee8021xTypeEapolKey = 3,
  kIIeee8021xTypeEapolEncapsulatedAsfAlert = 4
};

struct ALIGNAS(1) EapHeader {
  uint8_t code;
  uint8_t identifier;
  uint16_t length;  // including code and identifier; network byte order
};

enum EapCode {
  kEapCodeRequest = 1,
  kEapCodeRespnose = 2,
  kEapCodeSuccess = 3,
  kEapCodeFailure = 4
};

}  // namespace eap_protocol

}  // namespace shill

#endif  // SHILL_EAP_PROTOCOL_H_
