// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/privet/security_manager.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <set>

#include <base/bind.h>
#include <base/guid.h>
#include <base/logging.h>
#include <base/rand_util.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/stringprintf.h>
#include <base/time/time.h>
#include <weave/provider/task_runner.h>

#include "src/data_encoding.h"
#include "src/privet/auth_manager.h"
#include "src/privet/constants.h"
#include "src/privet/openssl_utils.h"
#include "src/string_utils.h"
#include "third_party/chromium/crypto/p224_spake.h"

namespace weave {
namespace privet {

namespace {

const int kSessionExpirationTimeMinutes = 5;
const int kPairingExpirationTimeMinutes = 5;
const int kMaxAllowedPairingAttemts = 3;
const int kPairingBlockingTimeMinutes = 1;

const int kAccessTokenExpirationSeconds = 3600;

class Spakep224Exchanger : public SecurityManager::KeyExchanger {
 public:
  explicit Spakep224Exchanger(const std::string& password)
      : spake_(crypto::P224EncryptedKeyExchange::kPeerTypeServer, password) {}
  ~Spakep224Exchanger() override = default;

  // SecurityManager::KeyExchanger methods.
  const std::string& GetMessage() override { return spake_.GetNextMessage(); }

  bool ProcessMessage(const std::string& message, ErrorPtr* error) override {
    switch (spake_.ProcessMessage(message)) {
      case crypto::P224EncryptedKeyExchange::kResultPending:
        return true;
      case crypto::P224EncryptedKeyExchange::kResultFailed:
        return Error::AddTo(error, FROM_HERE, errors::kInvalidClientCommitment,
                            spake_.error());
      default:
        LOG(FATAL) << "SecurityManager uses only one round trip";
    }
    return false;
  }

  const std::string& GetKey() const override {
    return spake_.GetUnverifiedKey();
  }

