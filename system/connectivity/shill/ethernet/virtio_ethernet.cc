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

#include "shill/ethernet/virtio_ethernet.h"

#include <unistd.h>

#include <string>

#include "shill/control_interface.h"
#include "shill/event_dispatcher.h"
#include "shill/logging.h"
#include "shill/manager.h"

using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kEthernet;
static string ObjectID(VirtioEthernet* v) { return v->GetRpcIdentifier(); }
}

VirtioEthernet::VirtioEthernet(ControlInterface* control_interface,
                               EventDispatcher* dispatcher,
                               Metrics* metrics,
                               Manager* manager,
                               const string& link_name,
                               const string& address,
                               int interface_index)
    : Ethernet(control_interface,
               dispatcher,
               metrics,
               manager,
               link_name,
               address,
               interface_index) {
  SLOG(this, 2) << "VirtioEthernet device " << link_name << " initialized.";
}

VirtioEthernet::~VirtioEthernet() {
  // Nothing to be done beyond what Ethernet dtor does.
}

void VirtioEthernet::Start(Error* error,
                           const EnabledStateChangedCallback& callback) {
  // We are sometimes instantiated (by DeviceInfo) before the Linux kernel
  // has completed the setup function for the device (virtio_net:virtnet_probe).
  //
  // Furthermore, setting the IFF_UP flag on the device (as done in
  // Ethernet::Start) may cause the kernel IPv6 code to send packets even
  // though virtnet_probe has not completed.
  //
  // When that happens, the device gets stuck in a state where it cannot
  // transmit any frames. (See crbug.com/212041)
  //
  // To avoid this, we sleep to let the device setup function complete.
  SLOG(this, 2) << "Sleeping to let virtio initialize.";
  sleep(2);
  SLOG(this, 2) << "Starting virtio Ethernet.";
  Ethernet::Start(error, callback);
}

}  // namespace shill
