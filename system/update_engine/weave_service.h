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

#ifndef UPDATE_ENGINE_WEAVE_SERVICE_H_
#define UPDATE_ENGINE_WEAVE_SERVICE_H_

#include <memory>
#include <string>

#include <base/memory/weak_ptr.h>
#include <libweaved/command.h>
#include <libweaved/service.h>

#include "update_engine/weave_service_interface.h"

namespace chromeos_update_engine {

class WeaveService : public WeaveServiceInterface {
 public:
  WeaveService() = default;
  ~WeaveService() override = default;

  bool Init(DelegateInterface* delegate);

  // ServiceObserverInterface overrides.
  void SendStatusUpdate(int64_t last_checked_time,
                        double progress,
                        update_engine::UpdateStatus status,
                        const std::string& new_version,
                        int64_t new_size) override;
  void SendChannelChangeUpdate(const std::string& tracking_channel) override;
  void SendPayloadApplicationComplete(ErrorCode error_code) override {}

 private:
  // Force a weave update.
  void UpdateWeaveState();

  void OnWeaveServiceConnected(const std::weak_ptr<weaved::Service>& service);

  // Weave command handlers. These are called from the message loop whenever a
  // command is received and dispatch the synchronous call to the |delegate_|.
  void OnCheckForUpdates(std::unique_ptr<weaved::Command> cmd);
  void OnTrackChannel(std::unique_ptr<weaved::Command> cmd);

  WeaveServiceInterface::DelegateInterface* delegate_{nullptr};

  std::unique_ptr<weaved::Service::Subscription> weave_service_subscription_;
  std::weak_ptr<weaved::Service> weave_service_;
};

}  // namespace chromeos_update_engine

#endif  // UPDATE_ENGINE_WEAVE_SERVICE_H_
