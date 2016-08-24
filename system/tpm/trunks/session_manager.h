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

#ifndef TRUNKS_SESSION_MANAGER_H_
#define TRUNKS_SESSION_MANAGER_H_

#include <string>

#include "trunks/hmac_authorization_delegate.h"
#include "trunks/tpm_generated.h"
#include "trunks/trunks_export.h"
#include "trunks/trunks_factory.h"

namespace trunks {

const trunks::TPM_HANDLE kUninitializedHandle = 0;

// This class is used to keep track of a TPM session. Each instance of this
// class is used to account for one instance of a TPM session. Currently
// this class is used by AuthorizationSession instances to keep track of TPM
// sessions.
// Note: This class is not intended to be used independently. However clients
// who want to manually manage their sessions can use this class to Start and
// Close TPM backed Sessions. Example usage:
// trunks::TrunksFactoryImpl factory;
// scoped_ptr<SessionManager> session_manager = factory.GetSessionManager();
// session_manager->StartSession(...);
// TPM_HANDLE session_handle = session_manager->GetSessionHandle();
class TRUNKS_EXPORT SessionManager {
 public:
  SessionManager() {}
  virtual ~SessionManager() {}

  // This method is used get the handle to the AuthorizationSession managed by
  // this instance.
  virtual TPM_HANDLE GetSessionHandle() const = 0;

  // This method is used to flush all TPM context associated with the current
  // session
  virtual void CloseSession() = 0;

  // This method is used to start a new AuthorizationSession. Once started,
  // GetSessionHandle() can be used to access the handle to the TPM session.
  // Since the sessions are salted, we need to ensure that TPM ownership is
  // taken and the salting key created before this method is called.
  // Returns TPM_RC_SUCCESS and returns the nonces used to create the session
  // on success.
  virtual TPM_RC StartSession(TPM_SE session_type,
                              TPMI_DH_ENTITY bind_entity,
                              const std::string& bind_authorization_value,
                              bool enable_encryption,
                              HmacAuthorizationDelegate* delegate) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(SessionManager);
};

}  // namespace trunks

#endif  // TRUNKS_SESSION_MANAGER_H_
