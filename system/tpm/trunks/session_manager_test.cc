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

#include "trunks/session_manager_impl.h"

#include <vector>

#include <base/logging.h>
#include <base/strings/string_number_conversions.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "trunks/error_codes.h"
#include "trunks/mock_tpm.h"
#include "trunks/tpm_generated.h"
#include "trunks/tpm_utility.h"
#include "trunks/trunks_factory_for_test.h"

using testing::_;
using testing::DoAll;
using testing::NiceMock;
using testing::Return;
using testing::SetArgPointee;

namespace trunks {

class SessionManagerTest : public testing::Test {
 public:
  SessionManagerTest() : session_manager_(factory_) {
    delegate_ = new HmacAuthorizationDelegate();
  }
  ~SessionManagerTest() override {}

  void SetUp() override {
    factory_.set_tpm(&mock_tpm_);
  }

  void SetHandle(TPM_HANDLE handle) {
    session_manager_.session_handle_ = handle;
  }

  TPM2B_PUBLIC_KEY_RSA GetValidRSAPublicKey() {
    const char kValidModulus[] =
        "A1D50D088994000492B5F3ED8A9C5FC8772706219F4C063B2F6A8C6B74D3AD6B"
        "212A53D01DABB34A6261288540D420D3BA59ED279D859DE6227A7AB6BD88FADD"
        "FC3078D465F4DF97E03A52A587BD0165AE3B180FE7B255B7BEDC1BE81CB1383F"
        "E9E46F9312B1EF28F4025E7D332E33F4416525FEB8F0FC7B815E8FBB79CDABE6"
        "327B5A155FEF13F559A7086CB8A543D72AD6ECAEE2E704FF28824149D7F4E393"
        "D3C74E721ACA97F7ADBE2CCF7B4BCC165F7380F48065F2C8370F25F066091259"
        "D14EA362BAF236E3CD8771A94BDEDA3900577143A238AB92B6C55F11DEFAFB31"
        "7D1DC5B6AE210C52B008D87F2A7BFF6EB5C4FB32D6ECEC6505796173951A3167";
    std::vector<uint8_t> bytes;
    CHECK(base::HexStringToBytes(kValidModulus, &bytes));
    CHECK_EQ(bytes.size(), 256u);
    TPM2B_PUBLIC_KEY_RSA rsa;
    rsa.size = bytes.size();
    memcpy(rsa.buffer, bytes.data(), bytes.size());
    return rsa;
  }

 protected:
  TrunksFactoryForTest factory_;
  NiceMock<MockTpm> mock_tpm_;
  HmacAuthorizationDelegate* delegate_;
  SessionManagerImpl session_manager_;
};

TEST_F(SessionManagerTest, CloseSessionSuccess) {
  TPM_HANDLE handle = TPM_RH_FIRST;
  SetHandle(handle);
  EXPECT_CALL(mock_tpm_, FlushContextSync(handle, nullptr))
      .WillOnce(Return(TPM_RC_SUCCESS));
  session_manager_.CloseSession();
}

TEST_F(SessionManagerTest, CloseSessionNoHandle) {
  TPM_HANDLE handle = kUninitializedHandle;
  SetHandle(handle);
  EXPECT_CALL(mock_tpm_, FlushContextSync(handle, nullptr))
      .Times(0);
  session_manager_.CloseSession();
}

TEST_F(SessionManagerTest, GetSessionHandleTest) {
  TPM_HANDLE handle = TPM_RH_FIRST;
  EXPECT_EQ(kUninitializedHandle, session_manager_.GetSessionHandle());
  SetHandle(handle);
  EXPECT_EQ(handle, session_manager_.GetSessionHandle());
}


TEST_F(SessionManagerTest, StartSessionSuccess) {
  TPM_SE session_type = TPM_SE_TRIAL;
  TPM2B_PUBLIC public_data;
  public_data.public_area.unique.rsa = GetValidRSAPublicKey();
  EXPECT_CALL(mock_tpm_, ReadPublicSync(kSaltingKey, _, _, _, _, nullptr))
      .WillOnce(DoAll(SetArgPointee<2>(public_data),
                      Return(TPM_RC_SUCCESS)));
  TPM_HANDLE handle = TPM_RH_FIRST;
  TPM2B_NONCE nonce;
  nonce.size = 20;
  EXPECT_CALL(mock_tpm_, StartAuthSessionSyncShort(_, handle,
                                                   _, _, session_type, _, _,
                                                   _, _, _))
      .WillOnce(DoAll(SetArgPointee<8>(nonce),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_SUCCESS, session_manager_.StartSession(session_type,
                                                          handle, "", false,
                                                          delegate_));
}

TEST_F(SessionManagerTest, StartSessionBadSaltingKey) {
  TPM2B_PUBLIC public_data;
  public_data.public_area.unique.rsa.size = 32;
  EXPECT_CALL(mock_tpm_, ReadPublicSync(kSaltingKey, _, _, _, _, nullptr))
      .WillOnce(DoAll(SetArgPointee<2>(public_data),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TRUNKS_RC_SESSION_SETUP_ERROR,
            session_manager_.StartSession(TPM_SE_TRIAL, TPM_RH_NULL, "", false,
                                          delegate_));
}

TEST_F(SessionManagerTest, StartSessionFailure) {
  TPM2B_PUBLIC public_data;
  public_data.public_area.unique.rsa = GetValidRSAPublicKey();
  EXPECT_CALL(mock_tpm_, ReadPublicSync(kSaltingKey, _, _, _, _, nullptr))
      .WillOnce(DoAll(SetArgPointee<2>(public_data),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_CALL(mock_tpm_, StartAuthSessionSyncShort(_,
                                                   TPM_RH_NULL,
                                                   _, _, _, _, _, _, _, _))
      .WillOnce(Return(TPM_RC_FAILURE));
  EXPECT_EQ(TPM_RC_FAILURE, session_manager_.StartSession(TPM_SE_TRIAL,
                                                          TPM_RH_NULL, "",
                                                          false, delegate_));
}

TEST_F(SessionManagerTest, StartSessionBadNonce) {
  TPM_SE session_type = TPM_SE_TRIAL;
  TPM2B_PUBLIC public_data;
  public_data.public_area.unique.rsa = GetValidRSAPublicKey();
  EXPECT_CALL(mock_tpm_, ReadPublicSync(kSaltingKey, _, _, _, _, nullptr))
      .WillOnce(DoAll(SetArgPointee<2>(public_data),
                      Return(TPM_RC_SUCCESS)));
  TPM_HANDLE handle = TPM_RH_FIRST;
  TPM2B_NONCE nonce;
  nonce.size = 0;
  EXPECT_CALL(mock_tpm_, StartAuthSessionSyncShort(_, handle,
                                                   _, _, session_type, _, _,
                                                   _, _, _))
      .WillOnce(DoAll(SetArgPointee<8>(nonce),
                      Return(TPM_RC_SUCCESS)));
  EXPECT_EQ(TPM_RC_FAILURE, session_manager_.StartSession(session_type,
                                                          handle, "", false,
                                                          delegate_));
}

}  // namespace trunks
