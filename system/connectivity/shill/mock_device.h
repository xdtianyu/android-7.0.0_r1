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

#ifndef SHILL_MOCK_DEVICE_H_
#define SHILL_MOCK_DEVICE_H_

#include <string>
#include <vector>

#include <base/memory/ref_counted.h>
#include <gmock/gmock.h>

#include "shill/device.h"
#include "shill/geolocation_info.h"

namespace shill {

class MockDevice : public Device {
 public:
  MockDevice(ControlInterface* control_interface,
             EventDispatcher* dispatcher,
             Metrics* metrics,
             Manager* manager,
             const std::string& link_name,
             const std::string& address,
             int interface_index);
  ~MockDevice() override;

  MOCK_METHOD0(Initialize, void());
  MOCK_METHOD2(Start, void(Error* error,
                           const EnabledStateChangedCallback& callback));
  MOCK_METHOD2(Stop, void(Error* error,
                          const EnabledStateChangedCallback& callback));
  MOCK_METHOD1(SetEnabled, void(bool));
  MOCK_METHOD3(SetEnabledPersistent, void(bool enable,
                                          Error* error,
                                          const ResultCallback& callback));
  MOCK_METHOD3(SetEnabledNonPersistent, void(bool enable,
                                             Error* error,
                                             const ResultCallback& callback));
  MOCK_METHOD3(Scan, void(Device::ScanType scan_type, Error* error,
                          const std::string& reason));
  MOCK_METHOD1(Load, bool(StoreInterface* storage));
  MOCK_METHOD1(Save, bool(StoreInterface* storage));
  MOCK_METHOD0(DisableIPv6, void());
  MOCK_METHOD0(EnableIPv6, void());
  MOCK_METHOD0(EnableIPv6Privacy, void());
  MOCK_METHOD1(SetLooseRouting, void(bool));
  MOCK_METHOD1(SetIsMultiHomed, void(bool is_multi_homed));
  MOCK_METHOD0(RestartPortalDetection, bool());
  MOCK_METHOD0(RequestPortalDetection, bool());
  MOCK_METHOD0(GetReceiveByteCount, uint64_t());
  MOCK_METHOD0(GetTransmitByteCount, uint64_t());
  MOCK_CONST_METHOD1(IsConnectedToService, bool(const ServiceRefPtr& service));
  MOCK_CONST_METHOD0(technology, Technology::Identifier());
  MOCK_METHOD1(OnBeforeSuspend, void(const ResultCallback& callback));
  MOCK_METHOD1(OnDarkResume, void(const ResultCallback& callback));
  MOCK_METHOD0(OnAfterResume, void());
  MOCK_METHOD0(OnConnectionUpdated, void());
  MOCK_METHOD0(OnIPv6AddressChanged, void());
  MOCK_CONST_METHOD0(GetGeolocationObjects, std::vector<GeolocationInfo>());
  MOCK_METHOD0(OnIPv6DnsServerAddressesChanged, void());
  MOCK_METHOD0(StartConnectivityTest, bool());
  MOCK_CONST_METHOD0(connection, const ConnectionRefPtr&());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockDevice);
};

}  // namespace shill

#endif  // SHILL_MOCK_DEVICE_H_
