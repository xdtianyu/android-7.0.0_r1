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

#include "shill/scope_logger.h"

#include <vector>

#include <base/strings/string_tokenizer.h>
#include <base/strings/string_util.h>

using base::StringTokenizer;
using std::string;
using std::vector;

namespace shill {

namespace {

const int kDefaultVerboseLevel = 0;

// Scope names corresponding to the scope defined by ScopeLogger::Scope.
const char* const kScopeNames[] = {
  "binder",
  "cellular",
  "connection",
  "crypto",
  "daemon",
  "dbus",
  "device",
  "dhcp",
  "dns",
  "ethernet",
  "http",
  "httpproxy",
  "inet",
  "link",
  "manager",
  "metrics",
  "modem",
  "portal",
  "power",
  "ppp",
  "pppoe",
  "profile",
  "property",
  "resolver",
  "route",
  "rtnl",
  "service",
  "storage",
  "task",
  "vpn",
  "wifi",
  "wimax",
};

static_assert(arraysize(kScopeNames) == ScopeLogger::kNumScopes,
              "Scope tags do not have expected number of strings");

// ScopeLogger needs to be a 'leaky' singleton as it needs to survive to
// handle logging till the very end of the shill process. Making ScopeLogger
// leaky is fine as it does not need to clean up or release any resource at
// destruction.
base::LazyInstance<ScopeLogger>::Leaky g_scope_logger =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

// static
ScopeLogger* ScopeLogger::GetInstance() {
  return g_scope_logger.Pointer();
}

ScopeLogger::ScopeLogger()
    : verbose_level_(kDefaultVerboseLevel) {
}

ScopeLogger::~ScopeLogger() {
}

bool ScopeLogger::IsLogEnabled(Scope scope, int verbose_level) const {
  return IsScopeEnabled(scope) && verbose_level <= verbose_level_;
}

bool ScopeLogger::IsScopeEnabled(Scope scope) const {
  CHECK_GE(scope, 0);
  CHECK_LT(scope, kNumScopes);

  return scope_enabled_[scope];
}

string ScopeLogger::GetAllScopeNames() const {
  vector<string> names(kScopeNames, kScopeNames + arraysize(kScopeNames));
  return base::JoinString(names, "+");
}

string ScopeLogger::GetEnabledScopeNames() const {
  vector<string> names;
  for (size_t i = 0; i < arraysize(kScopeNames); ++i) {
    if (scope_enabled_[i])
      names.push_back(kScopeNames[i]);
  }
  return base::JoinString(names, "+");
}

void ScopeLogger::EnableScopesByName(const string& expression) {
  if (expression.empty()) {
    DisableAllScopes();
    return;
  }

  // As described in the header file, if the first scope name in the
  // sequence specified by |expression| is not prefixed by a plus or
  // minus sign, it indicates that all scopes are first disabled before
  // enabled by |expression|.
  if (expression[0] != '+' && expression[0] != '-')
    DisableAllScopes();

  bool enable_scope = true;
  StringTokenizer tokenizer(expression, "+-");
  tokenizer.set_options(StringTokenizer::RETURN_DELIMS);
  while (tokenizer.GetNext()) {
    if (tokenizer.token_is_delim()) {
      enable_scope = (tokenizer.token() == "+");
      continue;
    }

    if (tokenizer.token().empty())
      continue;

    size_t i;
    for (i = 0; i < arraysize(kScopeNames); ++i) {
      if (tokenizer.token() == kScopeNames[i]) {
        SetScopeEnabled(static_cast<Scope>(i), enable_scope);
        break;
      }
    }
    LOG_IF(WARNING, i == arraysize(kScopeNames))
        << "Unknown scope '" << tokenizer.token() << "'";
  }
}

void ScopeLogger::RegisterScopeEnableChangedCallback(
    Scope scope, ScopeEnableChangedCallback callback) {
  CHECK_GE(scope, 0);
  CHECK_LT(scope, kNumScopes);
  log_scope_callbacks_[scope].push_back(callback);
}

void ScopeLogger::DisableAllScopes() {
  // Iterate over all scopes so the notification side-effect occurs.
  for (size_t i = 0; i < arraysize(kScopeNames); ++i) {
    SetScopeEnabled(static_cast<Scope>(i), false);
  }
}

void ScopeLogger::SetScopeEnabled(Scope scope, bool enabled) {
  CHECK_GE(scope, 0);
  CHECK_LT(scope, kNumScopes);

  if (scope_enabled_[scope] != enabled) {
    for (const auto& callback : log_scope_callbacks_[scope]) {
      callback.Run(enabled);
    }
  }

  scope_enabled_[scope] = enabled;
}

}  // namespace shill
