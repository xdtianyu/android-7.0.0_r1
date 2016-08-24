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
#ifndef PROXY_RPC_SERVER_H
#define PROXY_RPC_SERVER_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <iostream>

#include <base/logging.h>
#include <base/callback.h>

#include "XmlRpc.h"

#include "proxy_shill_wifi_client.h"

typedef const base::Callback<XmlRpc::XmlRpcValue(
    XmlRpc::XmlRpcValue, ProxyShillWifiClient*)> RpcServerMethodHandler;

class ProxyRpcServer;
class ProxyRpcServerMethod : public XmlRpc::XmlRpcServerMethod {
 public:
  ProxyRpcServerMethod(const std::string& method_name,
                       const RpcServerMethodHandler& handler,
                       ProxyShillWifiClient* shill_wifi_client,
                       ProxyRpcServer* server);
  // This is the function signature exposed by the XmlRpc++ library
  // that we depend on and hence the non-const references.
  void execute(XmlRpc::XmlRpcValue& params_in, XmlRpc::XmlRpcValue& value_out);
  std::string help(void);

 private:
  RpcServerMethodHandler handler_;
  // RPC server methods hold a copy of the raw pointer to the instance of
  // the |ShillWifiClient| owned by the RPC server.
  ProxyShillWifiClient* shill_wifi_client_;

  DISALLOW_COPY_AND_ASSIGN(ProxyRpcServerMethod);
};

class ProxyRpcServer : public XmlRpc::XmlRpcServer {
 public:
  ProxyRpcServer(int server_port,
                 std::unique_ptr<ProxyShillWifiClient> shill_wifi_client);
  void Run();
  void RegisterRpcMethod(const std::string& method_name,
                         const RpcServerMethodHandler& handler);

 private:
  int server_port_;
  // RPC server owns the only instance of the |ShillWifiClient| used.
  std::unique_ptr<ProxyShillWifiClient> shill_wifi_client_;
  // Instances of the various methods registered with the server.
  std::vector<std::unique_ptr<ProxyRpcServerMethod>> methods_;

  DISALLOW_COPY_AND_ASSIGN(ProxyRpcServer);
};

#endif // PROXY_RPC_SERVER_H
