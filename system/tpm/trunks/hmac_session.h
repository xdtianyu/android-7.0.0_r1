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

#ifndef TRUNKS_HMAC_SESSION_H_
#define TRUNKS_HMAC_SESSION_H_

#include <string>

#include <base/macros.h>

#include "trunks/tpm_generated.h"

namespace trunks {

class AuthorizationDelegate;

// HmacSession is an interface for managing hmac backed sessions for
// authorization and parameter encryption.
class HmacSession {
 public:
  HmacSession() {}
  virtual ~HmacSession() {}

  // Returns an authorization delegate for this session. Ownership of the
  // delegate pointer is retained by the session.
  virtual AuthorizationDelegate* GetDelegate() = 0;

  // Starts a salted session which is bound to |bind_entity| with
  // |bind_authorization_value|. Encryption is enabled if |enable_encryption| is
  // true. The session remains active until this object is destroyed or another
  // session is started with a call to Start*Session.
  virtual TPM_RC StartBoundSession(
      TPMI_DH_ENTITY bind_entity,
      const std::string& bind_authorization_value,
      bool enable_encryption) = 0;

  // Starts a salted, unbound session. Encryption is enabled if
  // |enable_encryption| is true. The session remains active until this object
  // is destroyed or another session is started with a call to Start*Session.
  virtual TPM_RC StartUnboundSession(bool enable_encryption) = 0;

  // Sets the current entity authorization value. This can be safely called
  // while the session is active and subsequent commands will use the value.
  virtual void SetEntityAuthorizationValue(const std::string& value) = 0;

  // Sets the future_authorization_value field in the HmacDelegate. This
  // is used in response validation for the TPM2_HierarchyChangeAuth command.
  // We need to perform this because the HMAC value returned from
  // HierarchyChangeAuth uses the new auth_value.
  virtual void SetFutureAuthorizationValue(const std::string& value) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(HmacSession);
};

}  // namespace trunks

#endif  // TRUNKS_HMAC_SESSION_H_
