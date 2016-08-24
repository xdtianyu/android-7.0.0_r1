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

#include "attestation/server/database_impl.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>

#include <base/files/file_path.h>
#include <base/files/file_util.h>
#include <base/files/important_file_writer.h>
#include <base/logging.h>
#include <base/stl_util.h>
#include <brillo/secure_blob.h>

using base::FilePath;

namespace {

const char kDatabasePath[] =
    "/mnt/stateful_partition/unencrypted/preserve/attestation.epb";
const mode_t kDatabasePermissions = 0600;

// A base::FilePathWatcher::Callback that just relays to |callback|.
void FileWatcherCallback(const base::Closure& callback, const FilePath&, bool) {
  callback.Run();
}

}  // namespace

namespace attestation {

DatabaseImpl::DatabaseImpl(CryptoUtility* crypto) : io_(this), crypto_(crypto) {
}

DatabaseImpl::~DatabaseImpl() {
  brillo::SecureMemset(string_as_array(&database_key_), 0,
                       database_key_.size());
}

void DatabaseImpl::Initialize() {
  // Start thread-checking now.
  thread_checker_.DetachFromThread();
  DCHECK(thread_checker_.CalledOnValidThread());
  io_->Watch(base::Bind(base::IgnoreResult(&DatabaseImpl::Reload),
                        base::Unretained(this)));
  if (!Reload()) {
    LOG(WARNING) << "Creating new attestation database.";
  }
}

const AttestationDatabase& DatabaseImpl::GetProtobuf() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return protobuf_;
}

AttestationDatabase* DatabaseImpl::GetMutableProtobuf() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return &protobuf_;
}

bool DatabaseImpl::SaveChanges() {
  DCHECK(thread_checker_.CalledOnValidThread());
  std::string buffer;
  if (!EncryptProtobuf(&buffer)) {
    return false;
  }
  return io_->Write(buffer);
}

bool DatabaseImpl::Reload() {
  DCHECK(thread_checker_.CalledOnValidThread());
  LOG(INFO) << "Loading attestation database.";
  std::string buffer;
  if (!io_->Read(&buffer)) {
    return false;
  }
  return DecryptProtobuf(buffer);
}

bool DatabaseImpl::Read(std::string* data) {
  const int kMask = base::FILE_PERMISSION_OTHERS_MASK;
  FilePath path(kDatabasePath);
  int permissions = 0;
  if (base::GetPosixFilePermissions(path, &permissions) &&
      (permissions & kMask) != 0) {
    LOG(WARNING) << "Attempting to fix permissions on attestation database.";
    base::SetPosixFilePermissions(path, permissions & ~kMask);
  }
  if (!base::ReadFileToString(path, data)) {
    PLOG(ERROR) << "Failed to read attestation database";
    return false;
  }
  return true;
}

bool DatabaseImpl::Write(const std::string& data) {
  FilePath file_path(kDatabasePath);
  if (!base::CreateDirectory(file_path.DirName())) {
    LOG(ERROR) << "Cannot create directory: " << file_path.DirName().value();
    return false;
  }
  if (!base::ImportantFileWriter::WriteFileAtomically(file_path, data)) {
    LOG(ERROR) << "Failed to write file: " << file_path.value();
    return false;
  }
  if (!base::SetPosixFilePermissions(file_path, kDatabasePermissions)) {
    LOG(ERROR) << "Failed to set permissions for file: " << file_path.value();
    return false;
  }
  // Sync the parent directory.
  std::string dir_name = file_path.DirName().value();
  int dir_fd = HANDLE_EINTR(open(dir_name.c_str(), O_RDONLY|O_DIRECTORY));
  if (dir_fd < 0) {
    PLOG(WARNING) << "Could not open " << dir_name << " for syncing";
    return false;
  }
  // POSIX specifies EINTR as a possible return value of fsync().
  int result = HANDLE_EINTR(fsync(dir_fd));
  if (result < 0) {
    PLOG(WARNING) << "Failed to sync " << dir_name;
    close(dir_fd);
    return false;
  }
  // close() may not be retried on error.
  result = IGNORE_EINTR(close(dir_fd));
  if (result < 0) {
    PLOG(WARNING) << "Failed to close after sync " << dir_name;
    return false;
  }
  return true;
}

void DatabaseImpl::Watch(const base::Closure& callback) {
  if (!file_watcher_) {
    file_watcher_.reset(new base::FilePathWatcher());
    file_watcher_->Watch(FilePath(kDatabasePath), false,
                         base::Bind(&FileWatcherCallback, callback));
  }
}

bool DatabaseImpl::EncryptProtobuf(std::string* encrypted_output) {
  std::string serial_proto;
  if (!protobuf_.SerializeToString(&serial_proto)) {
    LOG(ERROR) << "Failed to serialize db.";
    return false;
  }
  if (database_key_.empty() || sealed_database_key_.empty()) {
    if (!crypto_->CreateSealedKey(&database_key_, &sealed_database_key_)) {
      LOG(ERROR) << "Failed to generate database key.";
      return false;
    }
  }
  if (!crypto_->EncryptData(serial_proto, database_key_, sealed_database_key_,
                            encrypted_output)) {
    LOG(ERROR) << "Attestation: Failed to encrypt database.";
    return false;
  }
  return true;
}

bool DatabaseImpl::DecryptProtobuf(const std::string& encrypted_input) {
  if (!crypto_->UnsealKey(encrypted_input, &database_key_,
                          &sealed_database_key_)) {
    LOG(ERROR) << "Attestation: Could not unseal decryption key.";
    return false;
  }
  std::string serial_proto;
  if (!crypto_->DecryptData(encrypted_input, database_key_, &serial_proto)) {
    LOG(ERROR) << "Attestation: Failed to decrypt database.";
    return false;
  }
  if (!protobuf_.ParseFromString(serial_proto)) {
    // Previously the DB was encrypted with CryptoLib::AesEncrypt which appends
    // a SHA-1.  This can be safely ignored.
    const size_t kLegacyJunkSize = 20;
    if (serial_proto.size() < kLegacyJunkSize ||
        !protobuf_.ParseFromArray(serial_proto.data(),
                                  serial_proto.length() - kLegacyJunkSize)) {
      LOG(ERROR) << "Failed to parse database.";
      return false;
    }
  }
  return true;
}

}  // namespace attestation
