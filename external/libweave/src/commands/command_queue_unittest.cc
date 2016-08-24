// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/commands/command_queue.h"

#include <set>
#include <string>
#include <vector>

#include <base/bind.h>
#include <base/memory/weak_ptr.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <weave/provider/test/fake_task_runner.h>

#include "src/bind_lambda.h"
#include "src/string_utils.h"

namespace weave {

using testing::Return;
using testing::StrictMock;

class CommandQueueTest : public testing::Test {
 public:
  std::unique_ptr<CommandInstance> CreateDummyCommandInstance(
      const std::string& name,
      const std::string& id) {
    std::unique_ptr<CommandInstance> cmd{
        new CommandInstance{name, Command::Origin::kLocal, {}}};
    cmd->SetID(id);
    return cmd;
  }

  bool Remove(const std::string& id) { return queue_.Remove(id); }

  void Cleanup(const base::TimeDelta& interval) {
    return queue_.Cleanup(task_runner_.GetClock()->Now() + interval);
  }

  std::string GetFirstCommandToBeRemoved() const {
    return queue_.remove_queue_.top().second;
  }

  StrictMock<provider::test::FakeTaskRunner> task_runner_;
  CommandQueue queue_{&task_runner_, task_runner_.GetClock()};
};

// Keeps track of commands being added to and removed from the queue_.
// Aborts if duplicate commands are added or non-existent commands are removed.
class FakeDispatcher {
 public:
  explicit FakeDispatcher(CommandQueue* queue) {
    queue->AddCommandAddedCallback(base::Bind(&FakeDispatcher::OnCommandAdded,
                                              weak_ptr_factory_.GetWeakPtr()));
    queue->AddCommandRemovedCallback(base::Bind(
        &FakeDispatcher::OnCommandRemoved, weak_ptr_factory_.GetWeakPtr()));
  }

  void OnCommandAdded(Command* command) {
    CHECK(ids_.insert(command->GetID()).second) << "Command ID already exists: "
                                                << command->GetID();
    CHECK(commands_.insert(command).second)
        << "Command instance already exists";
  }

  void OnCommandRemoved(Command* command) {
    CHECK_EQ(1u, ids_.erase(command->GetID())) << "Command ID not found: "
                                               << command->GetID();
    CHECK_EQ(1u, commands_.erase(command)) << "Command instance not found";
  }

  // Get the comma-separated list of command IDs currently accumulated in the
  // command queue_.
  std::string GetIDs() const {
    return Join(",", std::vector<std::string>(ids_.begin(), ids_.end()));
  }

