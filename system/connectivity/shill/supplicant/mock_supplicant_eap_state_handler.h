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

#ifndef SHILL_SUPPLICANT_MOCK_SUPPLICANT_EAP_STATE_HANDLER_H_
#define SHILL_SUPPLICANT_MOCK_SUPPLICANT_EAP_STATE_HANDLER_H_

#include <string>

#include <gmock/gmock.h>

#include "shill/supplicant/supplicant_eap_state_handler.h"

namespace shill {

class MockSupplicantEAPStateHandler : public SupplicantEAPStateHandler {
 public:
  MockSupplicantEAPStateHandler();
  ~MockSupplicantEAPStateHandler() override;

  MOCK_METHOD3(ParseStatus, bool(const std::string& status,
                                 const std::string& parameter,
                                 Service::ConnectFailure* failure));
  MOCK_METHOD0(Reset, void());
  MOCK_METHOD0(is_eap_in_progress, bool());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockSupplicantEAPStateHandler);
};

}  // namespace shill

#endif  // SHILL_SUPPLICANT_MOCK_SUPPLICANT_EAP_STATE_HANDLER_H_
