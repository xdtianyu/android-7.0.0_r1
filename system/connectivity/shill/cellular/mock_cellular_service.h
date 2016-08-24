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

#ifndef SHILL_CELLULAR_MOCK_CELLULAR_SERVICE_H_
#define SHILL_CELLULAR_MOCK_CELLULAR_SERVICE_H_

#include <string>

#include <gmock/gmock.h>

#include "shill/cellular/cellular_service.h"

namespace shill {

class MockCellularService : public CellularService {
 public:
  MockCellularService(ModemInfo* modem_info,
                      const CellularRefPtr& device);
  ~MockCellularService() override;

  MOCK_METHOD0(AutoConnect, void());
  MOCK_METHOD1(SetLastGoodApn, void(const Stringmap& apn_info));
  MOCK_METHOD0(ClearLastGoodApn, void());
  MOCK_METHOD1(SetActivationState, void(const std::string& state));
  MOCK_METHOD2(Connect, void(Error* error, const char* reason));
  MOCK_METHOD2(Disconnect, void(Error* error, const char* reason));
  MOCK_METHOD1(SetState, void(ConnectState state));
  MOCK_METHOD1(SetFailure, void(ConnectFailure failure));
  MOCK_METHOD1(SetFailureSilent, void(ConnectFailure failure));
  MOCK_CONST_METHOD0(state, ConnectState());
  MOCK_CONST_METHOD0(explicitly_disconnected, bool());
  MOCK_CONST_METHOD0(activation_state, const std::string&());
  MOCK_CONST_METHOD0(resume_start_time, const base::Time&());

 private:
  std::string default_activation_state_;

  DISALLOW_COPY_AND_ASSIGN(MockCellularService);
};

}  // namespace shill

#endif  // SHILL_CELLULAR_MOCK_CELLULAR_SERVICE_H_
