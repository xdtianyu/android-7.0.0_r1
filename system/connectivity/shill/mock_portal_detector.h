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

#ifndef SHILL_MOCK_PORTAL_DETECTOR_H_
#define SHILL_MOCK_PORTAL_DETECTOR_H_

#include <string>

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/portal_detector.h"

namespace shill {

class MockPortalDetector : public PortalDetector {
 public:
  explicit MockPortalDetector(ConnectionRefPtr connection);
  ~MockPortalDetector() override;

  MOCK_METHOD1(Start, bool(const std::string&));
  MOCK_METHOD2(StartAfterDelay, bool(const std::string&, int delay_seconds));
  MOCK_METHOD0(Stop, void());
  MOCK_METHOD0(IsInProgress, bool());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockPortalDetector);
};

}  // namespace shill

#endif  // SHILL_MOCK_PORTAL_DETECTOR_H_
