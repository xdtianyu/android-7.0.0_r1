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

#include "update_engine/image_properties.h"

#include <string>

#include <base/logging.h>
#include <brillo/osrelease_reader.h>

#include "update_engine/common/boot_control_interface.h"
#include "update_engine/common/constants.h"
#include "update_engine/common/platform_constants.h"
#include "update_engine/common/prefs_interface.h"
#include "update_engine/system_state.h"

namespace chromeos_update_engine {

namespace {

// Build time properties name used in Brillo.
const char kProductId[] = "product_id";
const char kProductVersion[] = "product_version";

// Prefs used to store the target channel and powerwash settings.
const char kPrefsImgPropChannelName[] = "img-prop-channel-name";
const char kPrefsImgPropPowerwashAllowed[] = "img-prop-powerwash-allowed";

std::string GetStringWithDefault(const brillo::OsReleaseReader& osrelease,
                                 const std::string& key,
                                 const std::string& default_value) {
  std::string result;
  if (osrelease.GetString(key, &result))
    return result;
  LOG(INFO) << "Cannot load ImageProperty " << key << ", using default value "
            << default_value;
  return default_value;
}

}  // namespace

namespace test {
void SetImagePropertiesRootPrefix(const char* /* test_root_prefix */) {}
}  // namespace test

ImageProperties LoadImageProperties(SystemState* system_state) {
  ImageProperties result;

  brillo::OsReleaseReader osrelease;
  osrelease.Load();
  result.product_id = GetStringWithDefault(
      osrelease, kProductId, "developer-boards:brillo-starter-board");
  result.canary_product_id = result.product_id;
  result.version = GetStringWithDefault(osrelease, kProductVersion, "0.0.0.0");

  result.board = "brillo";

  // Brillo images don't have a channel assigned. We stored the name of the
  // channel where we got the image from in prefs at the time of the update, so
  // we use that as the current channel if available. During provisioning, there
  // is no value assigned, so we default to the "stable-channel".
  std::string current_channel_key =
      kPrefsChannelOnSlotPrefix +
      std::to_string(system_state->boot_control()->GetCurrentSlot());
  std::string current_channel;
  if (!system_state->prefs()->Exists(current_channel_key) ||
      !system_state->prefs()->GetString(current_channel_key, &current_channel))
    current_channel = "stable-channel";
  result.current_channel = current_channel;

  // Brillo only supports the official omaha URL.
  result.omaha_url = constants::kOmahaDefaultProductionURL;

  return result;
}

MutableImageProperties LoadMutableImageProperties(SystemState* system_state) {
  MutableImageProperties result;
  PrefsInterface* const prefs = system_state->prefs();
  if (!prefs->GetString(kPrefsImgPropChannelName, &result.target_channel))
    result.target_channel.clear();
  if (!prefs->GetBoolean(kPrefsImgPropPowerwashAllowed,
                         &result.is_powerwash_allowed)) {
    result.is_powerwash_allowed = false;
  }
  return result;
}

bool StoreMutableImageProperties(SystemState* system_state,
                                 const MutableImageProperties& properties) {
  PrefsInterface* const prefs = system_state->prefs();
  return (
      prefs->SetString(kPrefsImgPropChannelName, properties.target_channel) &&
      prefs->SetBoolean(kPrefsImgPropPowerwashAllowed,
                        properties.is_powerwash_allowed));
}

}  // namespace chromeos_update_engine
