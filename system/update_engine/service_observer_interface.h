//
// Copyright (C) 2016 The Android Open Source Project
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

#ifndef UPDATE_ENGINE_SERVICE_OBSERVER_INTERFACE_H_
#define UPDATE_ENGINE_SERVICE_OBSERVER_INTERFACE_H_

#include <memory>
#include <string>

#include "update_engine/client_library/include/update_engine/update_status.h"
#include "update_engine/common/error_code.h"

namespace chromeos_update_engine {

class ServiceObserverInterface {
 public:
  virtual ~ServiceObserverInterface() = default;

  // Called whenever the value of these parameters changes. For |progress|
  // value changes, this method will be called only if it changes significantly.
  virtual void SendStatusUpdate(int64_t last_checked_time,
                                double progress,
                                update_engine::UpdateStatus status,
                                const std::string& new_version,
                                int64_t new_size) = 0;

  // Called whenever an update attempt is completed.
  virtual void SendPayloadApplicationComplete(ErrorCode error_code) = 0;

  // Called whenever the channel we are tracking changes.
  virtual void SendChannelChangeUpdate(const std::string& tracking_channel) = 0;

 protected:
  ServiceObserverInterface() = default;
};

}  // namespace chromeos_update_engine

#endif  // UPDATE_ENGINE_SERVICE_OBSERVER_INTERFACE_H_
