//
// Copyright (C) 2014 The Android Open Source Project
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

#ifndef SHILL_NET_NDISC_H_
#define SHILL_NET_NDISC_H_

// Neighbor discovery related definitions. This is needed because kernel
// currently does not export these definitions to the user space.

// Netlink multicast group for neighbor discovery user option message.
#define RTMGRP_ND_USEROPT 0x80000

// Neighbor Discovery user option header definition.
struct NDUserOptionHeader {
  NDUserOptionHeader() {
    memset(this, 0, sizeof(*this));
  }
  uint8_t type;
  uint8_t length;
  uint16_t reserved;
  uint32_t lifetime;
} __attribute__((__packed__));

// Neighbor Discovery user option type definition.
#define ND_OPT_RDNSS 25       /* RFC 5006 */
#define ND_OPT_DNSSL 31       /* RFC 6106 */

// Infinity lifetime.
#define ND_OPT_LIFETIME_INFINITY 0xFFFFFFFF

#endif  // SHILL_NET_NDISC_H_
