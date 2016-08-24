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

#include "trunks/tpm_simulator_handle.h"

#include <unistd.h>

#if defined(USE_SIMULATOR)
extern "C" {
#include <tpm2/TpmBuildSwitches.h>
#include <tpm2/_TPM_Init_fp.h>
#include <tpm2/ExecCommand_fp.h>
#include <tpm2/Manufacture_fp.h>
#include <tpm2/Platform.h>
}  // extern "C"
#endif  // USE_SIMULATOR

#include <base/callback.h>
#include <base/logging.h>
#include <base/stl_util.h>

#include "trunks/error_codes.h"

namespace trunks {

TpmSimulatorHandle::TpmSimulatorHandle() {}

TpmSimulatorHandle::~TpmSimulatorHandle() {}

bool TpmSimulatorHandle::Init() {
#if defined(USE_SIMULATOR)
  // Initialize TPM.
  CHECK_EQ(chdir("/data/misc/trunksd"), 0);
  _plat__Signal_PowerOn();
  _TPM_Init();
  _plat__SetNvAvail();
  CHECK_EQ(TPM_Manufacture(TRUE), 0);
  LOG(INFO) << "Simulator initialized.";
#else
  LOG(FATAL) << "Simulator not configured.";
#endif
  return true;
}

void TpmSimulatorHandle::SendCommand(const std::string& command,
                            const ResponseCallback& callback) {
  callback.Run(SendCommandAndWait(command));
}

std::string TpmSimulatorHandle::SendCommandAndWait(const std::string& command) {
#if defined(USE_SIMULATOR)
  unsigned int response_size;
  unsigned char* response;
  std::string mutable_command(command);
  ExecuteCommand(command.size(), reinterpret_cast<unsigned char*>(
                                     string_as_array(&mutable_command)),
                 &response_size, &response);
  return std::string(reinterpret_cast<char*>(response), response_size);
#else
  return CreateErrorResponse(TCTI_RC_GENERAL_FAILURE);
#endif
}

}  // namespace trunks
