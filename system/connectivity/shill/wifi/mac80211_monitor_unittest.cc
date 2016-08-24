//
// Copyright (C) 2014 The Android Open Source Project
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

#include "shill/wifi/mac80211_monitor.h"

#include <vector>

#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/mock_event_dispatcher.h"
#include "shill/mock_log.h"
#include "shill/mock_metrics.h"
#include "shill/net/mock_time.h"

using std::string;
using std::vector;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::DoAll;
using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::Return;
using ::testing::SetArgumentPointee;
using ::testing::StrictMock;

namespace shill {

namespace {

const char kTestDeviceName[] = "test-dev";
const char kJunkData[] = "junk data";

}  // namespace

typedef Mac80211Monitor::QueueState QState;

class Mac80211MonitorTest : public testing::Test {
 public:
  Mac80211MonitorTest()
      : metrics_(nullptr),
        mac80211_monitor_(
            &event_dispatcher_,
            kTestDeviceName,
            kQueueLengthLimit,
            base::Bind(&Mac80211MonitorTest::OnRepairHandler,
                       base::Unretained(this)),
            &metrics_) {
    mac80211_monitor_.time_ = &time_;
  }
  virtual ~Mac80211MonitorTest() {}

 protected:
  static const size_t kQueueLengthLimit = 5;
  base::FilePath fake_queue_state_file_path_;  // Call FakeUpSysfs() first.
  base::FilePath fake_wake_queues_file_path_;  // Call FakeUpSysfs() first.

  // Getters for fixture fields.
  MockTime& time() { return time_; }
  MockEventDispatcher& event_dispatcher() { return event_dispatcher_; }
  MockMetrics& metrics() { return metrics_; }

  // Complex fixture methods.
  void AllowWakeQueuesIfNeededCommonCalls() {
    // Allow any number of these calls, as these aspects of
    // WakeQueuesIfNeeded interaction are tested elsewhere.
    EXPECT_CALL(event_dispatcher(), PostDelayedTask(_, _))
        .Times(AnyNumber());
    EXPECT_CALL(metrics(), SendEnumToUMA(_, _, _))
        .Times(AnyNumber());
    EXPECT_CALL(metrics(), SendToUMA(_, _, _, _, _))
        .Times(AnyNumber());
  }
  void FakeUpNotStuckState() {
    FakeUpQueueFiles("00: 0x00000000/10\n");
  }
  void FakeUpStuckByDriverState() {
    FakeUpQueueFiles("00: 0x00000001/10\n");
  }
  void FakeUpStuckByPowerSaveState() {
    FakeUpQueueFiles("00: 0x00000002/10\n");
  }
  void FakeUpSysfs() {
    CHECK(fake_sysfs_tree_.CreateUniqueTempDir());
    CHECK(base::CreateTemporaryFileInDir(
        fake_sysfs_tree_.path(), &fake_queue_state_file_path_));
    CHECK(base::CreateTemporaryFileInDir(
        fake_sysfs_tree_.path(), &fake_wake_queues_file_path_));
    PlumbFakeSysfs();
  }
  void DeleteQueueStateFile() {
    fake_queue_state_file_path_.clear();
    PlumbFakeSysfs();
  }
  bool IsRunning() const {
    return mac80211_monitor_.is_running_ &&
        !mac80211_monitor_.check_queues_callback_.IsCancelled();
  }
  bool IsStopped() const {
    return !mac80211_monitor_.is_running_ &&
        mac80211_monitor_.check_queues_callback_.IsCancelled();
  }
  bool IsWakeQueuesFileModified() const {
    CHECK(fake_sysfs_tree_.IsValid());  // Keep tests hermetic.
    string wake_file_contents;
    base::ReadFileToString(fake_wake_queues_file_path_, &wake_file_contents);
    return wake_file_contents != kJunkData;
  }
  MOCK_METHOD0(OnRepairHandler, void());

