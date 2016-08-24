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

#include "trunks/tpm_handle.h"

#include <fcntl.h>
#include <unistd.h>

#include <base/callback.h>
#include <base/logging.h>
#include <base/posix/eintr_wrapper.h>

namespace {

const char kTpmDevice[] = "/dev/tpm0";
const uint32_t kTpmBufferSize = 4096;
const int kInvalidFileDescriptor = -1;

}  // namespace

namespace trunks {

TpmHandle::TpmHandle() : fd_(kInvalidFileDescriptor) {}

TpmHandle::~TpmHandle() {
  int result = IGNORE_EINTR(close(fd_));
  if (result == -1) {
    PLOG(ERROR) << "TPM: couldn't close " << kTpmDevice;
  }
  LOG(INFO) << "TPM: " << kTpmDevice << " closed successfully";
}

bool TpmHandle::Init() {
  if (fd_ != kInvalidFileDescriptor) {
    VLOG(1) << "Tpm already initialized.";
    return true;
  }
  fd_ = HANDLE_EINTR(open(kTpmDevice, O_RDWR));
  if (fd_ == kInvalidFileDescriptor) {
    PLOG(ERROR) << "TPM: Error opening tpm0 file descriptor at " << kTpmDevice;
    return false;
  }
  LOG(INFO) << "TPM: " << kTpmDevice << " opened successfully";
  return true;
}

void TpmHandle::SendCommand(const std::string& command,
                            const ResponseCallback& callback) {
  callback.Run(SendCommandAndWait(command));
}

std::string TpmHandle::SendCommandAndWait(const std::string& command) {
  std::string response;
  TPM_RC result = SendCommandInternal(command, &response);
  if (result != TPM_RC_SUCCESS) {
    response = CreateErrorResponse(result);
  }
  return response;
}

TPM_RC TpmHandle::SendCommandInternal(const std::string& command,
                                      std::string* response) {
  CHECK_NE(fd_, kInvalidFileDescriptor);
  int result = HANDLE_EINTR(write(fd_, command.data(), command.length()));
  if (result < 0) {
    PLOG(ERROR) << "TPM: Error writing to TPM handle.";
    return TRUNKS_RC_WRITE_ERROR;
  }
  if (static_cast<size_t>(result) != command.length()) {
    LOG(ERROR) << "TPM: Error writing to TPM handle: " << result << " vs "
               << command.length();
    return TRUNKS_RC_WRITE_ERROR;
  }
  char response_buf[kTpmBufferSize];
  result = HANDLE_EINTR(read(fd_, response_buf, kTpmBufferSize));
  if (result < 0) {
    PLOG(ERROR) << "TPM: Error reading from TPM handle.";
    return TRUNKS_RC_READ_ERROR;
  }
  response->assign(response_buf, static_cast<size_t>(result));
  return TPM_RC_SUCCESS;
}

}  // namespace trunks
