//
// Copyright (C) 2011 The Android Open Source Project
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

#include "shill/mock_profile.h"

#include <string>

#include <base/memory/ref_counted.h>
#include <base/strings/stringprintf.h>
#include <gmock/gmock.h>

#include "shill/refptr_types.h"

namespace shill {

MockProfile::MockProfile(ControlInterface* control,
                         Metrics* metrics,
                         Manager* manager)
    : Profile(control, metrics, manager, Identifier("mock"), base::FilePath(),
              false) {
}

MockProfile::MockProfile(ControlInterface* control,
                         Metrics* metrics,
                         Manager* manager,
                         const std::string& identifier)
    : Profile(control, metrics, manager, Identifier(identifier),
              base::FilePath(), false) {
}

MockProfile::~MockProfile() {}

}  // namespace shill
