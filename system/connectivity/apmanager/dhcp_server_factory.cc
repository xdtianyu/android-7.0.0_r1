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

#include "apmanager/dhcp_server_factory.h"

namespace apmanager {

namespace {

base::LazyInstance<DHCPServerFactory> g_dhcp_server_factory
    = LAZY_INSTANCE_INITIALIZER;

}  // namespace

DHCPServerFactory::DHCPServerFactory() {}
DHCPServerFactory::~DHCPServerFactory() {}

DHCPServerFactory* DHCPServerFactory::GetInstance() {
  return g_dhcp_server_factory.Pointer();
}

DHCPServer* DHCPServerFactory::CreateDHCPServer(
    uint16_t server_addr_index, const std::string& interface_name) {
  return new DHCPServer(server_addr_index, interface_name);
}

}  // namespace apmanager
