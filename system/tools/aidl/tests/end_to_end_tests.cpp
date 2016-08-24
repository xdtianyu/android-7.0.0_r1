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

#include <memory>
#include <string>
#include <vector>

#include <android-base/logging.h>
#include <gtest/gtest.h>

#include "aidl.h"
#include "options.h"
#include "tests/fake_io_delegate.h"
#include "tests/test_data.h"
#include "tests/test_util.h"

using android::aidl::test::CanonicalNameToPath;
using android::aidl::test::FakeIoDelegate;
using std::string;
using std::unique_ptr;
using std::vector;

namespace android {
namespace aidl {

class EndToEndTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
  }

  void AddStubAidls(const char** parcelables, const char** interfaces,
                    const char* cpp_header=nullptr) {
    for ( ; *parcelables; ++parcelables) {
      io_delegate_.AddStubParcelable(
          *parcelables, (cpp_header) ? cpp_header : "");
    }
    for ( ; *interfaces; ++interfaces) {
      io_delegate_.AddStubInterface(*interfaces);
    }
  }

  void CheckFileContents(const string& rel_path,
                         const string& expected_content) {
    string actual_content;
    ASSERT_TRUE(io_delegate_.GetWrittenContents(rel_path, &actual_content))
        << "Expected aidl to write to " << rel_path << " but it did not.";

    if (actual_content == expected_content) {
      return;  // success!
    }

    test::PrintDiff(expected_content, actual_content);
    FAIL() << "Actual contents of " << rel_path
           << " did not match expected content";
  }

  FakeIoDelegate io_delegate_;
};

TEST_F(EndToEndTest, IExampleInterface) {
  using namespace ::android::aidl::test_data::example_interface;

  JavaOptions options;
  options.fail_on_parcelable_ = true;
  options.import_paths_.push_back("");
  options.input_file_name_ = CanonicalNameToPath(kCanonicalName, ".aidl");
  options.output_file_name_ = kJavaOutputPath;
  options.dep_file_name_ = "an/arbitrary/path/to/deps.P";

  // Load up our fake file system with data.
  io_delegate_.SetFileContents(options.input_file_name_, kInterfaceDefinition);
  io_delegate_.AddCompoundParcelable("android.test.CompoundParcelable",
                                     {"Subclass1", "Subclass2"});
  AddStubAidls(kImportedParcelables, kImportedInterfaces);

  // Check that we parse correctly.
  EXPECT_EQ(android::aidl::compile_aidl_to_java(options, io_delegate_), 0);
  CheckFileContents(kJavaOutputPath, kExpectedJavaOutput);
  CheckFileContents(options.DependencyFilePath(), kExpectedJavaDepsOutput);
}

TEST_F(EndToEndTest, IPingResponderCpp) {
  using namespace ::android::aidl::test_data::ping_responder;

  const string input_path = CanonicalNameToPath(kCanonicalName, ".aidl");
  const string output_file = kCppOutputPath;
  const size_t argc = 6;
  const char* cmdline[argc + 1] = {
      "aidl-cpp", "-ddeps.P", "-I.", input_path.c_str(), kGenHeaderDir,
      output_file.c_str(), nullptr
  };
  auto options = CppOptions::Parse(argc, cmdline);

  // Set up input paths.
  io_delegate_.SetFileContents(input_path, kInterfaceDefinition);
  AddStubAidls(kImportedParcelables, kImportedInterfaces, kCppParcelableHeader);

  // Check that we parse and generate code correctly.
  EXPECT_EQ(android::aidl::compile_aidl_to_cpp(*options, io_delegate_), 0);
  CheckFileContents(output_file, kExpectedCppOutput);
  CheckFileContents(kGenInterfaceHeaderPath, kExpectedIHeaderOutput);
  CheckFileContents(kGenClientHeaderPath, kExpectedBpHeaderOutput);
  CheckFileContents(kGenServerHeaderPath, kExpectedBnHeaderOutput);
  CheckFileContents(options->DependencyFilePath(), kExpectedCppDepsOutput);
}

}  // namespace android
}  // namespace aidl
