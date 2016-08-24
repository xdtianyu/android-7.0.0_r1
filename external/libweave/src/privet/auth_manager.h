// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_PRIVET_AUTH_MANAGER_H_
#define LIBWEAVE_SRC_PRIVET_AUTH_MANAGER_H_

#include <deque>
#include <string>
#include <vector>

#include <base/gtest_prod_util.h>
#include <base/time/default_clock.h>
#include <base/time/time.h>
#include <weave/error.h>

#include "src/privet/privet_types.h"

namespace weave {

class Config;
enum class RootClientTokenOwner;

namespace privet {

class AuthManager {
 public:
  AuthManager(Config* config,
              const std::vector<uint8_t>& certificate_fingerprint);

  // Constructor for tests.
  AuthManager(const std::vector<uint8_t>& auth_secret,
              const std::vector<uint8_t>& certificate_fingerprint,
              const std::vector<uint8_t>& access_secret,
              base::Clock* clock = nullptr);
  ~AuthManager();

  std::vector<uint8_t> CreateAccessToken(const UserInfo& user_info,
                                         base::TimeDelta ttl) const;

  bool ParseAccessToken(const std::vector<uint8_t>& token,
                        UserInfo* user_info,
                        ErrorPtr* error) const;

  const std::vector<uint8_t>& GetAuthSecret() const { return auth_secret_; }
  const std::vector<uint8_t>& GetAccessSecret() const { return access_secret_; }
  const std::vector<uint8_t>& GetCertificateFingerprint() const {
    return certificate_fingerprint_;
  }

  base::Time Now() const;

  std::vector<uint8_t> ClaimRootClientAuthToken(RootClientTokenOwner owner,
                                                ErrorPtr* error);
  bool ConfirmClientAuthToken(const std::vector<uint8_t>& token,
                              ErrorPtr* error);

  std::vector<uint8_t> GetRootClientAuthToken(RootClientTokenOwner owner) const;
  bool IsValidAuthToken(const std::vector<uint8_t>& token,
                        ErrorPtr* error) const;
  bool CreateAccessTokenFromAuth(const std::vector<uint8_t>& auth_token,
                                 base::TimeDelta ttl,
                                 std::vector<uint8_t>* access_token,
                                 AuthScope* access_token_scope,
                                 base::TimeDelta* access_token_ttl,
                                 ErrorPtr* error) const;

  void SetAuthSecret(const std::vector<uint8_t>& secret,
                     RootClientTokenOwner owner);

  std::string CreateSessionId() const;
  bool IsValidSessionId(const std::string& session_id) const;

 private:
  friend class AuthManagerTest;

  // Test helpers. Device does not need to implement delegation.
  std::vector<uint8_t> DelegateToUser(const std::vector<uint8_t>& token,
                                      base::TimeDelta ttl,
                                      const UserInfo& user_info) const;

  Config* config_{nullptr};  // Can be nullptr for tests.
  base::DefaultClock default_clock_;
  base::Clock* clock_{&default_clock_};
  mutable uint32_t session_counter_{0};

  std::vector<uint8_t> auth_secret_;  // Persistent.
  std::vector<uint8_t> certificate_fingerprint_;
  std::vector<uint8_t> access_secret_;  // New on every reboot.

  std::deque<std::pair<std::unique_ptr<AuthManager>, RootClientTokenOwner>>
      pending_claims_;

  DISALLOW_COPY_AND_ASSIGN(AuthManager);
};

}  // namespace privet
}  // namespace weave

#endif  // LIBWEAVE_SRC_PRIVET_AUTH_MANAGER_H_
