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

#ifndef DHCP_CLIENT_DHCP_H_
#define DHCP_CLIENT_DHCP_H_

#include <base/macros.h>

namespace dhcp_client {

class DHCP {
 public:
  // DHCP service type.
  enum ServiceType {
    SERVICE_TYPE_IPV4 = 0,
    SERVICE_TYPE_IPV6 = 1,
    SERVICE_TYPE_BOTH = 2
  };

  // DHCP states.
  enum class State {
    INIT,
    SELECT,
    REQUEST,
    BOUND,
    RENEW,
    REBIND,
    REBOOT,
    RELEASE
  };
};

}  // namespace dhcp_client

#endif  // DHCP_CLIENT_DHCP_H_
