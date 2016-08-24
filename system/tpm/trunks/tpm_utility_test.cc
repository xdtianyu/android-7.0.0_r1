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

#include <base/stl_util.h>
#include <crypto/sha2.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <openssl/aes.h>

#include "trunks/error_codes.h"
#include "trunks/hmac_authorization_delegate.h"
#include "trunks/mock_authorization_delegate.h"
#include "trunks/mock_blob_parser.h"
#include "trunks/mock_hmac_session.h"
#include "trunks/mock_policy_session.h"
#include "trunks/mock_tpm.h"
#include "trunks/mock_tpm_state.h"
#include "trunks/tpm_constants.h"
#include "trunks/tpm_utility_impl.h"
#include "trunks/trunks_factory_for_test.h"

using testing::_;
using testing::DoAll;
using testing::NiceMock;
using testing::Return;
using testing::SaveArg;
using testing::SetArgPointee;

namespace trunks {

// A test fixture for TpmUtility tests.
class TpmUtilityTest : public testing::Test {
 public:
  TpmUtilityTest() : utility_(factory_) {}
  ~TpmUtilityTest() override {}
  void SetUp() override {
    factory_.set_blob_parser(&mock_blob_parser_);
    factory_.set_tpm_state(&mock_tpm_state_);
    factory_.set_tpm(&mock_tpm_);
    factory_.set_hmac_session(&mock_hmac_session_);
    factory_.set_policy_session(&mock_policy_session_);
  }

  TPM_RC ComputeKeyName(const TPMT_PUBLIC& public_area,
                        std::string* object_name) {
    return utility_.ComputeKeyName(public_area, object_name);
  }

  void SetNVRAMMap(uint32_t index,
                   const TPMS_NV_PUBLIC& public_area) {
    utility_.nvram_public_area_map_[index] = public_area;
  }

  TPM_RC GetNVRAMMap(uint32_t index,
                     TPMS_NV_PUBLIC* public_area) {
    auto it = utility_.nvram_public_area_map_.find(index);
    if (it == utility_.nvram_public_area_map_.end()) {
      return TPM_RC_FAILURE;
    }
    *public_area = it->second;
    return TPM_RC_SUCCESS;
  }

  TPM_RC SetKnownOwnerPassword(const std::string& owner_password) {
    return utility_.SetKnownOwnerPassword(owner_password);
  }

  TPM_RC CreateStorageRootKeys(const std::string& owner_password) {
    return utility_.CreateStorageRootKeys(owner_password);
  }

  TPM_RC CreateSaltingKey(const std::string& owner_password) {
    return utility_.CreateSaltingKey(owner_password);
  }

  void SetExistingKeyHandleExpectation(TPM_HANDLE handle) {
    TPMS_CAPABILITY_DATA capability_data = {};
    TPML_HANDLE& handles = capability_data.data.handles;
    handles.count = 1;
    handles.handle[0] = handle;
    EXPECT_CALL(mock_tpm_,
                GetCapabilitySync(TPM_CAP_HANDLES, handle, _, _, _, _))
        .WillRepeatedly(
            DoAll(SetArgPointee<4>(capability_data), Return(TPM_RC_SUCCESS)));
  }

  void PopulatePCRSelection(bool has_sha1_pcrs,
                            bool make_sha1_bank_empty,
                            bool has_sha256_pcrs,
                            TPML_PCR_SELECTION* pcrs) {
    memset(pcrs, 0, sizeof(TPML_PCR_SELECTION));
    // By convention fill SHA-256 first. This is a bit brittle because order is
    // not important but it simplifies comparison to memcmp.
    if (has_sha256_pcrs) {
      pcrs->pcr_selections[pcrs->count].hash = TPM_ALG_SHA256;
      pcrs->pcr_selections[pcrs->count].sizeof_select = PCR_SELECT_MIN;
      for (int i = 0; i < PCR_SELECT_MIN; ++i) {
        pcrs->pcr_selections[pcrs->count].pcr_select[i] = 0xff;
      }
      ++pcrs->count;
    }
    if (has_sha1_pcrs) {
      pcrs->pcr_selections[pcrs->count].hash = TPM_ALG_SHA1;
      if (make_sha1_bank_empty) {
        pcrs->pcr_selections[pcrs->count].sizeof_select = PCR_SELECT_MAX;
      } else {
        pcrs->pcr_selections[pcrs->count].sizeof_select = PCR_SELECT_MIN;
        for (int i = 0; i < PCR_SELECT_MIN; ++i) {
          pcrs->pcr_selections[pcrs->count].pcr_select[i] = 0xff;
        }
      }
      ++pcrs->count;
    }
  }

  void SetExistingPCRSExpectation(bool has_sha1_pcrs, bool has_sha256_pcrs) {
    TPMS_CAPABILITY_DATA capability_data = {};
    TPML_PCR_SELECTION& pcrs = capability_data.data.assigned_pcr;
    PopulatePCRSelection(has_sha1_pcrs, false, has_sha256_pcrs, &pcrs);
    EXPECT_CALL(mock_tpm_,
                GetCapabilitySync(TPM_CAP_PCRS, _, _, _, _, _))
        .WillRepeatedly(
            DoAll(SetArgPointee<4>(capability_data), Return(TPM_RC_SUCCESS)));
  }

 protected:
  TrunksFactoryForTest factory_;
  NiceMock<MockBlobParser> mock_blob_parser_;
  NiceMock<MockTpmState> mock_tpm_state_;
  NiceMock<MockTpm> mock_tpm_;
  NiceMock<MockAuthorizationDelegate> mock_authorization_delegate_;
  NiceMock<MockHmacSession> mock_hmac_session_;
  NiceMock<MockPolicySession> mock_policy_session_;
  TpmUtilityImpl utility_;
};

TEST_F(TpmUtilityTest, StartupSuccess) {
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.Startup());
}

TEST_F(TpmUtilityTest, StartupAlreadyStarted) {
  EXPECT_CALL(mock_tpm_, StartupSync(_, _))
      .WillRepeatedly(Return(TPM_RC_INITIALIZE));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.Startup());
}

TEST_F(TpmUtilityTest, StartupFailure) {
  EXPECT_CALL(mock_tpm_, StartupSync(_, _))
      .WillRepeatedly(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.Startup());
}

TEST_F(TpmUtilityTest, StartupSelfTestFailure) {
  EXPECT_CALL(mock_tpm_, SelfTestSync(_, _))
      .WillRepeatedly(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.Startup());
}

TEST_F(TpmUtilityTest, ClearSuccess) {
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.Clear());
}

TEST_F(TpmUtilityTest, ClearAfterBadInit) {
  EXPECT_CALL(mock_tpm_, ClearSync(_, _, _))
      .WillOnce(Return(TPM_RC_AUTH_MISSING))
      .WillOnce(Return(TPM_RC_SUCCESS));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.Clear());
}

TEST_F(TpmUtilityTest, ClearFail) {
  EXPECT_CALL(mock_tpm_, ClearSync(_, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.Clear());
}

TEST_F(TpmUtilityTest, ShutdownTest) {
  EXPECT_CALL(mock_tpm_, ShutdownSync(TPM_SU_CLEAR, _));
  utility_.Shutdown();
}

TEST_F(TpmUtilityTest, InitializeTpmAlreadyInit) {
  SetExistingPCRSExpectation(false, true);
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.InitializeTpm());
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.InitializeTpm());
}

TEST_F(TpmUtilityTest, InitializeTpmSuccess) {
  SetExistingPCRSExpectation(false, true);
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.InitializeTpm());
}

TEST_F(TpmUtilityTest, InitializeTpmBadAuth) {
  SetExistingPCRSExpectation(false, true);
  // Reject attempts to set platform auth.
  EXPECT_CALL(mock_tpm_, HierarchyChangeAuthSync(TPM_RH_PLATFORM, _, _, _))
      .WillRepeatedly(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.InitializeTpm());
}

TEST_F(TpmUtilityTest, InitializeTpmDisablePHFails) {
  SetExistingPCRSExpectation(false, true);
  // Reject attempts to disable the platform hierarchy.
  EXPECT_CALL(mock_tpm_, HierarchyControlSync(_, _, TPM_RH_PLATFORM, _, _))
      .WillRepeatedly(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.InitializeTpm());
}

TEST_F(TpmUtilityTest, AllocatePCRFromNone) {
  SetExistingPCRSExpectation(false, false);
  TPML_PCR_SELECTION new_pcr_allocation;
  EXPECT_CALL(mock_tpm_, PCR_AllocateSync(TPM_RH_PLATFORM, _, _, _, _, _, _, _))
      .WillOnce(DoAll(SaveArg<2>(&new_pcr_allocation),
                      SetArgPointee<3>(YES),
                      Return(TPM_RC_SUCCESS)));
  ASSERT_EQ(TPM_RC_SUCCESS, utility_.AllocatePCR(""));
  ASSERT_EQ(1u, new_pcr_allocation.count);
  TPML_PCR_SELECTION expected_pcr_allocation;
  PopulatePCRSelection(false, false, true, &expected_pcr_allocation);
  ASSERT_EQ(0, memcmp(&expected_pcr_allocation, &new_pcr_allocation,
                      sizeof(TPML_PCR_SELECTION)));
}

TEST_F(TpmUtilityTest, AllocatePCRFromSHA1Only) {
  SetExistingPCRSExpectation(true, false);
  TPML_PCR_SELECTION new_pcr_allocation;
  EXPECT_CALL(mock_tpm_, PCR_AllocateSync(TPM_RH_PLATFORM, _, _, _, _, _, _, _))
      .WillOnce(DoAll(SaveArg<2>(&new_pcr_allocation),
                      SetArgPointee<3>(YES),
                      Return(TPM_RC_SUCCESS)));
  ASSERT_EQ(TPM_RC_SUCCESS, utility_.AllocatePCR(""));
  ASSERT_EQ(2u, new_pcr_allocation.count);
  TPML_PCR_SELECTION expected_pcr_allocation;
  PopulatePCRSelection(true, true, true, &expected_pcr_allocation);
  ASSERT_EQ(0, memcmp(&expected_pcr_allocation, &new_pcr_allocation,
                      sizeof(TPML_PCR_SELECTION)));
}

TEST_F(TpmUtilityTest, AllocatePCRFromSHA1AndSHA256) {
  SetExistingPCRSExpectation(true, true);
  TPML_PCR_SELECTION new_pcr_allocation;
  EXPECT_CALL(mock_tpm_, PCR_AllocateSync(TPM_RH_PLATFORM, _, _, _, _, _, _, _))
      .WillOnce(DoAll(SaveArg<2>(&new_pcr_allocation),
                      SetArgPointee<3>(YES),
                      Return(TPM_RC_SUCCESS)));
  ASSERT_EQ(TPM_RC_SUCCESS, utility_.AllocatePCR(""));
  ASSERT_EQ(1u, new_pcr_allocation.count);
  TPML_PCR_SELECTION expected_pcr_allocation;
  PopulatePCRSelection(true, true, false, &expected_pcr_allocation);
  ASSERT_EQ(0, memcmp(&expected_pcr_allocation, &new_pcr_allocation,
                      sizeof(TPML_PCR_SELECTION)));
}

