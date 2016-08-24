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

#ifndef ATTESTATION_SERVER_DATABASE_IMPL_H_
#define ATTESTATION_SERVER_DATABASE_IMPL_H_

#include "attestation/server/database.h"

#include <string>

#include <base/callback_forward.h>
#include <base/files/file_path_watcher.h>
#include <base/threading/thread_checker.h>

#include "attestation/common/crypto_utility.h"

namespace attestation {

// An I/O abstraction to help with testing.
class DatabaseIO {
 public:
  // Reads the persistent database blob.
  virtual bool Read(std::string* data) = 0;
  // Writes the persistent database blob.
  virtual bool Write(const std::string& data) = 0;
  // Watch for external changes to the database.
  virtual void Watch(const base::Closure& callback) = 0;
};

// An implementation of Database backed by an ordinary file. Not thread safe.
// All methods must be called on the same thread as the Initialize() call.
class DatabaseImpl : public Database,
                     public DatabaseIO {
 public:
  // Does not take ownership of pointers.
  explicit DatabaseImpl(CryptoUtility* crypto);
  ~DatabaseImpl() override;

  // Reads and decrypts any existing database on disk synchronously. Must be
  // called before calling other methods.
  void Initialize();

  // Database methods.
  const AttestationDatabase& GetProtobuf() const override;
  AttestationDatabase* GetMutableProtobuf() override;
  bool SaveChanges() override;
  bool Reload() override;

  // DatabaseIO methods.
  bool Read(std::string* data) override;
  bool Write(const std::string& data) override;
  void Watch(const base::Closure& callback) override;

  // Useful for testing.
  void set_io(DatabaseIO* io) {
    io_ = io;
  }

 private:
  // Encrypts |protobuf_| into |encrypted_output|. Returns true on success.
  bool EncryptProtobuf(std::string* encrypted_output);

  // Decrypts |encrypted_input| as output by EncryptProtobuf into |protobuf_|.
  // Returns true on success.
  bool DecryptProtobuf(const std::string& encrypted_input);

  AttestationDatabase protobuf_;
  DatabaseIO* io_;
  CryptoUtility* crypto_;
  std::string database_key_;
  std::string sealed_database_key_;
  std::unique_ptr<base::FilePathWatcher> file_watcher_;
  base::ThreadChecker thread_checker_;
};

}  // namespace attestation

#endif  // ATTESTATION_SERVER_DATABASE_IMPL_H_
