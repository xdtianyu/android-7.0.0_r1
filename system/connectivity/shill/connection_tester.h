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

#ifndef SHILL_CONNECTION_TESTER_H_
#define SHILL_CONNECTION_TESTER_H_

#include <memory>
#include <string>
#include <vector>

#include <base/callback.h>
#include <base/cancelable_callback.h>
#include <base/memory/ref_counted.h>
#include <base/memory/weak_ptr.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/connectivity_trial.h"
#include "shill/refptr_types.h"

namespace shill {

// The ConnectionTester class implements a single trial connectivity test
// to evaluate a connection in shill.  This will evaluate if a connection has
// "general internet connectivity."
//
// This test will be triggered through a D-Bus call on demand by a user to
// capture state of an existing connection and create detailed logging
// information to be used for debugging connectivity issues.
//
// This functionality will be implemented by testing the connection with a
// single ConnectivityTrial attempt.
class ConnectionTester {
 public:
  ConnectionTester(ConnectionRefPtr connection,
                   EventDispatcher* dispatcher,
                   const base::Closure& callback);
  virtual ~ConnectionTester();

  // Start a connectivity test.  The Start method creates a ConnectivityTrial
  // instance and performs a single ConnectivityTrial.  The results are logged
  // and when the trial completes, the supplied callback is notified.
  virtual void Start();

  // End the current ConnectivityTester by calling Stop on underlying
  // ConnectivityTrial.  The callback will not be called.
  virtual void Stop();

 private:
  friend class ConnectionTesterTest;
  FRIEND_TEST(ConnectionTesterTest, CompleteTest);

  // Time to wait for the attempt to complete.
  static const int kTrialTimeoutSeconds;
  // Callback used by ConnectivityTrial to report results.
  void CompleteTest(ConnectivityTrial::Result result);

  ConnectionRefPtr connection_;
  EventDispatcher* dispatcher_;
  base::WeakPtrFactory<ConnectionTester> weak_ptr_factory_;
  base::Callback<void()> tester_callback_;
  std::unique_ptr<ConnectivityTrial> connectivity_trial_;

  DISALLOW_COPY_AND_ASSIGN(ConnectionTester);
};

}  // namespace shill

#endif  // SHILL_CONNECTION_TESTER_H_
