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

#include <net/if_arp.h>

#include <string>

#include <base/bind.h>
#include <gtest/gtest.h>

#include "shill/arp_client_test_helper.h"
#include "shill/arp_packet.h"
#include "shill/logging.h"
#include "shill/mock_arp_client.h"
#include "shill/mock_connection.h"
#include "shill/mock_control.h"
#include "shill/mock_device_info.h"
#include "shill/mock_event_dispatcher.h"
#include "shill/mock_log.h"
#include "shill/mock_metrics.h"
#include "shill/net/byte_string.h"
#include "shill/net/ip_address.h"
#include "shill/net/mock_sockets.h"
#include "shill/net/mock_time.h"

using base::Bind;
using base::Unretained;
using std::string;
using testing::_;
using testing::AnyNumber;
using testing::HasSubstr;
using testing::Invoke;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::ReturnRef;
using testing::SetArgumentPointee;
using testing::StrictMock;
using testing::Test;

namespace shill {

namespace {
const char kInterfaceName[] = "int0";
const char kLocalIPAddress[] = "10.0.1.1";
const uint8_t kLocalMACAddress[] = { 0, 1, 2, 3, 4, 5 };
const char kRemoteIPAddress[] = "10.0.1.2";
const uint8_t kRemoteMACAddress[] = { 6, 7, 8, 9, 10, 11 };
const char kDBusPath[] = "/dbus/path";
}  // namespace


class ActiveLinkMonitorObserver {
 public:
  ActiveLinkMonitorObserver()
      : failure_callback_(
            Bind(&ActiveLinkMonitorObserver::OnFailureCallback,
                 Unretained(this))),
        success_callback_(
            Bind(&ActiveLinkMonitorObserver::OnSuccessCallback,
                 Unretained(this))) {}
  virtual ~ActiveLinkMonitorObserver() {}

  MOCK_METHOD3(OnFailureCallback,
               void(Metrics::LinkMonitorFailure failrue_code,
                    int broadcast_failure_count,
                    int unicast_failure_count));
  MOCK_METHOD0(OnSuccessCallback, void());

  const ActiveLinkMonitor::FailureCallback failure_callback() {
    return failure_callback_;
  }

  const ActiveLinkMonitor::SuccessCallback success_callback() {
    return success_callback_;
  }

 private:
  ActiveLinkMonitor::FailureCallback failure_callback_;
  ActiveLinkMonitor::SuccessCallback success_callback_;

  DISALLOW_COPY_AND_ASSIGN(ActiveLinkMonitorObserver);
};

MATCHER_P4(IsArpRequest, local_ip, remote_ip, local_mac, remote_mac, "") {
  if (local_ip.Equals(arg.local_ip_address()) &&
      remote_ip.Equals(arg.remote_ip_address()) &&
      local_mac.Equals(arg.local_mac_address()) &&
      remote_mac.Equals(arg.remote_mac_address()))
    return true;

  if (!local_ip.Equals(arg.local_ip_address())) {
    *result_listener << "Local IP '" << arg.local_ip_address().ToString()
                     << "' (wanted '" << local_ip.ToString() << "').";
  }

  if (!remote_ip.Equals(arg.remote_ip_address())) {
    *result_listener << "Remote IP '" << arg.remote_ip_address().ToString()
                     << "' (wanted '" << remote_ip.ToString() << "').";
  }

  if (!local_mac.Equals(arg.local_mac_address())) {
    *result_listener << "Local MAC '" << arg.local_mac_address().HexEncode()
                     << "' (wanted " << local_mac.HexEncode() << ")'.";
  }

  if (!remote_mac.Equals(arg.remote_mac_address())) {
    *result_listener << "Remote MAC '" << arg.remote_mac_address().HexEncode()
                     << "' (wanted " << remote_mac.HexEncode() << ")'.";
  }

  return false;
}

class ActiveLinkMonitorTest : public Test {
 public:
  ActiveLinkMonitorTest()
      : metrics_(&dispatcher_),
        device_info_(&control_, nullptr, nullptr, nullptr),
        connection_(new StrictMock<MockConnection>(&device_info_)),
        client_(new MockArpClient()),
        client_test_helper_(client_),
        gateway_ip_(IPAddress::kFamilyIPv4),
        local_ip_(IPAddress::kFamilyIPv4),
        gateway_mac_(kRemoteMACAddress, arraysize(kRemoteMACAddress)),
        local_mac_(kLocalMACAddress, arraysize(kLocalMACAddress)),
        zero_mac_(arraysize(kLocalMACAddress)),
        link_scope_logging_was_enabled_(false),
        interface_name_(kInterfaceName),
        monitor_(connection_,
                 &dispatcher_,
                 &metrics_,
                 &device_info_,
                 observer_.failure_callback(),
                 observer_.success_callback()) {}
  virtual ~ActiveLinkMonitorTest() {}

