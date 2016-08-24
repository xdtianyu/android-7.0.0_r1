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

#ifndef APMANAGER_MOCK_SERVICE_ADAPTOR_H_
#define APMANAGER_MOCK_SERVICE_ADAPTOR_H_

#include <string>

#include <base/macros.h>
#include <gmock/gmock.h>

#include "apmanager/service_adaptor_interface.h"

namespace apmanager {

class MockServiceAdaptor : public ServiceAdaptorInterface {
 public:
  MockServiceAdaptor();
  ~MockServiceAdaptor() override;

  MOCK_METHOD0(GetRpcObjectIdentifier, RPCObjectIdentifier());
  MOCK_METHOD1(SetConfig, void(Config* config));
  MOCK_METHOD1(SetState, void(const std::string& state));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockServiceAdaptor);
};

}  // namespace apmanager

#endif  // APMANAGER_MOCK_SERVICE_ADAPTOR_H_
