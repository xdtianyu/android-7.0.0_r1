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

#include "shill/wifi/mock_mac80211_monitor.h"

namespace shill {

MockMac80211Monitor::MockMac80211Monitor(
    EventDispatcher* dispatcher,
    const std::string& link_name,
    size_t queue_length_limit,
    const base::Closure& on_repair_callback,
    Metrics* metrics)
    : Mac80211Monitor(
        dispatcher, link_name, queue_length_limit, on_repair_callback, metrics)
{}

MockMac80211Monitor::~MockMac80211Monitor() {}

}  // namespace shill