 private:
  crypto::P224EncryptedKeyExchange spake_;
};

}  // namespace

SecurityManager::SecurityManager(const Config* config,
                                 AuthManager* auth_manager,
                                 provider::TaskRunner* task_runner)
    : config_{config}, auth_manager_{auth_manager}, task_runner_{task_runner} {
  CHECK(auth_manager_);
  CHECK_EQ(GetSettings().embedded_code.empty(),
           std::find(GetSettings().pairing_modes.begin(),
                     GetSettings().pairing_modes.end(),
                     PairingType::kEmbeddedCode) ==
               GetSettings().pairing_modes.end());
}

SecurityManager::~SecurityManager() {
  while (!pending_sessions_.empty())
    ClosePendingSession(pending_sessions_.begin()->first);
}

bool SecurityManager::CreateAccessTokenImpl(AuthType auth_type,
                                            AuthScope desired_scope,
                                            std::vector<uint8_t>* access_token,
                                            AuthScope* access_token_scope,
                                            base::TimeDelta* access_token_ttl) {
  auto user_id = std::to_string(++last_user_id_);
  UserInfo user_info{
      desired_scope,
      UserAppId{auth_type, {user_id.begin(), user_id.end()}, {}}};

  const base::TimeDelta kTtl =
      base::TimeDelta::FromSeconds(kAccessTokenExpirationSeconds);

  if (access_token)
    *access_token = auth_manager_->CreateAccessToken(user_info, kTtl);

  if (access_token_scope)
    *access_token_scope = user_info.scope();

  if (access_token_ttl)
    *access_token_ttl = kTtl;

  return true;
}

bool SecurityManager::CreateAccessTokenImpl(
    AuthType auth_type,
    const std::vector<uint8_t>& auth_code,
    AuthScope desired_scope,
    std::vector<uint8_t>* access_token,
    AuthScope* access_token_scope,
    base::TimeDelta* access_token_ttl,
    ErrorPtr* error) {
  auto disabled_mode = [](ErrorPtr* error) {
    return Error::AddTo(error, FROM_HERE, errors::kInvalidAuthMode,
                        "Mode is not available");
  };

  switch (auth_type) {
    case AuthType::kAnonymous:
      if (!IsAnonymousAuthSupported())
        return disabled_mode(error);
      return CreateAccessTokenImpl(auth_type, desired_scope, access_token,
                                   access_token_scope, access_token_ttl);
    case AuthType::kPairing:
      if (!IsPairingAuthSupported())
        return disabled_mode(error);
      if (!IsValidPairingCode(auth_code)) {
        return Error::AddTo(error, FROM_HERE, errors::kInvalidAuthCode,
                            "Invalid authCode");
      }
      return CreateAccessTokenImpl(auth_type, desired_scope, access_token,
                                   access_token_scope, access_token_ttl);
    case AuthType::kLocal:
      if (!IsLocalAuthSupported())
        return disabled_mode(error);
      const base::TimeDelta kTtl =
          base::TimeDelta::FromSeconds(kAccessTokenExpirationSeconds);
      return auth_manager_->CreateAccessTokenFromAuth(
          auth_code, kTtl, access_token, access_token_scope, access_token_ttl,
          error);
  }

  return Error::AddTo(error, FROM_HERE, errors::kInvalidAuthMode,
                      "Unsupported auth mode");
}

bool SecurityManager::CreateAccessToken(AuthType auth_type,
                                        const std::string& auth_code,
                                        AuthScope desired_scope,
                                        std::string* access_token,
                                        AuthScope* access_token_scope,
                                        base::TimeDelta* access_token_ttl,
                                        ErrorPtr* error) {
  std::vector<uint8_t> auth_decoded;
  if (auth_type != AuthType::kAnonymous &&
      !Base64Decode(auth_code, &auth_decoded)) {
    Error::AddToPrintf(error, FROM_HERE, errors::kInvalidAuthorization,
                       "Invalid auth_code encoding: %s", auth_code.c_str());
    return false;
  }

  std::vector<uint8_t> access_token_decoded;
  if (!CreateAccessTokenImpl(auth_type, auth_decoded, desired_scope,
                             &access_token_decoded, access_token_scope,
                             access_token_ttl, error)) {
    return false;
  }

  if (access_token)
    *access_token = Base64Encode(access_token_decoded);

  return true;
}

bool SecurityManager::ParseAccessToken(const std::string& token,
                                       UserInfo* user_info,
                                       ErrorPtr* error) const {
  std::vector<uint8_t> decoded;
  if (!Base64Decode(token, &decoded)) {
    Error::AddToPrintf(error, FROM_HERE, errors::kInvalidAuthorization,
                       "Invalid token encoding: %s", token.c_str());
    return false;
  }

  return auth_manager_->ParseAccessToken(decoded, user_info, error);
}

std::set<PairingType> SecurityManager::GetPairingTypes() const {
  return GetSettings().pairing_modes;
}

std::set<CryptoType> SecurityManager::GetCryptoTypes() const {
  std::set<CryptoType> result{CryptoType::kSpake_p224};
  return result;
}

std::set<AuthType> SecurityManager::GetAuthTypes() const {
  std::set<AuthType> result;
  if (IsAnonymousAuthSupported())
    result.insert(AuthType::kAnonymous);

  if (IsPairingAuthSupported())
    result.insert(AuthType::kPairing);

  if (IsLocalAuthSupported())
    result.insert(AuthType::kLocal);

  return result;
}

std::string SecurityManager::ClaimRootClientAuthToken(ErrorPtr* error) {
  return Base64Encode(auth_manager_->ClaimRootClientAuthToken(
      RootClientTokenOwner::kClient, error));
}

bool SecurityManager::ConfirmClientAuthToken(const std::string& token,
                                             ErrorPtr* error) {
  std::vector<uint8_t> token_decoded;
  if (!Base64Decode(token, &token_decoded)) {
    Error::AddToPrintf(error, FROM_HERE, errors::kInvalidFormat,
                       "Invalid auth token string: '%s'", token.c_str());
    return false;
  }
  return auth_manager_->ConfirmClientAuthToken(token_decoded, error);
}

const Config::Settings& SecurityManager::GetSettings() const {
  return config_->GetSettings();
}

bool SecurityManager::IsValidPairingCode(
    const std::vector<uint8_t>& auth_code) const {
  for (const auto& session : confirmed_sessions_) {
    const std::string& key = session.second->GetKey();
    const std::string& id = session.first;
    if (auth_code == HmacSha256(std::vector<uint8_t>(key.begin(), key.end()),
                                std::vector<uint8_t>(id.begin(), id.end()))) {
      pairing_attemts_ = 0;
      block_pairing_until_ = base::Time{};
      return true;
    }
  }
  LOG(ERROR) << "Attempt to authenticate with invalide code.";
  return false;
}

bool SecurityManager::StartPairing(PairingType mode,
                                   CryptoType crypto,
                                   std::string* session_id,
                                   std::string* device_commitment,
                                   ErrorPtr* error) {
  if (!CheckIfPairingAllowed(error))
    return false;

  const auto& pairing_modes = GetSettings().pairing_modes;
  if (std::find(pairing_modes.begin(), pairing_modes.end(), mode) ==
      pairing_modes.end()) {
    return Error::AddTo(error, FROM_HERE, errors::kInvalidParams,
                        "Pairing mode is not enabled");
  }

  std::string code;
  switch (mode) {
    case PairingType::kEmbeddedCode:
      CHECK(!GetSettings().embedded_code.empty());
      code = GetSettings().embedded_code;
      break;
    case PairingType::kPinCode:
      code = base::StringPrintf("%04i", base::RandInt(0, 9999));
      break;
    default:
      return Error::AddTo(error, FROM_HERE, errors::kInvalidParams,
                          "Unsupported pairing mode");
  }

  std::unique_ptr<KeyExchanger> spake;
  switch (crypto) {
    case CryptoType::kSpake_p224:
      spake.reset(new Spakep224Exchanger(code));
      break;
    // Fall through...
    default:
      return Error::AddTo(error, FROM_HERE, errors::kInvalidParams,
                          "Unsupported crypto");
  }

  // Allow only a single session at a time for now.
  while (!pending_sessions_.empty())
    ClosePendingSession(pending_sessions_.begin()->first);

  std::string session;
  do {
    session = base::GenerateGUID();
  } while (confirmed_sessions_.find(session) != confirmed_sessions_.end() ||
           pending_sessions_.find(session) != pending_sessions_.end());
  std::string commitment = spake->GetMessage();
  pending_sessions_.insert(std::make_pair(session, std::move(spake)));

  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::Bind(base::IgnoreResult(&SecurityManager::ClosePendingSession),
                 weak_ptr_factory_.GetWeakPtr(), session),
      base::TimeDelta::FromMinutes(kPairingExpirationTimeMinutes));

