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

#ifndef SHILL_DNS_SERVER_PROXY_FACTORY_H_
#define SHILL_DNS_SERVER_PROXY_FACTORY_H_

#include <string>
#include <vector>

#include <base/lazy_instance.h>

namespace shill {

class DNSServerProxy;

class DNSServerProxyFactory {
 public:
  virtual ~DNSServerProxyFactory();

  // This is a singleton. Use DNSServerProxyFactory::GetInstance()->Foo().
  static DNSServerProxyFactory* GetInstance();

  virtual DNSServerProxy* CreateDNSServerProxy(
      const std::vector<std::string>& dns_servers);

 protected:
  DNSServerProxyFactory();

 private:
  friend struct base::DefaultLazyInstanceTraits<DNSServerProxyFactory>;

  DISALLOW_COPY_AND_ASSIGN(DNSServerProxyFactory);
};

}  // namespace shill

#endif  // SHILL_DNS_SERVER_PROXY_FACTORY_H_
