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

#ifndef TPM_MANAGER_COMMON_TPM_NVRAM_DBUS_INTERFACE_H_
#define TPM_MANAGER_COMMON_TPM_NVRAM_DBUS_INTERFACE_H_

namespace tpm_manager {

constexpr char kTpmNvramInterface[] = "org.chromium.TpmNvram";

// Methods exported by tpm_manager nvram D-Bus interface.
constexpr char kDefineNvram[] = "DefineNvram";
constexpr char kDestroyNvram[] = "DestroyNvram";
constexpr char kWriteNvram[] = "WriteNvram";
constexpr char kReadNvram[] = "ReadNvram";
constexpr char kIsNvramDefined[] = "IsNvramDefined";
constexpr char kIsNvramLocked[] = "IsNvramLocked";
constexpr char kGetNvramSize[] = "GetNvramSize";

}  // namespace tpm_manager

#endif  // TPM_MANAGER_COMMON_TPM_NVRAM_DBUS_INTERFACE_H_