  // Getters for Mac80211Monitor state.
  bool GetIsDeviceConnected() const {
    return mac80211_monitor_.is_device_connected_;
  }
  time_t GetLastWokeQueuesMonotonicSeconds() const {
    return mac80211_monitor_.last_woke_queues_monotonic_seconds_;
  }
  const string& GetLinkName() const {
    return mac80211_monitor_.link_name_;
  }
  time_t GetMinimumTimeBetweenWakesSeconds() const {
    return Mac80211Monitor::kMinimumTimeBetweenWakesSeconds;
  }
  const string& GetPhyName() const {
    return mac80211_monitor_.phy_name_;
  }
  const base::FilePath& GetQueueStateFilePath() const {
    return mac80211_monitor_.queue_state_file_path_;
  }
  const base::FilePath& GetWakeQueuesFilePath() const {
    return mac80211_monitor_.wake_queues_file_path_;
  }

  // Pass-through methods to Mac80211Monitor methods.
  void StartMonitor(const string& phy_name) {
    EXPECT_CALL(
        event_dispatcher_,
        PostDelayedTask(
            _, Mac80211Monitor::kQueueStatePollIntervalSeconds * 1000));
    mac80211_monitor_.Start(phy_name);
    if (fake_sysfs_tree_.IsValid()) {
      PlumbFakeSysfs();  // Re-plumb, since un-plumbed by Start().
    }
  }
  void StopMonitor() {
    mac80211_monitor_.Stop();
  }
  uint32_t CheckAreQueuesStuck(const vector<QState>& queue_states) {
    return mac80211_monitor_.CheckAreQueuesStuck(queue_states);
  }
  void UpdateConnectedState(bool new_state) {
    mac80211_monitor_.UpdateConnectedState(new_state);
  }
  void WakeQueuesIfNeeded() {
    CHECK(fake_sysfs_tree_.IsValid());  // Keep tests hermetic.
    mac80211_monitor_.WakeQueuesIfNeeded();
  }

 private:
  base::ScopedTempDir fake_sysfs_tree_;  // Call FakeUpSysfs() first.
  StrictMock<MockEventDispatcher> event_dispatcher_;
  StrictMock<MockMetrics> metrics_;
  StrictMock<MockTime> time_;
  Mac80211Monitor mac80211_monitor_;

