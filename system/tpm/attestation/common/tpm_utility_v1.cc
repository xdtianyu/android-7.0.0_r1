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

#include "attestation/common/tpm_utility_v1.h"

#include <base/files/file_path.h>
#include <base/files/file_util.h>
#include <base/logging.h>
#include <base/memory/scoped_ptr.h>
#include <base/stl_util.h>
#include <crypto/scoped_openssl_types.h>
#include <crypto/sha2.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <trousers/scoped_tss_type.h>
#include <trousers/trousers.h>
#include <trousers/tss.h>

#define TPM_LOG(severity, result) \
    LOG(severity) << "TPM error 0x" << std::hex << result \
                  << " (" << Trspi_Error_String(result) << "): "

using trousers::ScopedTssContext;
using trousers::ScopedTssKey;
using trousers::ScopedTssMemory;
using trousers::ScopedTssPcrs;

namespace {

using ScopedByteArray = scoped_ptr<BYTE, base::FreeDeleter>;
using ScopedTssEncryptedData = trousers::ScopedTssObject<TSS_HENCDATA>;
using ScopedTssHash = trousers::ScopedTssObject<TSS_HHASH>;

const char* kTpmTpmEnabledFile = "/sys/class/tpm/tpm0/device/enabled";
const char* kMscTpmEnabledFile = "/sys/class/misc/tpm0/device/enabled";
const char* kTpmTpmOwnedFile = "/sys/class/tpm/tpm0/device/owned";
const char* kMscTpmOwnedFile = "/sys/class/misc/tpm0/device/owned";
const unsigned int kWellKnownExponent = 65537;
const unsigned char kSha256DigestInfo[] = {
  0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04,
  0x02, 0x01, 0x05, 0x00, 0x04, 0x20
};

std::string GetFirstByte(const char* file_name) {
  std::string content;
  base::ReadFileToString(base::FilePath(file_name), &content);
  if (content.size() > 1) {
    content.resize(1);
  }
  return content;
}

BYTE* StringAsTSSBuffer(std::string* s) {
  return reinterpret_cast<BYTE*>(string_as_array(s));
}

std::string TSSBufferAsString(const BYTE* buffer, size_t length) {
  return std::string(reinterpret_cast<const char*>(buffer), length);
}

}  // namespace

