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

// This class is a callback object that observes all nl80211 events that come
// up from the kernel.

#ifndef SHILL_WIFI_CALLBACK80211_METRICS_H_
#define SHILL_WIFI_CALLBACK80211_METRICS_H_

#include <base/macros.h>
#include <base/memory/weak_ptr.h>

#include "shill/net/ieee80211.h"
#include "shill/net/netlink_manager.h"

namespace shill {

class Metrics;
class NetlinkManager;
class NetlinkMessage;

// NetlinkManager callback object that sends stuff to UMA metrics.
class Callback80211Metrics :
  public base::SupportsWeakPtr<Callback80211Metrics> {
 public:
  explicit Callback80211Metrics(Metrics* metrics);

  // Called with each broadcast netlink message that arrives to NetlinkManager.
  // If the message is a deauthenticate message, the method collects the reason
  // for the deauthentication and communicates those to UMA.
  void CollectDisconnectStatistics(const NetlinkMessage& msg);

 private:
  static const char kMetricLinkDisconnectCount[];

  IEEE_80211::WiFiReasonCode WiFiReasonCodeFromUint16(uint16_t reason) const;

  Metrics* metrics_;

  DISALLOW_COPY_AND_ASSIGN(Callback80211Metrics);
};

}  // namespace shill

#endif  // SHILL_WIFI_CALLBACK80211_METRICS_H_