  *session_id = session;
  *device_commitment = Base64Encode(commitment);
  LOG(INFO) << "Pairing code for session " << *session_id << " is " << code;
  // TODO(vitalybuka): Handle case when device can't start multiple pairing
  // simultaneously and implement throttling to avoid brute force attack.
  if (!on_start_.is_null()) {
    on_start_.Run(session, mode,
                  std::vector<uint8_t>{code.begin(), code.end()});
  }

  return true;
}

bool SecurityManager::ConfirmPairing(const std::string& session_id,
                                     const std::string& client_commitment,
                                     std::string* fingerprint,
                                     std::string* signature,
                                     ErrorPtr* error) {
  auto session = pending_sessions_.find(session_id);
  if (session == pending_sessions_.end()) {
    Error::AddToPrintf(error, FROM_HERE, errors::kUnknownSession,
                       "Unknown session id: '%s'", session_id.c_str());
    return false;
  }

  std::vector<uint8_t> commitment;
  if (!Base64Decode(client_commitment, &commitment)) {
    ClosePendingSession(session_id);
    Error::AddToPrintf(error, FROM_HERE, errors::kInvalidFormat,
                       "Invalid commitment string: '%s'",
                       client_commitment.c_str());
    return false;
  }

  if (!session->second->ProcessMessage(
          std::string(commitment.begin(), commitment.end()), error)) {
    ClosePendingSession(session_id);
    return Error::AddTo(error, FROM_HERE, errors::kCommitmentMismatch,
                        "Pairing code or crypto implementation mismatch");
  }

  const std::string& key = session->second->GetKey();
  VLOG(3) << "KEY " << base::HexEncode(key.data(), key.size());

  const auto& certificate_fingerprint =
      auth_manager_->GetCertificateFingerprint();
  *fingerprint = Base64Encode(certificate_fingerprint);
  std::vector<uint8_t> cert_hmac = HmacSha256(
      std::vector<uint8_t>(key.begin(), key.end()), certificate_fingerprint);
  *signature = Base64Encode(cert_hmac);
  confirmed_sessions_.insert(
      std::make_pair(session->first, std::move(session->second)));
  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::Bind(base::IgnoreResult(&SecurityManager::CloseConfirmedSession),
                 weak_ptr_factory_.GetWeakPtr(), session_id),
      base::TimeDelta::FromMinutes(kSessionExpirationTimeMinutes));
  ClosePendingSession(session_id);
  return true;
}

