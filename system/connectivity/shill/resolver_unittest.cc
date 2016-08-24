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

#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <base/stl_util.h>
#include <base/strings/stringprintf.h>
#include <gtest/gtest.h>

#include "shill/mock_control.h"

using base::FilePath;
using std::string;
using std::vector;
using testing::Test;

namespace shill {

namespace {
const char kNameServer0[] = "8.8.8.8";
const char kNameServer1[] = "8.8.9.9";
const char kSearchDomain0[] = "chromium.org";
const char kSearchDomain1[] = "google.com";
const char kSearchDomain2[] = "crbug.com";
const char kExpectedOutput[] =
  "nameserver 8.8.8.8\n"
  "nameserver 8.8.9.9\n"
  "search chromium.org google.com\n"
  "options single-request timeout:1 attempts:5\n";
const char kExpectedIgnoredSearchOutput[] =
  "nameserver 8.8.8.8\n"
  "nameserver 8.8.9.9\n"
  "search google.com\n"
  "options single-request timeout:1 attempts:5\n";
}  // namespace

class ResolverTest : public Test {
 public:
  ResolverTest() : resolver_(Resolver::GetInstance()) {}

  virtual void SetUp() {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    path_ = temp_dir_.path().Append("resolver");
    resolver_->set_path(path_);
  }

  virtual void TearDown() {
    resolver_->set_path(FilePath(""));  // Don't try to save the store.
    ASSERT_TRUE(temp_dir_.Delete());
  }

 protected:
  string ReadFile();

  base::ScopedTempDir temp_dir_;
  Resolver* resolver_;
  FilePath path_;
};

string ResolverTest::ReadFile() {
  string data;
  EXPECT_TRUE(base::ReadFileToString(resolver_->path_, &data));
  return data;
}

TEST_F(ResolverTest, NonEmpty) {
  EXPECT_FALSE(base::PathExists(path_));
  EXPECT_TRUE(resolver_->ClearDNS());

  MockControl control;
  vector<string> dns_servers;
  vector<string> domain_search;
  dns_servers.push_back(kNameServer0);
  dns_servers.push_back(kNameServer1);
  domain_search.push_back(kSearchDomain0);
  domain_search.push_back(kSearchDomain1);

  EXPECT_TRUE(resolver_->SetDNSFromLists(dns_servers, domain_search));
  EXPECT_TRUE(base::PathExists(path_));
  EXPECT_EQ(kExpectedOutput, ReadFile());

  EXPECT_TRUE(resolver_->ClearDNS());
}

TEST_F(ResolverTest, Empty) {
  EXPECT_FALSE(base::PathExists(path_));

  MockControl control;
  vector<string> dns_servers;
  vector<string> domain_search;

  EXPECT_TRUE(resolver_->SetDNSFromLists(dns_servers, domain_search));
  EXPECT_FALSE(base::PathExists(path_));
}

TEST_F(ResolverTest, IgnoredSearchList) {
  EXPECT_FALSE(base::PathExists(path_));
  EXPECT_TRUE(resolver_->ClearDNS());

  MockControl control;
  vector<string> dns_servers;
  vector<string> domain_search;
  dns_servers.push_back(kNameServer0);
  dns_servers.push_back(kNameServer1);
  domain_search.push_back(kSearchDomain0);
  domain_search.push_back(kSearchDomain1);
  vector<string> ignored_search;
  ignored_search.push_back(kSearchDomain0);
  ignored_search.push_back(kSearchDomain2);
  resolver_->set_ignored_search_list(ignored_search);
  EXPECT_TRUE(resolver_->SetDNSFromLists(dns_servers, domain_search));
  EXPECT_TRUE(base::PathExists(path_));
  EXPECT_EQ(kExpectedIgnoredSearchOutput, ReadFile());

  EXPECT_TRUE(resolver_->ClearDNS());
}

}  // namespace shill
