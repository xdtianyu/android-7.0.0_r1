//
// Copyright 2015 The Android Open Source Project
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

#ifndef APMANAGER_FAKE_DEVICE_ADAPTOR_H_
#define APMANAGER_FAKE_DEVICE_ADAPTOR_H_

#include <string>

#include <base/macros.h>

#include "apmanager/device_adaptor_interface.h"

namespace apmanager {

class FakeDeviceAdaptor : public DeviceAdaptorInterface {
 public:
  FakeDeviceAdaptor();
  ~FakeDeviceAdaptor() override;

  void SetDeviceName(const std::string& device_name) override;
  std::string GetDeviceName() override;
  void SetPreferredApInterface(const std::string& interface_name) override;
  std::string GetPreferredApInterface() override;
  void SetInUse(bool in_use) override;
  bool GetInUse() override;

private:
  std::string device_name_;
  std::string preferred_ap_interface_;
  bool in_use_;

  DISALLOW_COPY_AND_ASSIGN(FakeDeviceAdaptor);
};

}  // namespace apmanager

#endif  // APMANAGER_FAKE_DEVICE_ADAPTOR_H_
