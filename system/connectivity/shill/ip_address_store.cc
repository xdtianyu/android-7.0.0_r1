//
// Copyright (C) 2013 The Android Open Source Project
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

#include "shill/ip_address_store.h"

#include <iterator>

#include <stdlib.h>
#include <time.h>

using std::advance;

namespace shill {

// This is a less than comparison so that IPAddress can be stored in a set.
// We do not care about a semantically meaningful comparison. This is
// deterministic, and that's all that matters.
bool IPAddressLTIgnorePrefix::operator () (const IPAddress& lhs,
                                           const IPAddress& rhs) const {
  return lhs.ToString() < rhs.ToString();
}

IPAddressStore::IPAddressStore() : random_engine_(time(nullptr)) {
}

IPAddressStore::~IPAddressStore() {}

void IPAddressStore::AddUnique(const IPAddress& ip) {
  ip_addresses_.insert(ip);
}

void IPAddressStore::Remove(const IPAddress& ip) {
  ip_addresses_.erase(ip);
}

void IPAddressStore::Clear() {
  ip_addresses_.clear();
}

bool IPAddressStore::Contains(const IPAddress& ip) const {
  return ip_addresses_.find(ip) != ip_addresses_.end();
}

size_t IPAddressStore::Count() const {
  return ip_addresses_.size();
}

bool IPAddressStore::Empty() const {
  return ip_addresses_.empty();
}

IPAddress IPAddressStore::GetRandomIP() {
  if (ip_addresses_.empty())
    return IPAddress(IPAddress::kFamilyUnknown);
  std::uniform_int_distribution<int> uniform_rand(0, ip_addresses_.size() - 1);
  int index = uniform_rand(random_engine_);
  IPAddresses::const_iterator cit = ip_addresses_.begin();
  advance(cit, index);
  return *cit;
}

}  // namespace shill
