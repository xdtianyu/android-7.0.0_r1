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

#ifndef ATTESTATION_SERVER_ATTESTATION_SERVICE_H_
#define ATTESTATION_SERVER_ATTESTATION_SERVICE_H_

#include "attestation/common/attestation_interface.h"

#include <memory>
#include <string>

#include <base/callback.h>
#include <base/macros.h>
#include <base/memory/weak_ptr.h>
#include <base/threading/thread.h>
#include <brillo/bind_lambda.h>
#include <brillo/http/http_transport.h>

#include "attestation/common/crypto_utility.h"
#include "attestation/common/crypto_utility_impl.h"
#include "attestation/common/tpm_utility.h"
#include "attestation/common/tpm_utility_v1.h"
#include "attestation/server/database.h"
#include "attestation/server/database_impl.h"
#include "attestation/server/key_store.h"
#include "attestation/server/pkcs11_key_store.h"

namespace attestation {

// An implementation of AttestationInterface for the core attestation service.
// Access to TPM, network and local file-system resources occurs asynchronously
// with the exception of Initialize(). All methods must be called on the same
// thread that originally called Initialize().
// Usage:
//   std::unique_ptr<AttestationInterface> attestation =
//       new AttestationService();
//   CHECK(attestation->Initialize());
//   attestation->CreateGoogleAttestedKey(...);
//
// THREADING NOTES:
// This class runs a worker thread and delegates all calls to it. This keeps the
// public methods non-blocking while allowing complex implementation details
// with dependencies on the TPM, network, and filesystem to be coded in a more
// readable way. It also serves to serialize method execution which reduces
// complexity with TPM state.
//
// Tasks that run on the worker thread are bound with base::Unretained which is
// safe because the thread is owned by this class (so it is guaranteed not to
// process a task after destruction). Weak pointers are used to post replies
// back to the main thread.
class AttestationService : public AttestationInterface {
 public:
  AttestationService();
  ~AttestationService() override = default;

  // AttestationInterface methods.
  bool Initialize() override;
  void CreateGoogleAttestedKey(
      const CreateGoogleAttestedKeyRequest& request,
      const CreateGoogleAttestedKeyCallback& callback) override;
  void GetKeyInfo(const GetKeyInfoRequest& request,
                  const GetKeyInfoCallback& callback) override;
  void GetEndorsementInfo(const GetEndorsementInfoRequest& request,
                          const GetEndorsementInfoCallback& callback) override;
  void GetAttestationKeyInfo(
      const GetAttestationKeyInfoRequest& request,
      const GetAttestationKeyInfoCallback& callback) override;
  void ActivateAttestationKey(
      const ActivateAttestationKeyRequest& request,
      const ActivateAttestationKeyCallback& callback) override;
  void CreateCertifiableKey(
      const CreateCertifiableKeyRequest& request,
      const CreateCertifiableKeyCallback& callback) override;
  void Decrypt(const DecryptRequest& request,
               const DecryptCallback& callback) override;
  void Sign(const SignRequest& request, const SignCallback& callback) override;
  void RegisterKeyWithChapsToken(
      const RegisterKeyWithChapsTokenRequest& request,
      const RegisterKeyWithChapsTokenCallback& callback) override;

  // Mutators useful for testing.
  void set_crypto_utility(CryptoUtility* crypto_utility) {
    crypto_utility_ = crypto_utility;
  }

  void set_database(Database* database) {
    database_ = database;
  }

  void set_http_transport(
      const std::shared_ptr<brillo::http::Transport>& transport) {
    http_transport_ = transport;
  }

  void set_key_store(KeyStore* key_store) {
    key_store_ = key_store;
  }

  void set_tpm_utility(TpmUtility* tpm_utility) {
    tpm_utility_ = tpm_utility;
  }

  // So tests don't need to duplicate URL decisions.
  const std::string& attestation_ca_origin() {
    return attestation_ca_origin_;
  }

 private:
  enum ACARequestType {
    kEnroll,          // Enrolls a device, certifying an identity key.
    kGetCertificate,  // Issues a certificate for a TPM-backed key.
  };

  // A relay callback which allows the use of weak pointer semantics for a reply
  // to TaskRunner::PostTaskAndReply.
  template<typename ReplyProtobufType>
  void TaskRelayCallback(
      const base::Callback<void(const ReplyProtobufType&)> callback,
      const std::shared_ptr<ReplyProtobufType>& reply) {
    callback.Run(*reply);
  }

  // A blocking implementation of CreateGoogleAttestedKey appropriate to run on
  // the worker thread.
  void CreateGoogleAttestedKeyTask(
      const CreateGoogleAttestedKeyRequest& request,
      const std::shared_ptr<CreateGoogleAttestedKeyReply>& result);

  // A blocking implementation of GetKeyInfo.
  void GetKeyInfoTask(
      const GetKeyInfoRequest& request,
      const std::shared_ptr<GetKeyInfoReply>& result);

  // A blocking implementation of GetEndorsementInfo.
  void GetEndorsementInfoTask(
      const GetEndorsementInfoRequest& request,
      const std::shared_ptr<GetEndorsementInfoReply>& result);

  // A blocking implementation of GetAttestationKeyInfo.
  void GetAttestationKeyInfoTask(
      const GetAttestationKeyInfoRequest& request,
      const std::shared_ptr<GetAttestationKeyInfoReply>& result);

  // A blocking implementation of ActivateAttestationKey.
  void ActivateAttestationKeyTask(
      const ActivateAttestationKeyRequest& request,
      const std::shared_ptr<ActivateAttestationKeyReply>& result);

  // A blocking implementation of CreateCertifiableKey.
  void CreateCertifiableKeyTask(
      const CreateCertifiableKeyRequest& request,
      const std::shared_ptr<CreateCertifiableKeyReply>& result);

