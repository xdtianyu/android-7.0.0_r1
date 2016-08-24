//
// Copyright (C) 2015 The Android Open Source Project
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

#ifndef SHILL_ACTIVE_LINK_MONITOR_H_
#define SHILL_ACTIVE_LINK_MONITOR_H_

#include <time.h>

#include <memory>
#include <string>

#include <base/callback.h>
#include <base/cancelable_callback.h>

#include "shill/metrics.h"
#include "shill/net/byte_string.h"
#include "shill/refptr_types.h"

namespace shill {

class ArpClient;
class DeviceInfo;
class EventDispatcher;
class IOHandler;
class Time;

// ActiveLinkMonitor probes the status of a connection by sending ARP
// messages to the default gateway for a connection. The link will be declared
// as failure if no ARP reply is received for 5 consecutive broadcast ARP
// requests or unicast ARP requests in the case when gateway unicast ARP
// support is established. And active when an ARP reply is received.
// A callback will be invoked  when the link is detected as failure or active.
// The active link monitor will automatically stop when the link status is
// determined. It also keeps track of response times which can be an indicator
// of link quality.
class ActiveLinkMonitor {
 public:
  // FailureCallback takes monitor failure code, broadcast failure count, and
  // unicast failure count as arguments.
  typedef base::Callback<void(Metrics::LinkMonitorFailure, int, int)>
      FailureCallback;
  typedef base::Closure SuccessCallback;

  // The default number of milliseconds between ARP requests. Needed by Metrics.
  static const int kDefaultTestPeriodMilliseconds;

  // The number of milliseconds between ARP requests when running a quick test.
  // Used when the device just resume from suspend. Also needed by unit tests.
  static const int kFastTestPeriodMilliseconds;

  // When the sum of consecutive counted unicast and broadcast failures
  // equals this value, the failure callback is called, the counters
  // are reset, and the link monitoring quiesces.  Needed by Metrics.
  static const int kFailureThreshold;

  ActiveLinkMonitor(const ConnectionRefPtr& connection,
                    EventDispatcher* dispatcher,
                    Metrics* metrics,
                    DeviceInfo* device_info,
                    const FailureCallback& failure_callback,
                    const SuccessCallback& success_callback);
  virtual ~ActiveLinkMonitor();

  // Starts an active link-monitoring cycle on the selected connection, with
  // specified |probe_period_millisecond| milliseconds between each ARP
  // requests. Returns true if successful, false otherwise.
  virtual bool Start(int probe_period_millisecond);
  // Stop active link-monitoring on the selected connection. Clears any
  // accumulated statistics.
  virtual void Stop();

  // Return modified cumulative average of the gateway ARP response
  // time.  Returns zero if no samples are available.  For each
  // missed ARP response, the sample is assumed to be the full
  // test period.
  int GetResponseTimeMilliseconds() const;

  // Returns true if the ActiveLinkMonitor was ever able to find the default
  // gateway via broadcast ARP.
  bool IsGatewayFound() const;

  virtual const ByteString& gateway_mac_address() const {
    return gateway_mac_address_;
  }
  virtual void set_gateway_mac_address(const ByteString& gateway_mac_address) {
    gateway_mac_address_ = gateway_mac_address;
  }

  virtual bool gateway_supports_unicast_arp() const {
    return gateway_supports_unicast_arp_;
  }
  virtual void set_gateway_supports_unicast_arp(
      bool gateway_supports_unicast_arp) {
    gateway_supports_unicast_arp_ = gateway_supports_unicast_arp;
  }

 private:
  friend class ActiveLinkMonitorTest;

  // The number of samples to compute a "strict" average over.  When
  // more samples than this number arrive, this determines how "slow"
  // our simple low-pass filter works.
  static const int kMaxResponseSampleFilterDepth;

  // When the sum of consecutive unicast successes equals this value,
  // we can assume that in general this gateway supports unicast ARP
  // requests, and we will count future unicast failures.
  static const int kUnicastReplyReliabilityThreshold;

  // Similar to Start, except that the initial probes use
  // |probe_period_milliseconds|. After successfully probing with both
  // broadcast and unicast ARPs (at least one of each), LinkMonitor
  // switches itself to kDefaultTestPeriodMilliseconds.
  virtual bool StartInternal(int probe_period_milliseconds);
  // Stop the current monitoring cycle. It is called when current monitor cycle
  // results in success.
  void StopMonitorCycle();
  // Add a response time sample to the buffer.
  void AddResponseTimeSample(int response_time_milliseconds);
  // Start and stop ARP client for sending/receiving ARP requests/replies.
  bool StartArpClient();
  void StopArpClient();
  // Convert a hardware address byte-string to a colon-separated string.
  static std::string HardwareAddressToString(const ByteString& address);
  // Denote a missed response.  Returns true if this loss has caused us
  // to exceed the failure threshold.
  bool AddMissedResponse();
  // This I/O callback is triggered whenever the ARP reception socket
  // has data available to be received.
  void ReceiveResponse(int fd);
  // Send the next ARP request.
  void SendRequest();

  // The connection on which to perform link monitoring.
  ConnectionRefPtr connection_;
  // Dispatcher on which to create delayed tasks.
  EventDispatcher* dispatcher_;
  // Metrics instance on which to post performance results.
  Metrics* metrics_;
  // DeviceInfo instance for retrieving the MAC address of a device.
  DeviceInfo* device_info_;
  // Callback methods to call when ActiveLinkMonitor completes a cycle.
  FailureCallback failure_callback_;
  SuccessCallback success_callback_;
  // The MAC address of device associated with this connection.
  ByteString local_mac_address_;
  // The MAC address of the default gateway.
  ByteString gateway_mac_address_;
  // ArpClient instance used for performing link tests.
  std::unique_ptr<ArpClient> arp_client_;

  // How frequently we send an ARP request. This is also the timeout
  // for a pending request.
  int test_period_milliseconds_;
  // The number of consecutive times we have failed in receiving
  // responses to broadcast ARP requests.
  int broadcast_failure_count_;
  // The number of consecutive times we have failed in receiving
  // responses to unicast ARP requests.
  int unicast_failure_count_;
  // The number of consecutive times we have succeeded in receiving
  // responses to broadcast ARP requests.
  int broadcast_success_count_;
  // The number of consecutive times we have succeeded in receiving
  // responses to unicast ARP requests.
  int unicast_success_count_;

  // Whether this iteration of the test was a unicast request
  // to the gateway instead of broadcast.  The active link monitor
  // alternates between unicast and broadcast requests so that
  // both types of network traffic is monitored.
  bool is_unicast_;

  // Whether we have observed that the gateway reliably responds
  // to unicast ARP requests.
  bool gateway_supports_unicast_arp_;

  // Number of response samples received in our rolling averge.
  int response_sample_count_;
  // The sum of response samples in our rolling average.
  int response_sample_bucket_;

  // IOCallback that fires when the socket associated with our ArpClient
  // has a packet to be received.  Calls ReceiveResponse().
  std::unique_ptr<IOHandler> receive_response_handler_;
  // Callback method used for periodic transmission of ARP requests.
  // When the timer expires this will call SendRequest() through the
  // void callback function SendRequestTask().
  base::CancelableClosure send_request_callback_;

  // The time at which the last ARP request was sent.
  struct timeval sent_request_at_;
  // Time instance for performing GetTimeMonotonic().
  Time* time_;

  DISALLOW_COPY_AND_ASSIGN(ActiveLinkMonitor);
};

}  // namespace shill

#endif  // SHILL_ACTIVE_LINK_MONITOR_H_
