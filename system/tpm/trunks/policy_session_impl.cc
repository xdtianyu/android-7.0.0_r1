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

#include "trunks/policy_session_impl.h"

#include <string>
#include <vector>

#include <base/logging.h>
#include <base/macros.h>
#include <base/stl_util.h>
#include <crypto/sha2.h>
#include <openssl/rand.h>

#include "trunks/error_codes.h"
#include "trunks/tpm_generated.h"

namespace trunks {

PolicySessionImpl::PolicySessionImpl(const TrunksFactory& factory)
    : factory_(factory),
      session_type_(TPM_SE_POLICY) {
  session_manager_ = factory_.GetSessionManager();
}

PolicySessionImpl::PolicySessionImpl(const TrunksFactory& factory,
                                     TPM_SE session_type)
    : factory_(factory),
      session_type_(session_type) {
  session_manager_ = factory_.GetSessionManager();
}

PolicySessionImpl::~PolicySessionImpl() {
  session_manager_->CloseSession();
}

AuthorizationDelegate* PolicySessionImpl::GetDelegate() {
  if (session_manager_->GetSessionHandle() == kUninitializedHandle) {
    return nullptr;
  }
  return &hmac_delegate_;
}

TPM_RC PolicySessionImpl::StartBoundSession(
    TPMI_DH_ENTITY bind_entity,
    const std::string& bind_authorization_value,
    bool enable_encryption) {
  hmac_delegate_.set_use_entity_authorization_for_encryption_only(true);
  if (session_type_ != TPM_SE_POLICY && session_type_ != TPM_SE_TRIAL) {
    LOG(ERROR) << "Cannot start a session of that type.";
    return SAPI_RC_INVALID_SESSIONS;
  }
  return session_manager_->StartSession(session_type_, bind_entity,
                                        bind_authorization_value,
                                        enable_encryption, &hmac_delegate_);
}

TPM_RC PolicySessionImpl::StartUnboundSession(bool enable_encryption) {
  // Just like a HmacAuthorizationSession, an unbound policy session is just
  // a session bound to TPM_RH_NULL.
  return StartBoundSession(TPM_RH_NULL, "", enable_encryption);
}

TPM_RC PolicySessionImpl::GetDigest(std::string* digest) {
  CHECK(digest);
  TPM2B_DIGEST policy_digest;
  TPM_RC result = factory_.GetTpm()->PolicyGetDigestSync(
      session_manager_->GetSessionHandle(),
      "",  // No name is needed for this command, as it does no authorization.
      &policy_digest,
      nullptr);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error getting policy digest: " << GetErrorString(result);
    return result;
  }
  *digest = StringFrom_TPM2B_DIGEST(policy_digest);
  return TPM_RC_SUCCESS;
}

TPM_RC PolicySessionImpl::PolicyOR(const std::vector<std::string>& digests) {
  if (digests.size() >= arraysize(TPML_DIGEST::digests)) {
    LOG(ERROR) << "TPM2.0 Spec only allows for up to 8 digests.";
    return SAPI_RC_BAD_PARAMETER;
  }
  TPML_DIGEST tpm_digests;
  tpm_digests.count = digests.size();
  for (size_t i = 0; i < digests.size(); i++) {
    tpm_digests.digests[i] = Make_TPM2B_DIGEST(digests[i]);
  }
  TPM_RC result = factory_.GetTpm()->PolicyORSync(
      session_manager_->GetSessionHandle(),
      "",  // No policy name is needed as we do no authorization checks.
      tpm_digests,
      nullptr);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error performing PolicyOR: " << GetErrorString(result);
    return result;
  }

  return TPM_RC_SUCCESS;
}

TPM_RC PolicySessionImpl::PolicyPCR(uint32_t pcr_index,
                                    const std::string& pcr_value) {
  TPML_PCR_SELECTION pcr_select;
  memset(&pcr_select, 0, sizeof(TPML_PCR_SELECTION));
  // This process of selecting pcrs is highlighted in TPM 2.0 Library Spec
  // Part 2 (Section 10.5 - PCR structures).
  uint8_t pcr_select_index = pcr_index / 8;
  uint8_t pcr_select_byte = 1 << (pcr_index % 8);
  pcr_select.count = 1;
  pcr_select.pcr_selections[0].hash = TPM_ALG_SHA256;
  pcr_select.pcr_selections[0].sizeof_select = PCR_SELECT_MIN;
  pcr_select.pcr_selections[0].pcr_select[pcr_select_index] = pcr_select_byte;
  TPM2B_DIGEST pcr_digest;
  if (pcr_value.empty()) {
    if (session_type_ == TPM_SE_TRIAL) {
      LOG(ERROR) << "Trial sessions have to define a PCR value.";
      return SAPI_RC_BAD_PARAMETER;
    }
    pcr_digest = Make_TPM2B_DIGEST("");
  } else {
    pcr_digest = Make_TPM2B_DIGEST(crypto::SHA256HashString(pcr_value));
  }

  TPM_RC result = factory_.GetTpm()->PolicyPCRSync(
      session_manager_->GetSessionHandle(),
      "",  // No policy name is needed as we do no authorization checks.
      pcr_digest,
      pcr_select,
      nullptr);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error performing PolicyPCR: " << GetErrorString(result);
    return result;
  }
  return TPM_RC_SUCCESS;
}

TPM_RC PolicySessionImpl::PolicyCommandCode(TPM_CC command_code) {
  TPM_RC result = factory_.GetTpm()->PolicyCommandCodeSync(
      session_manager_->GetSessionHandle(),
      "",  // No policy name is needed as we do no authorization checks.
      command_code,
      nullptr);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error performing PolicyCommandCode: "
               << GetErrorString(result);
    return result;
  }
  return TPM_RC_SUCCESS;
}

TPM_RC PolicySessionImpl::PolicyAuthValue() {
  TPM_RC result = factory_.GetTpm()->PolicyAuthValueSync(
      session_manager_->GetSessionHandle(),
      "",  // No policy name is needed as we do no authorization checks.
      nullptr);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error performing PolicyAuthValue: "
               << GetErrorString(result);
    return result;
  }
  hmac_delegate_.set_use_entity_authorization_for_encryption_only(false);
  return TPM_RC_SUCCESS;
}

void PolicySessionImpl::SetEntityAuthorizationValue(const std::string& value) {
  hmac_delegate_.set_entity_authorization_value(value);
}

}  // namespace trunks
