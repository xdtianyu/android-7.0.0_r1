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

#include "shill/cellular/mock_cellular.h"

#include <gmock/gmock.h>

#include "shill/error.h"

namespace shill {

// TODO(rochberg): The cellular constructor does work.  Ought to fix
// this so that we don't depend on passing real values in for Type.

MockCellular::MockCellular(ModemInfo* modem_info,
                           const std::string& link_name,
                           const std::string& address,
                           int interface_index,
                           Type type,
                           const std::string& service,
                           const std::string& path)
    : Cellular(modem_info, link_name, address, interface_index, type,
               service, path) {}

MockCellular::~MockCellular() {}

}  // namespace shill
