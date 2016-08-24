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
#include <vector>

#include <gtest/gtest.h>

#include "shill/cellular/mock_cellular.h"
#include "shill/cellular/mock_cellular_service.h"
#include "shill/cellular/mock_modem_info.h"
#include "shill/mock_connection.h"
#include "shill/mock_connection_health_checker.h"
#include "shill/mock_device_info.h"
#include "shill/mock_manager.h"
#include "shill/mock_traffic_monitor.h"
#include "shill/test_event_dispatcher.h"

using base::Bind;
using base::Unretained;
using std::string;
using std::vector;
using testing::_;
using testing::AnyNumber;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::ReturnPointee;
using testing::ReturnRef;
using testing::StrictMock;

namespace shill {

class ActivePassiveOutOfCreditsDetectorTest : public testing::Test {
 public:
  ActivePassiveOutOfCreditsDetectorTest()
      : modem_info_(nullptr, &dispatcher_, &metrics_, &manager_),
        device_info_(modem_info_.control_interface(), modem_info_.dispatcher(),
                     modem_info_.metrics(), modem_info_.manager()),
        manager_(modem_info_.control_interface(), modem_info_.dispatcher(),
                 modem_info_.metrics()),
        metrics_(modem_info_.dispatcher()),
        cellular_(new NiceMock<MockCellular>(&modem_info_,
                                             "usb0",
                                             kAddress,
                                             3,
                                             Cellular::kTypeCDMA,
                                             "",
                                             "")),
        service_(new NiceMock<MockCellularService>(&modem_info_, cellular_)),
        connection_(new NiceMock<MockConnection>(&device_info_)),
        out_of_credits_detector_(
            new ActivePassiveOutOfCreditsDetector(
                modem_info_.dispatcher(), modem_info_.manager(),
                modem_info_.metrics(), service_.get())) {}

  virtual void SetUp() {
    service_->connection_ = connection_;
    cellular_->service_ = service_;
    service_->SetRoamingState(kRoamingStateHome);
    ON_CALL(*connection_, interface_name())
        .WillByDefault(ReturnRef(interface_name_));
    ON_CALL(*connection_, dns_servers())
        .WillByDefault(ReturnRef(dns_servers_));
    ON_CALL(manager_, GetPortalCheckURL())
        .WillByDefault(ReturnRef(portal_check_url_));
    ON_CALL(*service_, explicitly_disconnected()).WillByDefault(Return(false));
    ON_CALL(*service_, resume_start_time())
        .WillByDefault(ReturnRef(resume_start_time_));
  }

  virtual void TearDown() {
    cellular_->service_ = nullptr;  // Break circular reference.
  }

  void OnConnectionHealthCheckerResult(
      ConnectionHealthChecker::Result result) {}

 protected:
  static const char kAddress[];

  void SetMockServiceState(Service::ConnectState old_state,
                           Service::ConnectState new_state) {
    out_of_credits_detector_->NotifyServiceStateChanged(old_state, new_state);
  }

  void SetTrafficMonitor(TrafficMonitor* traffic_monitor) {
    out_of_credits_detector_->set_traffic_monitor(traffic_monitor);
  }

  void SetConnectionHealthChecker(ConnectionHealthChecker* health_checker) {
    out_of_credits_detector_->set_connection_health_checker(health_checker);
  }

