//
// Copyright (C) 2011 The Android Open Source Project
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

#ifndef SHILL_RESOLVER_H_
#define SHILL_RESOLVER_H_

#include <string>
#include <vector>

#include <base/files/file_path.h>
#include <base/lazy_instance.h>
#include <base/memory/ref_counted.h>

#include "shill/refptr_types.h"

namespace shill {

// This provides a static function for dumping the DNS information out
// of an ipconfig into a "resolv.conf" formatted file.
class Resolver {
 public:
  // The default comma-separated list of search-list prefixes that
  // should be ignored when writing out a DNS configuration.  These
  // are usually preconfigured by a DHCP server and are not of real
  // value to the user.  This will release DNS bandwidth for searches
  // we expect will have a better chance of getting what the user is
  // looking for.
  static const char kDefaultIgnoredSearchList[];

  virtual ~Resolver();

  // Since this is a singleton, use Resolver::GetInstance()->Foo().
  static Resolver* GetInstance();

  virtual void set_path(const base::FilePath& path) { path_ = path; }

  // Install domain name service parameters, given a list of
  // DNS servers in |dns_servers|, and a list of DNS search suffixes in
  // |domain_search|.
  virtual bool SetDNSFromLists(const std::vector<std::string>& dns_servers,
                               const std::vector<std::string>& domain_search);

  // Remove any created domain name service file.
  virtual bool ClearDNS();

  // Sets the list of ignored DNS search suffixes.  This list will be used
  // to filter the domain_search parameter of later SetDNSFromLists() calls.
  virtual void set_ignored_search_list(
      const std::vector<std::string>& ignored_list) {
    ignored_search_list_ = ignored_list;
  }

 protected:
  Resolver();

 private:
  friend struct base::DefaultLazyInstanceTraits<Resolver>;
  friend class ResolverTest;

  base::FilePath path_;
  std::vector<std::string> ignored_search_list_;

  DISALLOW_COPY_AND_ASSIGN(Resolver);
};

}  // namespace shill

#endif  // SHILL_RESOLVER_H_
