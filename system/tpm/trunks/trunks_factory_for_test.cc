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

#include "trunks/trunks_factory_for_test.h"

#include <gmock/gmock.h>

#include "trunks/authorization_delegate.h"
#include "trunks/blob_parser.h"
#include "trunks/hmac_session.h"
#include "trunks/mock_blob_parser.h"
#include "trunks/mock_hmac_session.h"
#include "trunks/mock_policy_session.h"
#include "trunks/mock_session_manager.h"
#include "trunks/mock_tpm.h"
#include "trunks/mock_tpm_state.h"
#include "trunks/mock_tpm_utility.h"
#include "trunks/policy_session.h"
#include "trunks/session_manager.h"
#include "trunks/tpm_generated.h"
#include "trunks/tpm_state.h"
#include "trunks/tpm_utility.h"

using testing::NiceMock;

namespace trunks {

// Forwards all calls to a target instance.
class TpmStateForwarder : public TpmState {
 public:
  explicit TpmStateForwarder(TpmState* target) : target_(target) {}
  ~TpmStateForwarder() override = default;

  TPM_RC Initialize() override {
    return target_->Initialize();
  }

  bool IsOwnerPasswordSet() override {
    return target_->IsOwnerPasswordSet();
  }

  bool IsEndorsementPasswordSet() override {
    return target_->IsEndorsementPasswordSet();
  }

  bool IsLockoutPasswordSet() override {
    return target_->IsLockoutPasswordSet();
  }

  bool IsOwned() override {
    return target_->IsOwned();
  }

  bool IsInLockout() override {
    return target_->IsInLockout();
  }

  bool IsPlatformHierarchyEnabled() override {
    return target_->IsPlatformHierarchyEnabled();
  }

  bool IsStorageHierarchyEnabled() override {
    return target_->IsStorageHierarchyEnabled();
  }

  bool IsEndorsementHierarchyEnabled() override {
    return target_->IsEndorsementHierarchyEnabled();
  }

  bool IsEnabled() override {
    return target_->IsEnabled();
  }

  bool WasShutdownOrderly() override {
    return target_->WasShutdownOrderly();
  }

  bool IsRSASupported() override {
    return target_->IsRSASupported();
  }

  bool IsECCSupported() override {
    return target_->IsECCSupported();
  }

  uint32_t GetLockoutCounter() override {
    return target_->GetLockoutCounter();
  }

  uint32_t GetLockoutThreshold() override {
    return target_->GetLockoutThreshold();
  }

  uint32_t GetLockoutInterval() override {
    return target_->GetLockoutInterval();
  }

  uint32_t GetLockoutRecovery() override {
    return target_->GetLockoutRecovery();
  }

 private:
  TpmState* target_;
};

// Forwards all calls to a target instance.
class TpmUtilityForwarder : public TpmUtility {
 public:
  explicit TpmUtilityForwarder(TpmUtility* target) : target_(target) {}
  ~TpmUtilityForwarder() override = default;

  TPM_RC Startup() override {
    return target_->Startup();
  }

  TPM_RC Clear() override {
    return target_->Clear();
  }

  void Shutdown() override {
    return target_->Shutdown();
  }

  TPM_RC InitializeTpm() override {
    return target_->InitializeTpm();
  }

  TPM_RC AllocatePCR(const std::string& platform_password) override {
    return target_->AllocatePCR(platform_password);
  }

  TPM_RC TakeOwnership(const std::string& owner_password,
                       const std::string& endorsement_password,
                       const std::string& lockout_password) override {
    return target_->TakeOwnership(owner_password,
                                  endorsement_password,
                                  lockout_password);
  }

  TPM_RC StirRandom(const std::string& entropy_data,
                    AuthorizationDelegate* delegate) override {
    return target_->StirRandom(entropy_data, delegate);
  }

