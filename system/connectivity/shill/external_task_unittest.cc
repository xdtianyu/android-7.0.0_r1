//
// Copyright (C) 2012 The Android Open Source Project
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

#include "shill/external_task.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <base/bind.h>
#include <base/files/file_path.h>
#include <base/memory/weak_ptr.h>
#include <base/strings/string_util.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/mock_adaptors.h"
#include "shill/mock_process_manager.h"
#include "shill/nice_mock_control.h"
#include "shill/test_event_dispatcher.h"

using std::map;
using std::set;
using std::string;
using std::vector;
using testing::_;
using testing::Matcher;
using testing::MatchesRegex;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::SetArgumentPointee;
using testing::StrEq;

namespace shill {

class ExternalTaskTest : public testing::Test,
                         public RPCTaskDelegate {
 public:
  ExternalTaskTest()
      : weak_ptr_factory_(this),
        death_callback_(
          base::Bind(&ExternalTaskTest::TaskDiedCallback,
                     weak_ptr_factory_.GetWeakPtr())),
        external_task_(
            new ExternalTask(&control_, &process_manager_,
                             weak_ptr_factory_.GetWeakPtr(),
                             death_callback_)),
        test_rpc_task_destroyed_(false) {}

  virtual ~ExternalTaskTest() {}

  virtual void TearDown() {
    if (!external_task_) {
      return;
    }

    if (external_task_->pid_) {
      EXPECT_CALL(process_manager_, StopProcess(external_task_->pid_));
    }
  }

  void set_test_rpc_task_destroyed(bool destroyed) {
    test_rpc_task_destroyed_ = destroyed;
  }

  // Defined out-of-line, due to dependency on TestRPCTask.
  void FakeUpRunningProcess(unsigned int tag, int pid);

  void ExpectStop(unsigned int tag, int pid) {
    EXPECT_CALL(process_manager_, StopProcess(pid));
  }

  void VerifyStop() {
    if (external_task_) {
      EXPECT_EQ(0, external_task_->pid_);
      EXPECT_FALSE(external_task_->rpc_task_);
    }
    EXPECT_TRUE(test_rpc_task_destroyed_);
    // Make sure EXPECTations were met before the fixture's dtor.
    Mock::VerifyAndClearExpectations(&process_manager_);
  }

 protected:
  // Implements RPCTaskDelegate interface.
  MOCK_METHOD2(GetLogin, void(string* user, string* password));
  MOCK_METHOD2(Notify, void(const string& reason,
                            const map<string, string>& dict));

  MOCK_METHOD2(TaskDiedCallback, void(pid_t pid, int exit_status));

  NiceMockControl control_;
  EventDispatcherForTest dispatcher_;
  MockProcessManager process_manager_;
  base::WeakPtrFactory<ExternalTaskTest> weak_ptr_factory_;
  base::Callback<void(pid_t, int)> death_callback_;
  std::unique_ptr<ExternalTask> external_task_;
  bool test_rpc_task_destroyed_;
};

namespace {

class TestRPCTask : public RPCTask {
 public:
  TestRPCTask(ControlInterface* control, ExternalTaskTest* test);
  virtual ~TestRPCTask();

