// Copyright 2016 The Android Open Source Project
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

#include "buffet/binder_weave_service.h"

#include <algorithm>

#include <base/bind.h>
#include <weave/command.h>
#include <weave/device.h>

#include "buffet/binder_command_proxy.h"
#include "common/binder_utils.h"

using weaved::binder_utils::ToStatus;
using weaved::binder_utils::ToString;
using weaved::binder_utils::ToString16;

namespace buffet {

BinderWeaveService::BinderWeaveService(
    weave::Device* device,
    android::sp<android::weave::IWeaveClient> client)
    : device_{device}, client_{client} {}

BinderWeaveService::~BinderWeaveService() {
  // TODO(avakulenko): Make it possible to remove components from the tree in
  // libweave and enable the following code.
  // for (const std::string& component : components_)
  //   device_->RemoveComponent(component, nullptr);
}

android::binder::Status BinderWeaveService::addComponent(
    const android::String16& name,
    const std::vector<android::String16>& traits) {
  std::string component_name = ToString(name);
  weave::ErrorPtr error;
  std::vector<std::string> supported_traits;
  std::transform(traits.begin(), traits.end(),
                 std::back_inserter(supported_traits), ToString);
  if (!device_->AddComponent(component_name, supported_traits, &error))
    return ToStatus(false, &error);
  components_.push_back(component_name);
  return android::binder::Status::ok();
}

android::binder::Status BinderWeaveService::registerCommandHandler(
    const android::String16& component,
    const android::String16& command) {
  std::string component_name = ToString(component);
  std::string command_name = ToString(command);
  device_->AddCommandHandler(component_name, command_name,
                             base::Bind(&BinderWeaveService::OnCommand,
                                        weak_ptr_factory_.GetWeakPtr(),
                                        component_name, command_name));
  return android::binder::Status::ok();
}

android::binder::Status BinderWeaveService::updateState(
    const android::String16& component,
    const android::String16& state) {
  weave::ErrorPtr error;
  return ToStatus(device_->SetStatePropertiesFromJson(ToString(component),
                                                      ToString(state),
                                                      &error),
                  &error);
}

void BinderWeaveService::OnCommand(
    const std::string& component_name,
    const std::string& command_name,
    const std::weak_ptr<weave::Command>& command) {
  android::sp<android::weave::IWeaveCommand> command_proxy =
      new BinderCommandProxy{command};
  client_->onCommand(ToString16(component_name), ToString16(command_name),
                     command_proxy);
}

}  // namespace buffet
