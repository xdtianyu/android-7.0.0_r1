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

#ifndef UPDATE_ENGINE_UPDATE_MANAGER_REAL_SYSTEM_PROVIDER_H_
#define UPDATE_ENGINE_UPDATE_MANAGER_REAL_SYSTEM_PROVIDER_H_

#include <memory>
#include <string>

#include "update_engine/common/boot_control_interface.h"
#include "update_engine/common/hardware_interface.h"
#include "update_engine/update_manager/system_provider.h"

namespace chromeos_update_manager {

// SystemProvider concrete implementation.
class RealSystemProvider : public SystemProvider {
 public:
  explicit RealSystemProvider(
      chromeos_update_engine::HardwareInterface* hardware,
      chromeos_update_engine::BootControlInterface* boot_control)
      : hardware_(hardware), boot_control_(boot_control) {}

  // Initializes the provider and returns whether it succeeded.
  bool Init();

  Variable<bool>* var_is_normal_boot_mode() override {
    return var_is_normal_boot_mode_.get();
  }

  Variable<bool>* var_is_official_build() override {
    return var_is_official_build_.get();
  }

  Variable<bool>* var_is_oobe_complete() override {
    return var_is_oobe_complete_.get();
  }

  Variable<unsigned int>* var_num_slots() override {
    return var_num_slots_.get();
  }

 private:
  std::unique_ptr<Variable<bool>> var_is_normal_boot_mode_;
  std::unique_ptr<Variable<bool>> var_is_official_build_;
  std::unique_ptr<Variable<bool>> var_is_oobe_complete_;
  std::unique_ptr<Variable<unsigned int>> var_num_slots_;

  chromeos_update_engine::HardwareInterface* hardware_;
  chromeos_update_engine::BootControlInterface* boot_control_;

  DISALLOW_COPY_AND_ASSIGN(RealSystemProvider);
};

}  // namespace chromeos_update_manager

#endif  // UPDATE_ENGINE_UPDATE_MANAGER_REAL_SYSTEM_PROVIDER_H_
