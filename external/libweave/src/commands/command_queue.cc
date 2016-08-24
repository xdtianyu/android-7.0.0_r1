// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/commands/command_queue.h"

#include <base/bind.h>
#include <base/time/time.h>

namespace weave {

namespace {
const int kRemoveCommandDelayMin = 5;

std::string GetCommandHandlerKey(const std::string& component_path,
                                 const std::string& command_name) {
  return component_path + ":" + command_name;
}
}

CommandQueue::CommandQueue(provider::TaskRunner* task_runner,
                           base::Clock* clock)
    : task_runner_{task_runner}, clock_{clock} {}

void CommandQueue::AddCommandAddedCallback(const CommandCallback& callback) {
  on_command_added_.push_back(callback);
  // Send all pre-existed commands.
  for (const auto& command : map_)
    callback.Run(command.second.get());
}

void CommandQueue::AddCommandRemovedCallback(const CommandCallback& callback) {
  on_command_removed_.push_back(callback);
}

void CommandQueue::AddCommandHandler(
    const std::string& component_path,
    const std::string& command_name,
    const Device::CommandHandlerCallback& callback) {
  if (!command_name.empty()) {
    CHECK(default_command_callback_.is_null())
        << "Commands specific handler are not allowed after default one";

    for (const auto& command : map_) {
      if (command.second->GetState() == Command::State::kQueued &&
          command.second->GetName() == command_name &&
          command.second->GetComponent() == component_path) {
        callback.Run(command.second);
      }
    }

    std::string key = GetCommandHandlerKey(component_path, command_name);
    CHECK(command_callbacks_.insert(std::make_pair(key, callback)).second)
        << command_name << " already has handler";

  } else {
    CHECK(component_path.empty())
        << "Default handler must not be component-specific";
    for (const auto& command : map_) {
      std::string key = GetCommandHandlerKey(command.second->GetComponent(),
                                             command.second->GetName());
      if (command.second->GetState() == Command::State::kQueued &&
          command_callbacks_.find(key) == command_callbacks_.end()) {
        callback.Run(command.second);
      }
    }

    CHECK(default_command_callback_.is_null()) << "Already has default handler";
    default_command_callback_ = callback;
  }
}

void CommandQueue::Add(std::unique_ptr<CommandInstance> instance) {
  std::string id = instance->GetID();
  LOG_IF(FATAL, id.empty()) << "Command has no ID";
  instance->AttachToQueue(this);
  auto pair = map_.insert(std::make_pair(id, std::move(instance)));
  LOG_IF(FATAL, !pair.second) << "Command with ID '" << id
                              << "' is already in the queue";
  for (const auto& cb : on_command_added_)
    cb.Run(pair.first->second.get());

  std::string key = GetCommandHandlerKey(pair.first->second->GetComponent(),
                                         pair.first->second->GetName());
  auto it_handler = command_callbacks_.find(key);

  if (it_handler != command_callbacks_.end())
    it_handler->second.Run(pair.first->second);
  else if (!default_command_callback_.is_null())
    default_command_callback_.Run(pair.first->second);
}

void CommandQueue::RemoveLater(const std::string& id) {
  auto p = map_.find(id);
  if (p == map_.end())
    return;
  auto remove_delay = base::TimeDelta::FromMinutes(kRemoveCommandDelayMin);
  remove_queue_.push(std::make_pair(clock_->Now() + remove_delay, id));
  if (remove_queue_.size() == 1) {
    // The queue was empty, this is the first command to be removed, schedule
    // a clean-up task.
    ScheduleCleanup(remove_delay);
  }
}

bool CommandQueue::Remove(const std::string& id) {
  auto p = map_.find(id);
  if (p == map_.end())
    return false;
  std::shared_ptr<CommandInstance> instance = p->second;
  instance->DetachFromQueue();
  map_.erase(p);
  for (const auto& cb : on_command_removed_)
    cb.Run(instance.get());
  return true;
}

void CommandQueue::Cleanup(const base::Time& cutoff_time) {
  while (!remove_queue_.empty() && remove_queue_.top().first <= cutoff_time) {
    Remove(remove_queue_.top().second);
    remove_queue_.pop();
  }
}

void CommandQueue::ScheduleCleanup(base::TimeDelta delay) {
  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::Bind(&CommandQueue::PerformScheduledCleanup,
                 weak_ptr_factory_.GetWeakPtr()),
      delay);
}

void CommandQueue::PerformScheduledCleanup() {
  base::Time now = clock_->Now();
  Cleanup(now);
  if (!remove_queue_.empty())
    ScheduleCleanup(remove_queue_.top().first - now);
}

CommandInstance* CommandQueue::Find(const std::string& id) const {
  auto p = map_.find(id);
  return (p != map_.end()) ? p->second.get() : nullptr;
}

}  // namespace weave
