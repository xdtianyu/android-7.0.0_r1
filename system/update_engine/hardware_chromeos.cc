//
// Copyright (C) 2013 The Android Open Source Project
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

#include "update_engine/hardware_chromeos.h"

#include <base/files/file_util.h>
#include <base/logging.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/string_util.h>
#include <brillo/make_unique_ptr.h>
#include <vboot/crossystem.h>

extern "C" {
#include "vboot/vboot_host.h"
}

#include "update_engine/common/hardware.h"
#include "update_engine/common/hwid_override.h"
#include "update_engine/common/platform_constants.h"
#include "update_engine/common/subprocess.h"
#include "update_engine/common/utils.h"

using std::string;
using std::vector;

namespace {

const char kOOBECompletedMarker[] = "/home/chronos/.oobe_completed";

// The stateful directory used by update_engine to store powerwash-safe files.
// The files stored here must be whitelisted in the powerwash scripts.
const char kPowerwashSafeDirectory[] =
    "/mnt/stateful_partition/unencrypted/preserve";

// The powerwash_count marker file contains the number of times the device was
// powerwashed. This value is incremented by the clobber-state script when
// a powerwash is performed.
const char kPowerwashCountMarker[] = "powerwash_count";

}  // namespace

namespace chromeos_update_engine {

namespace hardware {

// Factory defined in hardware.h.
std::unique_ptr<HardwareInterface> CreateHardware() {
  return brillo::make_unique_ptr(new HardwareChromeOS());
}

}  // namespace hardware

bool HardwareChromeOS::IsOfficialBuild() const {
  return VbGetSystemPropertyInt("debug_build") == 0;
}

bool HardwareChromeOS::IsNormalBootMode() const {
  bool dev_mode = VbGetSystemPropertyInt("devsw_boot") != 0;
  return !dev_mode;
}

bool HardwareChromeOS::IsOOBEComplete(base::Time* out_time_of_oobe) const {
  struct stat statbuf;
  if (stat(kOOBECompletedMarker, &statbuf) != 0) {
    if (errno != ENOENT) {
      PLOG(ERROR) << "Error getting information about "
                  << kOOBECompletedMarker;
    }
    return false;
  }

  if (out_time_of_oobe != nullptr)
    *out_time_of_oobe = base::Time::FromTimeT(statbuf.st_mtime);
  return true;
}

static string ReadValueFromCrosSystem(const string& key) {
  char value_buffer[VB_MAX_STRING_PROPERTY];

  const char* rv = VbGetSystemPropertyString(key.c_str(), value_buffer,
                                             sizeof(value_buffer));
  if (rv != nullptr) {
    string return_value(value_buffer);
    base::TrimWhitespaceASCII(return_value, base::TRIM_ALL, &return_value);
    return return_value;
  }

  LOG(ERROR) << "Unable to read crossystem key " << key;
  return "";
}

string HardwareChromeOS::GetHardwareClass() const {
  if (USE_HWID_OVERRIDE) {
    return HwidOverride::Read(base::FilePath("/"));
  }
  return ReadValueFromCrosSystem("hwid");
}

string HardwareChromeOS::GetFirmwareVersion() const {
  return ReadValueFromCrosSystem("fwid");
}

string HardwareChromeOS::GetECVersion() const {
  string input_line;
  int exit_code = 0;
  vector<string> cmd = {"/usr/sbin/mosys", "-k", "ec", "info"};

  bool success = Subprocess::SynchronousExec(cmd, &exit_code, &input_line);
  if (!success || exit_code) {
    LOG(ERROR) << "Unable to read ec info from mosys (" << exit_code << ")";
    return "";
  }

  return utils::ParseECVersion(input_line);
}

int HardwareChromeOS::GetPowerwashCount() const {
  int powerwash_count;
  base::FilePath marker_path = base::FilePath(kPowerwashSafeDirectory).Append(
      kPowerwashCountMarker);
  string contents;
  if (!utils::ReadFile(marker_path.value(), &contents))
    return -1;
  base::TrimWhitespaceASCII(contents, base::TRIM_TRAILING, &contents);
  if (!base::StringToInt(contents, &powerwash_count))
    return -1;
  return powerwash_count;
}

bool HardwareChromeOS::GetNonVolatileDirectory(base::FilePath* path) const {
  *path = base::FilePath(constants::kNonVolatileDirectory);
  return true;
}

bool HardwareChromeOS::GetPowerwashSafeDirectory(base::FilePath* path) const {
  *path = base::FilePath(kPowerwashSafeDirectory);
  return true;
}

}  // namespace chromeos_update_engine
