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

#include "trunks/resource_manager.h"

#include <string>
#include <vector>

#include <base/bind.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "trunks/error_codes.h"
#include "trunks/mock_command_transceiver.h"
#include "trunks/mock_tpm.h"
#include "trunks/trunks_factory_for_test.h"

using testing::_;
using testing::DoAll;
using testing::Eq;
using testing::Field;
using testing::InSequence;
using testing::Return;
using testing::ReturnPointee;
using testing::SetArgumentPointee;
using testing::StrictMock;

namespace {

const trunks::TPM_HANDLE kArbitraryObjectHandle = trunks::TRANSIENT_FIRST + 25;
const trunks::TPM_HANDLE kArbitrarySessionHandle = trunks::HMAC_SESSION_FIRST;

void Assign(std::string* to, const std::string& from) {
  *to = from;
}

class ScopedDisableLogging {
 public:
  ScopedDisableLogging() : original_severity_(logging::GetMinLogLevel()) {
    logging::SetMinLogLevel(logging::LOG_FATAL);
  }
  ~ScopedDisableLogging() {
    logging::SetMinLogLevel(original_severity_);
  }

 private:
  logging::LogSeverity original_severity_;
};

}  // namespace

namespace trunks {

class ResourceManagerTest : public testing::Test {
 public:
  const std::vector<TPM_HANDLE> kNoHandles;
  const std::string kNoAuthorization;
  const std::string kNoParameters;

  ResourceManagerTest() : resource_manager_(factory_, &transceiver_) {}
  ~ResourceManagerTest() override {}

  void SetUp() override {
    factory_.set_tpm(&tpm_);
  }

  // Builds a well-formed command.
  std::string CreateCommand(TPM_CC code,
                            const std::vector<TPM_HANDLE>& handles,
                            const std::string& authorization,
                            const std::string& parameters) {
    std::string buffer;
    TPM_ST tag = authorization.empty() ? TPM_ST_NO_SESSIONS : TPM_ST_SESSIONS;
    UINT32 size = 10 + (handles.size() * 4) + authorization.size() +
                  parameters.size() + (authorization.empty() ? 0 : 4);
    Serialize_TPM_ST(tag, &buffer);
    Serialize_UINT32(size, &buffer);
    Serialize_TPM_CC(code, &buffer);
    for (auto handle : handles) {
      Serialize_TPM_HANDLE(handle, &buffer);
    }
    if (!authorization.empty()) {
      Serialize_UINT32(authorization.size(), &buffer);
    }
    return buffer + authorization + parameters;
  }

  // Builds a well-formed response.
  std::string CreateResponse(TPM_RC code,
                             const std::vector<TPM_HANDLE>& handles,
                             const std::string& authorization,
                             const std::string& parameters) {
    std::string buffer;
    TPM_ST tag = authorization.empty() ? TPM_ST_NO_SESSIONS : TPM_ST_SESSIONS;
    UINT32 size = 10 + (handles.size() * 4) + authorization.size() +
                  parameters.size() + (authorization.empty() ? 0 : 4);
    Serialize_TPM_ST(tag, &buffer);
    Serialize_UINT32(size, &buffer);
    Serialize_TPM_RC(code, &buffer);
    for (auto handle : handles) {
      Serialize_TPM_HANDLE(handle, &buffer);
    }
    if (!authorization.empty()) {
      Serialize_UINT32(parameters.size(), &buffer);
    }
    return buffer + parameters + authorization;
  }

  // Builds a well-formed command authorization section.
  std::string CreateCommandAuthorization(TPM_HANDLE handle,
                                         bool continue_session) {
    std::string buffer;
    Serialize_TPM_HANDLE(handle, &buffer);
    Serialize_TPM2B_NONCE(Make_TPM2B_DIGEST(std::string(32, 'A')), &buffer);
    Serialize_BYTE(continue_session ? 1 : 0, &buffer);
    Serialize_TPM2B_DIGEST(Make_TPM2B_DIGEST(std::string(32, 'B')), &buffer);
    return buffer;
  }

  // Builds a well-formed response authorization section.
  std::string CreateResponseAuthorization(bool continue_session) {
    std::string buffer;
    Serialize_TPM2B_NONCE(Make_TPM2B_DIGEST(std::string(32, 'A')), &buffer);
    Serialize_BYTE(continue_session ? 1 : 0, &buffer);
    Serialize_TPM2B_DIGEST(Make_TPM2B_DIGEST(std::string(32, 'B')), &buffer);
    return buffer;
  }

  std::string GetHeader(const std::string& message) {
    return message.substr(0, 10);
  }

  std::string StripHeader(const std::string& message) {
    return message.substr(10);
  }

