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

#ifndef SHILL_MOCK_DNS_CLIENT_FACTORY_H_
#define SHILL_MOCK_DNS_CLIENT_FACTORY_H_

#include <string>
#include <vector>

#include <base/lazy_instance.h>
#include <gmock/gmock.h>

#include "shill/dns_client_factory.h"
#include "shill/event_dispatcher.h"
#include "shill/net/ip_address.h"

namespace shill {

class MockDNSClientFactory : public DNSClientFactory {
 public:
  ~MockDNSClientFactory() override;

  // This is a singleton. Use MockDNSClientFactory::GetInstance()->Foo().
  static MockDNSClientFactory* GetInstance();

  MOCK_METHOD6(CreateDNSClient,
               DNSClient* (IPAddress::Family family,
                           const std::string& interface_name,
                           const std::vector<std::string>& dns_servers,
                           int timeout_ms,
                           EventDispatcher* dispatcher,
                           const DNSClient::ClientCallback& callback));

 protected:
  MockDNSClientFactory();

 private:
  friend struct base::DefaultLazyInstanceTraits<MockDNSClientFactory>;

  DISALLOW_COPY_AND_ASSIGN(MockDNSClientFactory);
};

}  // namespace shill

#endif  // SHILL_MOCK_DNS_CLIENT_FACTORY_H_
