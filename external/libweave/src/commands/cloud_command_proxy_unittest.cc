// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/commands/cloud_command_proxy.h"

#include <memory>
#include <queue>

#include <base/bind.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <weave/provider/test/fake_task_runner.h>
#include <weave/test/unittest_utils.h>

#include "src/commands/command_instance.h"
#include "src/mock_component_manager.h"

using testing::_;
using testing::AnyNumber;
using testing::DoAll;
using testing::Invoke;
using testing::Return;
using testing::ReturnPointee;
using testing::SaveArg;

namespace weave {

using test::CreateDictionaryValue;
using test::CreateValue;

namespace {

const char kCmdID[] = "abcd";

MATCHER_P(MatchJson, str, "") {
  return arg.Equals(CreateValue(str).get());
}

class MockCloudCommandUpdateInterface : public CloudCommandUpdateInterface {
 public:
  MOCK_METHOD3(UpdateCommand,
               void(const std::string&,
                    const base::DictionaryValue&,
                    const DoneCallback&));
};

// Test back-off entry that uses the test clock.
class TestBackoffEntry : public BackoffEntry {
 public:
  TestBackoffEntry(const Policy* const policy, base::Clock* clock)
      : BackoffEntry{policy}, clock_{clock} {
    creation_time_ = clock->Now();
  }

 private:
  // Override from BackoffEntry to use the custom test clock for
  // the backoff calculations.
  base::TimeTicks ImplGetTimeNow() const override {
    return base::TimeTicks::FromInternalValue(clock_->Now().ToInternalValue());
  }

  base::Clock* clock_;
  base::Time creation_time_;
};

class CloudCommandProxyWrapper : public CloudCommandProxy {
 public:
  CloudCommandProxyWrapper(CommandInstance* command_instance,
                           CloudCommandUpdateInterface* cloud_command_updater,
                           ComponentManager* component_manager,
                           std::unique_ptr<BackoffEntry> backoff_entry,
                           provider::TaskRunner* task_runner,
                           const base::Closure& destruct_callback)
      : CloudCommandProxy{command_instance, cloud_command_updater,
                          component_manager, std::move(backoff_entry),
                          task_runner},
        destruct_callback_{destruct_callback} {}

  ~CloudCommandProxyWrapper() {
    destruct_callback_.Run();
  }

 private:
  base::Closure destruct_callback_;
};

class CloudCommandProxyTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Set up the test ComponentManager.
    auto callback = [this](
        const base::Callback<void(ComponentManager::UpdateID)>& call) {
      return callbacks_.Add(call).release();
    };
    EXPECT_CALL(component_manager_, MockAddServerStateUpdatedCallback(_))
        .WillRepeatedly(Invoke(callback));
    EXPECT_CALL(component_manager_, GetLastStateChangeId())
        .WillRepeatedly(testing::ReturnPointee(&current_state_update_id_));

    CreateCommandInstance();
  }

  void CreateCommandInstance() {
    auto command_json = CreateDictionaryValue(R"({
      'name': 'calc.add',
      'id': 'abcd',
      'parameters': {
        'value1': 10,
        'value2': 20
      }
    })");
    CHECK(command_json.get());

    command_instance_ = CommandInstance::FromJson(
        command_json.get(), Command::Origin::kCloud, nullptr, nullptr);
    CHECK(command_instance_.get());

    // Backoff - start at 1s and double with each backoff attempt and no jitter.
    static const BackoffEntry::Policy policy{0,     1000, 2.0,  0.0,
                                             20000, -1,   false};
    std::unique_ptr<TestBackoffEntry> backoff{
        new TestBackoffEntry{&policy, task_runner_.GetClock()}};

    // Finally construct the CloudCommandProxy we are going to test here.
    std::unique_ptr<CloudCommandProxy> proxy{new CloudCommandProxyWrapper{
        command_instance_.get(), &cloud_updater_, &component_manager_,
        std::move(backoff), &task_runner_,
        base::Bind(&CloudCommandProxyTest::OnProxyDestroyed,
                   base::Unretained(this))}};
    // CloudCommandProxy::CloudCommandProxy() subscribe itself to weave::Command
    // notifications. When weave::Command is being destroyed it sends
    // ::OnCommandDestroyed() and CloudCommandProxy deletes itself.
    proxy.release();

    EXPECT_CALL(*this, OnProxyDestroyed()).Times(AnyNumber());
  }

  MOCK_METHOD0(OnProxyDestroyed, void());

  ComponentManager::UpdateID current_state_update_id_{0};
  base::CallbackList<void(ComponentManager::UpdateID)> callbacks_;
  testing::StrictMock<MockCloudCommandUpdateInterface> cloud_updater_;
  testing::StrictMock<MockComponentManager> component_manager_;
  testing::StrictMock<provider::test::FakeTaskRunner> task_runner_;
  std::queue<base::Closure> task_queue_;
  std::unique_ptr<CommandInstance> command_instance_;
};

}  // anonymous namespace

