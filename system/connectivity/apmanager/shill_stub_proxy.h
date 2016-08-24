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

#ifndef APMANAGER_SHILL_STUB_PROXY_H_
#define APMANAGER_SHILL_STUB_PROXY_H_

#include <string>

#include <base/macros.h>

#include "apmanager/shill_proxy_interface.h"

namespace apmanager {

class ShillStubProxy : public ShillProxyInterface {
 public:
  ShillStubProxy();
  ~ShillStubProxy() override;

  // Implementation of ShillProxyInterface.
  bool ClaimInterface(const std::string& interface_name) override;
  bool ReleaseInterface(const std::string& interface_name) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ShillStubProxy);
};

}  // namespace apmanager

#endif  // APMANAGER_SHILL_STUB_PROXY_H_
