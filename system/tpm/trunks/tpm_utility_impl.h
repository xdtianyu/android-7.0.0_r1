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

#ifndef TRUNKS_TPM_UTILITY_IMPL_H_
#define TRUNKS_TPM_UTILITY_IMPL_H_

#include "trunks/tpm_utility.h"

#include <map>
#include <string>

#include <base/macros.h>
#include <base/memory/scoped_ptr.h>
#include <gtest/gtest_prod.h>

#include "trunks/trunks_export.h"

namespace trunks {

class AuthorizationDelegate;
class TrunksFactory;

// A default implementation of TpmUtility.
class TRUNKS_EXPORT TpmUtilityImpl : public TpmUtility {
 public:
  explicit TpmUtilityImpl(const TrunksFactory& factory);
  ~TpmUtilityImpl() override;

  // TpmUtility methods.
  TPM_RC Startup() override;
  TPM_RC Clear() override;
  void Shutdown() override;
  TPM_RC InitializeTpm() override;
  TPM_RC AllocatePCR(const std::string& platform_password) override;
  TPM_RC TakeOwnership(const std::string& owner_password,
                       const std::string& endorsement_password,
                       const std::string& lockout_password) override;
  TPM_RC StirRandom(const std::string& entropy_data,
                    AuthorizationDelegate* delegate) override;
  TPM_RC GenerateRandom(size_t num_bytes,
                        AuthorizationDelegate* delegate,
                        std::string* random_data) override;
  TPM_RC ExtendPCR(int pcr_index,
                   const std::string& extend_data,
                   AuthorizationDelegate* delegate) override;
  TPM_RC ReadPCR(int pcr_index, std::string* pcr_value) override;
  TPM_RC AsymmetricEncrypt(TPM_HANDLE key_handle,
                           TPM_ALG_ID scheme,
                           TPM_ALG_ID hash_alg,
                           const std::string& plaintext,
                           AuthorizationDelegate* delegate,
                           std::string* ciphertext) override;
  TPM_RC AsymmetricDecrypt(TPM_HANDLE key_handle,
                           TPM_ALG_ID scheme,
                           TPM_ALG_ID hash_alg,
                           const std::string& ciphertext,
                           AuthorizationDelegate* delegate,
                           std::string* plaintext) override;
  TPM_RC Sign(TPM_HANDLE key_handle,
              TPM_ALG_ID scheme,
              TPM_ALG_ID hash_alg,
              const std::string& plaintext,
              AuthorizationDelegate* delegate,
              std::string* signature) override;
  TPM_RC Verify(TPM_HANDLE key_handle,
                TPM_ALG_ID scheme,
                TPM_ALG_ID hash_alg,
                const std::string& plaintext,
                const std::string& signature,
                AuthorizationDelegate* delegate) override;
  TPM_RC CertifyCreation(TPM_HANDLE key_handle,
                         const std::string& creation_blob) override;
  TPM_RC ChangeKeyAuthorizationData(TPM_HANDLE key_handle,
                                    const std::string& new_password,
                                    AuthorizationDelegate* delegate,
                                    std::string* key_blob) override;
  TPM_RC ImportRSAKey(AsymmetricKeyUsage key_type,
                      const std::string& modulus,
                      uint32_t public_exponent,
                      const std::string& prime_factor,
                      const std::string& password,
                      AuthorizationDelegate* delegate,
                      std::string* key_blob) override;
  TPM_RC CreateRSAKeyPair(AsymmetricKeyUsage key_type,
                          int modulus_bits,
                          uint32_t public_exponent,
                          const std::string& password,
                          const std::string& policy_digest,
                          bool use_only_policy_authorization,
                          int creation_pcr_index,
                          AuthorizationDelegate* delegate,
                          std::string* key_blob,
                          std::string* creation_blob) override;
  TPM_RC LoadKey(const std::string& key_blob,
                 AuthorizationDelegate* delegate,
                 TPM_HANDLE* key_handle) override;
  TPM_RC GetKeyName(TPM_HANDLE handle, std::string* name) override;
  TPM_RC GetKeyPublicArea(TPM_HANDLE handle,
                          TPMT_PUBLIC* public_data) override;
  TPM_RC SealData(const std::string& data_to_seal,
                  const std::string& policy_digest,
                  AuthorizationDelegate* delegate,
                  std::string* sealed_data) override;
  TPM_RC UnsealData(const std::string& sealed_data,
                    AuthorizationDelegate* delegate,
                    std::string* unsealed_data) override;
  TPM_RC StartSession(HmacSession* session) override;
  TPM_RC GetPolicyDigestForPcrValue(int pcr_index,
                                    const std::string& pcr_value,
                                    std::string* policy_digest) override;
  TPM_RC DefineNVSpace(uint32_t index,
                       size_t num_bytes,
                       AuthorizationDelegate* delegate) override;
  TPM_RC DestroyNVSpace(uint32_t index,
                        AuthorizationDelegate* delegate) override;
  TPM_RC LockNVSpace(uint32_t index, AuthorizationDelegate* delegate) override;
  TPM_RC WriteNVSpace(uint32_t index,
                      uint32_t offset,
                      const std::string& nvram_data,
                      AuthorizationDelegate* delegate) override;
  TPM_RC ReadNVSpace(uint32_t index,
                     uint32_t offset,
                     size_t num_bytes,
                     std::string* nvram_data,
                     AuthorizationDelegate* delegate) override;
  TPM_RC GetNVSpaceName(uint32_t index, std::string* name) override;
  TPM_RC GetNVSpacePublicArea(uint32_t index,
                              TPMS_NV_PUBLIC* public_data) override;

