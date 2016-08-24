// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_PRIVET_SECURITY_DELEGATE_H_
#define LIBWEAVE_SRC_PRIVET_SECURITY_DELEGATE_H_

#include <memory>
#include <set>
#include <string>

#include <base/time/time.h>

#include "src/privet/privet_types.h"

namespace weave {
namespace privet {

// Interface to provide Security related logic for |PrivetHandler|.
class SecurityDelegate {
 public:
  virtual ~SecurityDelegate() {}

  // Creates access token for the given scope, user id and |time|.
  virtual bool CreateAccessToken(AuthType auth_type,
                                 const std::string& auth_code,
                                 AuthScope desired_scope,
                                 std::string* access_token,
                                 AuthScope* granted_scope,
                                 base::TimeDelta* ttl,
                                 ErrorPtr* error) = 0;

  // Validates |token| and returns scope, user id parsed from that.
  virtual bool ParseAccessToken(const std::string& token,
                                UserInfo* user_info,
                                ErrorPtr* error) const = 0;

  // Returns list of pairing methods by device.
  virtual std::set<PairingType> GetPairingTypes() const = 0;

  // Returns list of crypto methods supported by devices.
  virtual std::set<CryptoType> GetCryptoTypes() const = 0;

  // Returns list of auth methods supported by devices.
  virtual std::set<AuthType> GetAuthTypes() const = 0;

  // Returns Root Client Authorization Token.
  virtual std::string ClaimRootClientAuthToken(ErrorPtr* error) = 0;

  // Confirms pending pending token claim or checks that token is valid for the
  // active secret.
  virtual bool ConfirmClientAuthToken(const std::string& token,
                                      ErrorPtr* error) = 0;

  virtual bool StartPairing(PairingType mode,
                            CryptoType crypto,
                            std::string* session_id,
                            std::string* device_commitment,
                            ErrorPtr* error) = 0;

  virtual bool ConfirmPairing(const std::string& session_id,
                              const std::string& client_commitment,
                              std::string* fingerprint,
                              std::string* signature,
                              ErrorPtr* error) = 0;

  virtual bool CancelPairing(const std::string& session_id,
                             ErrorPtr* error) = 0;

  virtual std::string CreateSessionId() = 0;
};

}  // namespace privet
}  // namespace weave

#endif  // LIBWEAVE_SRC_PRIVET_SECURITY_DELEGATE_H_
