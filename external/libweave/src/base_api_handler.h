// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_BASE_API_HANDLER_H_
#define LIBWEAVE_SRC_BASE_API_HANDLER_H_

#include <memory>
#include <string>

#include <base/memory/weak_ptr.h>

namespace weave {

class Command;
class Device;
class DeviceRegistrationInfo;
struct Settings;

// Handles commands from 'base' package.
// Objects of the class subscribe for notification from CommandManager and
// execute incoming commands.
// Handled commands:
//  base.updateDeviceInfo
//  base.updateBaseConfiguration
class BaseApiHandler final {
 public:
  BaseApiHandler(DeviceRegistrationInfo* device_info, Device* device);

 private:
  void UpdateBaseConfiguration(const std::weak_ptr<Command>& command);
  void UpdateDeviceInfo(const std::weak_ptr<Command>& command);
  void OnConfigChanged(const Settings& settings);

  DeviceRegistrationInfo* device_info_;
  Device* device_{nullptr};

  base::WeakPtrFactory<BaseApiHandler> weak_ptr_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(BaseApiHandler);
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_BASE_API_HANDLER_H_
