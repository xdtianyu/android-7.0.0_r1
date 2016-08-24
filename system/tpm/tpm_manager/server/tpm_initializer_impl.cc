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

#include "tpm_manager/server/tpm_initializer_impl.h"

#include <string>

#include <base/logging.h>
#include <base/stl_util.h>
#include <trousers/scoped_tss_type.h>

#include "tpm_manager/server/local_data_store.h"
#include "tpm_manager/server/tpm_connection.h"
#include "tpm_manager/common/tpm_manager_constants.h"
#include "tpm_manager/server/tpm_status.h"
#include "tpm_manager/server/tpm_util.h"

namespace {

const char kDefaultOwnerPassword[] = TSS_WELL_KNOWN_SECRET;
const size_t kDefaultPasswordSize = 20;
const int kMaxOwnershipTimeoutRetries = 5;
const char* kWellKnownSrkSecret = "well_known_srk_secret";

}  // namespace

namespace tpm_manager {

TpmInitializerImpl::TpmInitializerImpl(LocalDataStore* local_data_store,
                                       TpmStatus* tpm_status)
    : local_data_store_(local_data_store),
      tpm_status_(tpm_status) {}

bool TpmInitializerImpl::InitializeTpm() {
  if (tpm_status_->IsTpmOwned() && !TestTpmAuth(kDefaultOwnerPassword)) {
    // Tpm is already owned, so we do not need to do anything.
    VLOG(1) << "Tpm already owned.";
    return true;
  }
  TSS_HTPM tpm_handle = tpm_connection_.GetTpm();
  if (tpm_handle == 0) {
    return false;
  }
  if (!InitializeEndorsementKey(tpm_handle) ||
      !TakeOwnership(tpm_handle) ||
      !InitializeSrk(tpm_handle)) {
    return false;
  }
  std::string owner_password;
  if (!openssl_util_.GetRandomBytes(kDefaultPasswordSize, &owner_password)) {
    return false;
  }
  LocalData local_data;
  local_data.clear_owner_dependency();
  for (auto value: kInitialTpmOwnerDependencies) {
    local_data.add_owner_dependency(value);
  }
  local_data.set_owner_password(owner_password);
  if (!local_data_store_->Write(local_data)) {
    LOG(ERROR) << "Error saving local data.";
    return false;
  }
  if (!ChangeOwnerPassword(tpm_handle, owner_password)) {
    return false;
  }
  return true;
}

bool TpmInitializerImpl::InitializeEndorsementKey(TSS_HTPM tpm_handle) {
  trousers::ScopedTssKey local_key_handle(tpm_connection_.GetContext());
  TSS_RESULT result = Tspi_TPM_GetPubEndorsementKey(tpm_handle,
                                                    false,
                                                    nullptr,
                                                    local_key_handle.ptr());
  if (TPM_ERROR(result) == TPM_SUCCESS) {
    // In this case the EK already exists, so we can return true here.
    return true;
  }
  // At this point the EK does not exist, so we create it.
  TSS_FLAG init_flags = TSS_KEY_TYPE_LEGACY | TSS_KEY_SIZE_2048;
  if (TPM_ERROR(result = Tspi_Context_CreateObject(tpm_connection_.GetContext(),
                                                   TSS_OBJECT_TYPE_RSAKEY,
                                                   init_flags,
                                                   local_key_handle.ptr()))) {
    TPM_LOG(ERROR, result) << "Error calling Tspi_Context_CreateObject";
    return false;
  }
  if (TPM_ERROR(result = Tspi_TPM_CreateEndorsementKey(tpm_handle,
                                                       local_key_handle,
                                                       NULL))) {
    TPM_LOG(ERROR, result) << "Error calling Tspi_TPM_CreateEndorsementKey";
    return false;
  }
  return true;
}

bool TpmInitializerImpl::TakeOwnership(TSS_HTPM tpm_handle) {
  if (TestTpmAuth(kDefaultOwnerPassword)) {
    VLOG(1) << "The Tpm already has the default owner password.";
    return true;
  }
  TSS_RESULT result;
  trousers::ScopedTssKey srk_handle(tpm_connection_.GetContext());
  TSS_FLAG init_flags = TSS_KEY_TSP_SRK | TSS_KEY_AUTHORIZATION;
  if (TPM_ERROR(result = Tspi_Context_CreateObject(tpm_connection_.GetContext(),
                                                   TSS_OBJECT_TYPE_RSAKEY,
                                                   init_flags,
                                                   srk_handle.ptr()))) {
    TPM_LOG(ERROR, result) << "Error calling Tspi_Context_CreateObject";
    return false;
  }
  TSS_HPOLICY srk_usage_policy;
  if (TPM_ERROR(result = Tspi_GetPolicyObject(srk_handle,
                                              TSS_POLICY_USAGE,
                                              &srk_usage_policy))) {
    TPM_LOG(ERROR, result) << "Error calling Tspi_GetPolicyObject";
    return false;
  }
  if (TPM_ERROR(result = Tspi_Policy_SetSecret(
      srk_usage_policy,
      TSS_SECRET_MODE_PLAIN,
      strlen(kWellKnownSrkSecret),
      const_cast<BYTE *>(reinterpret_cast<const BYTE *>(
          kWellKnownSrkSecret))))) {
    TPM_LOG(ERROR, result) << "Error calling Tspi_Policy_SetSecret";
    return false;
  }
  // Tspi_TPM_TakeOwnership can potentailly take a long time to complete,
  // so we retry if there is a timeout in any layer. I chose 5, because the
  // longest TakeOwnership call that I have seen took ~2min, and the default
  // TSS timeout is 30s. This means that after 5 calls, it is quite likely that
  // this call will succeed.
  int retry_count = 0;
  do {
    result = Tspi_TPM_TakeOwnership(tpm_handle, srk_handle, 0);
    retry_count++;
  } while (((result == TDDL_E_TIMEOUT) ||
            (result == (TSS_LAYER_TDDL | TDDL_E_TIMEOUT)) ||
            (result == (TSS_LAYER_TDDL | TDDL_E_IOERROR))) &&
           (retry_count < kMaxOwnershipTimeoutRetries));
  if (result) {
    TPM_LOG(ERROR, result)
        << "Error calling Tspi_TPM_TakeOwnership, attempts: " << retry_count;
    return false;
  }
  return true;
}

bool TpmInitializerImpl::InitializeSrk(TSS_HTPM tpm_handle) {
  TSS_RESULT result;
  trousers::ScopedTssKey srk_handle(tpm_connection_.GetContext());
  TSS_UUID SRK_UUID = TSS_UUID_SRK;
  if (TPM_ERROR(result = Tspi_Context_LoadKeyByUUID(
      tpm_connection_.GetContext(),
      TSS_PS_TYPE_SYSTEM,
      SRK_UUID,
      srk_handle.ptr()))) {
    TPM_LOG(ERROR, result) << "Error calling Tspi_Context_LoadKeyByUUID";
    return false;
  }

  trousers::ScopedTssPolicy policy_handle(tpm_connection_.GetContext());
  if (TPM_ERROR(result = Tspi_Context_CreateObject(tpm_connection_.GetContext(),
                                                   TSS_OBJECT_TYPE_POLICY,
                                                   TSS_POLICY_USAGE,
                                                   policy_handle.ptr()))) {
    TPM_LOG(ERROR, result) << "Error calling Tspi_Context_CreateObject";
    return false;
  }
  BYTE new_password[0];
  if (TPM_ERROR(result = Tspi_Policy_SetSecret(policy_handle,
                                               TSS_SECRET_MODE_PLAIN,
                                               0,
                                               new_password))) {
    TPM_LOG(ERROR, result) << "Error calling Tspi_Policy_SetSecret";
    return false;
  }

  if (TPM_ERROR(result = Tspi_ChangeAuth(srk_handle,
                                         tpm_handle,
                                         policy_handle))) {
    TPM_LOG(ERROR, result) << "Error calling Tspi_ChangeAuth";
    return false;
  }
  TSS_BOOL is_srk_restricted = false;
  if (TPM_ERROR(result = Tspi_TPM_GetStatus(tpm_handle,
                                            TSS_TPMSTATUS_DISABLEPUBSRKREAD,
                                            &is_srk_restricted))) {
    TPM_LOG(ERROR, result) << "Error calling Tspi_TPM_GetStatus";
    return false;
  }
  // If the SRK is restricted, we unrestrict it.
  if (is_srk_restricted) {
    if (TPM_ERROR(result = Tspi_TPM_SetStatus(tpm_handle,
                                              TSS_TPMSTATUS_DISABLEPUBSRKREAD,
                                              false))) {
      TPM_LOG(ERROR, result) << "Error calling Tspi_TPM_SetStatus";
      return false;
    }
  }
  return true;
}

bool TpmInitializerImpl::ChangeOwnerPassword(
    TSS_HTPM tpm_handle, const std::string& owner_password) {
  TSS_RESULT result;
  trousers::ScopedTssPolicy policy_handle(tpm_connection_.GetContext());
  if (TPM_ERROR(result = Tspi_Context_CreateObject(tpm_connection_.GetContext(),
                                                   TSS_OBJECT_TYPE_POLICY,
                                                   TSS_POLICY_USAGE,
                                                   policy_handle.ptr()))) {
    TPM_LOG(ERROR, result) << "Error calling Tspi_Context_CreateObject";
    return false;
  }
  std::string mutable_owner_password(owner_password);
  if (TPM_ERROR(result = Tspi_Policy_SetSecret(policy_handle,
      TSS_SECRET_MODE_PLAIN,
      owner_password.size(),
      reinterpret_cast<BYTE *>(string_as_array(&mutable_owner_password))))) {
    TPM_LOG(ERROR, result) << "Error calling Tspi_Policy_SetSecret";
    return false;
  }

  if (TPM_ERROR(result = Tspi_ChangeAuth(tpm_handle, 0, policy_handle))) {
    TPM_LOG(ERROR, result) << "Error calling Tspi_ChangeAuth";
    return false;
  }

  return true;
}

bool TpmInitializerImpl::TestTpmAuth(const std::string& owner_password) {
  TSS_HTPM tpm_handle = tpm_connection_.GetTpmWithAuth(owner_password);
  if (tpm_handle == 0) {
    return false;
  }
  // Call Tspi_TPM_GetStatus to test the |owner_password| provided.
  TSS_RESULT result;
  TSS_BOOL current_status = false;
  if (TPM_ERROR(result = Tspi_TPM_GetStatus(tpm_handle,
                                            TSS_TPMSTATUS_DISABLED,
                                            &current_status))) {
    return false;
  }
  return true;
}

}  // namespace tpm_manager
