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

#include "dhcp_client/device_info.h"

#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>

#include <memory>
#include <string>

#include <base/logging.h>

using shill::ByteString;
using shill::Sockets;
using shill::RTNLHandler;
using std::unique_ptr;

namespace {

base::LazyInstance<dhcp_client::DeviceInfo> g_dhcp_device_info
    = LAZY_INSTANCE_INITIALIZER;

}  // namespace

namespace dhcp_client {

DeviceInfo::DeviceInfo()
    : sockets_(new Sockets()),
      rtnl_handler_(RTNLHandler::GetInstance()) {
}

DeviceInfo::~DeviceInfo() {}

DeviceInfo* DeviceInfo::GetInstance() {
  return g_dhcp_device_info.Pointer();
}

bool DeviceInfo::GetDeviceInfo(const std::string& interface_name,
                               ByteString* mac_address,
                               unsigned int* interface_index ) {
  struct ifreq ifr;
  size_t if_name_len = interface_name.size();
  if (if_name_len > IFNAMSIZ) {
    LOG(ERROR) << "Interface name is too long.";
    return false;
  }
  memcpy(ifr.ifr_name, interface_name.c_str(), if_name_len);
  ifr.ifr_name[if_name_len] = 0;
  int fd = sockets_->Socket(AF_INET, SOCK_DGRAM, 0);
  if (fd == -1) {
    PLOG(ERROR) << "Failed to create socket.";
    return false;
  }

  shill::ScopedSocketCloser socket_closer(sockets_.get(), fd);
  // Get interface hardware address
  if (sockets_->Ioctl(fd, SIOCGIFHWADDR, &ifr) == -1) {
    PLOG(ERROR) << "Failed to get interface hardware address.";
    return false;
  }
  int if_index = rtnl_handler_->GetInterfaceIndex(interface_name);
  if (if_index == -1) {
    LOG(ERROR) << "Unable to get interface index.";
    return false;
  }
  *interface_index = if_index;
  *mac_address = ByteString(ifr.ifr_hwaddr.sa_data, IFHWADDRLEN);

  return true;
}

}  // namespace dhcp_client

