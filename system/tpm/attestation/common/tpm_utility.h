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

#ifndef ATTESTATION_COMMON_TPM_UTILITY_H_
#define ATTESTATION_COMMON_TPM_UTILITY_H_

#include <string>

#include "attestation/common/interface.pb.h"

namespace attestation {

// A class which provides helpers for TPM-related tasks.
class TpmUtility {
 public:
  virtual ~TpmUtility() = default;

  // Returns true iff the TPM is enabled, owned, and ready for attestation.
  virtual bool IsTpmReady() = 0;

  // Activates an attestation identity key. Effectively this decrypts a
  // certificate or some other type of credential with the endorsement key. The
  // |delegate_blob| and |delegate_secret| must be authorized to activate with
  // owner privilege. The |identity_key_blob| is the key to which the credential
  // is bound. The |asym_ca_contents| and |sym_ca_attestation| parameters are
  // encrypted TPM structures, typically created by a CA (TPM_ASYM_CA_CONTENTS
  // and TPM_SYM_CA_ATTESTATION respectively). On success returns true and
  // populates the decrypted |credential|.
  virtual bool ActivateIdentity(const std::string& delegate_blob,
                                const std::string& delegate_secret,
                                const std::string& identity_key_blob,
                                const std::string& asym_ca_contents,
                                const std::string& sym_ca_attestation,
                                std::string* credential) = 0;

  // Generates and certifies a non-migratable key in the TPM. The new key will
  // correspond to |key_type| and |key_usage|. The parent key will be the
  // storage root key. The new key will be certified with the attestation
  // identity key represented by |identity_key_blob|. The |external_data| will
  // be included in the |key_info|. On success, returns true and populates
  // |public_key_tpm_format| with the public key of |key_blob| in TPM_PUBKEY
  // format, |key_info| with the TPM_CERTIFY_INFO that was signed, and |proof|
  // with the signature of |key_info| by the identity key.
  virtual bool CreateCertifiedKey(KeyType key_type,
                                  KeyUsage key_usage,
                                  const std::string& identity_key_blob,
                                  const std::string& external_data,
                                  std::string* key_blob,
                                  std::string* public_key,
                                  std::string* public_key_tpm_format,
                                  std::string* key_info,
                                  std::string* proof) = 0;

  // Seals |data| to the current value of PCR0 with the SRK and produces the
  // |sealed_data|. Returns true on success.
  virtual bool SealToPCR0(const std::string& data,
                          std::string* sealed_data) = 0;

  // Unseals |sealed_data| previously sealed with the SRK and produces the
  // unsealed |data|. Returns true on success.
  virtual bool Unseal(const std::string& sealed_data, std::string* data) = 0;

  // Reads the endorsement public key from the TPM.
  virtual bool GetEndorsementPublicKey(std::string* public_key) = 0;

  // Unbinds |bound_data| with the key loaded from |key_blob| by decrypting
  // using the TPM_ES_RSAESOAEP_SHA1_MGF1 scheme. The input must be in the
  // format of a TPM_BOUND_DATA structure. On success returns true and provides
  // the decrypted |data|.
  virtual bool Unbind(const std::string& key_blob,
                      const std::string& bound_data,
                      std::string* data) = 0;

  // Signs |data_to_sign| with the key loaded from |key_blob| using the
  // TPM_SS_RSASSAPKCS1v15_DER scheme with SHA-256. On success returns true and
  // provides the |signature|.
  virtual bool Sign(const std::string& key_blob,
                    const std::string& data_to_sign,
                    std::string* signature) = 0;
};

}  // namespace attestation

#endif  // ATTESTATION_COMMON_TPM_UTILITY_H_
