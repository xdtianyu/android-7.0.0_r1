//
// Copyright (C) 2013 The Android Open Source Project
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

#ifndef SHILL_DNS_CLIENT_FACTORY_H_
#define SHILL_DNS_CLIENT_FACTORY_H_

#include <string>
#include <vector>

#include <base/lazy_instance.h>

#include "shill/dns_client.h"
#include "shill/event_dispatcher.h"
#include "shill/net/ip_address.h"

namespace shill {

class DNSClientFactory {
 public:
  virtual ~DNSClientFactory();

  // This is a singleton. Use DNSClientFactory::GetInstance()->Foo().
  static DNSClientFactory* GetInstance();

  virtual DNSClient* CreateDNSClient(
      IPAddress::Family family,
      const std::string& interface_name,
      const std::vector<std::string>& dns_servers,
      int timeout_ms,
      EventDispatcher* dispatcher,
      const DNSClient::ClientCallback& callback);

 protected:
  DNSClientFactory();

 private:
  friend struct base::DefaultLazyInstanceTraits<DNSClientFactory>;

  DISALLOW_COPY_AND_ASSIGN(DNSClientFactory);
};

}  // namespace shill

#endif  // SHILL_DNS_CLIENT_FACTORY_H_
