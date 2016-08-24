//
// Copyright (C) 2014 The Android Open Source Project
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

#include "update_engine/update_manager/real_system_provider.h"

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <vector>

#include <base/logging.h>
#include <base/strings/stringprintf.h>
#include <base/time/time.h>

#include "update_engine/common/utils.h"
#include "update_engine/update_manager/generic_variables.h"

using std::string;

namespace chromeos_update_manager {

bool RealSystemProvider::Init() {
  var_is_normal_boot_mode_.reset(
      new ConstCopyVariable<bool>("is_normal_boot_mode",
                                  hardware_->IsNormalBootMode()));

  var_is_official_build_.reset(
      new ConstCopyVariable<bool>("is_official_build",
                                  hardware_->IsOfficialBuild()));

  var_is_oobe_complete_.reset(
      new CallCopyVariable<bool>(
          "is_oobe_complete",
          base::Bind(&chromeos_update_engine::HardwareInterface::IsOOBEComplete,
                     base::Unretained(hardware_), nullptr)));

  var_num_slots_.reset(
      new ConstCopyVariable<unsigned int>(
          "num_slots", boot_control_->GetNumSlots()));

  return true;
}

}  // namespace chromeos_update_manager