 private:
  std::set<std::string> ids_;
  std::set<Command*> commands_;
  base::WeakPtrFactory<FakeDispatcher> weak_ptr_factory_{this};
};

TEST_F(CommandQueueTest, Empty) {
  EXPECT_TRUE(queue_.IsEmpty());
  EXPECT_EQ(0u, queue_.GetCount());
}

TEST_F(CommandQueueTest, Add) {
  queue_.Add(CreateDummyCommandInstance("base.reboot", "id1"));
  queue_.Add(CreateDummyCommandInstance("base.reboot", "id2"));
  queue_.Add(CreateDummyCommandInstance("base.reboot", "id3"));
  EXPECT_EQ(3u, queue_.GetCount());
  EXPECT_FALSE(queue_.IsEmpty());
}

TEST_F(CommandQueueTest, Remove) {
  const std::string id1 = "id1";
  const std::string id2 = "id2";
  queue_.Add(CreateDummyCommandInstance("base.reboot", id1));
  queue_.Add(CreateDummyCommandInstance("base.reboot", id2));
  EXPECT_FALSE(queue_.IsEmpty());
  EXPECT_FALSE(Remove("dummy"));
  EXPECT_EQ(2u, queue_.GetCount());
  EXPECT_TRUE(Remove(id1));
  EXPECT_EQ(1u, queue_.GetCount());
  EXPECT_FALSE(Remove(id1));
  EXPECT_EQ(1u, queue_.GetCount());
  EXPECT_TRUE(Remove(id2));
  EXPECT_EQ(0u, queue_.GetCount());
  EXPECT_FALSE(Remove(id2));
  EXPECT_EQ(0u, queue_.GetCount());
  EXPECT_TRUE(queue_.IsEmpty());
}

TEST_F(CommandQueueTest, RemoveLater) {
  const std::string id1 = "id1";
  queue_.Add(CreateDummyCommandInstance("base.reboot", id1));
  EXPECT_EQ(1u, queue_.GetCount());

  queue_.RemoveLater(id1);
  EXPECT_EQ(1u, queue_.GetCount());

  Cleanup(base::TimeDelta::FromMinutes(1));
  EXPECT_EQ(1u, queue_.GetCount());

  Cleanup(base::TimeDelta::FromMinutes(15));
  EXPECT_EQ(0u, queue_.GetCount());
}

TEST_F(CommandQueueTest, RemoveLaterOnCleanupTask) {
  const std::string id1 = "id1";
  queue_.Add(CreateDummyCommandInstance("base.reboot", id1));
  EXPECT_EQ(1u, queue_.GetCount());

  queue_.RemoveLater(id1);
  EXPECT_EQ(1u, queue_.GetCount());
  ASSERT_EQ(1u, task_runner_.GetTaskQueueSize());

  task_runner_.RunOnce();

  EXPECT_EQ(0u, queue_.GetCount());
  EXPECT_EQ(0u, task_runner_.GetTaskQueueSize());
}

TEST_F(CommandQueueTest, CleanupMultipleCommands) {
  const std::string id1 = "id1";
  const std::string id2 = "id2";

  queue_.Add(CreateDummyCommandInstance("base.reboot", id1));
  queue_.Add(CreateDummyCommandInstance("base.reboot", id2));
  auto remove_task = [this](const std::string& id) { queue_.RemoveLater(id); };
  remove_task(id1);
  task_runner_.PostDelayedTask(FROM_HERE, base::Bind(remove_task, id2),
                               base::TimeDelta::FromSeconds(10));
  EXPECT_EQ(2u, queue_.GetCount());
  ASSERT_EQ(2u, task_runner_.GetTaskQueueSize());
  task_runner_.RunOnce();  // Executes "remove_task(id2) @ T+10s".
  ASSERT_EQ(2u, queue_.GetCount());
  ASSERT_EQ(1u, task_runner_.GetTaskQueueSize());
  EXPECT_EQ(id1, GetFirstCommandToBeRemoved());
  task_runner_.RunOnce();  // Should remove task "id1" from queue.
  ASSERT_EQ(1u, queue_.GetCount());
  ASSERT_EQ(1u, task_runner_.GetTaskQueueSize());
  EXPECT_EQ(id2, GetFirstCommandToBeRemoved());
  task_runner_.RunOnce();  // Should remove task "id2" from queue.
  EXPECT_EQ(0u, queue_.GetCount());
  EXPECT_EQ(0u, task_runner_.GetTaskQueueSize());
}

TEST_F(CommandQueueTest, Dispatch) {
  FakeDispatcher dispatch(&queue_);
  const std::string id1 = "id1";
  const std::string id2 = "id2";
  queue_.Add(CreateDummyCommandInstance("base.reboot", id1));
  queue_.Add(CreateDummyCommandInstance("base.reboot", id2));
  std::set<std::string> ids{id1, id2};  // Make sure they are sorted properly.
  std::string expected_set =
      Join(",", std::vector<std::string>(ids.begin(), ids.end()));
  EXPECT_EQ(expected_set, dispatch.GetIDs());
  Remove(id1);
  EXPECT_EQ(id2, dispatch.GetIDs());
  Remove(id2);
  EXPECT_EQ("", dispatch.GetIDs());
}

TEST_F(CommandQueueTest, Find) {
  const std::string id1 = "id1";
  const std::string id2 = "id2";
  queue_.Add(CreateDummyCommandInstance("base.reboot", id1));
  queue_.Add(CreateDummyCommandInstance("base.shutdown", id2));
  EXPECT_EQ(nullptr, queue_.Find("dummy"));
  auto cmd1 = queue_.Find(id1);
  EXPECT_NE(nullptr, cmd1);
  EXPECT_EQ("base.reboot", cmd1->GetName());
  EXPECT_EQ(id1, cmd1->GetID());
  auto cmd2 = queue_.Find(id2);
  EXPECT_NE(nullptr, cmd2);
  EXPECT_EQ("base.shutdown", cmd2->GetName());
  EXPECT_EQ(id2, cmd2->GetID());
}

}  // namespace weave
