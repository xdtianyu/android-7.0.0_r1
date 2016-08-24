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

#include "shill/net/event_history.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "shill/net/mock_time.h"
#include "shill/net/shill_time.h"

using std::deque;
using std::string;
using std::vector;
using ::testing::Mock;
using ::testing::Return;

namespace shill {

namespace {
// consts here
}  // namespace

class EventHistoryTest : public ::testing::Test {
 public:
  EventHistoryTest() : event_history_(new EventHistory()) {
    event_history_->time_ = &time_;
  }

  virtual ~EventHistoryTest() {}

  void SetMaxEventsSaved(int num_events) {
    event_history_->max_events_saved_ = num_events;
    event_history_->max_events_specified_ = true;
  }

  void SetNoMaxEvents() {
    event_history_->max_events_saved_ = 0;
    event_history_->max_events_specified_ = false;
  }

  int GetMaxEventsSaved() { return event_history_->max_events_saved_; }

  bool GetMaxEventsSpecified() { return event_history_->max_events_specified_; }

  deque<Timestamp>* GetEvents() { return &event_history_->events_; }

  void RecordEvent(Timestamp now) {
    EXPECT_CALL(time_, GetNow()).WillOnce(Return(now));
    event_history_->RecordEvent();
  }

  void ExpireEventsBefore(int seconds_ago, Timestamp now,
                          EventHistory::ClockType clock_type) {
    EXPECT_CALL(time_, GetNow()).WillOnce(Return(now));
    event_history_->ExpireEventsBefore(seconds_ago, clock_type);
  }

  void RecordEventAndExpireEventsBefore(int seconds_ago, Timestamp now,
                                        EventHistory::ClockType clock_type) {
    EXPECT_CALL(time_, GetNow()).WillOnce(Return(now));
    event_history_->RecordEventAndExpireEventsBefore(seconds_ago, clock_type);
  }

  vector<string> ExtractWallClockToStrings() {
    return event_history_->ExtractWallClockToStrings();
  }

  Timestamp GetTimestamp(int monotonic_seconds, int boottime_seconds,
                         const string& wall_clock) {
    struct timeval monotonic = {.tv_sec = monotonic_seconds, .tv_usec = 0};
    struct timeval boottime = {.tv_sec = boottime_seconds, .tv_usec = 0};
    return Timestamp(monotonic, boottime, wall_clock);
  }

  int CountEventsWithinInterval(int seconds_ago,
                                EventHistory::ClockType clock_type,
                                Timestamp now) {
    EXPECT_CALL(time_, GetNow()).WillOnce(Return(now));
    return event_history_->CountEventsWithinInterval(seconds_ago, clock_type);
  }

