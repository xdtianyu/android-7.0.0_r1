// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_USERDB_UTILS_H_
#define LIBBRILLO_BRILLO_USERDB_UTILS_H_

#include <sys/types.h>

#include <string>

#include <base/compiler_specific.h>
#include <base/macros.h>
#include <brillo/brillo_export.h>

namespace brillo {
namespace userdb {

// Looks up the UID and GID corresponding to |user|. Returns true on success.
// Passing nullptr for |uid| or |gid| causes them to be ignored.
BRILLO_EXPORT bool GetUserInfo(
    const std::string& user, uid_t* uid, gid_t* gid) WARN_UNUSED_RESULT;

// Looks up the GID corresponding to |group|. Returns true on success.
// Passing nullptr for |gid| causes it to be ignored.
BRILLO_EXPORT bool GetGroupInfo(
    const std::string& group, gid_t* gid) WARN_UNUSED_RESULT;

}  // namespace userdb
}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_USERDB_UTILS_H_
