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

#include <string>

#include <brillo/bind_lambda.h>
#include <brillo/dbus/dbus_object_test_helpers.h>
#include <dbus/mock_bus.h>
#include <dbus/mock_exported_object.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "tpm_manager/common/mock_tpm_nvram_interface.h"
#include "tpm_manager/common/mock_tpm_ownership_interface.h"
#include "tpm_manager/common/tpm_manager_constants.h"
#include "tpm_manager/common/tpm_nvram_dbus_interface.h"
#include "tpm_manager/common/tpm_ownership_dbus_interface.h"
#include "tpm_manager/server/dbus_service.h"

using testing::_;
using testing::Invoke;
using testing::NiceMock;
using testing::Return;
using testing::StrictMock;
using testing::WithArgs;

namespace tpm_manager {

class DBusServiceTest : public testing::Test {
 public:
  ~DBusServiceTest() override = default;
  void SetUp() override {
    dbus::Bus::Options options;
    mock_bus_ = new NiceMock<dbus::MockBus>(options);
    dbus::ObjectPath path(kTpmManagerServicePath);
    mock_exported_object_ = new NiceMock<dbus::MockExportedObject>(
        mock_bus_.get(), path);
    ON_CALL(*mock_bus_, GetExportedObject(path))
        .WillByDefault(Return(mock_exported_object_.get()));
    dbus_service_.reset(new DBusService(mock_bus_,
                                        &mock_nvram_service_,
                                        &mock_ownership_service_));
    dbus_service_->Register(brillo::dbus_utils::AsyncEventSequencer::
                                GetDefaultCompletionAction());
  }

  template<typename RequestProtobufType, typename ReplyProtobufType>
  void ExecuteMethod(const std::string& method_name,
                     const RequestProtobufType& request,
                     ReplyProtobufType* reply,
                     const std::string& interface) {
    std::unique_ptr<dbus::MethodCall> call = CreateMethodCall(method_name,
                                                              interface);
    dbus::MessageWriter writer(call.get());
    writer.AppendProtoAsArrayOfBytes(request);
    auto response = brillo::dbus_utils::testing::CallMethod(
        dbus_service_->dbus_object_, call.get());
    dbus::MessageReader reader(response.get());
    EXPECT_TRUE(reader.PopArrayOfBytesAsProto(reply));
  }

 protected:
  std::unique_ptr<dbus::MethodCall> CreateMethodCall(
      const std::string& method_name, const std::string& interface) {
    std::unique_ptr<dbus::MethodCall> call(new dbus::MethodCall(
        interface, method_name));
    call->SetSerial(1);
    return call;
  }

