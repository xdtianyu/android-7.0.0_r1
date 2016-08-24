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

#ifndef SHILL_CELLULAR_SUBSCRIPTION_STATE_OUT_OF_CREDITS_DETECTOR_H_
#define SHILL_CELLULAR_SUBSCRIPTION_STATE_OUT_OF_CREDITS_DETECTOR_H_

#include "shill/cellular/out_of_credits_detector.h"

namespace shill {

// Detects out-of-credits condition by using the subscription state.
class SubscriptionStateOutOfCreditsDetector : public OutOfCreditsDetector {
 public:
  SubscriptionStateOutOfCreditsDetector(EventDispatcher* dispatcher,
                                        Manager* manager,
                                        Metrics* metrics,
                                        CellularService* service);
  ~SubscriptionStateOutOfCreditsDetector() override;

  void ResetDetector() override {}
  bool IsDetecting() const override { return false; }
  void NotifyServiceStateChanged(
      Service::ConnectState old_state,
      Service::ConnectState new_state) override {}
  void NotifySubscriptionStateChanged(uint32_t subscription_state) override;

 private:
  friend class SubscriptionStateOutOfCreditsDetectorTest;

  DISALLOW_COPY_AND_ASSIGN(SubscriptionStateOutOfCreditsDetector);
};

}  // namespace shill

#endif  // SHILL_CELLULAR_SUBSCRIPTION_STATE_OUT_OF_CREDITS_DETECTOR_H_
