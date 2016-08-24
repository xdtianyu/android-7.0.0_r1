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

#include "tpm_manager/server/local_data_store_impl.h"

#include <fcntl.h>

#include <string>

#include <base/files/file_path.h>
#include <base/files/file_util.h>
#include <base/files/important_file_writer.h>

using base::FilePath;

namespace tpm_manager {

const char kTpmLocalDataFile[] = "/var/lib/tpm_manager/local_tpm_data";
const mode_t kLocalDataPermissions = 0600;

bool LocalDataStoreImpl::Read(LocalData* data) {
  CHECK(data);
  FilePath path(kTpmLocalDataFile);
  if (!base::PathExists(path)) {
    data->Clear();
    return true;
  }
  int permissions = 0;
  if (base::GetPosixFilePermissions(path, &permissions) &&
      (permissions & ~kLocalDataPermissions) != 0) {
    base::SetPosixFilePermissions(path, kLocalDataPermissions);
  }
  std::string file_data;
  if (!ReadFileToString(path, &file_data)) {
    LOG(ERROR) << "Error reading data store file.";
    return false;
  }
  if (!data->ParseFromString(file_data)) {
    LOG(ERROR) << "Error parsing file data into protobuf.";
    return false;
  }
  return true;
}

bool LocalDataStoreImpl::Write(const LocalData& data) {
  std::string file_data;
  if (!data.SerializeToString(&file_data)) {
    LOG(ERROR) << "Error serializing file to string.";
    return false;
  }
  FilePath path(kTpmLocalDataFile);
  if (!base::CreateDirectory(path.DirName())) {
    LOG(ERROR) << "Cannot create directory: " << path.DirName().value();
    return false;
  }
  if (!base::ImportantFileWriter::WriteFileAtomically(path, file_data)) {
    LOG(ERROR) << "Failed to write file: " << path.value();
    return false;
  }
  if (!base::SetPosixFilePermissions(path, kLocalDataPermissions)) {
    LOG(ERROR) << "Failed to set permissions for file: " << path.value();
    return false;
  }
  // Sync the parent directory.
  std::string dir_name = path.DirName().value();
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

}  // namespace tpm_manager
