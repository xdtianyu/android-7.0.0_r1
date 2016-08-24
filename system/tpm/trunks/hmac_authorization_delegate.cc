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

#include "trunks/hmac_authorization_delegate.h"

#include <base/logging.h>
#include <base/memory/scoped_ptr.h>
#include <base/stl_util.h>
#include <crypto/secure_util.h>
#include <openssl/aes.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

namespace trunks {

namespace {

const uint32_t kDigestBits = 256;
const uint16_t kNonceMinSize = 16;
const uint16_t kNonceMaxSize = 32;
const uint8_t kDecryptSession = 1<<5;
const uint8_t kEncryptSession = 1<<6;
const uint8_t kLabelSize = 4;
const size_t kAesIVSize = 16;
const uint32_t kTpmBufferSize = 4096;

}  // namespace

HmacAuthorizationDelegate::HmacAuthorizationDelegate()
    : session_handle_(0),
      is_parameter_encryption_enabled_(false),
      nonce_generated_(false),
      future_authorization_value_set_(false),
      use_entity_authorization_for_encryption_only_(false) {
  tpm_nonce_.size = 0;
  caller_nonce_.size = 0;
}

HmacAuthorizationDelegate::~HmacAuthorizationDelegate() {}

bool HmacAuthorizationDelegate::GetCommandAuthorization(
    const std::string& command_hash,
    bool is_command_parameter_encryption_possible,
    bool is_response_parameter_encryption_possible,
    std::string* authorization) {
  if (!session_handle_) {
    authorization->clear();
    LOG(ERROR) << "Delegate being used before Initialization,";
    return false;
  }
  TPMS_AUTH_COMMAND auth;
  auth.session_handle = session_handle_;
  if (!nonce_generated_) {
    RegenerateCallerNonce();
  }
  auth.nonce = caller_nonce_;
  auth.session_attributes = kContinueSession;
  if (is_parameter_encryption_enabled_) {
    if (is_command_parameter_encryption_possible) {
      auth.session_attributes |= kDecryptSession;
    }
    if (is_response_parameter_encryption_possible) {
      auth.session_attributes |= kEncryptSession;
    }
  }
  // We reset the |nonce_generated| flag in preperation of the next command.
  nonce_generated_ = false;
  std::string attributes_bytes;
  CHECK_EQ(Serialize_TPMA_SESSION(auth.session_attributes, &attributes_bytes),
           TPM_RC_SUCCESS) << "Error serializing session attributes.";

  std::string hmac_data;
  std::string hmac_key;
  if (!use_entity_authorization_for_encryption_only_) {
    hmac_key = session_key_ + entity_authorization_value_;
  } else {
    hmac_key = session_key_;
  }
  hmac_data.append(command_hash);
  hmac_data.append(reinterpret_cast<const char*>(caller_nonce_.buffer),
                   caller_nonce_.size);
  hmac_data.append(reinterpret_cast<const char*>(tpm_nonce_.buffer),
                   tpm_nonce_.size);
  hmac_data.append(attributes_bytes);
  std::string digest = HmacSha256(hmac_key, hmac_data);
  auth.hmac = Make_TPM2B_DIGEST(digest);

  TPM_RC serialize_error = Serialize_TPMS_AUTH_COMMAND(auth, authorization);
  if (serialize_error != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Could not serialize command auth.";
    return false;
  }
  return true;
}

bool HmacAuthorizationDelegate::CheckResponseAuthorization(
    const std::string& response_hash,
    const std::string& authorization) {
  if (!session_handle_) {
    return false;
  }
  TPMS_AUTH_RESPONSE auth_response;
  std::string mutable_auth_string(authorization);
  TPM_RC parse_error;
  parse_error = Parse_TPMS_AUTH_RESPONSE(&mutable_auth_string,
                                         &auth_response,
                                         nullptr);
  if (parse_error != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Could not parse authorization response.";
    return false;
  }
  if (auth_response.hmac.size != kHashDigestSize) {
    LOG(ERROR) << "TPM auth hmac was incorrect size.";
    return false;
  }
  if (auth_response.nonce.size < kNonceMinSize ||
      auth_response.nonce.size > kNonceMaxSize) {
    LOG(ERROR) << "TPM_nonce is not the correct length.";
    return false;
  }
  tpm_nonce_ = auth_response.nonce;
  std::string attributes_bytes;
  CHECK_EQ(Serialize_TPMA_SESSION(auth_response.session_attributes,
                                  &attributes_bytes),
           TPM_RC_SUCCESS) << "Error serializing session attributes.";

  std::string hmac_data;
  std::string hmac_key;
  if (!use_entity_authorization_for_encryption_only_) {
    // In a special case with TPM2_HierarchyChangeAuth, we need to use the
    // auth_value that was set.
    if (future_authorization_value_set_) {
      hmac_key = session_key_ + future_authorization_value_;
      future_authorization_value_set_ = false;
    } else {
      hmac_key = session_key_ + entity_authorization_value_;
    }
  } else {
    hmac_key = session_key_;
  }
  hmac_data.append(response_hash);
  hmac_data.append(reinterpret_cast<const char*>(tpm_nonce_.buffer),
                   tpm_nonce_.size);
  hmac_data.append(reinterpret_cast<const char*>(caller_nonce_.buffer),
                   caller_nonce_.size);
  hmac_data.append(attributes_bytes);
  std::string digest = HmacSha256(hmac_key, hmac_data);
  CHECK_EQ(digest.size(), auth_response.hmac.size);
  if (!crypto::SecureMemEqual(digest.data(), auth_response.hmac.buffer,
                              digest.size())) {
    LOG(ERROR) << "Authorization response hash did not match expected value.";
    return false;
  }
  return true;
}

bool HmacAuthorizationDelegate::EncryptCommandParameter(
    std::string* parameter) {
  CHECK(parameter);
  if (!session_handle_) {
    LOG(ERROR) << __func__ << ": Invalid session handle.";
    return false;
  }
  if (!is_parameter_encryption_enabled_) {
    // No parameter encryption enabled.
    return true;
  }
  if (parameter->size() > kTpmBufferSize) {
    LOG(ERROR) << "Parameter size is too large for TPM decryption.";
    return false;
  }
  RegenerateCallerNonce();
  nonce_generated_ = true;
  AesOperation(parameter, caller_nonce_, tpm_nonce_, AES_ENCRYPT);
  return true;
}

bool HmacAuthorizationDelegate::DecryptResponseParameter(
    std::string* parameter) {
  CHECK(parameter);
  if (!session_handle_) {
    LOG(ERROR) << __func__ << ": Invalid session handle.";
    return false;
  }
  if (!is_parameter_encryption_enabled_) {
    // No parameter decryption enabled.
    return true;
  }
  if (parameter->size() > kTpmBufferSize) {
    LOG(ERROR) << "Parameter size is too large for TPM encryption.";
    return false;
  }
  AesOperation(parameter, tpm_nonce_, caller_nonce_, AES_DECRYPT);
  return true;
}

bool HmacAuthorizationDelegate::InitSession(
    TPM_HANDLE session_handle,
    const TPM2B_NONCE& tpm_nonce,
    const TPM2B_NONCE& caller_nonce,
    const std::string& salt,
    const std::string& bind_auth_value,
    bool enable_parameter_encryption) {
  session_handle_ = session_handle;
  if (caller_nonce.size < kNonceMinSize || caller_nonce.size > kNonceMaxSize ||
      tpm_nonce.size < kNonceMinSize || tpm_nonce.size > kNonceMaxSize) {
    LOG(INFO) << "Session Nonces have to be between 16 and 32 bytes long.";
    return false;
  }
  tpm_nonce_ = tpm_nonce;
  caller_nonce_ = caller_nonce;
  std::string session_key_label("ATH", kLabelSize);
  is_parameter_encryption_enabled_ = enable_parameter_encryption;
  if (salt.length() == 0 && bind_auth_value.length() == 0) {
    // SessionKey is set to the empty string for unsalted and
    // unbound sessions.
    session_key_ = std::string();
  } else {
    session_key_ = CreateKey(bind_auth_value + salt,
                             session_key_label,
                             tpm_nonce_,
                             caller_nonce_);
  }
  return true;
}

void HmacAuthorizationDelegate::set_future_authorization_value(
    const std::string& auth_value) {
  future_authorization_value_ = auth_value;
  future_authorization_value_set_ = true;
}

std::string HmacAuthorizationDelegate::CreateKey(
    const std::string& hmac_key,
    const std::string& label,
    const TPM2B_NONCE& nonce_newer,
    const TPM2B_NONCE& nonce_older) {
  std::string counter;
  std::string digest_size_bits;
  if (Serialize_uint32_t(1, &counter) != TPM_RC_SUCCESS ||
      Serialize_uint32_t(kDigestBits, &digest_size_bits) != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error serializing uint32_t during session key generation.";
    return std::string();
  }
  CHECK_EQ(counter.size(), sizeof(uint32_t));
  CHECK_EQ(digest_size_bits.size(), sizeof(uint32_t));
  CHECK_EQ(label.size(), kLabelSize);

  std::string data;
  data.append(counter);
  data.append(label);
  data.append(reinterpret_cast<const char*>(nonce_newer.buffer),
              nonce_newer.size);
  data.append(reinterpret_cast<const char*>(nonce_older.buffer),
              nonce_older.size);
  data.append(digest_size_bits);
  std::string key = HmacSha256(hmac_key, data);
  return key;
}

std::string HmacAuthorizationDelegate::HmacSha256(const std::string& key,
                                                  const std::string& data) {
  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int digest_length;
  HMAC(EVP_sha256(),
       key.data(),
       key.size(),
       reinterpret_cast<const unsigned char*>(data.data()),
       data.size(),
       digest,
       &digest_length);
  CHECK_EQ(digest_length, kHashDigestSize);
  return std::string(reinterpret_cast<char*>(digest), digest_length);
}

void HmacAuthorizationDelegate::AesOperation(std::string* parameter,
                                             const TPM2B_NONCE& nonce_newer,
                                             const TPM2B_NONCE& nonce_older,
                                             int operation_type) {
  std::string label("CFB", kLabelSize);
  std::string compound_key = CreateKey(
      session_key_ + entity_authorization_value_,
      label,
      nonce_newer,
      nonce_older);
  CHECK_EQ(compound_key.size(), kAesKeySize + kAesIVSize);
  unsigned char aes_key[kAesKeySize];
  unsigned char aes_iv[kAesIVSize];
  memcpy(aes_key, &compound_key[0], kAesKeySize);
  memcpy(aes_iv, &compound_key[kAesKeySize], kAesIVSize);
  AES_KEY key;
  int iv_offset = 0;
  AES_set_encrypt_key(aes_key, kAesKeySize*8, &key);
  unsigned char decrypted[kTpmBufferSize];
  AES_cfb128_encrypt(reinterpret_cast<const unsigned char*>(parameter->data()),
                     decrypted,
                     parameter->size(),
                     &key, aes_iv,
                     &iv_offset,
                     operation_type);
  memcpy(string_as_array(parameter), decrypted, parameter->size());
}

void HmacAuthorizationDelegate::RegenerateCallerNonce() {
  CHECK(session_handle_);
  // RAND_bytes takes a signed number, but since nonce_size is guaranteed to be
  // less than 32 bytes and greater than 16 we dont have to worry about it.
  CHECK_EQ(RAND_bytes(caller_nonce_.buffer, caller_nonce_.size), 1) <<
      "Error regnerating a cryptographically random nonce.";
}

}  // namespace trunks
