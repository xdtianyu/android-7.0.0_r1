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

#include "trunks/trunks_factory_impl.h"

#include <base/logging.h>

#include "trunks/blob_parser.h"
#include "trunks/hmac_session_impl.h"
#include "trunks/password_authorization_delegate.h"
#include "trunks/policy_session_impl.h"
#include "trunks/session_manager_impl.h"
#include "trunks/tpm_generated.h"
#include "trunks/tpm_state_impl.h"
#include "trunks/tpm_utility_impl.h"
#if defined(USE_BINDER_IPC)
#include "trunks/trunks_binder_proxy.h"
#else
#include "trunks/trunks_dbus_proxy.h"
#endif

namespace trunks {

TrunksFactoryImpl::TrunksFactoryImpl(bool failure_is_fatal) {
#if defined(USE_BINDER_IPC)
  default_transceiver_.reset(new TrunksBinderProxy());
#else
  default_transceiver_.reset(new TrunksDBusProxy());
#endif
  transceiver_ = default_transceiver_.get();
  tpm_.reset(new Tpm(transceiver_));
  if (!transceiver_->Init()) {
    if (failure_is_fatal) {
      LOG(FATAL) << "Error initializing default IPC proxy.";
    } else {
      LOG(ERROR) << "Error initializing default IPC proxy.";
    }
  }
}

TrunksFactoryImpl::TrunksFactoryImpl(CommandTransceiver* transceiver) {
  transceiver_ = transceiver;
  tpm_.reset(new Tpm(transceiver_));
}

TrunksFactoryImpl::~TrunksFactoryImpl() {}

Tpm* TrunksFactoryImpl::GetTpm() const {
  return tpm_.get();
}

scoped_ptr<TpmState> TrunksFactoryImpl::GetTpmState() const {
  return scoped_ptr<TpmState>(new TpmStateImpl(*this));
}

scoped_ptr<TpmUtility> TrunksFactoryImpl::GetTpmUtility() const {
  return scoped_ptr<TpmUtility>(new TpmUtilityImpl(*this));
}

scoped_ptr<AuthorizationDelegate> TrunksFactoryImpl::GetPasswordAuthorization(
    const std::string& password) const {
  return scoped_ptr<AuthorizationDelegate>(
      new PasswordAuthorizationDelegate(password));
}

scoped_ptr<SessionManager> TrunksFactoryImpl::GetSessionManager() const {
  return scoped_ptr<SessionManager>(new SessionManagerImpl(*this));
}

scoped_ptr<HmacSession> TrunksFactoryImpl::GetHmacSession() const {
  return scoped_ptr<HmacSession>(new HmacSessionImpl(*this));
}

scoped_ptr<PolicySession> TrunksFactoryImpl::GetPolicySession() const {
  return scoped_ptr<PolicySession>(new PolicySessionImpl(*this, TPM_SE_POLICY));
}

scoped_ptr<PolicySession> TrunksFactoryImpl::GetTrialSession() const {
  return scoped_ptr<PolicySession>(new PolicySessionImpl(*this, TPM_SE_TRIAL));
}

scoped_ptr<BlobParser> TrunksFactoryImpl::GetBlobParser() const {
  return scoped_ptr<BlobParser>(new BlobParser());
}

}  // namespace trunks
