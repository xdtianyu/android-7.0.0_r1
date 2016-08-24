//
// Copyright (C) 2013 The Android Open Source Project
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

#ifndef SHILL_WIFI_MOCK_SCAN_SESSION_H_
#define SHILL_WIFI_MOCK_SCAN_SESSION_H_

#include "shill/wifi/scan_session.h"

#include <set>

#include <gmock/gmock.h>

#include "shill/wifi/wifi_provider.h"

namespace shill {

class ByteString;
class EventDispatcher;
class Metrics;
class NetlinkManager;

class MockScanSession : public ScanSession {
 public:
  MockScanSession(NetlinkManager* netlink_manager,
                  EventDispatcher* dispatcher,
                  const WiFiProvider::FrequencyCountList& previous_frequencies,
                  const std::set<uint16_t>& available_frequencies,
                  uint32_t ifindex,
                  const FractionList& fractions,
                  int min_frequencies,
                  int max_frequencies,
                  OnScanFailed on_scan_failed,
                  Metrics* metrics);
  ~MockScanSession() override;

  MOCK_CONST_METHOD0(HasMoreFrequencies, bool());
  MOCK_METHOD1(AddSsid, void(const ByteString& ssid));
  MOCK_METHOD0(InitiateScan, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockScanSession);
};

}  // namespace shill

#endif  // SHILL_WIFI_MOCK_SCAN_SESSION_H_
