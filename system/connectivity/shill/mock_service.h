//
// Copyright (C) 2012 The Android Open Source Project
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

#ifndef SHILL_MOCK_SERVICE_H_
#define SHILL_MOCK_SERVICE_H_

#include <string>

#include <base/memory/ref_counted.h>
#include <gmock/gmock.h>

#include "shill/connection.h"
#include "shill/refptr_types.h"
#include "shill/service.h"
#include "shill/technology.h"

namespace shill {

class MockService : public Service {
 public:
  // A constructor for the Service object
  MockService(ControlInterface* control_interface,
              EventDispatcher* dispatcher,
              Metrics* metrics,
              Manager* manager);
  ~MockService() override;

  MOCK_METHOD0(AutoConnect, void());
  MOCK_METHOD2(Connect, void(Error* error, const char* reason));
  MOCK_METHOD2(Disconnect, void(Error* error, const char* reason));
  MOCK_METHOD3(DisconnectWithFailure, void(Service::ConnectFailure failure,
                                           Error* error,
                                           const char* reason));
  MOCK_METHOD1(UserInitiatedDisconnect, void(Error* error));
  MOCK_METHOD1(CalculateState, std::string(Error* error));
  MOCK_CONST_METHOD0(state, ConnectState());
  MOCK_METHOD1(SetState, void(ConnectState state));
  MOCK_METHOD2(SetPortalDetectionFailure, void(const std::string& phase,
                                               const std::string& status));
  MOCK_CONST_METHOD0(IsConnected, bool());
  MOCK_CONST_METHOD0(IsConnecting, bool());
  MOCK_CONST_METHOD1(IsDependentOn, bool(const ServiceRefPtr& b));
  MOCK_CONST_METHOD0(IsFailed, bool());
  MOCK_CONST_METHOD0(IsOnline, bool());
  MOCK_CONST_METHOD0(IsVisible, bool());
  MOCK_METHOD1(SetFailure, void(ConnectFailure failure));
  MOCK_CONST_METHOD0(failure, ConnectFailure());
  MOCK_CONST_METHOD1(GetDeviceRpcId, std::string(Error* error));
  MOCK_CONST_METHOD0(GetInnerDeviceRpcIdentifier, std::string());
  MOCK_CONST_METHOD0(GetRpcIdentifier, std::string());
  MOCK_CONST_METHOD0(GetStorageIdentifier, std::string());
  MOCK_CONST_METHOD1(GetLoadableStorageIdentifier,
                     std::string(const StoreInterface& store_interface));
  MOCK_METHOD1(Load, bool(StoreInterface* store_interface));
  MOCK_METHOD0(Unload, bool());
  MOCK_METHOD1(Save, bool(StoreInterface* store_interface));
  MOCK_METHOD2(Configure, void(const KeyValueStore& args, Error* error));
  MOCK_CONST_METHOD1(DoPropertiesMatch, bool(const KeyValueStore& args));
  MOCK_CONST_METHOD0(Is8021xConnectable, bool());
  MOCK_CONST_METHOD0(HasStaticNameServers, bool());
  MOCK_CONST_METHOD0(IsPortalDetectionDisabled, bool());
  MOCK_CONST_METHOD0(IsPortalDetectionAuto, bool());
  MOCK_CONST_METHOD0(IsRemembered, bool());
  MOCK_CONST_METHOD0(HasProxyConfig, bool());
  MOCK_METHOD1(SetConnection, void(const ConnectionRefPtr& connection));
  MOCK_CONST_METHOD0(connection, const ConnectionRefPtr&());
  MOCK_CONST_METHOD0(explicitly_disconnected, bool());
#if !defined(DISABLE_WIFI) || !defined(DISABLE_WIRED_8021X)
  MOCK_CONST_METHOD0(eap, const EapCredentials*());
#endif  // DISABLE_WIFI || DISABLE_WIRED_8021X
  MOCK_CONST_METHOD0(technology, Technology::Identifier());
  MOCK_METHOD1(OnPropertyChanged, void(const std::string& property));
  MOCK_METHOD0(ClearExplicitlyDisconnected, void());
  MOCK_CONST_METHOD0(is_dns_auto_fallback_allowed, bool());
  MOCK_METHOD0(NotifyIPConfigChanges, void());
  MOCK_CONST_METHOD0(link_monitor_disabled, bool());
  MOCK_METHOD0(EnableAndRetainAutoConnect, void());

  // Set a string for this Service via |store|.  Can be wired to Save() for
  // test purposes.
  bool FauxSave(StoreInterface* store);
  // Sets the connection reference returned by default when connection()
  // is called.
  void set_mock_connection(const ConnectionRefPtr& connection) {
    mock_connection_ = connection;
  }
  const std::string& friendly_name() const { return Service::friendly_name(); }

 private:
  ConnectionRefPtr mock_connection_;
  DISALLOW_COPY_AND_ASSIGN(MockService);
};

}  // namespace shill

#endif  // SHILL_MOCK_SERVICE_H_
