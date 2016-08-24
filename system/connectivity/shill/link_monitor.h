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

#ifndef SHILL_LINK_MONITOR_H_
#define SHILL_LINK_MONITOR_H_

#include <time.h>

#include <memory>
#include <string>

#include <base/callback.h>
#include <base/cancelable_callback.h>

#include "shill/metrics.h"
#include "shill/net/byte_string.h"
#include "shill/refptr_types.h"

namespace shill {

class ActiveLinkMonitor;
class DeviceInfo;
class EventDispatcher;
class PassiveLinkMonitor;
class Time;

class LinkMonitor {
 public:
  typedef base::Closure FailureCallback;
  typedef base::Closure GatewayChangeCallback;

  // The default number of milliseconds between ARP requests used by
  // ActiveLinkMonitor. Needed by Metrics.
  static const int kDefaultTestPeriodMilliseconds;

  // The default list of technologies for which link monitoring is enabled.
  // Needed by DefaultProfile.
  static const char kDefaultLinkMonitorTechnologies[];

  // Failure threshold count used by ActiveLinkMonitor.  Needed by Metrics.
  static const int kFailureThreshold;

  LinkMonitor(const ConnectionRefPtr& connection,
              EventDispatcher* dispatcher,  // Owned by caller; can't be NULL.
              Metrics* metrics,  // Owned by caller; must not be NULL.
              DeviceInfo* device_info,
              const FailureCallback& failure_callback,
              const GatewayChangeCallback& gateway_change_callback);
  virtual ~LinkMonitor();

  // Starts link-monitoring on the selected connection.  Returns
  // true if successful, false otherwise.
  virtual bool Start();
  // Stop link-monitoring on the selected connection. Clears any
  // accumulated statistics.
  virtual void Stop();

  // Inform LinkMonitor that the system is resuming from sleep.
  // LinkMonitor will immediately start the ActiveLinkMonitor, using a lower
  // timeout than normal.
  virtual void OnAfterResume();

  // Return modified cumulative average of the gateway ARP response
  // time.  Returns zero if no samples are available.  For each
  // missed ARP response, the sample is assumed to be the full
  // test period.
  virtual int GetResponseTimeMilliseconds() const;

  // Returns true if the LinkMonitor was ever able to find the default
  // gateway via broadcast ARP.
  virtual bool IsGatewayFound() const;

  const ByteString& gateway_mac_address() const {
    return gateway_mac_address_;
  }

 private:
  friend class LinkMonitorTest;

  void OnActiveLinkMonitorFailure(Metrics::LinkMonitorFailure failure,
                                  int broadcast_failure_count,
                                  int unicast_failure_count);
  void OnActiveLinkMonitorSuccess();
  void OnPassiveLinkMonitorResultCallback(bool status);

  // The connection on which to perform link monitoring.
  ConnectionRefPtr connection_;
  // Dispatcher on which to create delayed tasks.
  EventDispatcher* dispatcher_;
  // Metrics instance on which to post performance results.
  Metrics* metrics_;
  // Failure callback method to call if LinkMonitor fails.
  FailureCallback failure_callback_;
  // Callback method to call if gateway mac address changes.
  GatewayChangeCallback gateway_change_callback_;
  std::unique_ptr<ActiveLinkMonitor> active_link_monitor_;
  std::unique_ptr<PassiveLinkMonitor> passive_link_monitor_;
  // The MAC address of the default gateway.
  ByteString gateway_mac_address_;
  // The time at which the link monitor started.
  struct timeval started_monitoring_at_;
  // Time instance for performing GetTimeMonotonic().
  Time* time_;

  DISALLOW_COPY_AND_ASSIGN(LinkMonitor);
};

}  // namespace shill

#endif  // SHILL_LINK_MONITOR_H_
