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

#include "shill/ppp_device_factory.h"

#include "shill/ppp_device.h"

using std::string;

namespace shill {

namespace {

base::LazyInstance<PPPDeviceFactory> g_ppp_device_factory
    = LAZY_INSTANCE_INITIALIZER;

}  // namespace

PPPDeviceFactory::PPPDeviceFactory() {}
PPPDeviceFactory::~PPPDeviceFactory() {}

PPPDeviceFactory* PPPDeviceFactory::GetInstance() {
  return g_ppp_device_factory.Pointer();
}

PPPDevice* PPPDeviceFactory::CreatePPPDevice(
    ControlInterface* control,
    EventDispatcher* dispatcher,
    Metrics* metrics,
    Manager* manager,
    const string& link_name,
    int interface_index) {
  return new PPPDevice(control, dispatcher, metrics, manager, link_name,
                       interface_index);
}

}  // namespace shill