  // Makes the resource manager aware of a transient object handle and returns
  // the newly associated virtual handle.
  TPM_HANDLE LoadHandle(TPM_HANDLE handle) {
    std::vector<TPM_HANDLE> input_handles = {PERSISTENT_FIRST};
    std::string command = CreateCommand(TPM_CC_Load,
                                        input_handles,
                                        kNoAuthorization,
                                        kNoParameters);
    std::vector<TPM_HANDLE> output_handles = {handle};
    std::string response = CreateResponse(TPM_RC_SUCCESS,
                                          output_handles,
                                          kNoAuthorization,
                                          kNoParameters);
    EXPECT_CALL(transceiver_, SendCommandAndWait(command))
        .WillOnce(Return(response));
    std::string actual_response = resource_manager_.SendCommandAndWait(command);
    std::string handle_blob = StripHeader(actual_response);
    TPM_HANDLE virtual_handle;
    CHECK_EQ(TPM_RC_SUCCESS, Parse_TPM_HANDLE(&handle_blob, &virtual_handle,
                                              NULL));
    return virtual_handle;
  }

  // Causes the resource manager to evict existing object handles.
  void EvictObjects() {
    std::string command = CreateCommand(TPM_CC_Startup,
                                        kNoHandles,
                                        kNoAuthorization,
                                        kNoParameters);
    std::string response = CreateErrorResponse(TPM_RC_OBJECT_MEMORY);
    std::string success_response = CreateResponse(TPM_RC_SUCCESS,
                                                  kNoHandles,
                                                  kNoAuthorization,
                                                  kNoParameters);
    EXPECT_CALL(transceiver_, SendCommandAndWait(_))
        .WillOnce(Return(response))
        .WillRepeatedly(Return(success_response));
    EXPECT_CALL(tpm_, ContextSaveSync(_, _, _, _))
        .WillRepeatedly(Return(TPM_RC_SUCCESS));
    EXPECT_CALL(tpm_, FlushContextSync(_, _))
        .WillRepeatedly(Return(TPM_RC_SUCCESS));
    resource_manager_.SendCommandAndWait(command);
  }

  // Makes the resource manager aware of a session handle.
  void StartSession(TPM_HANDLE handle) {
    std::vector<TPM_HANDLE> input_handles = {1, 2};
    std::string command = CreateCommand(TPM_CC_StartAuthSession,
                                        input_handles,
                                        kNoAuthorization,
                                        kNoParameters);
    std::vector<TPM_HANDLE> output_handles = {handle};
    std::string response = CreateResponse(TPM_RC_SUCCESS,
                                          output_handles,
                                          kNoAuthorization,
                                          kNoParameters);
    EXPECT_CALL(transceiver_, SendCommandAndWait(command))
        .WillOnce(Return(response));
    std::string actual_response = resource_manager_.SendCommandAndWait(command);
    ASSERT_EQ(response, actual_response);
  }

  // Causes the resource manager to evict an existing session handle.
  void EvictSession() {
    std::string command = CreateCommand(TPM_CC_Startup,
                                        kNoHandles,
                                        kNoAuthorization,
                                        kNoParameters);
    std::string response = CreateErrorResponse(TPM_RC_SESSION_MEMORY);
    std::string success_response = CreateResponse(TPM_RC_SUCCESS,
                                                  kNoHandles,
                                                  kNoAuthorization,
                                                  kNoParameters);
    EXPECT_CALL(transceiver_, SendCommandAndWait(_))
        .WillOnce(Return(response))
        .WillRepeatedly(Return(success_response));
    EXPECT_CALL(tpm_, ContextSaveSync(_, _, _, _))
        .WillOnce(Return(TPM_RC_SUCCESS));
    resource_manager_.SendCommandAndWait(command);
  }

  // Creates a TPMS_CONTEXT with the given sequence field.
  TPMS_CONTEXT CreateContext(UINT64 sequence) {
    TPMS_CONTEXT context;
    memset(&context, 0, sizeof(context));
    context.sequence = sequence;
    return context;
  }

  // Creates a serialized TPMS_CONTEXT with the given sequence field.
  std::string CreateContextParameter(UINT64 sequence) {
    std::string buffer;
    Serialize_TPMS_CONTEXT(CreateContext(sequence), &buffer);
    return buffer;
  }

