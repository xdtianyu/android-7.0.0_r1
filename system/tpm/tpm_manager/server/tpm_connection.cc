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

#include "tpm_manager/server/tpm_connection.h"

#include <base/logging.h>
#include <base/stl_util.h>
#include <base/threading/platform_thread.h>
#include <base/time/time.h>
#include <trousers/tss.h>
#include <trousers/trousers.h>  // NOLINT(build/include_alpha)

#include "tpm_manager/server/tpm_util.h"

namespace {

const int kTpmConnectRetries = 10;
const int kTpmConnectIntervalMs = 100;

}  // namespace

namespace tpm_manager {

TSS_HCONTEXT TpmConnection::GetContext() {
  if (!ConnectContextIfNeeded()) {
    return 0;
  }
  return context_.value();
}

TSS_HTPM TpmConnection::GetTpm() {
  if (!ConnectContextIfNeeded()) {
    return 0;
  }
  TSS_RESULT result;
  TSS_HTPM tpm_handle;
  if (TPM_ERROR(result = Tspi_Context_GetTpmObject(context_.value(),
                                                   &tpm_handle))) {
    TPM_LOG(ERROR, result) << "Error getting a handle to the TPM.";
    return 0;
  }
  return tpm_handle;
}

TSS_HTPM TpmConnection::GetTpmWithAuth(const std::string& owner_password) {
  TSS_HTPM tpm_handle = GetTpm();
  if (tpm_handle == 0) {
    return 0;
  }
  TSS_RESULT result;
  TSS_HPOLICY tpm_usage_policy;
  if (TPM_ERROR(result = Tspi_GetPolicyObject(tpm_handle,
                                              TSS_POLICY_USAGE,
                                              &tpm_usage_policy))) {
    TPM_LOG(ERROR, result) << "Error calling Tspi_GetPolicyObject";
    return false;
  }
  if (TPM_ERROR(result = Tspi_Policy_SetSecret(
      tpm_usage_policy,
      TSS_SECRET_MODE_PLAIN,
      owner_password.size(),
      reinterpret_cast<BYTE *>(const_cast<char*>(owner_password.data()))))) {
    TPM_LOG(ERROR, result) << "Error calling Tspi_Policy_SetSecret";
    return false;
  }
  return tpm_handle;
}

bool TpmConnection::ConnectContextIfNeeded() {
  if (context_.value() != 0) {
    return true;
  }
  TSS_RESULT result;
  if (TPM_ERROR(result = Tspi_Context_Create(context_.ptr()))) {
    TPM_LOG(ERROR, result) << "Error connecting to TPM.";
    return false;
  }
  // We retry on failure. It might be that tcsd is starting up.
  for (int i = 0; i < kTpmConnectRetries; i++) {
    if (TPM_ERROR(result = Tspi_Context_Connect(context_, nullptr))) {
      if (ERROR_CODE(result) == TSS_E_COMM_FAILURE) {
        base::PlatformThread::Sleep(
            base::TimeDelta::FromMilliseconds(kTpmConnectIntervalMs));
      } else {
        TPM_LOG(ERROR, result) << "Error connecting to TPM.";
        return false;
      }
    } else {
      break;
    }
  }
  return (context_.value() != 0);
}

}  // namespace tpm_manager
