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

#include "trunks/hmac_session_impl.h"

#include <string>

#include <base/logging.h>
#include <base/macros.h>
#include <base/stl_util.h>
#include <openssl/rand.h>

namespace trunks {

HmacSessionImpl::HmacSessionImpl(const TrunksFactory& factory)
    : factory_(factory) {
  session_manager_ = factory_.GetSessionManager();
}

HmacSessionImpl::~HmacSessionImpl() {
  session_manager_->CloseSession();
}

AuthorizationDelegate* HmacSessionImpl::GetDelegate() {
  if (session_manager_->GetSessionHandle() == kUninitializedHandle) {
    return nullptr;
  }
  return &hmac_delegate_;
}

TPM_RC HmacSessionImpl::StartBoundSession(
    TPMI_DH_ENTITY bind_entity,
    const std::string& bind_authorization_value,
    bool enable_encryption) {
  return session_manager_->StartSession(TPM_SE_HMAC, bind_entity,
                                        bind_authorization_value,
                                        enable_encryption, &hmac_delegate_);
}

TPM_RC HmacSessionImpl::StartUnboundSession(bool enable_encryption) {
  // Starting an unbound session is the same as starting a session bound to
  // TPM_RH_NULL. In this case, the authorization is the zero length buffer.
  // We can therefore simply call StartBoundSession with TPM_RH_NULL as the
  // binding entity, and the empty string as the authorization.
  return StartBoundSession(TPM_RH_NULL, "", enable_encryption);
}

void HmacSessionImpl::SetEntityAuthorizationValue(
    const std::string& value) {
  hmac_delegate_.set_entity_authorization_value(value);
}

void HmacSessionImpl::SetFutureAuthorizationValue(
    const std::string& value) {
  hmac_delegate_.set_future_authorization_value(value);
}

}  // namespace trunks
