// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/privet/auth_manager.h"

#include <algorithm>

#include <base/guid.h>
#include <base/rand_util.h>
#include <base/strings/string_number_conversions.h>

#include "src/config.h"
#include "src/data_encoding.h"
#include "src/privet/constants.h"
#include "src/privet/openssl_utils.h"
#include "src/string_utils.h"

extern "C" {
#include "third_party/libuweave/src/macaroon.h"
#include "third_party/libuweave/src/macaroon_caveat_internal.h"
}

namespace weave {
namespace privet {

namespace {

const time_t kJ2000ToTimeT = 946684800;
const size_t kMaxMacaroonSize = 1024;
const size_t kMaxPendingClaims = 10;
const char kInvalidTokenError[] = "invalid_token";
const int kSessionIdTtlMinutes = 1;

uint32_t ToJ2000Time(const base::Time& time) {
  return std::max(time.ToTimeT(), kJ2000ToTimeT) - kJ2000ToTimeT;
}

base::Time FromJ2000Time(uint32_t time) {
  return base::Time::FromTimeT(time + kJ2000ToTimeT);
}

template <class T>
void AppendToArray(T value, std::vector<uint8_t>* array) {
  auto begin = reinterpret_cast<const uint8_t*>(&value);
  array->insert(array->end(), begin, begin + sizeof(value));
}

class Caveat {
 public:
  Caveat(UwMacaroonCaveatType type, size_t str_len)
      : buffer_(uw_macaroon_caveat_creation_get_buffsize_(type, str_len)) {
    CHECK(!buffer_.empty());
  }
  const UwMacaroonCaveat& GetCaveat() const { return caveat_; }

 protected:
  UwMacaroonCaveat caveat_{};
  std::vector<uint8_t> buffer_;

  DISALLOW_COPY_AND_ASSIGN(Caveat);
};

class ScopeCaveat : public Caveat {
 public:
  explicit ScopeCaveat(UwMacaroonCaveatScopeType scope)
      : Caveat(kUwMacaroonCaveatTypeScope, 0) {
    CHECK(uw_macaroon_caveat_create_scope_(scope, buffer_.data(),
                                           buffer_.size(), &caveat_));
  }

  DISALLOW_COPY_AND_ASSIGN(ScopeCaveat);
};

class TimestampCaveat : public Caveat {
 public:
  explicit TimestampCaveat(const base::Time& timestamp)
      : Caveat(kUwMacaroonCaveatTypeDelegationTimestamp, 0) {
    CHECK(uw_macaroon_caveat_create_delegation_timestamp_(
        ToJ2000Time(timestamp), buffer_.data(), buffer_.size(), &caveat_));
  }

  DISALLOW_COPY_AND_ASSIGN(TimestampCaveat);
};

class ExpirationCaveat : public Caveat {
 public:
  explicit ExpirationCaveat(const base::Time& timestamp)
      : Caveat(kUwMacaroonCaveatTypeExpirationAbsolute, 0) {
    CHECK(uw_macaroon_caveat_create_expiration_absolute_(
        ToJ2000Time(timestamp), buffer_.data(), buffer_.size(), &caveat_));
  }

  DISALLOW_COPY_AND_ASSIGN(ExpirationCaveat);
};

class UserIdCaveat : public Caveat {
 public:
  explicit UserIdCaveat(const std::vector<uint8_t>& id)
      : Caveat(kUwMacaroonCaveatTypeDelegateeUser, id.size()) {
    CHECK(uw_macaroon_caveat_create_delegatee_user_(
        id.data(), id.size(), buffer_.data(), buffer_.size(), &caveat_));
  }

  DISALLOW_COPY_AND_ASSIGN(UserIdCaveat);
};

class AppIdCaveat : public Caveat {
 public:
  explicit AppIdCaveat(const std::vector<uint8_t>& id)
      : Caveat(kUwMacaroonCaveatTypeDelegateeApp, id.size()) {
    CHECK(uw_macaroon_caveat_create_delegatee_app_(
        id.data(), id.size(), buffer_.data(), buffer_.size(), &caveat_));
  }

  DISALLOW_COPY_AND_ASSIGN(AppIdCaveat);
};

class ServiceCaveat : public Caveat {
 public:
  explicit ServiceCaveat(const std::string& id)
      : Caveat(kUwMacaroonCaveatTypeDelegateeService, id.size()) {
    CHECK(uw_macaroon_caveat_create_delegatee_service_(
        reinterpret_cast<const uint8_t*>(id.data()), id.size(), buffer_.data(),
        buffer_.size(), &caveat_));
  }

