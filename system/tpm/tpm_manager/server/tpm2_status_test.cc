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

#include "tpm_manager/server/tpm2_status_impl.h"

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <trunks/mock_tpm_state.h>
#include <trunks/trunks_factory_for_test.h>

using testing::NiceMock;
using testing::Return;
using trunks::TPM_RC_FAILURE;
using trunks::TPM_RC_SUCCESS;

namespace tpm_manager {

class Tpm2StatusTest : public testing::Test {
 public:
  Tpm2StatusTest() : factory_(new trunks::TrunksFactoryForTest()) {}
  virtual ~Tpm2StatusTest() = default;

  void SetUp() {
    factory_->set_tpm_state(&mock_tpm_state_);
    tpm_status_.reset(new Tpm2StatusImpl(factory_.get()));
  }

 protected:
  NiceMock<trunks::MockTpmState> mock_tpm_state_;
  std::unique_ptr<trunks::TrunksFactoryForTest> factory_;
  std::unique_ptr<TpmStatus> tpm_status_;
};


TEST_F(Tpm2StatusTest, IsEnabledSuccess) {
  EXPECT_CALL(mock_tpm_state_, Initialize())
      .WillRepeatedly(Return(TPM_RC_SUCCESS));
  EXPECT_CALL(mock_tpm_state_, IsEnabled())
      .WillRepeatedly(Return(true));
  EXPECT_TRUE(tpm_status_->IsTpmEnabled());
}

TEST_F(Tpm2StatusTest, IsEnabledFailure) {
  EXPECT_CALL(mock_tpm_state_, IsEnabled())
      .WillRepeatedly(Return(false));
  EXPECT_FALSE(tpm_status_->IsTpmEnabled());
}

TEST_F(Tpm2StatusTest, IsEnabledNoRepeatedInitialization) {
  EXPECT_CALL(mock_tpm_state_, Initialize())
      .WillOnce(Return(TPM_RC_SUCCESS));
  EXPECT_TRUE(tpm_status_->IsTpmEnabled());
  EXPECT_TRUE(tpm_status_->IsTpmEnabled());
}

TEST_F(Tpm2StatusTest, IsOwnedSuccess) {
  EXPECT_CALL(mock_tpm_state_, Initialize())
      .WillRepeatedly(Return(TPM_RC_SUCCESS));
  EXPECT_CALL(mock_tpm_state_, IsOwned())
      .WillRepeatedly(Return(true));
  EXPECT_TRUE(tpm_status_->IsTpmOwned());
}

TEST_F(Tpm2StatusTest, IsOwnedFailure) {
  EXPECT_CALL(mock_tpm_state_, IsOwned())
      .WillRepeatedly(Return(false));
  EXPECT_FALSE(tpm_status_->IsTpmOwned());
}

TEST_F(Tpm2StatusTest, IsOwnedRepeatedInitializationOnFalse) {
  EXPECT_CALL(mock_tpm_state_, Initialize())
      .Times(2)
      .WillRepeatedly(Return(TPM_RC_SUCCESS));
  EXPECT_CALL(mock_tpm_state_, IsOwned())
      .WillOnce(Return(false));
  EXPECT_FALSE(tpm_status_->IsTpmOwned());
  EXPECT_CALL(mock_tpm_state_, IsOwned())
      .WillRepeatedly(Return(true));
  EXPECT_TRUE(tpm_status_->IsTpmOwned());
}

TEST_F(Tpm2StatusTest, IsOwnedNoRepeatedInitializationOnTrue) {
  EXPECT_CALL(mock_tpm_state_, Initialize())
      .WillOnce(Return(TPM_RC_SUCCESS));
  EXPECT_CALL(mock_tpm_state_, IsOwned())
      .WillRepeatedly(Return(true));
  EXPECT_TRUE(tpm_status_->IsTpmOwned());
  EXPECT_TRUE(tpm_status_->IsTpmOwned());
}

TEST_F(Tpm2StatusTest, GetDictionaryAttackInfoInitializeFailure) {
  EXPECT_CALL(mock_tpm_state_, Initialize())
      .WillRepeatedly(Return(TPM_RC_FAILURE));
  int count;
  int threshold;
  bool lockout;
  int seconds_remaining;
  EXPECT_FALSE(tpm_status_->GetDictionaryAttackInfo(&count,
                                                    &threshold,
                                                    &lockout,
                                                    &seconds_remaining));
}

TEST_F(Tpm2StatusTest, GetDictionaryAttackInfoForwarding) {
  int lockout_count = 3;
  int lockout_threshold = 16;
  bool is_locked = true;
  int lockout_interval = 3600;
  EXPECT_CALL(mock_tpm_state_, GetLockoutCounter())
      .WillRepeatedly(Return(lockout_count));
  EXPECT_CALL(mock_tpm_state_, GetLockoutThreshold())
      .WillRepeatedly(Return(lockout_threshold));
  EXPECT_CALL(mock_tpm_state_, IsInLockout())
      .WillRepeatedly(Return(is_locked));
  EXPECT_CALL(mock_tpm_state_, GetLockoutInterval())
      .WillRepeatedly(Return(lockout_interval));
  int count;
  int threshold;
  bool lockout;
  int seconds_remaining;
  EXPECT_TRUE(tpm_status_->GetDictionaryAttackInfo(&count,
                                                   &threshold,
                                                   &lockout,
                                                   &seconds_remaining));
  EXPECT_EQ(count, lockout_count);
  EXPECT_EQ(threshold, lockout_threshold);
  EXPECT_EQ(lockout, is_locked);
  EXPECT_EQ(seconds_remaining, lockout_count * lockout_interval);
}

TEST_F(Tpm2StatusTest, GetDictionaryAttackInfoAlwaysRefresh) {
  EXPECT_CALL(mock_tpm_state_, Initialize())
      .WillRepeatedly(Return(TPM_RC_SUCCESS));
  int count;
  int threshold;
  bool lockout;
  int seconds_remaining;
  EXPECT_TRUE(tpm_status_->GetDictionaryAttackInfo(&count,
                                                   &threshold,
                                                   &lockout,
                                                   &seconds_remaining));
}

}  // namespace tpm_manager
