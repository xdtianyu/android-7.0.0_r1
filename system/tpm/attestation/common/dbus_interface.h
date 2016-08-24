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

#ifndef ATTESTATION_COMMON_DBUS_INTERFACE_H_
#define ATTESTATION_COMMON_DBUS_INTERFACE_H_

namespace attestation {

// TODO(namnguyen): Move to brillo/system_api once we're ready.
constexpr char kAttestationInterface[] = "org.chromium.Attestation";
constexpr char kAttestationServicePath[] = "/org/chromium/Attestation";
constexpr char kAttestationServiceName[] = "org.chromium.Attestation";

// Methods exported by attestation.
constexpr char kCreateGoogleAttestedKey[] = "CreateGoogleAttestedKey";
constexpr char kGetKeyInfo[] = "GetKeyInfo";
constexpr char kGetEndorsementInfo[] = "GetEndorsementInfo";
constexpr char kGetAttestationKeyInfo[] = "GetAttestationKeyInfo";
constexpr char kActivateAttestationKey[] = "ActivateAttestationKey";
constexpr char kCreateCertifiableKey[] = "CreateCertifiableKey";
constexpr char kDecrypt[] = "Decrypt";
constexpr char kSign[] = "Sign";
constexpr char kRegisterKeyWithChapsToken[] = "RegisterKeyWithChapsToken";

}  // namespace attestation

#endif  // ATTESTATION_COMMON_DBUS_INTERFACE_H_