namespace attestation {

TpmUtilityV1::~TpmUtilityV1() {}

bool TpmUtilityV1::Initialize() {
  if (!ConnectContext(&context_handle_, &tpm_handle_)) {
    LOG(ERROR) << __func__ << ": Failed to connect to the TPM.";
    return false;
  }
  if (!IsTpmReady()) {
    LOG(WARNING) << __func__ << ": TPM is not owned; attestation services will "
                 << "not be available until ownership is taken.";
  }
  return true;
}

bool TpmUtilityV1::IsTpmReady() {
  if (!is_ready_) {
    if (base::PathExists(base::FilePath(kMscTpmEnabledFile))) {
      is_ready_ = (GetFirstByte(kMscTpmEnabledFile) == "1" &&
                   GetFirstByte(kMscTpmOwnedFile) == "1");
    } else {
      is_ready_ = (GetFirstByte(kTpmTpmEnabledFile) == "1" &&
                   GetFirstByte(kTpmTpmOwnedFile) == "1");
    }
  }
  return is_ready_;
}

bool TpmUtilityV1::ActivateIdentity(const std::string& delegate_blob,
                                    const std::string& delegate_secret,
                                    const std::string& identity_key_blob,
                                    const std::string& asym_ca_contents,
                                    const std::string& sym_ca_attestation,
                                    std::string* credential) {
  CHECK(credential);
  if (!SetupSrk()) {
    LOG(ERROR) << "SRK is not ready.";
    return false;
  }

  // Connect to the TPM as the owner delegate.
  ScopedTssContext context_handle;
  TSS_HTPM tpm_handle;
  if (!ConnectContextAsDelegate(delegate_blob, delegate_secret,
                                &context_handle, &tpm_handle)) {
    LOG(ERROR) << __func__ << ": Could not connect to the TPM.";
    return false;
  }
  // Load the Storage Root Key.
  TSS_RESULT result;
  ScopedTssKey srk_handle(context_handle);
  if (!LoadSrk(context_handle, &srk_handle)) {
    LOG(ERROR) << __func__ << ": Failed to load SRK.";
    return false;
  }
  // Load the AIK (which is wrapped by the SRK).
  std::string mutable_identity_key_blob(identity_key_blob);
  BYTE* identity_key_blob_buffer = StringAsTSSBuffer(
      &mutable_identity_key_blob);
  ScopedTssKey identity_key(context_handle);
  result = Tspi_Context_LoadKeyByBlob(
      context_handle,
      srk_handle,
      identity_key_blob.size(),
      identity_key_blob_buffer,
      identity_key.ptr());
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << __func__ << ": Failed to load AIK.";
    return false;
  }
  std::string mutable_asym_ca_contents(asym_ca_contents);
  BYTE* asym_ca_contents_buffer = StringAsTSSBuffer(&mutable_asym_ca_contents);
  std::string mutable_sym_ca_attestation(sym_ca_attestation);
  BYTE* sym_ca_attestation_buffer = StringAsTSSBuffer(
      &mutable_sym_ca_attestation);
  UINT32 credential_length = 0;
  ScopedTssMemory credential_buffer(context_handle);
  result = Tspi_TPM_ActivateIdentity(tpm_handle, identity_key,
                                     asym_ca_contents.size(),
                                     asym_ca_contents_buffer,
                                     sym_ca_attestation.size(),
                                     sym_ca_attestation_buffer,
                                     &credential_length,
                                     credential_buffer.ptr());
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << __func__ << ": Failed to activate identity.";
    return false;
  }
  credential->assign(TSSBufferAsString(credential_buffer.value(),
                                       credential_length));
  return true;
}

bool TpmUtilityV1::CreateCertifiedKey(KeyType key_type,
                                      KeyUsage key_usage,
                                      const std::string& identity_key_blob,
                                      const std::string& external_data,
                                      std::string* key_blob,
                                      std::string* public_key,
                                      std::string* public_key_tpm_format,
                                      std::string* key_info,
                                      std::string* proof) {
  CHECK(key_blob && public_key && public_key_tpm_format && key_info && proof);
  if (!SetupSrk()) {
    LOG(ERROR) << "SRK is not ready.";
    return false;
  }
  if (key_type != KEY_TYPE_RSA) {
    LOG(ERROR) << "Only RSA supported on TPM v1.2.";
    return false;
  }

  // Load the AIK (which is wrapped by the SRK).
  ScopedTssKey identity_key(context_handle_);
  if (!LoadKeyFromBlob(identity_key_blob, context_handle_, srk_handle_,
                       &identity_key)) {
    LOG(ERROR) << __func__ << "Failed to load AIK.";
    return false;
  }

  // Create a non-migratable RSA key.
  ScopedTssKey key(context_handle_);
  UINT32 tss_key_type = (key_usage == KEY_USAGE_SIGN) ? TSS_KEY_TYPE_SIGNING :
                                                        TSS_KEY_TYPE_BIND;
  UINT32 init_flags = tss_key_type |
                      TSS_KEY_NOT_MIGRATABLE |
                      TSS_KEY_VOLATILE |
                      TSS_KEY_NO_AUTHORIZATION |
                      TSS_KEY_SIZE_2048;
  TSS_RESULT result = Tspi_Context_CreateObject(context_handle_,
                                                TSS_OBJECT_TYPE_RSAKEY,
                                                init_flags, key.ptr());
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << __func__ << ": Failed to create object.";
    return false;
  }
  if (key_usage == KEY_USAGE_SIGN) {
    result = Tspi_SetAttribUint32(key,
                                  TSS_TSPATTRIB_KEY_INFO,
                                  TSS_TSPATTRIB_KEYINFO_SIGSCHEME,
                                  TSS_SS_RSASSAPKCS1V15_DER);
  } else {
    result = Tspi_SetAttribUint32(key,
                                  TSS_TSPATTRIB_KEY_INFO,
                                  TSS_TSPATTRIB_KEYINFO_ENCSCHEME,
                                  TSS_ES_RSAESOAEP_SHA1_MGF1);
  }
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << __func__ << ": Failed to set scheme.";
    return false;
  }
  result = Tspi_Key_CreateKey(key, srk_handle_, 0);
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << __func__ << ": Failed to create key.";
    return false;
  }
  result = Tspi_Key_LoadKey(key, srk_handle_);
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << __func__ << ": Failed to load key.";
    return false;
  }

  // Certify the key.
  TSS_VALIDATION validation;
  memset(&validation, 0, sizeof(validation));
  validation.ulExternalDataLength = external_data.size();
  std::string mutable_external_data(external_data);
  validation.rgbExternalData = StringAsTSSBuffer(&mutable_external_data);
  result = Tspi_Key_CertifyKey(key, identity_key, &validation);
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << __func__ << ": Failed to certify key.";
    return false;
  }
  ScopedTssMemory scoped_certified_data(0, validation.rgbData);
  ScopedTssMemory scoped_proof(0, validation.rgbValidationData);

  // Get the certified public key.
  if (!GetDataAttribute(context_handle_,
                        key,
                        TSS_TSPATTRIB_KEY_BLOB,
                        TSS_TSPATTRIB_KEYBLOB_PUBLIC_KEY,
                        public_key_tpm_format)) {
    LOG(ERROR) << __func__ << ": Failed to read public key.";
    return false;
  }
  if (!ConvertPublicKeyToDER(*public_key_tpm_format, public_key)) {
    return false;
  }

  // Get the certified key blob so we can load it later.
  if (!GetDataAttribute(context_handle_,
                        key,
                        TSS_TSPATTRIB_KEY_BLOB,
                        TSS_TSPATTRIB_KEYBLOB_BLOB,
                        key_blob)) {
    LOG(ERROR) << __func__ << ": Failed to read key blob.";
    return false;
  }

  // Get the data that was certified.
  key_info->assign(TSSBufferAsString(validation.rgbData,
                                     validation.ulDataLength));

  // Get the certification proof.
  proof->assign(TSSBufferAsString(validation.rgbValidationData,
                                  validation.ulValidationDataLength));
  return true;
}

