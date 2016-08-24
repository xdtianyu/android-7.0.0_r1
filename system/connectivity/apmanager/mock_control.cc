//
// Copyright 2015 The Android Open Source Project
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

#include "apmanager/mock_control.h"

namespace apmanager {

MockControl::MockControl() {}

MockControl::~MockControl() {}

std::unique_ptr<ConfigAdaptorInterface> MockControl::CreateConfigAdaptor(
      Config* /* config */, int /* service_identifier */) {
  return std::unique_ptr<ConfigAdaptorInterface>(CreateConfigAdaptorRaw());
}

std::unique_ptr<DeviceAdaptorInterface> MockControl::CreateDeviceAdaptor(
      Device* /* device */) {
  return std::unique_ptr<DeviceAdaptorInterface>(CreateDeviceAdaptorRaw());
}

std::unique_ptr<ManagerAdaptorInterface> MockControl::CreateManagerAdaptor(
      Manager* /* manager */) {
  return std::unique_ptr<ManagerAdaptorInterface>(CreateManagerAdaptorRaw());
}

std::unique_ptr<ServiceAdaptorInterface> MockControl::CreateServiceAdaptor(
      Service* /* service */) {
  return std::unique_ptr<ServiceAdaptorInterface>(CreateServiceAdaptorRaw());
}

std::unique_ptr<FirewallProxyInterface> MockControl::CreateFirewallProxy(
      const base::Closure& /* service_appeared_callback */,
      const base::Closure& /* service_vanished_callback */) {
  return std::unique_ptr<FirewallProxyInterface>(CreateFirewallProxyRaw());
}

std::unique_ptr<ShillProxyInterface> MockControl::CreateShillProxy(
      const base::Closure& /* service_appeared_callback */,
      const base::Closure& /* service_vanished_callback */) {
  return std::unique_ptr<ShillProxyInterface>(CreateShillProxyRaw());
}

}  // namespace apmanager
