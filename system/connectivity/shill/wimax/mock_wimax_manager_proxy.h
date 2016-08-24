//
// Copyright (C) 2012 The Android Open Source Project
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

#ifndef SHILL_WIMAX_MOCK_WIMAX_MANAGER_PROXY_H_
#define SHILL_WIMAX_MOCK_WIMAX_MANAGER_PROXY_H_

#include <vector>

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/wimax/wimax_manager_proxy_interface.h"

namespace shill {

class MockWiMaxManagerProxy : public WiMaxManagerProxyInterface {
 public:
  MockWiMaxManagerProxy();
  ~MockWiMaxManagerProxy() override;

  MOCK_METHOD1(set_devices_changed_callback,
               void(const DevicesChangedCallback& callback));
  MOCK_METHOD1(Devices, RpcIdentifiers(Error* error));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockWiMaxManagerProxy);
};

}  // namespace shill

#endif  // SHILL_WIMAX_MOCK_WIMAX_MANAGER_PROXY_H_