bool SecurityManager::CancelPairing(const std::string& session_id,
                                    ErrorPtr* error) {
  bool confirmed = CloseConfirmedSession(session_id);
  bool pending = ClosePendingSession(session_id);
  if (pending) {
    CHECK_GE(pairing_attemts_, 1);
    --pairing_attemts_;
  }
  CHECK(!confirmed || !pending);
  if (confirmed || pending)
    return true;
  Error::AddToPrintf(error, FROM_HERE, errors::kUnknownSession,
                     "Unknown session id: '%s'", session_id.c_str());
  return false;
}

std::string SecurityManager::CreateSessionId() {
  return auth_manager_->CreateSessionId();
}

void SecurityManager::RegisterPairingListeners(
    const PairingStartListener& on_start,
    const PairingEndListener& on_end) {
  CHECK(on_start_.is_null() && on_end_.is_null());
  on_start_ = on_start;
  on_end_ = on_end;
}

bool SecurityManager::CheckIfPairingAllowed(ErrorPtr* error) {
  if (block_pairing_until_ > auth_manager_->Now()) {
    return Error::AddTo(error, FROM_HERE, errors::kDeviceBusy,
                        "Too many pairing attempts");
  }

  if (++pairing_attemts_ >= kMaxAllowedPairingAttemts) {
    LOG(INFO) << "Pairing blocked for" << kPairingBlockingTimeMinutes
              << "minutes.";
    block_pairing_until_ = auth_manager_->Now();
    block_pairing_until_ +=
        base::TimeDelta::FromMinutes(kPairingBlockingTimeMinutes);
  }

  return true;
}

bool SecurityManager::ClosePendingSession(const std::string& session_id) {
  // The most common source of these session_id values is the map containing
  // the sessions, which we're about to clear out.  Make a local copy.
  const std::string safe_session_id{session_id};
  const size_t num_erased = pending_sessions_.erase(safe_session_id);
  if (num_erased > 0 && !on_end_.is_null())
    on_end_.Run(safe_session_id);
  return num_erased != 0;
}

bool SecurityManager::CloseConfirmedSession(const std::string& session_id) {
  return confirmed_sessions_.erase(session_id) != 0;
}

bool SecurityManager::IsAnonymousAuthSupported() const {
  return GetSettings().local_anonymous_access_role != AuthScope::kNone;
}

bool SecurityManager::IsPairingAuthSupported() const {
  return GetSettings().local_pairing_enabled;
}

bool SecurityManager::IsLocalAuthSupported() const {
  return GetSettings().root_client_token_owner != RootClientTokenOwner::kNone;
}

}  // namespace privet
}  // namespace weave
