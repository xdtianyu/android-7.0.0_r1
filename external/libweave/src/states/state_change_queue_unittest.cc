// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/states/state_change_queue.h"

#include <gtest/gtest.h>
#include <weave/test/unittest_utils.h>

#include "src/bind_lambda.h"

namespace weave {

using test::CreateDictionaryValue;

class StateChangeQueueTest : public ::testing::Test {
 public:
  void SetUp() override { queue_.reset(new StateChangeQueue(100)); }

  void TearDown() override { queue_.reset(); }

  std::unique_ptr<StateChangeQueue> queue_;
};

TEST_F(StateChangeQueueTest, Empty) {
  EXPECT_TRUE(queue_->GetAndClearRecordedStateChanges().empty());
}

TEST_F(StateChangeQueueTest, UpdateOne) {
  auto timestamp = base::Time::Now();
  ASSERT_TRUE(queue_->NotifyPropertiesUpdated(
      timestamp, *CreateDictionaryValue("{'prop': {'name': 23}}")));
  auto changes = queue_->GetAndClearRecordedStateChanges();
  ASSERT_EQ(1u, changes.size());
  EXPECT_EQ(timestamp, changes.front().timestamp);
  EXPECT_JSON_EQ("{'prop':{'name': 23}}", *changes.front().changed_properties);
  EXPECT_TRUE(queue_->GetAndClearRecordedStateChanges().empty());
}

TEST_F(StateChangeQueueTest, UpdateMany) {
  auto timestamp1 = base::Time::Now();
  const std::string state1 = "{'prop': {'name1': 23}}";
  auto timestamp2 = timestamp1 + base::TimeDelta::FromSeconds(1);
  const std::string state2 =
      "{'prop': {'name1': 17, 'name2': 1.0, 'name3': false}}";
  ASSERT_TRUE(queue_->NotifyPropertiesUpdated(timestamp1,
                                              *CreateDictionaryValue(state1)));
  ASSERT_TRUE(queue_->NotifyPropertiesUpdated(timestamp2,
                                              *CreateDictionaryValue(state2)));

  auto changes = queue_->GetAndClearRecordedStateChanges();
  ASSERT_EQ(2u, changes.size());
  EXPECT_EQ(timestamp1, changes[0].timestamp);
  EXPECT_JSON_EQ(state1, *changes[0].changed_properties);
  EXPECT_EQ(timestamp2, changes[1].timestamp);
  EXPECT_JSON_EQ(state2, *changes[1].changed_properties);
  EXPECT_TRUE(queue_->GetAndClearRecordedStateChanges().empty());
}

TEST_F(StateChangeQueueTest, GroupByTimestamp) {
  base::Time timestamp = base::Time::Now();
  base::TimeDelta time_delta = base::TimeDelta::FromMinutes(1);

  ASSERT_TRUE(queue_->NotifyPropertiesUpdated(
      timestamp, *CreateDictionaryValue("{'prop': {'name1': 1}}")));

  ASSERT_TRUE(queue_->NotifyPropertiesUpdated(
      timestamp, *CreateDictionaryValue("{'prop': {'name2': 2}}")));

  ASSERT_TRUE(queue_->NotifyPropertiesUpdated(
      timestamp, *CreateDictionaryValue("{'prop': {'name1': 3}}")));

  ASSERT_TRUE(queue_->NotifyPropertiesUpdated(
      timestamp + time_delta,
      *CreateDictionaryValue("{'prop': {'name1': 4}}")));

  auto changes = queue_->GetAndClearRecordedStateChanges();
  ASSERT_EQ(2u, changes.size());

  const std::string expected1 = "{'prop': {'name1': 3, 'name2': 2}}";
  const std::string expected2 = "{'prop': {'name1': 4}}";
  EXPECT_EQ(timestamp, changes[0].timestamp);
  EXPECT_JSON_EQ(expected1, *changes[0].changed_properties);
  EXPECT_EQ(timestamp + time_delta, changes[1].timestamp);
  EXPECT_JSON_EQ(expected2, *changes[1].changed_properties);
}

TEST_F(StateChangeQueueTest, MaxQueueSize) {
  queue_.reset(new StateChangeQueue(2));
  base::Time start_time = base::Time::Now();
  base::TimeDelta time_delta1 = base::TimeDelta::FromMinutes(1);
  base::TimeDelta time_delta2 = base::TimeDelta::FromMinutes(3);

  ASSERT_TRUE(queue_->NotifyPropertiesUpdated(
      start_time,
      *CreateDictionaryValue("{'prop': {'name1': 1, 'name2': 2}}")));

  ASSERT_TRUE(queue_->NotifyPropertiesUpdated(
      start_time + time_delta1,
      *CreateDictionaryValue("{'prop': {'name1': 3, 'name3': 4}}")));

  ASSERT_TRUE(queue_->NotifyPropertiesUpdated(
      start_time + time_delta2,
      *CreateDictionaryValue("{'prop': {'name10': 10, 'name11': 11}}")));

  auto changes = queue_->GetAndClearRecordedStateChanges();
  ASSERT_EQ(2u, changes.size());

  const std::string expected1 =
      "{'prop': {'name1': 3, 'name2': 2, 'name3': 4}}";
  EXPECT_EQ(start_time + time_delta1, changes[0].timestamp);
  EXPECT_JSON_EQ(expected1, *changes[0].changed_properties);

  const std::string expected2 = "{'prop': {'name10': 10, 'name11': 11}}";
  EXPECT_EQ(start_time + time_delta2, changes[1].timestamp);
  EXPECT_JSON_EQ(expected2, *changes[1].changed_properties);
}

}  // namespace weave
