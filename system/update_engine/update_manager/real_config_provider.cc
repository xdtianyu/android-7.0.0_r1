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

#include "update_engine/update_manager/real_config_provider.h"

#include <base/files/file_path.h>
#include <base/logging.h>
#include <brillo/key_value_store.h>

#include "update_engine/common/constants.h"
#include "update_engine/common/utils.h"
#include "update_engine/update_manager/generic_variables.h"

using brillo::KeyValueStore;

namespace {

const char* kConfigFilePath = "/etc/update_manager.conf";

// Config options:
const char* kConfigOptsIsOOBEEnabled = "is_oobe_enabled";

}  // namespace

namespace chromeos_update_manager {

bool RealConfigProvider::Init() {
  KeyValueStore store;

  if (hardware_->IsNormalBootMode()) {
    store.Load(base::FilePath(root_prefix_ + kConfigFilePath));
  } else {
    if (store.Load(base::FilePath(root_prefix_ +
                                  chromeos_update_engine::kStatefulPartition +
                                  kConfigFilePath))) {
      LOG(INFO) << "UpdateManager Config loaded from stateful partition.";
    } else {
      store.Load(base::FilePath(root_prefix_ + kConfigFilePath));
    }
  }

  bool is_oobe_enabled;
  if (!store.GetBoolean(kConfigOptsIsOOBEEnabled, &is_oobe_enabled))
    is_oobe_enabled = true;  // Default value.
  var_is_oobe_enabled_.reset(
      new ConstCopyVariable<bool>(kConfigOptsIsOOBEEnabled, is_oobe_enabled));

  return true;
}

}  // namespace chromeos_update_manager
