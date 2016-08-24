// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/osrelease_reader.h>

#include <base/files/file_enumerator.h>
#include <base/files/file_util.h>
#include <base/logging.h>
#include <brillo/strings/string_utils.h>

namespace brillo {

void OsReleaseReader::Load() {
  Load(base::FilePath("/"));
}

bool OsReleaseReader::GetString(const std::string& key,
                                std::string* value) const {
  CHECK(initialized_) << "OsReleaseReader.Load() must be called first.";
  return store_.GetString(key, value);
}

void OsReleaseReader::LoadTestingOnly(const base::FilePath& root_dir) {
  Load(root_dir);
}

void OsReleaseReader::Load(const base::FilePath& root_dir) {
  base::FilePath osrelease = root_dir.Append("etc").Append("os-release");
  if (!store_.Load(osrelease)) {
    // /etc/os-release might not be present (cros deploying a new configuration
    // or no fields set at all). Just print a debug message and continue.
    DLOG(INFO) << "Could not load fields from " << osrelease.value();
  }

  base::FilePath osreleased = root_dir.Append("etc").Append("os-release.d");
  base::FileEnumerator enumerator(
      osreleased, false, base::FileEnumerator::FILES);

  for (base::FilePath path = enumerator.Next(); !path.empty();
       path = enumerator.Next()) {
    std::string content;
    if (!base::ReadFileToString(path, &content)) {
      // The only way to fail is if a file exist in /etc/os-release.d but we
      // cannot read it.
      PLOG(FATAL) << "Could not read " << path.value();
    }
    // There might be a trailing new line. Strip it to keep only the first line
    // of the file.
    content = brillo::string_utils::SplitAtFirst(content, "\n", true).first;
    store_.SetString(path.BaseName().value(), content);
  }
  initialized_ = true;
}

}  // namespace brillo
