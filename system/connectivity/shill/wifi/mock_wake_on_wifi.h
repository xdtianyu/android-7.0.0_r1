//
// Copyright (C) 2014 The Android Open Source Project
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

#ifndef SHILL_WIFI_MOCK_WAKE_ON_WIFI_H_
#define SHILL_WIFI_MOCK_WAKE_ON_WIFI_H_

#include <string>
#include <vector>

#include <gmock/gmock.h>

#include "shill/net/nl80211_message.h"
#include "shill/wifi/wake_on_wifi.h"

namespace shill {

class MockWakeOnWiFi : public WakeOnWiFi {
 public:
  MockWakeOnWiFi(NetlinkManager* netlink_manager, EventDispatcher* dispatcher,
                 Metrics* metrics);
  ~MockWakeOnWiFi() override;

  MOCK_METHOD0(OnAfterResume, void());
  MOCK_METHOD7(OnBeforeSuspend,
               void(bool is_connected,
                    const std::vector<ByteString>& ssid_whitelist,
                    const ResultCallback& done_callback,
                    const base::Closure& renew_dhcp_lease_callback,
                    const base::Closure& remove_supplicant_networks_callback,
                    bool have_dhcp_lease, uint32_t time_to_next_lease_renewal));
  MOCK_METHOD6(OnDarkResume,
               void(bool is_connected,
                    const std::vector<ByteString>& ssid_whitelist,
                    const ResultCallback& done_callback,
                    const base::Closure& renew_dhcp_lease_callback,
                    const InitiateScanCallback& initiate_scan_callback,
                    const base::Closure& remove_supplicant_networks_callback));
  MOCK_METHOD2(OnConnectedAndReachable,
               void(bool start_lease_renewal_timer,
                    uint32_t time_to_next_lease_renewal));
  MOCK_METHOD1(ReportConnectedToServiceAfterWake, void(bool is_connected));
  MOCK_METHOD3(OnNoAutoConnectableServicesAfterScan,
               void(const std::vector<ByteString>& ssid_whitelist,
                    const base::Closure& remove_supplicant_networks_callback,
                    const InitiateScanCallback& initiate_scan_callback));
  MOCK_METHOD1(OnWakeupReasonReceived,
               void(const NetlinkMessage& netlink_message));
  MOCK_METHOD0(NotifyWakeupReasonReceived, void());
  MOCK_METHOD1(NotifyWakeOnWiFiOnDarkResume,
               void(WakeOnWiFi::WakeOnWiFiTrigger reason));
  MOCK_METHOD1(OnWiphyIndexReceived, void(uint32_t));
  MOCK_METHOD1(ParseWakeOnWiFiCapabilities,
               void(const Nl80211Message& nl80211_message));
  MOCK_METHOD1(OnScanStarted, void(bool is_active_scan));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockWakeOnWiFi);
};

}  // namespace shill

#endif  // SHILL_WIFI_MOCK_WAKE_ON_WIFI_H_
