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

#include "trunks/scoped_key_handle.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "trunks/mock_tpm.h"
#include "trunks/tpm_generated.h"
#include "trunks/trunks_factory_for_test.h"

using testing::_;
using testing::DoAll;
using testing::Invoke;
using testing::NiceMock;
using testing::Return;
using testing::SetArgPointee;
using testing::WithArgs;

namespace trunks {

// A test fixture for TpmState tests.
class ScopedKeyHandleTest : public testing::Test {
 public:
  ScopedKeyHandleTest() {}
  ~ScopedKeyHandleTest() override {}

  void SetUp() override {
    factory_.set_tpm(&mock_tpm_);
  }

 protected:
  TrunksFactoryForTest factory_;
  NiceMock<MockTpm> mock_tpm_;
};

TEST_F(ScopedKeyHandleTest, FlushHandle) {
  TPM_HANDLE handle = TPM_RH_FIRST;
  ScopedKeyHandle scoped_handle(factory_, handle);
  EXPECT_CALL(mock_tpm_, FlushContextSync(handle, _))
      .WillOnce(Return(TPM_RC_SUCCESS));
}

TEST_F(ScopedKeyHandleTest, GetTest) {
  TPM_HANDLE handle = TPM_RH_FIRST;
  ScopedKeyHandle scoped_handle(factory_, handle);
  EXPECT_EQ(handle, scoped_handle.get());
}

TEST_F(ScopedKeyHandleTest, ReleaseTest) {
  TPM_HANDLE handle = TPM_RH_FIRST;
  ScopedKeyHandle scoped_handle(factory_, handle);
  EXPECT_EQ(handle, scoped_handle.release());
  EXPECT_EQ(0u, scoped_handle.get());
}

TEST_F(ScopedKeyHandleTest, ResetAndFlush) {
  TPM_HANDLE old_handle = TPM_RH_FIRST;
  TPM_HANDLE new_handle = TPM_RH_NULL;
  ScopedKeyHandle scoped_handle(factory_, old_handle);
  EXPECT_EQ(old_handle, scoped_handle.get());
  EXPECT_CALL(mock_tpm_, FlushContextSync(old_handle, _))
      .WillOnce(Return(TPM_RC_SUCCESS));
  scoped_handle.reset(new_handle);
  EXPECT_EQ(new_handle, scoped_handle.get());
  EXPECT_CALL(mock_tpm_, FlushContextSync(new_handle, _))
      .WillOnce(Return(TPM_RC_SUCCESS));
}

TEST_F(ScopedKeyHandleTest, NullReset) {
  TPM_HANDLE handle = TPM_RH_FIRST;
  ScopedKeyHandle scoped_handle(factory_, handle);
  EXPECT_EQ(handle, scoped_handle.get());
  EXPECT_CALL(mock_tpm_, FlushContextSync(handle, _))
      .WillOnce(Return(TPM_RC_SUCCESS));
  scoped_handle.reset();
  EXPECT_EQ(0u, scoped_handle.get());
}

}  // namespace trunks
