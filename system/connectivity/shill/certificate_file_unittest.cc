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

#include "shill/certificate_file.h"

#include <string>
#include <vector>

#include <base/files/file_path.h>
#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <base/strings/stringprintf.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using base::FilePath;
using base::StringPrintf;
using std::string;
using std::vector;
using testing::_;
using testing::Return;
using testing::SetArgumentPointee;
using testing::StrEq;

namespace shill {

class CertificateFileTest : public testing::Test {
 public:
  CertificateFileTest() {}

  virtual void SetUp() {
    CHECK(temp_dir_.CreateUniqueTempDir());
    certificate_directory_ = temp_dir_.path().Append("certificates");
    certificate_file_.set_root_directory(certificate_directory_);
  }

 protected:
  static const char kDERData[];
  static const char kPEMData[];

  string ExtractHexData(const std::string& pem_data) {
    return CertificateFile::ExtractHexData(pem_data);
  }
  const FilePath& GetOutputFile() { return certificate_file_.output_file_; }
  const FilePath& GetRootDirectory() {
    return certificate_file_.root_directory_;
  }
  const char* GetPEMHeader() { return CertificateFile::kPEMHeader; }
  const char* GetPEMFooter() { return CertificateFile::kPEMFooter; }

  CertificateFile certificate_file_;
  base::ScopedTempDir temp_dir_;
  base::FilePath certificate_directory_;
};

const char CertificateFileTest::kDERData[] =
    "This does not have to be a real certificate "
    "since we are not testing its validity.";
const char CertificateFileTest::kPEMData[] =
    "VGhpcyBkb2VzIG5vdCBoYXZlIHRvIGJlIGEgcmVhbCBjZXJ0aWZpY2F0ZSBzaW5j\n"
    "ZSB3ZSBhcmUgbm90IHRlc3RpbmcgaXRzIHZhbGlkaXR5Lgo=\n";


TEST_F(CertificateFileTest, Construction) {
  EXPECT_TRUE(GetRootDirectory() == certificate_directory_);
  EXPECT_FALSE(base::PathExists(GetRootDirectory()));
  EXPECT_TRUE(GetOutputFile().empty());
}

TEST_F(CertificateFileTest, CreatePEMFromStrings) {
  // Create a formatted PEM file from the inner HEX data.
  const vector<string> kPEMVector0{ kPEMData };
  FilePath outfile0 = certificate_file_.CreatePEMFromStrings(kPEMVector0);
  EXPECT_FALSE(outfile0.empty());
  EXPECT_TRUE(base::PathExists(outfile0));
  EXPECT_TRUE(certificate_directory_.IsParent(outfile0));
  string file_string0;
  EXPECT_TRUE(base::ReadFileToString(outfile0, &file_string0));
  string expected_output0 = StringPrintf(
      "%s\n%s%s\n", GetPEMHeader(), kPEMData, GetPEMFooter());
  EXPECT_EQ(expected_output0, file_string0);

  // Create a formatted PEM file from formatted PEM.
  const vector<string> kPEMVector1{ expected_output0, kPEMData };
  FilePath outfile1 = certificate_file_.CreatePEMFromStrings(kPEMVector1);
  EXPECT_FALSE(outfile1.empty());
  EXPECT_TRUE(base::PathExists(outfile1));
  EXPECT_FALSE(base::PathExists(outfile0));  // Old file is deleted.
  string file_string1;
  EXPECT_TRUE(base::ReadFileToString(outfile1, &file_string1));
  string expected_output1 = StringPrintf(
      "%s%s", expected_output0.c_str(), expected_output0.c_str());
  EXPECT_EQ(expected_output1, file_string1);

  // Fail to create a PEM file.  Old file should not have been deleted.
  const vector<string> kPEMVector2{ kPEMData, "" };
  FilePath outfile2 = certificate_file_.CreatePEMFromStrings(kPEMVector2);
  EXPECT_TRUE(outfile2.empty());
  EXPECT_TRUE(base::PathExists(outfile1));
}

TEST_F(CertificateFileTest, ExtractHexData) {
  EXPECT_EQ("", ExtractHexData(""));
  EXPECT_EQ("foo\n", ExtractHexData("foo"));
  EXPECT_EQ("foo\nbar\n", ExtractHexData("foo\r\n\t\n bar\n"));
  EXPECT_EQ("", ExtractHexData(
      StringPrintf("%s\nfoo\nbar\n%s\n", GetPEMFooter(), GetPEMHeader())));
  EXPECT_EQ("", ExtractHexData(
      StringPrintf("%s\nfoo\nbar\n%s\n", GetPEMHeader(), GetPEMHeader())));
  EXPECT_EQ("", ExtractHexData(
      StringPrintf("%s\nfoo\nbar\n", GetPEMHeader())));
  EXPECT_EQ("", ExtractHexData(
      StringPrintf("foo\nbar\n%s\n", GetPEMFooter())));
  EXPECT_EQ("foo\nbar\n", ExtractHexData(
      StringPrintf("%s\nfoo\nbar\n%s\n", GetPEMHeader(), GetPEMFooter())));
  EXPECT_EQ("bar\n", ExtractHexData(
      StringPrintf("foo\n%s\nbar\n%s\nbaz\n", GetPEMHeader(), GetPEMFooter())));
}

TEST_F(CertificateFileTest, Destruction) {
  FilePath outfile;
  {
    CertificateFile certificate_file;
    certificate_file.set_root_directory(temp_dir_.path());
    outfile = certificate_file.CreatePEMFromStrings(vector<string>{ kPEMData });
    EXPECT_TRUE(base::PathExists(outfile));
  }
  // The output file should be deleted when certificate_file goes out-of-scope.
  EXPECT_FALSE(base::PathExists(outfile));
}

}  // namespace shill