TEST_F(CloudCommandProxyTest, EnsureDestroyed) {
  EXPECT_CALL(*this, OnProxyDestroyed()).Times(1);
  command_instance_.reset();
  // Verify that CloudCommandProxy has been destroyed already and not at some
  // point during the destruction of CloudCommandProxyTest class.
  testing::Mock::VerifyAndClearExpectations(this);
}

TEST_F(CloudCommandProxyTest, ImmediateUpdate) {
  const char expected[] = "{'state':'done'}";
  EXPECT_CALL(cloud_updater_, UpdateCommand(kCmdID, MatchJson(expected), _));
  command_instance_->Complete({}, nullptr);
  task_runner_.RunOnce();
}

TEST_F(CloudCommandProxyTest, DelayedUpdate) {
  // Simulate that the current device state has changed.
  current_state_update_id_ = 20;
  // No command update is expected here.
  command_instance_->Complete({}, nullptr);
  // Still no command update here...
  callbacks_.Notify(19);
  // Now we should get the update...
  const char expected[] = "{'state':'done'}";
  EXPECT_CALL(cloud_updater_, UpdateCommand(kCmdID, MatchJson(expected), _));
  callbacks_.Notify(20);
}

TEST_F(CloudCommandProxyTest, InFlightRequest) {
  // SetProgress causes two consecutive updates:
  //    state=inProgress
  //    progress={...}
  // The first state update is sent immediately, the second should be delayed.
  DoneCallback callback;
  EXPECT_CALL(
      cloud_updater_,
      UpdateCommand(
          kCmdID,
          MatchJson("{'state':'inProgress', 'progress':{'status':'ready'}}"),
          _))
      .WillOnce(SaveArg<2>(&callback));
  EXPECT_TRUE(command_instance_->SetProgress(
      *CreateDictionaryValue("{'status': 'ready'}"), nullptr));

  task_runner_.RunOnce();
}

TEST_F(CloudCommandProxyTest, CombineMultiple) {
  // Simulate that the current device state has changed.
  current_state_update_id_ = 20;
  // SetProgress causes two consecutive updates:
  //    state=inProgress
  //    progress={...}
  // Both updates will be held until device state is updated.
  EXPECT_TRUE(command_instance_->SetProgress(
      *CreateDictionaryValue("{'status': 'ready'}"), nullptr));

  // Now simulate the device state updated. Both updates should come in one
  // request.
  const char expected[] = R"({
    'progress': {'status':'ready'},
    'state':'inProgress'
  })";
  EXPECT_CALL(cloud_updater_, UpdateCommand(kCmdID, MatchJson(expected), _));
  callbacks_.Notify(20);
}

TEST_F(CloudCommandProxyTest, RetryFailed) {
  DoneCallback callback;

  const char expect[] =
      "{'state':'inProgress', 'progress': {'status': 'ready'}}";
  EXPECT_CALL(cloud_updater_, UpdateCommand(kCmdID, MatchJson(expect), _))
      .Times(3)
      .WillRepeatedly(SaveArg<2>(&callback));
  auto started = task_runner_.GetClock()->Now();
  EXPECT_TRUE(command_instance_->SetProgress(
      *CreateDictionaryValue("{'status': 'ready'}"), nullptr));
  task_runner_.Run();
  ErrorPtr error;
  Error::AddTo(&error, FROM_HERE, "TEST", "TEST");
  callback.Run(error->Clone());
  task_runner_.Run();
  EXPECT_GE(task_runner_.GetClock()->Now() - started,
            base::TimeDelta::FromSecondsD(0.9));

  callback.Run(error->Clone());
  task_runner_.Run();
  EXPECT_GE(task_runner_.GetClock()->Now() - started,
            base::TimeDelta::FromSecondsD(2.9));

  callback.Run(nullptr);
  task_runner_.Run();
  EXPECT_GE(task_runner_.GetClock()->Now() - started,
            base::TimeDelta::FromSecondsD(2.9));
}

TEST_F(CloudCommandProxyTest, GateOnStateUpdates) {
  current_state_update_id_ = 20;
  EXPECT_TRUE(command_instance_->SetProgress(
      *CreateDictionaryValue("{'status': 'ready'}"), nullptr));
  current_state_update_id_ = 21;
  EXPECT_TRUE(command_instance_->SetProgress(
      *CreateDictionaryValue("{'status': 'busy'}"), nullptr));
  current_state_update_id_ = 22;
  command_instance_->Complete({}, nullptr);

  // Device state #20 updated.
  DoneCallback callback;
  const char expect1[] = R"({
    'progress': {'status':'ready'},
    'state':'inProgress'
  })";
  EXPECT_CALL(cloud_updater_, UpdateCommand(kCmdID, MatchJson(expect1), _))
      .WillOnce(SaveArg<2>(&callback));
  callbacks_.Notify(20);
  callback.Run(nullptr);

  // Device state #21 updated.
  const char expect2[] = "{'progress': {'status':'busy'}}";
  EXPECT_CALL(cloud_updater_, UpdateCommand(kCmdID, MatchJson(expect2), _))
      .WillOnce(SaveArg<2>(&callback));
  callbacks_.Notify(21);

  // Device state #22 updated. Nothing happens here since the previous command
  // update request hasn't completed yet.
  callbacks_.Notify(22);

  // Now the command update is complete, send out the patch that happened after
  // the state #22 was updated.
  const char expect3[] = "{'state': 'done'}";
  EXPECT_CALL(cloud_updater_, UpdateCommand(kCmdID, MatchJson(expect3), _))
      .WillOnce(SaveArg<2>(&callback));
  callback.Run(nullptr);
}

