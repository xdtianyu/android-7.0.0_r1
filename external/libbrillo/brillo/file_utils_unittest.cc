// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brillo/file_utils.h"

#include <sys/stat.h>
#include <unistd.h>

#include <string>

#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <gtest/gtest.h>

namespace brillo {

class FileUtilsTest : public testing::Test {
 public:
  FileUtilsTest() {
    CHECK(temp_dir_.CreateUniqueTempDir());
    file_path_ = temp_dir_.path().Append("test.temp");
  }

 protected:
  base::FilePath file_path_;
  base::ScopedTempDir temp_dir_;

  // Writes |contents| to |file_path_|. Pulled into a separate function just
  // to improve readability of tests.
  void WriteFile(const std::string& contents) {
    EXPECT_EQ(contents.length(),
              base::WriteFile(file_path_, contents.c_str(), contents.length()));
  }

  // Verifies that the file at |file_path_| exists and contains |contents|.
  void ExpectFileContains(const std::string& contents) {
    EXPECT_TRUE(base::PathExists(file_path_));
    std::string new_contents;
    EXPECT_TRUE(base::ReadFileToString(file_path_, &new_contents));
    EXPECT_EQ(contents, new_contents);
  }

  // Verifies that the file at |file_path_| has |permissions|.
  void ExpectFilePermissions(int permissions) {
    int actual_permissions;
    EXPECT_TRUE(base::GetPosixFilePermissions(file_path_, &actual_permissions));
    EXPECT_EQ(permissions, actual_permissions);
  }
};

namespace {

enum {
  kPermissions600 =
      base::FILE_PERMISSION_READ_BY_USER | base::FILE_PERMISSION_WRITE_BY_USER,
  kPermissions700 = base::FILE_PERMISSION_USER_MASK,
  kPermissions777 = base::FILE_PERMISSION_MASK
};

}  // namespace

TEST_F(FileUtilsTest, TouchFileCreate) {
  EXPECT_TRUE(TouchFile(file_path_));
  ExpectFileContains("");
  ExpectFilePermissions(kPermissions600);
}

TEST_F(FileUtilsTest, TouchFileCreateThroughUmask) {
  mode_t old_umask = umask(kPermissions777);
  EXPECT_TRUE(TouchFile(file_path_));
  umask(old_umask);
  ExpectFileContains("");
  ExpectFilePermissions(kPermissions600);
}

TEST_F(FileUtilsTest, TouchFileCreateDirectoryStructure) {
  file_path_ = temp_dir_.path().Append("foo/bar/baz/test.temp");
  EXPECT_TRUE(TouchFile(file_path_));
  ExpectFileContains("");
}

TEST_F(FileUtilsTest, TouchFileExisting) {
  WriteFile("abcd");
  EXPECT_TRUE(TouchFile(file_path_));
  ExpectFileContains("abcd");
}

TEST_F(FileUtilsTest, TouchFileReplaceDirectory) {
  EXPECT_TRUE(base::CreateDirectory(file_path_));
  EXPECT_TRUE(TouchFile(file_path_));
  EXPECT_FALSE(base::DirectoryExists(file_path_));
  ExpectFileContains("");
}

TEST_F(FileUtilsTest, TouchFileReplaceSymlink) {
  base::FilePath symlink_target = temp_dir_.path().Append("target.temp");
  EXPECT_TRUE(base::CreateSymbolicLink(symlink_target, file_path_));
  EXPECT_TRUE(TouchFile(file_path_));
  EXPECT_FALSE(base::IsLink(file_path_));
  ExpectFileContains("");
}

TEST_F(FileUtilsTest, TouchFileReplaceOtherUser) {
  WriteFile("abcd");
  EXPECT_TRUE(TouchFile(file_path_, kPermissions777, geteuid() + 1, getegid()));
  ExpectFileContains("");
}

TEST_F(FileUtilsTest, TouchFileReplaceOtherGroup) {
  WriteFile("abcd");
  EXPECT_TRUE(TouchFile(file_path_, kPermissions777, geteuid(), getegid() + 1));
  ExpectFileContains("");
}

TEST_F(FileUtilsTest, TouchFileCreateWithAllPermissions) {
  EXPECT_TRUE(TouchFile(file_path_, kPermissions777, geteuid(), getegid()));
  ExpectFileContains("");
  ExpectFilePermissions(kPermissions777);
}

TEST_F(FileUtilsTest, TouchFileCreateWithOwnerPermissions) {
  EXPECT_TRUE(TouchFile(file_path_, kPermissions700, geteuid(), getegid()));
  ExpectFileContains("");
  ExpectFilePermissions(kPermissions700);
}

TEST_F(FileUtilsTest, TouchFileExistingPermissionsUnchanged) {
  EXPECT_TRUE(TouchFile(file_path_, kPermissions777, geteuid(), getegid()));
  EXPECT_TRUE(TouchFile(file_path_, kPermissions700, geteuid(), getegid()));
  ExpectFileContains("");
  ExpectFilePermissions(kPermissions777);
}

}  // namespace brillo
