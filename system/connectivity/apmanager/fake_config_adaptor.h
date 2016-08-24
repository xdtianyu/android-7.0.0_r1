//
// Copyright 2015 The Android Open Source Project
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

#ifndef APMANAGER_FAKE_CONFIG_ADAPTOR_H_
#define APMANAGER_FAKE_CONFIG_ADAPTOR_H_

#include <string>

#include <base/macros.h>

#include "apmanager/config_adaptor_interface.h"

namespace apmanager {

class FakeConfigAdaptor : public ConfigAdaptorInterface {
 public:
  FakeConfigAdaptor();
  ~FakeConfigAdaptor() override;

  RPCObjectIdentifier GetRpcObjectIdentifier() override;
  void SetSsid(const std::string& ssid) override;
  std::string GetSsid() override;
  void SetInterfaceName(const std::string& interface_name) override;
  std::string GetInterfaceName() override;
  void SetSecurityMode(const std::string& security_mode) override;
  std::string GetSecurityMode() override;
  void SetPassphrase(const std::string& passphrase) override;
  std::string GetPassphrase() override;
  void SetHwMode(const std::string& hw_mode) override;
  std::string GetHwMode() override;
  void SetOperationMode(const std::string& op_mode) override;
  std::string GetOperationMode() override;
  void SetChannel(uint16_t channel) override;
  uint16_t GetChannel() override;
  void SetHiddenNetwork(bool hidden) override;
  bool GetHiddenNetwork() override;
  void SetBridgeInterface(const std::string& interface_name) override;
  std::string GetBridgeInterface() override;
  void SetServerAddressIndex(uint16_t) override;
  uint16_t GetServerAddressIndex() override;
  void SetFullDeviceControl(bool full_control) override;
  bool GetFullDeviceControl() override;

 private:
  std::string ssid_;
  std::string interface_name_;
  std::string security_mode_;
  std::string passphrase_;
  std::string hw_mode_;
  std::string op_mode_;
  std::string bridge_interface_;
  bool hidden_network_;
  bool full_device_control_;
  uint16_t channel_;
  uint16_t server_address_index_;

  DISALLOW_COPY_AND_ASSIGN(FakeConfigAdaptor);
};

}  // namespace apmanager

#endif  // APMANAGER_FAKE_CONFIG_ADAPTOR_H_
