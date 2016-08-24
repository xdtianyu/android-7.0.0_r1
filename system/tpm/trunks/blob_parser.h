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

#ifndef TRUNKS_BLOB_PARSER_H_
#define TRUNKS_BLOB_PARSER_H_

#include <string>

#include "trunks/tpm_generated.h"
#include "trunks/trunks_export.h"

namespace trunks {

class TRUNKS_EXPORT BlobParser {
 public:
  BlobParser() = default;
  virtual ~BlobParser() = default;

  // This method is used to construct a |key_blob| given the associated key's
  // TPM2B_PUBLIC and TPM2B_PRIVATE structs. Returns true on successful
  // serialization, else false.
  virtual bool SerializeKeyBlob(const TPM2B_PUBLIC& public_info,
                                const TPM2B_PRIVATE& private_info,
                                std::string* key_blob);

  // This method returns the Public and Private structs associated with a given
  // |key_blob|. Returns true on success, else false.
  virtual bool ParseKeyBlob(const std::string& key_blob,
                            TPM2B_PUBLIC* public_info,
                            TPM2B_PRIVATE* private_info);

  // This method is used to construct a |creation_blob| given the associated
  // key's |creation_data|, |creation_hash| and |creation_ticket| structs.
  // Returns true on successful serializtion, else false.
  virtual bool SerializeCreationBlob(const TPM2B_CREATION_DATA& creation_data,
                                     const TPM2B_DIGEST& creation_hash,
                                     const TPMT_TK_CREATION& creation_ticket,
                                     std::string* creation_blob);

  // This method returns the creation structures associated with a given
  // |creation_blob|. Returns true on success, else false.
  virtual bool ParseCreationBlob(const std::string& creation_blob,
                                 TPM2B_CREATION_DATA* creation_data,
                                 TPM2B_DIGEST* creation_hash,
                                 TPMT_TK_CREATION* creation_ticket);

 private:
  DISALLOW_COPY_AND_ASSIGN(BlobParser);
};

}  // namespace trunks

#endif  // TRUNKS_BLOB_PARSER_H_
