// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "policy/libpolicy.h"

#include <base/logging.h>

#include "policy/device_policy.h"
#ifndef __ANDROID__
#include "policy/device_policy_impl.h"
#endif

namespace policy {

PolicyProvider::PolicyProvider()
    : device_policy_(nullptr),
      device_policy_is_loaded_(false) {
#ifndef __ANDROID__
  device_policy_.reset(new DevicePolicyImpl());
#endif
}

PolicyProvider::PolicyProvider(DevicePolicy* device_policy)
    : device_policy_(device_policy),
      device_policy_is_loaded_(true) {
}

PolicyProvider::~PolicyProvider() {
}

bool PolicyProvider::Reload() {
  if (!device_policy_)
    return false;
  device_policy_is_loaded_ = device_policy_->LoadPolicy();
  if (!device_policy_is_loaded_) {
    LOG(WARNING) << "Could not load the device policy file.";
  }
  return device_policy_is_loaded_;
}

bool PolicyProvider::device_policy_is_loaded() const {
  return device_policy_is_loaded_;
}

const DevicePolicy& PolicyProvider::GetDevicePolicy() const {
  if (!device_policy_is_loaded_)
    DCHECK("Trying to get policy data but policy was not loaded!");

  return *device_policy_;
}

}  // namespace policy
