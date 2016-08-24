// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_PRIVET_SECURITY_MANAGER_H_
#define LIBWEAVE_SRC_PRIVET_SECURITY_MANAGER_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <base/callback.h>
#include <base/gtest_prod_util.h>
#include <base/memory/weak_ptr.h>
#include <weave/error.h>

#include "src/config.h"
#include "src/privet/security_delegate.h"

namespace crypto {
class P224EncryptedKeyExchange;
}  // namespace crypto

namespace weave {

namespace provider {
class TaskRunner;
}

namespace privet {

class AuthManager;

class SecurityManager : public SecurityDelegate {
 public:
  using PairingStartListener =
      base::Callback<void(const std::string& session_id,
                          PairingType pairing_type,
                          const std::vector<uint8_t>& code)>;
  using PairingEndListener =
      base::Callback<void(const std::string& session_id)>;

  class KeyExchanger {
   public:
    virtual ~KeyExchanger() {}

    virtual const std::string& GetMessage() = 0;
    virtual bool ProcessMessage(const std::string& message,
                                ErrorPtr* error) = 0;
    virtual const std::string& GetKey() const = 0;
  };

  SecurityManager(const Config* config,
                  AuthManager* auth_manager,
                  // TODO(vitalybuka): Remove task_runner.
                  provider::TaskRunner* task_runner);
  ~SecurityManager() override;

  // SecurityDelegate methods
  bool CreateAccessToken(AuthType auth_type,
                         const std::string& auth_code,
                         AuthScope desired_scope,
                         std::string* access_token,
                         AuthScope* access_token_scope,
                         base::TimeDelta* access_token_ttl,
                         ErrorPtr* error) override;
  bool ParseAccessToken(const std::string& token,
                        UserInfo* user_info,
                        ErrorPtr* error) const override;
  std::set<PairingType> GetPairingTypes() const override;
  std::set<CryptoType> GetCryptoTypes() const override;
  std::set<AuthType> GetAuthTypes() const override;
  std::string ClaimRootClientAuthToken(ErrorPtr* error) override;
  bool ConfirmClientAuthToken(const std::string& token,
                              ErrorPtr* error) override;
  bool StartPairing(PairingType mode,
                    CryptoType crypto,
                    std::string* session_id,
                    std::string* device_commitment,
                    ErrorPtr* error) override;

  bool ConfirmPairing(const std::string& session_id,
                      const std::string& client_commitment,
                      std::string* fingerprint,
                      std::string* signature,
                      ErrorPtr* error) override;
  bool CancelPairing(const std::string& session_id, ErrorPtr* error) override;
  std::string CreateSessionId() override;

  void RegisterPairingListeners(const PairingStartListener& on_start,
                                const PairingEndListener& on_end);

 private:
  const Config::Settings& GetSettings() const;
  bool IsValidPairingCode(const std::vector<uint8_t>& auth_code) const;
  FRIEND_TEST_ALL_PREFIXES(SecurityManagerTest, ThrottlePairing);
  // Allows limited number of new sessions without successful authorization.
  bool CheckIfPairingAllowed(ErrorPtr* error);
  bool ClosePendingSession(const std::string& session_id);
  bool CloseConfirmedSession(const std::string& session_id);
  bool CreateAccessTokenImpl(AuthType auth_type,
                             const std::vector<uint8_t>& auth_code,
                             AuthScope desired_scope,
                             std::vector<uint8_t>* access_token,
                             AuthScope* access_token_scope,
                             base::TimeDelta* access_token_ttl,
                             ErrorPtr* error);
  bool CreateAccessTokenImpl(AuthType auth_type,
                             AuthScope desired_scope,
                             std::vector<uint8_t>* access_token,
                             AuthScope* access_token_scope,
                             base::TimeDelta* access_token_ttl);
  bool IsAnonymousAuthSupported() const;
  bool IsPairingAuthSupported() const;
  bool IsLocalAuthSupported() const;

  const Config* config_{nullptr};
  AuthManager* auth_manager_{nullptr};

  // TODO(vitalybuka): Session cleanup can be done without posting tasks.
  provider::TaskRunner* task_runner_{nullptr};
  std::map<std::string, std::unique_ptr<KeyExchanger>> pending_sessions_;
  std::map<std::string, std::unique_ptr<KeyExchanger>> confirmed_sessions_;
  mutable int pairing_attemts_{0};
  mutable base::Time block_pairing_until_;
  PairingStartListener on_start_;
  PairingEndListener on_end_;
  uint64_t last_user_id_{0};

  base::WeakPtrFactory<SecurityManager> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(SecurityManager);
};

}  // namespace privet
}  // namespace weave

#endif  // LIBWEAVE_SRC_PRIVET_SECURITY_MANAGER_H_
