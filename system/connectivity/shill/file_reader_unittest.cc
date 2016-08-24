//
// Copyright (C) 2013 The Android Open Source Project
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
//

#include "shill/file_reader.h"

#include <string>
#include <vector>

#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <base/strings/string_util.h>
#include <gtest/gtest.h>

using base::FilePath;
using std::string;
using std::vector;

namespace shill {

class FileReaderTest : public ::testing::Test {
 public:
  void VerifyReadLines(const FilePath& path, const vector<string>& lines) {
    string line;
    EXPECT_FALSE(reader_.ReadLine(&line));
    EXPECT_TRUE(reader_.Open(path));
    for (size_t i = 0; i < lines.size(); ++i) {
      EXPECT_TRUE(reader_.ReadLine(&line));
      EXPECT_EQ(lines[i], line);
    }
    EXPECT_FALSE(reader_.ReadLine(&line));
    reader_.Close();
    EXPECT_FALSE(reader_.ReadLine(&line));
  }

 protected:
  FileReader reader_;
};

TEST_F(FileReaderTest, OpenNonExistentFile) {
  EXPECT_FALSE(reader_.Open(FilePath("a_nonexistent_file")));
}

TEST_F(FileReaderTest, OpenEmptyFile) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  FilePath path;
  ASSERT_TRUE(base::CreateTemporaryFileInDir(temp_dir.path(), &path));

  EXPECT_TRUE(reader_.Open(path));
  string line;
  EXPECT_FALSE(reader_.ReadLine(&line));
  reader_.Close();
}

TEST_F(FileReaderTest, ReadLine) {
  vector<string> lines;
  lines.push_back("this is");
  lines.push_back("a");
  lines.push_back("");
  lines.push_back("test");
  string content = base::JoinString(lines, "\n");

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  FilePath path;
  ASSERT_TRUE(base::CreateTemporaryFileInDir(temp_dir.path(), &path));

  // Test a file not ending with a new-line character
  ASSERT_EQ(content.size(),
            base::WriteFile(path, content.c_str(), content.size()));
  VerifyReadLines(path, lines);

  // Test a file ending with a new-line character
  content.push_back('\n');
  ASSERT_EQ(content.size(),
            base::WriteFile(path, content.c_str(), content.size()));
  VerifyReadLines(path, lines);
}

}  // namespace shill
