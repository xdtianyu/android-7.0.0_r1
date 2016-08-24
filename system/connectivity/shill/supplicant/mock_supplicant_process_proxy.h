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

#ifndef SHILL_SUPPLICANT_MOCK_SUPPLICANT_PROCESS_PROXY_H_
#define SHILL_SUPPLICANT_MOCK_SUPPLICANT_PROCESS_PROXY_H_

#include <map>
#include <string>

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/supplicant/supplicant_process_proxy_interface.h"

namespace shill {

class MockSupplicantProcessProxy : public SupplicantProcessProxyInterface {
 public:
  MockSupplicantProcessProxy();
  ~MockSupplicantProcessProxy() override;

  MOCK_METHOD2(CreateInterface,
               bool(const KeyValueStore& args, std::string* rpc_identifier));
  MOCK_METHOD2(GetInterface,
               bool(const std::string& ifname, std::string* rpc_identifier));
  MOCK_METHOD1(RemoveInterface, bool(const std::string& rpc_identifier));
  MOCK_METHOD1(GetDebugLevel, bool(std::string* level));
  MOCK_METHOD1(SetDebugLevel, bool(const std::string& level));
  MOCK_METHOD0(ExpectDisconnect, bool());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockSupplicantProcessProxy);
};

}  // namespace shill

#endif  // SHILL_SUPPLICANT_MOCK_SUPPLICANT_PROCESS_PROXY_H_
