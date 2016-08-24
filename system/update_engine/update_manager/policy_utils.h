//
// Copyright (C) 2014 The Android Open Source Project
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

#ifndef UPDATE_ENGINE_UPDATE_MANAGER_POLICY_UTILS_H_
#define UPDATE_ENGINE_UPDATE_MANAGER_POLICY_UTILS_H_

#include "update_engine/update_manager/policy.h"

// Checks that the passed pointer value is not null, returning kFailed on the
// current context and setting the *error description when it is null. The
// intended use is to validate variable failures while using
// EvaluationContext::GetValue, for example:
//
//   const int* my_value = ec->GetValue(state->my_provider()->var_my_value());
//   POLICY_CHECK_VALUE_AND_FAIL(my_value, error);
//
#define POLICY_CHECK_VALUE_AND_FAIL(ptr, error) \
    do { \
      if ((ptr) == nullptr) { \
        *(error) = #ptr " is required but is null."; \
        return EvalStatus::kFailed; \
      } \
    } while (false)

#endif  // UPDATE_ENGINE_UPDATE_MANAGER_POLICY_UTILS_H_
