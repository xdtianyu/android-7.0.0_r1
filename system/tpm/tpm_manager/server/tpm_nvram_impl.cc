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

#include "tpm_manager/server/tpm_nvram_impl.h"

#include <arpa/inet.h>

#include <string>

#include <base/logging.h>
#include <base/stl_util.h>
#include <trousers/scoped_tss_type.h>

#include "tpm_manager/common/local_data.pb.h"
#include "tpm_manager/server/local_data_store.h"
#include "tpm_manager/server/tpm_util.h"

namespace {

// PCR0 at locality 1 is used to differentiate between developed and normal
// mode. Restricting nvram to the PCR0 value in locality 1 prevents nvram from
// persisting across mode switch.
const unsigned int kTpmBootPCR = 0;
const unsigned int kTpmPCRLocality = 1;

}  // namespace

namespace tpm_manager {

using trousers::ScopedTssMemory;
using trousers::ScopedTssNvStore;
using trousers::ScopedTssPcrs;

TpmNvramImpl::TpmNvramImpl(LocalDataStore* local_data_store)
    : local_data_store_(local_data_store) {}

bool TpmNvramImpl::DefineNvram(uint32_t index, size_t length) {
  ScopedTssNvStore nv_handle(tpm_connection_.GetContext());
  if (!(InitializeNvramHandle(&nv_handle, index) &&
        SetOwnerPolicy(&nv_handle))) {
    return false;
  }
  TSS_RESULT result;
  result = Tspi_SetAttribUint32(nv_handle, TSS_TSPATTRIB_NV_DATASIZE,
                                0, length);
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Could not set size on NVRAM object: " << length;
    return false;
  }
  // Restrict to only one write.
  result = Tspi_SetAttribUint32(nv_handle, TSS_TSPATTRIB_NV_PERMISSIONS,
                                0, TPM_NV_PER_WRITEDEFINE);
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Could not set PER_WRITEDEFINE on NVRAM object";
    return false;
  }
  // Restrict to writing only with owner authorization.
  result = Tspi_SetAttribUint32(nv_handle, TSS_TSPATTRIB_NV_PERMISSIONS,
                                0, TPM_NV_PER_OWNERWRITE);
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Could not set PER_OWNERWRITE on NVRAM object";
    return false;
  }
  ScopedTssPcrs pcr_handle(tpm_connection_.GetContext());
  if (!SetCompositePcr0(&pcr_handle)) {
    return false;
  }
  result = Tspi_NV_DefineSpace(nv_handle,
                               pcr_handle /* ReadPCRs restricted to PCR0 */,
                               pcr_handle /* WritePCRs restricted to PCR0 */);
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Could not define NVRAM space: " << index;
    return false;
  }
  return true;
}

bool TpmNvramImpl::DestroyNvram(uint32_t index) {
  bool defined;
  if (!IsNvramDefined(index, &defined)) {
    return false;
  }
  if (!defined) {
    // If the nvram space is not defined, we don't need to destroy it.
    return true;
  }
  ScopedTssNvStore nv_handle(tpm_connection_.GetContext());
  if (!(InitializeNvramHandle(&nv_handle, index) &&
        SetOwnerPolicy(&nv_handle))) {
    return false;
  }
  TSS_RESULT result = Tspi_NV_ReleaseSpace(nv_handle);
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Could not release NVRAM space: " << index;
    return false;
  }
  return true;
}

bool TpmNvramImpl::WriteNvram(uint32_t index, const std::string& data) {
  ScopedTssNvStore nv_handle(tpm_connection_.GetContext());
  if (!(InitializeNvramHandle(&nv_handle, index) &&
        SetOwnerPolicy(&nv_handle))) {
    return false;
  }
  TSS_RESULT result = Tspi_NV_WriteValue(
      nv_handle, 0 /* offset */, data.size(),
      reinterpret_cast<BYTE *>(const_cast<char*>(data.data())));
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Could not write to NVRAM space: " << index;
    return false;
  }
  return true;
}

