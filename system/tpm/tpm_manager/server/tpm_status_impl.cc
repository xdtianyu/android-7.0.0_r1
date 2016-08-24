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

#include "tpm_manager/server/tpm_status_impl.h"

#include <vector>

#include <base/logging.h>
#include <trousers/tss.h>
#include <trousers/trousers.h>  // NOLINT(build/include_alpha)

namespace tpm_manager {

// Minimum size of TPM_DA_INFO struct.
const size_t kMinimumDaInfoSize = 21;

bool TpmStatusImpl::IsTpmEnabled() {
  if (!is_enable_initialized_) {
    RefreshOwnedEnabledInfo();
  }
  return is_enabled_;
}

bool TpmStatusImpl::IsTpmOwned() {
  if (!is_owned_) {
    RefreshOwnedEnabledInfo();
  }
  return is_owned_;
}

bool TpmStatusImpl::GetDictionaryAttackInfo(int* counter,
                                            int* threshold,
                                            bool* lockout,
                                            int* seconds_remaining) {
  std::string capability_data;
  if (!GetCapability(TSS_TPMCAP_DA_LOGIC, TPM_ET_KEYHANDLE,
                     &capability_data, nullptr) ||
      capability_data.size() < kMinimumDaInfoSize) {
    LOG(ERROR) << "Error getting tpm capability data.";
    return false;
  }
  if (static_cast<uint16_t>(capability_data[1]) == TPM_TAG_DA_INFO) {
    TPM_DA_INFO da_info;
    uint64_t offset = 0;
    std::vector<BYTE> bytes(capability_data.begin(), capability_data.end());
    Trspi_UnloadBlob_DA_INFO(&offset, bytes.data(), &da_info);
    if (counter) { *counter = da_info.currentCount; }
    if (threshold) { *threshold = da_info.thresholdCount; }
    if (lockout) { *lockout = (da_info.state == TPM_DA_STATE_ACTIVE); }
    if (seconds_remaining) { *seconds_remaining = da_info.actionDependValue; }
  }
  return true;
}

void TpmStatusImpl::RefreshOwnedEnabledInfo() {
  TSS_RESULT result;
  std::string capability_data;
  if (!GetCapability(TSS_TPMCAP_PROPERTY, TSS_TPMCAP_PROP_OWNER,
                     &capability_data, &result)) {
    if (ERROR_CODE(result) == TPM_E_DISABLED) {
      is_enable_initialized_ = true;
      is_enabled_ = false;
    }
  } else {
    is_enable_initialized_ = true;
    is_enabled_ = true;
    // |capability_data| should be populated with a TSS_BOOL which is true iff
    // the Tpm is owned.
    if (capability_data.size() != sizeof(TSS_BOOL)) {
      LOG(ERROR) << "Error refreshing Tpm ownership information.";
      return;
    }
    is_owned_ = (capability_data[0] != 0);
  }
}

bool TpmStatusImpl::GetCapability(uint32_t capability,
                                  uint32_t sub_capability,
                                  std::string* data,
                                  TSS_RESULT* tpm_result) {
  CHECK(data);
  TSS_HTPM tpm_handle = tpm_connection_.GetTpm();
  if (tpm_handle == 0) {
    return false;
  }
  uint32_t length = 0;
  trousers::ScopedTssMemory buf(tpm_connection_.GetContext());
  TSS_RESULT result = Tspi_TPM_GetCapability(
      tpm_handle, capability, sizeof(uint32_t),
      reinterpret_cast<BYTE*>(&sub_capability),
      &length, buf.ptr());
  if (tpm_result) {
    *tpm_result = result;
  }
  if (TPM_ERROR(result)) {
    LOG(ERROR) << "Error getting TPM capability data.";
    return false;
  }
  data->assign(buf.value(), buf.value() + length);
  return true;
}

}  // namespace tpm_manager
