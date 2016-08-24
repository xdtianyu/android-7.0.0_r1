// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brillo/userdb_utils.h"

#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include <vector>

#include <base/logging.h>

namespace brillo {
namespace userdb {

bool GetUserInfo(const std::string& user, uid_t* uid, gid_t* gid) {
  ssize_t buf_len = sysconf(_SC_GETPW_R_SIZE_MAX);
  if (buf_len < 0)
    buf_len = 16384;  // 16K should be enough?...
  passwd pwd_buf;
  passwd* pwd = nullptr;
  std::vector<char> buf(buf_len);
  if (getpwnam_r(user.c_str(), &pwd_buf, buf.data(), buf_len, &pwd) || !pwd) {
    PLOG(ERROR) << "Unable to find user " << user;
    return false;
  }

  if (uid)
    *uid = pwd->pw_uid;
  if (gid)
    *gid = pwd->pw_gid;
  return true;
}

bool GetGroupInfo(const std::string& group, gid_t* gid) {
  ssize_t buf_len = sysconf(_SC_GETGR_R_SIZE_MAX);
  if (buf_len < 0)
    buf_len = 16384;  // 16K should be enough?...
  struct group grp_buf;
  struct group* grp = nullptr;
  std::vector<char> buf(buf_len);
  if (getgrnam_r(group.c_str(), &grp_buf, buf.data(), buf_len, &grp) || !grp) {
    PLOG(ERROR) << "Unable to find group " << group;
    return false;
  }

  if (gid)
    *gid = grp->gr_gid;
  return true;
}

}  // namespace userdb
}  // namespace brillo
