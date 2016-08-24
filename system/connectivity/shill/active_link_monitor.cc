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

#include "shill/active_link_monitor.h"

#include <string>
#include <vector>

#include <base/bind.h>
#include <base/strings/stringprintf.h>
#include <base/strings/string_util.h>

#include "shill/arp_client.h"
#include "shill/arp_packet.h"
#include "shill/connection.h"
#include "shill/device_info.h"
#include "shill/event_dispatcher.h"
#include "shill/logging.h"
#include "shill/metrics.h"
#include "shill/net/ip_address.h"
#include "shill/net/shill_time.h"

using base::Bind;
using base::Unretained;
using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kLink;
static string ObjectID(Connection* c) { return c->interface_name(); }
}

const int ActiveLinkMonitor::kDefaultTestPeriodMilliseconds = 5000;
const int ActiveLinkMonitor::kFailureThreshold = 5;
const int ActiveLinkMonitor::kFastTestPeriodMilliseconds = 200;
const int ActiveLinkMonitor::kMaxResponseSampleFilterDepth = 5;
const int ActiveLinkMonitor::kUnicastReplyReliabilityThreshold = 10;

ActiveLinkMonitor::ActiveLinkMonitor(const ConnectionRefPtr& connection,
                                     EventDispatcher* dispatcher,
                                     Metrics* metrics,
                                     DeviceInfo* device_info,
                                     const FailureCallback& failure_callback,
                                     const SuccessCallback& success_callback)
    : connection_(connection),
      dispatcher_(dispatcher),
      metrics_(metrics),
      device_info_(device_info),
      failure_callback_(failure_callback),
      success_callback_(success_callback),
      // Connection is not provided when this is used as a mock for testing
      // purpose.
      arp_client_(
          new ArpClient(connection ? connection->interface_index() : 0)),
      test_period_milliseconds_(kDefaultTestPeriodMilliseconds),
      broadcast_failure_count_(0),
      unicast_failure_count_(0),
      broadcast_success_count_(0),
      unicast_success_count_(0),
      is_unicast_(false),
      gateway_supports_unicast_arp_(false),
      response_sample_count_(0),
      response_sample_bucket_(0),
      time_(Time::GetInstance()) {
}

ActiveLinkMonitor::~ActiveLinkMonitor() {
  Stop();
}

bool ActiveLinkMonitor::Start(int test_period) {
  SLOG(connection_.get(), 2) << "In " << __func__ << ".";
  StopMonitorCycle();
  return StartInternal(test_period);
}

void ActiveLinkMonitor::Stop() {
  SLOG(connection_.get(), 2) << "In " << __func__ << ".";
  // Stop current cycle.
  StopMonitorCycle();

  // Clear stats accumulated from previous monitor cycles.
  local_mac_address_.Clear();
  gateway_mac_address_.Clear();
  broadcast_success_count_ = 0;
  unicast_success_count_ = 0;
  broadcast_failure_count_ = 0;
  unicast_failure_count_ = 0;
  is_unicast_ = false;
  gateway_supports_unicast_arp_ = false;
  response_sample_bucket_ = 0;
  response_sample_count_ = 0;
}

int ActiveLinkMonitor::GetResponseTimeMilliseconds() const {
  return response_sample_count_ ?
      response_sample_bucket_ / response_sample_count_ : 0;
}

bool ActiveLinkMonitor::IsGatewayFound() const {
  return !gateway_mac_address_.IsZero();
}

