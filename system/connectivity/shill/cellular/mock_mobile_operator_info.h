//
// Copyright (C) 2014 The Android Open Source Project
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

#ifndef SHILL_CELLULAR_MOCK_MOBILE_OPERATOR_INFO_H_
#define SHILL_CELLULAR_MOCK_MOBILE_OPERATOR_INFO_H_

#include <string>
#include <vector>

#include <gmock/gmock.h>

#include "shill/cellular/mobile_operator_info.h"

using testing::ReturnRef;

namespace shill {

class MockMobileOperatorInfo : public MobileOperatorInfo {
 public:
  MockMobileOperatorInfo(EventDispatcher* dispatcher,
                         const std::string& info_owner);
  ~MockMobileOperatorInfo() override;

  MOCK_CONST_METHOD0(IsMobileNetworkOperatorKnown, bool());

  MOCK_CONST_METHOD0(mccmnc, const std::string&());
  MOCK_CONST_METHOD0(olp_list,
                     const std::vector<MobileOperatorInfo::OnlinePortal>&());
  MOCK_CONST_METHOD0(activation_code, const std::string&());
  MOCK_CONST_METHOD0(operator_name, const std::string&());
  MOCK_CONST_METHOD0(country, const std::string&());
  MOCK_CONST_METHOD0(uuid, const std::string&());

  MOCK_METHOD1(UpdateMCCMNC, void(const std::string&));
  MOCK_METHOD1(UpdateSID, void(const std::string&));
  MOCK_METHOD1(UpdateIMSI, void(const std::string&));
  MOCK_METHOD1(UpdateNID, void(const std::string&));
  MOCK_METHOD1(UpdateOperatorName, void(const std::string&));

  // Sets up the mock object to return empty strings/vectors etc for all
  // propeties.
  void SetEmptyDefaultsForProperties();

 private:
  std::string empty_mccmnc_;
  std::vector<MobileOperatorInfo::OnlinePortal> empty_olp_list_;
  std::string empty_activation_code_;
  std::string empty_operator_name_;
  std::string empty_country_;
  std::string empty_uuid_;
};

}  // namespace shill

#endif  // SHILL_CELLULAR_MOCK_MOBILE_OPERATOR_INFO_H_
