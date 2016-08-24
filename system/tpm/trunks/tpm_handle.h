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

#ifndef TRUNKS_TPM_HANDLE_H_
#define TRUNKS_TPM_HANDLE_H_

#include "trunks/command_transceiver.h"

#include <string>

#include "trunks/error_codes.h"

namespace trunks {

// Sends commands to a TPM device via a handle to /dev/tpm0. All commands are
// sent synchronously. The SendCommand method is supported but does not return
// until a response is received and the callback has been called. Command and
// response data are opaque to this class; it performs no validation.
//
// Example:
//   TpmHandle handle;
//   if (!handle.Init()) {...}
//   std::string response = handle.SendCommandAndWait(command);
class TpmHandle : public CommandTransceiver  {
 public:
  TpmHandle();
  ~TpmHandle() override;

  // Initializes a TpmHandle instance. This method must be called successfully
  // before any other method. Returns true on success.
  bool Init() override;

  // CommandTranceiver methods.
  void SendCommand(const std::string& command,
                   const ResponseCallback& callback) override;
  std::string SendCommandAndWait(const std::string& command) override;

 private:
  // Writes a |command| to /dev/tpm0 and reads the |response|. Returns
  // TPM_RC_SUCCESS on success.
  TPM_RC SendCommandInternal(const std::string& command, std::string* response);

  int fd_;  // A file descriptor for /dev/tpm0.

  DISALLOW_COPY_AND_ASSIGN(TpmHandle);
};

}  // namespace trunks

#endif  // TRUNKS_TPM_HANDLE_H_