bool ActiveLinkMonitor::StartInternal(int probe_period_milliseconds) {
  test_period_milliseconds_ = probe_period_milliseconds;
  if (test_period_milliseconds_ > kDefaultTestPeriodMilliseconds) {
    LOG(WARNING) << "Long test period; UMA stats will be truncated.";
  }

  if (!device_info_->GetMACAddress(
          connection_->interface_index(), &local_mac_address_)) {
    LOG(ERROR) << "Could not get local MAC address.";
    metrics_->NotifyLinkMonitorFailure(
        connection_->technology(),
        Metrics::kLinkMonitorMacAddressNotFound,
        0, 0, 0);
    Stop();
    return false;
  }

  if (!StartArpClient()) {
    LOG(ERROR) << "Failed to start ARP client.";
    metrics_->NotifyLinkMonitorFailure(
        connection_->technology(),
        Metrics::kLinkMonitorClientStartFailure,
        0, 0, 0);
    Stop();
    return false;
  }

  if (gateway_mac_address_.IsEmpty()) {
    gateway_mac_address_ = ByteString(local_mac_address_.GetLength());
  }
  send_request_callback_.Reset(
      Bind(&ActiveLinkMonitor::SendRequest, Unretained(this)));
  // Post a task to send ARP request instead of calling it synchronously, to
  // maintain consistent expectation in the case of send failures, which will
  // always invoke failure callback.
  dispatcher_->PostTask(send_request_callback_.callback());
  return true;
}

void ActiveLinkMonitor::StopMonitorCycle() {
  StopArpClient();
  send_request_callback_.Cancel();
  timerclear(&sent_request_at_);
}

void ActiveLinkMonitor::AddResponseTimeSample(int response_time_milliseconds) {
  SLOG(connection_.get(), 2) << "In " << __func__ << " with sample "
                             << response_time_milliseconds << ".";
  metrics_->NotifyLinkMonitorResponseTimeSampleAdded(
      connection_->technology(), response_time_milliseconds);
  response_sample_bucket_ += response_time_milliseconds;
  if (response_sample_count_ < kMaxResponseSampleFilterDepth) {
    ++response_sample_count_;
  } else {
    response_sample_bucket_ =
        response_sample_bucket_ * kMaxResponseSampleFilterDepth /
            (kMaxResponseSampleFilterDepth + 1);
  }
}

// static
string ActiveLinkMonitor::HardwareAddressToString(const ByteString& address) {
  std::vector<string> address_parts;
  for (size_t i = 0; i < address.GetLength(); ++i) {
    address_parts.push_back(
        base::StringPrintf("%02x", address.GetConstData()[i]));
  }
  return base::JoinString(address_parts, ":");
}

bool ActiveLinkMonitor::StartArpClient() {
  if (!arp_client_->StartReplyListener()) {
    return false;
  }
  SLOG(connection_.get(), 4) << "Created ARP client; listening on socket "
                             << arp_client_->socket() << ".";
  receive_response_handler_.reset(
    dispatcher_->CreateReadyHandler(
        arp_client_->socket(),
        IOHandler::kModeInput,
        Bind(&ActiveLinkMonitor::ReceiveResponse, Unretained(this))));
  return true;
}

void ActiveLinkMonitor::StopArpClient() {
  arp_client_->Stop();
  receive_response_handler_.reset();
}

bool ActiveLinkMonitor::AddMissedResponse() {
  SLOG(connection_.get(), 2) << "In " << __func__ << ".";
  AddResponseTimeSample(test_period_milliseconds_);

  if (is_unicast_) {
    if (gateway_supports_unicast_arp_) {
      ++unicast_failure_count_;
    }
    unicast_success_count_ = 0;
  } else {
    ++broadcast_failure_count_;
    broadcast_success_count_ = 0;
  }

  if (unicast_failure_count_ + broadcast_failure_count_ >= kFailureThreshold) {
    LOG(ERROR) << "Link monitor has reached the failure threshold with "
               << broadcast_failure_count_
               << " broadcast failures and "
               << unicast_failure_count_
               << " unicast failures.";
    failure_callback_.Run(Metrics::kLinkMonitorFailureThresholdReached,
                          broadcast_failure_count_,
                          unicast_failure_count_);
    Stop();
    return true;
  }
  is_unicast_ = !is_unicast_;
  return false;
}

