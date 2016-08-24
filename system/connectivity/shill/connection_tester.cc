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

#include "shill/connection_tester.h"

#include <string>

#include <base/bind.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/connection.h"
#include "shill/connectivity_trial.h"
#include "shill/logging.h"

using base::Bind;
using base::Callback;
using base::StringPrintf;
using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kPortal;
static string ObjectID(Connection* c) { return c->interface_name(); }
}

const int ConnectionTester::kTrialTimeoutSeconds = 5;

ConnectionTester::ConnectionTester(
    ConnectionRefPtr connection,
    EventDispatcher* dispatcher,
    const Callback<void()>& callback)
    : connection_(connection),
      dispatcher_(dispatcher),
      weak_ptr_factory_(this),
      tester_callback_(callback),
      connectivity_trial_(
          new ConnectivityTrial(connection_,
                                dispatcher_,
                                kTrialTimeoutSeconds,
                                Bind(&ConnectionTester::CompleteTest,
                                     weak_ptr_factory_.GetWeakPtr()))) { }

ConnectionTester::~ConnectionTester() {
  Stop();
  connectivity_trial_.reset();
}

void ConnectionTester::Start() {
  SLOG(connection_.get(), 3) << "In " << __func__;
  if (!connectivity_trial_->Start(ConnectivityTrial::kDefaultURL, 0))
    LOG(ERROR) << StringPrintf("ConnectivityTrial failed to parse default "
                               "URL %s", ConnectivityTrial::kDefaultURL);
}

void ConnectionTester::Stop() {
  SLOG(connection_.get(), 3) << "In " << __func__;
  connectivity_trial_->Stop();
}

void ConnectionTester::CompleteTest(ConnectivityTrial::Result result) {
  LOG(INFO) << StringPrintf("ConnectivityTester completed with phase==%s, "
                            "status==%s",
                            ConnectivityTrial::PhaseToString(
                                result.phase).c_str(),
                            ConnectivityTrial::StatusToString(
                                result.status).c_str());
  Stop();
  tester_callback_.Run();
}

}  // namespace shill

