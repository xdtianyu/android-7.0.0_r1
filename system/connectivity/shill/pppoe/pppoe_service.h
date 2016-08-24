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

#ifndef SHILL_PPPOE_PPPOE_SERVICE_H_
#define SHILL_PPPOE_PPPOE_SERVICE_H_

#include <map>
#include <string>

#include <gtest/gtest_prod.h>

#include "shill/ethernet/ethernet.h"
#include "shill/ethernet/ethernet_service.h"
#include "shill/refptr_types.h"
#include "shill/rpc_task.h"

namespace shill {

class ControlInterface;
class Error;
class EventDispatcher;
class ExternalTask;
class Manager;
class Metrics;
class PPPDeviceFactory;
class ProcessManager;
class StoreInterface;

// PPPoEService is an EthernetService that manages PPPoE connectivity on a
// single Ethernet device.  To do this it spawns and manages pppd instances.
// When pppX interfaces are created in the course of a connection they are
// wrapped with a PPPDevice, and are made to SelectService the PPPoEService that
// created them.
class PPPoEService : public EthernetService, public RPCTaskDelegate {
 public:
  PPPoEService(ControlInterface* control_interface,
               EventDispatcher* dispatcher,
               Metrics* metrics,
               Manager* manager,
               base::WeakPtr<Ethernet> ethernet);
  ~PPPoEService() override;

  // Inherited from EthernetService.
  void Connect(Error* error, const char* reason) override;
  void Disconnect(Error* error, const char* reason) override;
  bool Load(StoreInterface* storage) override;
  bool Save(StoreInterface* storage) override;
  bool Unload() override;

  // Inherited from Service.
  std::string GetInnerDeviceRpcIdentifier() const override;

  // Inherited from RPCTaskDelegate.
  void GetLogin(std::string* user, std::string* password) override;
  void Notify(const std::string& reason,
              const std::map<std::string, std::string>& dict) override;

 private:
  friend class PPPoEServiceTest;
  FRIEND_TEST(PPPoEServiceTest, Disconnect);
  FRIEND_TEST(PPPoEServiceTest, OnPPPConnected);

  static const int kDefaultLCPEchoInterval;
  static const int kDefaultLCPEchoFailure;
  static const int kDefaultMaxAuthFailure;

  void OnPPPAuthenticating();
  void OnPPPAuthenticated();
  void OnPPPConnected(const std::map<std::string, std::string>& params);
  void OnPPPDisconnected();
  void OnPPPDied(pid_t pid, int exit);

  ControlInterface* control_interface_;
  PPPDeviceFactory* ppp_device_factory_;
  ProcessManager* process_manager_;

  std::string username_;
  std::string password_;
  int lcp_echo_interval_;
  int lcp_echo_failure_;
  int max_auth_failure_;

  bool authenticating_;
  std::unique_ptr<ExternalTask> pppd_;
  PPPDeviceRefPtr ppp_device_;

  base::WeakPtrFactory<PPPoEService> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PPPoEService);
};

}  // namespace shill

#endif  // SHILL_PPPOE_PPPOE_SERVICE_H_
