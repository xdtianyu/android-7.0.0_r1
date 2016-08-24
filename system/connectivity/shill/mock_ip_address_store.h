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

#ifndef SHILL_MOCK_IP_ADDRESS_STORE_H_
#define SHILL_MOCK_IP_ADDRESS_STORE_H_

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/ip_address_store.h"

namespace shill {

class MockIPAddressStore : public IPAddressStore {
 public:
  MockIPAddressStore();
  ~MockIPAddressStore() override;

  MOCK_METHOD1(AddUnique, void(const IPAddress& ip));
  MOCK_METHOD0(Clear, void());
  MOCK_CONST_METHOD0(Count, size_t());
  MOCK_CONST_METHOD0(Empty, bool());
  MOCK_METHOD0(GetRandomIP, IPAddress());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockIPAddressStore);
};

}  // namespace shill

#endif  // SHILL_MOCK_IP_ADDRESS_STORE_H_
