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

#ifndef SHILL_DNS_SERVER_PROXY_H_
#define SHILL_DNS_SERVER_PROXY_H_

#include <string>
#include <vector>

#include <base/macros.h>
#include <base/memory/weak_ptr.h>

namespace shill {

class ProcessManager;

// This setup a DNS server proxy to handle/redirect local DNS requests.
// Proxy is setup using dnsmasq.
class DNSServerProxy {
 public:
  DNSServerProxy(const std::vector<std::string>& dns_servers);
  virtual ~DNSServerProxy();

  // Start dnsmasq process for serving local DNS requests.
  virtual bool Start();

 private:
  // Stop dnsmasq process.
  void Stop();

  // Invoked when dnsmasq process exits.
  void OnProcessExited(int exit_status);

  ProcessManager* process_manager_;
  int pid_;
  std::vector<std::string> dns_servers_;

  base::WeakPtrFactory<DNSServerProxy> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(DNSServerProxy);
};

}  // namespace shill

#endif  // SHILL_DNS_SERVER_PROXY_H_
