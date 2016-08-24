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

#ifndef APMANAGER_CONTROL_INTERFACE_H_
#define APMANAGER_CONTROL_INTERFACE_H_

#include <base/callback.h>
#include <base/macros.h>

#include "apmanager/config_adaptor_interface.h"
#include "apmanager/device_adaptor_interface.h"
#include "apmanager/firewall_proxy_interface.h"
#include "apmanager/manager_adaptor_interface.h"
#include "apmanager/service_adaptor_interface.h"
#include "apmanager/shill_proxy_interface.h"

namespace apmanager {

class Config;
class Device;
class Manager;
class Service;

// This is the Interface for an object factory that creates adaptor/proxy
// objects
class ControlInterface {
 public:
  virtual ~ControlInterface() {}

  virtual void Init() = 0;
  virtual void Shutdown() = 0;

  // Adaptor creation APIs.
  virtual std::unique_ptr<ConfigAdaptorInterface> CreateConfigAdaptor(
      Config* config, int service_identifier) = 0;
  virtual std::unique_ptr<DeviceAdaptorInterface> CreateDeviceAdaptor(
      Device* device) = 0;
  virtual std::unique_ptr<ManagerAdaptorInterface> CreateManagerAdaptor(
      Manager* manager) = 0;
  virtual std::unique_ptr<ServiceAdaptorInterface> CreateServiceAdaptor(
      Service* service) = 0;

  // Proxy creation APIs.
  virtual std::unique_ptr<FirewallProxyInterface> CreateFirewallProxy(
      const base::Closure& service_appeared_callback,
      const base::Closure& service_vanished_callback) = 0;
  virtual std::unique_ptr<ShillProxyInterface> CreateShillProxy(
      const base::Closure& service_appeared_callback,
      const base::Closure& service_vanished_callback) = 0;
};

}  // namespace apmanager

#endif  // APMANAGER_CONTROL_INTERFACE_H_
