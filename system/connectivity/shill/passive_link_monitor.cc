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

#include "shill/passive_link_monitor.h"

#include <string>

#include <base/bind.h>

#include "shill/arp_client.h"
#include "shill/arp_packet.h"
#include "shill/connection.h"
#include "shill/event_dispatcher.h"
#include "shill/logging.h"
#include "shill/net/byte_string.h"

using base::Bind;
using base::Unretained;
using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kLink;
static string ObjectID(Connection* c) { return c->interface_name(); }
}

// static.
const int PassiveLinkMonitor::kDefaultMonitorCycles = 40;
const int PassiveLinkMonitor::kCyclePeriodMilliseconds = 25000;
const int PassiveLinkMonitor::kMinArpRequestsPerCycle = 5;

PassiveLinkMonitor::PassiveLinkMonitor(const ConnectionRefPtr& connection,
                                       EventDispatcher* dispatcher,
                                       const ResultCallback& result_callback)
    : connection_(connection),
      dispatcher_(dispatcher),
      // Connection is not provided when this is used as a mock for testing
      // purpose.
      arp_client_(
          new ArpClient(connection ? connection->interface_index() : 0)),
      result_callback_(result_callback),
      num_cycles_to_monitor_(kDefaultMonitorCycles),
      num_requests_received_(0),
      num_cycles_passed_(0) {
}

PassiveLinkMonitor::~PassiveLinkMonitor() {
  Stop();
}

bool PassiveLinkMonitor::Start(int num_cycles) {
  SLOG(connection_.get(), 2) << "In " << __func__ << ".";
  Stop();

  if (!StartArpClient()) {
    return false;
  }
  // Start the monitor cycle.
  monitor_cycle_timeout_callback_.Reset(
      Bind(&PassiveLinkMonitor::CycleTimeoutHandler, Unretained(this)));
  dispatcher_->PostDelayedTask(monitor_cycle_timeout_callback_.callback(),
                               kCyclePeriodMilliseconds);
  num_cycles_to_monitor_ = num_cycles;
  return true;
}

void PassiveLinkMonitor::Stop() {
  SLOG(connection_.get(), 2) << "In " << __func__ << ".";
  StopArpClient();
  num_requests_received_ = 0;
  num_cycles_passed_ = 0;
  monitor_cycle_timeout_callback_.Cancel();
  monitor_completed_callback_.Cancel();
}

bool PassiveLinkMonitor::StartArpClient() {
  if (!arp_client_->StartRequestListener()) {
    return false;
  }
  receive_request_handler_.reset(
      dispatcher_->CreateReadyHandler(
          arp_client_->socket(),
          IOHandler::kModeInput,
          Bind(&PassiveLinkMonitor::ReceiveRequest, Unretained(this))));
  return true;
}

void PassiveLinkMonitor::StopArpClient() {
  arp_client_->Stop();
  receive_request_handler_.reset();
}

void PassiveLinkMonitor::ReceiveRequest(int fd) {
  SLOG(connection_.get(), 2) << "In " << __func__ << ".";
  ArpPacket packet;
  ByteString sender;

  if (!arp_client_->ReceivePacket(&packet, &sender)) {
    return;
  }

  if (packet.IsReply()) {
    SLOG(connection_.get(), 4) << "This is not a request packet.  Ignoring.";
    return;
  }

  num_requests_received_++;
  // Stop ARP client if we receive enough requests for the current cycle.
  if (num_requests_received_ >= kMinArpRequestsPerCycle) {
    StopArpClient();
  }
}

void PassiveLinkMonitor::CycleTimeoutHandler() {
  bool status = false;
  if (num_requests_received_ >= kMinArpRequestsPerCycle) {
    num_requests_received_ = 0;
    num_cycles_passed_++;
    if (num_cycles_passed_ < num_cycles_to_monitor_) {
      // Continue on with the next cycle.
      StartArpClient();
      dispatcher_->PostDelayedTask(monitor_cycle_timeout_callback_.callback(),
                                   kCyclePeriodMilliseconds);
      return;
    }
    // Monitor completed.
    status = true;
  }

  // Post a task to perform cleanup and invoke result callback, since this
  // function is invoked from the callback that will be cancelled during
  // cleanup.
  monitor_completed_callback_.Reset(
      Bind(&PassiveLinkMonitor::MonitorCompleted, Unretained(this), status));
  dispatcher_->PostTask(monitor_completed_callback_.callback());
}

void PassiveLinkMonitor::MonitorCompleted(bool status) {
  // Stop the monitoring before invoking result callback, so that the ARP client
  // is stopped by the time result callback is invoked.
  Stop();
  result_callback_.Run(status);
}

}  // namespace shill
