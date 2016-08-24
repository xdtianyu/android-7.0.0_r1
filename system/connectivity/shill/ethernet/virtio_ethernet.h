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

#ifndef SHILL_ETHERNET_VIRTIO_ETHERNET_H_
#define SHILL_ETHERNET_VIRTIO_ETHERNET_H_

#include <string>

#include "shill/ethernet/ethernet.h"

namespace shill {

class VirtioEthernet : public Ethernet {
 public:
  VirtioEthernet(ControlInterface* control_interface,
                 EventDispatcher* dispatcher,
                 Metrics* metrics,
                 Manager* manager,
                 const std::string& link_name,
                 const std::string& address,
                 int interface_index);
  ~VirtioEthernet() override;

  void Start(Error* error,
             const EnabledStateChangedCallback& callback) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(VirtioEthernet);
};

}  // namespace shill

#endif  // SHILL_ETHERNET_VIRTIO_ETHERNET_H_
