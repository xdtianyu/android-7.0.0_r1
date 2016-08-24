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

#include <net/if_arp.h>

#include <string>

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
#include "shill/net/byte_string.h"
#include "shill/net/ip_address.h"

using base::Bind;
using base::Unretained;
using std::string;
using testing::_;
using testing::AnyNumber;
using testing::HasSubstr;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::ReturnRef;
using testing::StrictMock;
using testing::Test;

namespace shill {

namespace {
const char kInterfaceName[] = "test-interface";
const char kLocalIPAddress[] = "10.0.1.1";
const uint8_t kLocalMACAddress[] = { 0, 1, 2, 3, 4, 5 };
const char kRemoteIPAddress[] = "10.0.1.2";
const uint8_t kRemoteMACAddress[] = { 6, 7, 8, 9, 10, 11 };
}  // namespace

class ResultCallbackObserver {
 public:
  ResultCallbackObserver()
      : result_callback_(
          Bind(&ResultCallbackObserver::OnResultCallback,
               Unretained(this))) {}
  virtual ~ResultCallbackObserver() {}

  MOCK_METHOD1(OnResultCallback, void(bool status));

  const PassiveLinkMonitor::ResultCallback result_callback() {
    return result_callback_;
  }

 private:
  PassiveLinkMonitor::ResultCallback result_callback_;

  DISALLOW_COPY_AND_ASSIGN(ResultCallbackObserver);
};

class PassiveLinkMonitorTest : public Test {
 public:
  PassiveLinkMonitorTest()
      : device_info_(&control_, nullptr, nullptr, nullptr),
        connection_(new StrictMock<MockConnection>(&device_info_)),
        client_(new MockArpClient()),
        client_test_helper_(client_),
        link_monitor_(connection_, &dispatcher_, observer_.result_callback()),
        interface_name_(kInterfaceName) {}
  virtual ~PassiveLinkMonitorTest() {}

  virtual void SetUp() {
    ScopeLogger::GetInstance()->EnableScopesByName("link");
    ScopeLogger::GetInstance()->set_verbose_level(4);
    link_monitor_.arp_client_.reset(client_);

    EXPECT_CALL(*connection_, interface_name())
        .WillRepeatedly(ReturnRef(interface_name_));
  }

  virtual void TearDown() {
    ScopeLogger::GetInstance()->EnableScopesByName("-link");
    ScopeLogger::GetInstance()->set_verbose_level(0);
  }

  void ReceiveArpPacket(uint16_t operation) {
    client_test_helper_.GeneratePacket(
        operation,
        IPAddress(kLocalIPAddress),
        ByteString(kLocalMACAddress, arraysize(kLocalMACAddress)),
        IPAddress(kRemoteIPAddress),
        ByteString(kRemoteMACAddress, arraysize(kRemoteMACAddress)));
    link_monitor_.ReceiveRequest(0);
  }

  void MonitorCompleted(bool status) {
    link_monitor_.MonitorCompleted(status);
  }

  void InvokeCycleTimeoutHandler() {
    link_monitor_.CycleTimeoutHandler();
  }

  void SetCurrentCycleStats(int num_requests_received, int num_cycles_passed) {
    link_monitor_.num_requests_received_ = num_requests_received;
    link_monitor_.num_cycles_passed_ = num_cycles_passed;
  }

  void VerifyCurrentCycleStats(int num_requests_received,
                               int num_cycles_passed) {
    EXPECT_EQ(num_requests_received, link_monitor_.num_requests_received_);
    EXPECT_EQ(num_cycles_passed, link_monitor_.num_cycles_passed_);
  }

