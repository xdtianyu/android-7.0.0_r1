//
// Copyright (C) 2014 The Android Open Source Project
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

#ifndef APMANAGER_MANAGER_H_
#define APMANAGER_MANAGER_H_

#include <string>
#include <vector>

#include <base/macros.h>

#include "apmanager/device_info.h"
#include "apmanager/firewall_manager.h"
#include "apmanager/manager_adaptor_interface.h"
#include "apmanager/service.h"
#include "apmanager/shill_manager.h"

namespace apmanager {

class ControlInterface;

class Manager {
 public:
  explicit Manager(ControlInterface* control_interface);
  virtual ~Manager();

  // Register this object to the RPC interface asynchronously.
  void RegisterAsync(const base::Callback<void(bool)>& completion_callback);

  // Create and return a new Service instance. The newly created instance
  // will be added to the service list, it will only get deleted via
  // RemoveService.
  scoped_refptr<Service> CreateService();

  // Remove |service| from the service list. Return true if service is found
  // and deleted from the list, false otherwise. |error| will be populated
  // on failure.
  bool RemoveService(const scoped_refptr<Service>& service, Error* error);

  virtual void Start();
  virtual void Stop();

  virtual void RegisterDevice(const scoped_refptr<Device>& device);

  // Return an unuse device with AP interface mode support.
  virtual scoped_refptr<Device> GetAvailableDevice();

  // Return the device that's associated with the given interface
  // |interface_name|.
  virtual scoped_refptr<Device> GetDeviceFromInterfaceName(
      const std::string& interface_name);

  // Claim the given interface |interface_name| from shill.
  virtual void ClaimInterface(const std::string& interface_name);
  // Release the given interface |interface_name| to shill.
  virtual void ReleaseInterface(const std::string& interface_name);
#if defined(__BRILLO__)
  // Setup an AP mode interface. Returns true and sets |interface_name|
  // on success, false otherwise.
  virtual bool SetupApModeInterface(std::string* interface_name);
  // Setup a station mode interface. Returns true and sets |interface_name|
  // on success, false otherwise.
  virtual bool SetupStationModeInterface(std::string* interface_name);
#endif  // __BRILLO__

  // Request/release access to DHCP port for the specified interface.
  virtual void RequestDHCPPortAccess(const std::string& interface);
  virtual void ReleaseDHCPPortAccess(const std::string& interface);

  ControlInterface* control_interface() const { return control_interface_; }

 private:
  friend class ManagerTest;

  ControlInterface* control_interface_;
  int service_identifier_;
  std::vector<scoped_refptr<Device>> devices_;
  DeviceInfo device_info_;

  // Manager for communicating with shill (connection manager).
  ShillManager shill_manager_;
  // Manager for communicating with remote firewall service.
  FirewallManager firewall_manager_;

  // Put the service list after ShillManager and FirewallManager, since both
  // are needed for tearing down an active/running Service.
  std::vector<scoped_refptr<Service>> services_;

  std::unique_ptr<ManagerAdaptorInterface> adaptor_;

  DISALLOW_COPY_AND_ASSIGN(Manager);
};

}  // namespace apmanager

#endif  // APMANAGER_MANAGER_H_
