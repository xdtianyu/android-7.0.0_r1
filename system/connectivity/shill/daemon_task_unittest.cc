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

#include <stdint.h>

#include <memory>
#include <vector>

#include <base/bind.h>
#include <base/memory/ref_counted.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/daemon_task.h"
#include "shill/dhcp/mock_dhcp_provider.h"
#include "shill/logging.h"
#include "shill/mock_control.h"
#include "shill/mock_manager.h"
#include "shill/mock_metrics.h"
#include "shill/mock_process_manager.h"
#include "shill/mock_routing_table.h"
#include "shill/net/io_handler.h"
#include "shill/net/mock_rtnl_handler.h"
#include "shill/net/ndisc.h"
#include "shill/shill_test_config.h"
#include "shill/test_event_dispatcher.h"

#if !defined(DISABLE_WIFI)
#include "shill/net/mock_netlink_manager.h"
#include "shill/net/nl80211_message.h"
#include "shill/wifi/callback80211_metrics.h"
#endif  // DISABLE_WIFI

using base::Bind;
using base::Callback;
using base::Unretained;
using std::string;
using std::vector;

using ::testing::Expectation;
using ::testing::Mock;
using ::testing::Return;
using ::testing::Test;
using ::testing::_;

namespace shill {

class DaemonTaskForTest : public DaemonTask {
 public:
  DaemonTaskForTest(const Settings& setttings, Config* config)
      : DaemonTask(Settings(), config) {}
  virtual ~DaemonTaskForTest() {}

  bool quit_result() { return quit_result_; }

  void RunMessageLoop() { dispatcher_->DispatchForever(); }

  bool Quit(const base::Closure& completion_callback) override {
    quit_result_ = DaemonTask::Quit(completion_callback);
    dispatcher_->PostTask(base::MessageLoop::QuitWhenIdleClosure());
    return quit_result_;
  }

 private:
  bool quit_result_;
};

class DaemonTaskTest : public Test {
 public:
  DaemonTaskTest()
      : daemon_(DaemonTask::Settings(), &config_),
        dispatcher_(new EventDispatcherForTest()),
        control_(new MockControl()),
        metrics_(new MockMetrics(dispatcher_)),
        manager_(new MockManager(control_, dispatcher_, metrics_)),
#if !defined(DISABLE_WIFI)
        callback_metrics_(new Callback80211Metrics(metrics_)),
#endif  // DISABLE_WIFI
        device_info_(control_, dispatcher_, metrics_, manager_) {
  }
  virtual ~DaemonTaskTest() {}
  virtual void SetUp() {
    // Tests initialization done by the daemon's constructor
    daemon_.rtnl_handler_ = &rtnl_handler_;
    daemon_.routing_table_ = &routing_table_;
    daemon_.dhcp_provider_ = &dhcp_provider_;
    daemon_.process_manager_ = &process_manager_;
    daemon_.metrics_.reset(metrics_);        // Passes ownership
    daemon_.manager_.reset(manager_);        // Passes ownership
    daemon_.control_.reset(control_);        // Passes ownership
    daemon_.dispatcher_.reset(dispatcher_);  // Passes ownership

#if !defined(DISABLE_WIFI)
    daemon_.netlink_manager_ = &netlink_manager_;
    // Passes ownership
    daemon_.callback80211_metrics_.reset(callback_metrics_);
#endif  // DISABLE_WIFI
  }
  void StartDaemon() { daemon_.Start(); }

  void StopDaemon() { daemon_.Stop(); }

  void RunDaemon() { daemon_.RunMessageLoop(); }

  void ApplySettings(const DaemonTask::Settings& settings) {
    daemon_.settings_ = settings;
    daemon_.ApplySettings();
  }

  MOCK_METHOD0(TerminationAction, void());
  MOCK_METHOD0(BreakTerminationLoop, void());

