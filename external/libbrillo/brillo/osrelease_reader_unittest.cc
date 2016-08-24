// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/osrelease_reader.h>

#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <gtest/gtest.h>

using std::string;

namespace brillo {

class OsReleaseReaderTest : public ::testing::Test {
 public:
  void SetUp() override {
    CHECK(temp_dir_.CreateUniqueTempDir());
    osreleased_ = temp_dir_.path().Append("etc").Append("os-release.d");
    osrelease_ = temp_dir_.path().Append("etc").Append("os-release");
    base::CreateDirectory(osreleased_);
  }

 protected:
  base::FilePath temp_file_, osrelease_, osreleased_;
  base::ScopedTempDir temp_dir_;
  OsReleaseReader store_;  // reader under test.
};

TEST_F(OsReleaseReaderTest, MissingOsReleaseTest) {
  store_.LoadTestingOnly(temp_dir_.path());
}

TEST_F(OsReleaseReaderTest, MissingOsReleaseDTest) {
  base::DeleteFile(osreleased_, true);
  store_.LoadTestingOnly(temp_dir_.path());
}

TEST_F(OsReleaseReaderTest, CompleteTest) {
  string hello = "hello";
  string ola = "ola";
  string bob = "bob";
  string osreleasecontent = "TEST_KEY=bonjour\nNAME=bob\n";

  base::WriteFile(osreleased_.Append("TEST_KEY"), hello.data(), hello.size());
  base::WriteFile(osreleased_.Append("GREETINGS"), ola.data(), ola.size());
  base::WriteFile(osrelease_, osreleasecontent.data(), osreleasecontent.size());

  store_.LoadTestingOnly(temp_dir_.path());

  string test_key_value;
  ASSERT_TRUE(store_.GetString("TEST_KEY", &test_key_value));

  string greetings_value;
  ASSERT_TRUE(store_.GetString("GREETINGS", &greetings_value));

  string name_value;
  ASSERT_TRUE(store_.GetString("NAME", &name_value));

  string nonexistent_value;
  // Getting the string should fail if the key does not exist.
  ASSERT_FALSE(store_.GetString("DOES_NOT_EXIST", &nonexistent_value));

  // hello in chosen (from os-release.d) instead of bonjour from os-release.
  ASSERT_EQ(hello, test_key_value);

  // greetings is set to ola.
  ASSERT_EQ(ola, greetings_value);

  // Name from os-release is set.
  ASSERT_EQ(bob, name_value);
}

TEST_F(OsReleaseReaderTest, NoNewLine) {
  // New lines should be stripped from os-release.d files.
  string hello = "hello\n";
  string bonjour = "bonjour\ngarbage";

  base::WriteFile(osreleased_.Append("HELLO"), hello.data(), hello.size());
  base::WriteFile(
      osreleased_.Append("BONJOUR"), bonjour.data(), bonjour.size());

  store_.LoadTestingOnly(temp_dir_.path());

  string hello_value;
  string bonjour_value;

  ASSERT_TRUE(store_.GetString("HELLO", &hello_value));
  ASSERT_TRUE(store_.GetString("BONJOUR", &bonjour_value));

  ASSERT_EQ("hello", hello_value);
  ASSERT_EQ("bonjour", bonjour_value);
}

}  // namespace brillo