  TPM_RC GenerateRandom(size_t num_bytes,
                        AuthorizationDelegate* delegate,
                        std::string* random_data) override {
    return target_->GenerateRandom(num_bytes, delegate, random_data);
  }

  TPM_RC ExtendPCR(int pcr_index,
                   const std::string& extend_data,
                   AuthorizationDelegate* delegate) override {
    return target_->ExtendPCR(pcr_index, extend_data, delegate);
  }

  TPM_RC ReadPCR(int pcr_index, std::string* pcr_value) override {
    return target_->ReadPCR(pcr_index, pcr_value);
  }

  TPM_RC AsymmetricEncrypt(TPM_HANDLE key_handle,
                           TPM_ALG_ID scheme,
                           TPM_ALG_ID hash_alg,
                           const std::string& plaintext,
                           AuthorizationDelegate* delegate,
                           std::string* ciphertext) override {
    return target_->AsymmetricEncrypt(key_handle,
                                      scheme,
                                      hash_alg,
                                      plaintext,
                                      delegate,
                                      ciphertext);
  }

  TPM_RC AsymmetricDecrypt(TPM_HANDLE key_handle,
                           TPM_ALG_ID scheme,
                           TPM_ALG_ID hash_alg,
                           const std::string& ciphertext,
                           AuthorizationDelegate* delegate,
                           std::string* plaintext) override {
    return target_->AsymmetricDecrypt(key_handle,
                                      scheme,
                                      hash_alg,
                                      ciphertext,
                                      delegate,
                                      plaintext);
  }

  TPM_RC Sign(TPM_HANDLE key_handle,
              TPM_ALG_ID scheme,
              TPM_ALG_ID hash_alg,
              const std::string& plaintext,
              AuthorizationDelegate* delegate,
              std::string* signature) override {
    return target_->Sign(key_handle,
                         scheme,
                         hash_alg,
                         plaintext,
                         delegate,
                         signature);
  }

  TPM_RC Verify(TPM_HANDLE key_handle,
                TPM_ALG_ID scheme,
                TPM_ALG_ID hash_alg,
                const std::string& plaintext,
                const std::string& signature,
                AuthorizationDelegate* delegate) override {
    return target_->Verify(key_handle, scheme, hash_alg,
                           plaintext, signature, delegate);
  }

  TPM_RC CertifyCreation(TPM_HANDLE key_handle,
                         const std::string& creation_blob) override {
    return target_->CertifyCreation(key_handle, creation_blob);
  }

  TPM_RC ChangeKeyAuthorizationData(TPM_HANDLE key_handle,
                                    const std::string& new_password,
                                    AuthorizationDelegate* delegate,
                                    std::string* key_blob) override {
    return target_->ChangeKeyAuthorizationData(key_handle,
                                               new_password,
                                               delegate,
                                               key_blob);
  }

  TPM_RC ImportRSAKey(AsymmetricKeyUsage key_type,
                      const std::string& modulus,
                      uint32_t public_exponent,
                      const std::string& prime_factor,
                      const std::string& password,
                      AuthorizationDelegate* delegate,
                      std::string* key_blob) override {
    return target_->ImportRSAKey(key_type, modulus, public_exponent,
                                 prime_factor, password, delegate, key_blob);
  }

  TPM_RC CreateRSAKeyPair(AsymmetricKeyUsage key_type,
                          int modulus_bits,
                          uint32_t public_exponent,
                          const std::string& password,
                          const std::string& policy_digest,
                          bool use_only_policy_authorization,
                          int creation_pcr_index,
                          AuthorizationDelegate* delegate,
                          std::string* key_blob,
                          std::string* creation_blob) override {
    return target_->CreateRSAKeyPair(key_type, modulus_bits, public_exponent,
                                     password, policy_digest,
                                     use_only_policy_authorization,
                                     creation_pcr_index,
                                     delegate, key_blob, creation_blob);
  }