bool TpmUtilityV1::SealToPCR0(const std::string& data,
                              std::string* sealed_data) {
  CHECK(sealed_data);
  if (!SetupSrk()) {
    LOG(ERROR) << "SRK is not ready.";
    return false;
  }

  // Create a PCRS object which holds the value of PCR0.
  ScopedTssPcrs pcrs_handle(context_handle_);
  TSS_RESULT result;
  if (TPM_ERROR(result = Tspi_Context_CreateObject(context_handle_,
                                                   TSS_OBJECT_TYPE_PCRS,
                                                   TSS_PCRS_STRUCT_INFO,
                                                   pcrs_handle.ptr()))) {
    TPM_LOG(ERROR, result)
        << __func__ << ": Error calling Tspi_Context_CreateObject";
    return false;
  }
  UINT32 pcr_length = 0;
  ScopedTssMemory pcr_value(context_handle_);
  Tspi_TPM_PcrRead(tpm_handle_, 0, &pcr_length, pcr_value.ptr());
  Tspi_PcrComposite_SetPcrValue(pcrs_handle, 0, pcr_length, pcr_value.value());

  // Create a ENCDATA object to receive the sealed data.
  ScopedTssKey encrypted_data_handle(context_handle_);
  if (TPM_ERROR(result = Tspi_Context_CreateObject(
      context_handle_,
      TSS_OBJECT_TYPE_ENCDATA,
      TSS_ENCDATA_SEAL,
      encrypted_data_handle.ptr()))) {
    TPM_LOG(ERROR, result)
        << __func__ << ": Error calling Tspi_Context_CreateObject";
    return false;
  }

  // Seal the given value with the SRK.
  std::string mutable_data(data);
  BYTE* data_buffer = StringAsTSSBuffer(&mutable_data);
  if (TPM_ERROR(result = Tspi_Data_Seal(
      encrypted_data_handle,
      srk_handle_,
      data.size(),
      data_buffer,
      pcrs_handle))) {
    TPM_LOG(ERROR, result) << __func__ << ": Error calling Tspi_Data_Seal";
    return false;
  }

  // Extract the sealed value.
  ScopedTssMemory encrypted_data(context_handle_);
  UINT32 encrypted_data_length = 0;
  if (TPM_ERROR(result = Tspi_GetAttribData(encrypted_data_handle,
                                            TSS_TSPATTRIB_ENCDATA_BLOB,
                                            TSS_TSPATTRIB_ENCDATABLOB_BLOB,
                                            &encrypted_data_length,
                                            encrypted_data.ptr()))) {
    TPM_LOG(ERROR, result) << __func__ << ": Error calling Tspi_GetAttribData";
    return false;
  }
  sealed_data->assign(TSSBufferAsString(encrypted_data.value(),
                                        encrypted_data_length));
  return true;
}