  virtual void SetUp() {
    link_scope_logging_was_enabled_ = SLOG_IS_ON(Link, 0);
    if (!link_scope_logging_was_enabled_) {
      ScopeLogger::GetInstance()->EnableScopesByName("link");
      ScopeLogger::GetInstance()->set_verbose_level(4);
    }
    monitor_.arp_client_.reset(client_);
    monitor_.time_ = &time_;
    time_val_.tv_sec = 0;
    time_val_.tv_usec = 0;
    EXPECT_CALL(time_, GetTimeMonotonic(_))
        .WillRepeatedly(DoAll(SetArgumentPointee<0>(time_val_), Return(0)));
    EXPECT_TRUE(local_ip_.SetAddressFromString(kLocalIPAddress));
    EXPECT_CALL(*connection_, local()).WillRepeatedly(ReturnRef(local_ip_));
    EXPECT_TRUE(gateway_ip_.SetAddressFromString(kRemoteIPAddress));
    EXPECT_CALL(*connection_, gateway()).WillRepeatedly(ReturnRef(gateway_ip_));
    EXPECT_CALL(*connection_, technology())
        .WillRepeatedly(Return(Technology::kEthernet));
    EXPECT_CALL(*connection_, ipconfig_rpc_identifier())
        .WillRepeatedly(testing::ReturnPointee(&kDBusPath));
    EXPECT_CALL(*connection_, interface_name())
        .WillRepeatedly(ReturnRef(interface_name_));
  }

  virtual void TearDown() {
    if (!link_scope_logging_was_enabled_) {
      ScopeLogger::GetInstance()->EnableScopesByName("-link");
      ScopeLogger::GetInstance()->set_verbose_level(0);
    }
  }

  void AdvanceTime(int time_ms) {
    struct timeval adv_time = {
      static_cast<time_t>(time_ms/1000),
      static_cast<time_t>((time_ms % 1000) * 1000) };
    timeradd(&time_val_, &adv_time, &time_val_);
    EXPECT_CALL(time_, GetTimeMonotonic(_))
        .WillRepeatedly(DoAll(SetArgumentPointee<0>(time_val_), Return(0)));
  }

  string HardwareAddressToString(const ByteString& address) {
    return ActiveLinkMonitor::HardwareAddressToString(address);
  }

