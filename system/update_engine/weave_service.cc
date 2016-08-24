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

#include "update_engine/weave_service.h"

#include <cmath>
#include <string>

#include <base/bind.h>
#include <brillo/errors/error.h>
#include <brillo/message_loops/message_loop.h>

#include "update_engine/update_status_utils.h"

using std::string;

namespace {

const char kWeaveComponent[] = "updater";
const char kWeaveTrait[] = "_updater";

}  // namespace

namespace chromeos_update_engine {

bool WeaveService::Init(DelegateInterface* delegate) {
  delegate_ = delegate;
  weave_service_subscription_ = weaved::Service::Connect(
      brillo::MessageLoop::current(),
      base::Bind(&WeaveService::OnWeaveServiceConnected,
                 base::Unretained(this)));
  return true;
}

void WeaveService::OnWeaveServiceConnected(
    const std::weak_ptr<weaved::Service>& service) {
  weave_service_ = service;
  auto weave_service = weave_service_.lock();
  if (!weave_service)
    return;

  weave_service->AddComponent(kWeaveComponent, {kWeaveTrait}, nullptr);
  weave_service->AddCommandHandler(
      kWeaveComponent, kWeaveTrait, "checkForUpdates",
      base::Bind(&WeaveService::OnCheckForUpdates, base::Unretained(this)));
  weave_service->AddCommandHandler(
      kWeaveComponent, kWeaveTrait, "trackChannel",
      base::Bind(&WeaveService::OnTrackChannel, base::Unretained(this)));
  UpdateWeaveState();
}

void WeaveService::SendStatusUpdate(int64_t /* last_checked_time */,
                                    double /* progress */,
                                    update_engine::UpdateStatus /* status */,
                                    const string& /* new_version */,
                                    int64_t /* new_size */) {
  // We query the Weave
  UpdateWeaveState();
}

void WeaveService::SendChannelChangeUpdate(
    const string& /* tracking_channel */) {
  UpdateWeaveState();
}

void WeaveService::UpdateWeaveState() {
  auto weave_service = weave_service_.lock();
  if (!weave_service || !delegate_)
    return;

  int64_t last_checked_time;
  double progress;
  update_engine::UpdateStatus update_status;
  string current_channel;
  string tracking_channel;

  if (!delegate_->GetWeaveState(&last_checked_time,
                                &progress,
                                &update_status,
                                &current_channel,
                                &tracking_channel))
    return;

  // Round to progress to 1% (0.01) to avoid excessive and meaningless state
  // changes.
  progress = std::floor(progress * 100.) / 100.;

  base::DictionaryValue state;
  state.SetString("_updater.currentChannel", current_channel);
  state.SetString("_updater.trackingChannel", tracking_channel);
  state.SetString("_updater.status", UpdateStatusToWeaveStatus(update_status));
  state.SetDouble("_updater.progress", progress);
  state.SetDouble("_updater.lastUpdateCheckTimestamp",
                  static_cast<double>(last_checked_time));

  if (!weave_service->SetStateProperties(kWeaveComponent, state, nullptr)) {
    LOG(ERROR) << "Failed to update _updater state.";
  }
}

void WeaveService::OnCheckForUpdates(std::unique_ptr<weaved::Command> command) {
  brillo::ErrorPtr error;
  if (!delegate_->OnCheckForUpdates(&error)) {
    command->AbortWithCustomError(error.get(), nullptr);
    return;
  }
  command->Complete({}, nullptr);
}

void WeaveService::OnTrackChannel(std::unique_ptr<weaved::Command> command) {
  string channel = command->GetParameter<string>("channel");
  brillo::ErrorPtr error;
  if (!delegate_->OnTrackChannel(channel, &error)) {
    command->AbortWithCustomError(error.get(), nullptr);
    return;
  }
  command->Complete({}, nullptr);
}

}  // namespace chromeos_update_engine
