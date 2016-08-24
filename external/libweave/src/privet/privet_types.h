// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_PRIVET_PRIVET_TYPES_H_
#define LIBWEAVE_SRC_PRIVET_PRIVET_TYPES_H_

#include <string>

#include <base/logging.h>
#include <weave/error.h>
#include <weave/settings.h>

namespace weave {
namespace privet {

enum class CryptoType {
  kSpake_p224,
};

enum class AuthType {
  kAnonymous,
  kPairing,
  kLocal,
};

enum class WifiType {
  kWifi24,
  kWifi50,
};

struct UserAppId {
  UserAppId() = default;

  UserAppId(AuthType auth_type,
            const std::vector<uint8_t>& user_id,
            const std::vector<uint8_t>& app_id)
      : type{auth_type},
        user{user_id},
        app{user_id.empty() ? user_id : app_id} {}

  bool IsEmpty() const { return user.empty(); }

  AuthType type{};
  std::vector<uint8_t> user;
  std::vector<uint8_t> app;
};

inline bool operator==(const UserAppId& l, const UserAppId& r) {
  return l.user == r.user && l.app == r.app;
}

inline bool operator!=(const UserAppId& l, const UserAppId& r) {
  return l.user != r.user || l.app != r.app;
}

class UserInfo {
 public:
  explicit UserInfo(AuthScope scope = AuthScope::kNone,
                    const UserAppId& id = {})
      : scope_{scope}, id_{scope == AuthScope::kNone ? UserAppId{} : id} {}
  AuthScope scope() const { return scope_; }
  const UserAppId& id() const { return id_; }

 private:
  AuthScope scope_;
  UserAppId id_;
};

class ConnectionState final {
 public:
  enum Status {
    kDisabled,
    kUnconfigured,
    kConnecting,
    kOnline,
    kOffline,
  };

  explicit ConnectionState(Status status) : status_(status) {}
  explicit ConnectionState(ErrorPtr error)
      : status_(kOffline), error_(std::move(error)) {}

  Status status() const {
    CHECK(!error_);
    return status_;
  }

  bool IsStatusEqual(Status status) const {
    if (error_)
      return false;
    return status_ == status;
  }

  const Error* error() const { return error_.get(); }

 private:
  Status status_;
  ErrorPtr error_;
};

class SetupState final {
 public:
  enum Status {
    kNone,
    kInProgress,
    kSuccess,
  };

  explicit SetupState(Status status) : status_(status) {}
  explicit SetupState(ErrorPtr error)
      : status_(kNone), error_(std::move(error)) {}

  Status status() const {
    CHECK(!error_);
    return status_;
  }

  bool IsStatusEqual(Status status) const {
    if (error_)
      return false;
    return status_ == status;
  }

  const Error* error() const { return error_.get(); }

 private:
  Status status_;
  ErrorPtr error_;
};

}  // namespace privet
}  // namespace weave

#endif  // LIBWEAVE_SRC_PRIVET_PRIVET_TYPES_H_
