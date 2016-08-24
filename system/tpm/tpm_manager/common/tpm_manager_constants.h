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

#ifndef TPM_MANAGER_COMMON_TPM_MANAGER_CONSTANTS_H_
#define TPM_MANAGER_COMMON_TPM_MANAGER_CONSTANTS_H_

namespace tpm_manager {

// These constants are used to set up and access the D-Bus interface for
// TpmManager.
constexpr char kTpmManagerServiceName[] = "org.chromium.TpmManager";
constexpr char kTpmManagerServicePath[] = "/org/chromium/TpmManager";

// These constants define the ownership dependencies. On a chromeos system,
// there are dependencies on Attestation and InstallAttributes, but at the
// moment TpmManager has no clients, so we only add a dependency on Test.
// TODO(usanghi): Figure out a way to handle ownership dependencies
// dynamically (b/25341605).
constexpr const char* kTestDependency = "Test";

// Array to easily access the list of ownership dependencies.
// Note: When dependencies are added/removed from the above list, they should
// be modified here as well.
constexpr const char* kInitialTpmOwnerDependencies[] = { kTestDependency };

}  // namespace tpm_manager

#endif  // TPM_MANAGER_COMMON_TPM_MANAGER_CONSTANTS_H_