  TPM_RC LoadKey(const std::string& key_blob,
                 AuthorizationDelegate* delegate,
                 TPM_HANDLE* key_handle) override {
    return target_->LoadKey(key_blob, delegate, key_handle);
  }

  TPM_RC GetKeyName(TPM_HANDLE handle, std::string* name) override {
    return target_->GetKeyName(handle, name);
  }

  TPM_RC GetKeyPublicArea(TPM_HANDLE handle,
                          TPMT_PUBLIC* public_data) override {
    return target_->GetKeyPublicArea(handle, public_data);
  }

  TPM_RC SealData(const std::string& data_to_seal,
                  const std::string& policy_digest,
                  AuthorizationDelegate* delegate,
                  std::string* sealed_data) override {
    return target_->SealData(data_to_seal, policy_digest,
                             delegate, sealed_data);
  }

  TPM_RC UnsealData(const std::string& sealed_data,
                    AuthorizationDelegate* delegate,
                    std::string* unsealed_data) override {
    return target_->UnsealData(sealed_data, delegate, unsealed_data);
  }

  TPM_RC StartSession(HmacSession* session) override {
    return target_->StartSession(session);
  }

  TPM_RC GetPolicyDigestForPcrValue(int pcr_index,
                                    const std::string& pcr_value,
                                    std::string* policy_digest) override {
    return target_->GetPolicyDigestForPcrValue(pcr_index, pcr_value,
                                               policy_digest);
  }

  TPM_RC DefineNVSpace(uint32_t index,
                       size_t num_bytes,
                       AuthorizationDelegate* delegate) override {
    return target_->DefineNVSpace(index, num_bytes, delegate);
  }

  TPM_RC DestroyNVSpace(uint32_t index,
                        AuthorizationDelegate* delegate) override {
    return target_->DestroyNVSpace(index, delegate);
  }

  TPM_RC LockNVSpace(uint32_t index,
                     AuthorizationDelegate* delegate) override {
    return target_->LockNVSpace(index, delegate);
  }

  TPM_RC WriteNVSpace(uint32_t index,
                      uint32_t offset,
                      const std::string& nvram_data,
                      AuthorizationDelegate* delegate) override {
    return target_->WriteNVSpace(index, offset, nvram_data, delegate);
  }

  TPM_RC ReadNVSpace(uint32_t index,
                     uint32_t offset,
                     size_t num_bytes,
                     std::string* nvram_data,
                     AuthorizationDelegate* delegate) override {
    return target_->ReadNVSpace(index, offset, num_bytes, nvram_data, delegate);
  }

  TPM_RC GetNVSpaceName(uint32_t index, std::string* name) override {
    return target_->GetNVSpaceName(index, name);
  }

  TPM_RC GetNVSpacePublicArea(uint32_t index,
                              TPMS_NV_PUBLIC* public_data) override {
    return target_->GetNVSpacePublicArea(index, public_data);
  }

 private:
  TpmUtility* target_;
};

// Forwards all calls to a target instance.
class AuthorizationDelegateForwarder : public AuthorizationDelegate {
 public:
  explicit AuthorizationDelegateForwarder(AuthorizationDelegate* target)
      : target_(target) {}
  ~AuthorizationDelegateForwarder() override = default;

  bool GetCommandAuthorization(const std::string& command_hash,
                               bool is_command_parameter_encryption_possible,
                               bool is_response_parameter_encryption_possible,
                               std::string* authorization) override {
    return target_->GetCommandAuthorization(
        command_hash,
        is_command_parameter_encryption_possible,
        is_response_parameter_encryption_possible,
        authorization);
  }

  bool CheckResponseAuthorization(const std::string& response_hash,
                                  const std::string& authorization) override {
    return target_->CheckResponseAuthorization(response_hash, authorization);
  }

  bool EncryptCommandParameter(std::string* parameter) override {
    return target_->EncryptCommandParameter(parameter);
  }

  bool DecryptResponseParameter(std::string* parameter) override {
    return target_->DecryptResponseParameter(parameter);
  }