 protected:
  TestConfig config_;
  DaemonTaskForTest daemon_;
  MockRTNLHandler rtnl_handler_;
  MockRoutingTable routing_table_;
  MockDHCPProvider dhcp_provider_;
  MockProcessManager process_manager_;
  EventDispatcherForTest* dispatcher_;
  MockControl* control_;
  MockMetrics* metrics_;
  MockManager* manager_;
#if !defined(DISABLE_WIFI)
  MockNetlinkManager netlink_manager_;
  Callback80211Metrics* callback_metrics_;
#endif  // DISABLE_WIFI
  DeviceInfo device_info_;
};

TEST_F(DaemonTaskTest, StartStop) {
  // To ensure we do not have any stale routes, we flush a device's routes
  // when it is started.  This requires that the routing table is fully
  // populated before we create and start devices.  So test to make sure that
  // the RoutingTable starts before the Manager (which in turn starts
  // DeviceInfo who is responsible for creating and starting devices).
  // The result is that we request the dump of the routing table and when that
  // completes, we request the dump of the links.  For each link found, we
  // create and start the device.
  EXPECT_CALL(*metrics_, Start());
  EXPECT_CALL(rtnl_handler_, Start(RTMGRP_LINK | RTMGRP_IPV4_IFADDR |
                                   RTMGRP_IPV4_ROUTE | RTMGRP_IPV6_IFADDR |
                                   RTMGRP_IPV6_ROUTE | RTMGRP_ND_USEROPT));
  Expectation routing_table_started = EXPECT_CALL(routing_table_, Start());
  EXPECT_CALL(dhcp_provider_, Init(_, _, _));
  EXPECT_CALL(process_manager_, Init(_));
#if !defined(DISABLE_WIFI)
  EXPECT_CALL(netlink_manager_, Init());
  const uint16_t kNl80211MessageType = 42;  // Arbitrary.
  EXPECT_CALL(netlink_manager_,
              GetFamily(Nl80211Message::kMessageTypeString, _))
      .WillOnce(Return(kNl80211MessageType));
  EXPECT_CALL(netlink_manager_, Start());
#endif  // DISABLE_WIFI
  EXPECT_CALL(*manager_, Start()).After(routing_table_started);
  StartDaemon();
  Mock::VerifyAndClearExpectations(metrics_);
  Mock::VerifyAndClearExpectations(manager_);

  EXPECT_CALL(*manager_, Stop());
  EXPECT_CALL(*metrics_, Stop());
  EXPECT_CALL(process_manager_, Stop());
  StopDaemon();
}

ACTION_P2(CompleteAction, manager, name) {
  manager->TerminationActionComplete(name);
}

TEST_F(DaemonTaskTest, QuitWithTerminationAction) {
  // This expectation verifies that the termination actions are invoked.
  EXPECT_CALL(*this, TerminationAction())
      .WillOnce(CompleteAction(manager_, "daemon test"));
  EXPECT_CALL(*this, BreakTerminationLoop()).Times(1);

  manager_->AddTerminationAction(
      "daemon test",
      Bind(&DaemonTaskTest::TerminationAction, Unretained(this)));

  // Run Daemon::Quit() after the daemon starts running.
  dispatcher_->PostTask(
      Bind(IgnoreResult(&DaemonTask::Quit), Unretained(&daemon_),
           Bind(&DaemonTaskTest::BreakTerminationLoop, Unretained(this))));

  RunDaemon();
  EXPECT_FALSE(daemon_.quit_result());
}

TEST_F(DaemonTaskTest, QuitWithoutTerminationActions) {
  EXPECT_CALL(*this, BreakTerminationLoop()).Times(0);
  EXPECT_TRUE(daemon_.Quit(
      Bind(&DaemonTaskTest::BreakTerminationLoop, Unretained(this))));
}

TEST_F(DaemonTaskTest, ApplySettings) {
  DaemonTask::Settings settings;
  vector<string> kEmptyStringList;
  EXPECT_CALL(*manager_, SetBlacklistedDevices(kEmptyStringList));
  EXPECT_CALL(*manager_, SetDHCPv6EnabledDevices(kEmptyStringList));
  EXPECT_CALL(*manager_, SetTechnologyOrder("", _));
  EXPECT_CALL(*manager_, SetIgnoreUnknownEthernet(false));
  EXPECT_CALL(*manager_, SetStartupPortalList(_)).Times(0);
  EXPECT_CALL(*manager_, SetPassiveMode()).Times(0);
  EXPECT_CALL(*manager_, SetPrependDNSServers(""));
  EXPECT_CALL(*manager_, SetMinimumMTU(_)).Times(0);
  EXPECT_CALL(*manager_, SetAcceptHostnameFrom(""));
  ApplySettings(settings);
  Mock::VerifyAndClearExpectations(manager_);

  vector<string> kBlacklistedDevices = {"eth0", "eth1"};
  settings.device_blacklist = kBlacklistedDevices;
  settings.default_technology_order = "wifi,ethernet";
  vector<string> kDHCPv6EnabledDevices{"eth2", "eth3"};
  settings.dhcpv6_enabled_devices = kDHCPv6EnabledDevices;
  settings.ignore_unknown_ethernet = false;
  settings.portal_list = "wimax";
  settings.use_portal_list = true;
  settings.passive_mode = true;
  settings.prepend_dns_servers = "8.8.8.8,8.8.4.4";
  settings.minimum_mtu = 256;
  settings.accept_hostname_from = "eth*";
  EXPECT_CALL(*manager_, SetBlacklistedDevices(kBlacklistedDevices));
  EXPECT_CALL(*manager_, SetDHCPv6EnabledDevices(kDHCPv6EnabledDevices));
  EXPECT_CALL(*manager_, SetTechnologyOrder("wifi,ethernet", _));
  EXPECT_CALL(*manager_, SetIgnoreUnknownEthernet(false));
  EXPECT_CALL(*manager_, SetStartupPortalList("wimax"));
  EXPECT_CALL(*manager_, SetPassiveMode());
  EXPECT_CALL(*manager_, SetPrependDNSServers("8.8.8.8,8.8.4.4"));
  EXPECT_CALL(*manager_, SetMinimumMTU(256));
  EXPECT_CALL(*manager_, SetAcceptHostnameFrom("eth*"));
  ApplySettings(settings);
  Mock::VerifyAndClearExpectations(manager_);
}

}  // namespace shill
