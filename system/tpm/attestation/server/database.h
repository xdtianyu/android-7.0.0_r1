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

#ifndef ATTESTATION_SERVER_DATABASE_H_
#define ATTESTATION_SERVER_DATABASE_H_

#include "attestation/common/database.pb.h"

namespace attestation {

// Manages a persistent database of attestation-related data.
class Database {
 public:
  virtual ~Database() = default;

  // Const access to the database protobuf.
  virtual const AttestationDatabase& GetProtobuf() const = 0;

  // Mutable access to the database protobuf. Changes made to the protobuf will
  // be reflected immediately by GetProtobuf() but will not be persisted to disk
  // until SaveChanges is called successfully.
  virtual AttestationDatabase* GetMutableProtobuf() = 0;

  // Writes the current database protobuf to disk.
  virtual bool SaveChanges() = 0;

  // Reloads the database protobuf from disk.
  virtual bool Reload() = 0;
};

}  // namespace attestation

#endif  // ATTESTATION_SERVER_DATABASE_H_
