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

#ifndef UPDATE_ENGINE_WEAVE_SERVICE_INTERFACE_H_
#define UPDATE_ENGINE_WEAVE_SERVICE_INTERFACE_H_

#include <string>

#include <brillo/errors/error.h>

#include "update_engine/client_library/include/update_engine/update_status.h"
#include "update_engine/service_observer_interface.h"

namespace chromeos_update_engine {

// A WeaveServiceInterface instance allows to register the daemon with weaved,
// handle commands and update the weave status. This class only handles the
// registration with weaved and the connection, the actual work to handle the
// commands is implemented by the DelegateInterface, which will be called from
// this class.
class WeaveServiceInterface : public ServiceObserverInterface {
 public:
  // The delegate class that actually handles the command execution from
  class DelegateInterface {
   public:
    virtual ~DelegateInterface() = default;

    virtual bool OnCheckForUpdates(brillo::ErrorPtr* error) = 0;

    virtual bool OnTrackChannel(const std::string& channel,
                                brillo::ErrorPtr* error) = 0;

    // Return the current status.
    virtual bool GetWeaveState(int64_t* last_checked_time,
                               double* progress,
                               update_engine::UpdateStatus* update_status,
                               std::string* current_channel,
                               std::string* tracking_channel) = 0;
  };

  virtual ~WeaveServiceInterface() = default;

 protected:
  WeaveServiceInterface() = default;
};

}  // namespace chromeos_update_engine

#endif  // UPDATE_ENGINE_WEAVE_SERVICE_INTERFACE_H_
