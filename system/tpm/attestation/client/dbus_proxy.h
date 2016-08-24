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

#ifndef ATTESTATION_CLIENT_DBUS_PROXY_H_
#define ATTESTATION_CLIENT_DBUS_PROXY_H_

#include "attestation/common/attestation_interface.h"

#include <string>

#include <base/memory/ref_counted.h>
#include <dbus/bus.h>
#include <dbus/object_proxy.h>

namespace attestation {

// An implementation of AttestationInterface that forwards requests over D-Bus.
// Usage:
//   std::unique_ptr<AttestationInterface> attestation = new DBusProxy();
//   attestation->Initialize();
//   attestation->CreateGoogleAttestedKey(...);
class DBusProxy : public AttestationInterface {
 public:
  DBusProxy();
  virtual ~DBusProxy();

  // AttestationInterface methods.
  bool Initialize() override;
  void CreateGoogleAttestedKey(
      const CreateGoogleAttestedKeyRequest& request,
      const CreateGoogleAttestedKeyCallback& callback) override;
  void GetKeyInfo(const GetKeyInfoRequest& request,
                  const GetKeyInfoCallback& callback) override;
  void GetEndorsementInfo(const GetEndorsementInfoRequest& request,
                          const GetEndorsementInfoCallback& callback) override;
  void GetAttestationKeyInfo(
      const GetAttestationKeyInfoRequest& request,
      const GetAttestationKeyInfoCallback& callback) override;
  void ActivateAttestationKey(
      const ActivateAttestationKeyRequest& request,
      const ActivateAttestationKeyCallback& callback) override;
  void CreateCertifiableKey(
      const CreateCertifiableKeyRequest& request,
      const CreateCertifiableKeyCallback& callback) override;
  void Decrypt(const DecryptRequest& request,
               const DecryptCallback& callback) override;
  void Sign(const SignRequest& request, const SignCallback& callback) override;
  void RegisterKeyWithChapsToken(
      const RegisterKeyWithChapsTokenRequest& request,
      const RegisterKeyWithChapsTokenCallback& callback) override;

  // Useful for testing.
  void set_object_proxy(dbus::ObjectProxy* object_proxy) {
    object_proxy_ = object_proxy;
  }

 private:
  scoped_refptr<dbus::Bus> bus_;
  dbus::ObjectProxy* object_proxy_;
};

}  // namespace attestation

#endif  // ATTESTATION_CLIENT_DBUS_PROXY_H_