 private:
  ExternalTaskTest* test_;
};

TestRPCTask::TestRPCTask(ControlInterface* control, ExternalTaskTest* test)
    : RPCTask(control, test),
      test_(test) {
  test_->set_test_rpc_task_destroyed(false);
}

TestRPCTask::~TestRPCTask() {
  test_->set_test_rpc_task_destroyed(true);
  test_ = nullptr;
}

}  // namespace

void ExternalTaskTest::FakeUpRunningProcess(unsigned int tag, int pid) {
  external_task_->pid_ = pid;
  external_task_->rpc_task_.reset(new TestRPCTask(&control_, this));
}

TEST_F(ExternalTaskTest, Destructor) {
  const unsigned int kTag = 123;
  const int kPID = 123456;
  FakeUpRunningProcess(kTag, kPID);
  ExpectStop(kTag, kPID);
  external_task_.reset();
  VerifyStop();
}

TEST_F(ExternalTaskTest, DestroyLater) {
  const unsigned int kTag = 123;
  const int kPID = 123456;
  FakeUpRunningProcess(kTag, kPID);
  ExpectStop(kTag, kPID);
  external_task_.release()->DestroyLater(&dispatcher_);
  dispatcher_.DispatchPendingEvents();
  VerifyStop();
}

namespace {

// Returns true iff. there is at least one anchored match in |arg|,
// for each item in |expected_values|. Order of items does not matter.
//
// |arg| is a NULL-terminated array of C-strings.
// |expected_values| is a container of regular expressions (as strings).
MATCHER_P(HasElementsMatching, expected_values, "") {
  for (const auto& expected_value : expected_values) {
    auto regex_matcher(MatchesRegex(expected_value).impl());
    char** arg_local = arg;
    while (*arg_local) {
      if (regex_matcher.MatchAndExplain(*arg_local, result_listener)) {
        break;
      }
      ++arg_local;
    }
    if (*arg_local == nullptr) {
      *result_listener << "missing value " << expected_value << "\n";
      arg_local = arg;
      while (*arg_local) {
        *result_listener << "received: " << *arg_local << "\n";
        ++arg_local;
      }
      return false;
    }
  }
  return true;
}

}  // namespace

TEST_F(ExternalTaskTest, Start) {
  const string kCommand = "/run/me";
  const vector<string> kCommandOptions{"arg1", "arg2"};
  const map<string, string> kCommandEnv{{"env1", "val1"}, {"env2", "val2"}};
  map<string, string> expected_env;
  expected_env.emplace(kRPCTaskServiceVariable, RPCTaskMockAdaptor::kRpcConnId);
  expected_env.emplace(kRPCTaskPathVariable, RPCTaskMockAdaptor::kRpcId);
  expected_env.insert(kCommandEnv.begin(), kCommandEnv.end());
  const int kPID = 234678;
  EXPECT_CALL(process_manager_,
              StartProcess(_, base::FilePath(kCommand), kCommandOptions,
                           expected_env, false, _))
      .WillOnce(Return(-1))
      .WillOnce(Return(kPID));
  Error error;
  EXPECT_FALSE(external_task_->Start(
      base::FilePath(kCommand), kCommandOptions, kCommandEnv, false, &error));
  EXPECT_EQ(Error::kInternalError, error.type());
  EXPECT_FALSE(external_task_->rpc_task_);

  error.Reset();
  EXPECT_TRUE(external_task_->Start(
      base::FilePath(kCommand), kCommandOptions, kCommandEnv, false, &error));
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_EQ(kPID, external_task_->pid_);
  EXPECT_NE(nullptr, external_task_->rpc_task_);
}

TEST_F(ExternalTaskTest, Stop) {
  const unsigned int kTag = 123;
  const int kPID = 123456;
  FakeUpRunningProcess(kTag, kPID);
  ExpectStop(kTag, kPID);
  external_task_->Stop();
  ASSERT_NE(nullptr, external_task_);
  VerifyStop();
}

TEST_F(ExternalTaskTest, StopNotStarted) {
  EXPECT_CALL(process_manager_, StopProcess(_)).Times(0);
  external_task_->Stop();
  EXPECT_FALSE(test_rpc_task_destroyed_);
}

TEST_F(ExternalTaskTest, GetLogin) {
  string username;
  string password;
  EXPECT_CALL(*this, GetLogin(&username, &password));
  EXPECT_CALL(*this, Notify(_, _)).Times(0);
  external_task_->GetLogin(&username, &password);
}

TEST_F(ExternalTaskTest, Notify) {
  const string kReason("you may already have won!");
  const map<string, string>& kArgs{
    {"arg1", "val1"},
    {"arg2", "val2"}};
  EXPECT_CALL(*this, GetLogin(_, _)).Times(0);
  EXPECT_CALL(*this, Notify(kReason, kArgs));
  external_task_->Notify(kReason, kArgs);
}

TEST_F(ExternalTaskTest, OnTaskDied) {
  const int kPID = 99999;
  const int kExitStatus = 1;
  external_task_->pid_ = kPID;
  EXPECT_CALL(process_manager_, StopProcess(_)).Times(0);
  EXPECT_CALL(*this, TaskDiedCallback(kPID, kExitStatus));
  external_task_->OnTaskDied(kExitStatus);
  EXPECT_EQ(0, external_task_->pid_);
}

}  // namespace shill
