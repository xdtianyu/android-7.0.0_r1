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

#ifndef APMANAGER_SERVICE_ADAPTOR_INTERFACE_H_
#define APMANAGER_SERVICE_ADAPTOR_INTERFACE_H_

#include <string>

#include "apmanager/rpc_interface.h"

namespace apmanager {

class Config;

class ServiceAdaptorInterface {
 public:
  virtual ~ServiceAdaptorInterface() {}

  virtual RPCObjectIdentifier GetRpcObjectIdentifier() = 0;
  virtual void SetConfig(Config* config) = 0;
  virtual void SetState(const std::string& state) = 0;
};

}  // namespace apmanager

#endif  // APMANAGER_SERVICE_ADAPTOR_INTERFACE_H_
