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

#ifndef SHILL_MOCK_DEVICE_INFO_H_
#define SHILL_MOCK_DEVICE_INFO_H_

#include <string>
#include <vector>

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/device_info.h"

namespace shill {

class ByteString;
class ControlInterface;
class EventDispatcher;
class IPAddress;
class Manager;
class Metrics;

class MockDeviceInfo : public DeviceInfo {
 public:
  MockDeviceInfo(ControlInterface* control_interface,
                 EventDispatcher* dispatcher,
                 Metrics* metrics,
                 Manager* manager);
  ~MockDeviceInfo() override;

  MOCK_METHOD1(IsDeviceBlackListed, bool(const std::string& device_name));
  MOCK_METHOD1(AddDeviceToBlackList, void(const std::string& device_name));
  MOCK_METHOD1(RemoveDeviceFromBlackList, void(const std::string& device_name));
  MOCK_CONST_METHOD1(GetDevice, DeviceRefPtr(int interface_index));
  MOCK_CONST_METHOD1(GetIndex, int(const std::string& interface_name));
  MOCK_CONST_METHOD2(GetMACAddress, bool(int interface_index,
                                         ByteString* address));
  MOCK_CONST_METHOD1(GetMACAddressFromKernel, ByteString(int interface_index));
  MOCK_CONST_METHOD3(GetMACAddressOfPeer,
                     bool(int interface_index,
                          const IPAddress& peer,
                          ByteString* address));
  MOCK_CONST_METHOD3(GetByteCounts, bool(int interface_index,
                                         uint64_t* rx_bytes,
                                         uint64_t* tx_bytes));
  MOCK_CONST_METHOD2(GetFlags, bool(int interface_index,
                                    unsigned int* flags));
  MOCK_CONST_METHOD2(GetAddresses, bool(int interface_index,
                                        std::vector<AddressData>* addresses));
  MOCK_CONST_METHOD1(FlushAddresses, void(int interface_index));
  MOCK_CONST_METHOD2(HasOtherAddress,
                     bool(int interface_index,
                          const IPAddress& excluded_address));
  MOCK_CONST_METHOD2(HasDirectConnectivityTo,
                     bool(int interface_index,
                          const IPAddress& address));
  MOCK_METHOD2(GetPrimaryIPv6Address,
               bool(int interface_index, IPAddress* address));
  MOCK_METHOD3(GetIPv6DnsServerAddresses,
               bool(int interface_index,
                    std::vector<IPAddress>* address_list,
                    uint32_t* life_time));
  MOCK_CONST_METHOD1(CreateTunnelInterface,  bool(std::string* interface_name));
  MOCK_CONST_METHOD1(OpenTunnelInterface,
                     int(const std::string& interface_name));
  MOCK_CONST_METHOD1(DeleteInterface, bool(int interface_index));
  MOCK_METHOD1(RegisterDevice, void(const DeviceRefPtr&));
  MOCK_METHOD1(DeregisterDevice, void(const DeviceRefPtr&));
  MOCK_CONST_METHOD1(SetHostname, bool(const std::string& hostname));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockDeviceInfo);
};

}  // namespace shill

#endif  // SHILL_MOCK_DEVICE_INFO_H_
