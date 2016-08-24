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

#include "attestation/server/pkcs11_key_store.h"

#include <memory>
#include <string>

#include <base/bind.h>
#include <base/callback.h>
#include <base/files/file_path.h>
#include <base/logging.h>
#include <base/stl_util.h>
#include <base/strings/string_util.h>
#include <chaps/isolate.h>
#include <chaps/pkcs11/cryptoki.h>
#include <chaps/token_manager_client.h>
#include <brillo/cryptohome.h>
#include <crypto/scoped_openssl_types.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/x509.h>

namespace {

std::string Sha1(const std::string& input) {
  unsigned char output[SHA_DIGEST_LENGTH];
  SHA1(reinterpret_cast<const unsigned char*>(input.data()), input.size(),
       output);
  return std::string(reinterpret_cast<char*>(output), SHA_DIGEST_LENGTH);
}

}  // namespace

namespace attestation {

typedef crypto::ScopedOpenSSL<X509, X509_free> ScopedX509;

// An arbitrary application ID to identify PKCS #11 objects.
const char kApplicationID[] = "CrOS_d5bbc079d2497110feadfc97c40d718ae46f4658";

// A helper class to scope a PKCS #11 session.
class ScopedSession {
 public:
  explicit ScopedSession(CK_SLOT_ID slot) : handle_(CK_INVALID_HANDLE) {
    CK_RV rv = C_Initialize(nullptr);
    if (rv != CKR_OK && rv != CKR_CRYPTOKI_ALREADY_INITIALIZED) {
      // This may be normal in a test environment.
      LOG(INFO) << "PKCS #11 is not available.";
      return;
    }
    CK_FLAGS flags = CKF_RW_SESSION | CKF_SERIAL_SESSION;
    if (C_OpenSession(slot, flags, nullptr, nullptr, &handle_) != CKR_OK) {
      LOG(ERROR) << "Failed to open PKCS #11 session.";
      return;
    }
  }

  ~ScopedSession() {
    if (IsValid() && (C_CloseSession(handle_) != CKR_OK)) {
      LOG(WARNING) << "Failed to close PKCS #11 session.";
      handle_ = CK_INVALID_HANDLE;
    }
  }

  CK_SESSION_HANDLE handle() const {
    return handle_;
  }

  bool IsValid() const {
    return (handle_ != CK_INVALID_HANDLE);
  }

 private:
  CK_SESSION_HANDLE handle_;

