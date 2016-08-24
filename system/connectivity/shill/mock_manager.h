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

#ifndef SHILL_MOCK_MANAGER_H_
#define SHILL_MOCK_MANAGER_H_

#include <string>
#include <vector>

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/manager.h"

namespace shill {

class MockManager : public Manager {
 public:
  MockManager(ControlInterface* control_interface,
              EventDispatcher* dispatcher,
              Metrics* metrics);
  ~MockManager() override;

  MOCK_METHOD0(device_info, DeviceInfo*());
  MOCK_METHOD0(modem_info, ModemInfo*());
#if !defined(DISABLE_WIRED_8021X)
  MOCK_CONST_METHOD0(ethernet_eap_provider, EthernetEapProvider*());
#endif  // DISABLE_WIRED_8021X
  MOCK_METHOD0(wimax_provider, WiMaxProvider*());
  MOCK_METHOD0(mutable_store, PropertyStore*());
  MOCK_CONST_METHOD0(store, const PropertyStore&());
  MOCK_CONST_METHOD0(run_path, const base::FilePath&());
  MOCK_METHOD0(Start, void());
  MOCK_METHOD0(Stop, void());
  MOCK_METHOD3(SetProfileForService, void(const ServiceRefPtr& to_set,
                                          const std::string& profile,
                                          Error* error));
  MOCK_METHOD1(RegisterDevice, void(const DeviceRefPtr& to_manage));
  MOCK_METHOD1(DeregisterDevice, void(const DeviceRefPtr& to_forget));
  MOCK_METHOD1(HasService, bool(const ServiceRefPtr& to_manage));
  MOCK_METHOD1(RegisterService, void(const ServiceRefPtr& to_manage));
  MOCK_METHOD1(UpdateService, void(const ServiceRefPtr& to_update));
  MOCK_METHOD1(DeregisterService, void(const ServiceRefPtr& to_forget));
  MOCK_METHOD1(RegisterDefaultServiceCallback,
               int(const ServiceCallback& callback));
  MOCK_METHOD1(DeregisterDefaultServiceCallback, void(int tag));
  MOCK_METHOD1(UpdateDevice, void(const DeviceRefPtr& to_update));
  MOCK_METHOD0(UpdateWiFiProvider, void());
  MOCK_METHOD1(OnDeviceGeolocationInfoUpdated,
               void(const DeviceRefPtr& device));
  MOCK_METHOD1(RecheckPortalOnService, void(const ServiceRefPtr& service));
  MOCK_METHOD2(HandleProfileEntryDeletion,
               bool(const ProfileRefPtr& profile,
                    const std::string& entry_name));
  MOCK_CONST_METHOD0(GetDefaultService, ServiceRefPtr());
  MOCK_METHOD3(GetServiceWithStorageIdentifier,
               ServiceRefPtr(const ProfileRefPtr& profile,
                             const std::string& entry_name,
                             Error* error));
  MOCK_METHOD3(CreateTemporaryServiceFromProfile,
               ServiceRefPtr(const ProfileRefPtr& profile,
                             const std::string& entry_name,
                             Error* error));
  MOCK_CONST_METHOD0(IsConnected, bool());
  MOCK_METHOD0(UpdateEnabledTechnologies, void());
  MOCK_METHOD1(IsPortalDetectionEnabled, bool(Technology::Identifier tech));
  MOCK_CONST_METHOD1(IsServiceEphemeral,
                     bool(const ServiceConstRefPtr& service));
  MOCK_CONST_METHOD2(IsProfileBefore,
                     bool(const ProfileRefPtr& a,
                          const ProfileRefPtr& b));
  MOCK_CONST_METHOD1(IsTechnologyConnected,
                     bool(Technology::Identifier tech));
  MOCK_CONST_METHOD1(IsTechnologyLinkMonitorEnabled,
                     bool(Technology::Identifier tech));
  MOCK_CONST_METHOD1(IsTechnologyAutoConnectDisabled,
                     bool(Technology::Identifier tech));
  MOCK_CONST_METHOD1(IsDefaultProfile, bool(const StoreInterface* storage));
  MOCK_METHOD3(RequestScan, void(Device::ScanType request_origin,
                                 const std::string& technology, Error* error));
  MOCK_CONST_METHOD0(GetPortalCheckURL, const std::string&());
  MOCK_CONST_METHOD0(GetPortalCheckInterval, int());
  MOCK_METHOD0(IsSuspending, bool());
  MOCK_CONST_METHOD1(GetEnabledDeviceWithTechnology,
                     DeviceRefPtr(Technology::Identifier technology));
  MOCK_CONST_METHOD1(GetEnabledDeviceByLinkName,
                     DeviceRefPtr(const std::string& link_name));
  MOCK_CONST_METHOD0(GetMinimumMTU, int());
  MOCK_CONST_METHOD1(ShouldAcceptHostnameFrom,
                     bool(const std::string& device_name));
  MOCK_CONST_METHOD1(IsDHCPv6EnabledForDevice,
                     bool(const std::string& device_name));
  MOCK_METHOD1(SetBlacklistedDevices,
               void(const std::vector<std::string>& blacklisted_devices));
  MOCK_METHOD1(SetDHCPv6EnabledDevices,
               void(const std::vector<std::string>& device_list));
  MOCK_METHOD2(SetTechnologyOrder,
               void(const std::string& order, Error* error));
  MOCK_METHOD1(SetIgnoreUnknownEthernet, void(bool ignore));
  MOCK_METHOD1(SetStartupPortalList, void(const std::string& portal_list));
  MOCK_METHOD0(SetPassiveMode, void());
  MOCK_METHOD1(SetPrependDNSServers,
               void(const std::string& prepend_dns_servers));
  MOCK_METHOD1(SetMinimumMTU, void(const int mtu));
  MOCK_METHOD1(SetAcceptHostnameFrom, void(const std::string& hostname_from));
  MOCK_CONST_METHOD0(ignore_unknown_ethernet, bool());
  MOCK_CONST_METHOD1(FilterPrependDNSServersByFamily,
                     std::vector<std::string>(IPAddress::Family family));
  MOCK_METHOD0(OnInnerDevicesChanged, void());
  MOCK_METHOD3(ClaimDevice,
               void(const std::string& claimer_name,
                    const std::string& interface_name, Error* error));
  MOCK_METHOD4(ReleaseDevice, void(const std::string& claimer_name,
                                   const std::string& interface_name,
                                   bool* claimer_removed, Error* error));
  MOCK_METHOD0(OnDeviceClaimerVanished, void());
#if !defined(DISABLE_WIFI) && defined(__BRILLO__)
  MOCK_METHOD2(SetupApModeInterface,
               bool(std::string* out_interface_name, Error* error));
  MOCK_METHOD2(SetupStationModeInterface,
               bool(std::string* out_interface_name, Error* error));
  MOCK_METHOD0(OnApModeSetterVanished, void());
#endif  // !DISABLE_WIFI && __BRILLO__

  // Getter and setter for a mocked device info instance.
  DeviceInfo* mock_device_info() { return mock_device_info_; }
  void set_mock_device_info(DeviceInfo* mock_device_info) {
      mock_device_info_ = mock_device_info;
  }

 private:
  DeviceInfo* mock_device_info_;

  DISALLOW_COPY_AND_ASSIGN(MockManager);
};

}  // namespace shill

#endif  // SHILL_MOCK_MANAGER_H_
