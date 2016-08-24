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

#include "shill/cellular/mock_mobile_operator_info.h"

#include <gmock/gmock.h>

namespace shill {

MockMobileOperatorInfo::MockMobileOperatorInfo(EventDispatcher* dispatcher,
                                               const std::string& info_owner)
    : MobileOperatorInfo(dispatcher, info_owner) {}

MockMobileOperatorInfo::~MockMobileOperatorInfo() {}

void MockMobileOperatorInfo::SetEmptyDefaultsForProperties() {
  ON_CALL(*this, mccmnc()).WillByDefault(ReturnRef(empty_mccmnc_));
  ON_CALL(*this, olp_list()).WillByDefault(ReturnRef(empty_olp_list_));
  ON_CALL(*this, activation_code())
      .WillByDefault(ReturnRef(empty_activation_code_));
  ON_CALL(*this, operator_name())
      .WillByDefault(ReturnRef(empty_operator_name_));
  ON_CALL(*this, country())
      .WillByDefault(ReturnRef(empty_country_));
  ON_CALL(*this, uuid()).WillByDefault(ReturnRef(empty_uuid_));
}

}  // namespace shill
