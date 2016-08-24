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

#ifndef SHILL_VIRTUAL_DEVICE_H_
#define SHILL_VIRTUAL_DEVICE_H_

#include <string>

#include <base/macros.h>

#include "shill/device.h"
#include "shill/ipconfig.h"
#include "shill/service.h"
#include "shill/technology.h"

namespace shill {

class StorageInterface;

// A VirtualDevice represents a device that doesn't provide its own
// physical layer. This includes, e.g., tunnel interfaces used for
// OpenVPN, and PPP devices used for L2TPIPSec and 3G PPP dongles.
// (PPP devices are represented via the PPPDevice subclass.)
class VirtualDevice : public Device {
 public:
  VirtualDevice(ControlInterface* control,
                EventDispatcher* dispatcher,
                Metrics* metrics,
                Manager* manager,
                const std::string& link_name,
                int interface_index,
                Technology::Identifier technology);
  ~VirtualDevice() override;

  bool Load(StoreInterface* storage) override;
  bool Save(StoreInterface* storage) override;

  void Start(Error* error,
             const EnabledStateChangedCallback& callback) override;
  void Stop(Error* error,
            const EnabledStateChangedCallback& callback) override;

  virtual void UpdateIPConfig(const IPConfig::Properties& properties);

  // Expose protected device methods to manager of this device.
  // (E.g. Cellular, L2TPIPSecDriver, OpenVPNDriver.)
  void DropConnection() override;
  virtual void SelectService(const ServiceRefPtr& service);
  void SetServiceState(Service::ConnectState state) override;
  void SetServiceFailure(Service::ConnectFailure failure_state) override;
  void SetServiceFailureSilent(Service::ConnectFailure failure_state) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(VirtualDevice);
};

}  // namespace shill

#endif  // SHILL_VIRTUAL_DEVICE_H_
