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

#ifndef SHILL_ETHERNET_MOCK_ETHERNET_EAP_PROVIDER_H_
#define SHILL_ETHERNET_MOCK_ETHERNET_EAP_PROVIDER_H_

#include "shill/ethernet/ethernet_eap_provider.h"

#include <gmock/gmock.h>

#include "shill/ethernet/ethernet_eap_service.h"

namespace shill {

class MockEthernetEapProvider : public EthernetEapProvider {
 public:
  MockEthernetEapProvider();
  ~MockEthernetEapProvider() override;

  MOCK_METHOD0(Start, void());
  MOCK_METHOD0(Stop, void());
  MOCK_METHOD2(SetCredentialChangeCallback,
               void(Ethernet* device, CredentialChangeCallback callback));
  MOCK_METHOD1(ClearCredentialChangeCallback, void(Ethernet* device));
  MOCK_CONST_METHOD0(OnCredentialsChanged, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockEthernetEapProvider);
};

}  // namespace shill

#endif  // SHILL_ETHERNET_MOCK_ETHERNET_EAP_PROVIDER_H_
