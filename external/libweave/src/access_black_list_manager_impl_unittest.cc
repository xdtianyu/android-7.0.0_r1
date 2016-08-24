// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/access_black_list_manager_impl.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <weave/provider/test/mock_config_store.h>
#include <weave/test/unittest_utils.h>

#include "src/test/mock_clock.h"
#include "src/bind_lambda.h"

using testing::_;
using testing::Return;
using testing::StrictMock;

namespace weave {

class AccessBlackListManagerImplTest : public testing::Test {
 protected:
  void SetUp() {
    std::string to_load = R"([{
      "user": "BQID",
      "app": "BwQF",
      "expiration": 1410000000
    }, {
      "user": "AQID",
      "app": "AwQF",
      "expiration": 1419999999
    }])";

    EXPECT_CALL(config_store_, LoadSettings("black_list"))
        .WillOnce(Return(to_load));

    EXPECT_CALL(config_store_, SaveSettings("black_list", _, _))
        .WillOnce(testing::WithArgs<1, 2>(testing::Invoke(
            [](const std::string& json, const DoneCallback& callback) {
              std::string to_save = R"([{
                "user": "AQID",
                "app": "AwQF",
                "expiration": 1419999999
              }])";
              EXPECT_JSON_EQ(to_save, *test::CreateValue(json));
              if (!callback.is_null())
                callback.Run(nullptr);
            })));

    EXPECT_CALL(clock_, Now())
        .WillRepeatedly(Return(base::Time::FromTimeT(1412121212)));
    manager_.reset(new AccessBlackListManagerImpl{&config_store_, 10, &clock_});
  }
  StrictMock<test::MockClock> clock_;
  StrictMock<provider::test::MockConfigStore> config_store_{false};
  std::unique_ptr<AccessBlackListManagerImpl> manager_;
};

TEST_F(AccessBlackListManagerImplTest, Init) {
  EXPECT_EQ(1u, manager_->GetSize());
  EXPECT_EQ(10u, manager_->GetCapacity());
  EXPECT_EQ((std::vector<AccessBlackListManagerImpl::Entry>{{
                {1, 2, 3}, {3, 4, 5}, base::Time::FromTimeT(1419999999),
            }}),
            manager_->GetEntries());
}

TEST_F(AccessBlackListManagerImplTest, Block) {
  EXPECT_CALL(config_store_, SaveSettings("black_list", _, _))
      .WillOnce(testing::WithArgs<1, 2>(testing::Invoke(
          [](const std::string& json, const DoneCallback& callback) {
            std::string to_save = R"([{
                "user": "AQID",
                "app": "AwQF",
                "expiration": 1419999999
              }, {
                "app": "CAgI",
                "user": "BwcH",
                "expiration": 1419990000
              }])";
            EXPECT_JSON_EQ(to_save, *test::CreateValue(json));
            if (!callback.is_null())
              callback.Run(nullptr);
          })));
  manager_->Block({7, 7, 7}, {8, 8, 8}, base::Time::FromTimeT(1419990000), {});
}

TEST_F(AccessBlackListManagerImplTest, BlockExpired) {
  manager_->Block({}, {}, base::Time::FromTimeT(1400000000),
                  base::Bind([](ErrorPtr error) {
                    EXPECT_TRUE(error->HasError("aleady_expired"));
                  }));
}

TEST_F(AccessBlackListManagerImplTest, BlockListIsFull) {
  EXPECT_CALL(config_store_, SaveSettings("black_list", _, _))
      .WillRepeatedly(testing::WithArgs<1, 2>(testing::Invoke(
          [](const std::string& json, const DoneCallback& callback) {
            if (!callback.is_null())
              callback.Run(nullptr);
          })));
  for (size_t i = manager_->GetSize(); i < manager_->GetCapacity(); ++i) {
    manager_->Block(
        {99, static_cast<uint8_t>(i / 256), static_cast<uint8_t>(i % 256)},
        {8, 8, 8}, base::Time::FromTimeT(1419990000), {});
    EXPECT_EQ(i + 1, manager_->GetSize());
  }
  manager_->Block({99}, {8, 8, 8}, base::Time::FromTimeT(1419990000),
                  base::Bind([](ErrorPtr error) {
                    EXPECT_TRUE(error->HasError("blacklist_is_full"));
                  }));
}

TEST_F(AccessBlackListManagerImplTest, Unblock) {
  EXPECT_CALL(config_store_, SaveSettings("black_list", _, _))
      .WillOnce(testing::WithArgs<1, 2>(testing::Invoke(
          [](const std::string& json, const DoneCallback& callback) {
            EXPECT_JSON_EQ("[]", *test::CreateValue(json));
            if (!callback.is_null())
              callback.Run(nullptr);
          })));
  manager_->Unblock({1, 2, 3}, {3, 4, 5}, {});
}

TEST_F(AccessBlackListManagerImplTest, UnblockNotFound) {
  manager_->Unblock({5, 2, 3}, {5, 4, 5}, base::Bind([](ErrorPtr error) {
                      EXPECT_TRUE(error->HasError("entry_not_found"));
                    }));
}

TEST_F(AccessBlackListManagerImplTest, IsBlockedFalse) {
  EXPECT_FALSE(manager_->IsBlocked({7, 7, 7}, {8, 8, 8}));
}

class AccessBlackListManagerImplIsBlockedTest
    : public AccessBlackListManagerImplTest,
      public testing::WithParamInterface<
          std::tuple<std::vector<uint8_t>, std::vector<uint8_t>>> {
 public:
  void SetUp() override {
    AccessBlackListManagerImplTest::SetUp();
    EXPECT_CALL(config_store_, SaveSettings("black_list", _, _))
        .WillOnce(testing::WithArgs<2>(
            testing::Invoke([](const DoneCallback& callback) {
              if (!callback.is_null())
                callback.Run(nullptr);
            })));
    manager_->Block(std::get<0>(GetParam()), std::get<1>(GetParam()),
                    base::Time::FromTimeT(1419990000), {});
  }
};

TEST_P(AccessBlackListManagerImplIsBlockedTest, IsBlocked) {
  EXPECT_TRUE(manager_->IsBlocked({7, 7, 7}, {8, 8, 8}));
}

INSTANTIATE_TEST_CASE_P(
    Filters,
    AccessBlackListManagerImplIsBlockedTest,
    testing::Combine(testing::Values(std::vector<uint8_t>{},
                                     std::vector<uint8_t>{7, 7, 7}),
                     testing::Values(std::vector<uint8_t>{},
                                     std::vector<uint8_t>{8, 8, 8})));

}  // namespace weave