TEST_F(TpmUtilityTest, AllocatePCRFromSHA256Only) {
  SetExistingPCRSExpectation(false, true);
  EXPECT_CALL(mock_tpm_, PCR_AllocateSync(TPM_RH_PLATFORM, _, _, _, _, _, _, _))
      .Times(0);
  ASSERT_EQ(TPM_RC_SUCCESS, utility_.AllocatePCR(""));
}

TEST_F(TpmUtilityTest, AllocatePCRCommandFailure) {
  SetExistingPCRSExpectation(false, false);
  EXPECT_CALL(mock_tpm_, PCR_AllocateSync(_, _, _, _, _, _, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.AllocatePCR(""));
}

TEST_F(TpmUtilityTest, AllocatePCRTpmFailure) {
  SetExistingPCRSExpectation(false, false);
  EXPECT_CALL(mock_tpm_, PCR_AllocateSync(_, _, _, _, _, _, _, _))
      .WillOnce(DoAll(SetArgPointee<3>(NO),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.AllocatePCR(""));
}

TEST_F(TpmUtilityTest, TakeOwnershipSuccess) {
  EXPECT_CALL(mock_tpm_state_, IsOwnerPasswordSet())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(mock_tpm_state_, IsEndorsementPasswordSet())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(mock_tpm_state_, IsLockoutPasswordSet())
      .WillRepeatedly(Return(false));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.TakeOwnership("owner",
                                                   "endorsement",
                                                   "lockout"));
}

TEST_F(TpmUtilityTest, TakeOwnershipOwnershipDone) {
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.TakeOwnership("owner",
                                                   "endorsement",
                                                   "lockout"));
}

TEST_F(TpmUtilityTest, TakeOwnershipBadSession) {
  EXPECT_CALL(mock_hmac_session_, StartUnboundSession(true))
      .WillRepeatedly(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.TakeOwnership("owner",
                                                   "endorsement",
                                                   "lockout"));
}

TEST_F(TpmUtilityTest, TakeOwnershipFailure) {
  EXPECT_CALL(mock_tpm_, HierarchyChangeAuthSync(TPM_RH_OWNER, _, _, _))
      .WillRepeatedly(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.TakeOwnership("owner",
                                                   "endorsement",
                                                   "lockout"));
}

TEST_F(TpmUtilityTest, ChangeOwnerPasswordEndorsementDone) {
  EXPECT_CALL(mock_tpm_state_, IsOwnerPasswordSet())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(mock_tpm_state_, IsLockoutPasswordSet())
      .WillRepeatedly(Return(false));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.TakeOwnership("owner",
                                                   "endorsement",
                                                   "lockout"));
}

TEST_F(TpmUtilityTest, ChangeOwnerPasswordLockoutDone) {
  EXPECT_CALL(mock_tpm_state_, IsOwnerPasswordSet())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(mock_tpm_state_, IsEndorsementPasswordSet())
      .WillRepeatedly(Return(false));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.TakeOwnership("owner",
                                                   "endorsement",
                                                   "lockout"));
}

TEST_F(TpmUtilityTest, ChangeOwnerPasswordEndorsementLockoutDone) {
  EXPECT_CALL(mock_tpm_state_, IsOwnerPasswordSet())
      .WillRepeatedly(Return(false));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.TakeOwnership("owner",
                                                   "endorsement",
                                                   "lockout"));
}

TEST_F(TpmUtilityTest, ChangeOwnerPasswordEndorsementFail) {
  EXPECT_CALL(mock_tpm_state_, IsOwnerPasswordSet())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(mock_tpm_state_, IsEndorsementPasswordSet())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(mock_tpm_, HierarchyChangeAuthSync(_, _, _, _))
      .WillRepeatedly(Return(TPM_RC_SUCCESS));
  EXPECT_CALL(mock_tpm_, HierarchyChangeAuthSync(TPM_RH_ENDORSEMENT, _, _, _))
      .WillRepeatedly(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.TakeOwnership("owner",
                                                   "endorsement",
                                                   "lockout"));
}

TEST_F(TpmUtilityTest, ChangeOwnerPasswordLockoutFailure) {
  EXPECT_CALL(mock_tpm_state_, IsOwnerPasswordSet())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(mock_tpm_state_, IsEndorsementPasswordSet())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(mock_tpm_state_, IsLockoutPasswordSet())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(mock_tpm_, HierarchyChangeAuthSync(_, _, _, _))
      .WillRepeatedly(Return(TPM_RC_SUCCESS));
  EXPECT_CALL(mock_tpm_, HierarchyChangeAuthSync(TPM_RH_LOCKOUT, _, _, _))
      .WillRepeatedly(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.TakeOwnership("owner",
                                                   "endorsement",
                                                   "lockout"));
}

TEST_F(TpmUtilityTest, StirRandomSuccess) {
  std::string entropy_data("large test data", 100);
  EXPECT_EQ(TPM_RC_SUCCESS,
            utility_.StirRandom(entropy_data, &mock_authorization_delegate_));
}

TEST_F(TpmUtilityTest, StirRandomFails) {
  std::string entropy_data("test data");
  EXPECT_CALL(mock_tpm_, StirRandomSync(_, nullptr))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.StirRandom(entropy_data, nullptr));
}