 private:
  AuthorizationDelegate* target_;
};

// Forwards all calls to a target instance.
class SessionManagerForwarder : public SessionManager {
 public:
  explicit SessionManagerForwarder(SessionManager* target) : target_(target) {}
  ~SessionManagerForwarder() override {}

  TPM_HANDLE GetSessionHandle() const override {
    return target_->GetSessionHandle();
  }

  void CloseSession() override {
    return target_->CloseSession();
  }

  TPM_RC StartSession(TPM_SE session_type, TPMI_DH_ENTITY bind_entity,
                      const std::string& bind_authorization_value,
                      bool enable_encryption,
                      HmacAuthorizationDelegate* delegate) override {
    return target_->StartSession(session_type, bind_entity,
                                 bind_authorization_value,
                                 enable_encryption, delegate);
  }

 private:
  SessionManager* target_;
};

// Forwards all calls to a target instance.
class HmacSessionForwarder : public HmacSession {
 public:
  explicit HmacSessionForwarder(HmacSession* target): target_(target) {}
  ~HmacSessionForwarder() override = default;

  AuthorizationDelegate* GetDelegate() override {
    return target_->GetDelegate();
  }

  TPM_RC StartBoundSession(TPMI_DH_ENTITY bind_entity,
                           const std::string& bind_authorization_value,
                           bool enable_encryption) override {
    return target_->StartBoundSession(bind_entity,
                                      bind_authorization_value,
                                      enable_encryption);
  }

  TPM_RC StartUnboundSession(bool enable_encryption) override {
    return target_->StartUnboundSession(enable_encryption);
  }

  void SetEntityAuthorizationValue(const std::string& value) override {
    return target_->SetEntityAuthorizationValue(value);
  }

  void SetFutureAuthorizationValue(const std::string& value) override {
    return target_->SetFutureAuthorizationValue(value);
  }

 private:
  HmacSession* target_;
};


// Forwards all calls to a target instance.
class PolicySessionForwarder : public PolicySession {
 public:
  explicit PolicySessionForwarder(PolicySession* target): target_(target) {}
  ~PolicySessionForwarder() override = default;

  AuthorizationDelegate* GetDelegate() override {
    return target_->GetDelegate();
  }

  TPM_RC StartBoundSession(TPMI_DH_ENTITY bind_entity,
                           const std::string& bind_authorization_value,
                           bool enable_encryption) override {
    return target_->StartBoundSession(bind_entity,
                                      bind_authorization_value,
                                      enable_encryption);
  }

  TPM_RC StartUnboundSession(bool enable_encryption) override {
    return target_->StartUnboundSession(enable_encryption);
  }

  TPM_RC GetDigest(std::string* digest) override {
    return target_->GetDigest(digest);
  }

  TPM_RC PolicyOR(const std::vector<std::string>& digests) override {
    return target_->PolicyOR(digests);
  }

  TPM_RC PolicyPCR(uint32_t pcr_index, const std::string& pcr_value) override {
    return target_->PolicyPCR(pcr_index, pcr_value);
  }

  TPM_RC PolicyCommandCode(TPM_CC command_code) override {
    return target_->PolicyCommandCode(command_code);
  }

  TPM_RC PolicyAuthValue() override {
    return target_->PolicyAuthValue();
  }

  void SetEntityAuthorizationValue(const std::string& value) override {
    return target_->SetEntityAuthorizationValue(value);
  }

 private:
  PolicySession* target_;
};

// Forwards all calls to a target instance.
class BlobParserForwarder : public BlobParser {
 public:
  explicit BlobParserForwarder(BlobParser* target): target_(target) {}
  ~BlobParserForwarder() override = default;

  bool SerializeKeyBlob(const TPM2B_PUBLIC& public_info,
                        const TPM2B_PRIVATE& private_info,
                        std::string* key_blob) override {
    return target_->SerializeKeyBlob(public_info, private_info, key_blob);
  }