 protected:
  MockEventDispatcher dispatcher_;
  MockControl control_;
  NiceMock<MockDeviceInfo> device_info_;
  ResultCallbackObserver observer_;
  scoped_refptr<MockConnection> connection_;
  MockArpClient* client_;
  ArpClientTestHelper client_test_helper_;
  PassiveLinkMonitor link_monitor_;
  const string interface_name_;
};

TEST_F(PassiveLinkMonitorTest, StartFailedArpClient) {
  EXPECT_CALL(*client_, StartRequestListener()).WillOnce(Return(false));
  EXPECT_FALSE(link_monitor_.Start(PassiveLinkMonitor::kDefaultMonitorCycles));
}

TEST_F(PassiveLinkMonitorTest, StartSuccess) {
  EXPECT_CALL(*client_, StartRequestListener()).WillOnce(Return(true));
  EXPECT_CALL(dispatcher_, CreateReadyHandler(_, IOHandler::kModeInput, _))
      .Times(1);
  EXPECT_CALL(dispatcher_, PostDelayedTask(_, _)).Times(1);
  EXPECT_TRUE(link_monitor_.Start(PassiveLinkMonitor::kDefaultMonitorCycles));
}

TEST_F(PassiveLinkMonitorTest, Stop) {
  EXPECT_CALL(*client_, Stop()).Times(1);
  link_monitor_.Stop();
  Mock::VerifyAndClearExpectations(client_);
}

TEST_F(PassiveLinkMonitorTest, MonitorCompleted) {
  // Monitor failed.
  EXPECT_CALL(*client_, Stop()).Times(1);
  EXPECT_CALL(observer_, OnResultCallback(false)).Times(1);
  MonitorCompleted(false);
  Mock::VerifyAndClearExpectations(client_);
  Mock::VerifyAndClearExpectations(&observer_);

  // Monitor succeed.
  EXPECT_CALL(*client_, Stop()).Times(1);
  EXPECT_CALL(observer_, OnResultCallback(true)).Times(1);
  MonitorCompleted(true);
  Mock::VerifyAndClearExpectations(client_);
  Mock::VerifyAndClearExpectations(&observer_);
}

TEST_F(PassiveLinkMonitorTest, ReceiveArpReply) {
  // Setup initial stats.
  const int kRequestReceived = 0;
  const int kCurrentCycle = 0;
  SetCurrentCycleStats(kRequestReceived, kCurrentCycle);
  ScopedMockLog log;
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(log, Log(_, _, HasSubstr("This is not a request packet")))
      .Times(1);
  ReceiveArpPacket(ARPOP_REPLY);
  // Verify no change in receive count.
  VerifyCurrentCycleStats(kRequestReceived, kCurrentCycle);
  Mock::VerifyAndClearExpectations(&log);
}

TEST_F(PassiveLinkMonitorTest, ReceiveArpRequest) {
  // Setup initial stats.
  const int kRequestReceived = 0;
  const int kCurrentCycle = 0;
  SetCurrentCycleStats(kRequestReceived, kCurrentCycle);

  EXPECT_CALL(*client_, Stop()).Times(0);
  ReceiveArpPacket(ARPOP_REQUEST);
  ReceiveArpPacket(ARPOP_REQUEST);
  VerifyCurrentCycleStats(kRequestReceived + 2, kCurrentCycle);
  Mock::VerifyAndClearExpectations(client_);
}

TEST_F(PassiveLinkMonitorTest, ReceiveAllRequestsForCycle) {
  // 4 ARP requests received in the current cycle so far.
  const int kRequestReceived = 4;
  const int kCurrentCycle = 0;
  SetCurrentCycleStats(kRequestReceived, kCurrentCycle);

  // Received all required requests for a cycle, stop the ARP client.
  EXPECT_CALL(*client_, Stop()).Times(1);
  ReceiveArpPacket(ARPOP_REQUEST);
  Mock::VerifyAndClearExpectations(client_);
}

TEST_F(PassiveLinkMonitorTest, CycleFailed) {
  // 3 ARP requests received in the current cycle so far.
  const int kRequestReceived = 3;
  const int kCurrentCycle = 0;
  SetCurrentCycleStats(kRequestReceived, kCurrentCycle);

  // Monitor failed for the current cycle, post a task to perform cleanup and
  // invoke result callback.
  EXPECT_CALL(*client_, StartRequestListener()).Times(0);
  EXPECT_CALL(dispatcher_, PostDelayedTask(_, _)).Times(0);
  EXPECT_CALL(dispatcher_, PostTask(_)).Times(1);
  InvokeCycleTimeoutHandler();
}

TEST_F(PassiveLinkMonitorTest, CycleSucceed) {
  // 5 ARP requests received in the current cycle so far.
  const int kRequestReceived = 5;
  const int kCurrentCycle = 0;
  SetCurrentCycleStats(kRequestReceived, kCurrentCycle);

  // Monitor succeed for the current cycle, post a task to trigger a new cycle.
  EXPECT_CALL(*client_, StartRequestListener()).WillOnce(Return(true));
  EXPECT_CALL(dispatcher_, PostDelayedTask(_, _)).Times(1);
  EXPECT_CALL(dispatcher_, PostTask(_)).Times(0);
  InvokeCycleTimeoutHandler();
  // ARP request received count should be resetted.
  VerifyCurrentCycleStats(0, kCurrentCycle + 1);
}

TEST_F(PassiveLinkMonitorTest, AllCyclesCompleted) {
  // 5 ARP requests received in the current cycle so far.
  const int kRequestReceived = 5;
  const int kCurrentCycle = PassiveLinkMonitor::kDefaultMonitorCycles - 1;
  SetCurrentCycleStats(kRequestReceived, kCurrentCycle);

  // Monitor completed all the cycles, post a task to perform cleanup and
  // invoke result callback.
  EXPECT_CALL(dispatcher_, PostDelayedTask(_, _)).Times(0);
  EXPECT_CALL(dispatcher_, PostTask(_)).Times(1);
  InvokeCycleTimeoutHandler();
  VerifyCurrentCycleStats(0, PassiveLinkMonitor::kDefaultMonitorCycles);
}

}  // namespace shill
