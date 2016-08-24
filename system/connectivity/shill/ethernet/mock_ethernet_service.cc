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

#include "shill/ethernet/mock_ethernet.h"
#include "shill/ethernet/mock_ethernet_service.h"

#include "shill/ethernet/ethernet.h"  // Needed to pass an EthernetRefPtr.

namespace shill {

MockEthernetService::MockEthernetService(ControlInterface* control_interface,
                                         Metrics* metrics,
                                         base::WeakPtr<Ethernet> ethernet)
    : EthernetService(control_interface, nullptr, metrics, nullptr,
                      ethernet) {}

MockEthernetService::~MockEthernetService() {}

}  // namespace shill