 protected:
  StrictMock<MockTpm> tpm_;
  TrunksFactoryForTest factory_;
  StrictMock<MockCommandTransceiver> transceiver_;
  ResourceManager resource_manager_;
};

TEST_F(ResourceManagerTest, BasicPassThrough) {
  std::string command = CreateCommand(TPM_CC_Startup,
                                      kNoHandles,
                                      kNoAuthorization,
                                      kNoParameters);
  std::string response = CreateResponse(TPM_RC_SUCCESS,
                                        kNoHandles,
                                        kNoAuthorization,
                                        kNoParameters);
  EXPECT_CALL(transceiver_, SendCommandAndWait(command))
      .WillOnce(Return(response));
  std::string actual_response = resource_manager_.SendCommandAndWait(command);
  EXPECT_EQ(actual_response, response);
}

TEST_F(ResourceManagerTest, BasicPassThroughAsync) {
  std::string command = CreateCommand(TPM_CC_Startup,
                                      kNoHandles,
                                      kNoAuthorization,
                                      kNoParameters);
  std::string response = CreateResponse(TPM_RC_SUCCESS,
                                        kNoHandles,
                                        kNoAuthorization,
                                        kNoParameters);
  EXPECT_CALL(transceiver_, SendCommandAndWait(command))
      .WillOnce(Return(response));
  std::string actual_response;
  CommandTransceiver::ResponseCallback callback =
      base::Bind(&Assign, &actual_response);
  resource_manager_.SendCommand(command, callback);
  EXPECT_EQ(actual_response, response);
}

TEST_F(ResourceManagerTest, VirtualHandleOutput) {
  std::vector<TPM_HANDLE> input_handles = {PERSISTENT_FIRST};
  std::string command = CreateCommand(TPM_CC_Load,
                                      input_handles,
                                      kNoAuthorization,
                                      kNoParameters);
  std::vector<TPM_HANDLE> output_handles = {kArbitraryObjectHandle};
  std::string response = CreateResponse(TPM_RC_SUCCESS,
                                        output_handles,
                                        kNoAuthorization,
                                        kNoParameters);
  EXPECT_CALL(transceiver_, SendCommandAndWait(command))
      .WillOnce(Return(response));
  std::string actual_response = resource_manager_.SendCommandAndWait(command);
  EXPECT_EQ(response.size(), actual_response.size());
  // We expect the resource manager has replaced the output handle with a
  // virtual handle (which we can't predict, but it's unlikely to be the same as
  // the handle emitted by the mock).
  EXPECT_EQ(GetHeader(response), GetHeader(actual_response));
  EXPECT_NE(StripHeader(response), StripHeader(actual_response));
  TPM_HT handle_type = static_cast<TPM_HT>(StripHeader(actual_response)[0]);
  EXPECT_EQ(TPM_HT_TRANSIENT, handle_type);
}

TEST_F(ResourceManagerTest, VirtualHandleInput) {
  TPM_HANDLE tpm_handle = kArbitraryObjectHandle;
  TPM_HANDLE virtual_handle = LoadHandle(tpm_handle);
  std::vector<TPM_HANDLE> input_handles = {virtual_handle};
  std::string command = CreateCommand(TPM_CC_Sign,
                                      input_handles,
                                      kNoAuthorization,
                                      kNoParameters);
  // We expect the resource manager to replace |virtual_handle| with
  // |tpm_handle|.
  std::vector<TPM_HANDLE> expected_input_handles = {tpm_handle};
  std::string expected_command = CreateCommand(TPM_CC_Sign,
                                               expected_input_handles,
                                               kNoAuthorization,
                                               kNoParameters);
  std::string response = CreateResponse(TPM_RC_SUCCESS,
                                        kNoHandles,
                                        kNoAuthorization,
                                        kNoParameters);
  EXPECT_CALL(transceiver_, SendCommandAndWait(expected_command))
      .WillOnce(Return(response));
  std::string actual_response = resource_manager_.SendCommandAndWait(command);
  EXPECT_EQ(response, actual_response);
}

TEST_F(ResourceManagerTest, VirtualHandleCleanup) {
  TPM_HANDLE tpm_handle = kArbitraryObjectHandle;
  TPM_HANDLE virtual_handle = LoadHandle(tpm_handle);
  std::string parameters;
  Serialize_TPM_HANDLE(virtual_handle, &parameters);
  std::string command = CreateCommand(TPM_CC_FlushContext,
                                      kNoHandles,
                                      kNoAuthorization,
                                      parameters);
  std::string expected_parameters;
  Serialize_TPM_HANDLE(tpm_handle, &expected_parameters);
  std::string expected_command = CreateCommand(TPM_CC_FlushContext,
                                               kNoHandles,
                                               kNoAuthorization,
                                               expected_parameters);
  std::string response = CreateResponse(TPM_RC_SUCCESS,
                                        kNoHandles,
                                        kNoAuthorization,
                                        kNoParameters);
  EXPECT_CALL(transceiver_, SendCommandAndWait(expected_command))
      .WillOnce(Return(response));
  std::string actual_response = resource_manager_.SendCommandAndWait(command);
  EXPECT_EQ(response, actual_response);
  // Now we expect there to be no record of |virtual_handle|.
  std::vector<TPM_HANDLE> input_handles = {virtual_handle};
  command = CreateCommand(TPM_CC_Sign,
                          input_handles,
                          kNoAuthorization,
                          kNoParameters);
  response = CreateErrorResponse(TPM_RC_HANDLE | kResourceManagerTpmErrorBase);
  actual_response = resource_manager_.SendCommandAndWait(command);
  EXPECT_EQ(response, actual_response);

  // Try again but attempt to flush |tpm_handle| instead of |virtual_handle|.
  virtual_handle = LoadHandle(tpm_handle);
  parameters.clear();
  Serialize_TPM_HANDLE(tpm_handle, &parameters);
  command = CreateCommand(TPM_CC_FlushContext,
                          kNoHandles,
                          kNoAuthorization,
                          parameters);
  actual_response = resource_manager_.SendCommandAndWait(command);
  // TPM_RC_HANDLE also expected here.
  EXPECT_EQ(response, actual_response);
}

TEST_F(ResourceManagerTest, VirtualHandleLoadBeforeUse) {
  TPM_HANDLE tpm_handle = kArbitraryObjectHandle;
  TPM_HANDLE virtual_handle = LoadHandle(tpm_handle);
  EvictObjects();
  std::vector<TPM_HANDLE> input_handles = {virtual_handle};
  std::string command = CreateCommand(TPM_CC_Sign,
                                      input_handles,
                                      kNoAuthorization,
                                      kNoParameters);
  std::vector<TPM_HANDLE> expected_input_handles = {tpm_handle};
  std::string expected_command = CreateCommand(TPM_CC_Sign,
                                               expected_input_handles,
                                               kNoAuthorization,
                                               kNoParameters);
  std::string response = CreateResponse(TPM_RC_SUCCESS,
                                        kNoHandles,
                                        kNoAuthorization,
                                        kNoParameters);
  EXPECT_CALL(tpm_, ContextLoadSync(_, _, _))
      .WillOnce(Return(TPM_RC_SUCCESS));
  EXPECT_CALL(transceiver_, SendCommandAndWait(expected_command))
      .WillOnce(Return(response));
  std::string actual_response = resource_manager_.SendCommandAndWait(command);
  EXPECT_EQ(response, actual_response);
}

TEST_F(ResourceManagerTest, InvalidVirtualHandle) {
  std::vector<TPM_HANDLE> input_handles = {kArbitraryObjectHandle};
  std::string command = CreateCommand(TPM_CC_Sign,
                                      input_handles,
                                      kNoAuthorization,
                                      kNoParameters);
  std::string response = CreateErrorResponse(TPM_RC_HANDLE |
                                             kResourceManagerTpmErrorBase);
  std::string actual_response = resource_manager_.SendCommandAndWait(command);
  EXPECT_EQ(response, actual_response);
}

TEST_F(ResourceManagerTest, SimpleFuzzInputParser) {
  std::vector<TPM_HANDLE> handles = {1, 2};
  std::string parameters = "12345";
  std::string command = CreateCommand(TPM_CC_StartAuthSession,
                                      handles,
                                      CreateCommandAuthorization(
                                          kArbitrarySessionHandle,
                                          true),  // continue_session
                                      parameters);
  // We don't care about what happens, only that it doesn't crash.
  EXPECT_CALL(transceiver_, SendCommandAndWait(_))
      .WillRepeatedly(Return(CreateErrorResponse(TPM_RC_FAILURE)));
  ScopedDisableLogging no_logging;
  for (size_t i = 0; i < command.size(); ++i) {
    resource_manager_.SendCommandAndWait(command.substr(0, i));
    resource_manager_.SendCommandAndWait(command.substr(i));
    std::string fuzzed_command(command);
    for (uint8_t value = 0; ; value++) {
      fuzzed_command[i] = static_cast<char>(value);
      resource_manager_.SendCommandAndWait(fuzzed_command);
      if (value == 255) {
        break;
      }
    }
  }
}

TEST_F(ResourceManagerTest, SimpleFuzzOutputParser) {
  std::vector<TPM_HANDLE> handles = {1, 2};
  std::string parameters = "12345";
  std::string command = CreateCommand(TPM_CC_StartAuthSession,
                                      handles,
                                      CreateCommandAuthorization(
                                          kArbitrarySessionHandle,
                                          true),  // continue_session
                                      parameters);
  std::vector<TPM_HANDLE> out_handles = {3};
  std::string response = CreateResponse(TPM_RC_SUCCESS,
                                        out_handles,
                                        CreateResponseAuthorization(
                                            true),  // continue_session
                                        parameters);
  std::string fuzzed_response;
  EXPECT_CALL(transceiver_, SendCommandAndWait(_))
      .WillRepeatedly(ReturnPointee(&fuzzed_response));
  ScopedDisableLogging no_logging;
  for (size_t i = 0; i < response.size(); ++i) {
    fuzzed_response = response.substr(0, i);
    resource_manager_.SendCommandAndWait(command);
    fuzzed_response = response.substr(i);
    resource_manager_.SendCommandAndWait(command);
    fuzzed_response = response;
    for (uint8_t value = 0; ; value++) {
      fuzzed_response[i] = static_cast<char>(value);
      resource_manager_.SendCommandAndWait(command);
      if (value == 255) {
        break;
      }
    }
    fuzzed_response[i] = response[i];
  }
}

TEST_F(ResourceManagerTest, NewSession) {
  StartSession(kArbitrarySessionHandle);
  std::string command = CreateCommand(TPM_CC_Startup,
                                      kNoHandles,
                                      CreateCommandAuthorization(
                                          kArbitrarySessionHandle,
                                          true),  // continue_session
                                      kNoParameters);
  std::string response = CreateResponse(TPM_RC_SUCCESS,
                                        kNoHandles,
                                        CreateResponseAuthorization(
                                            true),  // continue_session
                                        kNoParameters);
  EXPECT_CALL(transceiver_, SendCommandAndWait(command))
      .WillOnce(Return(response));
  std::string actual_response = resource_manager_.SendCommandAndWait(command);
  EXPECT_EQ(response, actual_response);
}

TEST_F(ResourceManagerTest, DiscontinuedSession) {
  StartSession(kArbitrarySessionHandle);
  // Use the session but do not continue.
  std::string command = CreateCommand(TPM_CC_Startup,
                                      kNoHandles,
                                      CreateCommandAuthorization(
                                          kArbitrarySessionHandle,
                                          false),  // continue_session
                                      kNoParameters);
  std::string response = CreateResponse(TPM_RC_SUCCESS,
                                        kNoHandles,
                                        CreateResponseAuthorization(
                                            false),  // continue_session
                                        kNoParameters);
  EXPECT_CALL(transceiver_, SendCommandAndWait(command))
      .WillOnce(Return(response));
  std::string actual_response = resource_manager_.SendCommandAndWait(command);
  EXPECT_EQ(response, actual_response);
  // Now attempt to use it again and expect a handle error.
  response = CreateErrorResponse(TPM_RC_HANDLE | kResourceManagerTpmErrorBase);
  actual_response = resource_manager_.SendCommandAndWait(command);
  EXPECT_EQ(response, actual_response);
}

TEST_F(ResourceManagerTest, LoadSessionBeforeUse) {
  StartSession(kArbitrarySessionHandle);
  EvictSession();
  std::string command = CreateCommand(TPM_CC_Startup,
                                      kNoHandles,
                                      CreateCommandAuthorization(
                                          kArbitrarySessionHandle,
                                          true),  // continue_session
                                      kNoParameters);
  std::string response = CreateResponse(TPM_RC_SUCCESS,
                                        kNoHandles,
                                        CreateResponseAuthorization(
                                            true),  // continue_session
                                        kNoParameters);
  EXPECT_CALL(transceiver_, SendCommandAndWait(command))
      .WillOnce(Return(response));
  EXPECT_CALL(tpm_, ContextLoadSync(_, _, _))
      .WillOnce(Return(TPM_RC_SUCCESS));
  std::string actual_response = resource_manager_.SendCommandAndWait(command);
  EXPECT_EQ(response, actual_response);
}

TEST_F(ResourceManagerTest, SessionHandleCleanup) {
  StartSession(kArbitrarySessionHandle);
  std::string parameters;
  Serialize_TPM_HANDLE(kArbitrarySessionHandle, &parameters);
  std::string command = CreateCommand(TPM_CC_FlushContext,
                                      kNoHandles,
                                      kNoAuthorization,
                                      parameters);
  std::string response = CreateResponse(TPM_RC_SUCCESS,
                                        kNoHandles,
                                        kNoAuthorization,
                                        kNoParameters);
  EXPECT_CALL(transceiver_, SendCommandAndWait(command))
      .WillOnce(Return(response));
  std::string actual_response = resource_manager_.SendCommandAndWait(command);
  EXPECT_EQ(response, actual_response);
  // Now we expect there to be no record of |kArbitrarySessionHandle|.
  command = CreateCommand(TPM_CC_Startup,
                          kNoHandles,
                          CreateCommandAuthorization(
                              kArbitrarySessionHandle,
                              true),  // continue_session
                          kNoParameters);
  response = CreateErrorResponse(TPM_RC_HANDLE | kResourceManagerTpmErrorBase);
  actual_response = resource_manager_.SendCommandAndWait(command);
  EXPECT_EQ(response, actual_response);
}

TEST_F(ResourceManagerTest, EvictWhenObjectInUse) {
  TPM_HANDLE tpm_handle = kArbitraryObjectHandle;
  TPM_HANDLE virtual_handle = LoadHandle(tpm_handle);
  TPM_HANDLE tpm_handle2 = kArbitraryObjectHandle + 1;
  LoadHandle(tpm_handle2);
  std::vector<TPM_HANDLE> input_handles = {virtual_handle};
  std::string command = CreateCommand(TPM_CC_Sign,
                                      input_handles,
                                      kNoAuthorization,
                                      kNoParameters);
  // Trigger evict logic and verify |input_handles| are not evicted.
  std::string response = CreateErrorResponse(TPM_RC_OBJECT_MEMORY);
  std::string success_response = CreateResponse(TPM_RC_SUCCESS,
                                                kNoHandles,
                                                kNoAuthorization,
                                                kNoParameters);
  EXPECT_CALL(tpm_, ContextSaveSync(tpm_handle2, _, _, _))
      .WillOnce(Return(TPM_RC_SUCCESS));
  EXPECT_CALL(tpm_, FlushContextSync(tpm_handle2, _))
      .WillOnce(Return(TPM_RC_SUCCESS));
  EXPECT_CALL(transceiver_, SendCommandAndWait(_))
      .WillOnce(Return(response))
      .WillRepeatedly(Return(success_response));
  std::string actual_response = resource_manager_.SendCommandAndWait(command);
  EXPECT_EQ(success_response, actual_response);
}

TEST_F(ResourceManagerTest, EvictWhenSessionInUse) {
  StartSession(kArbitrarySessionHandle);
  StartSession(kArbitrarySessionHandle + 1);
  std::string command = CreateCommand(TPM_CC_Startup,
                                      kNoHandles,
                                      CreateCommandAuthorization(
                                          kArbitrarySessionHandle,
                                          true),  // continue_session
                                      kNoParameters);
  std::string response = CreateResponse(TPM_RC_SUCCESS,
                                        kNoHandles,
                                        CreateResponseAuthorization(
                                            true),  // continue_session
                                        kNoParameters);
  std::string error_response = CreateErrorResponse(TPM_RC_SESSION_MEMORY);
  EXPECT_CALL(transceiver_, SendCommandAndWait(_))
      .WillOnce(Return(error_response))
      .WillRepeatedly(Return(response));
  EXPECT_CALL(tpm_, ContextSaveSync(kArbitrarySessionHandle + 1, _, _, _))
      .WillOnce(Return(TPM_RC_SUCCESS));
  std::string actual_response = resource_manager_.SendCommandAndWait(command);
  EXPECT_EQ(response, actual_response);
}

TEST_F(ResourceManagerTest, EvictMultipleObjects) {
  const int kNumObjects = 10;
  std::map<TPM_HANDLE, TPM_HANDLE> handles;
  for (int i = 0; i < kNumObjects; ++i) {
    TPM_HANDLE handle = kArbitraryObjectHandle + i;
    handles[LoadHandle(handle)] = handle;
  }
  EvictObjects();
  std::string response = CreateResponse(TPM_RC_SUCCESS,
                                        kNoHandles,
                                        kNoAuthorization,
                                        kNoParameters);
  EXPECT_CALL(tpm_, ContextLoadSync(_, _, _))
      .Times(kNumObjects)
      .WillRepeatedly(Return(TPM_RC_SUCCESS));
  EXPECT_CALL(transceiver_, SendCommandAndWait(_))
      .WillRepeatedly(Return(response));
  for (auto item : handles) {
    std::vector<TPM_HANDLE> input_handles = {item.first};
    std::string command = CreateCommand(TPM_CC_Sign,
                                        input_handles,
                                        kNoAuthorization,
                                        kNoParameters);
    std::string actual_response = resource_manager_.SendCommandAndWait(command);
    EXPECT_EQ(response, actual_response);
  }
}

TEST_F(ResourceManagerTest, EvictMostStaleSession) {
  StartSession(kArbitrarySessionHandle);
  StartSession(kArbitrarySessionHandle + 1);
  StartSession(kArbitrarySessionHandle + 2);
  std::string response = CreateResponse(TPM_RC_SUCCESS,
                                        kNoHandles,
                                        CreateResponseAuthorization(
                                            true),  // continue_session
                                        kNoParameters);
  EXPECT_CALL(transceiver_, SendCommandAndWait(_))
      .WillRepeatedly(Return(response));
  // Use the first two sessions, leaving the third as the most stale.
  for (int i = 0; i < 2; ++i) {
    std::string command = CreateCommand(TPM_CC_Startup,
                                        kNoHandles,
                                        CreateCommandAuthorization(
                                            kArbitrarySessionHandle + i,
                                            true),  // continue_session
                                        kNoParameters);
    std::string actual_response = resource_manager_.SendCommandAndWait(command);
    EXPECT_EQ(response, actual_response);
  }
  EvictSession();
  // EvictSession will have messed with the expectations; set them again.
  EXPECT_CALL(transceiver_, SendCommandAndWait(_))
      .WillRepeatedly(Return(response));
  // Use the first two sessions again, expecting no calls to ContextLoad.
  for (int i = 0; i < 2; ++i) {
    std::string command = CreateCommand(TPM_CC_Startup,
                                        kNoHandles,
                                        CreateCommandAuthorization(
                                            kArbitrarySessionHandle + i,
                                            true),  // continue_session
                                        kNoParameters);
    std::string actual_response = resource_manager_.SendCommandAndWait(command);
    EXPECT_EQ(response, actual_response);
  }
  // Expect a call to ContextLoad if we use the third session.
  std::string command = CreateCommand(TPM_CC_Startup,
                                      kNoHandles,
                                      CreateCommandAuthorization(
                                          kArbitrarySessionHandle + 2,
                                          true),  // continue_session
                                      kNoParameters);
  EXPECT_CALL(tpm_, ContextLoadSync(_, _, _))
      .WillOnce(Return(TPM_RC_SUCCESS));
  std::string actual_response = resource_manager_.SendCommandAndWait(command);
  EXPECT_EQ(response, actual_response);
}

TEST_F(ResourceManagerTest, HandleContextGap) {
  const int kNumSessions = 7;
  const int kNumSessionsToUngap = 4;
  std::vector<TPM_HANDLE> expected_ungap_order;
  for (int i = 0; i < kNumSessions; ++i) {
    StartSession(kArbitrarySessionHandle + i);
    if (i < kNumSessionsToUngap) {
      EvictSession();
      expected_ungap_order.push_back(kArbitrarySessionHandle + i);
    }
  }
  // Invoke a context gap.
  std::string command = CreateCommand(TPM_CC_Startup,
                                      kNoHandles,
                                      kNoAuthorization,
                                      kNoParameters);
  std::string response = CreateErrorResponse(TPM_RC_CONTEXT_GAP);
  std::string success_response = CreateResponse(TPM_RC_SUCCESS,
                                                kNoHandles,
                                                kNoAuthorization,
                                                kNoParameters);
  {
    InSequence ungap_order;
    for (auto handle : expected_ungap_order) {
      EXPECT_CALL(tpm_, ContextLoadSync(_, _, _))
          .WillOnce(Return(TPM_RC_SUCCESS));
      EXPECT_CALL(tpm_, ContextSaveSync(handle, _, _, _))
          .WillOnce(Return(TPM_RC_SUCCESS));
    }
  }
  EXPECT_CALL(transceiver_, SendCommandAndWait(_))
      .WillOnce(Return(response))
      .WillRepeatedly(Return(success_response));
  std::string actual_response = resource_manager_.SendCommandAndWait(command);
  EXPECT_EQ(success_response, actual_response);
}

TEST_F(ResourceManagerTest, ExternalContext) {
  StartSession(kArbitrarySessionHandle);
  // Do an external context save.
  std::vector<TPM_HANDLE> handles = {kArbitrarySessionHandle};
  std::string context_save = CreateCommand(TPM_CC_ContextSave,
                                           handles,
                                           kNoAuthorization,
                                           kNoParameters);
  std::string context_parameter1 = CreateContextParameter(1);
  std::string context_save_response1 = CreateResponse(TPM_RC_SUCCESS,
                                                      kNoHandles,
                                                      kNoAuthorization,
                                                      context_parameter1);
  EXPECT_CALL(transceiver_, SendCommandAndWait(context_save))
      .WillOnce(Return(context_save_response1));
  std::string actual_response = resource_manager_.SendCommandAndWait(
      context_save);
  EXPECT_EQ(context_save_response1, actual_response);

  // Invoke a context gap (which will cause context1 to be mapped to context2).
  EXPECT_CALL(tpm_, ContextLoadSync(Field(&TPMS_CONTEXT::sequence, Eq(1u)),
                                    _, _))
      .WillOnce(Return(TPM_RC_SUCCESS));
  EXPECT_CALL(tpm_, ContextSaveSync(kArbitrarySessionHandle, _, _, _))
      .WillOnce(DoAll(SetArgumentPointee<2>(CreateContext(2)),
                      Return(TPM_RC_SUCCESS)));
  std::string command = CreateCommand(TPM_CC_Startup,
                                      kNoHandles,
                                      kNoAuthorization,
                                      kNoParameters);
  std::string response = CreateErrorResponse(TPM_RC_CONTEXT_GAP);
  std::string success_response = CreateResponse(TPM_RC_SUCCESS,
                                                kNoHandles,
                                                kNoAuthorization,
                                                kNoParameters);
  EXPECT_CALL(transceiver_, SendCommandAndWait(command))
      .WillOnce(Return(response))
      .WillOnce(Return(success_response));
  actual_response = resource_manager_.SendCommandAndWait(command);
  EXPECT_EQ(success_response, actual_response);

  // Now load external context1 and expect an actual load of context2.
  std::string context_load1 = CreateCommand(TPM_CC_ContextLoad,
                                            kNoHandles,
                                            kNoAuthorization,
                                            context_parameter1);
  std::string context_load2 = CreateCommand(TPM_CC_ContextLoad,
                                            kNoHandles,
                                            kNoAuthorization,
                                            CreateContextParameter(2));
  std::string context_load_response = CreateResponse(TPM_RC_SUCCESS,
                                                     handles,
                                                     kNoAuthorization,
                                                     kNoParameters);
  EXPECT_CALL(transceiver_, SendCommandAndWait(context_load2))
      .WillOnce(Return(context_load_response));
  actual_response = resource_manager_.SendCommandAndWait(context_load1);
  EXPECT_EQ(context_load_response, actual_response);
}

TEST_F(ResourceManagerTest, NestedFailures) {
  // The scenario being tested is when a command results in a warning to be
  // handled by the resource manager, and in the process of handling the first
  // warning another warning occurs which should be handled by the resource
  // manager, etc..
  for (int i = 0; i < 3; ++i) {
    LoadHandle(kArbitraryObjectHandle + i);
  }
  EvictObjects();
  for (int i = 3; i < 6; ++i) {
    LoadHandle(kArbitraryObjectHandle + i);
  }
  for (int i = 0; i < 10; ++i) {
    StartSession(kArbitrarySessionHandle + i);
    EvictSession();
  }
  for (int i = 10; i < 20; ++i) {
    StartSession(kArbitrarySessionHandle + i);
  }
  std::string error_response = CreateErrorResponse(TPM_RC_MEMORY);
  EXPECT_CALL(transceiver_, SendCommandAndWait(_))
      .WillRepeatedly(Return(error_response));
  // The TPM_RC_MEMORY will result in a context save, make that fail too.
  EXPECT_CALL(tpm_, ContextSaveSync(_, _, _, _))
      .WillRepeatedly(Return(TPM_RC_CONTEXT_GAP));
  // The TPM_RC_CONTEXT_GAP will result in a context load.
  EXPECT_CALL(tpm_, ContextLoadSync(_, _, _))
      .WillRepeatedly(Return(TPM_RC_SESSION_HANDLES));
  // The TPM_RC_SESSION_HANDLES will result in a context flush.
  EXPECT_CALL(tpm_, FlushContextSync(_, _))
      .WillRepeatedly(Return(TPM_RC_SESSION_MEMORY));
  // The resource manager should not handle the same warning twice so we expect
  // the error of the original call to bubble up.
  std::string command = CreateCommand(TPM_CC_Startup,
                                      kNoHandles,
                                      kNoAuthorization,
                                      kNoParameters);
  std::string response = resource_manager_.SendCommandAndWait(command);
  EXPECT_EQ(error_response, response);
}

TEST_F(ResourceManagerTest, OutOfMemory) {
  std::string error_response = CreateErrorResponse(TPM_RC_MEMORY);
  EXPECT_CALL(transceiver_, SendCommandAndWait(_))
      .WillRepeatedly(Return(error_response));
  std::string command = CreateCommand(TPM_CC_Startup,
                                      kNoHandles,
                                      kNoAuthorization,
                                      kNoParameters);
  std::string response = resource_manager_.SendCommandAndWait(command);
  EXPECT_EQ(error_response, response);
}

TEST_F(ResourceManagerTest, ReentrantFixGap) {
  for (int i = 0; i < 3; ++i) {
    StartSession(kArbitrarySessionHandle + i);
    EvictSession();
  }
  for (int i = 3; i < 6; ++i) {
    StartSession(kArbitrarySessionHandle + i);
  }
  std::string error_response = CreateErrorResponse(TPM_RC_CONTEXT_GAP);
  EXPECT_CALL(transceiver_, SendCommandAndWait(_))
      .WillRepeatedly(Return(error_response));
  EXPECT_CALL(tpm_, ContextSaveSync(_, _, _, _))
      .WillRepeatedly(Return(TPM_RC_CONTEXT_GAP));
  EXPECT_CALL(tpm_, ContextLoadSync(_, _, _))
      .WillOnce(Return(TPM_RC_CONTEXT_GAP))
      .WillRepeatedly(Return(TPM_RC_SUCCESS));
  std::string command = CreateCommand(TPM_CC_Startup,
                                      kNoHandles,
                                      kNoAuthorization,
                                      kNoParameters);
  std::string response = resource_manager_.SendCommandAndWait(command);
  EXPECT_EQ(error_response, response);
}

TEST_F(ResourceManagerTest, PasswordAuthorization) {
  std::string command = CreateCommand(TPM_CC_Startup,
                                      kNoHandles,
                                      CreateCommandAuthorization(
                                          TPM_RS_PW,
                                          false),  // continue_session
                                      kNoParameters);
  std::string response = CreateResponse(TPM_RC_SUCCESS,
                                        kNoHandles,
                                        CreateResponseAuthorization(
                                            false),  // continue_session
                                        kNoParameters);
  EXPECT_CALL(transceiver_, SendCommandAndWait(command))
      .WillOnce(Return(response));
  std::string actual_response = resource_manager_.SendCommandAndWait(command);
  EXPECT_EQ(response, actual_response);
}

}  // namespace trunks