 protected:
  MockTime time_;
  std::unique_ptr<EventHistory> event_history_;
};

TEST_F(EventHistoryTest, RecordEvent) {
  const int kTime1 = 5;
  const int kTime2 = 8;
  EXPECT_TRUE(GetEvents()->empty());
  RecordEvent(GetTimestamp(kTime1, kTime1, ""));
  EXPECT_EQ(1, GetEvents()->size());
  EXPECT_EQ(kTime1, GetEvents()->back().monotonic.tv_sec);
  EXPECT_EQ(kTime1, GetEvents()->back().boottime.tv_sec);

  // Latest events pushed to the back of the list.
  RecordEvent(GetTimestamp(kTime2, kTime2, ""));
  EXPECT_EQ(2, GetEvents()->size());
  EXPECT_EQ(kTime2, GetEvents()->back().monotonic.tv_sec);
  EXPECT_EQ(kTime2, GetEvents()->back().boottime.tv_sec);
}

TEST_F(EventHistoryTest, EventThresholdReached) {
  const int kMaxEventsThreshold = 10;
  const int kTime1 = 5;
  const int kTime2 = 8;
  SetMaxEventsSaved(kMaxEventsThreshold);
  EXPECT_TRUE(GetEvents()->empty());
  for (int i = 0; i < kMaxEventsThreshold; ++i) {
    RecordEvent(GetTimestamp(kTime1, kTime1, ""));
  }
  // All kMaxEventsThreshold events successfully saved.
  EXPECT_EQ(kMaxEventsThreshold, GetEvents()->size());
  EXPECT_EQ(kTime1, GetEvents()->back().monotonic.tv_sec);
  EXPECT_EQ(kTime1, GetEvents()->back().boottime.tv_sec);

  // One timestamp will be evicted to make way for the latest event timestamp,
  // which will be pushed to the back of the list.
  RecordEvent(GetTimestamp(kTime2, kTime2, ""));
  EXPECT_EQ(kMaxEventsThreshold, GetEvents()->size());
  EXPECT_EQ(kTime2, GetEvents()->back().monotonic.tv_sec);
  EXPECT_EQ(kTime2, GetEvents()->back().boottime.tv_sec);
}

TEST_F(EventHistoryTest, ExpireEventsBefore_EvictExpiredEvents) {
  const int kExpiryThresholdSeconds = 10;
  const int kTimeEarly = 5;
  const int kTimeLate = kTimeEarly + kExpiryThresholdSeconds + 1;
  const int kNumEarlierEvents = 20;

  EXPECT_TRUE(GetEvents()->empty());
  for (int i = 0; i < kNumEarlierEvents; ++i) {
    RecordEvent(GetTimestamp(kTimeEarly, kTimeEarly, ""));
  }
  EXPECT_EQ(kNumEarlierEvents, GetEvents()->size());
  EXPECT_EQ(kTimeEarly, GetEvents()->front().monotonic.tv_sec);
  EXPECT_EQ(kTimeEarly, GetEvents()->front().boottime.tv_sec);

  RecordEvent(GetTimestamp(kTimeLate, kTimeLate, ""));
  EXPECT_EQ(kNumEarlierEvents + 1, GetEvents()->size());
  EXPECT_EQ(kTimeEarly, GetEvents()->front().monotonic.tv_sec);
  EXPECT_EQ(kTimeEarly, GetEvents()->front().boottime.tv_sec);
  EXPECT_EQ(kTimeLate, GetEvents()->back().monotonic.tv_sec);
  EXPECT_EQ(kTimeLate, GetEvents()->back().boottime.tv_sec);

  // Expect that all the kTimeEarly event timestamps will be evicted since
  // they took place more than kExpiryThresholdSeconds ago.
  ExpireEventsBefore(kExpiryThresholdSeconds,
                     GetTimestamp(kTimeLate, kTimeLate, ""),
                     EventHistory::kClockTypeBoottime);
  EXPECT_EQ(1, GetEvents()->size());
  EXPECT_EQ(kTimeLate, GetEvents()->front().monotonic.tv_sec);
  EXPECT_EQ(kTimeLate, GetEvents()->front().boottime.tv_sec);
}

TEST_F(EventHistoryTest, ExpireEventsBefore_UseSuspendTime) {
  const int kExpiryThresholdSeconds = 10;
  const int kTime1 = 5;

  EventHistory::ClockType clock_type;

  EXPECT_TRUE(GetEvents()->empty());
  RecordEvent(GetTimestamp(kTime1, kTime1, ""));
  EXPECT_EQ(1, GetEvents()->size());
  EXPECT_EQ(kTime1, GetEvents()->front().monotonic.tv_sec);
  EXPECT_EQ(kTime1, GetEvents()->front().boottime.tv_sec);

  const int kTime2Monotonic = kTime1 + kExpiryThresholdSeconds - 1;
  const int kTime2Boot = kTime1 + kExpiryThresholdSeconds + 1;
  // If we don't count suspend time (i.e. use the monotonic clock), we will not
  // expire the event because it took place less than kExpiryThresholdSeconds
  // ago.
  clock_type = EventHistory::kClockTypeMonotonic;
  ExpireEventsBefore(kExpiryThresholdSeconds,
                     GetTimestamp(kTime2Monotonic, kTime2Boot, ""), clock_type);
  EXPECT_EQ(1, GetEvents()->size());

  // If we count suspend time (i.e. use the boottime clock), we will expire the
  // event because it took place more than kExpiryThresholdSeconds ago.
  clock_type = EventHistory::kClockTypeBoottime;
  ExpireEventsBefore(kExpiryThresholdSeconds,
                     GetTimestamp(kTime2Monotonic, kTime2Boot, ""), clock_type);
  EXPECT_TRUE(GetEvents()->empty());
}

TEST_F(EventHistoryTest, RecordEventAndExpireEventsBefore) {
  const int kExpiryThresholdSeconds = 10;
  const int kTimeEarly = 5;
  const int kTimeLate = kTimeEarly + kExpiryThresholdSeconds + 1;
  const int kNumEarlierEvents = 20;
  const int kMaxEventsThreshold = kNumEarlierEvents / 2;

  SetMaxEventsSaved(kMaxEventsThreshold);
  EXPECT_TRUE(GetEvents()->empty());
  for (int i = 0; i < kNumEarlierEvents; ++i) {
    RecordEventAndExpireEventsBefore(kExpiryThresholdSeconds,
                                     GetTimestamp(kTimeEarly, kTimeEarly, ""),
                                     EventHistory::kClockTypeBoottime);
  }
  // kNumEarlierEvents is greater than kMaxEventsThreshold, so only
  // kMaxEventsThreshold events should be saved.
  EXPECT_EQ(kMaxEventsThreshold, GetEvents()->size());
  EXPECT_EQ(kTimeEarly, GetEvents()->front().monotonic.tv_sec);
  EXPECT_EQ(kTimeEarly, GetEvents()->front().boottime.tv_sec);

  // Expect that the kTimeLate timestamp should be added and all the kTimeEarly
  // event timestamps will be evicted since the the former took place less than
  // kExpiryThresholdSeconds ago and the latter took place more than
  // kExpiryThresholdSeconds ago.
  RecordEventAndExpireEventsBefore(kExpiryThresholdSeconds,
                                   GetTimestamp(kTimeLate, kTimeLate, ""),
                                   EventHistory::kClockTypeBoottime);
  EXPECT_EQ(1, GetEvents()->size());
  EXPECT_EQ(kTimeLate, GetEvents()->front().monotonic.tv_sec);
  EXPECT_EQ(kTimeLate, GetEvents()->front().boottime.tv_sec);
}

TEST_F(EventHistoryTest, ConvertTimestampsToStrings) {
  EXPECT_TRUE(ExtractWallClockToStrings().empty());

  const Timestamp kValues[] = {
      GetTimestamp(123, 123, "2012-12-09T12:41:22.123456+0100"),
      GetTimestamp(234, 234, "2012-12-31T23:59:59.012345+0100")};
  for (size_t i = 0; i < arraysize(kValues); ++i) {
    RecordEvent(kValues[i]);
  }

  vector<string> strings = ExtractWallClockToStrings();
  EXPECT_GT(arraysize(kValues), 0);
  ASSERT_EQ(arraysize(kValues), strings.size());
  for (size_t i = 0; i < arraysize(kValues); i++) {
    EXPECT_EQ(kValues[i].wall_clock, strings[i]);
  }
}

TEST_F(EventHistoryTest, CountEventsWithinInterval) {
  const int kExpiryThresholdSeconds = 10;
  const int kTimeEarly = 5;
  const int kTimeLate = kTimeEarly + kExpiryThresholdSeconds + 1;
  const int kNumEarlierEvents = 20;
  const int kNumLaterEvents = 10;
  const int kMaxEventsThreshold = kNumEarlierEvents + kNumLaterEvents;

  SetMaxEventsSaved(kMaxEventsThreshold);
  EXPECT_TRUE(GetEvents()->empty());
  for (int i = 0; i < kNumEarlierEvents; ++i) {
    RecordEvent(GetTimestamp(kTimeEarly, kTimeEarly, ""));
  }
  for (int i = 0; i < kNumLaterEvents; ++i) {
    RecordEvent(GetTimestamp(kTimeLate, kTimeLate, ""));
  }
  EXPECT_EQ(kMaxEventsThreshold, GetEvents()->size());

  // Only count later events.
  EXPECT_EQ(kNumLaterEvents,
            CountEventsWithinInterval(kExpiryThresholdSeconds,
                                      EventHistory::kClockTypeBoottime,
                                      GetTimestamp(kTimeLate, kTimeLate, "")));

  // Count all events.
  EXPECT_EQ(
      kMaxEventsThreshold,
      CountEventsWithinInterval(kTimeLate, EventHistory::kClockTypeBoottime,
                                GetTimestamp(kTimeLate, kTimeLate, "")));
}

}  // namespace shill