 protected:
  void ExpectReset() {
    EXPECT_FALSE(monitor_.GetResponseTimeMilliseconds());
    EXPECT_TRUE(GetSendRequestCallback().IsCancelled());
    EXPECT_EQ(0, GetBroadcastFailureCount());
    EXPECT_EQ(0, GetUnicastFailureCount());
    EXPECT_EQ(0, GetBroadcastSuccessCount());
    EXPECT_EQ(0, GetUnicastSuccessCount());
    EXPECT_FALSE(IsUnicast());
    EXPECT_FALSE(GatewaySupportsUnicastArp());
  }
  void TriggerRequestTimer() {
    GetSendRequestCallback().callback().Run();
  }
  const base::CancelableClosure& GetSendRequestCallback() {
    return monitor_.send_request_callback_;
  }
  int GetBroadcastFailureCount() {
    return monitor_.broadcast_failure_count_;
  }
  int GetUnicastFailureCount() {
    return monitor_.unicast_failure_count_;
  }
  int GetBroadcastSuccessCount() {
    return monitor_.broadcast_success_count_;
  }
  int GetUnicastSuccessCount() {
    return monitor_.unicast_success_count_;
  }
  bool IsUnicast() { return monitor_.is_unicast_; }
  bool GatewaySupportsUnicastArp() {
    return monitor_.gateway_supports_unicast_arp_;
  }
  int GetCurrentTestPeriodMilliseconds() {
    return monitor_.test_period_milliseconds_;
  }
  int GetDefaultTestPeriodMilliseconds() {
    return ActiveLinkMonitor::kDefaultTestPeriodMilliseconds;
  }
  size_t GetFailureThreshold() {
    return ActiveLinkMonitor::kFailureThreshold;
  }
  size_t GetUnicastReplyReliabilityThreshold() {
    return ActiveLinkMonitor::kUnicastReplyReliabilityThreshold;
  }
  int GetFastTestPeriodMilliseconds() {
    return ActiveLinkMonitor::kFastTestPeriodMilliseconds;
  }
  int GetMaxResponseSampleFilterDepth() {
    return ActiveLinkMonitor::kMaxResponseSampleFilterDepth;
  }
  void ExpectTransmit(bool is_unicast, int transmit_period_milliseconds) {
    const ByteString& destination_mac = is_unicast ? gateway_mac_ : zero_mac_;
    EXPECT_CALL(*client_, TransmitRequest(
        IsArpRequest(local_ip_, gateway_ip_, local_mac_, destination_mac)))
        .WillOnce(Return(true));
    EXPECT_CALL(dispatcher_,
                PostDelayedTask(_, transmit_period_milliseconds));
  }
  void SendNextRequest() {
    EXPECT_CALL(*client_, TransmitRequest(_)).WillOnce(Return(true));
    EXPECT_CALL(dispatcher_,
                PostDelayedTask(_, GetCurrentTestPeriodMilliseconds()));
    TriggerRequestTimer();
  }
  void ExpectNoTransmit() {
    EXPECT_CALL(*client_, TransmitRequest(_)).Times(0);
  }
  void StartMonitor() {
    EXPECT_CALL(device_info_, GetMACAddress(0, _))
        .WillOnce(DoAll(SetArgumentPointee<1>(local_mac_), Return(true)));
    EXPECT_CALL(*client_, StartReplyListener()).WillOnce(Return(true));
    EXPECT_CALL(dispatcher_, PostTask(_)).Times(1);
    EXPECT_TRUE(monitor_.Start(
        ActiveLinkMonitor::kDefaultTestPeriodMilliseconds));
    EXPECT_FALSE(GetSendRequestCallback().IsCancelled());
  }
  void ReceiveResponse(uint16_t operation,
                       const IPAddress& local_ip,
                       const ByteString& local_mac,
                       const IPAddress& remote_ip,
                       const ByteString& remote_mac) {
    client_test_helper_.GeneratePacket(operation,
                                       local_ip,
                                       local_mac,
                                       remote_ip,
                                       remote_mac);
    monitor_.ReceiveResponse(0);
  }
  void ReceiveCorrectResponse() {
    ReceiveResponse(ARPOP_REPLY, gateway_ip_, gateway_mac_,
                    local_ip_, local_mac_);
  }
  void ReceiveReplyAndRestartMonitorCycle() {
    EXPECT_CALL(observer_, OnSuccessCallback()).Times(1);
    ReceiveCorrectResponse();
    Mock::VerifyAndClearExpectations(&observer_);
    StartMonitor();
  }
  void RunUnicastResponseCycle(int cycle_count,
                               bool should_respond_to_unicast_probes,
                               bool should_count_failures) {
    // This method expects the ActiveLinkMonitor to be in a state where it
    // is waiting for a broadcast response.  It also returns with the
    // ActiveLinkMonitor in the same state.
    // Successful receptions.
    EXPECT_CALL(metrics_, SendToUMA(
        HasSubstr("LinkMonitorResponseTimeSample"), 0, _, _, _))
        .Times(cycle_count * (should_respond_to_unicast_probes ? 2 : 1));
    // Unsuccessful unicast receptions.
    EXPECT_CALL(metrics_, SendToUMA(
        HasSubstr("LinkMonitorResponseTimeSample"),
        GetDefaultTestPeriodMilliseconds(),
        _, _, _)).Times(cycle_count *
                        (should_respond_to_unicast_probes ? 0 : 1));

    // Account for any successes / failures before we started.
    int expected_broadcast_success_count = GetBroadcastSuccessCount();
    int expected_unicast_success_count = GetUnicastSuccessCount();
    int expected_unicast_failure_count = GetUnicastFailureCount();

    LOG(INFO) << "RunUnicastResponseCycle: " << cycle_count;

    for (int i = 0; i < cycle_count; ++i) {
      // Respond to the pending broadcast request.
      ReceiveReplyAndRestartMonitorCycle();

      // Unicast ARP.
      ExpectTransmit(true, GetDefaultTestPeriodMilliseconds());
      TriggerRequestTimer();
      if (should_respond_to_unicast_probes) {
        ReceiveReplyAndRestartMonitorCycle();
      }

      // Initiate broadcast ARP.
      ExpectTransmit(false, GetDefaultTestPeriodMilliseconds());
      TriggerRequestTimer();

      ++expected_broadcast_success_count;
      if (should_respond_to_unicast_probes) {
        ++expected_unicast_success_count;
        expected_unicast_failure_count = 0;
      } else {
        if (should_count_failures) {
          ++expected_unicast_failure_count;
        }
        expected_unicast_success_count = 0;
      }
      EXPECT_EQ(expected_unicast_failure_count, GetUnicastFailureCount());
      EXPECT_EQ(expected_unicast_success_count, GetUnicastSuccessCount());
      EXPECT_EQ(0, GetBroadcastFailureCount());
      EXPECT_EQ(expected_broadcast_success_count, GetBroadcastSuccessCount());
    }
  }

