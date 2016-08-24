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

#ifndef BUFFET_BINDER_COMMAND_PROXY_H_
#define BUFFET_BINDER_COMMAND_PROXY_H_

#include <string>

#include <base/macros.h>
#include <weave/command.h>

#include "android/weave/BnWeaveCommand.h"

namespace buffet {

// Implementation of android::weave::IWeaveCommand binder object.
// This class simply redirects binder calls to the underlying weave::Command
// object (and performs necessary parameter/result type conversions).
class BinderCommandProxy : public android::weave::BnWeaveCommand {
 public:
  explicit BinderCommandProxy(const std::weak_ptr<weave::Command>& command);
  ~BinderCommandProxy() override = default;

  android::binder::Status getId(android::String16* id) override;
  android::binder::Status getName(android::String16* name) override;
  android::binder::Status getComponent(android::String16* component) override;
  android::binder::Status getState(android::String16* state) override;
  android::binder::Status getOrigin(android::String16* origin) override;
  android::binder::Status getParameters(android::String16* parameters) override;
  android::binder::Status getProgress(android::String16* progress) override;
  android::binder::Status getResults(android::String16* results) override;
  android::binder::Status setProgress(
      const android::String16& progress) override;
  android::binder::Status complete(const android::String16& results) override;
  android::binder::Status abort(const android::String16& errorCode,
                                const android::String16& errorMessage) override;
  android::binder::Status cancel() override;
  android::binder::Status pause() override;
  android::binder::Status setError(
      const android::String16& errorCode,
      const android::String16& errorMessage) override;

 private:
  std::weak_ptr<weave::Command> command_;

  DISALLOW_COPY_AND_ASSIGN(BinderCommandProxy);
};

}  // namespace buffet

#endif  // BUFFET_BINDER_COMMAND_PROXY_H_
