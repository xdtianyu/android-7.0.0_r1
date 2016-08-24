// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/commands/cloud_command_proxy.h"

#include <base/bind.h>
#include <weave/enum_to_string.h>
#include <weave/provider/task_runner.h>

#include "src/commands/command_instance.h"
#include "src/commands/schema_constants.h"
#include "src/utils.h"

namespace weave {

CloudCommandProxy::CloudCommandProxy(
    CommandInstance* command_instance,
    CloudCommandUpdateInterface* cloud_command_updater,
    ComponentManager* component_manager,
    std::unique_ptr<BackoffEntry> backoff_entry,
    provider::TaskRunner* task_runner)
    : command_instance_{command_instance},
      cloud_command_updater_{cloud_command_updater},
      component_manager_{component_manager},
      task_runner_{task_runner},
      cloud_backoff_entry_{std::move(backoff_entry)} {
  callback_token_ = component_manager_->AddServerStateUpdatedCallback(
      base::Bind(&CloudCommandProxy::OnDeviceStateUpdated,
                 weak_ptr_factory_.GetWeakPtr()));
  observer_.Add(command_instance);
}

void CloudCommandProxy::OnErrorChanged() {
  std::unique_ptr<base::DictionaryValue> patch{new base::DictionaryValue};
  patch->Set(commands::attributes::kCommand_Error,
             command_instance_->GetError()
                 ? ErrorInfoToJson(*command_instance_->GetError()).release()
                 : base::Value::CreateNullValue().release());
  QueueCommandUpdate(std::move(patch));
}

void CloudCommandProxy::OnResultsChanged() {
  std::unique_ptr<base::DictionaryValue> patch{new base::DictionaryValue};
  patch->Set(commands::attributes::kCommand_Results,
             command_instance_->GetResults().CreateDeepCopy());
  QueueCommandUpdate(std::move(patch));
}

void CloudCommandProxy::OnStateChanged() {
  std::unique_ptr<base::DictionaryValue> patch{new base::DictionaryValue};
  patch->SetString(commands::attributes::kCommand_State,
                   EnumToString(command_instance_->GetState()));
  QueueCommandUpdate(std::move(patch));
}

void CloudCommandProxy::OnProgressChanged() {
  std::unique_ptr<base::DictionaryValue> patch{new base::DictionaryValue};
  patch->Set(commands::attributes::kCommand_Progress,
             command_instance_->GetProgress().CreateDeepCopy());
  QueueCommandUpdate(std::move(patch));
}

void CloudCommandProxy::OnCommandDestroyed() {
  delete this;
}

void CloudCommandProxy::QueueCommandUpdate(
    std::unique_ptr<base::DictionaryValue> patch) {
  ComponentManager::UpdateID id = component_manager_->GetLastStateChangeId();
  if (update_queue_.empty() || update_queue_.back().first != id) {
    // If queue is currently empty or the device state has changed since the
    // last patch request queued, add a new request to the queue.
    update_queue_.push_back(std::make_pair(id, std::move(patch)));
  } else {
    // Device state hasn't changed since the last time this command update
    // was queued. We can coalesce the command update patches, unless the
    // current request is already in flight to the server.
    if (update_queue_.size() == 1 && command_update_in_progress_) {
      // Can't update the request which is being sent to the server.
      // Queue a new update.
      update_queue_.push_back(std::make_pair(id, std::move(patch)));
    } else {
      // Coalesce the patches.
      update_queue_.back().second->MergeDictionary(patch.get());
    }
  }
  // Send out an update request to the server, if needed.

  // Post to accumulate more changes during the current message loop task run.
  task_runner_->PostDelayedTask(
      FROM_HERE, base::Bind(&CloudCommandProxy::SendCommandUpdate,
                            backoff_weak_ptr_factory_.GetWeakPtr()),
      {});
}

void CloudCommandProxy::SendCommandUpdate() {
  if (command_update_in_progress_ || update_queue_.empty())
    return;

  // Check if we have any pending updates ready to be sent to the server.
  // We can only send updates for which the device state at the time the
  // requests have been queued were successfully propagated to the server.
  // That is, if the pending device state updates that we recorded while the
  // command update was queued haven't been acknowledged by the server, we
  // will hold the corresponding command updates until the related device state
  // has been successfully updated on the server.
  if (update_queue_.front().first > last_state_update_id_)
    return;

  backoff_weak_ptr_factory_.InvalidateWeakPtrs();
  if (cloud_backoff_entry_->ShouldRejectRequest()) {
    VLOG(1) << "Cloud request delayed for "
            << cloud_backoff_entry_->GetTimeUntilRelease()
            << " due to backoff policy";
    task_runner_->PostDelayedTask(
        FROM_HERE, base::Bind(&CloudCommandProxy::SendCommandUpdate,
                              backoff_weak_ptr_factory_.GetWeakPtr()),
        cloud_backoff_entry_->GetTimeUntilRelease());
    return;
  }

  // Coalesce any pending updates that were queued prior to the current device
  // state known to be propagated to the server successfully.
  auto iter = update_queue_.begin();
  auto start = ++iter;
  while (iter != update_queue_.end()) {
    if (iter->first > last_state_update_id_)
      break;
    update_queue_.front().first = iter->first;
    update_queue_.front().second->MergeDictionary(iter->second.get());
    ++iter;
  }
  // Remove all the intermediate items that have been merged into the first
  // entry.
  update_queue_.erase(start, iter);
  command_update_in_progress_ = true;
  cloud_command_updater_->UpdateCommand(
      command_instance_->GetID(), *update_queue_.front().second,
      base::Bind(&CloudCommandProxy::OnUpdateCommandDone,
                 weak_ptr_factory_.GetWeakPtr()));
}

void CloudCommandProxy::ResendCommandUpdate() {
  command_update_in_progress_ = false;
  SendCommandUpdate();
}

void CloudCommandProxy::OnUpdateCommandDone(ErrorPtr error) {
  command_update_in_progress_ = false;
  cloud_backoff_entry_->InformOfRequest(!error);
  if (!error) {
    // Remove the succeeded update from the queue.
    update_queue_.pop_front();
  }
  // If we have more pending updates, send a new request to the server
  // immediately, if possible.
  SendCommandUpdate();
}

void CloudCommandProxy::OnDeviceStateUpdated(
    ComponentManager::UpdateID update_id) {
  last_state_update_id_ = update_id;
  // Try to send out any queued command updates that could be performed after
  // a device state is updated.
  SendCommandUpdate();
}

}  // namespace weave
