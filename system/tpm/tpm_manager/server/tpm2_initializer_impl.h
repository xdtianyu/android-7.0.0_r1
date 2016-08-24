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

#ifndef TPM_MANAGER_SERVER_TPM2_INITIALIZER_IMPL_H_
#define TPM_MANAGER_SERVER_TPM2_INITIALIZER_IMPL_H_

#include "tpm_manager/server/tpm_initializer.h"

#include <string>
#include <memory>

#include <base/macros.h>
#include <trunks/trunks_factory.h>

#include "tpm_manager/server/local_data_store.h"
#include "tpm_manager/server/openssl_crypto_util.h"
#include "tpm_manager/server/tpm_status.h"

namespace tpm_manager {

// This class initializes a Tpm2.0 chip by taking ownership. Example use of
// this class is:
// LocalDataStore data_store;
// Tpm2StatusImpl status;
// Tpm2InitializerImpl initializer(&data_store, &status);
// initializer.InitializeTpm();
// If the tpm is unowned, InitializeTpm injects random owner, endorsement and
// lockout passwords, intializes the SRK with empty authorization, and persists
// the passwords to disk until all the owner dependencies are satisfied.
class Tpm2InitializerImpl : public TpmInitializer {
 public:
  // Does not take ownership of |local_data_store| or |tpm_status|.
  Tpm2InitializerImpl(LocalDataStore* local_data_store,
                      TpmStatus* tpm_status);
  // Does not take ownership of |openssl_util|, |local_data_store| or
  // |tpm_status|. Takes ownership of |factory|.
  Tpm2InitializerImpl(trunks::TrunksFactory* factory,
                      OpensslCryptoUtil* openssl_util,
                      LocalDataStore* local_data_store,
                      TpmStatus* tpm_status);
  ~Tpm2InitializerImpl() override = default;

  // TpmInitializer methods.
  bool InitializeTpm() override;

 private:
  // Seeds the onboard Tpm random number generator with random bytes from
  // Openssl, if the Tpm RNG has not been seeded yet. Returns true on success.
  bool SeedTpmRng();

  // Gets random bytes of length |num_bytes| and populates the string at
  // |random_data|. Returns true on success.
  bool GetTpmRandomData(size_t num_bytes, std::string* random_data);

  std::unique_ptr<trunks::TrunksFactory> trunks_factory_;
  OpensslCryptoUtil* openssl_util_;
  LocalDataStore* local_data_store_;
  TpmStatus* tpm_status_;

  DISALLOW_COPY_AND_ASSIGN(Tpm2InitializerImpl);
};

}  // namespace tpm_manager

#endif  // TPM_MANAGER_SERVER_TPM2_INITIALIZER_IMPL_H_