bool TpmUtilityV1::Unseal(const std::string& sealed_data, std::string* data) {
  CHECK(data);
  if (!SetupSrk()) {
    LOG(ERROR) << "SRK is not ready.";
    return false;
  }

  // Create an ENCDATA object with the sealed value.
  ScopedTssKey encrypted_data_handle(context_handle_);
  TSS_RESULT result;
  if (TPM_ERROR(result = Tspi_Context_CreateObject(
      context_handle_,
      TSS_OBJECT_TYPE_ENCDATA,
      TSS_ENCDATA_SEAL,
      encrypted_data_handle.ptr()))) {
    TPM_LOG(ERROR, result)
        << __func__ << ": Error calling Tspi_Context_CreateObject";
    return false;
  }

  std::string mutable_sealed_data(sealed_data);
  BYTE* sealed_data_buffer = StringAsTSSBuffer(&mutable_sealed_data);
  if (TPM_ERROR(result = Tspi_SetAttribData(encrypted_data_handle,
      TSS_TSPATTRIB_ENCDATA_BLOB,
      TSS_TSPATTRIB_ENCDATABLOB_BLOB,
      sealed_data.size(),
      sealed_data_buffer))) {
    TPM_LOG(ERROR, result) << __func__ << ": Error calling Tspi_SetAttribData";
    return false;
  }

  // Unseal using the SRK.
  ScopedTssMemory decrypted_data(context_handle_);
  UINT32 decrypted_data_length = 0;
  if (TPM_ERROR(result = Tspi_Data_Unseal(encrypted_data_handle,
                                          srk_handle_,
                                          &decrypted_data_length,
                                          decrypted_data.ptr()))) {
    TPM_LOG(ERROR, result) << __func__ << ": Error calling Tspi_Data_Unseal";
    return false;
  }
  data->assign(TSSBufferAsString(decrypted_data.value(),
                                 decrypted_data_length));
  return true;
}

bool TpmUtilityV1::GetEndorsementPublicKey(std::string* public_key) {
  // Get a handle to the EK public key.
  ScopedTssKey ek_public_key_object(context_handle_);
  TSS_RESULT result = Tspi_TPM_GetPubEndorsementKey(tpm_handle_, false, nullptr,
                                                    ek_public_key_object.ptr());
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << __func__ << ": Failed to get key.";
    return false;
  }
  // Get the public key in TPM_PUBKEY form.
  std::string ek_public_key_blob;
  if (!GetDataAttribute(context_handle_,
                        ek_public_key_object,
                        TSS_TSPATTRIB_KEY_BLOB,
                        TSS_TSPATTRIB_KEYBLOB_PUBLIC_KEY,
                        &ek_public_key_blob)) {
    LOG(ERROR) << __func__ << ": Failed to read public key.";
    return false;
  }
  // Get the public key in DER encoded form.
  if (!ConvertPublicKeyToDER(ek_public_key_blob, public_key)) {
    return false;
  }
  return true;
}

