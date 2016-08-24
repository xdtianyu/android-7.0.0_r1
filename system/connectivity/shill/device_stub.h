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

#ifndef SHILL_DEVICE_STUB_H_
#define SHILL_DEVICE_STUB_H_

#include <base/memory/ref_counted.h>

#include <string>
#include <vector>

#include "shill/device.h"
#include "shill/event_dispatcher.h"
#include "shill/service.h"

namespace shill {

class ControlInterface;
class DeviceAdaptorInterface;
class EventDispatcher;
class Endpoint;
class DeviceInfo;
class Manager;
class Metrics;

// Non-functional Device subclass used for non-operable or blacklisted devices
class DeviceStub : public Device {
 public:
  DeviceStub(ControlInterface* control_interface,
             EventDispatcher* dispatcher,
             Metrics* metrics,
             Manager* manager,
             const std::string& link_name,
             const std::string& address,
             int interface_index,
             Technology::Identifier technology)
      : Device(control_interface, dispatcher, metrics, manager, link_name,
               address, interface_index, technology) {}
  void Start(Error* /*error*/,
             const EnabledStateChangedCallback& /*callback*/) override {}
  void Stop(Error* /*error*/,
            const EnabledStateChangedCallback& /*callback*/) override {}
  void Initialize() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(DeviceStub);
};

}  // namespace shill

#endif  // SHILL_DEVICE_STUB_H_
