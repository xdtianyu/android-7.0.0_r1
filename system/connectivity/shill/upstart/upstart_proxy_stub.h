//
// Copyright (C) 2015 The Android Open Source Project
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

#ifndef SHILL_UPSTART_UPSTART_PROXY_STUB_H_
#define SHILL_UPSTART_UPSTART_PROXY_STUB_H_

#include <string>
#include <vector>

#include <base/macros.h>

#include "shill/upstart/upstart_proxy_interface.h"

namespace shill {

// Stub implementation of UpstartProxyInterface.
class UpstartProxyStub : public UpstartProxyInterface {
 public:
  UpstartProxyStub();
  ~UpstartProxyStub() override = default;

  // Inherited from UpstartProxyInterface.
  void EmitEvent(const std::string& name,
                 const std::vector<std::string>& env,
                 bool wait) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(UpstartProxyStub);
};

}  // namespace shill

#endif  // SHILL_UPSTART_UPSTART_PROXY_STUB_H_
