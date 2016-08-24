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

#ifndef APMANAGER_MOCK_CONTROL_H_
#define APMANAGER_MOCK_CONTROL_H_

#include <base/macros.h>
#include <gmock/gmock.h>

#include "apmanager/control_interface.h"

namespace apmanager {

class MockControl : public ControlInterface {
 public:
  MockControl();
  ~MockControl() override;

  MOCK_METHOD0(Init, void());
  MOCK_METHOD0(Shutdown, void());

  // Provide mock methods for creating raw pointer for adaptor/proxy.
  // This allows us to set expectations for adaptor/proxy creation
  // functions, since mock methods only support copyable return values,
  // and unique_ptr is not copyable.
  MOCK_METHOD0(CreateConfigAdaptorRaw, ConfigAdaptorInterface*());
  MOCK_METHOD0(CreateDeviceAdaptorRaw, DeviceAdaptorInterface*());
  MOCK_METHOD0(CreateFirewallProxyRaw, FirewallProxyInterface*());
  MOCK_METHOD0(CreateManagerAdaptorRaw, ManagerAdaptorInterface*());
  MOCK_METHOD0(CreateServiceAdaptorRaw, ServiceAdaptorInterface*());
  MOCK_METHOD0(CreateShillProxyRaw, ShillProxyInterface*());

  // These functions use the mock methods above for creating
  // raw object.
  std::unique_ptr<ConfigAdaptorInterface> CreateConfigAdaptor(
      Config* config, int service_identifier) override;
  std::unique_ptr<DeviceAdaptorInterface> CreateDeviceAdaptor(
      Device* device) override;
  std::unique_ptr<ManagerAdaptorInterface> CreateManagerAdaptor(
      Manager* manager) override;
  std::unique_ptr<ServiceAdaptorInterface> CreateServiceAdaptor(
      Service* service) override;
  std::unique_ptr<FirewallProxyInterface> CreateFirewallProxy(
      const base::Closure& service_appeared_callback,
      const base::Closure& service_vanished_callback) override;
  std::unique_ptr<ShillProxyInterface> CreateShillProxy(
      const base::Closure& service_appeared_callback,
      const base::Closure& service_vanished_callback) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MockControl);
};

}  // namespace apmanager

#endif  // APMANAGER_MOCK_CONTROL_H_
