// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_FILE_UTILS_H_
#define LIBBRILLO_BRILLO_FILE_UTILS_H_

#include <sys/types.h>

#include <base/files/file_path.h>
#include <brillo/brillo_export.h>

namespace brillo {

// Ensures a regular file owned by user |uid| and group |gid| exists at |path|.
// Any other entity at |path| will be deleted and replaced with an empty
// regular file. If a new file is needed, any missing parent directories will
// be created, and the file will be assigned |new_file_permissions|.
// Should be safe to use in all directories, including tmpdirs with the sticky
// bit set.
// Returns true if the file existed or was able to be created.
BRILLO_EXPORT bool TouchFile(const base::FilePath& path,
                             int new_file_permissions,
                             uid_t uid,
                             gid_t gid);

// Convenience version of TouchFile() defaulting to 600 permissions and the
// current euid/egid.
// Should be safe to use in all directories, including tmpdirs with the sticky
// bit set.
BRILLO_EXPORT bool TouchFile(const base::FilePath& path);

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_FILE_UTILS_H_
