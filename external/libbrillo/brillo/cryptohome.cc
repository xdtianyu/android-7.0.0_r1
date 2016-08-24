// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brillo/cryptohome.h"

#include <openssl/sha.h>
#include <stdint.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <vector>

#include <base/files/file_util.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/stringprintf.h>

using base::FilePath;

namespace brillo {
namespace cryptohome {
namespace home {

const char kGuestUserName[] = "$guest";

static char g_user_home_prefix[PATH_MAX] = "/home/user/";
static char g_root_home_prefix[PATH_MAX] = "/home/root/";
static char g_system_salt_path[PATH_MAX] = "/home/.shadow/salt";

static std::string* salt = nullptr;

static bool EnsureSystemSaltIsLoaded() {
  if (salt && !salt->empty())
    return true;
  FilePath salt_path(g_system_salt_path);
  int64_t file_size;
  if (!base::GetFileSize(salt_path, &file_size)) {
    PLOG(ERROR) << "Could not get size of system salt: " << g_system_salt_path;
    return false;
  }
  if (file_size > static_cast<int64_t>(std::numeric_limits<int>::max())) {
    LOG(ERROR) << "System salt too large: " << file_size;
    return false;
  }
  std::vector<char> buf;
  buf.resize(file_size);
  unsigned int data_read = base::ReadFile(salt_path, buf.data(), file_size);
  if (data_read != file_size) {
    PLOG(ERROR) << "Could not read entire file: " << data_read
                << " != " << file_size;
    return false;
  }

  if (!salt)
    salt = new std::string();
  salt->assign(buf.data(), file_size);
  return true;
}

std::string SanitizeUserName(const std::string& username) {
  if (!EnsureSystemSaltIsLoaded())
    return std::string();

  unsigned char binmd[SHA_DIGEST_LENGTH];
  std::string lowercase(username);
  std::transform(
      lowercase.begin(), lowercase.end(), lowercase.begin(), ::tolower);
  SHA_CTX ctx;
  SHA1_Init(&ctx);
  SHA1_Update(&ctx, salt->data(), salt->size());
  SHA1_Update(&ctx, lowercase.data(), lowercase.size());
  SHA1_Final(binmd, &ctx);
  std::string final = base::HexEncode(binmd, sizeof(binmd));
  // Stay compatible with CryptoLib::HexEncodeToBuffer()
  std::transform(final.begin(), final.end(), final.begin(), ::tolower);
  return final;
}

FilePath GetUserPathPrefix() {
  return FilePath(g_user_home_prefix);
}

FilePath GetRootPathPrefix() {
  return FilePath(g_root_home_prefix);
}

FilePath GetHashedUserPath(const std::string& hashed_username) {
  return FilePath(
      base::StringPrintf("%s%s", g_user_home_prefix, hashed_username.c_str()));
}

FilePath GetUserPath(const std::string& username) {
  if (!EnsureSystemSaltIsLoaded())
    return FilePath("");
  return GetHashedUserPath(SanitizeUserName(username));
}

FilePath GetRootPath(const std::string& username) {
  if (!EnsureSystemSaltIsLoaded())
    return FilePath("");
  return FilePath(base::StringPrintf(
      "%s%s", g_root_home_prefix, SanitizeUserName(username).c_str()));
}

FilePath GetDaemonPath(const std::string& username, const std::string& daemon) {
  if (!EnsureSystemSaltIsLoaded())
    return FilePath("");
  return GetRootPath(username).Append(daemon);
}

bool IsSanitizedUserName(const std::string& sanitized) {
  std::vector<uint8_t> bytes;
  return (sanitized.length() == 2 * SHA_DIGEST_LENGTH) &&
         base::HexStringToBytes(sanitized, &bytes);
}

void SetUserHomePrefix(const std::string& prefix) {
  if (prefix.length() < sizeof(g_user_home_prefix)) {
    snprintf(
        g_user_home_prefix, sizeof(g_user_home_prefix), "%s", prefix.c_str());
  }
}

void SetRootHomePrefix(const std::string& prefix) {
  if (prefix.length() < sizeof(g_root_home_prefix)) {
    snprintf(
        g_root_home_prefix, sizeof(g_root_home_prefix), "%s", prefix.c_str());
  }
}

std::string* GetSystemSalt() {
  return salt;
}

void SetSystemSalt(std::string* value) {
  salt = value;
}

}  // namespace home
}  // namespace cryptohome
}  // namespace brillo
