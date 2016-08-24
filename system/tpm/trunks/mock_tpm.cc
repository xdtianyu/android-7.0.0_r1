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

#include "trunks/mock_tpm.h"

#include "trunks/tpm_utility.h"

using testing::_;
using testing::DoAll;
using testing::Return;
using testing::SetArgPointee;

namespace trunks {

MockTpm::MockTpm() : Tpm(nullptr) {
  ON_CALL(*this, PCR_AllocateSync(_, _, _, _, _, _, _, _))
      .WillByDefault(DoAll(SetArgPointee<3>(YES),
                           Return(TPM_RC_SUCCESS)));
}

MockTpm::~MockTpm() {}

void MockTpm::StartAuthSession(
    const TPMI_DH_OBJECT& tpm_key,
    const std::string& tpm_key_name,
    const TPMI_DH_ENTITY& bind,
    const std::string& bind_name,
    const TPM2B_NONCE& nonce_caller,
    const TPM2B_ENCRYPTED_SECRET& encrypted_salt,
    const TPM_SE& session_type,
    const TPMT_SYM_DEF& symmetric,
    const TPMI_ALG_HASH& auth_hash,
    AuthorizationDelegate* authorization_delegate,
    const StartAuthSessionResponse& callback) {
  StartAuthSessionShort(tpm_key, bind, nonce_caller, encrypted_salt,
                        session_type, symmetric, auth_hash,
                        authorization_delegate, callback);
}

TPM_RC MockTpm::StartAuthSessionSync(
    const TPMI_DH_OBJECT& tpm_key,
    const std::string& tpm_key_name,
    const TPMI_DH_ENTITY& bind,
    const std::string& bind_name,
    const TPM2B_NONCE& nonce_caller,
    const TPM2B_ENCRYPTED_SECRET& encrypted_salt,
    const TPM_SE& session_type,
    const TPMT_SYM_DEF& symmetric,
    const TPMI_ALG_HASH& auth_hash,
    TPMI_SH_AUTH_SESSION* session_handle,
    TPM2B_NONCE* nonce_tpm,
    AuthorizationDelegate* authorization_delegate) {
  return StartAuthSessionSyncShort(tpm_key, bind, nonce_caller,
                                   encrypted_salt, session_type, symmetric,
                                   auth_hash, session_handle, nonce_tpm,
                                   authorization_delegate);
}
TPM_RC MockTpm::CreateSync(
    const TPMI_DH_OBJECT& parent_handle,
    const std::string& parent_handle_name,
    const TPM2B_SENSITIVE_CREATE& in_sensitive,
    const TPM2B_PUBLIC& in_public,
    const TPM2B_DATA& outside_info,
    const TPML_PCR_SELECTION& creation_pcr,
    TPM2B_PRIVATE* out_private,
    TPM2B_PUBLIC* out_public,
    TPM2B_CREATION_DATA* creation_data,
    TPM2B_DIGEST* creation_hash,
    TPMT_TK_CREATION* creation_ticket,
    AuthorizationDelegate* authorization_delegate) {
  return CreateSyncShort(parent_handle, in_sensitive, in_public, creation_pcr,
                         out_private, out_public, creation_data,
                         creation_hash, creation_ticket,
                         authorization_delegate);
}
TPM_RC MockTpm::CertifyCreationSync(
    const TPMI_DH_OBJECT& sign_handle,
    const std::string& sign_handle_name,
    const TPMI_DH_OBJECT& object_handle,
    const std::string& object_handle_name,
    const TPM2B_DATA& qualifying_data,
    const TPM2B_DIGEST& creation_hash,
    const TPMT_SIG_SCHEME& in_scheme,
    const TPMT_TK_CREATION& creation_ticket,
    TPM2B_ATTEST* certify_info,
    TPMT_SIGNATURE* signature,
    AuthorizationDelegate* authorization_delegate) {
  return CertifyCreationSyncShort(sign_handle, object_handle,
                                  qualifying_data, creation_hash, in_scheme,
                                  creation_ticket, certify_info, signature,
                                  authorization_delegate);
}
TPM_RC MockTpm::GetSessionAuditDigestSync(
    const TPMI_RH_ENDORSEMENT& privacy_admin_handle,
    const std::string& privacy_admin_handle_name,
    const TPMI_DH_OBJECT& sign_handle,
    const std::string& sign_handle_name,
    const TPMI_SH_HMAC& session_handle,
    const std::string& session_handle_name,
    const TPM2B_DATA& qualifying_data,
    const TPMT_SIG_SCHEME& in_scheme,
    TPM2B_ATTEST* audit_info,
    TPMT_SIGNATURE* signature,
    AuthorizationDelegate* authorization_delegate) {
  return GetSessionAuditDigestSyncShort(privacy_admin_handle, sign_handle,
                                        session_handle, qualifying_data,
                                        in_scheme, audit_info, signature,
                                        authorization_delegate);
}
TPM_RC MockTpm::CommitSync(
    const TPMI_DH_OBJECT& sign_handle,
    const std::string& sign_handle_name,
    const UINT32& param_size,
    const TPM2B_ECC_POINT& p1,
    const TPM2B_SENSITIVE_DATA& s2,
    const TPM2B_ECC_PARAMETER& y2,
    UINT32* param_size_out,
    TPM2B_ECC_POINT* k,
    TPM2B_ECC_POINT* l,
    TPM2B_ECC_POINT* e,
    UINT16* counter,
    AuthorizationDelegate* authorization_delegate) {
  return CommitSyncShort(sign_handle, param_size, p1, y2, param_size_out, k,
                         l, e, counter, authorization_delegate);
}
void MockTpm::PolicySigned(
    const TPMI_DH_OBJECT& auth_object,
    const std::string& auth_object_name,
    const TPMI_SH_POLICY& policy_session,
    const std::string& policy_session_name,
    const TPM2B_NONCE& nonce_tpm,
    const TPM2B_DIGEST& cp_hash_a,
    const TPM2B_NONCE& policy_ref,
    const INT32& expiration,
    const TPMT_SIGNATURE& auth,
    AuthorizationDelegate* authorization_delegate,
    const PolicySignedResponse& callback) {
  PolicySignedShort(auth_object, policy_session, nonce_tpm, cp_hash_a,
                    policy_ref, expiration, auth, authorization_delegate,
                    callback);
}
TPM_RC MockTpm::PolicySignedSync(
    const TPMI_DH_OBJECT& auth_object,
    const std::string& auth_object_name,
    const TPMI_SH_POLICY& policy_session,
    const std::string& policy_session_name,
    const TPM2B_NONCE& nonce_tpm,
    const TPM2B_DIGEST& cp_hash_a,
    const TPM2B_NONCE& policy_ref,
    const INT32& expiration,
    const TPMT_SIGNATURE& auth,
    TPM2B_TIMEOUT* timeout,
    TPMT_TK_AUTH* policy_ticket,
    AuthorizationDelegate* authorization_delegate) {
  return PolicySignedSyncShort(auth_object, policy_session, nonce_tpm,
                               cp_hash_a, policy_ref, expiration, auth, timeout,
                               policy_ticket, authorization_delegate);
}
TPM_RC MockTpm::PolicySecretSync(
    const TPMI_DH_ENTITY& auth_handle,
    const std::string& auth_handle_name,
    const TPMI_SH_POLICY& policy_session,
    const std::string& policy_session_name,
    const TPM2B_NONCE& nonce_tpm,
    const TPM2B_DIGEST& cp_hash_a,
    const TPM2B_NONCE& policy_ref,
    const INT32& expiration,
    TPM2B_TIMEOUT* timeout,
    TPMT_TK_AUTH* policy_ticket,
    AuthorizationDelegate* authorization_delegate) {
  return PolicySecretSyncShort(auth_handle, policy_session, nonce_tpm,
                               cp_hash_a, policy_ref, expiration, timeout,
                               policy_ticket, authorization_delegate);
}
void MockTpm::PolicyNV(const TPMI_RH_NV_AUTH& auth_handle,
                       const std::string& auth_handle_name,
                       const TPMI_RH_NV_INDEX& nv_index,
                       const std::string& nv_index_name,
                       const TPMI_SH_POLICY& policy_session,
                       const std::string& policy_session_name,
                       const TPM2B_OPERAND& operand_b,
                       const UINT16& offset,
                       const TPM_EO& operation,
                       AuthorizationDelegate* authorization_delegate,
                       const PolicyNVResponse& callback) {
  PolicyNVShort(auth_handle, nv_index, policy_session, operand_b, offset,
                operation, authorization_delegate, callback);
}
TPM_RC MockTpm::CreatePrimarySync(
    const TPMI_RH_HIERARCHY& primary_handle,
    const std::string& primary_handle_name,
    const TPM2B_SENSITIVE_CREATE& in_sensitive,
    const TPM2B_PUBLIC& in_public,
    const TPM2B_DATA& outside_info,
    const TPML_PCR_SELECTION& creation_pcr,
    TPM_HANDLE* object_handle,
    TPM2B_PUBLIC* out_public,
    TPM2B_CREATION_DATA* creation_data,
    TPM2B_DIGEST* creation_hash,
    TPMT_TK_CREATION* creation_ticket,
    TPM2B_NAME* name,
    AuthorizationDelegate* authorization_delegate) {
  return CreatePrimarySyncShort(primary_handle, in_public, creation_pcr,
                                object_handle, out_public, creation_data,
                                creation_hash, creation_ticket, name,
                                authorization_delegate);
}
void MockTpm::NV_Certify(const TPMI_DH_OBJECT& sign_handle,
                         const std::string& sign_handle_name,
                         const TPMI_RH_NV_AUTH& auth_handle,
                         const std::string& auth_handle_name,
                         const TPMI_RH_NV_INDEX& nv_index,
                         const std::string& nv_index_name,
                         const TPM2B_DATA& qualifying_data,
                         const TPMT_SIG_SCHEME& in_scheme,
                         const UINT16& size,
                         const UINT16& offset,
                         AuthorizationDelegate* authorization_delegate,
                         const NV_CertifyResponse& callback) {
  NV_CertifyShort(sign_handle, auth_handle, nv_index, qualifying_data,
                  in_scheme, size, offset, authorization_delegate, callback);
}
TPM_RC MockTpm::NV_CertifySync(
    const TPMI_DH_OBJECT& sign_handle,
    const std::string& sign_handle_name,
    const TPMI_RH_NV_AUTH& auth_handle,
    const std::string& auth_handle_name,
    const TPMI_RH_NV_INDEX& nv_index,
    const std::string& nv_index_name,
    const TPM2B_DATA& qualifying_data,
    const TPMT_SIG_SCHEME& in_scheme,
    const UINT16& size,
    const UINT16& offset,
    TPM2B_ATTEST* certify_info,
    TPMT_SIGNATURE* signature,
    AuthorizationDelegate* authorization_delegate) {
  return NV_CertifySyncShort(sign_handle, auth_handle, nv_index,
                             qualifying_data, in_scheme, size, offset,
                             certify_info, signature, authorization_delegate);
}

}  // namespace trunks
