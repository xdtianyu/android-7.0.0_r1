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

#include "shill/cellular/active_passive_out_of_credits_detector.h"

#include <string>

#include "shill/cellular/cellular_service.h"
#include "shill/connection.h"
#include "shill/connection_health_checker.h"
#include "shill/logging.h"
#include "shill/manager.h"
#include "shill/traffic_monitor.h"

using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kCellular;
static string ObjectID(ActivePassiveOutOfCreditsDetector* a) {
  return a->GetServiceRpcIdentifier();
}
}

// static
const int64_t
    ActivePassiveOutOfCreditsDetector::kOutOfCreditsConnectionDropSeconds = 15;
const int
    ActivePassiveOutOfCreditsDetector::kOutOfCreditsMaxConnectAttempts = 3;
const int64_t
    ActivePassiveOutOfCreditsDetector::kOutOfCreditsResumeIgnoreSeconds = 5;

ActivePassiveOutOfCreditsDetector::ActivePassiveOutOfCreditsDetector(
    EventDispatcher* dispatcher,
    Manager* manager,
    Metrics* metrics,
    CellularService* service)
    : OutOfCreditsDetector(dispatcher, manager, metrics, service),
      weak_ptr_factory_(this),
      traffic_monitor_(
          new TrafficMonitor(service->cellular(), dispatcher)),
      service_rpc_identifier_(service->GetRpcIdentifier()) {
  ResetDetector();
  traffic_monitor_->set_network_problem_detected_callback(
      Bind(&ActivePassiveOutOfCreditsDetector::OnNoNetworkRouting,
           weak_ptr_factory_.GetWeakPtr()));
}

ActivePassiveOutOfCreditsDetector::~ActivePassiveOutOfCreditsDetector() {
  StopTrafficMonitor();
}

void ActivePassiveOutOfCreditsDetector::ResetDetector() {
  SLOG(this, 2) << "Reset out-of-credits detection";
  out_of_credits_detection_in_progress_ = false;
  num_connect_attempts_ = 0;
}

bool ActivePassiveOutOfCreditsDetector::IsDetecting() const {
  return out_of_credits_detection_in_progress_;
}

void ActivePassiveOutOfCreditsDetector::NotifyServiceStateChanged(
    Service::ConnectState old_state, Service::ConnectState new_state) {
  SLOG(this, 2) << __func__ << ": " << old_state << " -> " << new_state;
  switch (new_state) {
    case Service::kStateUnknown:
    case Service::kStateIdle:
    case Service::kStateFailure:
      StopTrafficMonitor();
      health_checker_.reset();
      break;
    case Service::kStateAssociating:
      if (num_connect_attempts_ == 0)
        ReportOutOfCredits(false);
      if (old_state != Service::kStateAssociating) {
        connect_start_time_ = base::Time::Now();
        num_connect_attempts_++;
        SLOG(this, 2) << __func__
                      << ": num_connect_attempts="
                      << num_connect_attempts_;
      }
      break;
    case Service::kStateConnected:
      StartTrafficMonitor();
      SetupConnectionHealthChecker();
      break;
    case Service::kStatePortal:
      SLOG(this, 2) << "Portal detection failed. Launching active probe "
                    << "for out-of-credit detection.";
      RequestConnectionHealthCheck();
      break;
    case Service::kStateConfiguring:
    case Service::kStateOnline:
      break;
  }
  DetectConnectDisconnectLoop(old_state, new_state);
}

bool ActivePassiveOutOfCreditsDetector::StartTrafficMonitor() {
  SLOG(this, 2) << __func__;
  SLOG(this, 2) << "Service " << service()->friendly_name()
                << ": Traffic Monitor starting.";
  traffic_monitor_->Start();
  return true;
}

void ActivePassiveOutOfCreditsDetector::StopTrafficMonitor() {
  SLOG(this, 2) << __func__;
  SLOG(this, 2) << "Service " << service()->friendly_name()
                << ": Traffic Monitor stopping.";
  traffic_monitor_->Stop();
}

void ActivePassiveOutOfCreditsDetector::OnNoNetworkRouting(int reason) {
  SLOG(this, 2) << "Service " << service()->friendly_name()
                << ": Traffic Monitor detected network congestion.";
  SLOG(this, 2) << "Requesting active probe for out-of-credit detection.";
  RequestConnectionHealthCheck();
}

void ActivePassiveOutOfCreditsDetector::SetupConnectionHealthChecker() {
  DCHECK(service()->connection());
  // TODO(thieule): Consider moving health_checker_remote_ips() out of manager
  // (crbug.com/304974).
  if (!health_checker_.get()) {
    health_checker_.reset(
        new ConnectionHealthChecker(
            service()->connection(),
            dispatcher(),
            manager()->health_checker_remote_ips(),
            Bind(&ActivePassiveOutOfCreditsDetector::
                 OnConnectionHealthCheckerResult,
                 weak_ptr_factory_.GetWeakPtr())));
  } else {
    health_checker_->SetConnection(service()->connection());
  }
  // Add URL in either case because a connection reset could have dropped past
  // DNS queries.
  health_checker_->AddRemoteURL(manager()->GetPortalCheckURL());
}

void ActivePassiveOutOfCreditsDetector::RequestConnectionHealthCheck() {
  if (!health_checker_.get()) {
    SLOG(this, 2) << "No health checker exists, cannot request "
                  << "health check.";
    return;
  }
  if (health_checker_->health_check_in_progress()) {
    SLOG(this, 2) << "Health check already in progress.";
    return;
  }
  health_checker_->Start();
}

