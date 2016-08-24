// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_COMMANDS_CLOUD_COMMAND_PROXY_H_
#define LIBWEAVE_SRC_COMMANDS_CLOUD_COMMAND_PROXY_H_

#include <deque>
#include <memory>
#include <string>
#include <utility>

#include <base/macros.h>
#include <base/memory/weak_ptr.h>
#include <base/scoped_observer.h>
#include <weave/command.h>

#include "src/backoff_entry.h"
#include "src/commands/cloud_command_update_interface.h"
#include "src/commands/command_instance.h"
#include "src/component_manager.h"

namespace weave {

class CommandInstance;

namespace provider {
class TaskRunner;
}

// Command proxy which publishes command updates to the cloud.
class CloudCommandProxy : public CommandInstance::Observer {
 public:
  CloudCommandProxy(CommandInstance* command_instance,
                    CloudCommandUpdateInterface* cloud_command_updater,
                    ComponentManager* component_manager,
                    std::unique_ptr<BackoffEntry> backoff_entry,
                    provider::TaskRunner* task_runner);
  ~CloudCommandProxy() override = default;

  // CommandProxyInterface implementation/overloads.
  void OnCommandDestroyed() override;
  void OnErrorChanged() override;
  void OnProgressChanged() override;
  void OnResultsChanged() override;
  void OnStateChanged() override;

 private:
  using UpdateQueueEntry = std::pair<ComponentManager::UpdateID,
                                     std::unique_ptr<base::DictionaryValue>>;
  // Puts a command update data into the update queue, and optionally sends an
  // asynchronous request to GCD server to update the command resource, if there
  // are no pending device status updates.
  void QueueCommandUpdate(std::unique_ptr<base::DictionaryValue> patch);

  // Sends an asynchronous request to GCD server to update the command resource,
  // if there are no pending device status updates.
  void SendCommandUpdate();

  // Retry the last failed command update request to the server.
  void ResendCommandUpdate();

  // Callback invoked by the asynchronous PATCH request to the server.
  void OnUpdateCommandDone(ErrorPtr error);

  // Callback invoked by the device state change queue to notify of the
  // successful device state update. |update_id| is the ID of the state that
  // has been updated on the server.
  void OnDeviceStateUpdated(ComponentManager::UpdateID update_id);

  CommandInstance* command_instance_;
  CloudCommandUpdateInterface* cloud_command_updater_;
  ComponentManager* component_manager_;
  provider::TaskRunner* task_runner_{nullptr};

  // Backoff for SendCommandUpdate() method.
  std::unique_ptr<BackoffEntry> cloud_backoff_entry_;

  // Set to true while a pending PATCH request is in flight to the server.
  bool command_update_in_progress_{false};
  // Update queue with all the command update requests ready to be sent to
  // the server.
  std::deque<UpdateQueueEntry> update_queue_;

  // Callback token from the state change queue for OnDeviceStateUpdated()
  // callback for ask the device state change queue to call when the state
  // is updated on the server.
  ComponentManager::Token callback_token_;

  // Last device state update ID that has been sent out to the server
  // successfully.
  ComponentManager::UpdateID last_state_update_id_{0};

  ScopedObserver<CommandInstance, CommandInstance::Observer> observer_{this};

  base::WeakPtrFactory<CloudCommandProxy> backoff_weak_ptr_factory_{this};
  base::WeakPtrFactory<CloudCommandProxy> weak_ptr_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(CloudCommandProxy);
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_COMMANDS_CLOUD_COMMAND_PROXY_H_