 private:
  friend class TpmUtilityTest;

  const TrunksFactory& factory_;
  std::map<uint32_t, TPMS_NV_PUBLIC> nvram_public_area_map_;

  // This method sets a known owner password in the TPM_RH_OWNER hierarchy.
  TPM_RC SetKnownOwnerPassword(const std::string& known_owner_password);

  // Synchronously derives storage root keys for RSA and ECC and persists the
  // keys in the TPM. This operation must be authorized by the |owner_password|
  // and, on success, KRSAStorageRootKey and kECCStorageRootKey can be used
  // with an empty authorization value until the TPM is cleared.
  TPM_RC CreateStorageRootKeys(const std::string& owner_password);

  // This method creates an RSA decryption key to be used for salting sessions.
  // This method also makes the salting key permanent under the storage
  // hierarchy.
  TPM_RC CreateSaltingKey(const std::string& owner_password);

  // This method returns a partially filled TPMT_PUBLIC strucutre,
  // which can then be modified by other methods to create the public
  // template for a key. It takes a valid |key_type| tp construct the
  // parameters.
  TPMT_PUBLIC CreateDefaultPublicArea(TPM_ALG_ID key_alg);

  // Sets TPM |hierarchy| authorization to |password| using |authorization|.
  TPM_RC SetHierarchyAuthorization(TPMI_RH_HIERARCHY_AUTH hierarchy,
                                   const std::string& password,
                                   AuthorizationDelegate* authorization);

  // Disables the TPM platform hierarchy until the next startup. This requires
  // platform |authorization|.
  TPM_RC DisablePlatformHierarchy(AuthorizationDelegate* authorization);

  // Given a public area, this method computes the object name. Following
  // TPM2.0 Specification Part 1 section 16,
  // object_name = HashAlg || Hash(public_area);
  TPM_RC ComputeKeyName(const TPMT_PUBLIC& public_area,
                        std::string* object_name);

  // Given a public area, this method computers the NVSpace's name.
  // It follows TPM2.0 Specification Part 1 section 16,
  // nv_name = HashAlg || Hash(nv_public_area);
  TPM_RC ComputeNVSpaceName(const TPMS_NV_PUBLIC& nv_public_area,
                            std::string* nv_name);

  // This encrypts the |sensitive_data| struct according to the specification
  // defined in TPM2.0 spec Part 1: Figure 19.
  TPM_RC EncryptPrivateData(const TPMT_SENSITIVE& sensitive_area,
                            const TPMT_PUBLIC& public_area,
                            TPM2B_PRIVATE* encrypted_private_data,
                            TPM2B_DATA* encryption_key);

  // Looks for a given persistent |key_handle| and outputs whether or not it
  // |exists|. Returns TPM_RC_SUCCESS on success.
  TPM_RC DoesPersistentKeyExist(TPMI_DH_PERSISTENT key_handle, bool* exists);

  DISALLOW_COPY_AND_ASSIGN(TpmUtilityImpl);
};

}  // namespace trunks

#endif  // TRUNKS_TPM_UTILITY_IMPL_H_
