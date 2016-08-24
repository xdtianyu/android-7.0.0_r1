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

#include "tpm_manager/server/tpm2_nvram_impl.h"

#include <memory>
#include <string>

#include <base/logging.h>
#include <trunks/error_codes.h>
#include <trunks/tpm_constants.h>
#include <trunks/tpm_utility.h>
#include <trunks/trunks_factory_impl.h>

namespace tpm_manager {

using trunks::GetErrorString;
using trunks::TPM_RC;
using trunks::TPM_RC_SUCCESS;

Tpm2NvramImpl::Tpm2NvramImpl(LocalDataStore* local_data_store)
    : trunks_factory_(new trunks::TrunksFactoryImpl()),
      local_data_store_(local_data_store),
      initialized_(false),
      trunks_session_(trunks_factory_->GetHmacSession()),
      trunks_utility_(trunks_factory_->GetTpmUtility()) {}

Tpm2NvramImpl::Tpm2NvramImpl(std::unique_ptr<trunks::TrunksFactory> factory,
                             LocalDataStore* local_data_store)
    : trunks_factory_(std::move(factory)),
      local_data_store_(local_data_store),
      initialized_(false),
      trunks_session_(trunks_factory_->GetHmacSession()),
      trunks_utility_(trunks_factory_->GetTpmUtility()) {}

bool Tpm2NvramImpl::Initialize() {
  if (initialized_) {
    return true;
  }
  TPM_RC result = trunks_utility_->StartSession(trunks_session_.get());
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error starting an authorization session with trunks: "
               << GetErrorString(result);
    return false;
  }
  LocalData local_data;
  if (!local_data_store_->Read(&local_data)) {
    LOG(ERROR) << "Error reading local tpm data.";
    return false;
  }
  if (!local_data.owner_password().empty()) {
    owner_password_.assign(local_data.owner_password());
    initialized_ = true;
  }
  return true;
}

bool Tpm2NvramImpl::InitializeWithOwnerPassword() {
  if (!Initialize()) {
    return false;
  }
  if (owner_password_.empty()) {
    LOG(ERROR) << "Error owner password not available.";
    return false;
  }
  trunks_session_->SetEntityAuthorizationValue(owner_password_);
  return true;
}

bool Tpm2NvramImpl::DefineNvram(uint32_t index, size_t length) {
  if (!InitializeWithOwnerPassword()) {
    return false;
  }
  TPM_RC result = trunks_utility_->DefineNVSpace(
      index, length, trunks_session_->GetDelegate());
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error defining nvram space: " << GetErrorString(result);
    return false;
  }
  return true;
}

bool Tpm2NvramImpl::DestroyNvram(uint32_t index) {
  if (!InitializeWithOwnerPassword()) {
    return false;
  }
  TPM_RC result = trunks_utility_->DestroyNVSpace(
      index, trunks_session_->GetDelegate());
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error destroying nvram space:" << GetErrorString(result);
    return false;
  }
  return true;
}

bool Tpm2NvramImpl::WriteNvram(uint32_t index, const std::string& data) {
  if (!InitializeWithOwnerPassword()) {
    return false;
  }
  TPM_RC result = trunks_utility_->WriteNVSpace(index,
                                                0,  // offset
                                                data,
                                                trunks_session_->GetDelegate());
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error writing to nvram space: " << GetErrorString(result);
    return false;
  }
  result = trunks_utility_->LockNVSpace(index, trunks_session_->GetDelegate());
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error locking nvram space: " << GetErrorString(result);
    return false;
  }
  return true;
}

bool Tpm2NvramImpl::ReadNvram(uint32_t index, std::string* data) {
  if (!Initialize()) {
    return false;
  }
  size_t nvram_size;
  if (!GetNvramSize(index, &nvram_size)) {
    LOG(ERROR) << "Error getting size of nvram space.";
    return false;
  }
  trunks_session_->SetEntityAuthorizationValue("");
  TPM_RC result = trunks_utility_->ReadNVSpace(index,
                                               0,  // offset
                                               nvram_size,
                                               data,
                                               trunks_session_->GetDelegate());
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error reading nvram space: " << GetErrorString(result);
    return false;
  }
  return true;
}

bool Tpm2NvramImpl::IsNvramDefined(uint32_t index, bool* defined) {
  trunks::TPMS_NV_PUBLIC nvram_public;
  TPM_RC result = trunks_utility_->GetNVSpacePublicArea(index, &nvram_public);
  if (trunks::GetFormatOneError(result) == trunks::TPM_RC_HANDLE) {
    *defined = false;
  } else if (result == TPM_RC_SUCCESS) {
    *defined = true;
  } else {
    LOG(ERROR) << "Error reading NV space for index " << index
               << " with error: " << GetErrorString(result);
    return false;
  }
  return true;
}

bool Tpm2NvramImpl::IsNvramLocked(uint32_t index, bool* locked) {
  trunks::TPMS_NV_PUBLIC nvram_public;
  TPM_RC result = trunks_utility_->GetNVSpacePublicArea(index, &nvram_public);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error reading NV space for index " << index
               << " with error: " << GetErrorString(result);
    return false;
  }
  *locked = ((nvram_public.attributes & trunks::TPMA_NV_WRITELOCKED) != 0);
  return true;
}

bool Tpm2NvramImpl::GetNvramSize(uint32_t index, size_t* size) {
  trunks::TPMS_NV_PUBLIC nvram_public;
  TPM_RC result = trunks_utility_->GetNVSpacePublicArea(index, &nvram_public);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error reading NV space for index " << index
               << " with error: " << GetErrorString(result);
    return false;
  }
  *size = nvram_public.data_size;
  return true;
}

}  // namespace tpm_manager