void ActivePassiveOutOfCreditsDetector::OnConnectionHealthCheckerResult(
    ConnectionHealthChecker::Result result) {
  SLOG(this, 2) << __func__ << "(Result = "
                << ConnectionHealthChecker::ResultToString(result) << ")";

  if (result == ConnectionHealthChecker::kResultCongestedTxQueue) {
    LOG(WARNING) << "Active probe determined possible out-of-credits "
                 << "scenario.";
    if (service()) {
      Metrics::CellularOutOfCreditsReason reason =
          (result == ConnectionHealthChecker::kResultCongestedTxQueue) ?
              Metrics::kCellularOutOfCreditsReasonTxCongested :
              Metrics::kCellularOutOfCreditsReasonElongatedTimeWait;
      metrics()->NotifyCellularOutOfCredits(reason);

      ReportOutOfCredits(true);
      SLOG(this, 2) << "Disconnecting due to out-of-credit scenario.";
      Error error;
      service()->Disconnect(&error, "out-of-credits");
    }
  }
}

void ActivePassiveOutOfCreditsDetector::DetectConnectDisconnectLoop(
    Service::ConnectState curr_state, Service::ConnectState new_state) {
  // WORKAROUND:
  // Some modems on Verizon network do not properly redirect when a SIM
  // runs out of credits. This workaround is used to detect an out-of-credits
  // condition by retrying a connect request if it was dropped within
  // kOutOfCreditsConnectionDropSeconds. If the number of retries exceeds
  // kOutOfCreditsMaxConnectAttempts, then the SIM is considered
  // out-of-credits and the cellular service kOutOfCreditsProperty is set.
  // This will signal Chrome to display the appropriate UX and also suppress
  // auto-connect until the next time the user manually connects.
  //
  // TODO(thieule): Remove this workaround (crosbug.com/p/18169).
  if (out_of_credits()) {
    SLOG(this, 2) << __func__
                  << ": Already out-of-credits, skipping check";
    return;
  }
  base::TimeDelta
      time_since_resume = base::Time::Now() - service()->resume_start_time();
  if (time_since_resume.InSeconds() < kOutOfCreditsResumeIgnoreSeconds) {
    // On platforms that power down the modem during suspend, make sure that
    // we do not display a false out-of-credits warning to the user
    // due to the sequence below by skipping out-of-credits detection
    // immediately after a resume.
    //   1. User suspends Chromebook.
    //   2. Hardware turns off power to modem.
    //   3. User resumes Chromebook.
    //   4. Hardware restores power to modem.
    //   5. ModemManager still has instance of old modem.
    //      ModemManager does not delete this instance until udev fires a
    //      device removed event.  ModemManager does not detect new modem
    //      until udev fires a new device event.
    //   6. Shill performs auto-connect against the old modem.
    //      Make sure at this step that we do not display a false
    //      out-of-credits warning.
    //   7. Udev fires device removed event.
    //   8. Udev fires new device event.
    SLOG(this, 2) <<
        "Skipping out-of-credits detection, too soon since resume.";
    ResetDetector();
    return;
  }
  base::TimeDelta
      time_since_connect = base::Time::Now() - connect_start_time_;
  if (time_since_connect.InSeconds() > kOutOfCreditsConnectionDropSeconds) {
    ResetDetector();
    return;
  }
  // Verizon can drop the connection in two ways:
  //   - Denies the connect request
  //   - Allows connect request but disconnects later
  bool connection_dropped =
      (Service::IsConnectedState(curr_state) ||
       Service::IsConnectingState(curr_state)) &&
      (new_state == Service::kStateFailure ||
       new_state == Service::kStateIdle);
  if (!connection_dropped)
    return;
  if (service()->explicitly_disconnected())
    return;
  if (service()->roaming_state() == kRoamingStateRoaming &&
      !service()->cellular()->allow_roaming_property())
    return;
  if (time_since_connect.InSeconds() <= kOutOfCreditsConnectionDropSeconds) {
    if (num_connect_attempts_ < kOutOfCreditsMaxConnectAttempts) {
      SLOG(this, 2) << "Out-Of-Credits detection: Reconnecting "
                    << "(retry #" << num_connect_attempts_ << ")";
      // Prevent autoconnect logic from kicking in while we perform the
      // out-of-credits detection.
      out_of_credits_detection_in_progress_ = true;
      dispatcher()->PostTask(
          Bind(&ActivePassiveOutOfCreditsDetector::OutOfCreditsReconnect,
               weak_ptr_factory_.GetWeakPtr()));
    } else {
      LOG(INFO) << "Active/Passive Out-Of-Credits detection: "
                << "Marking service as out-of-credits";
      metrics()->NotifyCellularOutOfCredits(
          Metrics::kCellularOutOfCreditsReasonConnectDisconnectLoop);
      ReportOutOfCredits(true);
      ResetDetector();
    }
  }
}

void ActivePassiveOutOfCreditsDetector::OutOfCreditsReconnect() {
  Error error;
  service()->Connect(&error, __func__);
}

void ActivePassiveOutOfCreditsDetector::set_traffic_monitor(
    TrafficMonitor* traffic_monitor) {
  traffic_monitor_.reset(traffic_monitor);
}

void ActivePassiveOutOfCreditsDetector::set_connection_health_checker(
    ConnectionHealthChecker* health_checker) {
  health_checker_.reset(health_checker);
}

}  // namespace shill
