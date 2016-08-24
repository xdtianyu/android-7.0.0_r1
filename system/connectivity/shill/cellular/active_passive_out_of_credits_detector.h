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

#ifndef SHILL_CELLULAR_ACTIVE_PASSIVE_OUT_OF_CREDITS_DETECTOR_H_
#define SHILL_CELLULAR_ACTIVE_PASSIVE_OUT_OF_CREDITS_DETECTOR_H_

#include <memory>
#include <string>

#include <base/time/time.h>

#include "shill/cellular/out_of_credits_detector.h"
#include "shill/connection_health_checker.h"

namespace shill {

// Detects out-of-credits condition by monitoring for the following scenarios:
//   - Passively watch for network congestion and launch active probes to
//     determine if the network has stopped routing traffic.
//   - Watch for connect/disconnect loop.
class ActivePassiveOutOfCreditsDetector : public OutOfCreditsDetector {
 public:
  ActivePassiveOutOfCreditsDetector(EventDispatcher* dispatcher,
                                    Manager* manager,
                                    Metrics* metrics,
                                    CellularService* service);
  ~ActivePassiveOutOfCreditsDetector() override;

  void ResetDetector() override;
  bool IsDetecting() const override;
  void NotifyServiceStateChanged(
      Service::ConnectState old_state,
      Service::ConnectState new_state) override;
  void NotifySubscriptionStateChanged(uint32_t subscription_state) override {}

  const TrafficMonitor* traffic_monitor() const {
    return traffic_monitor_.get();
  }

  const std::string& GetServiceRpcIdentifier() const {
    return service_rpc_identifier_;
  }

 private:
  friend class ActivePassiveOutOfCreditsDetectorTest;
  FRIEND_TEST(ActivePassiveOutOfCreditsDetectorTest,
      ConnectDisconnectLoopDetectionIntermittentNetwork);
  FRIEND_TEST(ActivePassiveOutOfCreditsDetectorTest,
      ConnectDisconnectLoopDetectionNotSkippedAfterSlowResume);
  FRIEND_TEST(ActivePassiveOutOfCreditsDetectorTest,
      OnConnectionHealthCheckerResult);
  FRIEND_TEST(ActivePassiveOutOfCreditsDetectorTest, OnNoNetworkRouting);
  FRIEND_TEST(ActivePassiveOutOfCreditsDetectorTest, StopTrafficMonitor);

  static const int64_t kOutOfCreditsConnectionDropSeconds;
  static const int kOutOfCreditsMaxConnectAttempts;
  static const int64_t kOutOfCreditsResumeIgnoreSeconds;

  // Initiates traffic monitoring.
  bool StartTrafficMonitor();

  // Stops traffic monitoring.
  void StopTrafficMonitor();

  // Responds to a TrafficMonitor no-network-routing failure.
  void OnNoNetworkRouting(int reason);

  // Initializes and configures the connection health checker.
  void SetupConnectionHealthChecker();

  // Checks the network connectivity status by creating a TCP connection, and
  // optionally sending a small amout of data.
  void RequestConnectionHealthCheck();

  // Responds to the result from connection health checker in a device specific
  // manner.
  void OnConnectionHealthCheckerResult(ConnectionHealthChecker::Result result);

  // Performs out-of-credits detection by checking to see if we're stuck in a
  // connect/disconnect loop.
  void DetectConnectDisconnectLoop(Service::ConnectState curr_state,
                                   Service::ConnectState new_state);
  // Reconnects to the cellular service in the context of out-of-credits
  // detection.
  void OutOfCreditsReconnect();

  // Ownership of |traffic_monitor| is taken.
  void set_traffic_monitor(TrafficMonitor* traffic_monitor);

  // Ownership of |healther_checker| is taken.
  void set_connection_health_checker(ConnectionHealthChecker* health_checker);

  base::WeakPtrFactory<ActivePassiveOutOfCreditsDetector> weak_ptr_factory_;

  // Passively monitors network traffic for network failures.
  std::unique_ptr<TrafficMonitor> traffic_monitor_;
  // Determine network health through active probes.
  std::unique_ptr<ConnectionHealthChecker> health_checker_;

  // The following members are used by the connect/disconnect loop detection.
  // Time when the last connect request started.
  base::Time connect_start_time_;
  // Number of connect attempts.
  int num_connect_attempts_;
  // Flag indicating whether out-of-credits detection is in progress.
  bool out_of_credits_detection_in_progress_;

  // String to hold service identifier for scoped logging.
  std::string service_rpc_identifier_;

  DISALLOW_COPY_AND_ASSIGN(ActivePassiveOutOfCreditsDetector);
};

}  // namespace shill

#endif  // SHILL_CELLULAR_ACTIVE_PASSIVE_OUT_OF_CREDITS_DETECTOR_H_
