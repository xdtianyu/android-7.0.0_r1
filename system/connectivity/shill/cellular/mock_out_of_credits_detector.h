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

#ifndef SHILL_CELLULAR_MOCK_OUT_OF_CREDITS_DETECTOR_H_
#define SHILL_CELLULAR_MOCK_OUT_OF_CREDITS_DETECTOR_H_

#include <gmock/gmock.h>

#include "shill/cellular/out_of_credits_detector.h"

namespace shill {

class MockOutOfCreditsDetector : public OutOfCreditsDetector {
 public:
  MockOutOfCreditsDetector(EventDispatcher* dispatcher,
                           Manager* manager,
                           Metrics* metrics,
                           CellularService* service);
  ~MockOutOfCreditsDetector() override;

  MOCK_METHOD0(ResetDetector, void());
  MOCK_CONST_METHOD0(IsDetecting, bool());
  MOCK_METHOD2(NotifyServiceStateChanged,
               void(Service::ConnectState old_state,
                    Service::ConnectState new_state));
  MOCK_METHOD1(NotifySubscriptionStateChanged,
               void(uint32_t subscription_state));
  MOCK_CONST_METHOD0(out_of_credits, bool());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockOutOfCreditsDetector);
};

}  // namespace shill

#endif  // SHILL_CELLULAR_MOCK_OUT_OF_CREDITS_DETECTOR_H_
