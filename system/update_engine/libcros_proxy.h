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

#ifndef UPDATE_ENGINE_LIBCROS_PROXY_H_
#define UPDATE_ENGINE_LIBCROS_PROXY_H_

#include <memory>

#include <base/macros.h>
#include <dbus/bus.h>

#include "libcros/dbus-proxies.h"

namespace chromeos_update_engine {

// This class handles the DBus connection with chrome to resolve proxies. This
// is a thin class to just hold the generated proxies (real or mocked ones).
class LibCrosProxy final {
 public:
  explicit LibCrosProxy(const scoped_refptr<dbus::Bus>& bus);
  LibCrosProxy(
      std::unique_ptr<org::chromium::LibCrosServiceInterfaceProxyInterface>
          service_interface_proxy,
      std::unique_ptr<
          org::chromium::
              UpdateEngineLibcrosProxyResolvedInterfaceProxyInterface>
          ue_proxy_resolved_interface);

  ~LibCrosProxy() = default;

  // Getters for the two proxies.
  org::chromium::LibCrosServiceInterfaceProxyInterface*
  service_interface_proxy();
  org::chromium::UpdateEngineLibcrosProxyResolvedInterfaceProxyInterface*
  ue_proxy_resolved_interface();

 private:
  std::unique_ptr<org::chromium::LibCrosServiceInterfaceProxyInterface>
      service_interface_proxy_;
  std::unique_ptr<
      org::chromium::UpdateEngineLibcrosProxyResolvedInterfaceProxyInterface>
      ue_proxy_resolved_interface_;

  DISALLOW_COPY_AND_ASSIGN(LibCrosProxy);
};

}  // namespace chromeos_update_engine

#endif  // UPDATE_ENGINE_LIBCROS_PROXY_H_