  scoped_refptr<dbus::MockBus> mock_bus_;
  scoped_refptr<dbus::MockExportedObject> mock_exported_object_;
  StrictMock<MockTpmNvramInterface> mock_nvram_service_;
  StrictMock<MockTpmOwnershipInterface> mock_ownership_service_;
  std::unique_ptr<DBusService> dbus_service_;
};

TEST_F(DBusServiceTest, CopyableCallback) {
  EXPECT_CALL(mock_ownership_service_, GetTpmStatus(_, _))
      .WillOnce(WithArgs<1>(Invoke([](
          const TpmOwnershipInterface::GetTpmStatusCallback& callback) {
            // Copy the callback, then call the original.
            GetTpmStatusReply reply;
            base::Closure copy = base::Bind(callback, reply);
            callback.Run(reply);
          })));
  GetTpmStatusRequest request;
  GetTpmStatusReply reply;
  ExecuteMethod(kGetTpmStatus, request, &reply, kTpmOwnershipInterface);
}

TEST_F(DBusServiceTest, GetTpmStatus) {
  GetTpmStatusRequest request;
  EXPECT_CALL(mock_ownership_service_, GetTpmStatus(_, _))
      .WillOnce(Invoke([](
          const GetTpmStatusRequest& request,
          const TpmOwnershipInterface::GetTpmStatusCallback& callback) {
        GetTpmStatusReply reply;
        reply.set_status(STATUS_SUCCESS);
        reply.set_enabled(true);
        reply.set_owned(true);
        reply.set_dictionary_attack_counter(3);
        reply.set_dictionary_attack_threshold(4);
        reply.set_dictionary_attack_lockout_in_effect(true);
        reply.set_dictionary_attack_lockout_seconds_remaining(5);
        callback.Run(reply);
      }));
  GetTpmStatusReply reply;
  ExecuteMethod(kGetTpmStatus, request, &reply, kTpmOwnershipInterface);
  EXPECT_EQ(STATUS_SUCCESS, reply.status());
  EXPECT_TRUE(reply.enabled());
  EXPECT_TRUE(reply.owned());
  EXPECT_EQ(3, reply.dictionary_attack_counter());
  EXPECT_EQ(4, reply.dictionary_attack_threshold());
  EXPECT_TRUE(reply.dictionary_attack_lockout_in_effect());
  EXPECT_EQ(5, reply.dictionary_attack_lockout_seconds_remaining());
}

TEST_F(DBusServiceTest, TakeOwnership) {
  EXPECT_CALL(mock_ownership_service_, TakeOwnership(_, _))
      .WillOnce(Invoke([](
          const TakeOwnershipRequest& request,
          const TpmOwnershipInterface::TakeOwnershipCallback& callback) {
        TakeOwnershipReply reply;
        reply.set_status(STATUS_SUCCESS);
        callback.Run(reply);
      }));
  TakeOwnershipRequest request;
  TakeOwnershipReply reply;
  ExecuteMethod(kTakeOwnership, request, &reply, kTpmOwnershipInterface);
  EXPECT_EQ(STATUS_SUCCESS, reply.status());
}

TEST_F(DBusServiceTest, RemoveOwnerDependency) {
  std::string owner_dependency("owner_dependency");
  RemoveOwnerDependencyRequest request;
  request.set_owner_dependency(owner_dependency);
  EXPECT_CALL(mock_ownership_service_, RemoveOwnerDependency(_, _))
      .WillOnce(Invoke([&owner_dependency](
          const RemoveOwnerDependencyRequest& request,
          const TpmOwnershipInterface::RemoveOwnerDependencyCallback& callback)
      {
        EXPECT_TRUE(request.has_owner_dependency());
        EXPECT_EQ(owner_dependency, request.owner_dependency());
        RemoveOwnerDependencyReply reply;
        reply.set_status(STATUS_SUCCESS);
        callback.Run(reply);
      }));
  RemoveOwnerDependencyReply reply;
  ExecuteMethod(kRemoveOwnerDependency,
                request,
                &reply,
                kTpmOwnershipInterface);
  EXPECT_EQ(STATUS_SUCCESS, reply.status());
}

TEST_F(DBusServiceTest, DefineNvram) {
  uint32_t nvram_index = 5;
  size_t nvram_length = 32;
  DefineNvramRequest request;
  request.set_index(nvram_index);
  request.set_length(nvram_length);
  EXPECT_CALL(mock_nvram_service_, DefineNvram(_, _))
      .WillOnce(Invoke([nvram_index, nvram_length](
          const DefineNvramRequest& request,
          const TpmNvramInterface::DefineNvramCallback& callback) {
        EXPECT_TRUE(request.has_index());
        EXPECT_EQ(nvram_index, request.index());
        EXPECT_TRUE(request.has_length());
        EXPECT_EQ(nvram_length, request.length());
        DefineNvramReply reply;
        reply.set_status(STATUS_SUCCESS);
        callback.Run(reply);
      }));
  DefineNvramReply reply;
  ExecuteMethod(kDefineNvram, request, &reply, kTpmNvramInterface);
  EXPECT_EQ(STATUS_SUCCESS, reply.status());
}

TEST_F(DBusServiceTest, DestroyNvram) {
  uint32_t nvram_index = 5;
  DestroyNvramRequest request;
  request.set_index(nvram_index);
  EXPECT_CALL(mock_nvram_service_, DestroyNvram(_, _))
      .WillOnce(Invoke([nvram_index](
          const DestroyNvramRequest& request,
          const TpmNvramInterface::DestroyNvramCallback& callback) {
        EXPECT_TRUE(request.has_index());
        EXPECT_EQ(nvram_index, request.index());
        DestroyNvramReply reply;
        reply.set_status(STATUS_SUCCESS);
        callback.Run(reply);
      }));
  DestroyNvramReply reply;
  ExecuteMethod(kDestroyNvram, request, &reply, kTpmNvramInterface);
  EXPECT_EQ(STATUS_SUCCESS, reply.status());
}

TEST_F(DBusServiceTest, WriteNvram) {
  uint32_t nvram_index = 5;
  std::string nvram_data("nvram_data");
  WriteNvramRequest request;
  request.set_index(nvram_index);
  request.set_data(nvram_data);
  EXPECT_CALL(mock_nvram_service_, WriteNvram(_, _))
      .WillOnce(Invoke([nvram_index, nvram_data](
          const WriteNvramRequest& request,
          const TpmNvramInterface::WriteNvramCallback& callback) {
        EXPECT_TRUE(request.has_index());
        EXPECT_EQ(nvram_index, request.index());
        EXPECT_TRUE(request.has_data());
        EXPECT_EQ(nvram_data, request.data());
        WriteNvramReply reply;
        reply.set_status(STATUS_SUCCESS);
        callback.Run(reply);
      }));
  WriteNvramReply reply;
  ExecuteMethod(kWriteNvram, request, &reply, kTpmNvramInterface);
  EXPECT_EQ(STATUS_SUCCESS, reply.status());
}

TEST_F(DBusServiceTest, ReadNvram) {
  uint32_t nvram_index = 5;
  std::string nvram_data("nvram_data");
  ReadNvramRequest request;
  request.set_index(nvram_index);
  EXPECT_CALL(mock_nvram_service_, ReadNvram(_, _))
      .WillOnce(Invoke([nvram_index, nvram_data](
          const ReadNvramRequest& request,
          const TpmNvramInterface::ReadNvramCallback& callback) {
        EXPECT_TRUE(request.has_index());
        EXPECT_EQ(nvram_index, request.index());
        ReadNvramReply reply;
        reply.set_status(STATUS_SUCCESS);
        reply.set_data(nvram_data);
        callback.Run(reply);
      }));
  ReadNvramReply reply;
  ExecuteMethod(kReadNvram, request, &reply, kTpmNvramInterface);
  EXPECT_EQ(STATUS_SUCCESS, reply.status());
  EXPECT_TRUE(reply.has_data());
  EXPECT_EQ(nvram_data, reply.data());
}

TEST_F(DBusServiceTest, IsNvramDefined) {
  uint32_t nvram_index = 5;
  bool nvram_defined = true;
  IsNvramDefinedRequest request;
  request.set_index(nvram_index);
  EXPECT_CALL(mock_nvram_service_, IsNvramDefined(_, _))
      .WillOnce(Invoke([nvram_index, nvram_defined](
          const IsNvramDefinedRequest& request,
          const TpmNvramInterface::IsNvramDefinedCallback& callback) {
        EXPECT_TRUE(request.has_index());
        EXPECT_EQ(nvram_index, request.index());
        IsNvramDefinedReply reply;
        reply.set_status(STATUS_SUCCESS);
        reply.set_is_defined(nvram_defined);
        callback.Run(reply);
      }));
  IsNvramDefinedReply reply;
  ExecuteMethod(kIsNvramDefined, request, &reply, kTpmNvramInterface);
  EXPECT_EQ(STATUS_SUCCESS, reply.status());
  EXPECT_TRUE(reply.has_is_defined());
  EXPECT_EQ(nvram_defined, reply.is_defined());
}

TEST_F(DBusServiceTest, IsNvramLocked) {
  uint32_t nvram_index = 5;
  bool nvram_locked = true;
  IsNvramLockedRequest request;
  request.set_index(nvram_index);
  EXPECT_CALL(mock_nvram_service_, IsNvramLocked(_, _))
      .WillOnce(Invoke([nvram_index, nvram_locked](
          const IsNvramLockedRequest& request,
          const TpmNvramInterface::IsNvramLockedCallback& callback) {
        EXPECT_TRUE(request.has_index());
        EXPECT_EQ(nvram_index, request.index());
        IsNvramLockedReply reply;
        reply.set_status(STATUS_SUCCESS);
        reply.set_is_locked(nvram_locked);
        callback.Run(reply);
      }));
  IsNvramLockedReply reply;
  ExecuteMethod(kIsNvramLocked, request, &reply, kTpmNvramInterface);
  EXPECT_EQ(STATUS_SUCCESS, reply.status());
  EXPECT_TRUE(reply.has_is_locked());
  EXPECT_EQ(nvram_locked, reply.is_locked());
}

TEST_F(DBusServiceTest, GetNvramSize) {
  uint32_t nvram_index = 5;
  size_t nvram_size = 32;
  GetNvramSizeRequest request;
  request.set_index(nvram_index);
  EXPECT_CALL(mock_nvram_service_, GetNvramSize(_, _))
      .WillOnce(Invoke([nvram_index, nvram_size](
          const GetNvramSizeRequest& request,
          const TpmNvramInterface::GetNvramSizeCallback& callback) {
        EXPECT_TRUE(request.has_index());
        EXPECT_EQ(nvram_index, request.index());
        GetNvramSizeReply reply;
        reply.set_status(STATUS_SUCCESS);
        reply.set_size(nvram_size);
        callback.Run(reply);
      }));
  GetNvramSizeReply reply;
  ExecuteMethod(kGetNvramSize, request, &reply, kTpmNvramInterface);
  EXPECT_EQ(STATUS_SUCCESS, reply.status());
  EXPECT_TRUE(reply.has_size());
  EXPECT_EQ(nvram_size, reply.size());
}

}  // namespace tpm_manager
