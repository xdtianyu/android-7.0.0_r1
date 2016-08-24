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

#ifndef SHILL_RPC_TASK_ADAPTOR_STUB_H_
#define SHILL_RPC_TASK_ADAPTOR_STUB_H_

#include <string>

#include <base/macros.h>

#include "shill/adaptor_interfaces.h"
#include "shill/adaptor_stub.h"

namespace shill {

class RPCTaskAdaptorStub : public AdaptorStub, public RPCTaskAdaptorInterface {
 public:
  explicit RPCTaskAdaptorStub(const std::string& id);

  const std::string& GetRpcIdentifier() override { return rpc_id(); }
  const std::string& GetRpcConnectionIdentifier() override { return rpc_id(); }

 private:
  DISALLOW_COPY_AND_ASSIGN(RPCTaskAdaptorStub);
};

}  // namespace shill

#endif  // SHILL_RPC_TASK_ADAPTOR_STUB_H_
