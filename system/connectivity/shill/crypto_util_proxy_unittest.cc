//
// Copyright (C) 2013 The Android Open Source Project
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

#include "shill/crypto_util_proxy.h"

#include <algorithm>
#include <string>
#include <vector>

#include <base/callback.h>
#include <gtest/gtest.h>

#include "shill/callbacks.h"
#include "shill/mock_crypto_util_proxy.h"
#include "shill/mock_event_dispatcher.h"
#include "shill/mock_file_io.h"
#include "shill/mock_process_manager.h"

using base::Bind;
using std::min;
using std::string;
using std::vector;
using testing::AnyOf;
using testing::DoAll;
using testing::InSequence;
using testing::Invoke;
using testing::Mock;
using testing::NotNull;
using testing::Return;
using testing::StrEq;
using testing::WithoutArgs;
using testing::_;

namespace shill {

namespace {

const char kTestBSSID[] = "00:11:22:33:44:55";
const char kTestCertificate[] = "testcertgoeshere";
const char kTestData[] = "thisisthetestdata";
const char kTestDestinationUDN[] = "TEST1234-5678-ABCD";
const char kTestNonce[] = "abort abort abort";
const char kTestPublicKey[] = "YWJvcnQgYWJvcnQgYWJvcnQK";
const char kTestSerializedCommandMessage[] =
    "Since we're not testing protocol buffer seriallization, and no data "
    "actually makes it to a shim, we're safe to write whatever we want here.";
const char kTestSerializedCommandResponse[] =
    "Similarly, we never ask a protocol buffer to deserialize this string.";
const char kTestSignedData[] = "Ynl0ZXMgYnl0ZXMgYnl0ZXMK";
const int kTestStdinFd = 9111;
const int kTestStdoutFd = 9119;
const pid_t kTestShimPid = 989898;

}  // namespace

MATCHER_P(ErrorIsOfType, error_type, "") {
  if (error_type != arg.type()) {
    return false;
  }

  return true;
}

class CryptoUtilProxyTest : public testing::Test {
 public:
  CryptoUtilProxyTest()
      : crypto_util_proxy_(&dispatcher_) {
    test_ssid_.push_back(78);
    test_ssid_.push_back(69);
    test_ssid_.push_back(80);
    test_ssid_.push_back(84);
    test_ssid_.push_back(85);
    test_ssid_.push_back(78);
    test_ssid_.push_back(69);
  }

  virtual void SetUp() {
    crypto_util_proxy_.process_manager_ = &process_manager_;
    crypto_util_proxy_.file_io_ = &file_io_;
  }

  virtual void TearDown() {
    // Note that |crypto_util_proxy_| needs its process manager reference in
    // order not to segfault when it tries to kill any outstanding shims on
    // shutdown.  Thus we don't clear out those fields here, and we make sure
    // to declare the proxy after mocks it consumes.
  }

  // TODO(quiche): Consider refactoring
  // HandleStartInMinijailWithPipes, HandleStopProcess, and
  // HandleUpdateExitCallback into a FakeProcessManager. b/24210150
  pid_t HandleStartInMinijailWithPipes(
      const tracked_objects::Location& /* spawn_source */,
      const base::FilePath& /* program */,
      vector<string> /* program_args */,
      const std::string& /* run_as_user */,
      const std::string& /* run_as_group */,
      uint64_t /* capabilities_mask */,
      const base::Callback<void(int)>& exit_callback,
      int* stdin,
      int* stdout,
      int* /* stderr */) {
    exit_callback_ = exit_callback;
    *stdin = kTestStdinFd;
    *stdout = kTestStdoutFd;
    return kTestShimPid;
  }

