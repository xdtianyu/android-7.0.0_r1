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

#include "shill/mock_connection_health_checker.h"

#include "shill/async_connection.h"
#include "shill/connection.h"

using base::Callback;

namespace shill {

MockConnectionHealthChecker::MockConnectionHealthChecker(
    ConnectionRefPtr connection,
    EventDispatcher* dispatcher,
    IPAddressStore* remote_ips,
    const Callback<void(Result)>& result_callback)
    : ConnectionHealthChecker(connection,
                              dispatcher,
                              remote_ips,
                              result_callback) {}

MockConnectionHealthChecker::~MockConnectionHealthChecker() {}

}  // namespace shill
