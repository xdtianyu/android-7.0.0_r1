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

#include "update_engine/weave_service_factory.h"

#if USE_WEAVE
#include "update_engine/weave_service.h"
#endif

namespace chromeos_update_engine {

std::unique_ptr<WeaveServiceInterface> ConstructWeaveService(
    WeaveServiceInterface::DelegateInterface* delegate) {
  std::unique_ptr<WeaveServiceInterface> result;
  if (!delegate)
    return result;

#if USE_WEAVE
  WeaveService* weave_service = new WeaveService();
  result.reset(weave_service);
  if (!weave_service->Init(delegate))
    result.reset();
#endif
  return result;
}

}  // namespace chromeos_update_engine
