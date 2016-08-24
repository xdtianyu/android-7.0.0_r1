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

#ifndef SHILL_WIFI_MOCK_MAC80211_MONITOR_H_
#define SHILL_WIFI_MOCK_MAC80211_MONITOR_H_

#include <string>

#include <gmock/gmock.h>

#include "shill/wifi/mac80211_monitor.h"

namespace shill {

class MockMac80211Monitor : public Mac80211Monitor {
 public:
  MockMac80211Monitor(EventDispatcher* dispatcher,
                      const std::string& link_name,
                      size_t queue_length_limit,
                      const base::Closure& on_repair_callback,
                      Metrics* metrics);
  ~MockMac80211Monitor() override;

  MOCK_METHOD1(Start, void(const std::string& phy_name));
  MOCK_METHOD0(Stop, void());
  MOCK_METHOD1(UpdateConnectedState, void(bool new_state));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockMac80211Monitor);
};

}  // namespace shill

#endif  // SHILL_WIFI_MOCK_MAC80211_MONITOR_H_