bool TpmUtilityV1::Unbind(const std::string& key_blob,
                          const std::string& bound_data,
                          std::string* data) {
  CHECK(data);
  if (!SetupSrk()) {
    LOG(ERROR) << "SRK is not ready.";
    return false;
  }
  ScopedTssKey key_handle(context_handle_);
  if (!LoadKeyFromBlob(key_blob, context_handle_, srk_handle_, &key_handle)) {
    return false;
  }
  TSS_RESULT result;
  ScopedTssEncryptedData data_handle(context_handle_);
  if (TPM_ERROR(result = Tspi_Context_CreateObject(context_handle_,
                                                   TSS_OBJECT_TYPE_ENCDATA,
                                                   TSS_ENCDATA_BIND,
                                                   data_handle.ptr()))) {
    TPM_LOG(ERROR, result) << __func__ << ": Tspi_Context_CreateObject failed.";
    return false;
  }
  std::string mutable_bound_data(bound_data);
  if (TPM_ERROR(result = Tspi_SetAttribData(
      data_handle,
      TSS_TSPATTRIB_ENCDATA_BLOB,
      TSS_TSPATTRIB_ENCDATABLOB_BLOB,
      bound_data.size(),
      StringAsTSSBuffer(&mutable_bound_data)))) {
    TPM_LOG(ERROR, result) << __func__ << ": Tspi_SetAttribData failed.";
    return false;
  }

  ScopedTssMemory decrypted_data(context_handle_);
  UINT32 length = 0;
  if (TPM_ERROR(result = Tspi_Data_Unbind(data_handle, key_handle,
                                          &length, decrypted_data.ptr()))) {
    TPM_LOG(ERROR, result) << __func__ << ": Tspi_Data_Unbind failed.";
    return false;
  }
  data->assign(TSSBufferAsString(decrypted_data.value(), length));
  return true;
}

bool TpmUtilityV1::Sign(const std::string& key_blob,
                        const std::string& data_to_sign,
                        std::string* signature) {
  CHECK(signature);
  if (!SetupSrk()) {
    LOG(ERROR) << "SRK is not ready.";
    return false;
  }
  ScopedTssKey key_handle(context_handle_);
  if (!LoadKeyFromBlob(key_blob, context_handle_, srk_handle_, &key_handle)) {
    return false;
  }
  // Construct an ASN.1 DER DigestInfo.
  std::string digest_to_sign(std::begin(kSha256DigestInfo),
                             std::end(kSha256DigestInfo));
  digest_to_sign += crypto::SHA256HashString(data_to_sign);
  // Create a hash object to hold the digest.
  ScopedTssHash hash_handle(context_handle_);
  TSS_RESULT result = Tspi_Context_CreateObject(context_handle_,
                                                TSS_OBJECT_TYPE_HASH,
                                                TSS_HASH_OTHER,
                                                hash_handle.ptr());
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << __func__ << ": Failed to create hash object.";
    return false;
  }
  result = Tspi_Hash_SetHashValue(hash_handle,
                                  digest_to_sign.size(),
                                  StringAsTSSBuffer(&digest_to_sign));
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << __func__ << ": Failed to set hash data.";
    return false;
  }
  UINT32 length = 0;
  ScopedTssMemory buffer(context_handle_);
  result = Tspi_Hash_Sign(hash_handle, key_handle, &length, buffer.ptr());
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << __func__ << ": Failed to generate signature.";
    return false;
  }
  signature->assign(TSSBufferAsString(buffer.value(), length));
  return true;
}

bool TpmUtilityV1::ConnectContext(ScopedTssContext* context, TSS_HTPM* tpm) {
  *tpm = 0;
  TSS_RESULT result;
  if (TPM_ERROR(result = Tspi_Context_Create(context->ptr()))) {
    TPM_LOG(ERROR, result) << __func__ << ": Error calling Tspi_Context_Create";
    return false;
  }
  if (TPM_ERROR(result = Tspi_Context_Connect(*context, nullptr))) {
    TPM_LOG(ERROR, result) << __func__
                           << ": Error calling Tspi_Context_Connect";
    return false;
  }
  if (TPM_ERROR(result = Tspi_Context_GetTpmObject(*context, tpm))) {
    TPM_LOG(ERROR, result) << __func__
                           << ": Error calling Tspi_Context_GetTpmObject";
    return false;
  }
  return true;
}