  // A blocking implementation of Decrypt.
  void DecryptTask(const DecryptRequest& request,
                   const std::shared_ptr<DecryptReply>& result);

  // A blocking implementation of Sign.
  void SignTask(const SignRequest& request,
                const std::shared_ptr<SignReply>& result);

  // A synchronous implementation of RegisterKeyWithChapsToken.
  void RegisterKeyWithChapsTokenTask(
      const RegisterKeyWithChapsTokenRequest& request,
      const std::shared_ptr<RegisterKeyWithChapsTokenReply>& result);

  // Returns true iff all information required for enrollment with the Google
  // Attestation CA is available.
  bool IsPreparedForEnrollment();

  // Returns true iff enrollment with the Google Attestation CA has been
  // completed.
  bool IsEnrolled();

  // Creates an enrollment request compatible with the Google Attestation CA.
  // Returns true on success.
  bool CreateEnrollRequest(std::string* enroll_request);

  // Finishes enrollment given an |enroll_response| from the Google Attestation
  // CA. Returns true on success. On failure, returns false and sets
  // |server_error| to the error string from the CA.
  bool FinishEnroll(const std::string& enroll_response,
                    std::string* server_error);

  // Creates a |certificate_request| compatible with the Google Attestation CA
  // for the given |key|, according to the given |profile|, |username| and
  // |origin|.
  bool CreateCertificateRequest(const std::string& username,
                                const CertifiedKey& key,
                                CertificateProfile profile,
                                const std::string& origin,
                                std::string* certificate_request,
                                std::string* message_id);

  // Finishes a certificate request by decoding the |certificate_response| to
  // recover the |certificate_chain| and storing it in association with the
  // |key| identified by |username| and |key_label|. Returns true on success. On
  // failure, returns false and sets |server_error| to the error string from the
  // CA.
  bool FinishCertificateRequest(const std::string& certificate_response,
                                const std::string& username,
                                const std::string& key_label,
                                const std::string& message_id,
                                CertifiedKey* key,
                                std::string* certificate_chain,
                                std::string* server_error);

  // Sends a |request_type| |request| to the Google Attestation CA and waits for
  // the |reply|. Returns true on success.
  bool SendACARequestAndBlock(ACARequestType request_type,
                              const std::string& request,
                              std::string* reply);

  // Creates, certifies, and saves a new |key| for |username| with the given
  // |key_label|, |key_type|, and |key_usage|. Returns true on success.
  bool CreateKey(const std::string& username,
                 const std::string& key_label,
                 KeyType key_type,
                 KeyUsage key_usage,
                 CertifiedKey* key);

  // Finds the |key| associated with |username| and |key_label|. Returns false
  // if such a key does not exist.
  bool FindKeyByLabel(const std::string& username,
                      const std::string& key_label,
                      CertifiedKey* key);

  // Saves the |key| associated with |username| and |key_label|. Returns true on
  // success.
  bool SaveKey(const std::string& username,
               const std::string& key_label,
               const CertifiedKey& key);

  // Deletes the key associated with |username| and |key_label|.
  void DeleteKey(const std::string& username,
                 const std::string& key_label);

  // Adds named device-wide key to the attestation database.
  bool AddDeviceKey(const std::string& key_label, const CertifiedKey& key);

  // Removes a device-wide key from the attestation database.
  void RemoveDeviceKey(const std::string& key_label);

  // Creates a PEM certificate chain from the credential fields of a |key|.
  std::string CreatePEMCertificateChain(const CertifiedKey& key);

  // Creates a certificate in PEM format from a DER encoded X.509 certificate.
  std::string CreatePEMCertificate(const std::string& certificate);

  // Chooses a temporal index which will be used by the ACA to create a
  // certificate.  This decision factors in the currently signed-in |user| and
  // the |origin| of the certificate request.  The strategy is to find an index
  // which has not already been used by another user for the same origin.
  int ChooseTemporalIndex(const std::string& user, const std::string& origin);

  // Creates a Google Attestation CA URL for the given |request_type|.
  std::string GetACAURL(ACARequestType request_type) const;

  // Creates a X.509/DER SubjectPublicKeyInfo for the given |key_type| and
  // |public_key|. On success returns true and provides |public_key_info|.
  bool GetSubjectPublicKeyInfo(KeyType key_type,
                               const std::string& public_key,
                               std::string* public_key_info) const;

  base::WeakPtr<AttestationService> GetWeakPtr();


  const std::string attestation_ca_origin_;

  // Other than initialization and destruction, these are used only by the
  // worker thread.
  CryptoUtility* crypto_utility_{nullptr};
  Database* database_{nullptr};
  std::shared_ptr<brillo::http::Transport> http_transport_;
  KeyStore* key_store_{nullptr};
  TpmUtility* tpm_utility_{nullptr};

  // Default implementations for the above interfaces. These will be setup
  // during Initialize() if the corresponding interface has not been set with a
  // mutator.
  std::unique_ptr<CryptoUtilityImpl> default_crypto_utility_;
  std::unique_ptr<DatabaseImpl> default_database_;
  std::unique_ptr<Pkcs11KeyStore> default_key_store_;
  std::unique_ptr<chaps::TokenManagerClient> pkcs11_token_manager_;
  std::unique_ptr<TpmUtilityV1> default_tpm_utility_;

  // All work is done in the background. This serves to serialize requests and
  // allow synchronous implementation of complex methods. This is intentionally
  // declared after the thread-owned members.
  std::unique_ptr<base::Thread> worker_thread_;

  // Declared last so any weak pointers are destroyed first.
  base::WeakPtrFactory<AttestationService> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AttestationService);
};

}  // namespace attestation

#endif  // ATTESTATION_SERVER_ATTESTATION_SERVICE_H_
