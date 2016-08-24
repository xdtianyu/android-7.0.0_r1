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

#ifndef BUFFET_BINDER_WEAVE_SERVICE_H_
#define BUFFET_BINDER_WEAVE_SERVICE_H_

#include <memory>
#include <vector>
#include <string>

#include <base/macros.h>
#include <base/memory/weak_ptr.h>

#include "android/weave/IWeaveClient.h"
#include "android/weave/BnWeaveService.h"

namespace weave {
class Command;
class Device;
}

namespace buffet {

// An implementation of android::weave::IWeaveService binder.
// This object is a proxy for weave::Device. A new instance of weave service is
// created for each connected client. As soon as the client disconnects, this
// object takes care of cleaning up that client's resources (e.g. it removes
// the components and their state added by the client).
class BinderWeaveService final : public android::weave::BnWeaveService {
 public:
  BinderWeaveService(weave::Device* device,
                     android::sp<android::weave::IWeaveClient> client);
  ~BinderWeaveService() override;

 private:
  // Binder methods for android::weave::IWeaveService:
  android::binder::Status addComponent(
      const android::String16& name,
      const std::vector<android::String16>& traits) override;
  android::binder::Status registerCommandHandler(
      const android::String16& component,
      const android::String16& command) override;
  android::binder::Status updateState(
      const android::String16& component,
      const android::String16& state) override;

  void OnCommand(const std::string& component_name,
                 const std::string& command_name,
                 const std::weak_ptr<weave::Command>& command);

  weave::Device* device_;
  android::sp<android::weave::IWeaveClient> client_;
  std::vector<std::string> components_;

  base::WeakPtrFactory<BinderWeaveService> weak_ptr_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(BinderWeaveService);
};

}  // namespace buffet

#endif  // BUFFET_BINDER_WEAVE_SERVICE_H_