  bool ParseKeyBlob(const std::string& key_blob,
                    TPM2B_PUBLIC* public_info,
                    TPM2B_PRIVATE* private_info) override {
    return target_->ParseKeyBlob(key_blob, public_info, private_info);
  }

  bool SerializeCreationBlob(const TPM2B_CREATION_DATA& creation_data,
                             const TPM2B_DIGEST& creation_hash,
                             const TPMT_TK_CREATION& creation_ticket,
                             std::string* creation_blob) override {
    return target_->SerializeCreationBlob(creation_data, creation_hash,
                                          creation_ticket, creation_blob);
  }

  bool ParseCreationBlob(const std::string& creation_blob,
                         TPM2B_CREATION_DATA* creation_data,
                         TPM2B_DIGEST* creation_hash,
                         TPMT_TK_CREATION* creation_ticket) override {
    return target_->ParseCreationBlob(creation_blob, creation_data,
                                      creation_hash, creation_ticket);
  }

 private:
  BlobParser* target_;
};

TrunksFactoryForTest::TrunksFactoryForTest()
    : default_tpm_(new NiceMock<MockTpm>()),
      tpm_(default_tpm_.get()),
      default_tpm_state_(new NiceMock<MockTpmState>()),
      tpm_state_(default_tpm_state_.get()),
      default_tpm_utility_(new NiceMock<MockTpmUtility>()),
      tpm_utility_(default_tpm_utility_.get()),
      default_authorization_delegate_(new PasswordAuthorizationDelegate("")),
      password_authorization_delegate_(default_authorization_delegate_.get()),
      default_session_manager_(new NiceMock<MockSessionManager>()),
      session_manager_(default_session_manager_.get()),
      default_hmac_session_(new NiceMock<MockHmacSession>()),
      hmac_session_(default_hmac_session_.get()),
      default_policy_session_(new NiceMock<MockPolicySession>()),
      policy_session_(default_policy_session_.get()),
      default_blob_parser_(new NiceMock<MockBlobParser>()),
      blob_parser_(default_blob_parser_.get()) {
}

TrunksFactoryForTest::~TrunksFactoryForTest() {}

Tpm* TrunksFactoryForTest::GetTpm() const {
  return tpm_;
}

scoped_ptr<TpmState> TrunksFactoryForTest::GetTpmState() const {
  return scoped_ptr<TpmState>(new TpmStateForwarder(tpm_state_));
}

scoped_ptr<TpmUtility> TrunksFactoryForTest::GetTpmUtility() const {
  return scoped_ptr<TpmUtility>(new TpmUtilityForwarder(tpm_utility_));
}

scoped_ptr<AuthorizationDelegate>
    TrunksFactoryForTest::GetPasswordAuthorization(
        const std::string& password) const {
  return scoped_ptr<AuthorizationDelegate>(
      new AuthorizationDelegateForwarder(password_authorization_delegate_));
}

scoped_ptr<SessionManager> TrunksFactoryForTest::GetSessionManager() const {
  return scoped_ptr<SessionManager>(
      new SessionManagerForwarder(session_manager_));
}

scoped_ptr<HmacSession> TrunksFactoryForTest::GetHmacSession() const {
  return scoped_ptr<HmacSession>(new HmacSessionForwarder(hmac_session_));
}

scoped_ptr<PolicySession> TrunksFactoryForTest::GetPolicySession() const {
  return scoped_ptr<PolicySession>(new PolicySessionForwarder(policy_session_));
}

scoped_ptr<PolicySession> TrunksFactoryForTest::GetTrialSession() const {
  return scoped_ptr<PolicySession>(new PolicySessionForwarder(policy_session_));
}

scoped_ptr<BlobParser> TrunksFactoryForTest::GetBlobParser() const {
  return scoped_ptr<BlobParser>(new BlobParserForwarder(blob_parser_));
}

}  // namespace trunks
