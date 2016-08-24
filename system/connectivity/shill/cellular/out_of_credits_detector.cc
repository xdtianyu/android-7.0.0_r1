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

#include "shill/cellular/out_of_credits_detector.h"

#include <string>

#include "shill/cellular/active_passive_out_of_credits_detector.h"
#include "shill/cellular/cellular_service.h"
#include "shill/cellular/no_out_of_credits_detector.h"
#include "shill/cellular/subscription_state_out_of_credits_detector.h"
#include "shill/logging.h"

namespace shill {

using std::string;

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kCellular;
static string ObjectID(CellularService* c) { return c->GetRpcIdentifier(); }
}

OutOfCreditsDetector::OutOfCreditsDetector(EventDispatcher* dispatcher,
                                           Manager* manager,
                                           Metrics* metrics,
                                           CellularService* service)
    : dispatcher_(dispatcher),
      manager_(manager),
      metrics_(metrics),
      service_(service),
      out_of_credits_(false) {
}

OutOfCreditsDetector::~OutOfCreditsDetector() {
}

// static
OutOfCreditsDetector*
OutOfCreditsDetector::CreateDetector(OOCType detector_type,
                                     EventDispatcher* dispatcher,
                                     Manager* manager,
                                     Metrics* metrics,
                                     CellularService* service) {
  switch (detector_type) {
    case OOCTypeActivePassive:
      LOG(INFO) << __func__
                << ": Using active-passive out-of-credits detection";
      return
          new ActivePassiveOutOfCreditsDetector(dispatcher,
                                                manager,
                                                metrics,
                                                service);
    case OOCTypeSubscriptionState:
      LOG(INFO) << __func__
                << ": Using subscription status out-of-credits detection";
      return
          new SubscriptionStateOutOfCreditsDetector(dispatcher,
                                                    manager,
                                                    metrics,
                                                    service);
    default:
      LOG(INFO) << __func__ << ": No out-of-credits detection";
      return
          new NoOutOfCreditsDetector(dispatcher,
                                     manager,
                                     metrics,
                                     service);
  }
}

void OutOfCreditsDetector::ReportOutOfCredits(bool state) {
  SLOG(service_, 2) << __func__ << ": " << state;
  if (state == out_of_credits_) {
    return;
  }
  out_of_credits_ = state;
  service_->SignalOutOfCreditsChanged(state);
}

}  // namespace shill