  void StartAndCheckShim(const std::string& command,
                         const std::string& shim_stdin) {
    InSequence seq;
    // Delegate the start call to the real implementation just for this test.
    EXPECT_CALL(crypto_util_proxy_, StartShimForCommand(_, _, _))
        .WillOnce(Invoke(&crypto_util_proxy_,
                         &MockCryptoUtilProxy::RealStartShimForCommand));
    // All shims should be spawned in a Minijail.
    EXPECT_CALL(
        process_manager_,
        StartProcessInMinijailWithPipes(
            _,  // caller location
            base::FilePath(CryptoUtilProxy::kCryptoUtilShimPath),
            AnyOf(
                vector<string>{CryptoUtilProxy::kCommandVerify},
                vector<string>{CryptoUtilProxy::kCommandEncrypt}),
            "shill-crypto",
            "shill-crypto",
            0,  // no capabilities required
            _,  // exit_callback
            NotNull(),  // stdin
            NotNull(),  // stdout
            nullptr))  // stderr
        .WillOnce(Invoke(this,
                         &CryptoUtilProxyTest::HandleStartInMinijailWithPipes));
    // We should always schedule a shim timeout callback.
    EXPECT_CALL(dispatcher_, PostDelayedTask(_, _));
    // We don't allow file I/O to block.
    EXPECT_CALL(file_io_,
                SetFdNonBlocking(kTestStdinFd))
        .WillOnce(Return(0));
    EXPECT_CALL(file_io_,
                SetFdNonBlocking(kTestStdoutFd))
        .WillOnce(Return(0));
    // We instead do file I/O through async callbacks registered with the event
    // dispatcher.
    EXPECT_CALL(dispatcher_, CreateInputHandler(_, _, _)).Times(1);
    EXPECT_CALL(dispatcher_, CreateReadyHandler(_, _, _)).Times(1);
    // The shim is left in flight, not killed.
    EXPECT_CALL(process_manager_, StopProcess(_)).Times(0);
    crypto_util_proxy_.StartShimForCommand(
        command, shim_stdin,
        Bind(&MockCryptoUtilProxy::TestResultHandlerCallback,
             crypto_util_proxy_.base::SupportsWeakPtr<MockCryptoUtilProxy>::
                AsWeakPtr()));
    EXPECT_EQ(shim_stdin, crypto_util_proxy_.input_buffer_);
    EXPECT_TRUE(crypto_util_proxy_.output_buffer_.empty());
    EXPECT_EQ(crypto_util_proxy_.shim_pid_, kTestShimPid);
    Mock::VerifyAndClearExpectations(&crypto_util_proxy_);
    Mock::VerifyAndClearExpectations(&dispatcher_);
    Mock::VerifyAndClearExpectations(&process_manager_);
  }

  void ExpectCleanup(const Error& expected_result) {
    if (crypto_util_proxy_.shim_stdin_ > -1) {
      EXPECT_CALL(file_io_,
                  Close(crypto_util_proxy_.shim_stdin_)).Times(1);
    }
    if (crypto_util_proxy_.shim_stdout_ > -1) {
      EXPECT_CALL(file_io_,
                  Close(crypto_util_proxy_.shim_stdout_)).Times(1);
    }
    if (crypto_util_proxy_.shim_pid_) {
      EXPECT_CALL(process_manager_, UpdateExitCallback(_, _))
          .Times(1)
          .WillOnce(Invoke(this,
                           &CryptoUtilProxyTest::HandleUpdateExitCallback));
      EXPECT_CALL(process_manager_, StopProcess(crypto_util_proxy_.shim_pid_))
          .Times(1)
          .WillOnce(Invoke(this,
                           &CryptoUtilProxyTest::HandleStopProcess));
    }
  }

  void AssertShimDead() {
    EXPECT_FALSE(crypto_util_proxy_.shim_pid_);
  }

  bool HandleUpdateExitCallback(pid_t /*pid*/,
                                const base::Callback<void(int)>& new_callback) {
    exit_callback_ = new_callback;
    return true;
  }

  bool HandleStopProcess(pid_t /*pid*/) {
    const int kExitStatus = -1;
    // NB: in the real world, this ordering is not guaranteed. That
    // is, StopProcess() might return before executing the callback.
    exit_callback_.Run(kExitStatus);
    return true;
  }

  void StopAndCheckShim(const Error& error) {
    ExpectCleanup(error);
    crypto_util_proxy_.CleanupShim(error);
    crypto_util_proxy_.OnShimDeath(-1);
    EXPECT_EQ(crypto_util_proxy_.shim_pid_, 0);
    Mock::VerifyAndClearExpectations(&process_manager_);
  }

