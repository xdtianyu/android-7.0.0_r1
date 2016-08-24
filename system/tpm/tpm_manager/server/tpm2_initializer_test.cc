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

#include "tpm_manager/server/tpm2_initializer_impl.h"

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <trunks/mock_tpm_utility.h>
#include <trunks/trunks_factory_for_test.h>

#include "tpm_manager/common/tpm_manager_constants.h"
#include "tpm_manager/server/mock_local_data_store.h"
#include "tpm_manager/server/mock_openssl_crypto_util.h"
#include "tpm_manager/server/mock_tpm_status.h"

using testing::_;
using testing::DoAll;
using testing::NiceMock;
using testing::Return;
using testing::SaveArg;
using testing::SetArgPointee;

namespace tpm_manager {

class Tpm2InitializerTest : public testing::Test {
 public:
  Tpm2InitializerTest() = default;
  virtual ~Tpm2InitializerTest() = default;

  void SetUp() {
    trunks::TrunksFactoryForTest* factory = new trunks::TrunksFactoryForTest();
    factory->set_tpm_utility(&mock_tpm_utility_);
    tpm_initializer_.reset(new Tpm2InitializerImpl(factory,
                                                   &mock_openssl_util_,
                                                   &mock_data_store_,
                                                   &mock_tpm_status_));
  }

 protected:
  NiceMock<MockOpensslCryptoUtil> mock_openssl_util_;
  NiceMock<MockLocalDataStore> mock_data_store_;
  NiceMock<MockTpmStatus> mock_tpm_status_;
  NiceMock<trunks::MockTpmUtility> mock_tpm_utility_;
  std::unique_ptr<TpmInitializer> tpm_initializer_;
};

TEST_F(Tpm2InitializerTest, InitializeTpmNoSeedTpm) {
  EXPECT_CALL(mock_tpm_utility_, StirRandom(_, _))
      .WillRepeatedly(Return(trunks::TPM_RC_FAILURE));
  EXPECT_FALSE(tpm_initializer_->InitializeTpm());
}

TEST_F(Tpm2InitializerTest, InitializeTpmAlreadyOwned) {
  EXPECT_CALL(mock_tpm_status_, IsTpmOwned())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(mock_tpm_utility_, TakeOwnership(_, _, _))
      .Times(0);
  EXPECT_TRUE(tpm_initializer_->InitializeTpm());
}

TEST_F(Tpm2InitializerTest, InitializeTpmLocalDataReadError) {
  EXPECT_CALL(mock_tpm_status_, IsTpmOwned())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(mock_data_store_, Read(_))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(mock_tpm_utility_, TakeOwnership(_, _, _))
      .Times(0);
  EXPECT_FALSE(tpm_initializer_->InitializeTpm());
}

TEST_F(Tpm2InitializerTest, InitializeTpmLocalDataWriteError) {
  EXPECT_CALL(mock_tpm_status_, IsTpmOwned())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(mock_data_store_, Write(_))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(mock_tpm_utility_, TakeOwnership(_, _, _))
      .Times(0);
  EXPECT_FALSE(tpm_initializer_->InitializeTpm());
}

TEST_F(Tpm2InitializerTest, InitializeTpmOwnershipError) {
  EXPECT_CALL(mock_tpm_status_, IsTpmOwned())
      .WillOnce(Return(false));
  EXPECT_CALL(mock_tpm_utility_, TakeOwnership(_, _, _))
      .WillRepeatedly(Return(trunks::TPM_RC_FAILURE));
  EXPECT_FALSE(tpm_initializer_->InitializeTpm());
}

TEST_F(Tpm2InitializerTest, InitializeTpmSuccess) {
  EXPECT_CALL(mock_tpm_status_, IsTpmOwned())
      .WillOnce(Return(false));
  std::string owner_password;
  std::string endorsement_password;
  std::string lockout_password;
  LocalData local_data;
  EXPECT_CALL(mock_data_store_, Read(_))
      .WillOnce(DoAll(SetArgPointee<0>(local_data),
                      Return(true)));
  EXPECT_CALL(mock_tpm_utility_, GenerateRandom(_, _, _))
      .Times(3)  // Once for owner, endorsement and lockout passwords
      .WillRepeatedly(Return(trunks::TPM_RC_SUCCESS));
  EXPECT_CALL(mock_tpm_utility_, TakeOwnership(_, _, _))
      .WillOnce(Return(trunks::TPM_RC_SUCCESS));
  EXPECT_TRUE(tpm_initializer_->InitializeTpm());
}

TEST_F(Tpm2InitializerTest, InitializeTpmSuccessAfterError) {
  EXPECT_CALL(mock_tpm_status_, IsTpmOwned())
      .WillOnce(Return(false));
  std::string owner_password("owner");
  std::string endorsement_password("endorsement");
  std::string lockout_password("lockout");
  LocalData local_data;
  local_data.add_owner_dependency(kTestDependency);
  local_data.set_owner_password(owner_password);
  local_data.set_endorsement_password(endorsement_password);
  local_data.set_lockout_password(lockout_password);
  EXPECT_CALL(mock_data_store_, Read(_))
      .WillOnce(DoAll(SetArgPointee<0>(local_data),
                      Return(true)));
  EXPECT_CALL(mock_data_store_, Write(_))
      .WillOnce(DoAll(SaveArg<0>(&local_data),
                      Return(true)));
  EXPECT_EQ(1, local_data.owner_dependency_size());
  EXPECT_EQ(kTestDependency, local_data.owner_dependency(0));
  EXPECT_EQ(owner_password, local_data.owner_password());
  EXPECT_EQ(endorsement_password, local_data.endorsement_password());
  EXPECT_EQ(lockout_password, local_data.lockout_password());
  EXPECT_CALL(mock_tpm_utility_, TakeOwnership(owner_password,
                                               endorsement_password,
                                               lockout_password))
      .WillOnce(Return(trunks::TPM_RC_SUCCESS));
  EXPECT_TRUE(tpm_initializer_->InitializeTpm());
}

}  // namespace tpm_manager
