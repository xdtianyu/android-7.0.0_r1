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

#ifndef DHCP_CLIENT_DEVICE_INFO_H_
#define DHCP_CLIENT_DEVICE_INFO_H_

#include <memory>
#include <string>

#include <base/lazy_instance.h>
#include <base/macros.h>

#include "shill/net/byte_string.h"
#include "shill/net/rtnl_handler.h"
#include "shill/net/sockets.h"

namespace dhcp_client {

class DeviceInfo {
 public:
  virtual ~DeviceInfo();
  static DeviceInfo* GetInstance();
  bool GetDeviceInfo(const std::string& interface_name,
                     shill::ByteString* mac_address,
                     unsigned int* interface_index);
 protected:
  DeviceInfo();

 private:
  friend class DeviceInfoTest;

  std::unique_ptr<shill::Sockets> sockets_;
  shill::RTNLHandler* rtnl_handler_;
  friend struct base::DefaultLazyInstanceTraits<DeviceInfo>;

  DISALLOW_COPY_AND_ASSIGN(DeviceInfo);
};

}  // namespace dhcp_client

#endif  // DHCP_CLIENT_DEVICE_INFO_H_