TEST_F(CloudCommandProxyTest, CombineSomeStates) {
  current_state_update_id_ = 20;
  EXPECT_TRUE(command_instance_->SetProgress(
      *CreateDictionaryValue("{'status': 'ready'}"), nullptr));
  current_state_update_id_ = 21;
  EXPECT_TRUE(command_instance_->SetProgress(
      *CreateDictionaryValue("{'status': 'busy'}"), nullptr));
  current_state_update_id_ = 22;
  command_instance_->Complete({}, nullptr);

  // Device state 20-21 updated.
  DoneCallback callback;
  const char expect1[] = R"({
    'progress': {'status':'busy'},
    'state':'inProgress'
  })";
  EXPECT_CALL(cloud_updater_, UpdateCommand(kCmdID, MatchJson(expect1), _))
      .WillOnce(SaveArg<2>(&callback));
  callbacks_.Notify(21);
  callback.Run(nullptr);

  // Device state #22 updated.
  const char expect2[] = "{'state': 'done'}";
  EXPECT_CALL(cloud_updater_, UpdateCommand(kCmdID, MatchJson(expect2), _))
      .WillOnce(SaveArg<2>(&callback));
  callbacks_.Notify(22);
  callback.Run(nullptr);
}

TEST_F(CloudCommandProxyTest, CombineAllStates) {
  current_state_update_id_ = 20;
  EXPECT_TRUE(command_instance_->SetProgress(
      *CreateDictionaryValue("{'status': 'ready'}"), nullptr));
  current_state_update_id_ = 21;
  EXPECT_TRUE(command_instance_->SetProgress(
      *CreateDictionaryValue("{'status': 'busy'}"), nullptr));
  current_state_update_id_ = 22;
  command_instance_->Complete({}, nullptr);

  // Device state 30 updated.
  const char expected[] = R"({
    'progress': {'status':'busy'},
    'state':'done'
  })";
  EXPECT_CALL(cloud_updater_, UpdateCommand(kCmdID, MatchJson(expected), _));
  callbacks_.Notify(30);
}

TEST_F(CloudCommandProxyTest, CoalesceUpdates) {
  current_state_update_id_ = 20;
  EXPECT_TRUE(command_instance_->SetProgress(
      *CreateDictionaryValue("{'status': 'ready'}"), nullptr));
  EXPECT_TRUE(command_instance_->SetProgress(
      *CreateDictionaryValue("{'status': 'busy'}"), nullptr));
  EXPECT_TRUE(command_instance_->SetProgress(
      *CreateDictionaryValue("{'status': 'finished'}"), nullptr));
  EXPECT_TRUE(command_instance_->Complete(*CreateDictionaryValue("{'sum': 30}"),
                                          nullptr));

  const char expected[] = R"({
    'progress': {'status':'finished'},
    'results': {'sum':30},
    'state':'done'
  })";
  EXPECT_CALL(cloud_updater_, UpdateCommand(kCmdID, MatchJson(expected), _));
  callbacks_.Notify(30);
}

TEST_F(CloudCommandProxyTest, EmptyStateChangeQueue) {
  // Assume the device state update queue was empty and was at update ID 20.
  current_state_update_id_ = 20;

  // Recreate the command instance and proxy with the new state change queue.
  CreateCommandInstance();

  // Empty queue will immediately call back with the state change notification.
  callbacks_.Notify(20);

  // As soon as we change the command, the update to the server should be sent.
  const char expected[] = "{'state':'done'}";
  EXPECT_CALL(cloud_updater_, UpdateCommand(kCmdID, MatchJson(expected), _));
  command_instance_->Complete({}, nullptr);
  task_runner_.RunOnce();
}

TEST_F(CloudCommandProxyTest, NonEmptyStateChangeQueue) {
  // Assume the device state update queue was NOT empty when the command
  // instance was created.
  current_state_update_id_ = 20;

  // Recreate the command instance and proxy with the new state change queue.
  CreateCommandInstance();

  // No command updates right now.
  command_instance_->Complete({}, nullptr);

  // Only when the state #20 is published we should update the command
  const char expected[] = "{'state':'done'}";
  EXPECT_CALL(cloud_updater_, UpdateCommand(kCmdID, MatchJson(expected), _));
  callbacks_.Notify(20);
}

}  // namespace weave
