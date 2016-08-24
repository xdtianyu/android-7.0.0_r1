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

#include "update_engine/hardware_android.h"

#include <base/files/file_util.h>
#include <brillo/make_unique_ptr.h>
#include <cutils/properties.h>

#include "update_engine/common/hardware.h"
#include "update_engine/common/platform_constants.h"

using std::string;

namespace chromeos_update_engine {

namespace hardware {

// Factory defined in hardware.h.
std::unique_ptr<HardwareInterface> CreateHardware() {
  return brillo::make_unique_ptr(new HardwareAndroid());
}

}  // namespace hardware

// In Android there are normally three kinds of builds: eng, userdebug and user.
// These builds target respectively a developer build, a debuggable version of
// the final product and the pristine final product the end user will run.
// Apart from the ro.build.type property name, they differ in the following
// properties that characterize the builds:
// * eng builds: ro.secure=0 and ro.debuggable=1
// * userdebug builds: ro.secure=1 and ro.debuggable=1
// * user builds: ro.secure=1 and ro.debuggable=0
//
// See IsOfficialBuild() and IsNormalMode() for the meaning of these options in
// Android.

bool HardwareAndroid::IsOfficialBuild() const {
  // We run an official build iff ro.secure == 1, because we expect the build to
  // behave like the end user product and check for updates. Note that while
  // developers are able to build "official builds" by just running "make user",
  // that will only result in a more restrictive environment. The important part
  // is that we don't produce and push "non-official" builds to the end user.
  //
  // In case of a non-bool value, we take the most restrictive option and
  // assume we are in an official-build.
  return property_get_bool("ro.secure", 1) != 0;
}

bool HardwareAndroid::IsNormalBootMode() const {
  // We are running in "dev-mode" iff ro.debuggable == 1. In dev-mode the
  // update_engine will allow extra developers options, such as providing a
  // different update URL. In case of error, we assume the build is in
  // normal-mode.
  return property_get_bool("ro.debuggable", 0) != 1;
}

bool HardwareAndroid::IsOOBEComplete(base::Time* out_time_of_oobe) const {
  LOG(WARNING) << "STUB: Assuming OOBE is complete.";
  if (out_time_of_oobe)
    *out_time_of_oobe = base::Time();
  return true;
}

string HardwareAndroid::GetHardwareClass() const {
  LOG(WARNING) << "STUB: GetHardwareClass().";
  return "ANDROID";
}

string HardwareAndroid::GetFirmwareVersion() const {
  LOG(WARNING) << "STUB: GetFirmwareVersion().";
  return "0";
}

string HardwareAndroid::GetECVersion() const {
  LOG(WARNING) << "STUB: GetECVersion().";
  return "0";
}

int HardwareAndroid::GetPowerwashCount() const {
  LOG(WARNING) << "STUB: Assuming no factory reset was performed.";
  return 0;
}

bool HardwareAndroid::GetNonVolatileDirectory(base::FilePath* path) const {
  base::FilePath local_path(constants::kNonVolatileDirectory);
  if (!base::PathExists(local_path)) {
    LOG(ERROR) << "Non-volatile directory not found: " << local_path.value();
    return false;
  }
  *path = local_path;
  return true;
}

bool HardwareAndroid::GetPowerwashSafeDirectory(base::FilePath* path) const {
  // On Android, we don't have a directory persisted across powerwash.
  return false;
}

}  // namespace chromeos_update_engine
