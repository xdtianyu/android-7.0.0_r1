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

#ifndef SHILL_WIFI_MAC80211_MONITOR_H_
#define SHILL_WIFI_MAC80211_MONITOR_H_

#include <time.h>

#include <string>
#include <vector>

#include <base/callback.h>
#include <base/cancelable_callback.h>
#include <base/files/file_path.h>
#include <base/macros.h>
#include <base/memory/weak_ptr.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

namespace shill {

class EventDispatcher;
class Metrics;
class Time;

class Mac80211Monitor {
 public:
  struct QueueState {
    QueueState(size_t queue_number_in,
               uint32_t stop_flags_in,
               size_t queue_length_in)
    : queue_number(queue_number_in), stop_flags(stop_flags_in),
      queue_length(queue_length_in) {}

    size_t queue_number;
    uint32_t stop_flags;
    size_t queue_length;
  };

  Mac80211Monitor(EventDispatcher* dispatcher,
                  const std::string& link_name,
                  size_t queue_length_limit,
                  const base::Closure& on_repair_callback,
                  Metrics* metrics);
  virtual ~Mac80211Monitor();

  virtual void Start(const std::string& phy_name);
  virtual void Stop();
  virtual void UpdateConnectedState(bool new_state);

  const std::string& link_name() const { return link_name_; }

 private:
  friend class Mac80211MonitorTest;
  FRIEND_TEST(Mac80211MonitorTest, CheckAreQueuesStuckMultipleReasons);
  FRIEND_TEST(Mac80211MonitorTest, CheckAreQueuesStuckMultipleQueues);
  FRIEND_TEST(Mac80211MonitorTest, CheckAreQueuesStuckNotStuck);
  FRIEND_TEST(Mac80211MonitorTest, CheckAreQueuesStuckQueueLength);
  FRIEND_TEST(Mac80211MonitorTest,
              CheckAreQueuesStuckQueueLengthIgnoresUnstopped);
  FRIEND_TEST(Mac80211MonitorTest, CheckAreQueuesStuckSingleReason);
  FRIEND_TEST(Mac80211MonitorTest, ParseQueueStateBadInput);
  FRIEND_TEST(Mac80211MonitorTest, ParseQueueStateSimple);
  FRIEND_TEST(Mac80211MonitorTest, ParseQueueStateStopped);

  static const size_t kMaxQueueStateSizeBytes;
  static const char kQueueStatusPathFormat[];
  static const char kWakeQueuesPathFormat[];
  static const time_t kQueueStatePollIntervalSeconds;
  static const time_t kMinimumTimeBetweenWakesSeconds;

  // Values must be kept in sync with ieee80211_i.h.
  enum QueueStopReason {
    kStopReasonDriver,
    kStopReasonPowerSave,
    kStopReasonChannelSwitch,
    kStopReasonAggregation,
    kStopReasonSuspend,
    kStopReasonBufferAdd,
    kStopReasonChannelTypeChange,
    kStopReasonMax = kStopReasonChannelTypeChange
  };
  enum QueueStopFlag {
    kStopFlagDriver = 1 << kStopReasonDriver,
    kStopFlagPowerSave = 1 << kStopReasonPowerSave,
    kStopFlagChannelSwitch = 1 << kStopReasonChannelSwitch,
    kStopFlagAggregation = 1 << kStopReasonAggregation,
    kStopFlagSuspend = 1 << kStopReasonSuspend,
    kStopFlagBufferAdd = 1 << kStopReasonBufferAdd,
    kStopFlagChannelTypeChange = 1 << kStopReasonChannelTypeChange,
    kStopFlagInvalid
  };

  void StartTimer();
  void StopTimer();

  // Check if queues need to be woken. If so, and we haven't woken them
  // too recently, then wake them now.
  void WakeQueuesIfNeeded();

  // Check |queue_states|, to determine if any queues are stuck.
  // Returns a bitmask of QueueStopFlags. A flag will be set if
  // any of the queues has that flag set, and is non-empty.
  // A return value if 0 indicates no queues are stuck.
  uint32_t CheckAreQueuesStuck(const std::vector<QueueState>& queue_states);

  static std::vector<QueueState> ParseQueueState(
      const std::string& state_string);
  static QueueStopFlag GetFlagForReason(QueueStopReason reason);

  Time* time_;  // for mocking in tests
  EventDispatcher* dispatcher_;
  const std::string link_name_;
  size_t queue_length_limit_;
  base::Closure on_repair_callback_;
  Metrics* metrics_;
  std::string phy_name_;
  time_t last_woke_queues_monotonic_seconds_;
  bool is_running_;
  bool have_ever_read_queue_state_file_;
  base::FilePath queue_state_file_path_;
  base::FilePath wake_queues_file_path_;
  base::CancelableClosure check_queues_callback_;
  bool is_device_connected_;
  base::WeakPtrFactory<Mac80211Monitor> weak_ptr_factory_;  // keep last

  DISALLOW_COPY_AND_ASSIGN(Mac80211Monitor);
};

}  // namespace shill

#endif  // SHILL_WIFI_MAC80211_MONITOR_H_
