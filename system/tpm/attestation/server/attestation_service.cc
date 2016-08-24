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

#include "attestation/server/attestation_service.h"

#include <string>

#include <base/callback.h>
#include <brillo/bind_lambda.h>
#include <brillo/data_encoding.h>
#include <brillo/http/http_utils.h>
#include <brillo/mime_utils.h>
#include <crypto/sha2.h>

#include "attestation/common/attestation_ca.pb.h"
#include "attestation/common/database.pb.h"
#include "attestation/server/database_impl.h"

namespace {

#ifndef USE_TEST_ACA
const char kACAWebOrigin[] = "https://chromeos-ca.gstatic.com";
#else
const char kACAWebOrigin[] = "https://asbestos-qa.corp.google.com";
#endif
const size_t kNonceSize = 20;  // As per TPM_NONCE definition.
const int kNumTemporalValues = 5;

}  // namespace

namespace attestation {

AttestationService::AttestationService()
    : attestation_ca_origin_(kACAWebOrigin),
      weak_factory_(this) {}

bool AttestationService::Initialize() {
  LOG(INFO) << "Attestation service started.";
  worker_thread_.reset(new base::Thread("Attestation Service Worker"));
  worker_thread_->StartWithOptions(
      base::Thread::Options(base::MessageLoop::TYPE_IO, 0));
  if (!tpm_utility_) {
    default_tpm_utility_.reset(new TpmUtilityV1());
    if (!default_tpm_utility_->Initialize()) {
      return false;
    }
    tpm_utility_ = default_tpm_utility_.get();
  }
  if (!crypto_utility_) {
    default_crypto_utility_.reset(new CryptoUtilityImpl(tpm_utility_));
    crypto_utility_ = default_crypto_utility_.get();
  }
  if (!database_) {
    default_database_.reset(new DatabaseImpl(crypto_utility_));
    worker_thread_->task_runner()->PostTask(FROM_HERE, base::Bind(
        &DatabaseImpl::Initialize,
        base::Unretained(default_database_.get())));
    database_ = default_database_.get();
  }
  if (!key_store_) {
    pkcs11_token_manager_.reset(new chaps::TokenManagerClient());
    default_key_store_.reset(new Pkcs11KeyStore(pkcs11_token_manager_.get()));
    key_store_ = default_key_store_.get();
  }
  return true;
}

void AttestationService::CreateGoogleAttestedKey(
    const CreateGoogleAttestedKeyRequest& request,
    const CreateGoogleAttestedKeyCallback& callback) {
  auto result = std::make_shared<CreateGoogleAttestedKeyReply>();
  base::Closure task = base::Bind(
      &AttestationService::CreateGoogleAttestedKeyTask,
      base::Unretained(this),
      request,
      result);
  base::Closure reply = base::Bind(
      &AttestationService::TaskRelayCallback<CreateGoogleAttestedKeyReply>,
      GetWeakPtr(),
      callback,
      result);
  worker_thread_->task_runner()->PostTaskAndReply(FROM_HERE, task, reply);
}

void AttestationService::CreateGoogleAttestedKeyTask(
    const CreateGoogleAttestedKeyRequest& request,
    const std::shared_ptr<CreateGoogleAttestedKeyReply>& result) {
  LOG(INFO) << "Creating attested key: " << request.key_label();
  if (!IsPreparedForEnrollment()) {
    LOG(ERROR) << "Attestation: TPM is not ready.";
    result->set_status(STATUS_NOT_READY);
    return;
  }
  if (!IsEnrolled()) {
    std::string enroll_request;
    if (!CreateEnrollRequest(&enroll_request)) {
      result->set_status(STATUS_UNEXPECTED_DEVICE_ERROR);
      return;
    }
    std::string enroll_reply;
    if (!SendACARequestAndBlock(kEnroll,
                                enroll_request,
                                &enroll_reply)) {
      result->set_status(STATUS_CA_NOT_AVAILABLE);
      return;
    }
    std::string server_error;
    if (!FinishEnroll(enroll_reply, &server_error)) {
      if (server_error.empty()) {
        result->set_status(STATUS_UNEXPECTED_DEVICE_ERROR);
        return;
      }
      result->set_status(STATUS_REQUEST_DENIED_BY_CA);
      result->set_server_error(server_error);
      return;
    }
  }
  CertifiedKey key;
  if (!CreateKey(request.username(), request.key_label(), request.key_type(),
                 request.key_usage(), &key)) {
    result->set_status(STATUS_UNEXPECTED_DEVICE_ERROR);
    return;
  }
  std::string certificate_request;
  std::string message_id;
  if (!CreateCertificateRequest(request.username(),
                                key,
                                request.certificate_profile(),
                                request.origin(),
                                &certificate_request,
                                &message_id)) {
    result->set_status(STATUS_UNEXPECTED_DEVICE_ERROR);
    return;
  }
  std::string certificate_reply;
  if (!SendACARequestAndBlock(kGetCertificate,
                              certificate_request,
                              &certificate_reply)) {
    result->set_status(STATUS_CA_NOT_AVAILABLE);
    return;
  }
  std::string certificate_chain;
  std::string server_error;
  if (!FinishCertificateRequest(certificate_reply,
                                request.username(),
                                request.key_label(),
                                message_id,
                                &key,
                                &certificate_chain,
                                &server_error)) {
    if (server_error.empty()) {
      result->set_status(STATUS_UNEXPECTED_DEVICE_ERROR);
      return;
    }
    result->set_status(STATUS_REQUEST_DENIED_BY_CA);
    result->set_server_error(server_error);
    return;
  }
  result->set_certificate_chain(certificate_chain);
}

void AttestationService::GetKeyInfo(const GetKeyInfoRequest& request,
                                    const GetKeyInfoCallback& callback) {
  auto result = std::make_shared<GetKeyInfoReply>();
  base::Closure task = base::Bind(
      &AttestationService::GetKeyInfoTask,
      base::Unretained(this),
      request,
      result);
  base::Closure reply = base::Bind(
      &AttestationService::TaskRelayCallback<GetKeyInfoReply>,
      GetWeakPtr(),
      callback,
      result);
  worker_thread_->task_runner()->PostTaskAndReply(FROM_HERE, task, reply);
}

void AttestationService::GetKeyInfoTask(
    const GetKeyInfoRequest& request,
    const std::shared_ptr<GetKeyInfoReply>& result) {
  CertifiedKey key;
  if (!FindKeyByLabel(request.username(), request.key_label(), &key)) {
    result->set_status(STATUS_INVALID_PARAMETER);
    return;
  }
  std::string public_key_info;
  if (!GetSubjectPublicKeyInfo(key.key_type(), key.public_key(),
                               &public_key_info)) {
    LOG(ERROR) << __func__ << ": Bad public key.";
    result->set_status(STATUS_UNEXPECTED_DEVICE_ERROR);
    return;
  }
  result->set_key_type(key.key_type());
  result->set_key_usage(key.key_usage());
  result->set_public_key(public_key_info);
  result->set_certify_info(key.certified_key_info());
  result->set_certify_info_signature(key.certified_key_proof());
  if (key.has_intermediate_ca_cert()) {
    result->set_certificate(CreatePEMCertificateChain(key));
  } else {
    result->set_certificate(key.certified_key_credential());
  }
}

void AttestationService::GetEndorsementInfo(
    const GetEndorsementInfoRequest& request,
    const GetEndorsementInfoCallback& callback) {
  auto result = std::make_shared<GetEndorsementInfoReply>();
  base::Closure task = base::Bind(
      &AttestationService::GetEndorsementInfoTask,
      base::Unretained(this),
      request,
      result);
  base::Closure reply = base::Bind(
      &AttestationService::TaskRelayCallback<GetEndorsementInfoReply>,
      GetWeakPtr(),
      callback,
      result);
  worker_thread_->task_runner()->PostTaskAndReply(FROM_HERE, task, reply);
}

void AttestationService::GetEndorsementInfoTask(
    const GetEndorsementInfoRequest& request,
    const std::shared_ptr<GetEndorsementInfoReply>& result) {
  if (request.key_type() != KEY_TYPE_RSA) {
    result->set_status(STATUS_INVALID_PARAMETER);
    return;
  }
  auto database_pb = database_->GetProtobuf();
  if (!database_pb.has_credentials() ||
      !database_pb.credentials().has_endorsement_public_key()) {
    // Try to read the public key directly.
    std::string public_key;
    if (!tpm_utility_->GetEndorsementPublicKey(&public_key)) {
      result->set_status(STATUS_NOT_AVAILABLE);
      return;
    }
    database_pb.mutable_credentials()->set_endorsement_public_key(public_key);
  }
  std::string public_key_info;
  if (!GetSubjectPublicKeyInfo(
      request.key_type(),
      database_pb.credentials().endorsement_public_key(),
      &public_key_info)) {
    LOG(ERROR) << __func__ << ": Bad public key.";
    result->set_status(STATUS_UNEXPECTED_DEVICE_ERROR);
    return;
  }
  result->set_ek_public_key(public_key_info);
  if (database_pb.credentials().has_endorsement_credential()) {
    result->set_ek_certificate(
        database_pb.credentials().endorsement_credential());
  }
}

void AttestationService::GetAttestationKeyInfo(
    const GetAttestationKeyInfoRequest& request,
    const GetAttestationKeyInfoCallback& callback) {
  auto result = std::make_shared<GetAttestationKeyInfoReply>();
  base::Closure task = base::Bind(
      &AttestationService::GetAttestationKeyInfoTask,
      base::Unretained(this),
      request,
      result);
  base::Closure reply = base::Bind(
      &AttestationService::TaskRelayCallback<GetAttestationKeyInfoReply>,
      GetWeakPtr(),
      callback,
      result);
  worker_thread_->task_runner()->PostTaskAndReply(FROM_HERE, task, reply);
}

void AttestationService::GetAttestationKeyInfoTask(
    const GetAttestationKeyInfoRequest& request,
    const std::shared_ptr<GetAttestationKeyInfoReply>& result) {
  if (request.key_type() != KEY_TYPE_RSA) {
    result->set_status(STATUS_INVALID_PARAMETER);
    return;
  }
  auto database_pb = database_->GetProtobuf();
  if (!IsPreparedForEnrollment() || !database_pb.has_identity_key()) {
    result->set_status(STATUS_NOT_AVAILABLE);
    return;
  }
  if (database_pb.identity_key().has_identity_public_key()) {
    std::string public_key_info;
    if (!GetSubjectPublicKeyInfo(
        request.key_type(),
        database_pb.identity_key().identity_public_key(),
        &public_key_info)) {
      LOG(ERROR) << __func__ << ": Bad public key.";
      result->set_status(STATUS_UNEXPECTED_DEVICE_ERROR);
      return;
    }
    result->set_public_key(public_key_info);
  }
  if (database_pb.has_identity_binding() &&
      database_pb.identity_binding().has_identity_public_key()) {
    result->set_public_key_tpm_format(
        database_pb.identity_binding().identity_public_key());
  }
  if (database_pb.identity_key().has_identity_credential()) {
    result->set_certificate(database_pb.identity_key().identity_credential());
  }
  if (database_pb.has_pcr0_quote()) {
    *result->mutable_pcr0_quote() = database_pb.pcr0_quote();
  }
  if (database_pb.has_pcr1_quote()) {
    *result->mutable_pcr1_quote() = database_pb.pcr1_quote();
  }
}

void AttestationService::ActivateAttestationKey(
    const ActivateAttestationKeyRequest& request,
    const ActivateAttestationKeyCallback& callback) {
  auto result = std::make_shared<ActivateAttestationKeyReply>();
  base::Closure task = base::Bind(
      &AttestationService::ActivateAttestationKeyTask,
      base::Unretained(this),
      request,
      result);
  base::Closure reply = base::Bind(
      &AttestationService::TaskRelayCallback<ActivateAttestationKeyReply>,
      GetWeakPtr(),
      callback,
      result);
  worker_thread_->task_runner()->PostTaskAndReply(FROM_HERE, task, reply);
}

void AttestationService::ActivateAttestationKeyTask(
    const ActivateAttestationKeyRequest& request,
    const std::shared_ptr<ActivateAttestationKeyReply>& result) {
  if (request.key_type() != KEY_TYPE_RSA) {
    result->set_status(STATUS_INVALID_PARAMETER);
    return;
  }
  std::string certificate;
  auto database_pb = database_->GetProtobuf();
  if (!tpm_utility_->ActivateIdentity(
      database_pb.delegate().blob(),
      database_pb.delegate().secret(),
      database_pb.identity_key().identity_key_blob(),
      request.encrypted_certificate().asym_ca_contents(),
      request.encrypted_certificate().sym_ca_attestation(),
      &certificate)) {
    LOG(ERROR) << __func__ << ": Failed to activate identity.";
    result->set_status(STATUS_UNEXPECTED_DEVICE_ERROR);
    return;
  }
  if (request.save_certificate()) {
    database_->GetMutableProtobuf()->mutable_identity_key()->
        set_identity_credential(certificate);
    if (!database_->SaveChanges()) {
      LOG(ERROR) << __func__ << ": Failed to persist database changes.";
      result->set_status(STATUS_UNEXPECTED_DEVICE_ERROR);
    }
  }
  result->set_certificate(certificate);
}

void AttestationService::CreateCertifiableKey(
    const CreateCertifiableKeyRequest& request,
    const CreateCertifiableKeyCallback& callback) {
  auto result = std::make_shared<CreateCertifiableKeyReply>();
  base::Closure task = base::Bind(
      &AttestationService::CreateCertifiableKeyTask,
      base::Unretained(this),
      request,
      result);
  base::Closure reply = base::Bind(
      &AttestationService::TaskRelayCallback<CreateCertifiableKeyReply>,
      GetWeakPtr(),
      callback,
      result);
  worker_thread_->task_runner()->PostTaskAndReply(FROM_HERE, task, reply);
}

void AttestationService::CreateCertifiableKeyTask(
    const CreateCertifiableKeyRequest& request,
    const std::shared_ptr<CreateCertifiableKeyReply>& result) {
  CertifiedKey key;
  if (!CreateKey(request.username(), request.key_label(), request.key_type(),
                 request.key_usage(), &key)) {
    result->set_status(STATUS_UNEXPECTED_DEVICE_ERROR);
    return;
  }
  std::string public_key_info;
  if (!GetSubjectPublicKeyInfo(key.key_type(), key.public_key(),
                               &public_key_info)) {
    LOG(ERROR) << __func__ << ": Bad public key.";
    result->set_status(STATUS_UNEXPECTED_DEVICE_ERROR);
    return;
  }
  result->set_public_key(public_key_info);
  result->set_certify_info(key.certified_key_info());
  result->set_certify_info_signature(key.certified_key_proof());
}

void AttestationService::Decrypt(const DecryptRequest& request,
                                 const DecryptCallback& callback) {
  auto result = std::make_shared<DecryptReply>();
  base::Closure task = base::Bind(
      &AttestationService::DecryptTask,
      base::Unretained(this),
      request,
      result);
  base::Closure reply = base::Bind(
      &AttestationService::TaskRelayCallback<DecryptReply>,
      GetWeakPtr(),
      callback,
      result);
  worker_thread_->task_runner()->PostTaskAndReply(FROM_HERE, task, reply);
}

void AttestationService::DecryptTask(
    const DecryptRequest& request,
    const std::shared_ptr<DecryptReply>& result) {
  CertifiedKey key;
  if (!FindKeyByLabel(request.username(), request.key_label(), &key)) {
    result->set_status(STATUS_INVALID_PARAMETER);
    return;
  }
  std::string data;
  if (!tpm_utility_->Unbind(key.key_blob(), request.encrypted_data(), &data)) {
    result->set_status(STATUS_UNEXPECTED_DEVICE_ERROR);
    return;
  }
  result->set_decrypted_data(data);
}

void AttestationService::Sign(const SignRequest& request,
                              const SignCallback& callback) {
  auto result = std::make_shared<SignReply>();
  base::Closure task = base::Bind(
      &AttestationService::SignTask,
      base::Unretained(this),
      request,
      result);
  base::Closure reply = base::Bind(
      &AttestationService::TaskRelayCallback<SignReply>,
      GetWeakPtr(),
      callback,
      result);
  worker_thread_->task_runner()->PostTaskAndReply(FROM_HERE, task, reply);
}

void AttestationService::SignTask(const SignRequest& request,
                                  const std::shared_ptr<SignReply>& result) {
  CertifiedKey key;
  if (!FindKeyByLabel(request.username(), request.key_label(), &key)) {
    result->set_status(STATUS_INVALID_PARAMETER);
    return;
  }
  std::string signature;
  if (!tpm_utility_->Sign(key.key_blob(), request.data_to_sign(), &signature)) {
    result->set_status(STATUS_UNEXPECTED_DEVICE_ERROR);
    return;
  }
  result->set_signature(signature);
}

void AttestationService::RegisterKeyWithChapsToken(
    const RegisterKeyWithChapsTokenRequest& request,
    const RegisterKeyWithChapsTokenCallback& callback) {
  auto result = std::make_shared<RegisterKeyWithChapsTokenReply>();
  base::Closure task = base::Bind(
      &AttestationService::RegisterKeyWithChapsTokenTask,
      base::Unretained(this),
      request,
      result);
  base::Closure reply = base::Bind(
      &AttestationService::TaskRelayCallback<RegisterKeyWithChapsTokenReply>,
      GetWeakPtr(),
      callback,
      result);
  worker_thread_->task_runner()->PostTaskAndReply(FROM_HERE, task, reply);
}

void AttestationService::RegisterKeyWithChapsTokenTask(
    const RegisterKeyWithChapsTokenRequest& request,
    const std::shared_ptr<RegisterKeyWithChapsTokenReply>& result) {
  CertifiedKey key;
  if (!FindKeyByLabel(request.username(), request.key_label(), &key)) {
    result->set_status(STATUS_INVALID_PARAMETER);
    return;
  }
  if (!key_store_->Register(request.username(), request.key_label(),
                            key.key_type(), key.key_usage(), key.key_blob(),
                            key.public_key(), key.certified_key_credential())) {
    result->set_status(STATUS_UNEXPECTED_DEVICE_ERROR);
    return;
  }
  if (key.has_intermediate_ca_cert() &&
      !key_store_->RegisterCertificate(request.username(),
                                       key.intermediate_ca_cert())) {
    result->set_status(STATUS_UNEXPECTED_DEVICE_ERROR);
    return;
  }
  for (int i = 0; i < key.additional_intermediate_ca_cert_size(); ++i) {
    if (!key_store_->RegisterCertificate(
            request.username(),
            key.additional_intermediate_ca_cert(i))) {
      result->set_status(STATUS_UNEXPECTED_DEVICE_ERROR);
      return;
    }
  }
  DeleteKey(request.username(), request.key_label());
}

bool AttestationService::IsPreparedForEnrollment() {
  if (!tpm_utility_->IsTpmReady()) {
    return false;
  }
  auto database_pb = database_->GetProtobuf();
  if (!database_pb.has_credentials()) {
    return false;
  }
  return (database_pb.credentials().has_endorsement_credential() ||
          database_pb.credentials()
              .has_default_encrypted_endorsement_credential());
}

bool AttestationService::IsEnrolled() {
  auto database_pb = database_->GetProtobuf();
  return database_pb.has_identity_key() &&
         database_pb.identity_key().has_identity_credential();
}

bool AttestationService::CreateEnrollRequest(std::string* enroll_request) {
  if (!IsPreparedForEnrollment()) {
    LOG(ERROR) << __func__ << ": Enrollment is not possible, attestation data "
               << "does not exist.";
    return false;
  }
  auto database_pb = database_->GetProtobuf();
  AttestationEnrollmentRequest request_pb;
  *request_pb.mutable_encrypted_endorsement_credential() =
      database_pb.credentials().default_encrypted_endorsement_credential();
  request_pb.set_identity_public_key(
      database_pb.identity_binding().identity_public_key());
  *request_pb.mutable_pcr0_quote() = database_pb.pcr0_quote();
  *request_pb.mutable_pcr1_quote() = database_pb.pcr1_quote();
  if (!request_pb.SerializeToString(enroll_request)) {
    LOG(ERROR) << __func__ << ": Failed to serialize protobuf.";
    return false;
  }
  return true;
}

bool AttestationService::FinishEnroll(const std::string& enroll_response,
                                      std::string* server_error) {
  if (!tpm_utility_->IsTpmReady()) {
    return false;
  }
  AttestationEnrollmentResponse response_pb;
  if (!response_pb.ParseFromString(enroll_response)) {
    LOG(ERROR) << __func__ << ": Failed to parse response from CA.";
    return false;
  }
  if (response_pb.status() != OK) {
    *server_error = response_pb.detail();
    LOG(ERROR) << __func__ << ": Error received from CA: "
               << response_pb.detail();
    return false;
  }
  std::string credential;
  auto database_pb = database_->GetProtobuf();
  if (!tpm_utility_->ActivateIdentity(
      database_pb.delegate().blob(),
      database_pb.delegate().secret(),
      database_pb.identity_key().identity_key_blob(),
      response_pb.encrypted_identity_credential().asym_ca_contents(),
      response_pb.encrypted_identity_credential().sym_ca_attestation(),
      &credential)) {
    LOG(ERROR) << __func__ << ": Failed to activate identity.";
    return false;
  }
  database_->GetMutableProtobuf()->mutable_identity_key()->
      set_identity_credential(credential);
  if (!database_->SaveChanges()) {
    LOG(ERROR) << __func__ << ": Failed to persist database changes.";
    return false;
  }
  LOG(INFO) << "Attestation: Enrollment complete.";
  return true;
}

bool AttestationService::CreateCertificateRequest(
    const std::string& username,
    const CertifiedKey& key,
    CertificateProfile profile,
    const std::string& origin,
    std::string* certificate_request,
    std::string* message_id) {
  if (!tpm_utility_->IsTpmReady()) {
    return false;
  }
  if (!IsEnrolled()) {
    LOG(ERROR) << __func__ << ": Device is not enrolled for attestation.";
    return false;
  }
  AttestationCertificateRequest request_pb;
  if (!crypto_utility_->GetRandom(kNonceSize, message_id)) {
    LOG(ERROR) << __func__ << ": GetRandom(message_id) failed.";
    return false;
  }
  request_pb.set_message_id(*message_id);
  auto database_pb = database_->GetProtobuf();
  request_pb.set_identity_credential(
      database_pb.identity_key().identity_credential());
  request_pb.set_profile(profile);
  if (!origin.empty() &&
      (profile == CONTENT_PROTECTION_CERTIFICATE_WITH_STABLE_ID)) {
    request_pb.set_origin(origin);
    request_pb.set_temporal_index(ChooseTemporalIndex(username, origin));
  }
  request_pb.set_certified_public_key(key.public_key_tpm_format());
  request_pb.set_certified_key_info(key.certified_key_info());
  request_pb.set_certified_key_proof(key.certified_key_proof());
  if (!request_pb.SerializeToString(certificate_request)) {
    LOG(ERROR) << __func__ << ": Failed to serialize protobuf.";
    return false;
  }
  return true;
}

bool AttestationService::FinishCertificateRequest(
    const std::string& certificate_response,
    const std::string& username,
    const std::string& key_label,
    const std::string& message_id,
    CertifiedKey* key,
    std::string* certificate_chain,
    std::string* server_error) {
  if (!tpm_utility_->IsTpmReady()) {
    return false;
  }
  AttestationCertificateResponse response_pb;
  if (!response_pb.ParseFromString(certificate_response)) {
    LOG(ERROR) << __func__ << ": Failed to parse response from Privacy CA.";
    return false;
  }
  if (response_pb.status() != OK) {
    *server_error = response_pb.detail();
    LOG(ERROR) << __func__ << ": Error received from Privacy CA: "
               << response_pb.detail();
    return false;
  }
  if (message_id != response_pb.message_id()) {
    LOG(ERROR) << __func__ << ": Message ID mismatch.";
    return false;
  }

  // Finish populating the CertifiedKey protobuf and store it.
  key->set_certified_key_credential(response_pb.certified_key_credential());
  key->set_intermediate_ca_cert(response_pb.intermediate_ca_cert());
  key->mutable_additional_intermediate_ca_cert()->MergeFrom(
      response_pb.additional_intermediate_ca_cert());
  if (!SaveKey(username, key_label, *key)) {
    return false;
  }
  LOG(INFO) << "Attestation: Certified key credential received and stored.";
  *certificate_chain = CreatePEMCertificateChain(*key);
  return true;
}

bool AttestationService::SendACARequestAndBlock(ACARequestType request_type,
                                                const std::string& request,
                                                std::string* reply) {
  std::shared_ptr<brillo::http::Transport> transport = http_transport_;
  if (!transport) {
    transport = brillo::http::Transport::CreateDefault();
  }
  std::unique_ptr<brillo::http::Response> response = PostBinaryAndBlock(
      GetACAURL(request_type),
      request.data(),
      request.size(),
      brillo::mime::application::kOctet_stream,
      {},  // headers
      transport,
      nullptr);  // error
  if (!response || !response->IsSuccessful()) {
    LOG(ERROR) << "HTTP request to Attestation CA failed.";
    return false;
  }
  *reply = response->ExtractDataAsString();
  return true;
}

bool AttestationService::FindKeyByLabel(const std::string& username,
                                        const std::string& key_label,
                                        CertifiedKey* key) {
  if (!username.empty()) {
    std::string key_data;
    if (!key_store_->Read(username, key_label, &key_data)) {
      LOG(INFO) << "Key not found: " << key_label;
      return false;
    }
    if (key && !key->ParseFromString(key_data)) {
      LOG(ERROR) << "Failed to parse key: " << key_label;
      return false;
    }
    return true;
  }
  auto database_pb = database_->GetProtobuf();
  for (int i = 0; i < database_pb.device_keys_size(); ++i) {
    if (database_pb.device_keys(i).key_name() == key_label) {
      *key = database_pb.device_keys(i);
      return true;
    }
  }
  LOG(INFO) << "Key not found: " << key_label;
  return false;
}

bool AttestationService::CreateKey(const std::string& username,
                                   const std::string& key_label,
                                   KeyType key_type,
                                   KeyUsage key_usage,
                                   CertifiedKey* key) {
  std::string nonce;
  if (!crypto_utility_->GetRandom(kNonceSize, &nonce)) {
    LOG(ERROR) << __func__ << ": GetRandom(nonce) failed.";
    return false;
  }
  std::string key_blob;
  std::string public_key;
  std::string public_key_tpm_format;
  std::string key_info;
  std::string proof;
  auto database_pb = database_->GetProtobuf();
  if (!tpm_utility_->CreateCertifiedKey(
      key_type,
      key_usage,
      database_pb.identity_key().identity_key_blob(),
      nonce,
      &key_blob,
      &public_key,
      &public_key_tpm_format,
      &key_info,
      &proof)) {
    return false;
  }
  key->set_key_blob(key_blob);
  key->set_public_key(public_key);
  key->set_key_name(key_label);
  key->set_public_key_tpm_format(public_key_tpm_format);
  key->set_certified_key_info(key_info);
  key->set_certified_key_proof(proof);
  return SaveKey(username, key_label, *key);
}

bool AttestationService::SaveKey(const std::string& username,
                                 const std::string& key_label,
                                 const CertifiedKey& key) {
  if (!username.empty()) {
    std::string key_data;
    if (!key.SerializeToString(&key_data)) {
      LOG(ERROR) << __func__ << ": Failed to serialize protobuf.";
      return false;
    }
    if (!key_store_->Write(username, key_label, key_data)) {
      LOG(ERROR) << __func__ << ": Failed to store certified key for user.";
      return false;
    }
  } else {
    if (!AddDeviceKey(key_label, key)) {
      LOG(ERROR) << __func__ << ": Failed to store certified key for device.";
      return false;
    }
  }
  return true;
}

void AttestationService::DeleteKey(const std::string& username,
                                   const std::string& key_label) {
  if (!username.empty()) {
    key_store_->Delete(username, key_label);
  } else {
    RemoveDeviceKey(key_label);
  }
}

bool AttestationService::AddDeviceKey(const std::string& key_label,
                                      const CertifiedKey& key) {
  // If a key by this name already exists, reuse the field.
  auto* database_pb = database_->GetMutableProtobuf();
  bool found = false;
  for (int i = 0; i < database_pb->device_keys_size(); ++i) {
    if (database_pb->device_keys(i).key_name() == key_label) {
      found = true;
      *database_pb->mutable_device_keys(i) = key;
      break;
    }
  }
  if (!found)
    *database_pb->add_device_keys() = key;
  return database_->SaveChanges();
}

void AttestationService::RemoveDeviceKey(const std::string& key_label) {
  auto* database_pb = database_->GetMutableProtobuf();
  bool found = false;
  for (int i = 0; i < database_pb->device_keys_size(); ++i) {
    if (database_pb->device_keys(i).key_name() == key_label) {
      found = true;
      int last = database_pb->device_keys_size() - 1;
      if (i < last) {
        database_pb->mutable_device_keys()->SwapElements(i, last);
      }
      database_pb->mutable_device_keys()->RemoveLast();
      break;
    }
  }
  if (found) {
    if (!database_->SaveChanges()) {
      LOG(WARNING) << __func__ << ": Failed to persist key deletion.";
    }
  }
}

std::string AttestationService::CreatePEMCertificateChain(
    const CertifiedKey& key) {
  if (key.certified_key_credential().empty()) {
    LOG(WARNING) << "Certificate is empty.";
    return std::string();
  }
  std::string pem = CreatePEMCertificate(key.certified_key_credential());
  if (!key.intermediate_ca_cert().empty()) {
    pem += "\n";
    pem += CreatePEMCertificate(key.intermediate_ca_cert());
  }
  for (int i = 0; i < key.additional_intermediate_ca_cert_size(); ++i) {
    pem += "\n";
    pem += CreatePEMCertificate(key.additional_intermediate_ca_cert(i));
  }
  return pem;
}

std::string AttestationService::CreatePEMCertificate(
    const std::string& certificate) {
  const char kBeginCertificate[] = "-----BEGIN CERTIFICATE-----\n";
  const char kEndCertificate[] = "-----END CERTIFICATE-----";

  std::string pem = kBeginCertificate;
  pem += brillo::data_encoding::Base64EncodeWrapLines(certificate);
  pem += kEndCertificate;
  return pem;
}


int AttestationService::ChooseTemporalIndex(const std::string& user,
                                            const std::string& origin) {
  std::string user_hash = crypto::SHA256HashString(user);
  std::string origin_hash = crypto::SHA256HashString(origin);
  int histogram[kNumTemporalValues] = {};
  auto database_pb = database_->GetProtobuf();
  for (int i = 0; i < database_pb.temporal_index_record_size(); ++i) {
    const AttestationDatabase::TemporalIndexRecord& record =
        database_pb.temporal_index_record(i);
    // Ignore out-of-range index values.
    if (record.temporal_index() < 0 ||
        record.temporal_index() >= kNumTemporalValues)
      continue;
    if (record.origin_hash() == origin_hash) {
      if (record.user_hash() == user_hash) {
        // We've previously chosen this index for this user, reuse it.
        return record.temporal_index();
      } else {
        // We've previously chosen this index for another user.
        ++histogram[record.temporal_index()];
      }
    }
  }
  int least_used_index = 0;
  for (int i = 1; i < kNumTemporalValues; ++i) {
    if (histogram[i] < histogram[least_used_index])
      least_used_index = i;
  }
  if (histogram[least_used_index] > 0) {
    LOG(WARNING) << "Unique origin-specific identifiers have been exhausted.";
  }
  // Record our choice for later reference.
  AttestationDatabase::TemporalIndexRecord* new_record =
      database_pb.add_temporal_index_record();
  new_record->set_origin_hash(origin_hash);
  new_record->set_user_hash(user_hash);
  new_record->set_temporal_index(least_used_index);
  database_->SaveChanges();
  return least_used_index;
}

std::string AttestationService::GetACAURL(ACARequestType request_type) const {
  std::string url = attestation_ca_origin_;
  switch (request_type) {
    case kEnroll:
      url += "/enroll";
      break;
    case kGetCertificate:
      url += "/sign";
      break;
    default:
      NOTREACHED();
  }
  return url;
}

bool AttestationService::GetSubjectPublicKeyInfo(
    KeyType key_type,
    const std::string& public_key,
    std::string* public_key_info) const {
  // Only RSA is supported currently.
  if (key_type != KEY_TYPE_RSA) {
    return false;
  }
  return crypto_utility_->GetRSASubjectPublicKeyInfo(public_key,
                                                     public_key_info);
}

base::WeakPtr<AttestationService> AttestationService::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

}  // namespace attestation