bool TpmUtilityV1::ConnectContextAsDelegate(const std::string& delegate_blob,
                                            const std::string& delegate_secret,
                                            ScopedTssContext* context,
                                            TSS_HTPM* tpm) {
  *tpm = 0;
  if (!ConnectContext(context, tpm)) {
    return false;
  }
  TSS_RESULT result;
  TSS_HPOLICY tpm_usage_policy;
  if (TPM_ERROR(result = Tspi_GetPolicyObject(*tpm,
                                              TSS_POLICY_USAGE,
                                              &tpm_usage_policy))) {
    TPM_LOG(ERROR, result) << __func__
                           << ": Error calling Tspi_GetPolicyObject";
    return false;
  }
  std::string mutable_delegate_secret(delegate_secret);
  BYTE* secret_buffer = StringAsTSSBuffer(&mutable_delegate_secret);
  if (TPM_ERROR(result = Tspi_Policy_SetSecret(tpm_usage_policy,
                                               TSS_SECRET_MODE_PLAIN,
                                               delegate_secret.size(),
                                               secret_buffer))) {
    TPM_LOG(ERROR, result) << __func__
                           << ": Error calling Tspi_Policy_SetSecret";
    return false;
  }
  std::string mutable_delegate_blob(delegate_blob);
  BYTE* blob_buffer = StringAsTSSBuffer(&mutable_delegate_blob);
  if (TPM_ERROR(result = Tspi_SetAttribData(
      tpm_usage_policy,
      TSS_TSPATTRIB_POLICY_DELEGATION_INFO,
      TSS_TSPATTRIB_POLDEL_OWNERBLOB,
      delegate_blob.size(),
      blob_buffer))) {
    TPM_LOG(ERROR, result) << __func__ << ": Error calling Tspi_SetAttribData";
    return false;
  }
  return true;
}

bool TpmUtilityV1::SetupSrk() {
  if (!IsTpmReady()) {
    return false;
  }
  if (srk_handle_) {
    return true;
  }
  srk_handle_.reset(context_handle_, 0);
  if (!LoadSrk(context_handle_, &srk_handle_)) {
    LOG(ERROR) << __func__ << ": Failed to load SRK.";
    return false;
  }
  // In order to wrap a key with the SRK we need access to the SRK public key
  // and we need to get it manually. Once it's in the key object, we don't need
  // to do this again.
  UINT32 length = 0;
  ScopedTssMemory buffer(context_handle_);
  TSS_RESULT result;
  result = Tspi_Key_GetPubKey(srk_handle_, &length, buffer.ptr());
  if (result != TSS_SUCCESS) {
    TPM_LOG(INFO, result) << __func__ << ": Failed to read SRK public key.";
    return false;
  }
  return true;
}

bool TpmUtilityV1::LoadSrk(TSS_HCONTEXT context_handle,
                           ScopedTssKey* srk_handle) {
  TSS_RESULT result;
  TSS_UUID uuid = TSS_UUID_SRK;
  if (TPM_ERROR(result = Tspi_Context_LoadKeyByUUID(context_handle,
                                                    TSS_PS_TYPE_SYSTEM,
                                                    uuid,
                                                    srk_handle->ptr()))) {
    TPM_LOG(ERROR, result) << __func__
                           << ": Error calling Tspi_Context_LoadKeyByUUID";
    return false;
  }
  // Check if the SRK wants a password.
  UINT32 auth_usage;
  if (TPM_ERROR(result = Tspi_GetAttribUint32(*srk_handle,
                                              TSS_TSPATTRIB_KEY_INFO,
                                              TSS_TSPATTRIB_KEYINFO_AUTHUSAGE,
                                              &auth_usage))) {
    TPM_LOG(ERROR, result) << __func__
                           << ": Error calling Tspi_GetAttribUint32";
    return false;
  }
  if (auth_usage) {
    // Give it an empty password if needed.
    TSS_HPOLICY usage_policy;
    if (TPM_ERROR(result = Tspi_GetPolicyObject(*srk_handle,
                                                TSS_POLICY_USAGE,
                                                &usage_policy))) {
    TPM_LOG(ERROR, result) << __func__
                           << ": Error calling Tspi_GetPolicyObject";
      return false;
    }

    BYTE empty_password[] = {};
    if (TPM_ERROR(result = Tspi_Policy_SetSecret(usage_policy,
                                                 TSS_SECRET_MODE_PLAIN,
                                                 0, empty_password))) {
      TPM_LOG(ERROR, result) << __func__
                             << ": Error calling Tspi_Policy_SetSecret";
      return false;
    }
  }
  return true;
}

