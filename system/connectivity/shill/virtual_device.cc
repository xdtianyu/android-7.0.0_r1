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

#include "shill/virtual_device.h"

#include <netinet/ether.h>
#include <linux/if.h>  // NOLINT - Needs definitions from netinet/ether.h

#include "shill/logging.h"
#include "shill/net/rtnl_handler.h"

using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kDevice;
static string ObjectID(VirtualDevice* v) { return "(virtual_device)"; }
}

namespace {
const char kHardwareAddressEmpty[] = "";
}  // namespace

VirtualDevice::VirtualDevice(ControlInterface* control,
                             EventDispatcher* dispatcher,
                             Metrics* metrics,
                             Manager* manager,
                             const string& link_name,
                             int interface_index,
                             Technology::Identifier technology)
    : Device(control, dispatcher, metrics, manager, link_name,
             kHardwareAddressEmpty, interface_index, technology) {}

VirtualDevice::~VirtualDevice() {}

bool VirtualDevice::Load(StoreInterface* /*storage*/) {
  // Virtual devices have no persistent state.
  return true;
}

bool VirtualDevice::Save(StoreInterface* /*storage*/) {
  // Virtual devices have no persistent state.
  return true;
}

void VirtualDevice::Start(Error* error,
                          const EnabledStateChangedCallback& /*callback*/) {
  rtnl_handler()->SetInterfaceFlags(interface_index(), IFF_UP, IFF_UP);
  // TODO(quiche): Should we call OnEnabledStateChanged, like other Devices?
  if (error)
    error->Reset();  // indicate immediate completion
}

void VirtualDevice::Stop(Error* error,
                         const EnabledStateChangedCallback& /*callback*/) {
  // TODO(quiche): Should we call OnEnabledStateChanged, like other Devices?
  if (error)
    error->Reset();  // indicate immediate completion
}

void VirtualDevice::UpdateIPConfig(const IPConfig::Properties& properties) {
  SLOG(this, 2) << __func__ << " on " << link_name();
  if (!ipconfig()) {
    set_ipconfig(new IPConfig(control_interface(), link_name()));
  }
  ipconfig()->set_properties(properties);
  OnIPConfigUpdated(ipconfig(), true);
}

void VirtualDevice::DropConnection() {
  Device::DropConnection();
}

void VirtualDevice::SelectService(const ServiceRefPtr& service) {
  Device::SelectService(service);
}

void VirtualDevice::SetServiceState(Service::ConnectState state) {
  Device::SetServiceState(state);
}

void VirtualDevice::SetServiceFailure(Service::ConnectFailure failure_state) {
  Device::SetServiceFailure(failure_state);
}

void VirtualDevice::SetServiceFailureSilent(
    Service::ConnectFailure failure_state) {
  Device::SetServiceFailureSilent(failure_state);
}

}  // namespace shill
