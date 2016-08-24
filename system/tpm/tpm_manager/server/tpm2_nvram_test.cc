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

#include "tpm_manager/server/tpm2_nvram_impl.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <trunks/mock_hmac_session.h>
#include <trunks/mock_tpm_utility.h>
#include <trunks/tpm_constants.h>
#include <trunks/trunks_factory_for_test.h>

#include "tpm_manager/server/mock_local_data_store.h"

namespace {
const char kTestOwnerPassword[] = "owner";
}  // namespace

namespace tpm_manager {

using testing::_;
using testing::DoAll;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::SetArgPointee;
using trunks::TPM_RC_SUCCESS;
using trunks::TPM_RC_FAILURE;

class Tpm2NvramTest : public testing::Test {
 public:
  Tpm2NvramTest() = default;
  virtual ~Tpm2NvramTest() = default;

  void SetUp() {
    trunks::TrunksFactoryForTest* factory = new trunks::TrunksFactoryForTest();
    factory->set_hmac_session(&mock_hmac_session_);
    factory->set_tpm_utility(&mock_tpm_utility_);
    tpm_nvram_.reset(new Tpm2NvramImpl(
        std::unique_ptr<trunks::TrunksFactory>(factory),
        &mock_data_store_));
  }

  void InitializeNvram(const std::string& owner_password) {
    LocalData local_data;
    local_data.set_owner_password(owner_password);
    ON_CALL(mock_data_store_, Read(_))
        .WillByDefault(DoAll(SetArgPointee<0>(local_data),
                             Return(true)));
    tpm_nvram_->Initialize();
    Mock::VerifyAndClearExpectations(&mock_data_store_);
    Mock::VerifyAndClearExpectations(&mock_hmac_session_);
    Mock::VerifyAndClearExpectations(&mock_tpm_utility_);
  }

