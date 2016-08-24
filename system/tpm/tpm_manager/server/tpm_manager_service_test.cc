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

#include <base/run_loop.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "tpm_manager/server/mock_local_data_store.h"
#include "tpm_manager/server/mock_tpm_initializer.h"
#include "tpm_manager/server/mock_tpm_nvram.h"
#include "tpm_manager/server/mock_tpm_status.h"
#include "tpm_manager/server/tpm_manager_service.h"

using testing::_;
using testing::AtLeast;
using testing::Invoke;
using testing::NiceMock;
using testing::Return;
using testing::SaveArg;
using testing::SetArgPointee;

namespace {

const char kOwnerPassword[] = "owner";
const char kOwnerDependency[] = "owner_dependency";
const char kOtherDependency[] = "other_dependency";

}

namespace tpm_manager {

// A test fixture that takes care of message loop management and configuring a
// TpmManagerService instance with mock dependencies.
class TpmManagerServiceTest : public testing::Test {
 public:
  ~TpmManagerServiceTest() override = default;
  void SetUp() override {
    service_.reset(new TpmManagerService(true /*wait_for_ownership*/,
                                         &mock_local_data_store_,
                                         &mock_tpm_status_,
                                         &mock_tpm_initializer_,
                                         &mock_tpm_nvram_));
    SetupService();
  }

 protected:
  void Run() {
    run_loop_.Run();
  }

  void RunServiceWorkerAndQuit() {
    // Run out the service worker loop by posting a new command and waiting for
    // the response.
    auto callback = [this](const GetTpmStatusReply& reply) {
      Quit();
    };
    GetTpmStatusRequest request;
    service_->GetTpmStatus(request, base::Bind(callback));
    Run();
  }

  void Quit() {
    run_loop_.Quit();
  }

  void SetupService() {
    CHECK(service_->Initialize());
  }

  NiceMock<MockLocalDataStore> mock_local_data_store_;
  NiceMock<MockTpmInitializer> mock_tpm_initializer_;
  NiceMock<MockTpmNvram> mock_tpm_nvram_;
  NiceMock<MockTpmStatus> mock_tpm_status_;
  std::unique_ptr<TpmManagerService> service_;

