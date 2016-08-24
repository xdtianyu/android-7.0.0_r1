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

#include "attestation/common/crypto_utility_impl.h"

#include <limits>
#include <string>

#include <arpa/inet.h>
#include <base/sha1.h>
#include <base/stl_util.h>
#include <crypto/scoped_openssl_types.h>
#include <crypto/secure_util.h>
#include <crypto/sha2.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/x509.h>

namespace {

const size_t kAesKeySize = 32;
const size_t kAesBlockSize = 16;

std::string GetOpenSSLError() {
  BIO* bio = BIO_new(BIO_s_mem());
  ERR_print_errors(bio);
  char* data = nullptr;
  int data_len = BIO_get_mem_data(bio, &data);
  std::string error_string(data, data_len);
  BIO_free(bio);
  return error_string;
}

unsigned char* StringAsOpenSSLBuffer(std::string* s) {
  return reinterpret_cast<unsigned char*>(string_as_array(s));
}

}  // namespace

namespace attestation {

CryptoUtilityImpl::CryptoUtilityImpl(TpmUtility* tpm_utility)
    : tpm_utility_(tpm_utility) {
  OpenSSL_add_all_algorithms();
  ERR_load_crypto_strings();
}

CryptoUtilityImpl::~CryptoUtilityImpl() {
  EVP_cleanup();
  ERR_free_strings();
}

bool CryptoUtilityImpl::GetRandom(size_t num_bytes,
                                  std::string* random_data) const {
  // OpenSSL takes a signed integer.
  if (num_bytes > static_cast<size_t>(std::numeric_limits<int>::max())) {
    return false;
  }
  random_data->resize(num_bytes);
  unsigned char* buffer = StringAsOpenSSLBuffer(random_data);
  return (RAND_bytes(buffer, num_bytes) == 1);
}

bool CryptoUtilityImpl::CreateSealedKey(std::string* aes_key,
                                        std::string* sealed_key) {
  if (!GetRandom(kAesKeySize, aes_key)) {
    LOG(ERROR) << __func__ << ": GetRandom failed.";
    return false;
  }
  if (!tpm_utility_->SealToPCR0(*aes_key, sealed_key)) {
    LOG(ERROR) << __func__ << ": Failed to seal cipher key.";
    return false;
  }
  return true;
}

bool CryptoUtilityImpl::EncryptData(const std::string& data,
                                    const std::string& aes_key,
                                    const std::string& sealed_key,
                                    std::string* encrypted_data) {
  std::string iv;
  if (!GetRandom(kAesBlockSize, &iv)) {
    LOG(ERROR) << __func__ << ": GetRandom failed.";
    return false;
  }
  std::string raw_encrypted_data;
  if (!AesEncrypt(data, aes_key, iv, &raw_encrypted_data)) {
    LOG(ERROR) << __func__ << ": AES encryption failed.";
    return false;
  }
  EncryptedData encrypted_pb;
  encrypted_pb.set_wrapped_key(sealed_key);
  encrypted_pb.set_iv(iv);
  encrypted_pb.set_encrypted_data(raw_encrypted_data);
  encrypted_pb.set_mac(HmacSha512(iv + raw_encrypted_data, aes_key));
  if (!encrypted_pb.SerializeToString(encrypted_data)) {
    LOG(ERROR) << __func__ << ": Failed to serialize protobuf.";
    return false;
  }
  return true;
}

bool CryptoUtilityImpl::UnsealKey(const std::string& encrypted_data,
                                  std::string* aes_key,
                                  std::string* sealed_key) {
  EncryptedData encrypted_pb;
  if (!encrypted_pb.ParseFromString(encrypted_data)) {
    LOG(ERROR) << __func__ << ": Failed to parse protobuf.";
    return false;
  }
  *sealed_key = encrypted_pb.wrapped_key();
  if (!tpm_utility_->Unseal(*sealed_key, aes_key)) {
    LOG(ERROR) << __func__ << ": Cannot unseal aes key.";
    return false;
  }
  return true;
}

bool CryptoUtilityImpl::DecryptData(const std::string& encrypted_data,
                                    const std::string& aes_key,
                                    std::string* data) {
  EncryptedData encrypted_pb;
  if (!encrypted_pb.ParseFromString(encrypted_data)) {
    LOG(ERROR) << __func__ << ": Failed to parse protobuf.";
    return false;
  }
  std::string mac = HmacSha512(
      encrypted_pb.iv() + encrypted_pb.encrypted_data(),
      aes_key);
  if (mac.length() != encrypted_pb.mac().length()) {
    LOG(ERROR) << __func__ << ": Corrupted data in encrypted pb.";
    return false;
  }
  if (!crypto::SecureMemEqual(mac.data(), encrypted_pb.mac().data(),
                              mac.length())) {
    LOG(ERROR) << __func__ << ": Corrupted data in encrypted pb.";
    return false;
  }
  if (!AesDecrypt(encrypted_pb.encrypted_data(), aes_key, encrypted_pb.iv(),
                  data)) {
    LOG(ERROR) << __func__ << ": AES decryption failed.";
    return false;
  }
  return true;
}

bool CryptoUtilityImpl::GetRSASubjectPublicKeyInfo(
    const std::string& public_key,
    std::string* public_key_info) {
  auto asn1_ptr = reinterpret_cast<const unsigned char*>(public_key.data());
  crypto::ScopedRSA rsa(d2i_RSAPublicKey(nullptr, &asn1_ptr,
                                         public_key.size()));
  if (!rsa.get()) {
    LOG(ERROR) << __func__ << ": Failed to decode public key: "
               << GetOpenSSLError();
    return false;
  }
  unsigned char* buffer = nullptr;
  int length = i2d_RSA_PUBKEY(rsa.get(), &buffer);
  if (length <= 0) {
    LOG(ERROR) << __func__ << ": Failed to encode public key: "
               << GetOpenSSLError();
    return false;
  }
  crypto::ScopedOpenSSLBytes scoped_buffer(buffer);
  public_key_info->assign(reinterpret_cast<char*>(buffer), length);
  return true;
}

bool CryptoUtilityImpl::GetRSAPublicKey(const std::string& public_key_info,
                                        std::string* public_key) {
  auto asn1_ptr = reinterpret_cast<const unsigned char*>(
      public_key_info.data());
  crypto::ScopedRSA rsa(d2i_RSA_PUBKEY(NULL, &asn1_ptr,
                                       public_key_info.size()));
  if (!rsa.get()) {
    LOG(ERROR) << __func__ << ": Failed to decode public key: "
               << GetOpenSSLError();
    return false;
  }
  unsigned char* buffer = NULL;
  int length = i2d_RSAPublicKey(rsa.get(), &buffer);
  if (length <= 0) {
    LOG(ERROR) << __func__ << ": Failed to encode public key: "
               << GetOpenSSLError();
    return false;
  }
  crypto::ScopedOpenSSLBytes scoped_buffer(buffer);
  public_key->assign(reinterpret_cast<char*>(buffer), length);
  return true;
}

bool CryptoUtilityImpl::EncryptIdentityCredential(
    const std::string& credential,
    const std::string& ek_public_key_info,
    const std::string& aik_public_key,
    EncryptedIdentityCredential* encrypted) {
  const char kAlgAES256 = 9;  // This comes from TPM_ALG_AES256.
  const char kEncModeCBC = 2;  // This comes from TPM_SYM_MODE_CBC.
  const char kAsymContentHeader[] =
      {0, 0, 0, kAlgAES256, 0, kEncModeCBC, 0, kAesKeySize};
  const char kSymContentHeader[12] = {};

  // Generate an AES key and encrypt the credential.
  std::string aes_key;
  if (!GetRandom(kAesKeySize, &aes_key)) {
    LOG(ERROR) << __func__ << ": GetRandom failed.";
    return false;
  }
  std::string encrypted_credential;
  if (!TssCompatibleEncrypt(credential, aes_key, &encrypted_credential)) {
    LOG(ERROR) << __func__ << ": Failed to encrypt credential.";
    return false;
  }

  // Construct a TPM_ASYM_CA_CONTENTS structure.
  std::string asym_header(std::begin(kAsymContentHeader),
                          std::end(kAsymContentHeader));
  std::string asym_content = asym_header + aes_key +
                             base::SHA1HashString(aik_public_key);

  // Encrypt the TPM_ASYM_CA_CONTENTS with the EK public key.
  auto asn1_ptr = reinterpret_cast<const unsigned char*>(
      ek_public_key_info.data());
  crypto::ScopedRSA rsa(d2i_RSA_PUBKEY(NULL, &asn1_ptr,
                                       ek_public_key_info.size()));
  if (!rsa.get()) {
    LOG(ERROR) << __func__ << ": Failed to decode EK public key: "
               << GetOpenSSLError();
    return false;
  }
  std::string encrypted_asym_content;
  if (!TpmCompatibleOAEPEncrypt(asym_content, rsa.get(),
                                &encrypted_asym_content)) {
    LOG(ERROR) << __func__ << ": Failed to encrypt with EK public key.";
    return false;
  }

  // Construct a TPM_SYM_CA_ATTESTATION structure.
  uint32_t length = htonl(encrypted_credential.size());
  auto length_bytes = reinterpret_cast<const char*>(&length);
  std::string length_blob(length_bytes, sizeof(uint32_t));
  std::string sym_header(std::begin(kSymContentHeader),
                         std::end(kSymContentHeader));
  std::string sym_content = length_blob + sym_header + encrypted_credential;

  encrypted->set_asym_ca_contents(encrypted_asym_content);
  encrypted->set_sym_ca_attestation(sym_content);
  return true;
}

bool CryptoUtilityImpl::EncryptForUnbind(const std::string& public_key,
                                         const std::string& data,
                                         std::string* encrypted_data) {
  // Construct a TPM_BOUND_DATA structure.
  const char kBoundDataHeader[] = {1, 1, 0, 0, 2 /* TPM_PT_BIND */};
  std::string header(std::begin(kBoundDataHeader), std::end(kBoundDataHeader));
  std::string bound_data = header + data;

  // Encrypt using the TPM_ES_RSAESOAEP_SHA1_MGF1 scheme.
  auto asn1_ptr = reinterpret_cast<const unsigned char*>(public_key.data());
  crypto::ScopedRSA rsa(d2i_RSA_PUBKEY(NULL, &asn1_ptr, public_key.size()));
  if (!rsa.get()) {
    LOG(ERROR) << __func__ << ": Failed to decode public key: "
               << GetOpenSSLError();
    return false;
  }
  if (!TpmCompatibleOAEPEncrypt(bound_data, rsa.get(), encrypted_data)) {
    LOG(ERROR) << __func__ << ": Failed to encrypt with public key.";
    return false;
  }
  return true;
}

bool CryptoUtilityImpl::VerifySignature(const std::string& public_key,
                                        const std::string& data,
                                        const std::string& signature) {
  auto asn1_ptr = reinterpret_cast<const unsigned char*>(public_key.data());
  crypto::ScopedRSA rsa(d2i_RSA_PUBKEY(NULL, &asn1_ptr, public_key.size()));
  if (!rsa.get()) {
    LOG(ERROR) << __func__ << ": Failed to decode public key: "
               << GetOpenSSLError();
    return false;
  }
  std::string digest = crypto::SHA256HashString(data);
  auto digest_buffer = reinterpret_cast<const unsigned char*>(digest.data());
  std::string mutable_signature(signature);
  unsigned char* signature_buffer = StringAsOpenSSLBuffer(&mutable_signature);
  return (RSA_verify(NID_sha256, digest_buffer, digest.size(),
                     signature_buffer, signature.size(), rsa.get()) == 1);
}

bool CryptoUtilityImpl::AesEncrypt(const std::string& data,
                                   const std::string& key,
                                   const std::string& iv,
                                   std::string* encrypted_data) {
  if (key.size() != kAesKeySize || iv.size() != kAesBlockSize) {
    return false;
  }
  if (data.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
    // EVP_EncryptUpdate takes a signed int.
    return false;
  }
  std::string mutable_data(data);
  unsigned char* input_buffer = StringAsOpenSSLBuffer(&mutable_data);
  std::string mutable_key(key);
  unsigned char* key_buffer = StringAsOpenSSLBuffer(&mutable_key);
  std::string mutable_iv(iv);
  unsigned char* iv_buffer = StringAsOpenSSLBuffer(&mutable_iv);
  // Allocate enough space for the output (including padding).
  encrypted_data->resize(data.size() + kAesBlockSize);
  auto output_buffer = reinterpret_cast<unsigned char*>(
      string_as_array(encrypted_data));
  int output_size = 0;
  const EVP_CIPHER* cipher = EVP_aes_256_cbc();
  EVP_CIPHER_CTX encryption_context;
  EVP_CIPHER_CTX_init(&encryption_context);
  if (!EVP_EncryptInit_ex(&encryption_context, cipher, nullptr, key_buffer,
                          iv_buffer)) {
    LOG(ERROR) << __func__ << ": " << GetOpenSSLError();
    return false;
  }
  if (!EVP_EncryptUpdate(&encryption_context, output_buffer, &output_size,
                         input_buffer, data.size())) {
    LOG(ERROR) << __func__ << ": " << GetOpenSSLError();
    EVP_CIPHER_CTX_cleanup(&encryption_context);
    return false;
  }
  size_t total_size = output_size;
  output_buffer += output_size;
  output_size = 0;
  if (!EVP_EncryptFinal_ex(&encryption_context, output_buffer, &output_size)) {
    LOG(ERROR) << __func__ << ": " << GetOpenSSLError();
    EVP_CIPHER_CTX_cleanup(&encryption_context);
    return false;
  }
  total_size += output_size;
  encrypted_data->resize(total_size);
  EVP_CIPHER_CTX_cleanup(&encryption_context);
  return true;
}

bool CryptoUtilityImpl::AesDecrypt(const std::string& encrypted_data,
                                   const std::string& key,
                                   const std::string& iv,
                                   std::string* data) {
  if (key.size() != kAesKeySize || iv.size() != kAesBlockSize) {
    return false;
  }
  if (encrypted_data.size() >
      static_cast<size_t>(std::numeric_limits<int>::max())) {
    // EVP_DecryptUpdate takes a signed int.
    return false;
  }
  std::string mutable_encrypted_data(encrypted_data);
  unsigned char* input_buffer = StringAsOpenSSLBuffer(&mutable_encrypted_data);
  std::string mutable_key(key);
  unsigned char* key_buffer = StringAsOpenSSLBuffer(&mutable_key);
  std::string mutable_iv(iv);
  unsigned char* iv_buffer = StringAsOpenSSLBuffer(&mutable_iv);
  // Allocate enough space for the output.
  data->resize(encrypted_data.size());
  unsigned char* output_buffer = StringAsOpenSSLBuffer(data);
  int output_size = 0;
  const EVP_CIPHER* cipher = EVP_aes_256_cbc();
  EVP_CIPHER_CTX decryption_context;
  EVP_CIPHER_CTX_init(&decryption_context);
  if (!EVP_DecryptInit_ex(&decryption_context, cipher, nullptr, key_buffer,
                          iv_buffer)) {
    LOG(ERROR) << __func__ << ": " << GetOpenSSLError();
    return false;
  }
  if (!EVP_DecryptUpdate(&decryption_context, output_buffer, &output_size,
                         input_buffer, encrypted_data.size())) {
    LOG(ERROR) << __func__ << ": " << GetOpenSSLError();
    EVP_CIPHER_CTX_cleanup(&decryption_context);
    return false;
  }
  size_t total_size = output_size;
  output_buffer += output_size;
  output_size = 0;
  if (!EVP_DecryptFinal_ex(&decryption_context, output_buffer, &output_size)) {
    LOG(ERROR) << __func__ << ": " << GetOpenSSLError();
    EVP_CIPHER_CTX_cleanup(&decryption_context);
    return false;
  }
  total_size += output_size;
  data->resize(total_size);
  EVP_CIPHER_CTX_cleanup(&decryption_context);
  return true;
}

std::string CryptoUtilityImpl::HmacSha512(const std::string& data,
                                          const std::string& key) {
  unsigned char mac[SHA512_DIGEST_LENGTH];
  std::string mutable_data(data);
  unsigned char* data_buffer = StringAsOpenSSLBuffer(&mutable_data);
  HMAC(EVP_sha512(), key.data(), key.size(), data_buffer, data.size(), mac,
       nullptr);
  return std::string(std::begin(mac), std::end(mac));
}

bool CryptoUtilityImpl::TssCompatibleEncrypt(const std::string& input,
                                             const std::string& key,
                                             std::string* output) {
  CHECK(output);
  CHECK_EQ(key.size(), kAesKeySize);
  std::string iv;
  if (!GetRandom(kAesBlockSize, &iv)) {
    LOG(ERROR) << __func__ << ": GetRandom failed.";
    return false;
  }
  std::string encrypted;
  if (!AesEncrypt(input, key, iv, &encrypted)) {
    LOG(ERROR) << __func__ << ": Encryption failed.";
    return false;
  }
  *output = iv + encrypted;
  return true;
}

bool CryptoUtilityImpl::TpmCompatibleOAEPEncrypt(const std::string& input,
                                                 RSA* key,
                                                 std::string* output) {
  CHECK(output);
  // The custom OAEP parameter as specified in TPM Main Part 1, Section 31.1.1.
  const unsigned char oaep_param[4] = {'T', 'C', 'P', 'A'};
  std::string padded_input;
  padded_input.resize(RSA_size(key));
  auto padded_buffer = reinterpret_cast<unsigned char*>(
      string_as_array(&padded_input));
  auto input_buffer = reinterpret_cast<const unsigned char*>(input.data());
  int result = RSA_padding_add_PKCS1_OAEP(padded_buffer, padded_input.size(),
                                          input_buffer, input.size(),
                                          oaep_param, arraysize(oaep_param));
  if (!result) {
    LOG(ERROR) << __func__ << ": Failed to add OAEP padding: "
               << GetOpenSSLError();
    return false;
  }
  output->resize(padded_input.size());
  auto output_buffer = reinterpret_cast<unsigned char*>(
      string_as_array(output));
  result = RSA_public_encrypt(padded_input.size(), padded_buffer,
                              output_buffer, key, RSA_NO_PADDING);
  if (result == -1) {
    LOG(ERROR) << __func__ << ": Failed to encrypt OAEP padded input: "
               << GetOpenSSLError();
    return false;
  }
  return true;
}

}  // namespace attestation