  MockEventDispatcher dispatcher_;
  StrictMock<MockMetrics> metrics_;
  MockControl control_;
  NiceMock<MockDeviceInfo> device_info_;
  scoped_refptr<MockConnection> connection_;
  MockTime time_;
  struct timeval time_val_;
  // This is owned by the LinkMonitor, and only tracked here for EXPECT*().
  MockArpClient* client_;
  ArpClientTestHelper client_test_helper_;
  ActiveLinkMonitorObserver observer_;
  IPAddress gateway_ip_;
  IPAddress local_ip_;
  ByteString gateway_mac_;
  ByteString local_mac_;
  ByteString zero_mac_;
  bool link_scope_logging_was_enabled_;
  const string interface_name_;
  ActiveLinkMonitor monitor_;
};


TEST_F(ActiveLinkMonitorTest, Constructor) {
  ExpectReset();
}

TEST_F(ActiveLinkMonitorTest, StartFailedGetMACAddress) {
  ScopedMockLog log;
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(log,
      Log(logging::LOG_ERROR, _,
          HasSubstr("Could not get local MAC address"))).Times(1);
  EXPECT_CALL(device_info_, GetMACAddress(0, _)).WillOnce(Return(false));
  EXPECT_CALL(metrics_, SendEnumToUMA(
      HasSubstr("LinkMonitorFailure"), Metrics::kLinkMonitorMacAddressNotFound,
      _));
  EXPECT_CALL(*client_, StartReplyListener()).Times(0);
  EXPECT_FALSE(monitor_.Start(
      ActiveLinkMonitor::kDefaultTestPeriodMilliseconds));
  ExpectReset();
}

TEST_F(ActiveLinkMonitorTest, StartFailedArpClient) {
  ScopedMockLog log;
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(log,
      Log(logging::LOG_ERROR, _,
          HasSubstr("Failed to start ARP client"))).Times(1);
  EXPECT_CALL(metrics_, SendEnumToUMA(
      HasSubstr("LinkMonitorFailure"), Metrics::kLinkMonitorClientStartFailure,
      _));
  EXPECT_CALL(device_info_, GetMACAddress(0, _)).WillOnce(Return(true));
  EXPECT_CALL(*client_, StartReplyListener()).WillOnce(Return(false));
  EXPECT_FALSE(monitor_.Start(
      ActiveLinkMonitor::kDefaultTestPeriodMilliseconds));
  ExpectReset();
}

TEST_F(ActiveLinkMonitorTest, StartSuccess) {
  StartMonitor();
}

TEST_F(ActiveLinkMonitorTest, Stop) {
  StartMonitor();
  EXPECT_CALL(*client_, Stop()).Times(1);
  monitor_.Stop();
  ExpectReset();
  Mock::VerifyAndClearExpectations(client_);
}

TEST_F(ActiveLinkMonitorTest, ReplyReception) {
  StartMonitor();
  const int kResponseTime = 1234;
  AdvanceTime(kResponseTime);
  ScopedMockLog log;

  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(log, Log(_, _, HasSubstr("not for our IP"))).Times(1);
  ReceiveResponse(ARPOP_REPLY, gateway_ip_, gateway_mac_,
                  gateway_ip_, local_mac_);
  Mock::VerifyAndClearExpectations(&log);

  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(log, Log(_, _, HasSubstr("not for our MAC"))).Times(1);
  ReceiveResponse(ARPOP_REPLY, gateway_ip_, gateway_mac_,
                  local_ip_, gateway_mac_);
  Mock::VerifyAndClearExpectations(&log);

  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(log, Log(_, _, HasSubstr("not from the gateway"))).Times(1);
  ReceiveResponse(ARPOP_REPLY, local_ip_, gateway_mac_, local_ip_, local_mac_);
  Mock::VerifyAndClearExpectations(&log);

  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(log, Log(_, _, HasSubstr("This is not a reply packet"))).Times(1);
  ReceiveResponse(ARPOP_REQUEST, gateway_ip_, gateway_mac_,
                  local_ip_, local_mac_);
  Mock::VerifyAndClearExpectations(&log);

  EXPECT_FALSE(monitor_.GetResponseTimeMilliseconds());
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(log, Log(_, _, HasSubstr("Found gateway"))).Times(1);
  EXPECT_CALL(metrics_, SendToUMA(
      HasSubstr("LinkMonitorResponseTimeSample"), kResponseTime,
       _, _, _)).Times(1);
  EXPECT_CALL(*client_, Stop()).Times(1);
  EXPECT_CALL(observer_, OnSuccessCallback()).Times(1);
  ReceiveCorrectResponse();
  EXPECT_EQ(kResponseTime, monitor_.GetResponseTimeMilliseconds());
  EXPECT_TRUE(IsUnicast());
  Mock::VerifyAndClearExpectations(client_);
}

TEST_F(ActiveLinkMonitorTest, TimeoutBroadcast) {
  EXPECT_CALL(metrics_, SendToUMA(
      HasSubstr("LinkMonitorResponseTimeSample"),
      GetDefaultTestPeriodMilliseconds(),
      _, _, _)).Times(GetFailureThreshold());
  StartMonitor();
  // This value doesn't match real life (the timer in this scenario
  // should advance by LinkMonitor::kDefaultTestPeriodMilliseconds),
  // but this demonstrates the LinkMonitorSecondsToFailure independent
  // from the response-time figures.
  const int kTimeIncrement = 1000;
  // Transmit initial request.
  ExpectTransmit(false, GetDefaultTestPeriodMilliseconds());
  AdvanceTime(kTimeIncrement);
  TriggerRequestTimer();
  for (size_t i = 1; i < GetFailureThreshold(); ++i) {
    ExpectTransmit(false, GetDefaultTestPeriodMilliseconds());
    AdvanceTime(kTimeIncrement);
    TriggerRequestTimer();
    EXPECT_FALSE(IsUnicast());
    EXPECT_EQ(i, GetBroadcastFailureCount());
    EXPECT_EQ(0, GetUnicastFailureCount());
    EXPECT_EQ(0, GetBroadcastSuccessCount());
    EXPECT_EQ(0, GetUnicastSuccessCount());
    EXPECT_EQ(GetDefaultTestPeriodMilliseconds(),
              monitor_.GetResponseTimeMilliseconds());
  }
  ScopedMockLog log;
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(log,
      Log(logging::LOG_ERROR, _,
          HasSubstr("monitor has reached the failure threshold"))).Times(1);
  EXPECT_CALL(observer_,
              OnFailureCallback(Metrics::kLinkMonitorFailureThresholdReached,
                                GetFailureThreshold(),
                                0)).Times(1);
  EXPECT_FALSE(GetSendRequestCallback().IsCancelled());
  // Transmit final request.
  ExpectNoTransmit();
  AdvanceTime(kTimeIncrement);
  TriggerRequestTimer();
  ExpectReset();
}

TEST_F(ActiveLinkMonitorTest, TimeoutUnicast) {
  StartMonitor();

  // Setup expectation for Time::GetTimeMonotonic.
  const int kTimeIncrement = 1000;
  AdvanceTime(kTimeIncrement);
  // Initiate a broadcast ARP.
  ExpectTransmit(false, GetDefaultTestPeriodMilliseconds());
  TriggerRequestTimer();

  ScopedMockLog log;
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(log,
      Log(logging::LOG_ERROR, _,
          HasSubstr("monitor has reached the failure threshold"))).Times(0);

  // Unicast failures should not cause LinkMonitor errors if we haven't
  // noted the gateway as reliably replying to unicast ARP messages.  Test
  // this by doing threshold - 1 successful unicast responses, followed
  // by a ton of unicast failures.
  // Initiate broadcast ARP.
  RunUnicastResponseCycle(GetUnicastReplyReliabilityThreshold() - 1,
                          true, false);
  EXPECT_EQ(GetUnicastReplyReliabilityThreshold() - 1,
            GetUnicastSuccessCount());
  RunUnicastResponseCycle(GetFailureThreshold() +
                          GetUnicastReplyReliabilityThreshold(), false, false);
  EXPECT_FALSE(GetSendRequestCallback().IsCancelled());
  EXPECT_FALSE(GatewaySupportsUnicastArp());
  EXPECT_EQ(0, GetUnicastSuccessCount());
  EXPECT_EQ(0, GetUnicastFailureCount());

  // Cross the the unicast reliability threshold.
  RunUnicastResponseCycle(GetUnicastReplyReliabilityThreshold() - 1,
                          true, false);
  EXPECT_CALL(log,
      Log(_, _, HasSubstr("Unicast failures will now count")));
  EXPECT_FALSE(GatewaySupportsUnicastArp());
  RunUnicastResponseCycle(1, true, false);
  EXPECT_TRUE(GatewaySupportsUnicastArp());

  // Induce one less failures than will cause a link monitor failure, and
  // confirm that these failures are counted.
  RunUnicastResponseCycle(GetFailureThreshold() - 1, false, true);
  EXPECT_EQ(GetFailureThreshold() - 1, GetUnicastFailureCount());

  Mock::VerifyAndClearExpectations(&log);
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());

