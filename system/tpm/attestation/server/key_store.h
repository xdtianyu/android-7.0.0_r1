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

#ifndef ATTESTATION_SERVER_KEY_STORE_H_
#define ATTESTATION_SERVER_KEY_STORE_H_

#include <string>

#include <base/macros.h>

#include "attestation/common/common.pb.h"

namespace attestation {

// A mock-able key storage interface.
class KeyStore {
 public:
  KeyStore() {}
  virtual ~KeyStore() {}

  // Reads key data from the store for the key identified by |key_label| and by
  // |username|. On success true is returned and |key_data| is populated.
  virtual bool Read(const std::string& username,
                    const std::string& key_label,
                    std::string* key_data) = 0;

  // Writes key data to the store for the key identified by |key_label| and by
  // |username|. If such a key already exists the existing data will be
  // overwritten.
  virtual bool Write(const std::string& username,
                     const std::string& key_label,
                     const std::string& key_data) = 0;

  // Deletes key data for the key identified by |key_label| and by |username|.
  // Returns false if key data exists but could not be deleted.
  virtual bool Delete(const std::string& username,
                      const std::string& key_label) = 0;

  // Deletes key data for all keys identified by |key_prefix| and by |username|
  // Returns false if key data exists but could not be deleted.
  virtual bool DeleteByPrefix(const std::string& username,
                              const std::string& key_prefix) = 0;

  // Registers a key to be associated with |username|.
  // The provided |label| will be associated with all registered objects.
  // |private_key_blob| holds the private key in some opaque format and
  // |public_key_der| holds the public key in PKCS #1 RSAPublicKey format.
  // If a non-empty |certificate| is provided it will be registered along with
  // the key. Returns true on success.
  virtual bool Register(const std::string& username,
                        const std::string& label,
                        KeyType key_type,
                        KeyUsage key_usage,
                        const std::string& private_key_blob,
                        const std::string& public_key_der,
                        const std::string& certificate) = 0;

  // Registers a |certificate| that is not associated to a registered key. The
  // certificate will be associated with |username|.
  virtual bool RegisterCertificate(const std::string& username,
                                   const std::string& certificate) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(KeyStore);
};

}  // namespace attestation

#endif  // ATTESTATION_SERVER_KEY_STORE_H_
