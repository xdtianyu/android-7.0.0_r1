//
// Copyright (C) 2015 The Android Open Source Project
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

#ifndef DHCP_CLIENT_DHCP_OPTIONS_H_
#define DHCP_CLIENT_DHCP_OPTIONS_H_

namespace dhcp_client {
// Constants for DHCP options.
const uint8_t kDHCPOptionPad = 0;
const uint8_t kDHCPOptionSubnetMask = 1;
const uint8_t kDHCPOptionRouter = 3;
const uint8_t kDHCPOptionDNSServer = 6;
const uint8_t kDHCPOptionDomainName = 15;
const uint8_t kDHCPOptionVendorSpecificInformation = 43;
const uint8_t kDHCPOptionRequestedIPAddr = 50;
const uint8_t kDHCPOptionLeaseTime = 51;
const uint8_t kDHCPOptionMessageType = 53;
const uint8_t kDHCPOptionServerIdentifier = 54;
const uint8_t kDHCPOptionParameterRequestList = 55;
const uint8_t kDHCPOptionMessage = 56;
const uint8_t kDHCPOptionRenewalTime = 58;
const uint8_t kDHCPOptionRebindingTime = 59;
const uint8_t kDHCPOptionClientIdentifier = 61;
const uint8_t kDHCPOptionEnd = 255;

const int kDHCPOptionLength = 312;
}  // namespace dhcp_client

#endif  // DHCP_CLIENT_DHCP_OPTIONS_H_
