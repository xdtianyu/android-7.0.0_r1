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

#include "webservd/temp_file_manager.h"

#include <base/format_macros.h>
#include <base/files/file_util.h>
#include <base/strings/stringprintf.h>

namespace webservd {

TempFileManager::TempFileManager(const base::FilePath& temp_dir_path,
                                 FileDeleterInterface* file_deleter)
    : temp_dir_path_{temp_dir_path}, file_deleter_{file_deleter} {
}

TempFileManager::~TempFileManager() {
  for (const auto& pair : request_files_)
    DeleteFiles(pair.second);
}

base::FilePath TempFileManager::CreateTempFileName(
    const std::string& request_id) {
  std::vector<base::FilePath>& file_list_ref = request_files_[request_id];
  std::string name = base::StringPrintf("%s-%" PRIuS, request_id.c_str(),
                                        file_list_ref.size() + 1);
  base::FilePath file_name = temp_dir_path_.AppendASCII(name);
  file_list_ref.push_back(file_name);
  return file_name;
}

void TempFileManager::DeleteRequestTempFiles(const std::string& request_id) {
  auto iter = request_files_.find(request_id);
  if (iter == request_files_.end())
    return;
  DeleteFiles(iter->second);
  request_files_.erase(iter);
}

void TempFileManager::DeleteFiles(const std::vector<base::FilePath>& files) {
  for (const base::FilePath& file : files)
    CHECK(file_deleter_->DeleteFile(file));
}

bool FileDeleter::DeleteFile(const base::FilePath& path) {
  return base::DeleteFile(path, false);
}

}  // namespace webservd
