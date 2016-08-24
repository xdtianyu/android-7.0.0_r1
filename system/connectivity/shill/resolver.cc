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

#include "shill/resolver.h"

#include <algorithm>
#include <string>
#include <vector>

#include <base/files/file_util.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>

#include "shill/ipconfig.h"
#include "shill/logging.h"

using base::StringPrintf;
using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kResolver;
static string ObjectID(Resolver* r) { return "(resolver)"; }
}

namespace {
base::LazyInstance<Resolver> g_resolver = LAZY_INSTANCE_INITIALIZER;
}  // namespace

const char Resolver::kDefaultIgnoredSearchList[] = "gateway.2wire.net";

Resolver::Resolver() {}

Resolver::~Resolver() {}

Resolver* Resolver::GetInstance() {
  return g_resolver.Pointer();
}

bool Resolver::SetDNSFromLists(const std::vector<std::string>& dns_servers,
                               const std::vector<std::string>& domain_search) {
  SLOG(this, 2) << __func__;

  if (dns_servers.empty() && domain_search.empty()) {
    SLOG(this, 2) << "DNS list is empty";
    return ClearDNS();
  }

  vector<string> lines;
  vector<string>::const_iterator iter;
  for (iter = dns_servers.begin();
       iter != dns_servers.end(); ++iter) {
    lines.push_back("nameserver " + *iter);
  }

  vector<string> filtered_domain_search;
  for (iter = domain_search.begin();
       iter != domain_search.end(); ++iter) {
    if (std::find(ignored_search_list_.begin(),
                  ignored_search_list_.end(),
                  *iter) == ignored_search_list_.end()) {
      filtered_domain_search.push_back(*iter);
    }
  }

  if (!filtered_domain_search.empty()) {
    lines.push_back("search " + base::JoinString(filtered_domain_search, " "));
  }

  // - Send queries one-at-a-time, rather than parallelizing IPv4
  //   and IPv6 queries for a single host.
  // - Override the default 5-second request timeout and use a
  //   1-second timeout instead. (NOTE: Chrome's ADNS will use
  //   one second, regardless of what we put here.)
  // - Allow 5 attempts, rather than the default of 2.
  //   - For glibc, the worst case number of queries will be
  //        attempts * count(servers) * (count(search domains)+1)
  //   - For Chrome, the worst case number of queries will be
  //        attempts * count(servers) + 3 * glibc
  //   See crbug.com/224756 for supporting data.
  lines.push_back("options single-request timeout:1 attempts:5");

  // Newline at end of file
  lines.push_back("");

  string contents = base::JoinString(lines, "\n");

  SLOG(this, 2) << "Writing DNS out to " << path_.value();
  int count = base::WriteFile(path_, contents.c_str(), contents.size());

  return count == static_cast<int>(contents.size());
}

bool Resolver::ClearDNS() {
  SLOG(this, 2) << __func__;

  CHECK(!path_.empty());

  return base::DeleteFile(path_, false);
}

}  // namespace shill
