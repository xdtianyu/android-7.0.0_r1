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
#include <dbus/mock_object_proxy.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "tpm_manager/client/tpm_ownership_dbus_proxy.h"

using testing::_;
using testing::Invoke;
using testing::StrictMock;
using testing::WithArgs;

namespace tpm_manager {

class TpmOwnershipDBusProxyTest : public testing::Test {
 public:
  ~TpmOwnershipDBusProxyTest() override = default;
  void SetUp() override {
    mock_object_proxy_ = new StrictMock<dbus::MockObjectProxy>(
        nullptr, "", dbus::ObjectPath(""));
    proxy_.set_object_proxy(mock_object_proxy_.get());
  }

 protected:
  scoped_refptr<StrictMock<dbus::MockObjectProxy>> mock_object_proxy_;
  TpmOwnershipDBusProxy proxy_;
};

TEST_F(TpmOwnershipDBusProxyTest, GetTpmStatus) {
  auto fake_dbus_call = [](
      dbus::MethodCall* method_call,
      const dbus::MockObjectProxy::ResponseCallback& response_callback) {
    // Verify request protobuf.
    dbus::MessageReader reader(method_call);
    GetTpmStatusRequest request;
    EXPECT_TRUE(reader.PopArrayOfBytesAsProto(&request));
    // Create reply protobuf.
    auto response = dbus::Response::CreateEmpty();
    dbus::MessageWriter writer(response.get());
    GetTpmStatusReply reply;
    reply.set_status(STATUS_SUCCESS);
    reply.set_enabled(true);
    reply.set_owned(true);
    reply.set_dictionary_attack_counter(3);
    reply.set_dictionary_attack_threshold(4);
    reply.set_dictionary_attack_lockout_in_effect(true);
    reply.set_dictionary_attack_lockout_seconds_remaining(5);
    writer.AppendProtoAsArrayOfBytes(reply);
    response_callback.Run(response.release());
  };
  EXPECT_CALL(*mock_object_proxy_, CallMethodWithErrorCallback(_, _, _, _))
      .WillOnce(WithArgs<0, 2>(Invoke(fake_dbus_call)));

  // Set expectations on the outputs.
  int callback_count = 0;
  auto callback = [&callback_count](const GetTpmStatusReply& reply) {
    callback_count++;
    EXPECT_EQ(STATUS_SUCCESS, reply.status());
    EXPECT_TRUE(reply.enabled());
    EXPECT_TRUE(reply.owned());
    EXPECT_EQ(3, reply.dictionary_attack_counter());
    EXPECT_EQ(4, reply.dictionary_attack_threshold());
    EXPECT_TRUE(reply.dictionary_attack_lockout_in_effect());
    EXPECT_EQ(5, reply.dictionary_attack_lockout_seconds_remaining());
  };
  GetTpmStatusRequest request;
  proxy_.GetTpmStatus(request, base::Bind(callback));
  EXPECT_EQ(1, callback_count);
}

TEST_F(TpmOwnershipDBusProxyTest, TakeOwnership) {
  auto fake_dbus_call = [](
      dbus::MethodCall* method_call,
      const dbus::MockObjectProxy::ResponseCallback& response_callback) {
    // Verify request protobuf.
    dbus::MessageReader reader(method_call);
    TakeOwnershipRequest request;
    EXPECT_TRUE(reader.PopArrayOfBytesAsProto(&request));
    // Create reply protobuf.
    auto response = dbus::Response::CreateEmpty();
    dbus::MessageWriter writer(response.get());
    TakeOwnershipReply reply;
    reply.set_status(STATUS_SUCCESS);
    writer.AppendProtoAsArrayOfBytes(reply);
    response_callback.Run(response.release());
  };
  EXPECT_CALL(*mock_object_proxy_, CallMethodWithErrorCallback(_, _, _, _))
      .WillOnce(WithArgs<0, 2>(Invoke(fake_dbus_call)));

  // Set expectations on the outputs.
  int callback_count = 0;
  auto callback = [&callback_count](const TakeOwnershipReply& reply) {
    callback_count++;
    EXPECT_EQ(STATUS_SUCCESS, reply.status());
  };
  TakeOwnershipRequest request;
  proxy_.TakeOwnership(request, base::Bind(callback));
  EXPECT_EQ(1, callback_count);
}

TEST_F(TpmOwnershipDBusProxyTest, RemoveOwnerDependency) {
  const std::string owner_dependency("owner");
  auto fake_dbus_call = [&owner_dependency](
      dbus::MethodCall* method_call,
      const dbus::MockObjectProxy::ResponseCallback& response_callback) {
    // Verify request protobuf.
    dbus::MessageReader reader(method_call);
    RemoveOwnerDependencyRequest request;
    EXPECT_TRUE(reader.PopArrayOfBytesAsProto(&request));
    EXPECT_TRUE(request.has_owner_dependency());
    EXPECT_EQ(owner_dependency, request.owner_dependency());
    // Create reply protobuf.
    auto response = dbus::Response::CreateEmpty();
    dbus::MessageWriter writer(response.get());
    RemoveOwnerDependencyReply reply;
    reply.set_status(STATUS_SUCCESS);
    writer.AppendProtoAsArrayOfBytes(reply);
    response_callback.Run(response.release());
  };
  EXPECT_CALL(*mock_object_proxy_, CallMethodWithErrorCallback(_, _, _, _))
      .WillOnce(WithArgs<0, 2>(Invoke(fake_dbus_call)));

  // Set expectations on the outputs.
  int callback_count = 0;
  auto callback = [&callback_count](const RemoveOwnerDependencyReply& reply) {
    callback_count++;
    EXPECT_EQ(STATUS_SUCCESS, reply.status());
  };
  RemoveOwnerDependencyRequest request;
  request.set_owner_dependency(owner_dependency);
  proxy_.RemoveOwnerDependency(request, base::Bind(callback));
  EXPECT_EQ(1, callback_count);
}

}  // namespace tpm_manager