 protected:
  MockProcessManager process_manager_;
  MockEventDispatcher dispatcher_;
  MockFileIO file_io_;
  MockCryptoUtilProxy crypto_util_proxy_;
  std::vector<uint8_t> test_ssid_;
  base::Callback<void(int)> exit_callback_;
};

TEST_F(CryptoUtilProxyTest, BasicAPIUsage) {
  {
    InSequence seq;
    // Delegate the API call to the real implementation for this test.
    EXPECT_CALL(crypto_util_proxy_,
                VerifyDestination(_, _, _, _, _, _, _, _, _))
        .WillOnce(Invoke(&crypto_util_proxy_,
                         &MockCryptoUtilProxy::RealVerifyDestination));
    // API calls are just thin wrappers that write up a message to a shim, then
    // send it via StartShimForCommand.  Expect that a shim will be started in
    // response to the API being called.
    EXPECT_CALL(crypto_util_proxy_,
                StartShimForCommand(CryptoUtilProxy::kCommandVerify, _, _))
        .WillOnce(Return(true));
    ResultBoolCallback result_callback =
        Bind(&MockCryptoUtilProxy::TestResultBoolCallback,
        crypto_util_proxy_.
            base::SupportsWeakPtr<MockCryptoUtilProxy>::AsWeakPtr());
    Error error;
    EXPECT_TRUE(crypto_util_proxy_.VerifyDestination(kTestCertificate,
                                                     kTestPublicKey,
                                                     kTestNonce,
                                                     kTestSignedData,
                                                     kTestDestinationUDN,
                                                     test_ssid_,
                                                     kTestBSSID,
                                                     result_callback,
                                                     &error));
    EXPECT_TRUE(error.IsSuccess());
  }
  {
    // And very similarly...
    InSequence seq;
    EXPECT_CALL(crypto_util_proxy_, EncryptData(_, _, _, _))
        .WillOnce(Invoke(&crypto_util_proxy_,
                         &MockCryptoUtilProxy::RealEncryptData));
    EXPECT_CALL(crypto_util_proxy_,
                StartShimForCommand(CryptoUtilProxy::kCommandEncrypt, _, _))
        .WillOnce(Return(true));
    ResultStringCallback result_callback =
        Bind(&MockCryptoUtilProxy::TestResultStringCallback,
        crypto_util_proxy_.
            base::SupportsWeakPtr<MockCryptoUtilProxy>::AsWeakPtr());
    Error error;
    // Normally, we couldn't have these two operations run successfully without
    // finishing the first one, since only one shim can be in flight at a time.
    // However, this works because we didn't actually start a shim, we just
    // trapped the call in our mock.
    EXPECT_TRUE(crypto_util_proxy_.EncryptData(kTestPublicKey, kTestData,
                                               result_callback, &error));
    EXPECT_TRUE(error.IsSuccess());
  }
}

TEST_F(CryptoUtilProxyTest, ShimCleanedBeforeCallback) {
  // Some operations, like VerifyAndEncryptData in the manager, chain two
  // shim operations together.  Make sure that we don't call back with results
  // before the shim state is clean.
  {
    StartAndCheckShim(CryptoUtilProxy::kCommandEncrypt,
                      kTestSerializedCommandMessage);
    Error e(Error::kOperationFailed);
    ExpectCleanup(e);
    EXPECT_CALL(crypto_util_proxy_,
                TestResultHandlerCallback(
                    StrEq(""), ErrorIsOfType(Error::kOperationFailed)))
        .Times(1)
        .WillOnce(WithoutArgs(Invoke(this,
                                     &CryptoUtilProxyTest::AssertShimDead)));
    crypto_util_proxy_.HandleShimError(e);
  }
  {
    StartAndCheckShim(CryptoUtilProxy::kCommandEncrypt,
                      kTestSerializedCommandMessage);
    EXPECT_CALL(crypto_util_proxy_,
                TestResultHandlerCallback(
                    StrEq(""), ErrorIsOfType(Error::kSuccess)))
        .Times(1)
        .WillOnce(WithoutArgs(Invoke(this,
                                     &CryptoUtilProxyTest::AssertShimDead)));
    ExpectCleanup(Error(Error::kSuccess));
    InputData data;
    data.buf = nullptr;
    data.len = 0;
    crypto_util_proxy_.HandleShimOutput(&data);
  }
}

// Verify that even when we have errors, we'll call the result handler.
// Ultimately, this is supposed to make sure that we always return something to
// our callers over DBus.
TEST_F(CryptoUtilProxyTest, FailuresReturnValues) {
  StartAndCheckShim(CryptoUtilProxy::kCommandEncrypt,
                    kTestSerializedCommandMessage);
  EXPECT_CALL(crypto_util_proxy_, TestResultHandlerCallback(
      StrEq(""), ErrorIsOfType(Error::kOperationFailed))).Times(1);
  Error e(Error::kOperationFailed);
  ExpectCleanup(e);
  crypto_util_proxy_.HandleShimError(e);
}

TEST_F(CryptoUtilProxyTest, TimeoutsTriggerFailure) {
  StartAndCheckShim(CryptoUtilProxy::kCommandEncrypt,
                    kTestSerializedCommandMessage);
  EXPECT_CALL(crypto_util_proxy_, TestResultHandlerCallback(
      StrEq(""), ErrorIsOfType(Error::kOperationTimeout))).Times(1);
  ExpectCleanup(Error(Error::kOperationTimeout));
  // This timeout is scheduled by StartShimForCommand.
  crypto_util_proxy_.HandleShimTimeout();
}

TEST_F(CryptoUtilProxyTest, OnlyOneInstanceInFlightAtATime) {
  StartAndCheckShim(CryptoUtilProxy::kCommandEncrypt,
                    kTestSerializedCommandMessage);
  // Can't start things twice.
  EXPECT_FALSE(crypto_util_proxy_.RealStartShimForCommand(
      CryptoUtilProxy::kCommandEncrypt, kTestSerializedCommandMessage,
      Bind(&MockCryptoUtilProxy::TestResultHandlerCallback,
           crypto_util_proxy_.
              base::SupportsWeakPtr<MockCryptoUtilProxy>::AsWeakPtr())));
  // But if some error (or completion) caused us to clean up the shim...
  StopAndCheckShim(Error(Error::kSuccess));
  // Then we could start the shim again.
  StartAndCheckShim(CryptoUtilProxy::kCommandEncrypt,
                    kTestSerializedCommandMessage);
  // Clean up after ourselves.
  StopAndCheckShim(Error(Error::kOperationFailed));
}

// This test walks the CryptoUtilProxy through the life time of a shim by
// simulating the API call, file I/O operations, and the final handler on shim
// completion.
TEST_F(CryptoUtilProxyTest, ShimLifeTime) {
  const int kBytesAtATime = 10;
  StartAndCheckShim(CryptoUtilProxy::kCommandEncrypt,
                    kTestSerializedCommandMessage);
  // Emulate the operating system pulling bytes through the pipe, and the event
  // loop notifying us that the file descriptor is ready.
  int bytes_left = strlen(kTestSerializedCommandMessage);
  while (bytes_left > 0) {
    int bytes_written = min(kBytesAtATime, bytes_left);
    EXPECT_CALL(file_io_, Write(kTestStdinFd, _, bytes_left))
        .Times(1).WillOnce(Return(bytes_written));
    bytes_left -= bytes_written;
    if (bytes_left < 1) {
      EXPECT_CALL(file_io_, Close(kTestStdinFd));
    }
    crypto_util_proxy_.HandleShimStdinReady(crypto_util_proxy_.shim_stdin_);
    Mock::VerifyAndClearExpectations(&crypto_util_proxy_);
  }

  // At this point, the shim goes off and does terribly complex crypto stuff,
  // before responding with a string of bytes over stdout.  Emulate the shim
  // and the event loop to push those bytes back.
  const int response_length = bytes_left =
      strlen(kTestSerializedCommandResponse);
  InputData data;
  while (bytes_left > 0) {
    int bytes_written = min(kBytesAtATime, bytes_left);
    data.len = bytes_written;
    data.buf = reinterpret_cast<unsigned char*>(const_cast<char*>(
          kTestSerializedCommandResponse + response_length - bytes_left));
    bytes_left -= bytes_written;
    crypto_util_proxy_.HandleShimOutput(&data);
  }
  // Write 0 bytes in to signify the end of the stream. This should in turn
  // cause our callback to be called.
  data.len = 0;
  data.buf = nullptr;
  EXPECT_CALL(
      crypto_util_proxy_,
      TestResultHandlerCallback(string(kTestSerializedCommandResponse),
                                ErrorIsOfType(Error::kSuccess))).Times(1);
  ExpectCleanup(Error(Error::kSuccess));
  crypto_util_proxy_.HandleShimOutput(&data);
}

}  // namespace shill