bool TpmNvramImpl::ReadNvram(uint32_t index, std::string* data) {
  CHECK(data);
  TSS_RESULT result;
  ScopedTssNvStore nv_handle(tpm_connection_.GetContext());
  if (!InitializeNvramHandle(&nv_handle, index)) {
    return false;
  }
  size_t nvram_size;
  if (!GetNvramSize(index, &nvram_size)) {
    return false;
  }
  data->resize(nvram_size);
  // The Tpm1.2 Specification defines the maximum read size of 128 bytes.
  // Therefore we have to loop through the data returned.
  const size_t kMaxDataSize = 128;
  uint32_t offset = 0;
  while (offset < nvram_size) {
    uint32_t chunk_size = std::max(nvram_size - offset, kMaxDataSize);
    ScopedTssMemory space_data(tpm_connection_.GetContext());
    if ((result = Tspi_NV_ReadValue(nv_handle, offset, &chunk_size,
                                    space_data.ptr()))) {
      TPM_LOG(ERROR, result) << "Could not read from NVRAM space: " << index;
      return false;
    }
    if (!space_data.value()) {
      LOG(ERROR) << "No data read from NVRAM space: " << index;
      return false;
    }
    CHECK_LE((offset + chunk_size), data->size());
    data->replace(offset,
                  chunk_size,
                  reinterpret_cast<char*>(space_data.value()),
                  chunk_size);
    offset += chunk_size;
  }
  return true;
}

bool TpmNvramImpl::IsNvramDefined(uint32_t index, bool* defined) {
  CHECK(defined);
  uint32_t nv_list_data_length = 0;
  ScopedTssMemory nv_list_data(tpm_connection_.GetContext());
  TSS_RESULT result = Tspi_TPM_GetCapability(tpm_connection_.GetTpm(),
                                             TSS_TPMCAP_NV_LIST,
                                             0,
                                             NULL,
                                             &nv_list_data_length,
                                             nv_list_data.ptr());
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Error calling Tspi_TPM_GetCapability";
    return false;
  }
  // Walk the list and check if the index exists.
  uint32_t* nv_list = reinterpret_cast<uint32_t*>(nv_list_data.value());
  uint32_t nv_list_length = nv_list_data_length / sizeof(uint32_t);
  index = htonl(index);  // TPM data is network byte order.
  for (uint32_t i = 0; i < nv_list_length; ++i) {
    if (index == nv_list[i]) {
      *defined = true;
      return true;
    }
  }
  *defined = false;
  return true;
}

bool TpmNvramImpl::IsNvramLocked(uint32_t index, bool* locked) {
  CHECK(locked);
  uint32_t nv_index_data_length = 0;
  ScopedTssMemory nv_index_data(tpm_connection_.GetContext());
  TSS_RESULT result = Tspi_TPM_GetCapability(tpm_connection_.GetTpm(),
                                             TSS_TPMCAP_NV_INDEX,
                                             sizeof(index),
                                             reinterpret_cast<BYTE*>(&index),
                                             &nv_index_data_length,
                                             nv_index_data.ptr());
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Error calling Tspi_TPM_GetCapability";
    return false;
  }
  if (nv_index_data_length < (sizeof(uint32_t) + sizeof(TPM_BOOL))) {
    return false;
  }
  // TPM_NV_DATA_PUBLIC->bWriteDefine is the second to last element in the
  // struct.
  uint32_t* nv_data_public = reinterpret_cast<uint32_t*>(
                               nv_index_data.value() + nv_index_data_length -
                               (sizeof(uint32_t) + sizeof(TPM_BOOL)));
  *locked = (*nv_data_public != 0);
  return true;
}

bool TpmNvramImpl::GetNvramSize(uint32_t index, size_t* size) {
  CHECK(size);
  UINT32 nv_index_data_length = 0;
  ScopedTssMemory nv_index_data(tpm_connection_.GetContext());
  TSS_RESULT result = Tspi_TPM_GetCapability(tpm_connection_.GetTpm(),
                                             TSS_TPMCAP_NV_INDEX,
                                             sizeof(index),
                                             reinterpret_cast<BYTE*>(&index),
                                             &nv_index_data_length,
                                             nv_index_data.ptr());
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Error calling Tspi_TPM_GetCapability";
    return false;
  }
  if (nv_index_data_length < sizeof(uint32_t)) {
    return false;
  }
  // TPM_NV_DATA_PUBLIC->dataSize is the last element in the struct.
  uint32_t* nv_data_public = reinterpret_cast<uint32_t*>(
                               nv_index_data.value() + nv_index_data_length -
                               sizeof(uint32_t));
  *size = htonl(*nv_data_public);
  return true;
}

