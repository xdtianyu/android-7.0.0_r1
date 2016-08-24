/*
 * Copyright (C) 2015, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "import_resolver.h"

#include <unistd.h>

#ifdef _WIN32
#include <io.h>
#endif

#include "os.h"

using std::string;
using std::vector;

namespace android {
namespace aidl {

ImportResolver::ImportResolver(const IoDelegate& io_delegate,
                               const vector<string>& import_paths)
    : io_delegate_(io_delegate) {
  for (string path : import_paths) {
    if (path.empty()) {
      path = ".";
    }
    if (path[path.size() - 1] != OS_PATH_SEPARATOR) {
      path += OS_PATH_SEPARATOR;
    }
    import_paths_.push_back(std::move(path));
  }
}


string ImportResolver::FindImportFile(const string& canonical_name) const {
  // Convert the canonical name to a relative file path.
  string relative_path = canonical_name;
  for (char& c : relative_path) {
    if (c == '.') {
      c = OS_PATH_SEPARATOR;
    }
  }
  relative_path += ".aidl";

  // Look for that relative path at each of our import roots.
  for (string path : import_paths_) {
    path = path + relative_path;
    if (io_delegate_.FileIsReadable(path)) {
      return path;
    }
  }

  return "";
}

}  // namespace android
}  // namespace aidl