  // Induce a final broadcast success followed by a unicast failure.
  EXPECT_CALL(metrics_, SendToUMA(
      HasSubstr("LinkMonitorResponseTimeSample"), 0, _, _, _));
  ReceiveReplyAndRestartMonitorCycle();

  ExpectTransmit(true, GetDefaultTestPeriodMilliseconds());
  TriggerRequestTimer();
  EXPECT_FALSE(GetSendRequestCallback().IsCancelled());

  EXPECT_CALL(metrics_, SendToUMA(
      HasSubstr("LinkMonitorResponseTimeSample"),
      GetDefaultTestPeriodMilliseconds(),
      _, _, _));
  EXPECT_CALL(log,
      Log(logging::LOG_ERROR, _,
          HasSubstr("monitor has reached the failure threshold"))).Times(1);
  EXPECT_CALL(observer_,
              OnFailureCallback(Metrics::kLinkMonitorFailureThresholdReached,
                                0,
                                GetFailureThreshold())).Times(1);
  ExpectNoTransmit();
  TriggerRequestTimer();
  ExpectReset();
}

TEST_F(ActiveLinkMonitorTest, Average) {
  const int kSamples[] = { 200, 950, 1200, 4096, 5000,
                           86, 120, 3060, 842, 750 };
  const size_t filter_depth = GetMaxResponseSampleFilterDepth();
  EXPECT_CALL(metrics_, SendToUMA(
      HasSubstr("LinkMonitorResponseTimeSample"), _, _, _, _))
      .Times(arraysize(kSamples));
  ASSERT_GT(arraysize(kSamples), filter_depth);
  StartMonitor();
  size_t i = 0;
  int sum = 0;
  for (; i < filter_depth; ++i) {
    AdvanceTime(kSamples[i]);
    ReceiveReplyAndRestartMonitorCycle();
    sum += kSamples[i];
    EXPECT_EQ(sum / (i + 1), monitor_.GetResponseTimeMilliseconds());
    SendNextRequest();
  }
  for (; i < arraysize(kSamples); ++i) {
    AdvanceTime(kSamples[i]);
    ReceiveReplyAndRestartMonitorCycle();
    sum = (sum + kSamples[i]) * filter_depth / (filter_depth + 1);
    EXPECT_EQ(sum / filter_depth, monitor_.GetResponseTimeMilliseconds());
    SendNextRequest();
  }
}