bool TpmUtilityV1::LoadKeyFromBlob(const std::string& key_blob,
                                   TSS_HCONTEXT context_handle,
                                   TSS_HKEY parent_key_handle,
                                   ScopedTssKey* key_handle) {
  std::string mutable_key_blob(key_blob);
  BYTE* key_blob_buffer = StringAsTSSBuffer(&mutable_key_blob);
  TSS_RESULT result = Tspi_Context_LoadKeyByBlob(
      context_handle,
      parent_key_handle,
      key_blob.size(),
      key_blob_buffer,
      key_handle->ptr());
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << __func__ << ": Failed to load key by blob.";
    return false;
  }
  return true;
}

bool TpmUtilityV1::GetDataAttribute(TSS_HCONTEXT context,
                                    TSS_HOBJECT object,
                                    TSS_FLAG flag,
                                    TSS_FLAG sub_flag,
                                    std::string* data) {
  UINT32 length = 0;
  ScopedTssMemory buffer(context);
  TSS_RESULT result = Tspi_GetAttribData(object, flag, sub_flag, &length,
                                         buffer.ptr());
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << __func__ << "Failed to read object attribute.";
    return false;
  }
  data->assign(TSSBufferAsString(buffer.value(), length));
  return true;
}

bool TpmUtilityV1::ConvertPublicKeyToDER(const std::string& public_key,
                                         std::string* public_key_der) {
  // Parse the serialized TPM_PUBKEY.
  UINT64 offset = 0;
  std::string mutable_public_key(public_key);
  BYTE* buffer = StringAsTSSBuffer(&mutable_public_key);
  TPM_PUBKEY parsed;
  TSS_RESULT result = Trspi_UnloadBlob_PUBKEY(&offset, buffer, &parsed);
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Failed to parse TPM_PUBKEY.";
    return false;
  }
  ScopedByteArray scoped_key(parsed.pubKey.key);
  ScopedByteArray scoped_parms(parsed.algorithmParms.parms);
  TPM_RSA_KEY_PARMS* parms =
      reinterpret_cast<TPM_RSA_KEY_PARMS*>(parsed.algorithmParms.parms);
  crypto::ScopedRSA rsa(RSA_new());
  CHECK(rsa.get());
  // Get the public exponent.
  if (parms->exponentSize == 0) {
    rsa.get()->e = BN_new();
    CHECK(rsa.get()->e);
    BN_set_word(rsa.get()->e, kWellKnownExponent);
  } else {
    rsa.get()->e = BN_bin2bn(parms->exponent, parms->exponentSize, nullptr);
    CHECK(rsa.get()->e);
  }
  // Get the modulus.
  rsa.get()->n = BN_bin2bn(parsed.pubKey.key, parsed.pubKey.keyLength, nullptr);
  CHECK(rsa.get()->n);

  // DER encode.
  int der_length = i2d_RSAPublicKey(rsa.get(), nullptr);
  if (der_length < 0) {
    LOG(ERROR) << "Failed to DER-encode public key.";
    return false;
  }
  public_key_der->resize(der_length);
  unsigned char* der_buffer = reinterpret_cast<unsigned char*>(
      string_as_array(public_key_der));
  der_length = i2d_RSAPublicKey(rsa.get(), &der_buffer);
  if (der_length < 0) {
    LOG(ERROR) << "Failed to DER-encode public key.";
    return false;
  }
  public_key_der->resize(der_length);
  return true;
}

}  // namespace attestation