TEST_F(TpmUtilityTest, GenerateRandomSuccess) {
  // This number is larger than the max bytes the GetRandom call can return.
  // Therefore we expect software to make multiple calls to fill this many
  // bytes.
  size_t num_bytes = 72;
  std::string random_data;
  TPM2B_DIGEST large_random;
  large_random.size = 32;
  TPM2B_DIGEST small_random;
  small_random.size = 8;
  EXPECT_CALL(mock_tpm_, GetRandomSync(_, _, &mock_authorization_delegate_))
      .Times(2)
      .WillRepeatedly(DoAll(SetArgPointee<1>(large_random),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_CALL(mock_tpm_, GetRandomSync(8, _, &mock_authorization_delegate_))
      .WillOnce(DoAll(SetArgPointee<1>(small_random),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.GenerateRandom(
      num_bytes, &mock_authorization_delegate_, &random_data));
  EXPECT_EQ(num_bytes, random_data.size());
}

TEST_F(TpmUtilityTest, GenerateRandomFails) {
  size_t num_bytes = 5;
  std::string random_data;
  EXPECT_CALL(mock_tpm_, GetRandomSync(_, _, nullptr))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE,
            utility_.GenerateRandom(num_bytes, nullptr, &random_data));
}

TEST_F(TpmUtilityTest, ExtendPCRSuccess) {
  TPM_HANDLE pcr_handle = HR_PCR + 1;
  TPML_DIGEST_VALUES digests;
  EXPECT_CALL(mock_tpm_,
              PCR_ExtendSync(pcr_handle, _, _, &mock_authorization_delegate_))
      .WillOnce(DoAll(SaveArg<2>(&digests),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.ExtendPCR(1, "test digest",
                                               &mock_authorization_delegate_));
  EXPECT_EQ(1u, digests.count);
  EXPECT_EQ(TPM_ALG_SHA256, digests.digests[0].hash_alg);
  std::string hash_string = crypto::SHA256HashString("test digest");
  EXPECT_EQ(0, memcmp(hash_string.data(),
                      digests.digests[0].digest.sha256,
                      crypto::kSHA256Length));
}

TEST_F(TpmUtilityTest, ExtendPCRFail) {
  int pcr_index = 0;
  TPM_HANDLE pcr_handle = HR_PCR + pcr_index;
  EXPECT_CALL(mock_tpm_, PCR_ExtendSync(pcr_handle, _, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE,
            utility_.ExtendPCR(pcr_index, "test digest", nullptr));
}

TEST_F(TpmUtilityTest, ExtendPCRBadParam) {
  EXPECT_EQ(TPM_RC_FAILURE, utility_.ExtendPCR(-1, "test digest", nullptr));
}

TEST_F(TpmUtilityTest, ReadPCRSuccess) {
  // The |pcr_index| is chosen to match the structure for |pcr_select|.
  // If you change |pcr_index|, remember to change |pcr_select|.
  int pcr_index = 1;
  std::string pcr_value;
  TPML_PCR_SELECTION pcr_select;
  pcr_select.count = 1;
  pcr_select.pcr_selections[0].hash = TPM_ALG_SHA256;
  pcr_select.pcr_selections[0].sizeof_select = 1;
  pcr_select.pcr_selections[0].pcr_select[0] = 2;
  TPML_DIGEST pcr_values;
  pcr_values.count = 1;
  pcr_values.digests[0].size = 5;
  EXPECT_CALL(mock_tpm_, PCR_ReadSync(_, _, _, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(pcr_select),
                      SetArgPointee<3>(pcr_values),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.ReadPCR(pcr_index, &pcr_value));
}

TEST_F(TpmUtilityTest, ReadPCRFail) {
  std::string pcr_value;
  EXPECT_CALL(mock_tpm_, PCR_ReadSync(_, _, _, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.ReadPCR(1, &pcr_value));
}

TEST_F(TpmUtilityTest, ReadPCRBadReturn) {
  std::string pcr_value;
  EXPECT_EQ(TPM_RC_FAILURE, utility_.ReadPCR(1, &pcr_value));
}

TEST_F(TpmUtilityTest, AsymmetricEncryptSuccess) {
  TPM_HANDLE key_handle;
  std::string plaintext;
  std::string output_ciphertext("ciphertext");
  std::string ciphertext;
  TPM2B_PUBLIC_KEY_RSA out_message = Make_TPM2B_PUBLIC_KEY_RSA(
      output_ciphertext);
  TPM2B_PUBLIC public_area;
  public_area.public_area.type = TPM_ALG_RSA;
  public_area.public_area.object_attributes = kDecrypt;
  public_area.public_area.auth_policy.size = 0;
  public_area.public_area.unique.rsa.size = 0;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_CALL(mock_tpm_, RSA_EncryptSync(key_handle, _, _, _, _, _,
                                         &mock_authorization_delegate_))
      .WillOnce(DoAll(SetArgPointee<5>(out_message),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.AsymmetricEncrypt(
      key_handle,
      TPM_ALG_NULL,
      TPM_ALG_NULL,
      plaintext,
      &mock_authorization_delegate_,
      &ciphertext));
  EXPECT_EQ(0, ciphertext.compare(output_ciphertext));
}

TEST_F(TpmUtilityTest, AsymmetricEncryptFail) {
  TPM_HANDLE key_handle;
  std::string plaintext;
  std::string ciphertext;
  TPM2B_PUBLIC public_area;
  public_area.public_area.type = TPM_ALG_RSA;
  public_area.public_area.object_attributes = kDecrypt;
  public_area.public_area.auth_policy.size = 0;
  public_area.public_area.unique.rsa.size = 0;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_CALL(mock_tpm_, RSA_EncryptSync(key_handle, _, _, _, _, _, nullptr))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.AsymmetricEncrypt(key_handle,
                                                      TPM_ALG_NULL,
                                                      TPM_ALG_NULL,
                                                      plaintext,
                                                      nullptr,
                                                      &ciphertext));
}

TEST_F(TpmUtilityTest, AsymmetricEncryptBadParams) {
  TPM_HANDLE key_handle = TPM_RH_FIRST;
  std::string plaintext;
  std::string ciphertext;
  TPM2B_PUBLIC public_area;
  public_area.public_area.type = TPM_ALG_RSA;
  public_area.public_area.object_attributes = kDecrypt | kRestricted;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, nullptr))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(SAPI_RC_BAD_PARAMETER, utility_.AsymmetricEncrypt(key_handle,
                                                             TPM_ALG_RSAES,
                                                             TPM_ALG_NULL,
                                                             plaintext,
                                                             nullptr,
                                                             &ciphertext));
}

TEST_F(TpmUtilityTest, AsymmetricEncryptNullSchemeForward) {
  TPM_HANDLE key_handle;
  std::string plaintext;
  std::string output_ciphertext("ciphertext");
  std::string ciphertext;
  TPM2B_PUBLIC_KEY_RSA out_message = Make_TPM2B_PUBLIC_KEY_RSA(
      output_ciphertext);
  TPM2B_PUBLIC public_area;
  public_area.public_area.type = TPM_ALG_RSA;
  public_area.public_area.object_attributes = kDecrypt;
  public_area.public_area.auth_policy.size = 0;
  public_area.public_area.unique.rsa.size = 0;
  TPMT_RSA_DECRYPT scheme;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_CALL(mock_tpm_, RSA_EncryptSync(key_handle, _, _, _, _, _, nullptr))
      .WillOnce(DoAll(SetArgPointee<5>(out_message),
                      SaveArg<3>(&scheme),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.AsymmetricEncrypt(key_handle,
                                                      TPM_ALG_NULL,
                                                      TPM_ALG_NULL,
                                                      plaintext,
                                                      nullptr,
                                                      &ciphertext));
  EXPECT_EQ(scheme.scheme, TPM_ALG_OAEP);
  EXPECT_EQ(scheme.details.oaep.hash_alg, TPM_ALG_SHA256);
}

TEST_F(TpmUtilityTest, AsymmetricEncryptSchemeForward) {
  TPM_HANDLE key_handle;
  std::string plaintext;
  std::string output_ciphertext("ciphertext");
  std::string ciphertext;
  TPM2B_PUBLIC_KEY_RSA out_message = Make_TPM2B_PUBLIC_KEY_RSA(
      output_ciphertext);
  TPM2B_PUBLIC public_area;
  public_area.public_area.type = TPM_ALG_RSA;
  public_area.public_area.object_attributes = kDecrypt;
  public_area.public_area.auth_policy.size = 0;
  public_area.public_area.unique.rsa.size = 0;
  TPMT_RSA_DECRYPT scheme;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_CALL(mock_tpm_, RSA_EncryptSync(key_handle, _, _, _, _, _, nullptr))
      .WillOnce(DoAll(SetArgPointee<5>(out_message),
                      SaveArg<3>(&scheme),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.AsymmetricEncrypt(key_handle,
                                                      TPM_ALG_RSAES,
                                                      TPM_ALG_NULL,
                                                      plaintext,
                                                      nullptr,
                                                      &ciphertext));
  EXPECT_EQ(scheme.scheme, TPM_ALG_RSAES);
}

TEST_F(TpmUtilityTest, AsymmetricDecryptSuccess) {
  TPM_HANDLE key_handle;
  std::string plaintext;
  std::string output_plaintext("plaintext");
  std::string ciphertext;
  std::string password("password");
  TPM2B_PUBLIC_KEY_RSA out_message = Make_TPM2B_PUBLIC_KEY_RSA(
      output_plaintext);
  TPM2B_PUBLIC public_area;
  public_area.public_area.type = TPM_ALG_RSA;
  public_area.public_area.object_attributes = kDecrypt;
  public_area.public_area.auth_policy.size = 0;
  public_area.public_area.unique.rsa.size = 0;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_CALL(mock_tpm_, RSA_DecryptSync(key_handle, _, _, _, _, _,
                                         &mock_authorization_delegate_))
      .WillOnce(DoAll(SetArgPointee<5>(out_message),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.AsymmetricDecrypt(
      key_handle,
      TPM_ALG_NULL,
      TPM_ALG_NULL,
      ciphertext,
      &mock_authorization_delegate_,
      &plaintext));
  EXPECT_EQ(0, plaintext.compare(output_plaintext));
}

TEST_F(TpmUtilityTest, AsymmetricDecryptFail) {
  TPM_HANDLE key_handle;
  std::string key_name;
  std::string plaintext;
  std::string ciphertext;
  std::string password;
  TPM2B_PUBLIC public_area;
  public_area.public_area.type = TPM_ALG_RSA;
  public_area.public_area.object_attributes = kDecrypt;
  public_area.public_area.auth_policy.size = 0;
  public_area.public_area.unique.rsa.size = 0;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_CALL(mock_tpm_, RSA_DecryptSync(key_handle, _, _, _, _, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.AsymmetricDecrypt(
      key_handle,
      TPM_ALG_NULL,
      TPM_ALG_NULL,
      ciphertext,
      &mock_authorization_delegate_,
      &plaintext));
}

TEST_F(TpmUtilityTest, AsymmetricDecryptBadParams) {
  TPM_HANDLE key_handle = TPM_RH_FIRST;
  std::string plaintext;
  std::string ciphertext;
  TPM2B_PUBLIC public_area;
  public_area.public_area.type = TPM_ALG_RSA;
  public_area.public_area.object_attributes = kDecrypt | kRestricted;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(SAPI_RC_BAD_PARAMETER, utility_.AsymmetricDecrypt(
      key_handle,
      TPM_ALG_RSAES,
      TPM_ALG_NULL,
      ciphertext,
      &mock_authorization_delegate_,
      &plaintext));
}

TEST_F(TpmUtilityTest, AsymmetricDecryptBadSession) {
  TPM_HANDLE key_handle = TPM_RH_FIRST;
  std::string key_name;
  std::string plaintext;
  std::string ciphertext;
  std::string password;
  EXPECT_EQ(SAPI_RC_INVALID_SESSIONS, utility_.AsymmetricDecrypt(
      key_handle, TPM_ALG_RSAES, TPM_ALG_NULL,
      ciphertext, nullptr, &plaintext));
}

TEST_F(TpmUtilityTest, AsymmetricDecryptNullSchemeForward) {
  TPM_HANDLE key_handle;
  std::string plaintext;
  std::string output_plaintext("plaintext");
  std::string ciphertext;
  std::string password;
  TPM2B_PUBLIC_KEY_RSA out_message = Make_TPM2B_PUBLIC_KEY_RSA(
      output_plaintext);
  TPM2B_PUBLIC public_area;
  public_area.public_area.type = TPM_ALG_RSA;
  public_area.public_area.object_attributes = kDecrypt;
  public_area.public_area.auth_policy.size = 0;
  public_area.public_area.unique.rsa.size = 0;
  TPMT_RSA_DECRYPT scheme;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_CALL(mock_tpm_, RSA_DecryptSync(key_handle, _, _, _, _, _, _))
      .WillOnce(DoAll(SetArgPointee<5>(out_message),
                      SaveArg<3>(&scheme),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.AsymmetricDecrypt(
      key_handle,
      TPM_ALG_NULL,
      TPM_ALG_NULL,
      ciphertext,
      &mock_authorization_delegate_,
      &plaintext));
  EXPECT_EQ(scheme.scheme, TPM_ALG_OAEP);
  EXPECT_EQ(scheme.details.oaep.hash_alg, TPM_ALG_SHA256);
}

TEST_F(TpmUtilityTest, AsymmetricDecryptSchemeForward) {
  TPM_HANDLE key_handle;
  std::string plaintext;
  std::string output_plaintext("plaintext");
  std::string ciphertext;
  std::string password;
  TPM2B_PUBLIC_KEY_RSA out_message = Make_TPM2B_PUBLIC_KEY_RSA(
      output_plaintext);
  TPM2B_PUBLIC public_area;
  public_area.public_area.type = TPM_ALG_RSA;
  public_area.public_area.object_attributes = kDecrypt;
  public_area.public_area.auth_policy.size = 0;
  public_area.public_area.unique.rsa.size = 0;
  TPMT_RSA_DECRYPT scheme;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_CALL(mock_tpm_, RSA_DecryptSync(key_handle, _, _, _, _, _, _))
      .WillOnce(DoAll(SetArgPointee<5>(out_message),
                      SaveArg<3>(&scheme),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.AsymmetricDecrypt(
      key_handle,
      TPM_ALG_RSAES,
      TPM_ALG_NULL,
      ciphertext,
      &mock_authorization_delegate_,
      &plaintext));
  EXPECT_EQ(scheme.scheme, TPM_ALG_RSAES);
}

TEST_F(TpmUtilityTest, SignSuccess) {
  TPM_HANDLE key_handle;
  std::string password("password");
  std::string digest(32, 'a');
  TPMT_SIGNATURE signature_out;
  signature_out.signature.rsassa.sig.size = 2;
  signature_out.signature.rsassa.sig.buffer[0] = 'h';
  signature_out.signature.rsassa.sig.buffer[1] = 'i';
  std::string signature;
  TPM2B_PUBLIC public_area;
  public_area.public_area.type = TPM_ALG_RSA;
  public_area.public_area.object_attributes = kSign;
  public_area.public_area.auth_policy.size = 0;
  public_area.public_area.unique.rsa.size = 0;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_CALL(mock_tpm_, SignSync(key_handle, _, _, _, _, _,
                                  &mock_authorization_delegate_))
      .WillOnce(DoAll(SetArgPointee<5>(signature_out),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.Sign(key_handle,
                                          TPM_ALG_NULL,
                                          TPM_ALG_NULL,
                                          digest,
                                          &mock_authorization_delegate_,
                                          &signature));
  EXPECT_EQ(0, signature.compare("hi"));
}

TEST_F(TpmUtilityTest, SignFail) {
  TPM_HANDLE key_handle;
  std::string password;
  std::string digest(32, 'a');
  std::string signature;
  TPM2B_PUBLIC public_area;
  public_area.public_area.type = TPM_ALG_RSA;
  public_area.public_area.object_attributes = kSign;
  public_area.public_area.auth_policy.size = 0;
  public_area.public_area.unique.rsa.size = 0;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_CALL(mock_tpm_, SignSync(key_handle, _, _, _, _, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.Sign(key_handle,
                                          TPM_ALG_NULL,
                                          TPM_ALG_NULL,
                                          digest,
                                          &mock_authorization_delegate_,
                                          &signature));
}

TEST_F(TpmUtilityTest, SignBadParams1) {
  TPM_HANDLE key_handle;
  std::string password;
  std::string digest(32, 'a');
  std::string signature;
  TPM2B_PUBLIC public_area;
  public_area.public_area.type = TPM_ALG_RSA;
  public_area.public_area.object_attributes = kSign | kRestricted;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(SAPI_RC_BAD_PARAMETER, utility_.Sign(key_handle,
                                                 TPM_ALG_RSAPSS,
                                                 TPM_ALG_NULL,
                                                 digest,
                                                 &mock_authorization_delegate_,
                                                 &signature));
}

TEST_F(TpmUtilityTest, SignBadAuthorizationSession) {
  TPM_HANDLE key_handle = TPM_RH_FIRST;
  std::string password;
  std::string digest(32, 'a');
  std::string signature;
  EXPECT_EQ(SAPI_RC_INVALID_SESSIONS, utility_.Sign(key_handle,
                                                    TPM_ALG_RSAPSS,
                                                    TPM_ALG_NULL,
                                                    digest,
                                                    nullptr,
                                                    &signature));
}

TEST_F(TpmUtilityTest, SignBadParams2) {
  TPM_HANDLE key_handle;
  std::string password;
  std::string digest(32, 'a');
  std::string signature;
  TPM2B_PUBLIC public_area;
  public_area.public_area.type = TPM_ALG_RSA;
  public_area.public_area.object_attributes = kDecrypt;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(SAPI_RC_BAD_PARAMETER, utility_.Sign(key_handle,
                                                 TPM_ALG_RSAPSS,
                                                 TPM_ALG_NULL,
                                                 digest,
                                                 &mock_authorization_delegate_,
                                                 &signature));
}

TEST_F(TpmUtilityTest, SignBadParams3) {
  TPM_HANDLE key_handle;
  std::string password;
  std::string digest(32, 'a');
  std::string signature;
  TPM2B_PUBLIC public_area;
  public_area.public_area.type = TPM_ALG_ECC;
  public_area.public_area.object_attributes = kSign;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(SAPI_RC_BAD_PARAMETER, utility_.Sign(key_handle,
                                                 TPM_ALG_RSAPSS,
                                                 TPM_ALG_NULL,
                                                 digest,
                                                 &mock_authorization_delegate_,
                                                 &signature));
}

TEST_F(TpmUtilityTest, SignBadParams4) {
  TPM_HANDLE key_handle;
  std::string password;
  std::string digest(32, 'a');
  std::string signature;
  TPM2B_PUBLIC public_area;
  public_area.public_area.type = TPM_ALG_RSA;
  public_area.public_area.object_attributes = kSign;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_FAILURE)));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.Sign(key_handle,
                                          TPM_ALG_RSAPSS,
                                          TPM_ALG_NULL,
                                          digest,
                                          &mock_authorization_delegate_,
                                          &signature));
}

TEST_F(TpmUtilityTest, SignBadParams5) {
  TPM_HANDLE key_handle = 0;
  std::string password;
  std::string digest(32, 'a');
  std::string signature;
  EXPECT_EQ(SAPI_RC_BAD_PARAMETER, utility_.Sign(key_handle,
                                                 TPM_ALG_AES,
                                                 TPM_ALG_NULL,
                                                 digest,
                                                 &mock_authorization_delegate_,
                                                 &signature));
}


TEST_F(TpmUtilityTest, SignNullSchemeForward) {
  TPM_HANDLE key_handle;
  std::string password;
  std::string digest(32, 'a');
  TPMT_SIGNATURE signature_out;
  signature_out.signature.rsassa.sig.size = 0;
  std::string signature;
  TPM2B_PUBLIC public_area;
  TPMT_SIG_SCHEME scheme;
  public_area.public_area.type = TPM_ALG_RSA;
  public_area.public_area.object_attributes = kSign;
  public_area.public_area.auth_policy.size = 0;
  public_area.public_area.unique.rsa.size = 0;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_CALL(mock_tpm_, SignSync(key_handle, _, _, _, _, _, _))
      .WillOnce(DoAll(SetArgPointee<5>(signature_out),
                      SaveArg<3>(&scheme),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.Sign(key_handle,
                                          TPM_ALG_NULL,
                                          TPM_ALG_NULL,
                                          digest,
                                          &mock_authorization_delegate_,
                                          &signature));
  EXPECT_EQ(scheme.scheme, TPM_ALG_RSASSA);
  EXPECT_EQ(scheme.details.rsassa.hash_alg, TPM_ALG_SHA256);
}

TEST_F(TpmUtilityTest, SignSchemeForward) {
  TPM_HANDLE key_handle;
  std::string password;
  std::string digest(64, 'a');
  TPMT_SIGNATURE signature_out;
  signature_out.signature.rsassa.sig.size = 0;
  std::string signature;
  TPM2B_PUBLIC public_area;
  TPMT_SIG_SCHEME scheme;
  public_area.public_area.type = TPM_ALG_RSA;
  public_area.public_area.object_attributes = kSign;
  public_area.public_area.auth_policy.size = 0;
  public_area.public_area.unique.rsa.size = 0;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_CALL(mock_tpm_, SignSync(key_handle, _, _, _, _, _, _))
      .WillOnce(DoAll(SetArgPointee<5>(signature_out),
                      SaveArg<3>(&scheme),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.Sign(key_handle,
                                          TPM_ALG_RSAPSS,
                                          TPM_ALG_SHA1,
                                          digest,
                                          &mock_authorization_delegate_,
                                          &signature));
  EXPECT_EQ(scheme.scheme, TPM_ALG_RSAPSS);
  EXPECT_EQ(scheme.details.rsapss.hash_alg, TPM_ALG_SHA1);
}

TEST_F(TpmUtilityTest, VerifySuccess) {
  TPM_HANDLE key_handle;
  std::string digest(32, 'a');
  std::string signature;
  TPM2B_PUBLIC public_area;
  public_area.public_area.type = TPM_ALG_RSA;
  public_area.public_area.object_attributes = kSign;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_CALL(mock_tpm_, VerifySignatureSync(key_handle, _, _, _, _, _))
      .WillOnce(Return(TPM_RC_SUCCESS));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.Verify(key_handle,
                                            TPM_ALG_NULL,
                                            TPM_ALG_NULL,
                                            digest,
                                            signature,
                                            nullptr));
}

TEST_F(TpmUtilityTest, VerifyFail) {
  TPM_HANDLE key_handle;
  std::string digest(32, 'a');
  std::string signature;
  TPM2B_PUBLIC public_area;
  public_area.public_area.type = TPM_ALG_RSA;
  public_area.public_area.object_attributes = kSign;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_CALL(mock_tpm_, VerifySignatureSync(key_handle, _, _, _, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.Verify(key_handle,
                                            TPM_ALG_NULL,
                                            TPM_ALG_NULL,
                                            digest,
                                            signature,
                                            nullptr));
}

TEST_F(TpmUtilityTest, VerifyBadParams1) {
  TPM_HANDLE key_handle;
  std::string digest(32, 'a');
  std::string signature;
  TPM2B_PUBLIC public_area;
  public_area.public_area.type = TPM_ALG_RSA;
  public_area.public_area.object_attributes = kSign | kRestricted;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(SAPI_RC_BAD_PARAMETER, utility_.Verify(key_handle,
                                                   TPM_ALG_NULL,
                                                   TPM_ALG_NULL,
                                                   digest,
                                                   signature,
                                                   nullptr));
}

TEST_F(TpmUtilityTest, VerifyBadParams2) {
  TPM_HANDLE key_handle;
  std::string digest(32, 'a');
  std::string signature;
  TPM2B_PUBLIC public_area;
  public_area.public_area.type = TPM_ALG_RSA;
  public_area.public_area.object_attributes = kDecrypt;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(SAPI_RC_BAD_PARAMETER, utility_.Verify(key_handle,
                                                   TPM_ALG_NULL,
                                                   TPM_ALG_NULL,
                                                   digest,
                                                   signature,
                                                   nullptr));
}

TEST_F(TpmUtilityTest, VerifyBadParams3) {
  TPM_HANDLE key_handle;
  std::string digest(32, 'a');
  std::string signature;
  TPM2B_PUBLIC public_area;
  public_area.public_area.type = TPM_ALG_ECC;
  public_area.public_area.object_attributes = kSign;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(SAPI_RC_BAD_PARAMETER, utility_.Verify(key_handle,
                                                   TPM_ALG_NULL,
                                                   TPM_ALG_NULL,
                                                   digest,
                                                   signature,
                                                   nullptr));
}

TEST_F(TpmUtilityTest, VerifyBadParams4) {
  TPM_HANDLE key_handle;
  std::string digest(32, 'a');
  std::string signature;
  TPM2B_PUBLIC public_area;
  public_area.public_area.type = TPM_ALG_RSA;
  public_area.public_area.object_attributes = kSign;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_FAILURE)));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.Verify(key_handle,
                                            TPM_ALG_NULL,
                                            TPM_ALG_NULL,
                                            digest,
                                            signature,
                                            nullptr));
}

TEST_F(TpmUtilityTest, VerifyBadParams5) {
  TPM_HANDLE key_handle;
  std::string digest(32, 'a');
  std::string signature;
  TPM2B_PUBLIC public_area;
  public_area.public_area.type = TPM_ALG_RSA;
  public_area.public_area.object_attributes = kSign;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(SAPI_RC_BAD_PARAMETER, utility_.Verify(key_handle,
                                                   TPM_ALG_AES,
                                                   TPM_ALG_NULL,
                                                   digest,
                                                   signature,
                                                   nullptr));
}

TEST_F(TpmUtilityTest, VerifyNullSchemeForward) {
  TPM_HANDLE key_handle;
  std::string digest(32, 'a');
  std::string signature;
  TPM2B_PUBLIC public_area;
  TPMT_SIGNATURE signature_in;
  public_area.public_area.type = TPM_ALG_RSA;
  public_area.public_area.object_attributes = kSign;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_CALL(mock_tpm_, VerifySignatureSync(key_handle, _, _, _, _, _))
      .WillOnce(DoAll(SaveArg<3>(&signature_in),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.Verify(key_handle,
                                            TPM_ALG_NULL,
                                            TPM_ALG_NULL,
                                            digest,
                                            signature,
                                            nullptr));
  EXPECT_EQ(signature_in.sig_alg, TPM_ALG_RSASSA);
  EXPECT_EQ(signature_in.signature.rsassa.hash, TPM_ALG_SHA256);
}

TEST_F(TpmUtilityTest, VerifySchemeForward) {
  TPM_HANDLE key_handle;
  std::string digest(64, 'a');
  std::string signature;
  TPM2B_PUBLIC public_area;
  TPMT_SIGNATURE signature_in;
  public_area.public_area.type = TPM_ALG_RSA;
  public_area.public_area.object_attributes = kSign;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_CALL(mock_tpm_, VerifySignatureSync(key_handle, _, _, _, _, _))
      .WillOnce(DoAll(SaveArg<3>(&signature_in),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.Verify(key_handle,
                                            TPM_ALG_RSAPSS,
                                            TPM_ALG_SHA1,
                                            digest,
                                            signature,
                                            nullptr));
  EXPECT_EQ(signature_in.sig_alg, TPM_ALG_RSAPSS);
  EXPECT_EQ(signature_in.signature.rsassa.hash, TPM_ALG_SHA1);
}

TEST_F(TpmUtilityTest, CertifyCreationSuccess) {
  TPM_HANDLE key_handle = 42;
  std::string creation_blob;
  EXPECT_CALL(mock_tpm_, CertifyCreationSyncShort(TPM_RH_NULL, key_handle,
                                                  _, _, _, _, _, _, _))
      .WillOnce(Return(TPM_RC_SUCCESS));
  EXPECT_EQ(TPM_RC_SUCCESS,
            utility_.CertifyCreation(key_handle, creation_blob));
}

TEST_F(TpmUtilityTest, CertifyCreationParserError) {
  TPM_HANDLE key_handle = 42;
  std::string creation_blob;
  EXPECT_CALL(mock_blob_parser_, ParseCreationBlob(creation_blob, _, _, _))
      .WillOnce(Return(false));
  EXPECT_EQ(SAPI_RC_BAD_PARAMETER,
            utility_.CertifyCreation(key_handle, creation_blob));
}

TEST_F(TpmUtilityTest, CertifyCreationFailure) {
  TPM_HANDLE key_handle = 42;
  std::string creation_blob;
  EXPECT_CALL(mock_tpm_, CertifyCreationSyncShort(TPM_RH_NULL, key_handle,
                                                  _, _, _, _, _, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE,
            utility_.CertifyCreation(key_handle, creation_blob));
}

TEST_F(TpmUtilityTest, ChangeAuthDataSuccess) {
  TPM_HANDLE key_handle = 1;
  std::string new_password;
  std::string key_blob;
  TPM2B_PUBLIC public_area;
  public_area.public_area.type = TPM_ALG_RSA;
  public_area.public_area.auth_policy.size = 0;
  public_area.public_area.unique.rsa.size = 0;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(_, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.ChangeKeyAuthorizationData(
    key_handle, new_password, &mock_authorization_delegate_, &key_blob));
}

TEST_F(TpmUtilityTest, ChangeAuthDataKeyNameFail) {
  TPM_HANDLE key_handle = 1;
  std::string old_password;
  std::string new_password;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(key_handle, _, _, _, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.ChangeKeyAuthorizationData(
      key_handle, new_password, &mock_authorization_delegate_, nullptr));
}

TEST_F(TpmUtilityTest, ChangeAuthDataFailure) {
  TPM_HANDLE key_handle = 1;
  std::string new_password;
  EXPECT_CALL(mock_tpm_, ObjectChangeAuthSync(key_handle, _, _, _, _, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.ChangeKeyAuthorizationData(
      key_handle, new_password, &mock_authorization_delegate_, nullptr));
}

TEST_F(TpmUtilityTest, ChangeAuthDataParserFail) {
  TPM_HANDLE key_handle = 1;
  std::string new_password;
  std::string key_blob;
  TPM2B_PUBLIC public_area;
  public_area.public_area.type = TPM_ALG_RSA;
  public_area.public_area.auth_policy.size = 0;
  public_area.public_area.unique.rsa.size = 0;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(_, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_area),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_CALL(mock_blob_parser_, SerializeKeyBlob(_, _, &key_blob))
      .WillOnce(Return(false));
  EXPECT_EQ(SAPI_RC_BAD_TCTI_STRUCTURE, utility_.ChangeKeyAuthorizationData(
    key_handle, new_password, &mock_authorization_delegate_, &key_blob));
}

TEST_F(TpmUtilityTest, ImportRSAKeySuccess) {
  uint32_t public_exponent = 0x10001;
  std::string modulus(256, 'a');
  std::string prime_factor(128, 'b');
  std::string password("password");
  std::string key_blob;
  TPM2B_DATA encryption_key;
  TPM2B_PUBLIC public_data;
  TPM2B_PRIVATE private_data;
  EXPECT_CALL(mock_tpm_, ImportSync(_, _, _, _, _, _, _, _, _))
      .WillOnce(DoAll(SaveArg<2>(&encryption_key),
                      SaveArg<3>(&public_data),
                      SaveArg<4>(&private_data),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.ImportRSAKey(
      TpmUtility::AsymmetricKeyUsage::kDecryptKey,
      modulus,
      public_exponent,
      prime_factor,
      password,
      &mock_authorization_delegate_,
      &key_blob));
  // Validate that the public area was properly constructed.
  EXPECT_EQ(public_data.public_area.parameters.rsa_detail.key_bits,
            modulus.size() * 8);
  EXPECT_EQ(public_data.public_area.parameters.rsa_detail.exponent,
            public_exponent);
  EXPECT_EQ(public_data.public_area.unique.rsa.size, modulus.size());
  EXPECT_EQ(0, memcmp(public_data.public_area.unique.rsa.buffer,
                      modulus.data(), modulus.size()));
  // Validate the private struct construction.
  EXPECT_EQ(kAesKeySize, encryption_key.size);
  AES_KEY key;
  AES_set_encrypt_key(encryption_key.buffer, kAesKeySize * 8, &key);
  unsigned char iv[MAX_AES_BLOCK_SIZE_BYTES] = {0};
  int iv_in = 0;
  std::string unencrypted_private(private_data.size, 0);
  AES_cfb128_encrypt(
    reinterpret_cast<const unsigned char*>(private_data.buffer),
    reinterpret_cast<unsigned char*>(string_as_array(&unencrypted_private)),
    private_data.size, &key, iv, &iv_in, AES_DECRYPT);
  TPM2B_DIGEST inner_integrity;
  EXPECT_EQ(TPM_RC_SUCCESS, Parse_TPM2B_DIGEST(&unencrypted_private,
                                               &inner_integrity, nullptr));
  std::string object_name;
  EXPECT_EQ(TPM_RC_SUCCESS,
            ComputeKeyName(public_data.public_area, &object_name));
  std::string integrity_value = crypto::SHA256HashString(unencrypted_private +
                                                         object_name);
  EXPECT_EQ(integrity_value.size(), inner_integrity.size);
  EXPECT_EQ(0, memcmp(inner_integrity.buffer,
                      integrity_value.data(),
                      inner_integrity.size));
  TPM2B_SENSITIVE sensitive_data;
  EXPECT_EQ(TPM_RC_SUCCESS, Parse_TPM2B_SENSITIVE(&unencrypted_private,
                                                  &sensitive_data, nullptr));
  EXPECT_EQ(sensitive_data.sensitive_area.auth_value.size, password.size());
  EXPECT_EQ(0, memcmp(sensitive_data.sensitive_area.auth_value.buffer,
                      password.data(), password.size()));
  EXPECT_EQ(sensitive_data.sensitive_area.sensitive.rsa.size,
            prime_factor.size());
  EXPECT_EQ(0, memcmp(sensitive_data.sensitive_area.sensitive.rsa.buffer,
                      prime_factor.data(), prime_factor.size()));
}

TEST_F(TpmUtilityTest, ImportRSAKeySuccessWithNoBlob) {
  uint32_t public_exponent = 0x10001;
  std::string modulus(256, 'a');
  std::string prime_factor(128, 'b');
  std::string password;
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.ImportRSAKey(
      TpmUtility::AsymmetricKeyUsage::kDecryptKey,
      modulus,
      public_exponent,
      prime_factor,
      password,
      &mock_authorization_delegate_,
      nullptr));
}

TEST_F(TpmUtilityTest, ImportRSAKeyParentNameFail) {
  uint32_t public_exponent = 0x10001;
  std::string modulus(256, 'a');
  std::string prime_factor(128, 'b');
  std::string password;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(_, _, _, _, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.ImportRSAKey(
      TpmUtility::AsymmetricKeyUsage::kDecryptKey,
      modulus,
      public_exponent,
      prime_factor,
      password,
      &mock_authorization_delegate_,
      nullptr));
}

TEST_F(TpmUtilityTest, ImportRSAKeyFail) {
  std::string modulus;
  std::string prime_factor;
  std::string password;
  EXPECT_CALL(mock_tpm_, ImportSync(_, _, _, _, _, _, _, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.ImportRSAKey(
      TpmUtility::AsymmetricKeyUsage::kDecryptKey,
      modulus,
      0x10001,
      prime_factor,
      password,
      &mock_authorization_delegate_,
      nullptr));
}

TEST_F(TpmUtilityTest, ImportRSAKeyParserFail) {
  std::string modulus;
  std::string prime_factor;
  std::string password;
  std::string key_blob;
  EXPECT_CALL(mock_blob_parser_, SerializeKeyBlob(_, _, &key_blob))
      .WillOnce(Return(false));
  EXPECT_EQ(SAPI_RC_BAD_TCTI_STRUCTURE, utility_.ImportRSAKey(
      TpmUtility::AsymmetricKeyUsage::kDecryptKey,
      modulus,
      0x10001,
      prime_factor,
      password,
      &mock_authorization_delegate_,
      &key_blob));
}

TEST_F(TpmUtilityTest, CreateRSAKeyPairSuccess) {
  TPM2B_PUBLIC public_area;
  TPML_PCR_SELECTION creation_pcrs;
  EXPECT_CALL(mock_tpm_, CreateSyncShort(kRSAStorageRootKey,
                                         _, _, _, _, _, _, _, _,
                                         &mock_authorization_delegate_))
      .WillOnce(DoAll(SaveArg<2>(&public_area),
                      SaveArg<3>(&creation_pcrs),
                      Return(TPM_RC_SUCCESS)));
  std::string key_blob;
  std::string creation_blob;
  int creation_pcr = 12;
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.CreateRSAKeyPair(
      TpmUtility::AsymmetricKeyUsage::kDecryptAndSignKey, 2048, 0x10001,
      "password", "", false, creation_pcr, &mock_authorization_delegate_,
      &key_blob, &creation_blob));
  EXPECT_EQ(public_area.public_area.object_attributes & kDecrypt, kDecrypt);
  EXPECT_EQ(public_area.public_area.object_attributes & kSign, kSign);
  EXPECT_EQ(public_area.public_area.object_attributes & kUserWithAuth,
            kUserWithAuth);
  EXPECT_EQ(public_area.public_area.object_attributes & kAdminWithPolicy, 0u);
  EXPECT_EQ(public_area.public_area.parameters.rsa_detail.scheme.scheme,
            TPM_ALG_NULL);
  EXPECT_EQ(1u, creation_pcrs.count);
  EXPECT_EQ(TPM_ALG_SHA256, creation_pcrs.pcr_selections[0].hash);
  EXPECT_EQ(PCR_SELECT_MIN, creation_pcrs.pcr_selections[0].sizeof_select);
  EXPECT_EQ(1u << (creation_pcr % 8),
            creation_pcrs.pcr_selections[0].pcr_select[creation_pcr / 8]);
}

TEST_F(TpmUtilityTest, CreateRSAKeyPairDecryptKeySuccess) {
  TPM2B_PUBLIC public_area;
  EXPECT_CALL(mock_tpm_, CreateSyncShort(kRSAStorageRootKey,
                                         _, _, _, _, _, _, _, _,
                                         &mock_authorization_delegate_))
      .WillOnce(DoAll(SaveArg<2>(&public_area),
                      Return(TPM_RC_SUCCESS)));
  std::string key_blob;
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.CreateRSAKeyPair(
      TpmUtility::AsymmetricKeyUsage::kDecryptKey, 2048, 0x10001, "password",
      "", false, kNoCreationPCR, &mock_authorization_delegate_, &key_blob,
      nullptr));
  EXPECT_EQ(public_area.public_area.object_attributes & kDecrypt, kDecrypt);
  EXPECT_EQ(public_area.public_area.object_attributes & kSign, 0u);
  EXPECT_EQ(public_area.public_area.parameters.rsa_detail.scheme.scheme,
            TPM_ALG_NULL);
}

TEST_F(TpmUtilityTest, CreateRSAKeyPairSignKeySuccess) {
  TPM2B_PUBLIC public_area;
  TPM2B_SENSITIVE_CREATE sensitive_create;
  EXPECT_CALL(mock_tpm_, CreateSyncShort(kRSAStorageRootKey,
                                         _, _, _, _, _, _, _, _,
                                         &mock_authorization_delegate_))
      .WillOnce(DoAll(SaveArg<1>(&sensitive_create),
                      SaveArg<2>(&public_area),
                      Return(TPM_RC_SUCCESS)));
  std::string key_blob;
  std::string policy_digest(32, 'a');
  std::string key_auth("password");
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.CreateRSAKeyPair(
      TpmUtility::AsymmetricKeyUsage::kSignKey, 2048, 0x10001, key_auth,
      policy_digest, true  /* use_only_policy_authorization */, kNoCreationPCR,
      &mock_authorization_delegate_, &key_blob, nullptr));
  EXPECT_EQ(public_area.public_area.object_attributes & kDecrypt, 0u);
  EXPECT_EQ(public_area.public_area.object_attributes & kSign, kSign);
  EXPECT_EQ(public_area.public_area.object_attributes & kUserWithAuth, 0u);
  EXPECT_EQ(public_area.public_area.object_attributes & kAdminWithPolicy,
            kAdminWithPolicy);
  EXPECT_EQ(public_area.public_area.parameters.rsa_detail.scheme.scheme,
            TPM_ALG_NULL);
  EXPECT_EQ(public_area.public_area.parameters.rsa_detail.key_bits, 2048);
  EXPECT_EQ(public_area.public_area.parameters.rsa_detail.exponent, 0x10001u);
  EXPECT_EQ(public_area.public_area.auth_policy.size, policy_digest.size());
  EXPECT_EQ(0, memcmp(public_area.public_area.auth_policy.buffer,
                      policy_digest.data(), policy_digest.size()));
  EXPECT_EQ(sensitive_create.sensitive.user_auth.size, key_auth.size());
  EXPECT_EQ(0, memcmp(sensitive_create.sensitive.user_auth.buffer,
                      key_auth.data(), key_auth.size()));
}

TEST_F(TpmUtilityTest, CreateRSAKeyPairBadDelegate) {
  std::string key_blob;
  EXPECT_EQ(SAPI_RC_INVALID_SESSIONS, utility_.CreateRSAKeyPair(
      TpmUtility::AsymmetricKeyUsage::kDecryptKey, 2048, 0x10001, "password",
      "", false, kNoCreationPCR, nullptr, &key_blob, nullptr));
}

TEST_F(TpmUtilityTest, CreateRSAKeyPairFailure) {
  EXPECT_CALL(mock_tpm_, CreateSyncShort(kRSAStorageRootKey,
                                         _, _, _, _, _, _, _, _,
                                         &mock_authorization_delegate_))
      .WillOnce(Return(TPM_RC_FAILURE));
  std::string key_blob;
  EXPECT_EQ(TPM_RC_FAILURE, utility_.CreateRSAKeyPair(
      TpmUtility::AsymmetricKeyUsage::kSignKey, 2048, 0x10001, "password",
      "", false, kNoCreationPCR, &mock_authorization_delegate_, &key_blob,
      nullptr));
}

TEST_F(TpmUtilityTest, CreateRSAKeyPairKeyParserFail) {
  std::string key_blob;
  EXPECT_CALL(mock_blob_parser_, SerializeKeyBlob(_, _, &key_blob))
      .WillOnce(Return(false));
  EXPECT_EQ(SAPI_RC_BAD_TCTI_STRUCTURE, utility_.CreateRSAKeyPair(
      TpmUtility::AsymmetricKeyUsage::kSignKey, 2048, 0x10001, "password",
      "", false, kNoCreationPCR, &mock_authorization_delegate_, &key_blob,
      nullptr));
}

TEST_F(TpmUtilityTest, CreateRSAKeyPairCreationParserFail) {
  std::string creation_blob;
  std::string key_blob;
  EXPECT_CALL(mock_blob_parser_, SerializeCreationBlob(_, _, _, &creation_blob))
      .WillOnce(Return(false));
  EXPECT_EQ(SAPI_RC_BAD_TCTI_STRUCTURE, utility_.CreateRSAKeyPair(
      TpmUtility::AsymmetricKeyUsage::kSignKey, 2048, 0x10001, "password",
      "", false, kNoCreationPCR, &mock_authorization_delegate_, &key_blob,
      &creation_blob));
}

TEST_F(TpmUtilityTest, LoadKeySuccess) {
  TPM_HANDLE key_handle = TPM_RH_FIRST;
  TPM_HANDLE loaded_handle;
  EXPECT_CALL(mock_tpm_, LoadSync(kRSAStorageRootKey, _, _, _, _, _,
                                  &mock_authorization_delegate_))
      .WillOnce(DoAll(SetArgPointee<4>(key_handle),
                      Return(TPM_RC_SUCCESS)));
  std::string key_blob;
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.LoadKey(
      key_blob, &mock_authorization_delegate_, &loaded_handle));
  EXPECT_EQ(loaded_handle, key_handle);
}

TEST_F(TpmUtilityTest, LoadKeyFailure) {
  TPM_HANDLE key_handle;
  EXPECT_CALL(mock_tpm_, LoadSync(_, _, _, _, _, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  std::string key_blob;
  EXPECT_EQ(TPM_RC_FAILURE, utility_.LoadKey(
      key_blob, &mock_authorization_delegate_, &key_handle));
}

TEST_F(TpmUtilityTest, LoadKeyBadDelegate) {
  TPM_HANDLE key_handle;
  std::string key_blob;
  EXPECT_EQ(SAPI_RC_INVALID_SESSIONS, utility_.LoadKey(
      key_blob, nullptr, &key_handle));
}

TEST_F(TpmUtilityTest, LoadKeyParserFail) {
  TPM_HANDLE key_handle;
  std::string key_blob;
  EXPECT_CALL(mock_blob_parser_, ParseKeyBlob(key_blob, _, _))
      .WillOnce(Return(false));
  EXPECT_EQ(SAPI_RC_BAD_TCTI_STRUCTURE, utility_.LoadKey(
      key_blob, &mock_authorization_delegate_, &key_handle));
}

TEST_F(TpmUtilityTest, SealedDataSuccess) {
  std::string data_to_seal("seal_data");
  std::string sealed_data;
  TPM2B_SENSITIVE_CREATE sensitive_create;
  TPM2B_PUBLIC in_public;
  EXPECT_CALL(mock_tpm_, CreateSyncShort(kRSAStorageRootKey, _, _,
                                         _, _, _, _, _, _, _))
      .WillOnce(DoAll(SaveArg<1>(&sensitive_create),
                      SaveArg<2>(&in_public),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.SealData(
      data_to_seal, "", &mock_authorization_delegate_, &sealed_data));
  EXPECT_EQ(sensitive_create.sensitive.data.size, data_to_seal.size());
  EXPECT_EQ(0, memcmp(sensitive_create.sensitive.data.buffer,
                      data_to_seal.data(), data_to_seal.size()));
  EXPECT_EQ(in_public.public_area.type, TPM_ALG_KEYEDHASH);
  EXPECT_EQ(in_public.public_area.name_alg, TPM_ALG_SHA256);
}

TEST_F(TpmUtilityTest, SealDataBadDelegate) {
  std::string data_to_seal("seal_data");
  std::string sealed_data;
  EXPECT_EQ(SAPI_RC_INVALID_SESSIONS, utility_.SealData(
      data_to_seal, "", nullptr, &sealed_data));
}

TEST_F(TpmUtilityTest, SealDataFailure) {
  std::string data_to_seal("seal_data");
  std::string sealed_data;
  EXPECT_CALL(mock_tpm_, CreateSyncShort(kRSAStorageRootKey, _, _,
                                         _, _, _, _, _, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.SealData(
      data_to_seal, "", &mock_authorization_delegate_, &sealed_data));
}

TEST_F(TpmUtilityTest, SealDataParserFail) {
  std::string data_to_seal("seal_data");
  std::string sealed_data;
  EXPECT_CALL(mock_blob_parser_, SerializeKeyBlob(_, _, &sealed_data))
      .WillOnce(Return(false));
  EXPECT_EQ(SAPI_RC_BAD_TCTI_STRUCTURE, utility_.SealData(
      data_to_seal, "", &mock_authorization_delegate_, &sealed_data));
}

TEST_F(TpmUtilityTest, UnsealDataSuccess) {
  std::string sealed_data;
  std::string tpm_unsealed_data("password");
  std::string unsealed_data;
  TPM_HANDLE object_handle = 42;
  TPM2B_PUBLIC public_data;
  public_data.public_area.auth_policy.size = 0;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(_, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_data),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_CALL(mock_tpm_, ReadPublicSync(object_handle, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<2>(public_data),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_CALL(mock_tpm_, LoadSync(_, _, _, _, _, _, _))
      .WillOnce(DoAll(SetArgPointee<4>(object_handle),
                      Return(TPM_RC_SUCCESS)));
  TPM2B_SENSITIVE_DATA out_data = Make_TPM2B_SENSITIVE_DATA(tpm_unsealed_data);
  EXPECT_CALL(mock_tpm_, UnsealSync(object_handle, _, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(out_data),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.UnsealData(
      sealed_data, &mock_authorization_delegate_, &unsealed_data));
  EXPECT_EQ(unsealed_data, tpm_unsealed_data);
}

TEST_F(TpmUtilityTest, UnsealDataBadDelegate) {
  std::string sealed_data;
  std::string unsealed_data;
  EXPECT_EQ(SAPI_RC_INVALID_SESSIONS, utility_.UnsealData(
      sealed_data, nullptr, &unsealed_data));
}

TEST_F(TpmUtilityTest, UnsealDataLoadFail) {
  std::string sealed_data;
  std::string unsealed_data;
  EXPECT_CALL(mock_tpm_, LoadSync(_, _, _, _, _, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.UnsealData(
      sealed_data, &mock_authorization_delegate_, &unsealed_data));
}

TEST_F(TpmUtilityTest, UnsealDataBadKeyName) {
  std::string sealed_data;
  std::string unsealed_data;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(_, _, _, _, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.UnsealData(
      sealed_data, &mock_authorization_delegate_, &unsealed_data));
}

TEST_F(TpmUtilityTest, UnsealObjectFailure) {
  std::string sealed_data;
  std::string unsealed_data;
  EXPECT_CALL(mock_tpm_, UnsealSync(_, _, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.UnsealData(
      sealed_data, &mock_authorization_delegate_, &unsealed_data));
}

TEST_F(TpmUtilityTest, StartSessionSuccess) {
  EXPECT_CALL(mock_hmac_session_, StartUnboundSession(true))
      .WillOnce(Return(TPM_RC_SUCCESS));
  EXPECT_EQ(TPM_RC_SUCCESS,
      utility_.StartSession(&mock_hmac_session_));
}

TEST_F(TpmUtilityTest, StartSessionFailure) {
  EXPECT_CALL(mock_hmac_session_, StartUnboundSession(true))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE,
      utility_.StartSession(&mock_hmac_session_));
}

TEST_F(TpmUtilityTest, GetPolicyDigestForPcrValueSuccess) {
  int index = 5;
  std::string pcr_value("pcr_value");
  std::string policy_digest;
  TPML_PCR_SELECTION pcr_select;
  pcr_select.count = 1;
  pcr_select.pcr_selections[0].hash = TPM_ALG_SHA256;
  pcr_select.pcr_selections[0].sizeof_select = 1;
  pcr_select.pcr_selections[0].pcr_select[index / 8] = 1 << (index % 8);
  TPML_DIGEST pcr_values;
  pcr_values.count = 1;
  pcr_values.digests[0] = Make_TPM2B_DIGEST(pcr_value);
  EXPECT_CALL(mock_tpm_, PCR_ReadSync(_, _, _, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(pcr_select),
                      SetArgPointee<3>(pcr_values),
                      Return(TPM_RC_SUCCESS)));
  std::string tpm_pcr_value;
  EXPECT_CALL(mock_policy_session_, PolicyPCR(index, _))
      .WillOnce(DoAll(SaveArg<1>(&tpm_pcr_value),
                      Return(TPM_RC_SUCCESS)));
  std::string tpm_policy_digest("digest");
  EXPECT_CALL(mock_policy_session_, GetDigest(_))
      .WillOnce(DoAll(SetArgPointee<0>(tpm_policy_digest),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_SUCCESS,
      utility_.GetPolicyDigestForPcrValue(index, "", &policy_digest));
  EXPECT_EQ(policy_digest, tpm_policy_digest);
  EXPECT_EQ(pcr_value, tpm_pcr_value);
}

TEST_F(TpmUtilityTest, GetPolicyDigestForPcrValueSuccessWithPcrValue) {
  int index = 5;
  std::string pcr_value("pcr_value");
  std::string policy_digest;
  std::string tpm_pcr_value;
  EXPECT_CALL(mock_policy_session_, PolicyPCR(index, _))
      .WillOnce(DoAll(SaveArg<1>(&tpm_pcr_value),
                      Return(TPM_RC_SUCCESS)));
  std::string tpm_policy_digest("digest");
  EXPECT_CALL(mock_policy_session_, GetDigest(_))
      .WillOnce(DoAll(SetArgPointee<0>(tpm_policy_digest),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_SUCCESS,
      utility_.GetPolicyDigestForPcrValue(index, pcr_value, &policy_digest));
  EXPECT_EQ(policy_digest, tpm_policy_digest);
  EXPECT_EQ(pcr_value, tpm_pcr_value);
}

TEST_F(TpmUtilityTest, GetPolicyDigestForPcrValueBadSession) {
  int index = 5;
  std::string pcr_value("value");
  std::string policy_digest;
  EXPECT_CALL(mock_policy_session_, StartUnboundSession(false))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE,
      utility_.GetPolicyDigestForPcrValue(index, pcr_value, &policy_digest));
}

TEST_F(TpmUtilityTest, GetPolicyDigestForPcrValuePcrReadFail) {
  int index = 5;
  std::string policy_digest;
  EXPECT_CALL(mock_tpm_, PCR_ReadSync(_, _, _, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE,
      utility_.GetPolicyDigestForPcrValue(index, "", &policy_digest));
}

TEST_F(TpmUtilityTest, GetPolicyDigestForPcrValueBadPcr) {
  int index = 5;
  std::string pcr_value("value");
  std::string policy_digest;
  EXPECT_CALL(mock_policy_session_, PolicyPCR(index, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE,
      utility_.GetPolicyDigestForPcrValue(index, pcr_value, &policy_digest));
}

TEST_F(TpmUtilityTest, GetPolicyDigestForPcrValueBadDigest) {
  int index = 5;
  std::string pcr_value("value");
  std::string policy_digest;
  EXPECT_CALL(mock_policy_session_, GetDigest(&policy_digest))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE,
      utility_.GetPolicyDigestForPcrValue(index, pcr_value, &policy_digest));
}

TEST_F(TpmUtilityTest, DefineNVSpaceSuccess) {
  uint32_t index = 59;
  uint32_t nvram_index = NV_INDEX_FIRST + index;
  size_t length  = 256;
  TPM2B_NV_PUBLIC public_data;
  EXPECT_CALL(mock_tpm_, NV_DefineSpaceSync(TPM_RH_OWNER, _, _, _, _))
      .WillOnce(DoAll(SaveArg<3>(&public_data),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.DefineNVSpace(
      index, length, &mock_authorization_delegate_));
  EXPECT_EQ(public_data.nv_public.nv_index, nvram_index);
  EXPECT_EQ(public_data.nv_public.name_alg, TPM_ALG_SHA256);
  EXPECT_EQ(public_data.nv_public.attributes,
            TPMA_NV_NO_DA | TPMA_NV_OWNERWRITE | TPMA_NV_WRITEDEFINE |
                TPMA_NV_AUTHREAD);
  EXPECT_EQ(public_data.nv_public.data_size, length);
}

TEST_F(TpmUtilityTest, DefineNVSpaceBadLength) {
  size_t bad_length = 3000;
  EXPECT_EQ(SAPI_RC_BAD_SIZE,
      utility_.DefineNVSpace(0, bad_length, &mock_authorization_delegate_));
}

TEST_F(TpmUtilityTest, DefineNVSpaceBadIndex) {
  uint32_t bad_index = 1<<29;
  EXPECT_EQ(SAPI_RC_BAD_PARAMETER,
      utility_.DefineNVSpace(bad_index, 2, &mock_authorization_delegate_));
}

TEST_F(TpmUtilityTest, DefineNVSpaceBadSession) {
  EXPECT_EQ(SAPI_RC_INVALID_SESSIONS, utility_.DefineNVSpace(0, 2, nullptr));
}

TEST_F(TpmUtilityTest, DefineNVSpaceFail) {
  uint32_t index = 59;
  size_t length  = 256;
  EXPECT_CALL(mock_tpm_, NV_DefineSpaceSync(TPM_RH_OWNER, _, _, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE,
      utility_.DefineNVSpace(index, length, &mock_authorization_delegate_));
}

TEST_F(TpmUtilityTest, DestroyNVSpaceSuccess) {
  uint32_t index = 53;
  uint32_t nvram_index = NV_INDEX_FIRST + index;
  EXPECT_CALL(mock_tpm_,
              NV_UndefineSpaceSync(TPM_RH_OWNER, _, nvram_index, _, _));
  EXPECT_EQ(TPM_RC_SUCCESS,
            utility_.DestroyNVSpace(index, &mock_authorization_delegate_));
}

TEST_F(TpmUtilityTest, DestroyNVSpaceBadIndex) {
  uint32_t bad_index = 1<<29;
  EXPECT_EQ(SAPI_RC_BAD_PARAMETER,
            utility_.DestroyNVSpace(bad_index, &mock_authorization_delegate_));
}

TEST_F(TpmUtilityTest, DestroyNVSpaceBadSession) {
  EXPECT_EQ(SAPI_RC_INVALID_SESSIONS, utility_.DestroyNVSpace(3, nullptr));
}

TEST_F(TpmUtilityTest, DestroyNVSpaceFailure) {
  uint32_t index = 53;
  uint32_t nvram_index = NV_INDEX_FIRST + index;
  EXPECT_CALL(mock_tpm_,
              NV_UndefineSpaceSync(TPM_RH_OWNER, _, nvram_index, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE,
            utility_.DestroyNVSpace(index, &mock_authorization_delegate_));
}

TEST_F(TpmUtilityTest, LockNVSpaceSuccess) {
  uint32_t index = 53;
  uint32_t nvram_index = NV_INDEX_FIRST + index;
  EXPECT_CALL(mock_tpm_, NV_WriteLockSync(TPM_RH_OWNER, _, nvram_index, _, _))
      .WillOnce(Return(TPM_RC_SUCCESS));
  EXPECT_EQ(TPM_RC_SUCCESS,
            utility_.LockNVSpace(index, &mock_authorization_delegate_));
  TPMS_NV_PUBLIC public_area;
  EXPECT_EQ(TPM_RC_SUCCESS, GetNVRAMMap(index, &public_area));
  EXPECT_EQ(public_area.attributes & TPMA_NV_WRITELOCKED, TPMA_NV_WRITELOCKED);
}

TEST_F(TpmUtilityTest, LockNVSpaceBadIndex) {
  uint32_t bad_index = 1<<24;
  EXPECT_EQ(SAPI_RC_BAD_PARAMETER,
            utility_.LockNVSpace(bad_index, &mock_authorization_delegate_));
}

TEST_F(TpmUtilityTest, LockNVSpaceBadSession) {
  EXPECT_EQ(SAPI_RC_INVALID_SESSIONS, utility_.LockNVSpace(52, nullptr));
}

TEST_F(TpmUtilityTest, LockNVSpaceFailure) {
  uint32_t index = 53;
  uint32_t nvram_index = NV_INDEX_FIRST + index;
  EXPECT_CALL(mock_tpm_, NV_WriteLockSync(TPM_RH_OWNER, _, nvram_index, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE,
            utility_.LockNVSpace(index, &mock_authorization_delegate_));
}

TEST_F(TpmUtilityTest, WriteNVSpaceSuccess) {
  uint32_t index = 53;
  uint32_t offset = 5;
  uint32_t nvram_index = NV_INDEX_FIRST + index;
  EXPECT_CALL(mock_tpm_,
              NV_WriteSync(TPM_RH_OWNER, _, nvram_index, _, _, offset, _))
      .WillOnce(Return(TPM_RC_SUCCESS));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.WriteNVSpace(
      index, offset, "", &mock_authorization_delegate_));
  TPMS_NV_PUBLIC public_area;
  EXPECT_EQ(TPM_RC_SUCCESS, GetNVRAMMap(index, &public_area));
  EXPECT_EQ(public_area.attributes & TPMA_NV_WRITTEN, TPMA_NV_WRITTEN);
}

TEST_F(TpmUtilityTest, WriteNVSpaceBadSize) {
  uint32_t index = 53;
  std::string nvram_data(1025, 0);
  EXPECT_EQ(SAPI_RC_BAD_SIZE, utility_.WriteNVSpace(
      index, 0, nvram_data, &mock_authorization_delegate_));
}

TEST_F(TpmUtilityTest, WriteNVSpaceBadIndex) {
  uint32_t bad_index = 1<<24;
  EXPECT_EQ(SAPI_RC_BAD_PARAMETER, utility_.WriteNVSpace(
      bad_index, 0, "", &mock_authorization_delegate_));
}

TEST_F(TpmUtilityTest, WriteNVSpaceBadSessions) {
  EXPECT_EQ(SAPI_RC_INVALID_SESSIONS,
            utility_.WriteNVSpace(53, 0, "", nullptr));
}

TEST_F(TpmUtilityTest, WriteNVSpaceFailure) {
  uint32_t index = 53;
  uint32_t offset = 5;
  uint32_t nvram_index = NV_INDEX_FIRST + index;
  EXPECT_CALL(mock_tpm_,
              NV_WriteSync(TPM_RH_OWNER, _, nvram_index, _, _, offset, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.WriteNVSpace(
      index, offset, "", &mock_authorization_delegate_));
}

TEST_F(TpmUtilityTest, ReadNVSpaceSuccess) {
  uint32_t index = 53;
  uint32_t offset = 5;
  uint32_t nv_index = NV_INDEX_FIRST + index;
  size_t length = 24;
  std::string nvram_data;
  EXPECT_CALL(mock_tpm_,
              NV_ReadSync(nv_index, _, nv_index, _, length, offset, _, _))
      .WillOnce(Return(TPM_RC_SUCCESS));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.ReadNVSpace(
      index, offset, length, &nvram_data, &mock_authorization_delegate_));
}

TEST_F(TpmUtilityTest, ReadNVSpaceBadReadLength) {
  size_t length = 1025;
  std::string nvram_data;
  EXPECT_EQ(SAPI_RC_BAD_SIZE, utility_.ReadNVSpace(
      52, 0, length, &nvram_data, &mock_authorization_delegate_));
}

TEST_F(TpmUtilityTest, ReadNVSpaceBadIndex) {
  uint32_t bad_index = 1<<24;
  std::string nvram_data;
  EXPECT_EQ(SAPI_RC_BAD_PARAMETER, utility_.ReadNVSpace(
      bad_index, 0, 5, &nvram_data, &mock_authorization_delegate_));
}

TEST_F(TpmUtilityTest, ReadNVSpaceBadSession) {
  std::string nvram_data;
  EXPECT_EQ(SAPI_RC_INVALID_SESSIONS,
            utility_.ReadNVSpace(53, 0, 5, &nvram_data, nullptr));
}

TEST_F(TpmUtilityTest, ReadNVSpaceFailure) {
  uint32_t index = 53;
  uint32_t offset = 5;
  uint32_t nv_index = NV_INDEX_FIRST + index;
  size_t length = 24;
  std::string nvram_data;
  EXPECT_CALL(mock_tpm_,
              NV_ReadSync(nv_index, _, nv_index, _, length, offset, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.ReadNVSpace(
      index, offset, length, &nvram_data, &mock_authorization_delegate_));
}

TEST_F(TpmUtilityTest, GetNVSpaceNameSuccess) {
  uint32_t index = 53;
  uint32_t nvram_index = NV_INDEX_FIRST + index;
  std::string name;
  EXPECT_CALL(mock_tpm_, NV_ReadPublicSync(nvram_index, _, _, _, _))
      .WillOnce(Return(TPM_RC_SUCCESS));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.GetNVSpaceName(index, &name));
}

TEST_F(TpmUtilityTest, GetNVSpaceNameFailure) {
  uint32_t index = 53;
  std::string name;
  EXPECT_CALL(mock_tpm_, NV_ReadPublicSync(_, _, _, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.GetNVSpaceName(index, &name));
}

TEST_F(TpmUtilityTest, GetNVSpacePublicAreaCachedSuccess) {
  uint32_t index = 53;
  TPMS_NV_PUBLIC public_area;
  SetNVRAMMap(index, public_area);
  EXPECT_CALL(mock_tpm_, NV_ReadPublicSync(_, _, _, _, _))
      .Times(0);
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.GetNVSpacePublicArea(index, &public_area));
}

TEST_F(TpmUtilityTest, GetNVSpacePublicAreaSuccess) {
  uint32_t index = 53;
  uint32_t nvram_index = NV_INDEX_FIRST + index;
  TPMS_NV_PUBLIC public_area;
  EXPECT_CALL(mock_tpm_, NV_ReadPublicSync(nvram_index, _, _, _, _))
      .WillOnce(Return(TPM_RC_SUCCESS));
  EXPECT_EQ(TPM_RC_SUCCESS, utility_.GetNVSpacePublicArea(index, &public_area));
}

TEST_F(TpmUtilityTest, GetNVSpacePublicAreaFailure) {
  uint32_t index = 53;
  TPMS_NV_PUBLIC public_area;
  EXPECT_CALL(mock_tpm_, NV_ReadPublicSync(_, _, _, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, utility_.GetNVSpacePublicArea(index, &public_area));
}

TEST_F(TpmUtilityTest, SetKnownPasswordSuccess) {
  EXPECT_CALL(mock_tpm_state_, IsOwnerPasswordSet())
      .WillOnce(Return(false));
  EXPECT_CALL(mock_tpm_, HierarchyChangeAuthSync(TPM_RH_OWNER, _, _, _))
      .WillOnce(Return(TPM_RC_SUCCESS));
  EXPECT_EQ(TPM_RC_SUCCESS, SetKnownOwnerPassword("password"));
}

TEST_F(TpmUtilityTest, SetKnownPasswordOwnershipDone) {
  EXPECT_EQ(TPM_RC_SUCCESS, SetKnownOwnerPassword("password"));
}

TEST_F(TpmUtilityTest, SetKnownPasswordFailure) {
  EXPECT_CALL(mock_tpm_state_, IsOwnerPasswordSet())
      .WillOnce(Return(false));
  EXPECT_CALL(mock_tpm_, HierarchyChangeAuthSync(TPM_RH_OWNER, _, _, _))
      .WillRepeatedly(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, SetKnownOwnerPassword("password"));
}

TEST_F(TpmUtilityTest, RootKeysSuccess) {
  EXPECT_EQ(TPM_RC_SUCCESS, CreateStorageRootKeys("password"));
}

TEST_F(TpmUtilityTest, RootKeysHandleConsistency) {
  TPM_HANDLE test_handle = 42;
  EXPECT_CALL(mock_tpm_, CreatePrimarySyncShort(_, _, _, _, _, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<3>(test_handle),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_CALL(mock_tpm_, EvictControlSync(_, _, test_handle, _, _, _))
      .WillRepeatedly(Return(TPM_RC_SUCCESS));
  EXPECT_EQ(TPM_RC_SUCCESS, CreateStorageRootKeys("password"));
}

TEST_F(TpmUtilityTest, RootKeysCreateFailure) {
  EXPECT_CALL(mock_tpm_, CreatePrimarySyncShort(_, _, _, _, _, _, _, _, _, _))
      .WillRepeatedly(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, CreateStorageRootKeys("password"));
}

TEST_F(TpmUtilityTest, RootKeysPersistFailure) {
  EXPECT_CALL(mock_tpm_, EvictControlSync(_, _, _, _, _, _))
      .WillRepeatedly(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, CreateStorageRootKeys("password"));
}

TEST_F(TpmUtilityTest, RootKeysAlreadyExist) {
  SetExistingKeyHandleExpectation(kRSAStorageRootKey);
  SetExistingKeyHandleExpectation(kECCStorageRootKey);
  EXPECT_EQ(TPM_RC_SUCCESS, CreateStorageRootKeys("password"));
}

TEST_F(TpmUtilityTest, SaltingKeySuccess) {
  TPM2B_PUBLIC public_area;
  EXPECT_CALL(mock_tpm_, CreateSyncShort(_, _, _, _, _, _, _, _, _, _))
      .WillOnce(DoAll(SaveArg<2>(&public_area),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_SUCCESS, CreateSaltingKey("password"));
  EXPECT_EQ(TPM_ALG_SHA256, public_area.public_area.name_alg);
}

TEST_F(TpmUtilityTest, SaltingKeyConsistency) {
  TPM_HANDLE test_handle = 42;
  EXPECT_CALL(mock_tpm_, LoadSync(_, _, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<4>(test_handle),
                            Return(TPM_RC_SUCCESS)));
  EXPECT_CALL(mock_tpm_, EvictControlSync(_, _, test_handle, _, _, _))
      .WillRepeatedly(Return(TPM_RC_SUCCESS));
  EXPECT_EQ(TPM_RC_SUCCESS, CreateSaltingKey("password"));
}

TEST_F(TpmUtilityTest, SaltingKeyCreateFailure) {
  EXPECT_CALL(mock_tpm_, CreateSyncShort(_, _, _, _, _, _, _, _, _, _))
      .WillRepeatedly(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, CreateSaltingKey("password"));
}

TEST_F(TpmUtilityTest, SaltingKeyLoadFailure) {
  EXPECT_CALL(mock_tpm_, LoadSync(_, _, _, _, _, _, _))
      .WillRepeatedly(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, CreateSaltingKey("password"));
}

TEST_F(TpmUtilityTest, SaltingKeyPersistFailure) {
  EXPECT_CALL(mock_tpm_, EvictControlSync(_, _, _, _, _, _))
      .WillRepeatedly(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, CreateSaltingKey("password"));
}

TEST_F(TpmUtilityTest, SaltingKeyAlreadyExists) {
  SetExistingKeyHandleExpectation(kSaltingKey);
  EXPECT_EQ(TPM_RC_SUCCESS, CreateSaltingKey("password"));
}

}  // namespace trunks
