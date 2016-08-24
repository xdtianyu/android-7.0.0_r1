//
// Copyright (C) 2016 The Android Open Source Project
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

#ifndef SHILL_PROFILE_ADAPTOR_STUB_H_
#define SHILL_PROFILE_ADAPTOR_STUB_H_

#include <string>

#include <base/macros.h>

#include "shill/adaptor_interfaces.h"
#include "shill/adaptor_stub.h"

namespace shill {

class ProfileAdaptorStub : public AdaptorStub, public ProfileAdaptorInterface {
 public:
  explicit ProfileAdaptorStub(const std::string& id);

  const std::string& GetRpcIdentifier() override { return rpc_id(); }
  void EmitBoolChanged(const std::string& name, bool value) override {}
  void EmitUintChanged(const std::string& name, uint32_t value) override {}
  void EmitIntChanged(const std::string& name, int value) override {}
  void EmitStringChanged(const std::string& name,
                         const std::string& value) override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(ProfileAdaptorStub);
};

}  // namespace shill

#endif  // SHILL_PROFILE_ADAPTOR_STUB_H_
