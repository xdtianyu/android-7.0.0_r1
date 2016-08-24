//
// Copyright (C) 2012 The Android Open Source Project
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

#ifndef SHILL_SCOPE_LOGGER_H_
#define SHILL_SCOPE_LOGGER_H_

#include <bitset>
#include <string>
#include <vector>

#include <base/lazy_instance.h>
#include <base/macros.h>
#include <gtest/gtest_prod.h>

#include "shill/callbacks.h"

namespace shill {

// A class that enables logging based on scope and verbose level. It is not
// intended to be used directly but via the SLOG() macros in shill/logging.h
class ScopeLogger {
 public:
  // Logging scopes.
  //
  // Update kScopeNames in scope_logger.cc after changing this enumerated type.
  // These scope identifiers are sorted by their scope names alphabetically.
  enum Scope {
    kBinder = 0,
    kCellular,
    kConnection,
    kCrypto,
    kDaemon,
    kDBus,
    kDevice,
    kDHCP,
    kDNS,
    kEthernet,
    kHTTP,
    kHTTPProxy,
    kInet,
    kLink,
    kManager,
    kMetrics,
    kModem,
    kPortal,
    kPower,
    kPPP,
    kPPPoE,
    kProfile,
    kProperty,
    kResolver,
    kRoute,
    kRTNL,
    kService,
    kStorage,
    kTask,
    kVPN,
    kWiFi,
    kWiMax,
    kNumScopes
  };

  typedef base::Callback<void(bool)> ScopeEnableChangedCallback;
  typedef std::vector<ScopeEnableChangedCallback>ScopeEnableChangedCallbacks;

  // Returns a singleton of this class.
  static ScopeLogger* GetInstance();

  ScopeLogger();
  ~ScopeLogger();

  // Returns true if logging is enabled for |scope| and |verbose_level|, i.e.
  // scope_enable_[|scope|] is true and |verbose_level| <= |verbose_level_|
  bool IsLogEnabled(Scope scope, int verbose_level) const;

  // Returns true if logging is enabled for |scope| at any verbosity level.
  bool IsScopeEnabled(Scope scope) const;

  // Returns a string comprising the names, separated by commas, of all scopes.
  std::string GetAllScopeNames() const;

  // Returns a string comprising the names, separated by plus signs, of all
  // scopes that are enabled for logging.
  std::string GetEnabledScopeNames() const;

  // Enables/disables scopes as specified by |expression|.
  //
  // |expression| is a string comprising a sequence of scope names, each
  // prefixed by a plus '+' or minus '-' sign. A scope prefixed by a plus
  // sign is enabled for logging, whereas a scope prefixed by a minus sign
  // is disabled for logging. Scopes that are not mentioned in |expression|
  // remain the same state.
  //
  // To allow resetting the state of all scopes, an exception is made for the
  // first scope name in the sequence, which may not be prefixed by any sign.
  // That is considered as an implicit plus sign for that scope and also
  // indicates that all scopes are first disabled before enabled by
  // |expression|.
  //
  // If |expression| is an empty string, all scopes are disabled. Any unknown
  // scope name found in |expression| is ignored.
  void EnableScopesByName(const std::string& expression);

  // Register for log scope enable/disable state changes for |scope|.
  void RegisterScopeEnableChangedCallback(
      Scope scope, ScopeEnableChangedCallback callback);

  // Sets the verbose level for all scopes to |verbose_level|.
  void set_verbose_level(int verbose_level) { verbose_level_ = verbose_level; }

 private:
  // Required for constructing LazyInstance<ScopeLogger>.
  friend struct base::DefaultLazyInstanceTraits<ScopeLogger>;
  friend class ScopeLoggerTest;
  FRIEND_TEST(ScopeLoggerTest, GetEnabledScopeNames);
  FRIEND_TEST(ScopeLoggerTest, SetScopeEnabled);
  FRIEND_TEST(ScopeLoggerTest, SetVerboseLevel);

  // Disables logging for all scopes.
  void DisableAllScopes();

  // Enables or disables logging for |scope|.
  void SetScopeEnabled(Scope scope, bool enabled);

  // Boolean values to indicate whether logging is enabled for each scope.
  std::bitset<kNumScopes> scope_enabled_;

  // Verbose level that is applied to all scopes.
  int verbose_level_;

  // Hooks to notify interested parties of changes to log scopes.
  ScopeEnableChangedCallbacks log_scope_callbacks_[kNumScopes];

  DISALLOW_COPY_AND_ASSIGN(ScopeLogger);
};

}  // namespace shill

#endif  // SHILL_SCOPE_LOGGER_H_
