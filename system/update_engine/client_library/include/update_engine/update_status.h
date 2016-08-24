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

#ifndef UPDATE_ENGINE_CLIENT_LIBRARY_INCLUDE_UPDATE_ENGINE_UPDATE_STATUS_H_
#define UPDATE_ENGINE_CLIENT_LIBRARY_INCLUDE_UPDATE_ENGINE_UPDATE_STATUS_H_

namespace update_engine {

enum class UpdateStatus {
  IDLE = 0,
  CHECKING_FOR_UPDATE,
  UPDATE_AVAILABLE,
  DOWNLOADING,
  VERIFYING,
  FINALIZING,
  UPDATED_NEED_REBOOT,
  REPORTING_ERROR_EVENT,
  ATTEMPTING_ROLLBACK,
  DISABLED,
};

}  // namespace update_engine

#endif  // UPDATE_ENGINE_CLIENT_LIBRARY_INCLUDE_UPDATE_ENGINE_UPDATE_STATUS_H_