 private:
  base::MessageLoop message_loop_;
  base::RunLoop run_loop_;
};

// Tests must call SetupService().
class TpmManagerServiceTest_NoWaitForOwnership : public TpmManagerServiceTest {
 public:
  ~TpmManagerServiceTest_NoWaitForOwnership() override = default;
  void SetUp() override {
    service_.reset(new TpmManagerService(false /*wait_for_ownership*/,
                                         &mock_local_data_store_,
                                         &mock_tpm_status_,
                                         &mock_tpm_initializer_,
                                         &mock_tpm_nvram_));
  }
};

TEST_F(TpmManagerServiceTest_NoWaitForOwnership, AutoInitialize) {
  // Make sure InitializeTpm doesn't get multiple calls.
  EXPECT_CALL(mock_tpm_initializer_, InitializeTpm()).Times(1);
  SetupService();
  RunServiceWorkerAndQuit();
}

TEST_F(TpmManagerServiceTest_NoWaitForOwnership, AutoInitializeNoTpm) {
  EXPECT_CALL(mock_tpm_status_, IsTpmEnabled()).WillRepeatedly(Return(false));
  EXPECT_CALL(mock_tpm_initializer_, InitializeTpm()).Times(0);
  SetupService();
  RunServiceWorkerAndQuit();
}

TEST_F(TpmManagerServiceTest_NoWaitForOwnership, AutoInitializeFailure) {
  EXPECT_CALL(mock_tpm_initializer_, InitializeTpm())
      .WillRepeatedly(Return(false));
  SetupService();
  RunServiceWorkerAndQuit();
}

TEST_F(TpmManagerServiceTest_NoWaitForOwnership,
       TakeOwnershipAfterAutoInitialize) {
  EXPECT_CALL(mock_tpm_initializer_, InitializeTpm()).Times(AtLeast(2));
  SetupService();
  auto callback = [this](const TakeOwnershipReply& reply) {
    EXPECT_EQ(STATUS_SUCCESS, reply.status());
    Quit();
  };
  TakeOwnershipRequest request;
  service_->TakeOwnership(request, base::Bind(callback));
  Run();
}

TEST_F(TpmManagerServiceTest, NoAutoInitialize) {
  EXPECT_CALL(mock_tpm_initializer_, InitializeTpm()).Times(0);
  RunServiceWorkerAndQuit();
}

TEST_F(TpmManagerServiceTest, GetTpmStatusSuccess) {
  EXPECT_CALL(mock_tpm_status_, GetDictionaryAttackInfo(_, _, _, _))
      .WillRepeatedly(Invoke([](int* counter, int* threshold, bool* lockout,
                                int* seconds_remaining) {
        *counter = 5;
        *threshold = 6;
        *lockout = true;
        *seconds_remaining = 7;
        return true;
      }));
  LocalData local_data;
  local_data.set_owner_password(kOwnerPassword);
  EXPECT_CALL(mock_local_data_store_, Read(_))
      .WillRepeatedly(DoAll(SetArgPointee<0>(local_data), Return(true)));

  auto callback = [this](const GetTpmStatusReply& reply) {
    EXPECT_EQ(STATUS_SUCCESS, reply.status());
    EXPECT_TRUE(reply.enabled());
    EXPECT_TRUE(reply.owned());
    EXPECT_EQ(kOwnerPassword, reply.local_data().owner_password());
    EXPECT_EQ(5, reply.dictionary_attack_counter());
    EXPECT_EQ(6, reply.dictionary_attack_threshold());
    EXPECT_TRUE(reply.dictionary_attack_lockout_in_effect());
    EXPECT_EQ(7, reply.dictionary_attack_lockout_seconds_remaining());
    Quit();
  };
  GetTpmStatusRequest request;
  service_->GetTpmStatus(request, base::Bind(callback));
  Run();
}

TEST_F(TpmManagerServiceTest, GetTpmStatusLocalDataFailure) {
  EXPECT_CALL(mock_local_data_store_, Read(_))
      .WillRepeatedly(Return(false));
  auto callback = [this](const GetTpmStatusReply& reply) {
    EXPECT_EQ(STATUS_SUCCESS, reply.status());
    EXPECT_TRUE(reply.enabled());
    EXPECT_TRUE(reply.owned());
    EXPECT_FALSE(reply.has_local_data());
    EXPECT_TRUE(reply.has_dictionary_attack_counter());
    EXPECT_TRUE(reply.has_dictionary_attack_threshold());
    EXPECT_TRUE(reply.has_dictionary_attack_lockout_in_effect());
    EXPECT_TRUE(reply.has_dictionary_attack_lockout_seconds_remaining());
    Quit();
  };
  GetTpmStatusRequest request;
  service_->GetTpmStatus(request, base::Bind(callback));
  Run();
}

TEST_F(TpmManagerServiceTest, GetTpmStatusNoTpm) {
  EXPECT_CALL(mock_tpm_status_, IsTpmEnabled()).WillRepeatedly(Return(false));
  EXPECT_CALL(mock_tpm_status_, GetDictionaryAttackInfo(_, _, _, _))
      .WillRepeatedly(Return(false));
  auto callback = [this](const GetTpmStatusReply& reply) {
    EXPECT_EQ(STATUS_SUCCESS, reply.status());
    EXPECT_FALSE(reply.enabled());
    EXPECT_TRUE(reply.owned());
    EXPECT_TRUE(reply.has_local_data());
    EXPECT_FALSE(reply.has_dictionary_attack_counter());
    EXPECT_FALSE(reply.has_dictionary_attack_threshold());
    EXPECT_FALSE(reply.has_dictionary_attack_lockout_in_effect());
    EXPECT_FALSE(reply.has_dictionary_attack_lockout_seconds_remaining());
    Quit();
  };
  GetTpmStatusRequest request;
  service_->GetTpmStatus(request, base::Bind(callback));
  Run();
}

TEST_F(TpmManagerServiceTest, TakeOwnershipSuccess) {
  // Make sure InitializeTpm doesn't get multiple calls.
  EXPECT_CALL(mock_tpm_initializer_, InitializeTpm()).Times(1);
  auto callback = [this](const TakeOwnershipReply& reply) {
    EXPECT_EQ(STATUS_SUCCESS, reply.status());
    Quit();
  };
  TakeOwnershipRequest request;
  service_->TakeOwnership(request, base::Bind(callback));
  Run();
}

TEST_F(TpmManagerServiceTest, TakeOwnershipFailure) {
  EXPECT_CALL(mock_tpm_initializer_, InitializeTpm())
      .WillRepeatedly(Return(false));
  auto callback = [this](const TakeOwnershipReply& reply) {
    EXPECT_EQ(STATUS_UNEXPECTED_DEVICE_ERROR, reply.status());
    Quit();
  };
  TakeOwnershipRequest request;
  service_->TakeOwnership(request, base::Bind(callback));
  Run();
}

TEST_F(TpmManagerServiceTest, TakeOwnershipNoTpm) {
  EXPECT_CALL(mock_tpm_status_, IsTpmEnabled()).WillRepeatedly(Return(false));
  auto callback = [this](const TakeOwnershipReply& reply) {
    EXPECT_EQ(STATUS_NOT_AVAILABLE, reply.status());
    Quit();
  };
  TakeOwnershipRequest request;
  service_->TakeOwnership(request, base::Bind(callback));
  Run();
}

TEST_F(TpmManagerServiceTest, RemoveOwnerDependencyReadFailure) {
  EXPECT_CALL(mock_local_data_store_, Read(_))
    .WillRepeatedly(Return(false));
  auto callback = [this](const RemoveOwnerDependencyReply& reply) {
    EXPECT_EQ(STATUS_UNEXPECTED_DEVICE_ERROR, reply.status());
    Quit();
  };
  RemoveOwnerDependencyRequest request;
  request.set_owner_dependency(kOwnerDependency);
  service_->RemoveOwnerDependency(request, base::Bind(callback));
  Run();
}

TEST_F(TpmManagerServiceTest, RemoveOwnerDependencyWriteFailure) {
  EXPECT_CALL(mock_local_data_store_, Write(_))
    .WillRepeatedly(Return(false));
  auto callback = [this](const RemoveOwnerDependencyReply& reply) {
    EXPECT_EQ(STATUS_UNEXPECTED_DEVICE_ERROR, reply.status());
    Quit();
  };
  RemoveOwnerDependencyRequest request;
  request.set_owner_dependency(kOwnerDependency);
  service_->RemoveOwnerDependency(request, base::Bind(callback));
  Run();
}

TEST_F(TpmManagerServiceTest, RemoveOwnerDependencyNotCleared) {
  LocalData local_data;
  local_data.set_owner_password(kOwnerPassword);
  local_data.add_owner_dependency(kOwnerDependency);
  local_data.add_owner_dependency(kOtherDependency);
  EXPECT_CALL(mock_local_data_store_, Read(_))
      .WillOnce(DoAll(SetArgPointee<0>(local_data),
                      Return(true)));
  EXPECT_CALL(mock_local_data_store_, Write(_))
      .WillOnce(DoAll(SaveArg<0>(&local_data),
                      Return(true)));
  auto callback = [this, &local_data](const RemoveOwnerDependencyReply& reply) {
    EXPECT_EQ(STATUS_SUCCESS, reply.status());
    EXPECT_EQ(1, local_data.owner_dependency_size());
    EXPECT_EQ(kOtherDependency, local_data.owner_dependency(0));
    EXPECT_TRUE(local_data.has_owner_password());
    EXPECT_EQ(kOwnerPassword, local_data.owner_password());
    Quit();
  };
  RemoveOwnerDependencyRequest request;
  request.set_owner_dependency(kOwnerDependency);
  service_->RemoveOwnerDependency(request, base::Bind(callback));
  Run();
}

TEST_F(TpmManagerServiceTest, RemoveOwnerDependencyCleared) {
  LocalData local_data;
  local_data.set_owner_password(kOwnerPassword);
  local_data.add_owner_dependency(kOwnerDependency);
  EXPECT_CALL(mock_local_data_store_, Read(_))
      .WillOnce(DoAll(SetArgPointee<0>(local_data),
                      Return(true)));
  EXPECT_CALL(mock_local_data_store_, Write(_))
      .WillOnce(DoAll(SaveArg<0>(&local_data),
                      Return(true)));
  auto callback = [this, &local_data](const RemoveOwnerDependencyReply& reply) {
    EXPECT_EQ(STATUS_SUCCESS, reply.status());
    EXPECT_EQ(0, local_data.owner_dependency_size());
    EXPECT_FALSE(local_data.has_owner_password());
    Quit();
  };
  RemoveOwnerDependencyRequest request;
  request.set_owner_dependency(kOwnerDependency);
  service_->RemoveOwnerDependency(request, base::Bind(callback));
  Run();
}

TEST_F(TpmManagerServiceTest, RemoveOwnerDependencyNotPresent) {
  LocalData local_data;
  local_data.set_owner_password(kOwnerPassword);
  local_data.add_owner_dependency(kOwnerDependency);
  EXPECT_CALL(mock_local_data_store_, Read(_))
      .WillOnce(DoAll(SetArgPointee<0>(local_data),
                      Return(true)));
  EXPECT_CALL(mock_local_data_store_, Write(_))
      .WillOnce(DoAll(SaveArg<0>(&local_data),
                      Return(true)));
  auto callback = [this, &local_data](const RemoveOwnerDependencyReply& reply) {
    EXPECT_EQ(STATUS_SUCCESS, reply.status());
    EXPECT_EQ(1, local_data.owner_dependency_size());
    EXPECT_EQ(kOwnerDependency, local_data.owner_dependency(0));
    EXPECT_TRUE(local_data.has_owner_password());
    EXPECT_EQ(kOwnerPassword, local_data.owner_password());
    Quit();
  };
  RemoveOwnerDependencyRequest request;
  request.set_owner_dependency(kOtherDependency);
  service_->RemoveOwnerDependency(request, base::Bind(callback));
  Run();
}

TEST_F(TpmManagerServiceTest, DefineNvramFailure) {
  uint32_t nvram_index = 5;
  size_t nvram_length = 32;
  EXPECT_CALL(mock_tpm_nvram_, DefineNvram(nvram_index, nvram_length))
      .WillRepeatedly(Return(false));
  auto callback = [this](const DefineNvramReply& reply) {
    EXPECT_EQ(STATUS_UNEXPECTED_DEVICE_ERROR, reply.status());
    Quit();
  };
  DefineNvramRequest request;
  request.set_index(nvram_index);
  request.set_length(nvram_length);
  service_->DefineNvram(request, base::Bind(callback));
  Run();
}

TEST_F(TpmManagerServiceTest, DefineNvramSuccess) {
  uint32_t nvram_index = 5;
  uint32_t nvram_length = 32;
  auto define_callback = [this](const DefineNvramReply& reply) {
    EXPECT_EQ(STATUS_SUCCESS, reply.status());
  };
  auto is_defined_callback = [this](const IsNvramDefinedReply& reply) {
    EXPECT_EQ(STATUS_SUCCESS, reply.status());
    EXPECT_EQ(true, reply.is_defined());
  };
  auto size_callback = [this, nvram_length](const GetNvramSizeReply& reply) {
    EXPECT_EQ(STATUS_SUCCESS, reply.status());
    EXPECT_EQ(nvram_length, reply.size());
  };
  DefineNvramRequest define_request;
  define_request.set_index(nvram_index);
  define_request.set_length(nvram_length);
  service_->DefineNvram(define_request, base::Bind(define_callback));
  IsNvramDefinedRequest is_defined_request;
  is_defined_request.set_index(nvram_index);
  service_->IsNvramDefined(is_defined_request, base::Bind(is_defined_callback));
  GetNvramSizeRequest size_request;
  size_request.set_index(nvram_index);
  service_->GetNvramSize(size_request, base::Bind(size_callback));
  RunServiceWorkerAndQuit();
}

TEST_F(TpmManagerServiceTest, DestroyUnitializedNvram) {
  auto callback = [this](const DestroyNvramReply& reply) {
    EXPECT_EQ(STATUS_UNEXPECTED_DEVICE_ERROR, reply.status());
    Quit();
  };
  DestroyNvramRequest request;
  service_->DestroyNvram(request, base::Bind(callback));
  Run();
}

TEST_F(TpmManagerServiceTest, DestroyNvramSuccess) {
  uint32_t nvram_index = 5;
  uint32_t nvram_length = 32;
  auto define_callback = [this](const DefineNvramReply& reply) {
    EXPECT_EQ(STATUS_SUCCESS, reply.status());
  };
  auto destroy_callback = [this](const DestroyNvramReply& reply) {
    EXPECT_EQ(STATUS_SUCCESS, reply.status());
  };
  DefineNvramRequest define_request;
  define_request.set_index(nvram_index);
  define_request.set_length(nvram_length);
  service_->DefineNvram(define_request, base::Bind(define_callback));
  DestroyNvramRequest destroy_request;
  destroy_request.set_index(nvram_index);
  service_->DestroyNvram(destroy_request, base::Bind(destroy_callback));
  RunServiceWorkerAndQuit();
}

TEST_F(TpmManagerServiceTest, DoubleDestroyNvram) {
  uint32_t nvram_index = 5;
  uint32_t nvram_length = 32;
  auto define_callback = [this](const DefineNvramReply& reply) {
    EXPECT_EQ(STATUS_SUCCESS, reply.status());
  };
  auto destroy_callback_success = [this](const DestroyNvramReply& reply) {
    EXPECT_EQ(STATUS_SUCCESS, reply.status());
  };
  auto destroy_callback_failure = [this](const DestroyNvramReply& reply) {
    EXPECT_EQ(STATUS_UNEXPECTED_DEVICE_ERROR, reply.status());
  };
  DefineNvramRequest define_request;
  define_request.set_index(nvram_index);
  define_request.set_length(nvram_length);
  service_->DefineNvram(define_request, base::Bind(define_callback));
  DestroyNvramRequest destroy_request;
  destroy_request.set_index(nvram_index);
  service_->DestroyNvram(destroy_request, base::Bind(destroy_callback_success));
  service_->DestroyNvram(destroy_request, base::Bind(destroy_callback_failure));
  RunServiceWorkerAndQuit();
}

TEST_F(TpmManagerServiceTest, WriteUninitializedNvram) {
  auto callback = [this](const WriteNvramReply& reply) {
    EXPECT_EQ(STATUS_UNEXPECTED_DEVICE_ERROR, reply.status());
    Quit();
  };
  WriteNvramRequest request;
  service_->WriteNvram(request, base::Bind(callback));
  Run();
}

TEST_F(TpmManagerServiceTest, WriteNvramIncorrectSize) {
  uint32_t nvram_index = 5;
  std::string nvram_data("nvram_data");
  auto define_callback = [this](const DefineNvramReply& reply) {
    EXPECT_EQ(STATUS_SUCCESS, reply.status());
  };
  auto write_callback = [this](const WriteNvramReply& reply) {
    EXPECT_EQ(STATUS_UNEXPECTED_DEVICE_ERROR, reply.status());
  };
  DefineNvramRequest define_request;
  define_request.set_index(nvram_index);
  define_request.set_length(nvram_data.size() - 1);
  service_->DefineNvram(define_request, base::Bind(define_callback));
  WriteNvramRequest write_request;
  write_request.set_index(nvram_index);
  write_request.set_data(nvram_data);
  service_->WriteNvram(write_request, base::Bind(write_callback));
  RunServiceWorkerAndQuit();
}

TEST_F(TpmManagerServiceTest, DoubleWrite) {
  uint32_t nvram_index = 5;
  std::string nvram_data("nvram_data");
  auto define_callback = [this](const DefineNvramReply& reply) {
    EXPECT_EQ(STATUS_SUCCESS, reply.status());
  };
  auto write_callback_success = [this](const WriteNvramReply& reply) {
    EXPECT_EQ(STATUS_SUCCESS, reply.status());
  };
  auto write_callback_failure = [this](const WriteNvramReply& reply) {
    EXPECT_EQ(STATUS_UNEXPECTED_DEVICE_ERROR, reply.status());
  };
  DefineNvramRequest define_request;
  define_request.set_index(nvram_index);
  define_request.set_length(nvram_data.size());
  service_->DefineNvram(define_request, base::Bind(define_callback));
  WriteNvramRequest write_request;
  write_request.set_index(nvram_index);
  write_request.set_data(nvram_data);
  service_->WriteNvram(write_request, base::Bind(write_callback_success));
  service_->WriteNvram(write_request, base::Bind(write_callback_failure));
  RunServiceWorkerAndQuit();
}

TEST_F(TpmManagerServiceTest, ReadUninitializedNvram) {
  auto callback = [this](const ReadNvramReply& reply) {
    EXPECT_EQ(STATUS_UNEXPECTED_DEVICE_ERROR, reply.status());
    Quit();
  };
  ReadNvramRequest request;
  service_->ReadNvram(request, base::Bind(callback));
  Run();
}

TEST_F(TpmManagerServiceTest, ReadUnwrittenNvram) {
  uint32_t nvram_index = 5;
  uint32_t nvram_length = 32;
  auto define_callback = [this](const DefineNvramReply& reply) {
    EXPECT_EQ(STATUS_SUCCESS, reply.status());
  };
  auto read_callback = [this](const ReadNvramReply& reply) {
    EXPECT_EQ(STATUS_UNEXPECTED_DEVICE_ERROR, reply.status());
  };
  DefineNvramRequest define_request;
  define_request.set_index(nvram_index);
  define_request.set_length(nvram_length);
  service_->DefineNvram(define_request, base::Bind(define_callback));
  ReadNvramRequest read_request;
  read_request.set_index(nvram_index);
  service_->ReadNvram(read_request, base::Bind(read_callback));
  RunServiceWorkerAndQuit();
}

TEST_F(TpmManagerServiceTest, ReadWriteNvramSuccess) {
  uint32_t nvram_index = 5;
  std::string nvram_data("nvram_data");
  auto define_callback = [this](const DefineNvramReply& reply) {
    EXPECT_EQ(STATUS_SUCCESS, reply.status());
  };
  auto write_callback = [this](const WriteNvramReply& reply) {
    EXPECT_EQ(STATUS_SUCCESS, reply.status());
  };
  auto read_callback = [this, nvram_data](const ReadNvramReply& reply) {
    EXPECT_EQ(STATUS_SUCCESS, reply.status());
    EXPECT_EQ(nvram_data, reply.data());
  };
  auto locked_callback = [this](const IsNvramLockedReply& reply) {
    EXPECT_EQ(STATUS_SUCCESS, reply.status());
    EXPECT_EQ(true, reply.is_locked());
  };
  DefineNvramRequest define_request;
  define_request.set_index(nvram_index);
  define_request.set_length(nvram_data.size());
  service_->DefineNvram(define_request, base::Bind(define_callback));
  WriteNvramRequest write_request;
  write_request.set_index(nvram_index);
  write_request.set_data(nvram_data);
  service_->WriteNvram(write_request, base::Bind(write_callback));
  ReadNvramRequest read_request;
  read_request.set_index(nvram_index);
  service_->ReadNvram(read_request, base::Bind(read_callback));
  IsNvramLockedRequest locked_request;
  locked_request.set_index(nvram_index);
  service_->IsNvramLocked(locked_request, base::Bind(locked_callback));
  RunServiceWorkerAndQuit();
}

}  // namespace tpm_manager