 protected:
  NiceMock<trunks::MockHmacSession> mock_hmac_session_;
  NiceMock<MockLocalDataStore> mock_data_store_;
  NiceMock<trunks::MockTpmUtility> mock_tpm_utility_;
  std::unique_ptr<Tpm2NvramImpl> tpm_nvram_;
};

TEST_F(Tpm2NvramTest, NvramNoOwnerFailure) {
  uint32_t index = 42;
  EXPECT_FALSE(tpm_nvram_->DefineNvram(index, 5));
  EXPECT_FALSE(tpm_nvram_->DestroyNvram(index));
  EXPECT_FALSE(tpm_nvram_->WriteNvram(index, "data"));
}

TEST_F(Tpm2NvramTest, DefineNvramSuccess) {
  InitializeNvram(kTestOwnerPassword);
  EXPECT_CALL(mock_hmac_session_,
              SetEntityAuthorizationValue(kTestOwnerPassword));
  uint32_t index = 42;
  size_t length = 20;
  EXPECT_CALL(mock_tpm_utility_, DefineNVSpace(index, length, _))
      .WillOnce(Return(TPM_RC_SUCCESS));
  EXPECT_TRUE(tpm_nvram_->DefineNvram(index, length));
}

TEST_F(Tpm2NvramTest, DefineNvramFailure) {
  InitializeNvram(kTestOwnerPassword);
  uint32_t index = 42;
  size_t length = 20;
  EXPECT_CALL(mock_tpm_utility_, DefineNVSpace(index, length, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_FALSE(tpm_nvram_->DefineNvram(index, length));
}

TEST_F(Tpm2NvramTest, DestroyNvramSuccess) {
  InitializeNvram(kTestOwnerPassword);
  EXPECT_CALL(mock_hmac_session_,
              SetEntityAuthorizationValue(kTestOwnerPassword));
  uint32_t index = 42;
  EXPECT_CALL(mock_tpm_utility_, DestroyNVSpace(index, _))
      .WillOnce(Return(TPM_RC_SUCCESS));
  EXPECT_TRUE(tpm_nvram_->DestroyNvram(index));
}

TEST_F(Tpm2NvramTest, DestroyNvramFailure) {
  InitializeNvram(kTestOwnerPassword);
  uint32_t index = 42;
  EXPECT_CALL(mock_tpm_utility_, DestroyNVSpace(index, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_FALSE(tpm_nvram_->DestroyNvram(index));
}

TEST_F(Tpm2NvramTest, WriteNvramSuccess) {
  InitializeNvram(kTestOwnerPassword);
  EXPECT_CALL(mock_hmac_session_,
              SetEntityAuthorizationValue(kTestOwnerPassword));
  uint32_t index = 42;
  std::string data("data");
  EXPECT_CALL(mock_tpm_utility_, WriteNVSpace(index, 0, data, _))
      .WillOnce(Return(TPM_RC_SUCCESS));
  EXPECT_CALL(mock_hmac_session_, SetEntityAuthorizationValue(""));
  EXPECT_CALL(mock_tpm_utility_, LockNVSpace(index, _))
      .WillOnce(Return(TPM_RC_SUCCESS));
  EXPECT_TRUE(tpm_nvram_->WriteNvram(index, data));
}

TEST_F(Tpm2NvramTest, WriteNvramLockError) {
  InitializeNvram(kTestOwnerPassword);
  uint32_t index = 42;
  EXPECT_CALL(mock_tpm_utility_, WriteNVSpace(index, _, _, _))
      .WillOnce(Return(TPM_RC_SUCCESS));
  EXPECT_CALL(mock_tpm_utility_, LockNVSpace(index, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_FALSE(tpm_nvram_->WriteNvram(index, "data"));
}

TEST_F(Tpm2NvramTest, WriteNvramFailure) {
  InitializeNvram(kTestOwnerPassword);
  uint32_t index = 42;
  EXPECT_CALL(mock_tpm_utility_, WriteNVSpace(index, _, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_FALSE(tpm_nvram_->WriteNvram(index, "data"));
}

TEST_F(Tpm2NvramTest, ReadNvramSuccess) {
  uint32_t index = 42;
  std::string tpm_data("data");
  size_t size = tpm_data.size();
  trunks::TPMS_NV_PUBLIC nvram_public;
  nvram_public.data_size = size;
  EXPECT_CALL(mock_tpm_utility_, GetNVSpacePublicArea(_, _))
      .WillOnce(DoAll(SetArgPointee<1>(nvram_public),
                      Return(TPM_RC_SUCCESS)));

  EXPECT_CALL(mock_hmac_session_, SetEntityAuthorizationValue(""));
  EXPECT_CALL(mock_tpm_utility_, ReadNVSpace(index, 0, size, _, _))
      .WillOnce(DoAll(SetArgPointee<3>(tpm_data),
                      Return(TPM_RC_SUCCESS)));
  std::string read_data;
  EXPECT_TRUE(tpm_nvram_->ReadNvram(index, &read_data));
  EXPECT_EQ(read_data, tpm_data);
}

TEST_F(Tpm2NvramTest, ReadNvramNonexistant) {
  uint32_t index = 42;
  EXPECT_CALL(mock_tpm_utility_, GetNVSpacePublicArea(index, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  std::string read_data;
  EXPECT_FALSE(tpm_nvram_->ReadNvram(index, &read_data));
}

TEST_F(Tpm2NvramTest, ReadNvramFailure) {
  uint32_t index = 42;
  trunks::TPMS_NV_PUBLIC nvram_public;
  EXPECT_CALL(mock_tpm_utility_, GetNVSpacePublicArea(index, _))
      .WillOnce(DoAll(SetArgPointee<1>(nvram_public),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_CALL(mock_tpm_utility_, ReadNVSpace(index, _, _, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  std::string read_data;
  EXPECT_FALSE(tpm_nvram_->ReadNvram(index, &read_data));
}

TEST_F(Tpm2NvramTest, IsNvramDefinedSuccess) {
  uint32_t index = 42;
  EXPECT_CALL(mock_tpm_utility_, GetNVSpacePublicArea(index, _))
      .WillOnce(Return(TPM_RC_SUCCESS));
  bool defined;
  EXPECT_TRUE(tpm_nvram_->IsNvramDefined(index, &defined));
  EXPECT_TRUE(defined);
}

TEST_F(Tpm2NvramTest, IsNvramDefinedNonexistant) {
  uint32_t index = 42;
  EXPECT_CALL(mock_tpm_utility_, GetNVSpacePublicArea(index, _))
      .WillOnce(Return(trunks::TPM_RC_HANDLE));
  bool defined;
  EXPECT_TRUE(tpm_nvram_->IsNvramDefined(index, &defined));
  EXPECT_FALSE(defined);
}

TEST_F(Tpm2NvramTest, IsNvramDefinedFailure) {
  uint32_t index = 42;
  EXPECT_CALL(mock_tpm_utility_, GetNVSpacePublicArea(index, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  bool defined;
  EXPECT_FALSE(tpm_nvram_->IsNvramDefined(index, &defined));
}

TEST_F(Tpm2NvramTest, IsNvramLockedSuccess) {
  uint32_t index = 42;
  trunks::TPMS_NV_PUBLIC nvram_public;
  nvram_public.attributes = trunks::TPMA_NV_WRITELOCKED;
  EXPECT_CALL(mock_tpm_utility_, GetNVSpacePublicArea(index, _))
      .WillOnce(DoAll(SetArgPointee<1>(nvram_public),
                      Return(TPM_RC_SUCCESS)));
  bool locked;
  EXPECT_TRUE(tpm_nvram_->IsNvramLocked(index, &locked));
  EXPECT_TRUE(locked);
}

TEST_F(Tpm2NvramTest, IsNvramLockedUnlocked) {
  uint32_t index = 42;
  trunks::TPMS_NV_PUBLIC nvram_public;
  nvram_public.attributes = 0;
  EXPECT_CALL(mock_tpm_utility_, GetNVSpacePublicArea(index, _))
      .WillOnce(DoAll(SetArgPointee<1>(nvram_public),
                      Return(TPM_RC_SUCCESS)));
  bool locked;
  EXPECT_TRUE(tpm_nvram_->IsNvramLocked(index, &locked));
  EXPECT_FALSE(locked);
}

TEST_F(Tpm2NvramTest, IsNvramLockedFailure) {
  uint32_t index = 42;
  EXPECT_CALL(mock_tpm_utility_, GetNVSpacePublicArea(index, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  bool locked;
  EXPECT_FALSE(tpm_nvram_->IsNvramLocked(index, &locked));
}

TEST_F(Tpm2NvramTest, GetNvramSizeSuccess) {
  uint32_t index = 42;
  size_t nvram_size = 20;
  trunks::TPMS_NV_PUBLIC nvram_public;
  nvram_public.data_size = nvram_size;
  EXPECT_CALL(mock_tpm_utility_, GetNVSpacePublicArea(index, _))
      .WillOnce(DoAll(SetArgPointee<1>(nvram_public),
                      Return(TPM_RC_SUCCESS)));
  size_t size;
  EXPECT_TRUE(tpm_nvram_->GetNvramSize(index, &size));
  EXPECT_EQ(size, nvram_size);
}

TEST_F(Tpm2NvramTest, GetNvramSizeFailure) {
  uint32_t index = 42;
  EXPECT_CALL(mock_tpm_utility_, GetNVSpacePublicArea(index, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  size_t size;
  EXPECT_FALSE(tpm_nvram_->GetNvramSize(index, &size));
}

}  // namespace tpm_manager
