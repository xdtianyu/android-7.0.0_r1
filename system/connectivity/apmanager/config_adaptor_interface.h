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

#ifndef APMANAGER_CONFIG_ADAPTOR_INTERFACE_H_
#define APMANAGER_CONFIG_ADAPTOR_INTERFACE_H_

#include <string>

#include "apmanager/rpc_interface.h"

namespace apmanager {

class ConfigAdaptorInterface {
 public:
  virtual ~ConfigAdaptorInterface() {}

  // Returns an identifier/handle that represents this object over
  // the IPC interface (e.g. dbus::ObjectPath for D-Bus, IBinder
  // for Binder).
  virtual RPCObjectIdentifier GetRpcObjectIdentifier() = 0;

  // Getter/setter for configuration properties.
  virtual void SetSsid(const std::string& ssid) = 0;
  virtual std::string GetSsid() = 0;
  virtual void SetInterfaceName(const std::string& interface_name) = 0;
  virtual std::string GetInterfaceName() = 0;
  virtual void SetSecurityMode(const std::string& security_mode) = 0;
  virtual std::string GetSecurityMode() = 0;
  virtual void SetPassphrase(const std::string& passphrase) = 0;
  virtual std::string GetPassphrase() = 0;
  virtual void SetHwMode(const std::string& hw_mode) = 0;
  virtual std::string GetHwMode() = 0;
  virtual void SetOperationMode(const std::string& op_mode) = 0;
  virtual std::string GetOperationMode() = 0;
  virtual void SetChannel(uint16_t channel) = 0;
  virtual uint16_t GetChannel() = 0;
  virtual void SetHiddenNetwork(bool hidden) = 0;
  virtual bool GetHiddenNetwork() = 0;
  virtual void SetBridgeInterface(const std::string& interface_name) = 0;
  virtual std::string GetBridgeInterface() = 0;
  virtual void SetServerAddressIndex(uint16_t) = 0;
  virtual uint16_t GetServerAddressIndex() = 0;
  virtual void SetFullDeviceControl(bool full_control) = 0;
  virtual bool GetFullDeviceControl() = 0;
};

}  // namespace apmanager

#endif  // APMANAGER_CONFIG_ADAPTOR_INTERFACE_H_