bool TpmNvramImpl::InitializeNvramHandle(ScopedTssNvStore* nv_handle,
                                         uint32_t index) {

  TSS_RESULT result = Tspi_Context_CreateObject(tpm_connection_.GetContext(),
                                                TSS_OBJECT_TYPE_NV,
                                                0,
                                                nv_handle->ptr());
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Could not acquire an NVRAM object handle";
    return false;
  }
  result = Tspi_SetAttribUint32(
      nv_handle->value(), TSS_TSPATTRIB_NV_INDEX, 0, index);
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Could not set index on NVRAM object: " << index;
    return false;
  }
  return true;
}

bool TpmNvramImpl::SetOwnerPolicy(ScopedTssNvStore* nv_handle) {
  trousers::ScopedTssPolicy policy_handle(tpm_connection_.GetContext());
  TSS_RESULT result;
  result = Tspi_Context_CreateObject(tpm_connection_.GetContext(),
                                     TSS_OBJECT_TYPE_POLICY,
                                     TSS_POLICY_USAGE,
                                     policy_handle.ptr());
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Error calling Tspi_Context_CreateObject";
    return false;
  }
  std::string owner_password;
  if (!GetOwnerPassword(&owner_password)) {
    return false;
  }
  result = Tspi_Policy_SetSecret(
      policy_handle,
      TSS_SECRET_MODE_PLAIN,
      owner_password.size(),
      reinterpret_cast<BYTE *>(const_cast<char*>(owner_password.data())));
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Error calling Tspi_Policy_SetSecret";
    return false;
  }
  result = Tspi_Policy_AssignToObject(policy_handle.value(),
                                      nv_handle->value());
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Could not set NVRAM object policy.";
    return false;
  }
  return true;
}

bool TpmNvramImpl::SetCompositePcr0(ScopedTssPcrs* pcr_handle) {
  TSS_RESULT result = Tspi_Context_CreateObject(tpm_connection_.GetContext(),
                                                TSS_OBJECT_TYPE_PCRS,
                                                TSS_PCRS_STRUCT_INFO_SHORT,
                                                pcr_handle->ptr());
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Could not acquire PCR object handle";
    return false;
  }
  uint32_t pcr_len;
  std::string owner_password;
  if (!GetOwnerPassword(&owner_password)) {
    return false;
  }
  ScopedTssMemory pcr_value(tpm_connection_.GetContext());
  result = Tspi_TPM_PcrRead(tpm_connection_.GetTpmWithAuth(owner_password),
                            kTpmBootPCR,
                            &pcr_len,
                            pcr_value.ptr());
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Could not read PCR0 value";
    return false;
  }
  result = Tspi_PcrComposite_SetPcrValue(pcr_handle->value(),
                                         kTpmBootPCR,
                                         pcr_len,
                                         pcr_value.value());
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Could not set value for PCR0 in PCR handle";
    return false;
  }
  result = Tspi_PcrComposite_SetPcrLocality(pcr_handle->value(),
                                            kTpmPCRLocality);
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Could not set locality for PCR0 in PCR handle";
    return false;
  }
  return true;
}

bool TpmNvramImpl::GetOwnerPassword(std::string* owner_password) {
  LocalData local_data;
  if (!local_data_store_->Read(&local_data)) {
    LOG(ERROR) << "Error reading local data for owner password.";
    return false;
  }
  if (local_data.owner_password().empty()) {
    LOG(ERROR) << "No owner password present in tpm local_data.";
    return false;
  }
  owner_password->assign(local_data.owner_password());
  return true;
}

}  // namespace tpm_manager
