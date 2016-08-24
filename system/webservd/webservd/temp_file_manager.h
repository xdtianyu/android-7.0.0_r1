// Copyright 2015 The Android Open Source Project
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

#ifndef WEBSERVER_WEBSERVD_TEMP_FILE_MANAGER_H_
#define WEBSERVER_WEBSERVD_TEMP_FILE_MANAGER_H_

#include <map>
#include <string>
#include <vector>

#include <base/files/file_path.h>
#include <base/macros.h>

namespace webservd {

// TempFileManager maintains life-times of temporary files associated with HTTP
// requests. Web server might require temporary storage to back certain large
// requests and this class is used to track those files and make sure that all
// the temporary files are deleted when the request is complete.
class TempFileManager {
 public:
  // FileSystemInterface allows to abstract the file system in tests.
  class FileDeleterInterface {
   public:
    virtual bool DeleteFile(const base::FilePath& path) = 0;

   protected:
    virtual ~FileDeleterInterface() = default;
  };

  TempFileManager(const base::FilePath& temp_dir_path,
                  FileDeleterInterface* file_deleter);
  ~TempFileManager();

  // Generate a new temporary file name for a request with unique ID
  // |request_id|. No actual file is created on the file system at this time.
  // The file name is registered with the request ID so it can be later deleted
  // when request is completed.
  base::FilePath CreateTempFileName(const std::string& request_id);

  // Deletes all the files belonging to the given request.
  void DeleteRequestTempFiles(const std::string& request_id);

 private:
  // Deletes all the files in the list.
  void DeleteFiles(const std::vector<base::FilePath>& files);

  // Root temp directory to store temporary files into.
  base::FilePath temp_dir_path_;
  // File system interface to abstract underlying file system for testing.
  FileDeleterInterface* file_deleter_;

  // List of files belonging to a particular request.
  std::map<std::string, std::vector<base::FilePath>> request_files_;

  DISALLOW_COPY_AND_ASSIGN(TempFileManager);
};

// Actual implementation of FileDeleterInterface to delete temporary files
// on the real file system.
class FileDeleter : public TempFileManager::FileDeleterInterface {
 public:
  bool DeleteFile(const base::FilePath& path) override;
};

}  // namespace webservd

#endif  // WEBSERVER_WEBSERVD_TEMP_FILE_MANAGER_H_
