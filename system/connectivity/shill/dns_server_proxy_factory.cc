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

#include "shill/dns_server_proxy_factory.h"

#include "shill/dns_server_proxy.h"

namespace shill {

namespace {

base::LazyInstance<DNSServerProxyFactory> g_dns_server_proxy_factory
    = LAZY_INSTANCE_INITIALIZER;

}  // namespace

DNSServerProxyFactory::DNSServerProxyFactory() {}
DNSServerProxyFactory::~DNSServerProxyFactory() {}

DNSServerProxyFactory* DNSServerProxyFactory::GetInstance() {
  return g_dns_server_proxy_factory.Pointer();
}

DNSServerProxy* DNSServerProxyFactory::CreateDNSServerProxy(
    const std::vector<std::string>& dns_servers) {
  return new DNSServerProxy(dns_servers);
}

}  // namespace shill