  void FakeUpQueueFiles(const string& queue_state_string) {
    CHECK(fake_sysfs_tree_.IsValid());  // Keep tests hermetic.
    base::WriteFile(fake_queue_state_file_path_,
                    queue_state_string.c_str(),
                    queue_state_string.length());
    ASSERT_TRUE(base::WriteFile(fake_wake_queues_file_path_,
                                kJunkData, strlen(kJunkData)));
  }
  void PlumbFakeSysfs() {
    mac80211_monitor_.queue_state_file_path_ = fake_queue_state_file_path_;
    mac80211_monitor_.wake_queues_file_path_ = fake_wake_queues_file_path_;
  }
};

// Can't be in an anonymous namespace, due to ADL.
// Instead, we use static to constain visibility to this unit.
static bool operator==(const QState& a, const QState& b) {
  return a.queue_number == b.queue_number &&
      a.stop_flags == b.stop_flags &&
      a.queue_length == b.queue_length;
}

TEST_F(Mac80211MonitorTest, Ctor) {
  EXPECT_TRUE(IsStopped());
  EXPECT_EQ(kTestDeviceName, GetLinkName());
}

TEST_F(Mac80211MonitorTest, Start) {
  StartMonitor("test-phy");
  EXPECT_TRUE(IsRunning());
  EXPECT_EQ("test-phy", GetPhyName());
  EXPECT_EQ("/sys/kernel/debug/ieee80211/test-phy/queues",
            GetQueueStateFilePath().value());
  EXPECT_EQ("/sys/kernel/debug/ieee80211/test-phy/wake_queues",
            GetWakeQueuesFilePath().value());
  EXPECT_EQ(0, GetLastWokeQueuesMonotonicSeconds());
}

TEST_F(Mac80211MonitorTest, Stop) {
  StartMonitor("dont-care-phy");
  EXPECT_TRUE(IsRunning());
  StopMonitor();
  EXPECT_TRUE(IsStopped());
}

TEST_F(Mac80211MonitorTest, UpdateConnectedState) {
  UpdateConnectedState(false);
  EXPECT_FALSE(GetIsDeviceConnected());

  UpdateConnectedState(true);
  EXPECT_TRUE(GetIsDeviceConnected());

  // Initial state was unknown. Ensure that we can move from true to false.
  UpdateConnectedState(false);
  EXPECT_FALSE(GetIsDeviceConnected());
}

TEST_F(Mac80211MonitorTest, WakeQueuesIfNeededFullMacDevice) {
  FakeUpSysfs();
  StartMonitor("dont-care-phy");
  UpdateConnectedState(false);
  EXPECT_CALL(event_dispatcher(), PostDelayedTask(_, _));
  ScopedMockLog log;
  EXPECT_CALL(log, Log(_, _, HasSubstr(": incomplete read on "))).Times(0);

  // In case of using device with Full-Mac support,
  // there is no queue state file in debugfs.
  DeleteQueueStateFile();
  WakeQueuesIfNeeded();
}

TEST_F(Mac80211MonitorTest, WakeQueuesIfNeededRearmsTimerWhenDisconnected) {
  FakeUpSysfs();
  StartMonitor("dont-care-phy");
  UpdateConnectedState(false);
  EXPECT_CALL(event_dispatcher(), PostDelayedTask(_, _));
  WakeQueuesIfNeeded();
}

TEST_F(Mac80211MonitorTest, WakeQueuesIfNeededFailToReadQueueState) {
  FakeUpSysfs();
  StartMonitor("dont-care-phy");
  UpdateConnectedState(false);
  AllowWakeQueuesIfNeededCommonCalls();
  WakeQueuesIfNeeded();

  // In case we succeeded reading queue state before, but fail this time.
  ScopedMockLog log;
  EXPECT_CALL(log, Log(_, _, HasSubstr(": incomplete read on "))).Times(1);
  DeleteQueueStateFile();
  WakeQueuesIfNeeded();
}

TEST_F(Mac80211MonitorTest, WakeQueuesIfNeededRearmsTimerWhenConnected) {
  FakeUpSysfs();
  StartMonitor("dont-care-phy");
  UpdateConnectedState(true);
  EXPECT_CALL(event_dispatcher(), PostDelayedTask(_, _));
  WakeQueuesIfNeeded();
}

TEST_F(Mac80211MonitorTest, WakeQueuesIfNeededWakeNeeded) {
  FakeUpSysfs();
  FakeUpStuckByPowerSaveState();
  StartMonitor("dont-care-phy");
  EXPECT_EQ(0, GetLastWokeQueuesMonotonicSeconds());

  const time_t kNowMonotonicSeconds = GetMinimumTimeBetweenWakesSeconds();
  EXPECT_CALL(time(), GetSecondsMonotonic(_))
      .WillOnce(DoAll(SetArgumentPointee<0>(kNowMonotonicSeconds),
                      Return(true)));
  EXPECT_CALL(*this, OnRepairHandler());
  AllowWakeQueuesIfNeededCommonCalls();
  WakeQueuesIfNeeded();

  EXPECT_EQ(kNowMonotonicSeconds, GetLastWokeQueuesMonotonicSeconds());
  EXPECT_TRUE(IsWakeQueuesFileModified());
}

TEST_F(Mac80211MonitorTest, WakeQueuesIfNeededRateLimiting) {
  FakeUpSysfs();
  FakeUpStuckByPowerSaveState();
  StartMonitor("dont-care-phy");
  EXPECT_EQ(0, GetLastWokeQueuesMonotonicSeconds());

  EXPECT_CALL(time(), GetSecondsMonotonic(_))
      .WillOnce(DoAll(
          SetArgumentPointee<0>(GetMinimumTimeBetweenWakesSeconds() - 1),
          Return(true)));
  EXPECT_CALL(*this, OnRepairHandler()).Times(0);
  AllowWakeQueuesIfNeededCommonCalls();
  WakeQueuesIfNeeded();

  EXPECT_EQ(0, GetLastWokeQueuesMonotonicSeconds());
  EXPECT_FALSE(IsWakeQueuesFileModified());
}

TEST_F(Mac80211MonitorTest, WakeQueuesIfNeededNotStuck) {
  FakeUpSysfs();
  FakeUpNotStuckState();
  StartMonitor("dont-care-phy");
  EXPECT_EQ(0, GetLastWokeQueuesMonotonicSeconds());

  EXPECT_CALL(*this, OnRepairHandler()).Times(0);
  AllowWakeQueuesIfNeededCommonCalls();
  WakeQueuesIfNeeded();

  EXPECT_EQ(0, GetLastWokeQueuesMonotonicSeconds());
  EXPECT_FALSE(IsWakeQueuesFileModified());
}

TEST_F(Mac80211MonitorTest, WakeQueuesIfNeededStuckByDriver) {
  FakeUpSysfs();
  FakeUpStuckByDriverState();
  StartMonitor("dont-care-phy");
  EXPECT_EQ(0, GetLastWokeQueuesMonotonicSeconds());

  EXPECT_CALL(*this, OnRepairHandler()).Times(0);
  AllowWakeQueuesIfNeededCommonCalls();
  WakeQueuesIfNeeded();

  EXPECT_EQ(0, GetLastWokeQueuesMonotonicSeconds());
  EXPECT_FALSE(IsWakeQueuesFileModified());
}

TEST_F(Mac80211MonitorTest, ParseQueueStateSimple) {
  // Single queue.
  EXPECT_THAT(Mac80211Monitor::ParseQueueState("00: 0x00000000/0\n"),
              ElementsAre(QState(0, 0, 0)));

  // Multiple queues, non-empty.
  EXPECT_THAT(
      Mac80211Monitor::ParseQueueState(
          "00: 0x00000000/10\n"
          "01: 0x00000000/20\n"),
      ElementsAre(QState(0, 0, 10), QState(1, 0, 20)));
}

TEST_F(Mac80211MonitorTest, ParseQueueStateStopped) {
  // Single queue, stopped for various reasons.
  EXPECT_THAT(
      Mac80211Monitor::ParseQueueState("00: 0x00000001/10\n"),
      ElementsAre(QState(0, Mac80211Monitor::kStopFlagDriver, 10)));
  EXPECT_THAT(
      Mac80211Monitor::ParseQueueState("00: 0x00000003/10\n"),
      ElementsAre(QState(0,
                         Mac80211Monitor::kStopFlagDriver |
                         Mac80211Monitor::kStopFlagPowerSave,
                         10)));
  EXPECT_THAT(
      Mac80211Monitor::ParseQueueState("00: 0x00000007/10\n"),
      ElementsAre(QState(0,
                         Mac80211Monitor::kStopFlagDriver |
                         Mac80211Monitor::kStopFlagPowerSave |
                         Mac80211Monitor::kStopFlagChannelSwitch,
                         10)));
  EXPECT_THAT(
      Mac80211Monitor::ParseQueueState("00: 0x0000000f/10\n"),
      ElementsAre(QState(0,
                         Mac80211Monitor::kStopFlagDriver |
                         Mac80211Monitor::kStopFlagPowerSave |
                         Mac80211Monitor::kStopFlagChannelSwitch |
                         Mac80211Monitor::kStopFlagAggregation,
                         10)));
  EXPECT_THAT(
      Mac80211Monitor::ParseQueueState("00: 0x0000001f/10\n"),
      ElementsAre(QState(0,
                         Mac80211Monitor::kStopFlagDriver |
                         Mac80211Monitor::kStopFlagPowerSave |
                         Mac80211Monitor::kStopFlagChannelSwitch |
                         Mac80211Monitor::kStopFlagAggregation |
                         Mac80211Monitor::kStopFlagSuspend,
                         10)));
  EXPECT_THAT(
      Mac80211Monitor::ParseQueueState("00: 0x0000003f/10\n"),
      ElementsAre(QState(0,
                         Mac80211Monitor::kStopFlagDriver |
                         Mac80211Monitor::kStopFlagPowerSave |
                         Mac80211Monitor::kStopFlagChannelSwitch |
                         Mac80211Monitor::kStopFlagAggregation |
                         Mac80211Monitor::kStopFlagSuspend |
                         Mac80211Monitor::kStopFlagBufferAdd,
                         10)));
  EXPECT_THAT(
      Mac80211Monitor::ParseQueueState("00: 0x0000007f/10\n"),
      ElementsAre(QState(0,
                         Mac80211Monitor::kStopFlagDriver |
                         Mac80211Monitor::kStopFlagPowerSave |
                         Mac80211Monitor::kStopFlagChannelSwitch |
                         Mac80211Monitor::kStopFlagAggregation |
                         Mac80211Monitor::kStopFlagSuspend |
                         Mac80211Monitor::kStopFlagBufferAdd |
                         Mac80211Monitor::kStopFlagChannelTypeChange,
                         10)));
}

TEST_F(Mac80211MonitorTest, ParseQueueStateBadInput) {
  // Empty input -> Empty output.
  EXPECT_TRUE(Mac80211Monitor::ParseQueueState("").empty());

  // Missing queue length for queue 0.
  EXPECT_THAT(
      Mac80211Monitor::ParseQueueState(
          "00: 0x00000000\n"
          "01: 0xffffffff/10\n"),
      ElementsAre(QState(1, 0xffffffff, 10)));

  // Missing flags for queue 0.
  EXPECT_THAT(
      Mac80211Monitor::ParseQueueState(
          "00: 0\n"
          "01: 0xffffffff/10\n"),
      ElementsAre(QState(1, 0xffffffff, 10)));

  // Bad number for queue 0.
  EXPECT_THAT(
      Mac80211Monitor::ParseQueueState(
          "aa: 0xabcdefgh/0\n"
          "01: 0xffffffff/10\n"),
      ElementsAre(QState(1, 0xffffffff, 10)));

  // Bad flags for queue 0.
  EXPECT_THAT(
      Mac80211Monitor::ParseQueueState(
          "00: 0xabcdefgh/0\n"
          "01: 0xffffffff/10\n"),
      ElementsAre(QState(1, 0xffffffff, 10)));

  // Bad length for queue 0.
  EXPECT_THAT(
      Mac80211Monitor::ParseQueueState(
          "00: 0x00000000/-1\n"
          "01: 0xffffffff/10\n"),
      ElementsAre(QState(1, 0xffffffff, 10)));
}

TEST_F(Mac80211MonitorTest, CheckAreQueuesStuckNotStuck) {
  EXPECT_FALSE(CheckAreQueuesStuck({}));
  EXPECT_FALSE(CheckAreQueuesStuck({QState(0, 0, 0)}));
  // Not stuck when queue length is below limit.
  EXPECT_FALSE(CheckAreQueuesStuck({
        QState(0, Mac80211Monitor::kStopFlagPowerSave, kQueueLengthLimit-1)}));
}

TEST_F(Mac80211MonitorTest, CheckAreQueuesStuckSingleReason) {
  EXPECT_CALL(metrics(), SendEnumToUMA(
      Metrics::kMetricWifiStoppedTxQueueReason,
      Mac80211Monitor::kStopReasonDriver,
      Mac80211Monitor::kStopReasonMax));
  EXPECT_CALL(metrics(), SendEnumToUMA(
      Metrics::kMetricWifiStoppedTxQueueReason,
      Mac80211Monitor::kStopReasonPowerSave,
      Mac80211Monitor::kStopReasonMax));
  EXPECT_CALL(metrics(), SendToUMA(
      Metrics::kMetricWifiStoppedTxQueueLength,
      kQueueLengthLimit,
      Metrics::kMetricWifiStoppedTxQueueLengthMin,
      Metrics::kMetricWifiStoppedTxQueueLengthMax,
      Metrics::kMetricWifiStoppedTxQueueLengthNumBuckets)).Times(2);
  EXPECT_EQ(Mac80211Monitor::kStopFlagDriver,
            CheckAreQueuesStuck({
                QState(0,
                       Mac80211Monitor::kStopFlagDriver,
                       kQueueLengthLimit)}));
  EXPECT_EQ(Mac80211Monitor::kStopFlagPowerSave,
            CheckAreQueuesStuck({
                QState(0,
                       Mac80211Monitor::kStopFlagPowerSave,
                       kQueueLengthLimit)}));
}

TEST_F(Mac80211MonitorTest, CheckAreQueuesStuckMultipleReasons) {
  EXPECT_CALL(metrics(), SendEnumToUMA(
      Metrics::kMetricWifiStoppedTxQueueReason,
      Mac80211Monitor::kStopReasonPowerSave,
      Mac80211Monitor::kStopReasonMax)).Times(2);
  EXPECT_CALL(metrics(), SendEnumToUMA(
      Metrics::kMetricWifiStoppedTxQueueReason,
      Mac80211Monitor::kStopReasonDriver,
      Mac80211Monitor::kStopReasonMax)).Times(2);
  EXPECT_CALL(metrics(), SendEnumToUMA(
      Metrics::kMetricWifiStoppedTxQueueReason,
      Mac80211Monitor::kStopReasonChannelSwitch,
      Mac80211Monitor::kStopReasonMax)).Times(2);
  EXPECT_CALL(metrics(), SendToUMA(
      Metrics::kMetricWifiStoppedTxQueueLength,
      kQueueLengthLimit,
      Metrics::kMetricWifiStoppedTxQueueLengthMin,
      Metrics::kMetricWifiStoppedTxQueueLengthMax,
      Metrics::kMetricWifiStoppedTxQueueLengthNumBuckets)).Times(3);
  EXPECT_EQ(Mac80211Monitor::kStopFlagDriver |
            Mac80211Monitor::kStopFlagPowerSave,
            CheckAreQueuesStuck({
                QState(0,
                       Mac80211Monitor::kStopFlagDriver |
                       Mac80211Monitor::kStopFlagPowerSave,
                       kQueueLengthLimit)}));
  EXPECT_EQ(Mac80211Monitor::kStopFlagPowerSave |
            Mac80211Monitor::kStopFlagChannelSwitch,
            CheckAreQueuesStuck({
                QState(0,
                       Mac80211Monitor::kStopFlagPowerSave |
                       Mac80211Monitor::kStopFlagChannelSwitch,
                       kQueueLengthLimit)}));
  EXPECT_EQ(Mac80211Monitor::kStopFlagDriver |
            Mac80211Monitor::kStopFlagChannelSwitch,
            CheckAreQueuesStuck({
                QState(0,
                       Mac80211Monitor::kStopFlagDriver |
                       Mac80211Monitor::kStopFlagChannelSwitch,
                       kQueueLengthLimit)}));
}

TEST_F(Mac80211MonitorTest, CheckAreQueuesStuckMultipleQueues) {
  EXPECT_CALL(metrics(), SendEnumToUMA(
      Metrics::kMetricWifiStoppedTxQueueReason,
      Mac80211Monitor::kStopReasonPowerSave,
      Mac80211Monitor::kStopReasonMax)).Times(5);
  EXPECT_CALL(metrics(), SendEnumToUMA(
      Metrics::kMetricWifiStoppedTxQueueReason,
      Mac80211Monitor::kStopReasonDriver,
      Mac80211Monitor::kStopReasonMax)).Times(2);
  EXPECT_CALL(metrics(), SendToUMA(
      Metrics::kMetricWifiStoppedTxQueueLength,
      kQueueLengthLimit,
      Metrics::kMetricWifiStoppedTxQueueLengthMin,
      Metrics::kMetricWifiStoppedTxQueueLengthMax,
      Metrics::kMetricWifiStoppedTxQueueLengthNumBuckets)).Times(5);
  EXPECT_EQ(Mac80211Monitor::kStopFlagPowerSave,
            CheckAreQueuesStuck({
                QState(0, 0, 0),
                QState(0,
                       Mac80211Monitor::kStopFlagPowerSave,
                       kQueueLengthLimit)}));
  EXPECT_EQ(Mac80211Monitor::kStopFlagPowerSave,
            CheckAreQueuesStuck({
                QState(0,
                       Mac80211Monitor::kStopFlagPowerSave,
                       kQueueLengthLimit),
                QState(0, 0, 0)}));
  EXPECT_EQ(Mac80211Monitor::kStopFlagPowerSave,
            CheckAreQueuesStuck({
                QState(0,
                       Mac80211Monitor::kStopFlagPowerSave,
                       kQueueLengthLimit),
                QState(0,
                       Mac80211Monitor::kStopFlagPowerSave,
                       kQueueLengthLimit)}));
  EXPECT_EQ(Mac80211Monitor::kStopFlagDriver |
            Mac80211Monitor::kStopFlagPowerSave,
            CheckAreQueuesStuck({
                QState(0,
                       Mac80211Monitor::kStopFlagPowerSave,
                       kQueueLengthLimit),
                QState(0,
                       Mac80211Monitor::kStopFlagDriver,
                       kQueueLengthLimit)}));
  EXPECT_EQ(Mac80211Monitor::kStopFlagDriver |
            Mac80211Monitor::kStopFlagPowerSave,
            CheckAreQueuesStuck({
                QState(0, Mac80211Monitor::kStopFlagDriver, kQueueLengthLimit),
                QState(0,
                       Mac80211Monitor::kStopFlagPowerSave,
                       kQueueLengthLimit)}));
}

TEST_F(Mac80211MonitorTest, CheckAreQueuesStuckQueueLength) {
  EXPECT_CALL(metrics(), SendEnumToUMA(
      Metrics::kMetricWifiStoppedTxQueueReason,
      Mac80211Monitor::kStopReasonPowerSave,
      Mac80211Monitor::kStopReasonMax)).Times(4);
  EXPECT_CALL(metrics(), SendToUMA(
      Metrics::kMetricWifiStoppedTxQueueLength,
      kQueueLengthLimit,
      Metrics::kMetricWifiStoppedTxQueueLengthMin,
      Metrics::kMetricWifiStoppedTxQueueLengthMax,
      Metrics::kMetricWifiStoppedTxQueueLengthNumBuckets)).Times(4);
  EXPECT_TRUE(CheckAreQueuesStuck({
        QState(0, Mac80211Monitor::kStopFlagPowerSave, kQueueLengthLimit)}));
  EXPECT_TRUE(CheckAreQueuesStuck({
        QState(0, Mac80211Monitor::kStopFlagPowerSave, kQueueLengthLimit-2),
        QState(0, Mac80211Monitor::kStopFlagPowerSave, kQueueLengthLimit-1),
        QState(0, Mac80211Monitor::kStopFlagPowerSave, kQueueLengthLimit)}));
  EXPECT_TRUE(CheckAreQueuesStuck({
        QState(0, Mac80211Monitor::kStopFlagPowerSave, kQueueLengthLimit),
        QState(0, Mac80211Monitor::kStopFlagPowerSave, kQueueLengthLimit-1),
        QState(0, Mac80211Monitor::kStopFlagPowerSave, kQueueLengthLimit-2)}));
  EXPECT_TRUE(CheckAreQueuesStuck({
        QState(0, Mac80211Monitor::kStopFlagPowerSave, kQueueLengthLimit-1),
        QState(0, Mac80211Monitor::kStopFlagPowerSave, kQueueLengthLimit),
        QState(0, Mac80211Monitor::kStopFlagPowerSave, kQueueLengthLimit-2)}));
}

TEST_F(Mac80211MonitorTest, CheckAreQueuesStuckQueueLengthIgnoresUnstopped) {
  EXPECT_CALL(metrics(), SendEnumToUMA(
      Metrics::kMetricWifiStoppedTxQueueReason,
      Mac80211Monitor::kStopReasonPowerSave,
      Mac80211Monitor::kStopReasonMax));
  EXPECT_CALL(metrics(), SendToUMA(
      Metrics::kMetricWifiStoppedTxQueueLength,
      kQueueLengthLimit,
      Metrics::kMetricWifiStoppedTxQueueLengthMin,
      Metrics::kMetricWifiStoppedTxQueueLengthMax,
      Metrics::kMetricWifiStoppedTxQueueLengthNumBuckets));
  EXPECT_TRUE(CheckAreQueuesStuck({
        QState(0, 0, kQueueLengthLimit * 10),
        QState(0, Mac80211Monitor::kStopFlagPowerSave, kQueueLengthLimit)}));
}

}  // namespace shill
