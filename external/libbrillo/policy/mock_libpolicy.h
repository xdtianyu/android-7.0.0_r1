// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_POLICY_MOCK_LIBPOLICY_H_
#define LIBBRILLO_POLICY_MOCK_LIBPOLICY_H_

#include <gmock/gmock.h>
#include <set>

#include "policy/libpolicy.h"

#pragma GCC visibility push(default)

namespace policy {

// This is a generic mock of the PolicyProvider class.
class MockPolicyProvider : public PolicyProvider {
 public:
  MockPolicyProvider() = default;
  ~MockPolicyProvider() override = default;

  MOCK_METHOD0(Reload, bool(void));
  MOCK_CONST_METHOD0(device_policy_is_loaded, bool(void));
  MOCK_CONST_METHOD0(GetDevicePolicy, const DevicePolicy&(void));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockPolicyProvider);
};

}  // namespace policy

#pragma GCC visibility pop

#endif  // LIBBRILLO_POLICY_MOCK_LIBPOLICY_H_
