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

#include "trunks/session_manager_impl.h"

#include <string>

#include <base/logging.h>
#include <base/stl_util.h>
#include <crypto/openssl_util.h>
#include <crypto/scoped_openssl_types.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/mem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>

#include "trunks/error_codes.h"
#include "trunks/tpm_generated.h"
#include "trunks/tpm_utility.h"

namespace {
const size_t kWellKnownExponent = 0x10001;

std::string GetOpenSSLError() {
  BIO* bio = BIO_new(BIO_s_mem());
  ERR_print_errors(bio);
  char* data = nullptr;
  int data_len = BIO_get_mem_data(bio, &data);
  std::string error_string(data, data_len);
  BIO_free(bio);
  return error_string;
}
}  // namespace

namespace trunks {

SessionManagerImpl::SessionManagerImpl(const TrunksFactory& factory)
    : factory_(factory),
      session_handle_(kUninitializedHandle) {
  crypto::EnsureOpenSSLInit();
}

SessionManagerImpl::~SessionManagerImpl() {
  CloseSession();
}

void SessionManagerImpl::CloseSession() {
  if (session_handle_ == kUninitializedHandle) {
    return;
  }
  TPM_RC result = factory_.GetTpm()->FlushContextSync(session_handle_, nullptr);
  if (result != TPM_RC_SUCCESS) {
    LOG(WARNING) << "Error closing tpm session: " << GetErrorString(result);
  }
  session_handle_ = kUninitializedHandle;
}

TPM_RC SessionManagerImpl::StartSession(
    TPM_SE session_type,
    TPMI_DH_ENTITY bind_entity,
    const std::string& bind_authorization_value,
    bool enable_encryption,
    HmacAuthorizationDelegate* delegate) {
  CHECK(delegate);
  // If we already have an active session, close it.
  CloseSession();

  std::string salt(SHA256_DIGEST_SIZE, 0);
  unsigned char* salt_buffer =
      reinterpret_cast<unsigned char*>(string_as_array(&salt));
  CHECK_EQ(RAND_bytes(salt_buffer, salt.size()), 1)
      << "Error generating a cryptographically random salt.";
  // First we encrypt the cryptographically secure salt using PKCS1_OAEP
  // padded RSA public key encryption. This is specified in TPM2.0
  // Part1 Architecture, Appendix B.10.2.
  std::string encrypted_salt;
  TPM_RC salt_result = EncryptSalt(salt, &encrypted_salt);
  if (salt_result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error encrypting salt: " << GetErrorString(salt_result);
    return salt_result;
  }

  TPM2B_ENCRYPTED_SECRET encrypted_secret =
      Make_TPM2B_ENCRYPTED_SECRET(encrypted_salt);
  // Then we use TPM2_StartAuthSession to start a HMAC session with the TPM.
  // The tpm returns the tpm_nonce and the session_handle referencing the
  // created session.
  TPMI_ALG_HASH hash_algorithm = TPM_ALG_SHA256;
  TPMT_SYM_DEF symmetric_algorithm;
  symmetric_algorithm.algorithm = TPM_ALG_AES;
  symmetric_algorithm.key_bits.aes = 128;
  symmetric_algorithm.mode.aes = TPM_ALG_CFB;

  TPM2B_NONCE nonce_caller;
  TPM2B_NONCE nonce_tpm;
  // We use sha1_digest_size here because that is the minimum length
  // needed for the nonce.
  nonce_caller.size = SHA1_DIGEST_SIZE;
  CHECK_EQ(RAND_bytes(nonce_caller.buffer, nonce_caller.size), 1)
      << "Error generating a cryptographically random nonce.";

  Tpm* tpm = factory_.GetTpm();
  // The TPM2 command below needs no authorization. This is why we can use
  // the empty string "", when referring to the handle names for the salting
  // key and the bind entity.
  TPM_RC tpm_result = tpm->StartAuthSessionSync(kSaltingKey,
                                                "",  // salt_handle_name.
                                                bind_entity,
                                                "",  // bind_entity_name.
                                                nonce_caller,
                                                encrypted_secret,
                                                session_type,
                                                symmetric_algorithm,
                                                hash_algorithm,
                                                &session_handle_,
                                                &nonce_tpm,
                                                nullptr);  // No Authorization.
  if (tpm_result) {
    LOG(ERROR) << "Error creating an authorization session: "
               << GetErrorString(tpm_result);
    return tpm_result;
  }
  bool hmac_result = delegate->InitSession(
    session_handle_,
    nonce_tpm,
    nonce_caller,
    salt,
    bind_authorization_value,
    enable_encryption);
  if (!hmac_result) {
    LOG(ERROR) << "Failed to initialize an authorization session delegate.";
    return TPM_RC_FAILURE;
  }
  return TPM_RC_SUCCESS;
}

TPM_RC SessionManagerImpl::EncryptSalt(const std::string& salt,
                                       std::string* encrypted_salt) {
  TPM2B_NAME out_name;
  TPM2B_NAME qualified_name;
  TPM2B_PUBLIC public_data;
  public_data.public_area.unique.rsa.size = 0;
  TPM_RC result = factory_.GetTpm()->ReadPublicSync(
      kSaltingKey, "" /*object_handle_name (not used)*/, &public_data,
      &out_name, &qualified_name, nullptr /*authorization_delegate*/);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error fetching salting key public info: "
               << GetErrorString(result);
    return result;
  }
  crypto::ScopedRSA salting_key_rsa(RSA_new());
  salting_key_rsa->e = BN_new();
  if (!salting_key_rsa->e) {
    LOG(ERROR) << "Error creating exponent for RSA: " << GetOpenSSLError();
    return TRUNKS_RC_SESSION_SETUP_ERROR;
  }
  BN_set_word(salting_key_rsa->e, kWellKnownExponent);
  salting_key_rsa->n =
      BN_bin2bn(public_data.public_area.unique.rsa.buffer,
                public_data.public_area.unique.rsa.size, nullptr);
  if (!salting_key_rsa->n) {
    LOG(ERROR) << "Error setting public area of rsa key: " << GetOpenSSLError();
    return TRUNKS_RC_SESSION_SETUP_ERROR;
  }
  crypto::ScopedEVP_PKEY salting_key(EVP_PKEY_new());
  if (!EVP_PKEY_set1_RSA(salting_key.get(), salting_key_rsa.get())) {
    LOG(ERROR) << "Error setting up EVP_PKEY: " << GetOpenSSLError();
    return TRUNKS_RC_SESSION_SETUP_ERROR;
  }
  // Label for RSAES-OAEP. Defined in TPM2.0 Part1 Architecture,
  // Appendix B.10.2.
  const size_t kOaepLabelSize = 7;
  const char kOaepLabelValue[] = "SECRET\0";
  // EVP_PKEY_CTX_set0_rsa_oaep_label takes ownership so we need to malloc.
  uint8_t* oaep_label = static_cast<uint8_t*>(OPENSSL_malloc(kOaepLabelSize));
  memcpy(oaep_label, kOaepLabelValue, kOaepLabelSize);
  crypto::ScopedEVP_PKEY_CTX salt_encrypt_context(
      EVP_PKEY_CTX_new(salting_key.get(), nullptr));
  if (!EVP_PKEY_encrypt_init(salt_encrypt_context.get()) ||
      !EVP_PKEY_CTX_set_rsa_padding(salt_encrypt_context.get(),
                                    RSA_PKCS1_OAEP_PADDING) ||
      !EVP_PKEY_CTX_set_rsa_oaep_md(salt_encrypt_context.get(), EVP_sha256()) ||
      !EVP_PKEY_CTX_set_rsa_mgf1_md(salt_encrypt_context.get(), EVP_sha256()) ||
      !EVP_PKEY_CTX_set0_rsa_oaep_label(salt_encrypt_context.get(), oaep_label,
                                        kOaepLabelSize)) {
    LOG(ERROR) << "Error setting up salt encrypt context: "
               << GetOpenSSLError();
    return TRUNKS_RC_SESSION_SETUP_ERROR;
  }
  size_t out_length = EVP_PKEY_size(salting_key.get());
  encrypted_salt->resize(out_length);
  if (!EVP_PKEY_encrypt(
          salt_encrypt_context.get(),
          reinterpret_cast<uint8_t*>(string_as_array(encrypted_salt)),
          &out_length, reinterpret_cast<const uint8_t*>(salt.data()),
          salt.size())) {
    LOG(ERROR) << "Error encrypting salt: " << GetOpenSSLError();
    return TRUNKS_RC_SESSION_SETUP_ERROR;
  }
  encrypted_salt->resize(out_length);
  return TPM_RC_SUCCESS;
}

}  // namespace trunks