  DISALLOW_COPY_AND_ASSIGN(ScopedSession);
};

Pkcs11KeyStore::Pkcs11KeyStore(chaps::TokenManagerClient* token_manager)
    : token_manager_(token_manager) {}

Pkcs11KeyStore::~Pkcs11KeyStore() {}

bool Pkcs11KeyStore::Read(const std::string& username,
                          const std::string& key_name,
                          std::string* key_data) {
  CK_SLOT_ID slot;
  if (!GetUserSlot(username, &slot)) {
    LOG(ERROR) << "Pkcs11KeyStore: No token for user.";
    return false;
  }
  ScopedSession session(slot);
  if (!session.IsValid()) {
    LOG(ERROR) << "Pkcs11KeyStore: Failed to open token session.";
    return false;
  }
  CK_OBJECT_HANDLE key_handle = FindObject(session.handle(), key_name);
  if (key_handle == CK_INVALID_HANDLE) {
    LOG(WARNING) << "Pkcs11KeyStore: Key does not exist: " << key_name;
    return false;
  }
  // First get the attribute with a NULL buffer which will give us the length.
  CK_ATTRIBUTE attribute = {CKA_VALUE, nullptr, 0};
  if (C_GetAttributeValue(session.handle(),
                          key_handle,
                          &attribute, 1) != CKR_OK) {
    LOG(ERROR) << "Pkcs11KeyStore: Failed to read key data: " << key_name;
    return false;
  }
  key_data->resize(attribute.ulValueLen);
  attribute.pValue = string_as_array(key_data);
  if (C_GetAttributeValue(session.handle(),
                          key_handle,
                          &attribute, 1) != CKR_OK) {
    LOG(ERROR) << "Pkcs11KeyStore: Failed to read key data: " << key_name;
    return false;
  }
  key_data->resize(attribute.ulValueLen);
  return true;
}

bool Pkcs11KeyStore::Write(const std::string& username,
                           const std::string& key_name,
                           const std::string& key_data) {
  // Delete any existing key with the same name.
  if (!Delete(username, key_name)) {
    return false;
  }
  CK_SLOT_ID slot;
  if (!GetUserSlot(username, &slot)) {
    LOG(ERROR) << "Pkcs11KeyStore: No token for user.";
    return false;
  }
  ScopedSession session(slot);
  if (!session.IsValid()) {
    LOG(ERROR) << "Pkcs11KeyStore: Failed to open token session.";
    return false;
  }
  std::string mutable_key_name(key_name);
  std::string mutable_key_data(key_data);
  std::string mutable_application_id(kApplicationID);
  // Create a new data object for the key.
  CK_OBJECT_CLASS object_class = CKO_DATA;
  CK_BBOOL true_value = CK_TRUE;
  CK_BBOOL false_value = CK_FALSE;
  CK_ATTRIBUTE attributes[] = {
    {CKA_CLASS, &object_class, sizeof(object_class)},
    {
      CKA_LABEL,
      string_as_array(&mutable_key_name),
      mutable_key_name.size()
    },
    {
      CKA_VALUE,
      string_as_array(&mutable_key_data),
      mutable_key_data.size()
    },
    {
      CKA_APPLICATION,
      string_as_array(&mutable_application_id),
      mutable_application_id.size()
    },
    {CKA_TOKEN, &true_value, sizeof(true_value)},
    {CKA_PRIVATE, &true_value, sizeof(true_value)},
    {CKA_MODIFIABLE, &false_value, sizeof(false_value)}
  };
  CK_OBJECT_HANDLE key_handle = CK_INVALID_HANDLE;
  if (C_CreateObject(session.handle(),
                     attributes,
                     arraysize(attributes),
                     &key_handle) != CKR_OK) {
    LOG(ERROR) << "Pkcs11KeyStore: Failed to write key data: " << key_name;
    return false;
  }
  return true;
}

bool Pkcs11KeyStore::Delete(const std::string& username,
                            const std::string& key_name) {
  CK_SLOT_ID slot;
  if (!GetUserSlot(username, &slot)) {
    LOG(ERROR) << "Pkcs11KeyStore: No token for user.";
    return false;
  }
  ScopedSession session(slot);
  if (!session.IsValid()) {
    LOG(ERROR) << "Pkcs11KeyStore: Failed to open token session.";
    return false;
  }
  CK_OBJECT_HANDLE key_handle = FindObject(session.handle(), key_name);
  if (key_handle != CK_INVALID_HANDLE) {
    if (C_DestroyObject(session.handle(), key_handle) != CKR_OK) {
      LOG(ERROR) << "Pkcs11KeyStore: Failed to delete key data.";
      return false;
    }
  }
  return true;
}

bool Pkcs11KeyStore::DeleteByPrefix(const std::string& username,
                                    const std::string& key_prefix) {
  CK_SLOT_ID slot;
  if (!GetUserSlot(username, &slot)) {
    LOG(ERROR) << "Pkcs11KeyStore: No token for user.";
    return false;
  }
  ScopedSession session(slot);
  if (!session.IsValid()) {
    LOG(ERROR) << "Pkcs11KeyStore: Failed to open token session.";
    return false;
  }
  EnumObjectsCallback callback = base::Bind(
      &Pkcs11KeyStore::DeleteIfMatchesPrefix,
      base::Unretained(this),
      session.handle(),
      key_prefix);
  if (!EnumObjects(session.handle(), callback)) {
    LOG(ERROR) << "Pkcs11KeyStore: Failed to delete key data.";
    return false;
  }
  return true;
}

bool Pkcs11KeyStore::Register(const std::string& username,
                              const std::string& label,
                              KeyType key_type,
                              KeyUsage key_usage,
                              const std::string& private_key_blob,
                              const std::string& public_key_der,
                              const std::string& certificate) {
  const CK_ATTRIBUTE_TYPE kKeyBlobAttribute = CKA_VENDOR_DEFINED + 1;

  if (key_type != KEY_TYPE_RSA) {
    LOG(ERROR) << "Pkcs11KeyStore: Only RSA supported.";
    return false;
  }
  CK_SLOT_ID slot;
  if (!GetUserSlot(username, &slot)) {
    LOG(ERROR) << "Pkcs11KeyStore: No token for user.";
    return false;
  }
  ScopedSession session(slot);
  if (!session.IsValid()) {
    LOG(ERROR) << "Pkcs11KeyStore: Failed to open token session.";
    return false;
  }

  // Extract the modulus from the public key.
  const unsigned char* asn1_ptr = reinterpret_cast<const unsigned char*>(
      public_key_der.data());
  crypto::ScopedRSA public_key(d2i_RSAPublicKey(nullptr,
                                                &asn1_ptr,
                                                public_key_der.size()));
  if (!public_key.get()) {
    LOG(ERROR) << "Pkcs11KeyStore: Failed to decode public key.";
    return false;
  }
  std::string modulus(BN_num_bytes(public_key.get()->n), 0);
  int length = BN_bn2bin(public_key.get()->n, reinterpret_cast<unsigned char*>(
      string_as_array(&modulus)));
  if (length <= 0) {
    LOG(ERROR) << "Pkcs11KeyStore: Failed to extract public key modulus.";
    return false;
  }
  modulus.resize(length);

  // Construct a PKCS #11 template for the public key object.
  CK_BBOOL true_value = CK_TRUE;
  CK_BBOOL false_value = CK_FALSE;
  CK_KEY_TYPE p11_key_type = CKK_RSA;
  CK_OBJECT_CLASS public_key_class = CKO_PUBLIC_KEY;
  std::string id = Sha1(modulus);
  std::string mutable_label(label);
  CK_ULONG modulus_bits = modulus.size() * 8;
  CK_BBOOL sign_usage = (key_usage == KEY_USAGE_SIGN);
  CK_BBOOL decrypt_usage = (key_usage == KEY_USAGE_DECRYPT);
  unsigned char public_exponent[] = {1, 0, 1};
  CK_ATTRIBUTE public_key_attributes[] = {
    {CKA_CLASS, &public_key_class, sizeof(public_key_class)},
    {CKA_TOKEN, &true_value, sizeof(true_value)},
    {CKA_DERIVE, &false_value, sizeof(false_value)},
    {CKA_WRAP, &false_value, sizeof(false_value)},
    {CKA_VERIFY, &sign_usage, sizeof(sign_usage)},
    {CKA_VERIFY_RECOVER, &false_value, sizeof(false_value)},
    {CKA_ENCRYPT, &decrypt_usage, sizeof(decrypt_usage)},
    {CKA_KEY_TYPE, &p11_key_type, sizeof(p11_key_type)},
    {CKA_ID, string_as_array(&id), id.size()},
    {CKA_LABEL, string_as_array(&mutable_label), mutable_label.size()},
    {CKA_MODULUS_BITS, &modulus_bits, sizeof(modulus_bits)},
    {CKA_PUBLIC_EXPONENT, public_exponent, arraysize(public_exponent)},
    {CKA_MODULUS, string_as_array(&modulus), modulus.size()}
  };

  CK_OBJECT_HANDLE object_handle = CK_INVALID_HANDLE;
  if (C_CreateObject(session.handle(),
                     public_key_attributes,
                     arraysize(public_key_attributes),
                     &object_handle) != CKR_OK) {
    LOG(ERROR) << "Pkcs11KeyStore: Failed to create public key object.";
    return false;
  }

  // Construct a PKCS #11 template for the private key object.
  std::string mutable_private_key_blob(private_key_blob);
  CK_OBJECT_CLASS private_key_class = CKO_PRIVATE_KEY;
  CK_ATTRIBUTE private_key_attributes[] = {
    {CKA_CLASS, &private_key_class, sizeof(private_key_class)},
    {CKA_TOKEN, &true_value, sizeof(true_value)},
    {CKA_PRIVATE, &true_value, sizeof(true_value)},
    {CKA_SENSITIVE, &true_value, sizeof(true_value)},
    {CKA_EXTRACTABLE, &false_value, sizeof(false_value)},
    {CKA_DERIVE, &false_value, sizeof(false_value)},
    {CKA_UNWRAP, &false_value, sizeof(false_value)},
    {CKA_SIGN, &sign_usage, sizeof(sign_usage)},
    {CKA_SIGN_RECOVER, &false_value, sizeof(false_value)},
    {CKA_DECRYPT, &decrypt_usage, sizeof(decrypt_usage)},
    {CKA_KEY_TYPE, &p11_key_type, sizeof(p11_key_type)},
    {CKA_ID, string_as_array(&id), id.size()},
    {CKA_LABEL, string_as_array(&mutable_label), mutable_label.size()},
    {CKA_PUBLIC_EXPONENT, public_exponent, arraysize(public_exponent)},
    {CKA_MODULUS, string_as_array(&modulus), modulus.size()},
    {
      kKeyBlobAttribute,
      string_as_array(&mutable_private_key_blob),
      mutable_private_key_blob.size()
    }
  };

  if (C_CreateObject(session.handle(),
                     private_key_attributes,
                     arraysize(private_key_attributes),
                     &object_handle) != CKR_OK) {
    LOG(ERROR) << "Pkcs11KeyStore: Failed to create private key object.";
    return false;
  }

  if (!certificate.empty()) {
    std::string subject;
    std::string issuer;
    std::string serial_number;
    if (!GetCertificateFields(certificate, &subject, &issuer, &serial_number)) {
      LOG(WARNING) << "Pkcs11KeyStore: Failed to find certificate fields.";
    }
    // Construct a PKCS #11 template for a certificate object.
    std::string mutable_certificate = certificate;
    CK_OBJECT_CLASS certificate_class = CKO_CERTIFICATE;
    CK_CERTIFICATE_TYPE certificate_type = CKC_X_509;
    CK_ATTRIBUTE certificate_attributes[] = {
      {CKA_CLASS, &certificate_class, sizeof(certificate_class)},
      {CKA_TOKEN, &true_value, sizeof(true_value)},
      {CKA_PRIVATE, &false_value, sizeof(false_value)},
      {CKA_ID, string_as_array(&id), id.size()},
      {CKA_LABEL, string_as_array(&mutable_label), mutable_label.size()},
      {CKA_CERTIFICATE_TYPE, &certificate_type, sizeof(certificate_type)},
      {CKA_SUBJECT, string_as_array(&subject), subject.size()},
      {CKA_ISSUER, string_as_array(&issuer), issuer.size()},
      {
        CKA_SERIAL_NUMBER,
        string_as_array(&serial_number),
        serial_number.size()
      },
      {
        CKA_VALUE,
        string_as_array(&mutable_certificate),
        mutable_certificate.size()
      }
    };

    if (C_CreateObject(session.handle(),
                       certificate_attributes,
                       arraysize(certificate_attributes),
                       &object_handle) != CKR_OK) {
      LOG(ERROR) << "Pkcs11KeyStore: Failed to create certificate object.";
      return false;
    }
  }

  // Close all sessions in an attempt to trigger other modules to find the new
  // objects.
  C_CloseAllSessions(slot);

  return true;
}

bool Pkcs11KeyStore::RegisterCertificate(const std::string& username,
                                         const std::string& certificate) {
  CK_SLOT_ID slot;
  if (!GetUserSlot(username, &slot)) {
    LOG(ERROR) << "Pkcs11KeyStore: No token for user.";
    return false;
  }
  ScopedSession session(slot);
  if (!session.IsValid()) {
    LOG(ERROR) << "Pkcs11KeyStore: Failed to open token session.";
    return false;
  }

  if (DoesCertificateExist(session.handle(), certificate)) {
    LOG(INFO) << "Pkcs11KeyStore: Certificate already exists.";
    return true;
  }
  std::string subject;
  std::string issuer;
  std::string serial_number;
  if (!GetCertificateFields(certificate, &subject, &issuer, &serial_number)) {
    LOG(WARNING) << "Pkcs11KeyStore: Failed to find certificate fields.";
  }
  // Construct a PKCS #11 template for a certificate object.
  std::string mutable_certificate = certificate;
  CK_OBJECT_CLASS certificate_class = CKO_CERTIFICATE;
  CK_CERTIFICATE_TYPE certificate_type = CKC_X_509;
  CK_BBOOL true_value = CK_TRUE;
  CK_BBOOL false_value = CK_FALSE;
  CK_ATTRIBUTE certificate_attributes[] = {
    {CKA_CLASS, &certificate_class, sizeof(certificate_class)},
    {CKA_TOKEN, &true_value, sizeof(true_value)},
    {CKA_PRIVATE, &false_value, sizeof(false_value)},
    {CKA_CERTIFICATE_TYPE, &certificate_type, sizeof(certificate_type)},
    {CKA_SUBJECT, string_as_array(&subject), subject.size()},
    {CKA_ISSUER, string_as_array(&issuer), issuer.size()},
    {CKA_SERIAL_NUMBER, string_as_array(&serial_number), serial_number.size()},
    {
      CKA_VALUE,
      string_as_array(&mutable_certificate),
      mutable_certificate.size()
    }
  };
  CK_OBJECT_HANDLE object_handle = CK_INVALID_HANDLE;
  if (C_CreateObject(session.handle(),
                     certificate_attributes,
                     arraysize(certificate_attributes),
                     &object_handle) != CKR_OK) {
    LOG(ERROR) << "Pkcs11KeyStore: Failed to create certificate object.";
    return false;
  }
  return true;
}

CK_OBJECT_HANDLE Pkcs11KeyStore::FindObject(CK_SESSION_HANDLE session_handle,
                                            const std::string& key_name) {
  // Assemble a search template.
  std::string mutable_key_name(key_name);
  std::string mutable_application_id(kApplicationID);
  CK_OBJECT_CLASS object_class = CKO_DATA;
  CK_BBOOL true_value = CK_TRUE;
  CK_BBOOL false_value = CK_FALSE;
  CK_ATTRIBUTE attributes[] = {
    {CKA_CLASS, &object_class, sizeof(object_class)},
    {
      CKA_LABEL,
      string_as_array(&mutable_key_name),
      mutable_key_name.size()
    },
    {
      CKA_APPLICATION,
      string_as_array(&mutable_application_id),
      mutable_application_id.size()
    },
    {CKA_TOKEN, &true_value, sizeof(true_value)},
    {CKA_PRIVATE, &true_value, sizeof(true_value)},
    {CKA_MODIFIABLE, &false_value, sizeof(false_value)}
  };
  CK_OBJECT_HANDLE key_handle = CK_INVALID_HANDLE;
  CK_ULONG count = 0;
  if ((C_FindObjectsInit(session_handle,
                         attributes,
                         arraysize(attributes)) != CKR_OK) ||
      (C_FindObjects(session_handle, &key_handle, 1, &count) != CKR_OK) ||
      (C_FindObjectsFinal(session_handle) != CKR_OK)) {
    LOG(ERROR) << "Key search failed: " << key_name;
    return CK_INVALID_HANDLE;
  }
  if (count == 1)
    return key_handle;
  return CK_INVALID_HANDLE;
}

bool Pkcs11KeyStore::GetUserSlot(const std::string& username,
                                 CK_SLOT_ID_PTR slot) {
  const char kChapsDaemonName[] = "chaps";
  const char kChapsSystemToken[] = "/var/lib/chaps";
  base::FilePath token_path = username.empty() ?
      base::FilePath(kChapsSystemToken) :
      brillo::cryptohome::home::GetDaemonPath(username, kChapsDaemonName);
  CK_RV rv;
  rv = C_Initialize(nullptr);
  if (rv != CKR_OK && rv != CKR_CRYPTOKI_ALREADY_INITIALIZED) {
    LOG(WARNING) << __func__ << ": C_Initialize failed.";
    return false;
  }
  CK_ULONG num_slots = 0;
  rv = C_GetSlotList(CK_TRUE, nullptr, &num_slots);
  if (rv != CKR_OK) {
    LOG(WARNING) << __func__ << ": C_GetSlotList(nullptr) failed.";
    return false;
  }
  std::unique_ptr<CK_SLOT_ID[]> slot_list(new CK_SLOT_ID[num_slots]);
  rv = C_GetSlotList(CK_TRUE, slot_list.get(), &num_slots);
  if (rv != CKR_OK) {
    LOG(WARNING) << __func__ << ": C_GetSlotList failed.";
    return false;
  }
  // Look through all slots for |token_path|.
  for (CK_ULONG i = 0; i < num_slots; ++i) {
    base::FilePath slot_path;
    if (token_manager_->GetTokenPath(
        chaps::IsolateCredentialManager::GetDefaultIsolateCredential(),
        slot_list[i],
        &slot_path) && (token_path == slot_path)) {
      *slot = slot_list[i];
      return true;
    }
  }
  LOG(WARNING) << __func__ << ": Path not found.";
  return false;
}

bool Pkcs11KeyStore::EnumObjects(
    CK_SESSION_HANDLE session_handle,
    const Pkcs11KeyStore::EnumObjectsCallback& callback) {
  std::string mutable_application_id(kApplicationID);
  // Assemble a search template.
  CK_OBJECT_CLASS object_class = CKO_DATA;
  CK_BBOOL true_value = CK_TRUE;
  CK_BBOOL false_value = CK_FALSE;
  CK_ATTRIBUTE attributes[] = {
    {CKA_CLASS, &object_class, sizeof(object_class)},
    {
      CKA_APPLICATION,
      string_as_array(&mutable_application_id),
      mutable_application_id.size()
    },
    {CKA_TOKEN, &true_value, sizeof(true_value)},
    {CKA_PRIVATE, &true_value, sizeof(true_value)},
    {CKA_MODIFIABLE, &false_value, sizeof(false_value)}
  };
  const CK_ULONG kMaxHandles = 100;  // Arbitrary.
  CK_OBJECT_HANDLE handles[kMaxHandles];
  CK_ULONG count = 0;
  if ((C_FindObjectsInit(session_handle,
                         attributes,
                         arraysize(attributes)) != CKR_OK) ||
      (C_FindObjects(session_handle, handles, kMaxHandles, &count) != CKR_OK)) {
    LOG(ERROR) << "Key search failed.";
    return false;
  }
  while (count > 0) {
    for (CK_ULONG i = 0; i < count; ++i) {
      std::string key_name;
      if (!GetKeyName(session_handle, handles[i], &key_name)) {
        LOG(WARNING) << "Found key object but failed to get name.";
        continue;
      }
      if (!callback.Run(key_name, handles[i]))
        return false;
    }
    if (C_FindObjects(session_handle, handles, kMaxHandles, &count) != CKR_OK) {
      LOG(ERROR) << "Key search continuation failed.";
      return false;
    }
  }
  if (C_FindObjectsFinal(session_handle) != CKR_OK) {
    LOG(WARNING) << "Failed to finalize key search.";
  }
  return true;
}

bool Pkcs11KeyStore::GetKeyName(CK_SESSION_HANDLE session_handle,
                                CK_OBJECT_HANDLE object_handle,
                                std::string* key_name) {
  CK_ATTRIBUTE attribute = {CKA_LABEL, nullptr, 0};
  if (C_GetAttributeValue(session_handle, object_handle, &attribute, 1) !=
      CKR_OK) {
    LOG(ERROR) << "C_GetAttributeValue(CKA_LABEL) [length] failed.";
    return false;
  }
  key_name->resize(attribute.ulValueLen);
  attribute.pValue = string_as_array(key_name);
  if (C_GetAttributeValue(session_handle, object_handle, &attribute, 1) !=
      CKR_OK) {
    LOG(ERROR) << "C_GetAttributeValue(CKA_LABEL) failed.";
    return false;
  }
  return true;
}

bool Pkcs11KeyStore::DeleteIfMatchesPrefix(CK_SESSION_HANDLE session_handle,
                                           const std::string& key_prefix,
                                           const std::string& key_name,
                                           CK_OBJECT_HANDLE object_handle) {
  if (base::StartsWith(key_name, key_prefix, base::CompareCase::SENSITIVE)) {
    if (C_DestroyObject(session_handle, object_handle) != CKR_OK) {
      LOG(ERROR) << "C_DestroyObject failed.";
      return false;
    }
  }
  return true;
}

bool Pkcs11KeyStore::GetCertificateFields(const std::string& certificate,
                                          std::string* subject,
                                          std::string* issuer,
                                          std::string* serial_number) {
  const unsigned char* asn1_ptr = reinterpret_cast<const unsigned char*>(
      certificate.data());
  ScopedX509 x509(d2i_X509(nullptr, &asn1_ptr, certificate.size()));
  if (!x509.get() || !x509->cert_info || !x509->cert_info->subject) {
    LOG(WARNING) << "Pkcs11KeyStore: Failed to decode certificate.";
    return false;
  }
  unsigned char* subject_buffer = nullptr;
  int length = i2d_X509_NAME(x509->cert_info->subject, &subject_buffer);
  crypto::ScopedOpenSSLBytes scoped_subject_buffer(subject_buffer);
  if (length <= 0) {
    LOG(WARNING) << "Pkcs11KeyStore: Failed to encode certificate subject.";
    return false;
  }
  subject->assign(reinterpret_cast<char*>(subject_buffer), length);

  unsigned char* issuer_buffer = nullptr;
  length = i2d_X509_NAME(x509->cert_info->issuer, &issuer_buffer);
  crypto::ScopedOpenSSLBytes scoped_issuer_buffer(issuer_buffer);
  if (length <= 0) {
    LOG(WARNING) << "Pkcs11KeyStore: Failed to encode certificate issuer.";
    return false;
  }
  issuer->assign(reinterpret_cast<char*>(issuer_buffer), length);

  unsigned char* serial_number_buffer = nullptr;
  length = i2d_ASN1_INTEGER(x509->cert_info->serialNumber,
                            &serial_number_buffer);
  crypto::ScopedOpenSSLBytes scoped_serial_number_buffer(serial_number_buffer);
  if (length <= 0) {
    LOG(WARNING) << "Pkcs11KeyStore: Failed to encode certificate serial "
                    "number.";
    return false;
  }
  serial_number->assign(reinterpret_cast<char*>(serial_number_buffer), length);
  return true;
}

bool Pkcs11KeyStore::DoesCertificateExist(
    CK_SESSION_HANDLE session_handle,
    const std::string& certificate) {
  CK_OBJECT_CLASS object_class = CKO_CERTIFICATE;
  CK_BBOOL true_value = CK_TRUE;
  CK_BBOOL false_value = CK_FALSE;
  std::string mutable_certificate = certificate;
  CK_ATTRIBUTE attributes[] = {
    {CKA_CLASS, &object_class, sizeof(object_class)},
    {CKA_TOKEN, &true_value, sizeof(true_value)},
    {CKA_PRIVATE, &false_value, sizeof(false_value)},
    {
      CKA_VALUE,
      string_as_array(&mutable_certificate),
      mutable_certificate.size()
    }
  };
  CK_OBJECT_HANDLE object_handle = CK_INVALID_HANDLE;
  CK_ULONG count = 0;
  if ((C_FindObjectsInit(session_handle,
                         attributes,
                         arraysize(attributes)) != CKR_OK) ||
      (C_FindObjects(session_handle, &object_handle, 1, &count) != CKR_OK) ||
      (C_FindObjectsFinal(session_handle) != CKR_OK)) {
    return false;
  }
  return (count > 0);
}

}  // namespace attestation