  DISALLOW_COPY_AND_ASSIGN(ServiceCaveat);
};

class SessionIdCaveat : public Caveat {
 public:
  explicit SessionIdCaveat(const std::string& id)
      : Caveat(kUwMacaroonCaveatTypeLanSessionID, id.size()) {
    CHECK(uw_macaroon_caveat_create_lan_session_id_(
        reinterpret_cast<const uint8_t*>(id.data()), id.size(), buffer_.data(),
        buffer_.size(), &caveat_));
  }

  DISALLOW_COPY_AND_ASSIGN(SessionIdCaveat);
};

class ClientAuthTokenCaveat : public Caveat {
 public:
  ClientAuthTokenCaveat()
      : Caveat(kUwMacaroonCaveatTypeClientAuthorizationTokenV1, 0) {
    CHECK(uw_macaroon_caveat_create_client_authorization_token_(
        nullptr, 0, buffer_.data(), buffer_.size(), &caveat_));
  }

  DISALLOW_COPY_AND_ASSIGN(ClientAuthTokenCaveat);
};

std::vector<uint8_t> CreateSecret() {
  std::vector<uint8_t> secret(kSha256OutputSize);
  base::RandBytes(secret.data(), secret.size());
  return secret;
}

bool IsClaimAllowed(RootClientTokenOwner curret, RootClientTokenOwner claimer) {
  return claimer > curret || claimer == RootClientTokenOwner::kCloud;
}

std::vector<uint8_t> CreateMacaroonToken(
    const std::vector<uint8_t>& secret,
    const base::Time& time,
    const std::vector<const UwMacaroonCaveat*>& caveats) {
  CHECK_EQ(kSha256OutputSize, secret.size());

  UwMacaroonContext context{};
  CHECK(uw_macaroon_context_create_(ToJ2000Time(time), nullptr, 0, &context));

  UwMacaroon macaroon{};
  CHECK(uw_macaroon_create_from_root_key_(&macaroon, secret.data(),
                                          secret.size(), &context,
                                          caveats.data(), caveats.size()));

  std::vector<uint8_t> serialized_token(kMaxMacaroonSize);
  size_t len = 0;
  CHECK(uw_macaroon_serialize_(&macaroon, serialized_token.data(),
                               serialized_token.size(), &len));
  serialized_token.resize(len);

  return serialized_token;
}

std::vector<uint8_t> ExtendMacaroonToken(
    const UwMacaroon& macaroon,
    const base::Time& time,
    const std::vector<const UwMacaroonCaveat*>& caveats) {
  UwMacaroonContext context{};
  CHECK(uw_macaroon_context_create_(ToJ2000Time(time), nullptr, 0, &context));

  UwMacaroon prev_macaroon = macaroon;
  std::vector<uint8_t> prev_buffer(kMaxMacaroonSize);
  std::vector<uint8_t> new_buffer(kMaxMacaroonSize);

  for (auto caveat : caveats) {
    UwMacaroon new_macaroon{};
    CHECK(uw_macaroon_extend_(&prev_macaroon, &new_macaroon, &context, caveat,
                              new_buffer.data(), new_buffer.size()));
    new_buffer.swap(prev_buffer);
    prev_macaroon = new_macaroon;
  }

  std::vector<uint8_t> serialized_token(kMaxMacaroonSize);
  size_t len = 0;
  CHECK(uw_macaroon_serialize_(&prev_macaroon, serialized_token.data(),
                               serialized_token.size(), &len));
  serialized_token.resize(len);

  return serialized_token;
}

bool LoadMacaroon(const std::vector<uint8_t>& token,
                  std::vector<uint8_t>* buffer,
                  UwMacaroon* macaroon,
                  ErrorPtr* error) {
  buffer->resize(kMaxMacaroonSize);
  if (!uw_macaroon_deserialize_(token.data(), token.size(), buffer->data(),
                                buffer->size(), macaroon)) {
    return Error::AddTo(error, FROM_HERE, kInvalidTokenError,
                        "Invalid token format");
  }
  return true;
}

bool VerifyMacaroon(const std::vector<uint8_t>& secret,
                    const UwMacaroon& macaroon,
                    const base::Time& time,
                    UwMacaroonValidationResult* result,
                    ErrorPtr* error) {
  CHECK_EQ(kSha256OutputSize, secret.size());
  UwMacaroonContext context = {};
  CHECK(uw_macaroon_context_create_(ToJ2000Time(time), nullptr, 0, &context));

  if (!uw_macaroon_validate_(&macaroon, secret.data(), secret.size(), &context,
                             result)) {
    return Error::AddTo(error, FROM_HERE, "invalid_token",
                        "Invalid token signature");
  }
  return true;
}

UwMacaroonCaveatScopeType ToMacaroonScope(AuthScope scope) {
  switch (scope) {
    case AuthScope::kViewer:
      return kUwMacaroonCaveatScopeTypeViewer;
    case AuthScope::kUser:
      return kUwMacaroonCaveatScopeTypeUser;
    case AuthScope::kManager:
      return kUwMacaroonCaveatScopeTypeManager;
    case AuthScope::kOwner:
      return kUwMacaroonCaveatScopeTypeOwner;
    default:
      NOTREACHED() << EnumToString(scope);
  }
  return kUwMacaroonCaveatScopeTypeViewer;
}

AuthScope FromMacaroonScope(uint32_t scope) {
  if (scope <= kUwMacaroonCaveatScopeTypeOwner)
    return AuthScope::kOwner;
  if (scope <= kUwMacaroonCaveatScopeTypeManager)
    return AuthScope::kManager;
  if (scope <= kUwMacaroonCaveatScopeTypeUser)
    return AuthScope::kUser;
  if (scope <= kUwMacaroonCaveatScopeTypeViewer)
    return AuthScope::kViewer;
  return AuthScope::kNone;
}

}  // namespace

AuthManager::AuthManager(Config* config,
                         const std::vector<uint8_t>& certificate_fingerprint)
    : config_{config},
      certificate_fingerprint_{certificate_fingerprint},
      access_secret_{CreateSecret()} {
  if (config_) {
    SetAuthSecret(config_->GetSettings().secret,
                  config_->GetSettings().root_client_token_owner);
  } else {
    SetAuthSecret({}, RootClientTokenOwner::kNone);
  }
}

AuthManager::AuthManager(const std::vector<uint8_t>& auth_secret,
                         const std::vector<uint8_t>& certificate_fingerprint,
                         const std::vector<uint8_t>& access_secret,
                         base::Clock* clock)
    : AuthManager(nullptr, certificate_fingerprint) {
  access_secret_ = access_secret.size() == kSha256OutputSize ? access_secret
                                                             : CreateSecret();
  SetAuthSecret(auth_secret, RootClientTokenOwner::kNone);
  if (clock)
    clock_ = clock;
}

void AuthManager::SetAuthSecret(const std::vector<uint8_t>& secret,
                                RootClientTokenOwner owner) {
  auth_secret_ = secret;

  if (auth_secret_.size() != kSha256OutputSize) {
    auth_secret_ = CreateSecret();
    owner = RootClientTokenOwner::kNone;
  }

  if (!config_ || (config_->GetSettings().secret == auth_secret_ &&
                   config_->GetSettings().root_client_token_owner == owner)) {
    return;
  }

  Config::Transaction change{config_};
  change.set_secret(secret);
  change.set_root_client_token_owner(owner);
  change.Commit();
}

AuthManager::~AuthManager() {}

std::vector<uint8_t> AuthManager::CreateAccessToken(const UserInfo& user_info,
                                                    base::TimeDelta ttl) const {
  const base::Time now = Now();
  TimestampCaveat issued{now};
  ScopeCaveat scope{ToMacaroonScope(user_info.scope())};
  // Macaroons have no caveats for auth type. So we just append the type to the
  // user ID.
  std::vector<uint8_t> id_with_type{user_info.id().user};
  id_with_type.push_back(static_cast<uint8_t>(user_info.id().type));
  UserIdCaveat user{id_with_type};
  AppIdCaveat app{user_info.id().app};
  ExpirationCaveat expiration{now + ttl};
  return CreateMacaroonToken(
      access_secret_, now,
      {

          &issued.GetCaveat(), &scope.GetCaveat(), &user.GetCaveat(),
          &app.GetCaveat(), &expiration.GetCaveat(),
      });
}

bool AuthManager::ParseAccessToken(const std::vector<uint8_t>& token,
                                   UserInfo* user_info,
                                   ErrorPtr* error) const {
  std::vector<uint8_t> buffer;
  UwMacaroon macaroon{};

  UwMacaroonValidationResult result{};
  const base::Time now = Now();
  if (!LoadMacaroon(token, &buffer, &macaroon, error) ||
      macaroon.num_caveats != 5 ||
      !VerifyMacaroon(access_secret_, macaroon, now, &result, error)) {
    return Error::AddTo(error, FROM_HERE, errors::kInvalidAuthorization,
                        "Invalid token");
  }

  AuthScope auth_scope{FromMacaroonScope(result.granted_scope)};
  if (auth_scope == AuthScope::kNone) {
    return Error::AddTo(error, FROM_HERE, errors::kInvalidAuthorization,
                        "Invalid token data");
  }

  // If token is valid and token was not extended, it should has precisely this
  // values.
  CHECK_GE(FromJ2000Time(result.expiration_time), now);
  CHECK_EQ(2u, result.num_delegatees);
  CHECK_EQ(kUwMacaroonDelegateeTypeUser, result.delegatees[0].type);
  CHECK_EQ(kUwMacaroonDelegateeTypeApp, result.delegatees[1].type);
  CHECK_GT(result.delegatees[0].id_len, 1u);
  std::vector<uint8_t> user_id{
      result.delegatees[0].id,
      result.delegatees[0].id + result.delegatees[0].id_len};
  // Last byte is used for type. See |CreateAccessToken|.
  AuthType type = static_cast<AuthType>(user_id.back());
  user_id.pop_back();

  std::vector<uint8_t> app_id{
      result.delegatees[1].id,
      result.delegatees[1].id + result.delegatees[1].id_len};
  if (user_info)
    *user_info = UserInfo{auth_scope, UserAppId{type, user_id, app_id}};

  return true;
}

std::vector<uint8_t> AuthManager::ClaimRootClientAuthToken(
    RootClientTokenOwner owner,
    ErrorPtr* error) {
  CHECK(RootClientTokenOwner::kNone != owner);
  if (config_) {
    auto current = config_->GetSettings().root_client_token_owner;
    if (!IsClaimAllowed(current, owner)) {
      Error::AddToPrintf(error, FROM_HERE, errors::kAlreadyClaimed,
                         "Device already claimed by '%s'",
                         EnumToString(current).c_str());
      return {};
    }
  };

  pending_claims_.push_back(std::make_pair(
      std::unique_ptr<AuthManager>{new AuthManager{nullptr, {}}}, owner));
  if (pending_claims_.size() > kMaxPendingClaims)
    pending_claims_.pop_front();
  return pending_claims_.back().first->GetRootClientAuthToken(owner);
}

bool AuthManager::ConfirmClientAuthToken(const std::vector<uint8_t>& token,
                                         ErrorPtr* error) {
  // Cover case when caller sent confirm twice.
  if (pending_claims_.empty())
    return IsValidAuthToken(token, error);

  auto claim =
      std::find_if(pending_claims_.begin(), pending_claims_.end(),
                   [&token](const decltype(pending_claims_)::value_type& auth) {
                     return auth.first->IsValidAuthToken(token, nullptr);
                   });
  if (claim == pending_claims_.end()) {
    return Error::AddTo(error, FROM_HERE, errors::kNotFound, "Unknown claim");
  }

  SetAuthSecret(claim->first->GetAuthSecret(), claim->second);
  pending_claims_.clear();
  return true;
}

std::vector<uint8_t> AuthManager::GetRootClientAuthToken(
    RootClientTokenOwner owner) const {
  CHECK(RootClientTokenOwner::kNone != owner);
  ClientAuthTokenCaveat auth_token;
  const base::Time now = Now();
  TimestampCaveat issued{now};

  ServiceCaveat client{owner == RootClientTokenOwner::kCloud ? "google.com"
                                                             : ""};
  return CreateMacaroonToken(
      auth_secret_, now,
      {
          &auth_token.GetCaveat(), &issued.GetCaveat(), &client.GetCaveat(),
      });
}

base::Time AuthManager::Now() const {
  return clock_->Now();
}

bool AuthManager::IsValidAuthToken(const std::vector<uint8_t>& token,
                                   ErrorPtr* error) const {
  std::vector<uint8_t> buffer;
  UwMacaroon macaroon{};
  UwMacaroonValidationResult result{};
  if (!LoadMacaroon(token, &buffer, &macaroon, error) ||
      !VerifyMacaroon(auth_secret_, macaroon, Now(), &result, error)) {
    return Error::AddTo(error, FROM_HERE, errors::kInvalidAuthCode,
                        "Invalid token");
  }
  return true;
}

bool AuthManager::CreateAccessTokenFromAuth(
    const std::vector<uint8_t>& auth_token,
    base::TimeDelta ttl,
    std::vector<uint8_t>* access_token,
    AuthScope* access_token_scope,
    base::TimeDelta* access_token_ttl,
    ErrorPtr* error) const {
  std::vector<uint8_t> buffer;
  UwMacaroon macaroon{};
  UwMacaroonValidationResult result{};
  const base::Time now = Now();
  if (!LoadMacaroon(auth_token, &buffer, &macaroon, error) ||
      !VerifyMacaroon(auth_secret_, macaroon, now, &result, error)) {
    return Error::AddTo(error, FROM_HERE, errors::kInvalidAuthCode,
                        "Invalid token");
  }

  AuthScope auth_scope{FromMacaroonScope(result.granted_scope)};
  if (auth_scope == AuthScope::kNone) {
    return Error::AddTo(error, FROM_HERE, errors::kInvalidAuthCode,
                        "Invalid token data");
  }

  // TODO: Integrate black list checks.
  auto delegates_rbegin = std::reverse_iterator<const UwMacaroonDelegateeInfo*>(
      result.delegatees + result.num_delegatees);
  auto delegates_rend =
      std::reverse_iterator<const UwMacaroonDelegateeInfo*>(result.delegatees);
  auto last_user_id =
      std::find_if(delegates_rbegin, delegates_rend,
                   [](const UwMacaroonDelegateeInfo& delegatee) {
                     return delegatee.type == kUwMacaroonDelegateeTypeUser;
                   });
  auto last_app_id =
      std::find_if(delegates_rbegin, delegates_rend,
                   [](const UwMacaroonDelegateeInfo& delegatee) {
                     return delegatee.type == kUwMacaroonDelegateeTypeApp;
                   });

  if (last_user_id == delegates_rend || !last_user_id->id_len) {
    return Error::AddTo(error, FROM_HERE, errors::kInvalidAuthCode,
                        "User ID is missing");
  }

  const char* session_id = reinterpret_cast<const char*>(result.lan_session_id);
  if (!IsValidSessionId({session_id, session_id + result.lan_session_id_len})) {
    return Error::AddTo(error, FROM_HERE, errors::kInvalidAuthCode,
                        "Invalid session id");
  }

  CHECK_GE(FromJ2000Time(result.expiration_time), now);

  if (!access_token)
    return true;

  std::vector<uint8_t> user_id{last_user_id->id,
                               last_user_id->id + last_user_id->id_len};
  std::vector<uint8_t> app_id;
  if (last_app_id != delegates_rend)
    app_id.assign(last_app_id->id, last_app_id->id + last_app_id->id_len);

  UserInfo info{auth_scope, {AuthType::kLocal, user_id, app_id}};

  ttl = std::min(ttl, FromJ2000Time(result.expiration_time) - now);
  *access_token = CreateAccessToken(info, ttl);

  if (access_token_scope)
    *access_token_scope = info.scope();

  if (access_token_ttl)
    *access_token_ttl = ttl;
  return true;
}

std::string AuthManager::CreateSessionId() const {
  return std::to_string(ToJ2000Time(Now())) + ":" +
         std::to_string(++session_counter_);
}

bool AuthManager::IsValidSessionId(const std::string& session_id) const {
  base::Time ssid_time = FromJ2000Time(std::atoi(session_id.c_str()));
  return Now() - base::TimeDelta::FromMinutes(kSessionIdTtlMinutes) <=
             ssid_time &&
         ssid_time <= Now();
}

std::vector<uint8_t> AuthManager::DelegateToUser(
    const std::vector<uint8_t>& token,
    base::TimeDelta ttl,
    const UserInfo& user_info) const {
  std::vector<uint8_t> buffer;
  UwMacaroon macaroon{};
  CHECK(LoadMacaroon(token, &buffer, &macaroon, nullptr));

  const base::Time now = Now();
  TimestampCaveat issued{now};
  ExpirationCaveat expiration{now + ttl};
  ScopeCaveat scope{ToMacaroonScope(user_info.scope())};
  UserIdCaveat user{user_info.id().user};
  AppIdCaveat app{user_info.id().app};
  SessionIdCaveat session{CreateSessionId()};

  std::vector<const UwMacaroonCaveat*> caveats{
      &issued.GetCaveat(), &expiration.GetCaveat(), &scope.GetCaveat(),
      &user.GetCaveat(),
  };

  if (!user_info.id().app.empty())
    caveats.push_back(&app.GetCaveat());

  caveats.push_back(&session.GetCaveat());

  return ExtendMacaroonToken(macaroon, now, caveats);
}

}  // namespace privet
}  // namespace weave
