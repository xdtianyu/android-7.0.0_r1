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

#include "shill/cellular/mock_cellular_service.h"

#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

using testing::ReturnRef;

namespace shill {

MockCellularService::MockCellularService(ModemInfo* modem_info,
                                         const CellularRefPtr& device)
    : CellularService(modem_info, device),
      default_activation_state_(kActivationStateUnknown) {
  ON_CALL(*this, activation_state())
      .WillByDefault(ReturnRef(default_activation_state_));
}

MockCellularService::~MockCellularService() {}

}  // namespace shill
