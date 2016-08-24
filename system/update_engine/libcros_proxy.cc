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

#include "update_engine/libcros_proxy.h"

using org::chromium::LibCrosServiceInterfaceProxy;
using org::chromium::LibCrosServiceInterfaceProxyInterface;
using org::chromium::UpdateEngineLibcrosProxyResolvedInterfaceProxy;
using org::chromium::UpdateEngineLibcrosProxyResolvedInterfaceProxyInterface;

namespace {
const char kLibCrosServiceName[] = "org.chromium.LibCrosService";
}  // namespace

namespace chromeos_update_engine {

LibCrosProxy::LibCrosProxy(
    std::unique_ptr<LibCrosServiceInterfaceProxyInterface>
        service_interface_proxy,
    std::unique_ptr<UpdateEngineLibcrosProxyResolvedInterfaceProxyInterface>
        ue_proxy_resolved_interface)
    : service_interface_proxy_(std::move(service_interface_proxy)),
      ue_proxy_resolved_interface_(std::move(ue_proxy_resolved_interface)) {
}

LibCrosProxy::LibCrosProxy(const scoped_refptr<dbus::Bus>& bus)
    : service_interface_proxy_(
          new LibCrosServiceInterfaceProxy(bus, kLibCrosServiceName)),
      ue_proxy_resolved_interface_(
          new UpdateEngineLibcrosProxyResolvedInterfaceProxy(
              bus,
              kLibCrosServiceName)) {
}

LibCrosServiceInterfaceProxyInterface* LibCrosProxy::service_interface_proxy() {
  return service_interface_proxy_.get();
}

UpdateEngineLibcrosProxyResolvedInterfaceProxyInterface*
LibCrosProxy::ue_proxy_resolved_interface() {
  return ue_proxy_resolved_interface_.get();
}

}  // namespace chromeos_update_engine
