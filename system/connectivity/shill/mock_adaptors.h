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

#ifndef SHILL_MOCK_ADAPTORS_H_
#define SHILL_MOCK_ADAPTORS_H_

#include <string>
#include <vector>

#include <gmock/gmock.h>

#include "shill/adaptor_interfaces.h"
#include "shill/error.h"

namespace shill {

// These are the functions that a Device adaptor must support
class DeviceMockAdaptor : public DeviceAdaptorInterface {
 public:
  static const char kRpcId[];
  static const char kRpcConnId[];

  DeviceMockAdaptor();
  ~DeviceMockAdaptor() override;
  const std::string& GetRpcIdentifier() override;

  MOCK_METHOD2(EmitBoolChanged, void(const std::string& name, bool value));
  MOCK_METHOD2(EmitUintChanged, void(const std::string& name, uint32_t value));
  MOCK_METHOD2(EmitUint16Changed,
               void(const std::string& name, uint16_t value));
  MOCK_METHOD2(EmitIntChanged, void(const std::string& name, int value));
  MOCK_METHOD2(EmitStringChanged, void(const std::string& name,
                                       const std::string& value));
  MOCK_METHOD2(EmitStringmapChanged, void(const std::string& name,
                                          const Stringmap& value));
  MOCK_METHOD2(EmitStringmapsChanged, void(const std::string& name,
                                           const Stringmaps& value));
  MOCK_METHOD2(EmitStringsChanged, void(const std::string& name,
                                        const Strings& value));
  MOCK_METHOD2(EmitKeyValueStoreChanged, void(const std::string& name,
                                              const KeyValueStore& value));
  MOCK_METHOD2(EmitRpcIdentifierChanged,
               void(const std::string& name,
                    const std::string& value));
  MOCK_METHOD2(EmitRpcIdentifierArrayChanged,
               void(const std::string& name,
                    const std::vector<std::string>& value));

 private:
  const std::string rpc_id_;
  const std::string rpc_conn_id_;
};

// These are the functions that a IPConfig adaptor must support
class IPConfigMockAdaptor : public IPConfigAdaptorInterface {
 public:
  static const char kRpcId[];

  IPConfigMockAdaptor();
  ~IPConfigMockAdaptor() override;
  const std::string& GetRpcIdentifier() override;

  MOCK_METHOD2(EmitBoolChanged, void(const std::string&, bool));
  MOCK_METHOD2(EmitUintChanged, void(const std::string&, uint32_t));
  MOCK_METHOD2(EmitIntChanged, void(const std::string&, int));
  MOCK_METHOD2(EmitStringChanged, void(const std::string&, const std::string&));
  MOCK_METHOD2(EmitStringsChanged,
               void(const std::string&, const std::vector<std::string>&));

 private:
  const std::string rpc_id_;
};

// These are the functions that a Manager adaptor must support
class ManagerMockAdaptor : public ManagerAdaptorInterface {
 public:
  static const char kRpcId[];

  ManagerMockAdaptor();
  ~ManagerMockAdaptor() override;
  const std::string& GetRpcIdentifier() override;

  MOCK_METHOD1(RegisterAsync,
               void(const base::Callback<void(bool)>& completion_callback));
  MOCK_METHOD2(EmitBoolChanged, void(const std::string&, bool));
  MOCK_METHOD2(EmitUintChanged, void(const std::string&, uint32_t));
  MOCK_METHOD2(EmitIntChanged, void(const std::string&, int));
  MOCK_METHOD2(EmitStringChanged, void(const std::string&, const std::string&));
  MOCK_METHOD2(EmitStringsChanged,
               void(const std::string&, const std::vector<std::string>&));
  MOCK_METHOD2(EmitRpcIdentifierChanged,
               void(const std::string&, const std::string&));
  MOCK_METHOD2(EmitRpcIdentifierArrayChanged,
               void(const std::string&, const std::vector<std::string>&));

 private:
  const std::string rpc_id_;
};

// These are the functions that a Profile adaptor must support
class ProfileMockAdaptor : public ProfileAdaptorInterface {
 public:
  static const char kRpcId[];

  ProfileMockAdaptor();
  ~ProfileMockAdaptor() override;
  const std::string& GetRpcIdentifier() override;

  MOCK_METHOD2(EmitBoolChanged, void(const std::string&, bool));
  MOCK_METHOD2(EmitUintChanged, void(const std::string&, uint32_t));
  MOCK_METHOD2(EmitIntChanged, void(const std::string&, int));
  MOCK_METHOD2(EmitStringChanged, void(const std::string&, const std::string&));

 private:
  const std::string rpc_id_;
};

// These are the functions that a Task adaptor must support
class RPCTaskMockAdaptor : public RPCTaskAdaptorInterface {
 public:
  static const char kRpcId[];
  static const char kRpcInterfaceId[];
  static const char kRpcConnId[];

  RPCTaskMockAdaptor();
  ~RPCTaskMockAdaptor() override;

  const std::string& GetRpcIdentifier() override;
  const std::string& GetRpcConnectionIdentifier() override;

 private:
  const std::string rpc_id_;
  const std::string rpc_interface_id_;
  const std::string rpc_conn_id_;
};

// These are the functions that a Service adaptor must support
class ServiceMockAdaptor : public ServiceAdaptorInterface {
 public:
  static const char kRpcId[];

  ServiceMockAdaptor();
  ~ServiceMockAdaptor() override;
  const std::string& GetRpcIdentifier() override;

  MOCK_METHOD2(EmitBoolChanged, void(const std::string& name, bool value));
  MOCK_METHOD2(EmitUint8Changed, void(const std::string& name, uint8_t value));
  MOCK_METHOD2(EmitUint16Changed,
               void(const std::string& name, uint16_t value));
  MOCK_METHOD2(EmitUint16sChanged, void(const std::string& name,
                                        const Uint16s& value));
  MOCK_METHOD2(EmitUintChanged, void(const std::string& name, uint32_t value));
  MOCK_METHOD2(EmitIntChanged, void(const std::string& name, int value));
  MOCK_METHOD2(EmitRpcIdentifierChanged,
               void(const std::string& name, const std::string& value));
  MOCK_METHOD2(EmitStringChanged,
               void(const std::string& name, const std::string& value));
  MOCK_METHOD2(EmitStringmapChanged,
               void(const std::string& name, const Stringmap& value));

 private:
  const std::string rpc_id_;
};

#ifndef DISABLE_VPN
class ThirdPartyVpnMockAdaptor : public ThirdPartyVpnAdaptorInterface {
 public:
  ThirdPartyVpnMockAdaptor();
  ~ThirdPartyVpnMockAdaptor() override;

  MOCK_METHOD1(EmitPacketReceived, void(const std::vector<uint8_t>& packet));

  MOCK_METHOD1(EmitPlatformMessage, void(uint32_t message));
};
#endif

}  // namespace shill

#endif  // SHILL_MOCK_ADAPTORS_H_
