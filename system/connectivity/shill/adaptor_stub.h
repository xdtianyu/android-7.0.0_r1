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

#ifndef SHILL_ADAPTOR_STUB_H_
#define SHILL_ADAPTOR_STUB_H_

#include <string>

#include <base/macros.h>

namespace shill {

class AdaptorStub {
 public:
  explicit AdaptorStub(const std::string& id);

 protected:
  const std::string& rpc_id() { return rpc_id_; }

 private:
  std::string rpc_id_;

  DISALLOW_COPY_AND_ASSIGN(AdaptorStub);
};

}  // namespace shill

#endif  // SHILL_ADAPTOR_STUB_H_
