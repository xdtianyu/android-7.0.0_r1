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

#ifndef SHILL_SUPPLICANT_MOCK_SUPPLICANT_INTERFACE_PROXY_H_
#define SHILL_SUPPLICANT_MOCK_SUPPLICANT_INTERFACE_PROXY_H_

#include <map>
#include <string>

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/refptr_types.h"
#include "shill/supplicant/supplicant_interface_proxy_interface.h"

namespace shill {

class MockSupplicantInterfaceProxy : public SupplicantInterfaceProxyInterface {
 public:
  MockSupplicantInterfaceProxy();
  ~MockSupplicantInterfaceProxy() override;

  MOCK_METHOD2(AddNetwork,
               bool(const KeyValueStore& args, std::string* rpc_identifier));
  MOCK_METHOD0(EnableHighBitrates, bool());
  MOCK_METHOD0(EAPLogoff, bool());
  MOCK_METHOD0(EAPLogon, bool());
  MOCK_METHOD0(Disconnect, bool());
  MOCK_METHOD1(FlushBSS, bool(const uint32_t& age));
  MOCK_METHOD3(NetworkReply, bool(const std::string& network,
                                  const std::string& field,
                                  const std::string& value));
  MOCK_METHOD0(Reassociate, bool());
  MOCK_METHOD0(Reattach, bool());
  MOCK_METHOD0(RemoveAllNetworks, bool());
  MOCK_METHOD1(RemoveNetwork, bool(const std::string& network));
  MOCK_METHOD1(Roam, bool(const std::string& addr));
  MOCK_METHOD1(Scan, bool(const KeyValueStore& args));
  MOCK_METHOD1(SelectNetwork, bool(const std::string& network));
  MOCK_METHOD1(SetFastReauth, bool(bool enabled));
  MOCK_METHOD1(SetRoamThreshold, bool(uint16_t threshold));
  MOCK_METHOD1(SetScanInterval, bool(int32_t seconds));
  MOCK_METHOD1(SetDisableHighBitrates, bool(bool disable_high_bitrates));
  MOCK_METHOD1(SetSchedScan, bool(bool enable));
  MOCK_METHOD1(SetScan, bool(bool enable));
  MOCK_METHOD1(TDLSDiscover, bool(const std::string& peer));
  MOCK_METHOD1(TDLSSetup, bool(const std::string& peer));
  MOCK_METHOD2(TDLSStatus, bool(const std::string& peer, std::string* status));
  MOCK_METHOD1(TDLSTeardown, bool(const std::string& peer));
  MOCK_METHOD2(SetHT40Enable, bool(const std::string& network, bool enable));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockSupplicantInterfaceProxy);
};

}  // namespace shill

#endif  // SHILL_SUPPLICANT_MOCK_SUPPLICANT_INTERFACE_PROXY_H_