void ActiveLinkMonitor::ReceiveResponse(int fd) {
  SLOG(connection_.get(), 2) << "In " << __func__ << ".";
  ArpPacket packet;
  ByteString sender;
  if (!arp_client_->ReceivePacket(&packet, &sender)) {
    return;
  }

  if (!packet.IsReply()) {
    SLOG(connection_.get(), 4) << "This is not a reply packet.  Ignoring.";
    return;
  }

  if (!connection_->local().address().Equals(
           packet.remote_ip_address().address())) {
    SLOG(connection_.get(), 4) << "Response is not for our IP address.";
    return;
  }

  if (!local_mac_address_.Equals(packet.remote_mac_address())) {
    SLOG(connection_.get(), 4) << "Response is not for our MAC address.";
    return;
  }

  if (!connection_->gateway().address().Equals(
           packet.local_ip_address().address())) {
    SLOG(connection_.get(), 4)
        << "Response is not from the gateway IP address.";
    return;
  }

  struct timeval now, elapsed_time;
  time_->GetTimeMonotonic(&now);
  timersub(&now, &sent_request_at_, &elapsed_time);

  AddResponseTimeSample(elapsed_time.tv_sec * 1000 +
                        elapsed_time.tv_usec / 1000);

  if (is_unicast_) {
    ++unicast_success_count_;
    unicast_failure_count_ = 0;
    if (unicast_success_count_ >= kUnicastReplyReliabilityThreshold) {
      SLOG_IF(Link, 2, !gateway_supports_unicast_arp_)
          << "Gateway is now considered a reliable unicast responder.  "
             "Unicast failures will now count.";
      gateway_supports_unicast_arp_ = true;
    }
  } else {
    ++broadcast_success_count_;
    broadcast_failure_count_ = 0;
  }

  if (!gateway_mac_address_.Equals(packet.local_mac_address())) {
    const ByteString& new_mac_address = packet.local_mac_address();
    if (!IsGatewayFound()) {
      SLOG(connection_.get(), 2) << "Found gateway at "
                                 << HardwareAddressToString(new_mac_address);
    } else {
      SLOG(connection_.get(), 2) << "Gateway MAC address changed.";
    }
    gateway_mac_address_ = new_mac_address;
  }

  is_unicast_ = !is_unicast_;

  // Stop the current cycle, and invoke the success callback. All the
  // accumulated stats regarding the gateway are not cleared.
  StopMonitorCycle();
  success_callback_.Run();
}

void ActiveLinkMonitor::SendRequest() {
  SLOG(connection_.get(), 2) << "In " << __func__ << ".";

  // Timeout waiting for ARP reply and exceed the failure threshold.
  if (timerisset(&sent_request_at_) && AddMissedResponse()) {
    return;
  }

  ByteString destination_mac_address(gateway_mac_address_.GetLength());
  if (!IsGatewayFound()) {
    // The remote MAC addess is set by convention to be all-zeroes in the
    // ARP header if not known.  The ArpClient will translate an all-zeroes
    // remote address into a send to the broadcast (all-ones) address in
    // the Ethernet frame header.
    SLOG_IF(Link, 2, is_unicast_) << "Sending broadcast since "
                                  << "gateway MAC is unknown";
    is_unicast_ = false;
  } else if (is_unicast_) {
    destination_mac_address = gateway_mac_address_;
  }

  ArpPacket request(connection_->local(), connection_->gateway(),
                    local_mac_address_, destination_mac_address);
  if (!arp_client_->TransmitRequest(request)) {
    LOG(ERROR) << "Failed to send ARP request.  Stopping.";
    failure_callback_.Run(Metrics::kLinkMonitorTransmitFailure,
                          broadcast_failure_count_,
                          unicast_failure_count_);
    Stop();
    return;
  }

  time_->GetTimeMonotonic(&sent_request_at_);

  dispatcher_->PostDelayedTask(send_request_callback_.callback(),
                               test_period_milliseconds_);
}

}  // namespace shill