TEST_F(ActiveLinkMonitorTest, ImpulseResponse) {
  const int kNormalValue = 50;
  const int kExceptionalValue = 5000;
  const int filter_depth = GetMaxResponseSampleFilterDepth();
  EXPECT_CALL(metrics_, SendToUMA(
      HasSubstr("LinkMonitorResponseTimeSample"), _, _, _, _))
      .Times(AnyNumber());
  StartMonitor();
  for (int i = 0; i < filter_depth * 2; ++i) {
    AdvanceTime(kNormalValue);
    ReceiveReplyAndRestartMonitorCycle();
    EXPECT_EQ(kNormalValue, monitor_.GetResponseTimeMilliseconds());
    SendNextRequest();
  }
  AdvanceTime(kExceptionalValue);
  ReceiveReplyAndRestartMonitorCycle();
  // Our expectation is that an impulse input will be a
  // impulse_height / (filter_depth + 1) increase to the running average.
  int expected_impulse_response =
      kNormalValue + (kExceptionalValue - kNormalValue) / (filter_depth + 1);
  EXPECT_EQ(expected_impulse_response, monitor_.GetResponseTimeMilliseconds());
  SendNextRequest();

  // From here, if we end up continuing to receive normal values, our
  // running average should decay backwards to the normal value.
  const int failsafe = 100;
  int last_value = monitor_.GetResponseTimeMilliseconds();
  for (int i = 0; i < failsafe && last_value != kNormalValue; ++i) {
    AdvanceTime(kNormalValue);
    ReceiveReplyAndRestartMonitorCycle();
    // We should advance monotonically (but not necessarily linearly)
    // back towards the normal value.
    EXPECT_GE(last_value, monitor_.GetResponseTimeMilliseconds());
    SendNextRequest();
    last_value = monitor_.GetResponseTimeMilliseconds();
  }
  EXPECT_EQ(kNormalValue, last_value);
}

TEST_F(ActiveLinkMonitorTest, HardwareAddressToString) {
  const uint8_t address0[] = { 0, 1, 2, 3, 4, 5 };
  EXPECT_EQ("00:01:02:03:04:05",
            HardwareAddressToString(ByteString(address0, arraysize(address0))));
  const uint8_t address1[] = { 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd };
  EXPECT_EQ("88:99:aa:bb:cc:dd",
            HardwareAddressToString(ByteString(address1, arraysize(address1))));
}

}  // namespace shill