  EventDispatcherForTest dispatcher_;
  MockModemInfo modem_info_;
  NiceMock<MockDeviceInfo> device_info_;
  NiceMock<MockManager> manager_;
  NiceMock<MockMetrics> metrics_;
  scoped_refptr<NiceMock<MockCellular>> cellular_;
  scoped_refptr<NiceMock<MockCellularService>> service_;
  scoped_refptr<NiceMock<MockConnection>> connection_;
  string interface_name_;
  vector<string> dns_servers_;
  string portal_check_url_;
  base::Time resume_start_time_;
  std::unique_ptr<ActivePassiveOutOfCreditsDetector> out_of_credits_detector_;
};

const char ActivePassiveOutOfCreditsDetectorTest::kAddress[] = "000102030405";

TEST_F(ActivePassiveOutOfCreditsDetectorTest,
    ConnectDisconnectLoopOutOfCreditsDetected) {
  EXPECT_CALL(*service_, Connect(_, _)).Times(2);
  SetMockServiceState(Service::kStateIdle, Service::kStateAssociating);
  SetMockServiceState(Service::kStateAssociating, Service::kStateConnected);
  SetMockServiceState(Service::kStateConnected, Service::kStateFailure);
  EXPECT_TRUE(out_of_credits_detector_->IsDetecting());
  dispatcher_.DispatchPendingEvents();
  SetMockServiceState(Service::kStateIdle, Service::kStateAssociating);
  SetMockServiceState(Service::kStateAssociating, Service::kStateConfiguring);
  SetMockServiceState(Service::kStateConfiguring, Service::kStateIdle);
  EXPECT_TRUE(out_of_credits_detector_->IsDetecting());
  dispatcher_.DispatchPendingEvents();
  SetMockServiceState(Service::kStateIdle, Service::kStateAssociating);
  SetMockServiceState(Service::kStateAssociating, Service::kStateConnected);
  SetMockServiceState(Service::kStateConnected, Service::kStateIdle);
  EXPECT_TRUE(out_of_credits_detector_->out_of_credits());
  EXPECT_FALSE(out_of_credits_detector_->IsDetecting());
}

TEST_F(ActivePassiveOutOfCreditsDetectorTest,
    ConnectDisconnectLoopDetectionNotSkippedAfterSlowResume) {
  resume_start_time_ =
      base::Time::Now() -
      base::TimeDelta::FromSeconds(
      ActivePassiveOutOfCreditsDetector::kOutOfCreditsResumeIgnoreSeconds + 1);
  EXPECT_CALL(*service_, Connect(_, _)).Times(2);
  SetMockServiceState(Service::kStateIdle, Service::kStateAssociating);
  SetMockServiceState(Service::kStateAssociating, Service::kStateFailure);
  EXPECT_TRUE(out_of_credits_detector_->IsDetecting());
  dispatcher_.DispatchPendingEvents();
  SetMockServiceState(Service::kStateIdle, Service::kStateAssociating);
  SetMockServiceState(Service::kStateAssociating, Service::kStateConfiguring);
  SetMockServiceState(Service::kStateConfiguring, Service::kStateIdle);
  EXPECT_TRUE(out_of_credits_detector_->IsDetecting());
  dispatcher_.DispatchPendingEvents();
  SetMockServiceState(Service::kStateIdle, Service::kStateAssociating);
  SetMockServiceState(Service::kStateAssociating, Service::kStateConnected);
  SetMockServiceState(Service::kStateConnected, Service::kStateIdle);
  EXPECT_TRUE(out_of_credits_detector_->out_of_credits());
  EXPECT_FALSE(out_of_credits_detector_->IsDetecting());
}

TEST_F(ActivePassiveOutOfCreditsDetectorTest,
    ConnectDisconnectLoopDetectionSkippedAfterResume) {
  resume_start_time_ = base::Time::Now();
  ON_CALL(*service_, resume_start_time())
      .WillByDefault(ReturnRef(resume_start_time_));
  EXPECT_CALL(*service_, Connect(_, _)).Times(0);
  SetMockServiceState(Service::kStateIdle, Service::kStateAssociating);
  SetMockServiceState(Service::kStateAssociating, Service::kStateConnected);
  SetMockServiceState(Service::kStateConnected, Service::kStateIdle);
  EXPECT_FALSE(out_of_credits_detector_->out_of_credits());
  EXPECT_FALSE(out_of_credits_detector_->IsDetecting());
  // There should not be any pending connect requests but dispatch pending
  // events anyway to be sure.
  dispatcher_.DispatchPendingEvents();
}

TEST_F(ActivePassiveOutOfCreditsDetectorTest,
    ConnectDisconnectLoopDetectionSkippedAlreadyOutOfCredits) {
  EXPECT_CALL(*service_, Connect(_, _)).Times(0);
  out_of_credits_detector_->ReportOutOfCredits(true);
  SetMockServiceState(Service::kStateAssociating, Service::kStateConnected);
  SetMockServiceState(Service::kStateConnected, Service::kStateIdle);
  EXPECT_FALSE(out_of_credits_detector_->IsDetecting());
  // There should not be any pending connect requests but dispatch pending
  // events anyway to be sure.
  dispatcher_.DispatchPendingEvents();
}

TEST_F(ActivePassiveOutOfCreditsDetectorTest,
    ConnectDisconnectLoopDetectionSkippedExplicitDisconnect) {
  EXPECT_CALL(*service_, Connect(_, _)).Times(0);
  SetMockServiceState(Service::kStateIdle, Service::kStateAssociating);
  SetMockServiceState(Service::kStateAssociating, Service::kStateConnected);
  EXPECT_CALL(*service_, explicitly_disconnected()).WillOnce(Return(true));
  SetMockServiceState(Service::kStateConnected, Service::kStateIdle);
  EXPECT_FALSE(out_of_credits_detector_->out_of_credits());
  EXPECT_FALSE(out_of_credits_detector_->IsDetecting());
  // There should not be any pending connect requests but dispatch pending
  // events anyway to be sure.
  dispatcher_.DispatchPendingEvents();
}

TEST_F(ActivePassiveOutOfCreditsDetectorTest,
    ConnectDisconnectLoopDetectionConnectionNotDropped) {
  EXPECT_CALL(*service_, Connect(_, _)).Times(0);
  SetMockServiceState(Service::kStateIdle, Service::kStateAssociating);
  SetMockServiceState(Service::kStateAssociating, Service::kStateConfiguring);
  SetMockServiceState(Service::kStateConfiguring, Service::kStateConnected);
  EXPECT_FALSE(out_of_credits_detector_->out_of_credits());
  EXPECT_FALSE(out_of_credits_detector_->IsDetecting());
  // There should not be any pending connect requests but dispatch pending
  // events anyway to be sure.
  dispatcher_.DispatchPendingEvents();
}

TEST_F(ActivePassiveOutOfCreditsDetectorTest,
    ConnectDisconnectLoopDetectionIntermittentNetwork) {
  EXPECT_CALL(*service_, Connect(_, _)).Times(0);
  SetMockServiceState(Service::kStateAssociating, Service::kStateConnected);
  out_of_credits_detector_->connect_start_time_ =
      base::Time::Now() -
      base::TimeDelta::FromSeconds(
          ActivePassiveOutOfCreditsDetector::
          kOutOfCreditsConnectionDropSeconds + 1);
  SetMockServiceState(Service::kStateConnected, Service::kStateIdle);
  EXPECT_FALSE(out_of_credits_detector_->out_of_credits());
  EXPECT_FALSE(out_of_credits_detector_->IsDetecting());
  // There should not be any pending connect requests but dispatch pending
  // events anyway to be sure.
  dispatcher_.DispatchPendingEvents();
}

TEST_F(ActivePassiveOutOfCreditsDetectorTest, StartTrafficMonitor) {
  MockTrafficMonitor* traffic_monitor = new StrictMock<MockTrafficMonitor>();
  SetTrafficMonitor(traffic_monitor);  // Passes ownership.

  // Traffic monitor should only start when the service is connected.
  EXPECT_CALL(*traffic_monitor, Start()).Times(1);
  SetMockServiceState(Service::kStateAssociating, Service::kStateConnected);
  Mock::VerifyAndClearExpectations(traffic_monitor);

  // Traffic monitor should not start for other state transitions.
  EXPECT_CALL(*traffic_monitor, Start()).Times(0);
  EXPECT_CALL(*traffic_monitor, Stop()).Times(AnyNumber());
  SetMockServiceState(Service::kStateConnected, Service::kStateIdle);
  SetMockServiceState(Service::kStateIdle, Service::kStateConfiguring);
  SetMockServiceState(Service::kStateConfiguring, Service::kStateFailure);
  SetMockServiceState(Service::kStateIdle, Service::kStateAssociating);
  SetMockServiceState(Service::kStateConfiguring, Service::kStatePortal);
  SetMockServiceState(Service::kStatePortal, Service::kStateOnline);
}

TEST_F(ActivePassiveOutOfCreditsDetectorTest, StopTrafficMonitor) {
  // Traffic monitor should stop when the service is disconnected.
  MockTrafficMonitor* traffic_monitor = new StrictMock<MockTrafficMonitor>();
  SetTrafficMonitor(traffic_monitor);  // Passes ownership.
  EXPECT_CALL(*traffic_monitor, Start());
  EXPECT_CALL(*traffic_monitor, Stop());
  SetMockServiceState(Service::kStateAssociating, Service::kStateConnected);
  SetMockServiceState(Service::kStateConnected, Service::kStateIdle);
  Mock::VerifyAndClearExpectations(traffic_monitor);

  EXPECT_CALL(*traffic_monitor, Start());
  EXPECT_CALL(*traffic_monitor, Stop());
  SetMockServiceState(Service::kStateIdle, Service::kStateConnected);
  SetMockServiceState(Service::kStateConnected, Service::kStateFailure);
  Mock::VerifyAndClearExpectations(traffic_monitor);

  // Need an additional call to Stop() because |traffic_monitor| destructor
  // will call stop.
  EXPECT_CALL(*traffic_monitor, Stop());
}

TEST_F(ActivePassiveOutOfCreditsDetectorTest, OnNoNetworkRouting) {
  // Make sure the connection health checker starts when there is no network
  // routing.
  EXPECT_FALSE(out_of_credits_detector_->out_of_credits());
  MockConnectionHealthChecker* health_checker =
      new MockConnectionHealthChecker(
          service_->connection(),
          modem_info_.dispatcher(),
          manager_.health_checker_remote_ips(),
          Bind(&ActivePassiveOutOfCreditsDetectorTest::
               OnConnectionHealthCheckerResult,
               Unretained(this)));
  SetConnectionHealthChecker(health_checker);  // Passes ownership.
  EXPECT_CALL(*health_checker, Start());
  out_of_credits_detector_->OnNoNetworkRouting(0);
  EXPECT_FALSE(out_of_credits_detector_->out_of_credits());
  Mock::VerifyAndClearExpectations(health_checker);

  // Make sure connection health checker does not start again if there is a
  // health check in progress.
  EXPECT_CALL(*health_checker, health_check_in_progress())
      .WillOnce(Return(true));
  EXPECT_CALL(*health_checker, Start()).Times(0);
  out_of_credits_detector_->OnNoNetworkRouting(0);
}

TEST_F(ActivePassiveOutOfCreditsDetectorTest,
    OnConnectionHealthCheckerResult) {
  EXPECT_FALSE(out_of_credits_detector_->out_of_credits());
  EXPECT_CALL(*service_, Disconnect(_, _)).Times(0);
  out_of_credits_detector_->OnConnectionHealthCheckerResult(
      ConnectionHealthChecker::kResultUnknown);
  EXPECT_FALSE(out_of_credits_detector_->out_of_credits());
  out_of_credits_detector_->OnConnectionHealthCheckerResult(
      ConnectionHealthChecker::kResultConnectionFailure);
  EXPECT_FALSE(out_of_credits_detector_->out_of_credits());
  Mock::VerifyAndClearExpectations(service_.get());

  EXPECT_CALL(*service_, Disconnect(_,
      ::testing::StrEq("out-of-credits"))).
          Times(1);
  out_of_credits_detector_->OnConnectionHealthCheckerResult(
      ConnectionHealthChecker::kResultCongestedTxQueue);
  EXPECT_TRUE(out_of_credits_detector_->out_of_credits());
}

}  // namespace shill
