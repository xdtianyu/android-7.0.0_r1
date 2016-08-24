// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstdint>
#include <cstdio>
#include <sysexits.h>

#include <base/command_line.h>
#include <base/macros.h>
#include <brillo/flag_helper.h>

#include <gtest/gtest.h>

namespace brillo {

class FlagHelperTest : public ::testing::Test {
 public:
  FlagHelperTest() {}
  ~FlagHelperTest() override { brillo::FlagHelper::ResetForTesting(); }
  static void SetUpTestCase() { base::CommandLine::Init(0, nullptr); }
};

// Test that the DEFINE_xxxx macros can create the respective variables
// correctly with the default value.
TEST_F(FlagHelperTest, Defaults) {
  DEFINE_bool(bool1, true, "Test bool flag");
  DEFINE_bool(bool2, false, "Test bool flag");
  DEFINE_int32(int32_1, INT32_MIN, "Test int32 flag");
  DEFINE_int32(int32_2, 0, "Test int32 flag");
  DEFINE_int32(int32_3, INT32_MAX, "Test int32 flag");
  DEFINE_int64(int64_1, INT64_MIN, "Test int64 flag");
  DEFINE_int64(int64_2, 0, "Test int64 flag");
  DEFINE_int64(int64_3, INT64_MAX, "Test int64 flag");
  DEFINE_uint64(uint64_1, 0, "Test uint64 flag");
  DEFINE_uint64(uint64_2, UINT_LEAST64_MAX, "Test uint64 flag");
  DEFINE_double(double_1, -100.5, "Test double flag");
  DEFINE_double(double_2, 0, "Test double flag");
  DEFINE_double(double_3, 100.5, "Test double flag");
  DEFINE_string(string_1, "", "Test string flag");
  DEFINE_string(string_2, "value", "Test string flag");

  const char* argv[] = {"test_program"};
  base::CommandLine command_line(arraysize(argv), argv);

  brillo::FlagHelper::GetInstance()->set_command_line_for_testing(
      &command_line);
  brillo::FlagHelper::Init(arraysize(argv), argv, "TestDefaultTrue");

  EXPECT_TRUE(FLAGS_bool1);
  EXPECT_FALSE(FLAGS_bool2);
  EXPECT_EQ(FLAGS_int32_1, INT32_MIN);
  EXPECT_EQ(FLAGS_int32_2, 0);
  EXPECT_EQ(FLAGS_int32_3, INT32_MAX);
  EXPECT_EQ(FLAGS_int64_1, INT64_MIN);
  EXPECT_EQ(FLAGS_int64_2, 0);
  EXPECT_EQ(FLAGS_int64_3, INT64_MAX);
  EXPECT_EQ(FLAGS_uint64_1, 0);
  EXPECT_EQ(FLAGS_uint64_2, UINT_LEAST64_MAX);
  EXPECT_DOUBLE_EQ(FLAGS_double_1, -100.5);
  EXPECT_DOUBLE_EQ(FLAGS_double_2, 0);
  EXPECT_DOUBLE_EQ(FLAGS_double_3, 100.5);
  EXPECT_STREQ(FLAGS_string_1.c_str(), "");
  EXPECT_STREQ(FLAGS_string_2.c_str(), "value");
}

// Test that command line flag values are parsed and update the flag
// variable values correctly when using double '--' flags
TEST_F(FlagHelperTest, SetValueDoubleDash) {
  DEFINE_bool(bool1, false, "Test bool flag");
  DEFINE_bool(bool2, true, "Test bool flag");
  DEFINE_bool(bool3, false, "Test bool flag");
  DEFINE_bool(bool4, true, "Test bool flag");
  DEFINE_int32(int32_1, 1, "Test int32 flag");
  DEFINE_int32(int32_2, 1, "Test int32 flag");
  DEFINE_int32(int32_3, 1, "Test int32 flag");
  DEFINE_int64(int64_1, 1, "Test int64 flag");
  DEFINE_int64(int64_2, 1, "Test int64 flag");
  DEFINE_int64(int64_3, 1, "Test int64 flag");
  DEFINE_uint64(uint64_1, 1, "Test uint64 flag");
  DEFINE_uint64(uint64_2, 1, "Test uint64 flag");
  DEFINE_double(double_1, 1, "Test double flag");
  DEFINE_double(double_2, 1, "Test double flag");
  DEFINE_double(double_3, 1, "Test double flag");
  DEFINE_string(string_1, "default", "Test string flag");
  DEFINE_string(string_2, "default", "Test string flag");

  const char* argv[] = {"test_program",
                        "--bool1",
                        "--nobool2",
                        "--bool3=true",
                        "--bool4=false",
                        "--int32_1=-2147483648",
                        "--int32_2=0",
                        "--int32_3=2147483647",
                        "--int64_1=-9223372036854775808",
                        "--int64_2=0",
                        "--int64_3=9223372036854775807",
                        "--uint64_1=0",
                        "--uint64_2=18446744073709551615",
                        "--double_1=-100.5",
                        "--double_2=0",
                        "--double_3=100.5",
                        "--string_1=",
                        "--string_2=value"};
  base::CommandLine command_line(arraysize(argv), argv);

  brillo::FlagHelper::GetInstance()->set_command_line_for_testing(
      &command_line);
  brillo::FlagHelper::Init(arraysize(argv), argv, "TestDefaultTrue");

  EXPECT_TRUE(FLAGS_bool1);
  EXPECT_FALSE(FLAGS_bool2);
  EXPECT_TRUE(FLAGS_bool3);
  EXPECT_FALSE(FLAGS_bool4);
  EXPECT_EQ(FLAGS_int32_1, INT32_MIN);
  EXPECT_EQ(FLAGS_int32_2, 0);
  EXPECT_EQ(FLAGS_int32_3, INT32_MAX);
  EXPECT_EQ(FLAGS_int64_1, INT64_MIN);
  EXPECT_EQ(FLAGS_int64_2, 0);
  EXPECT_EQ(FLAGS_int64_3, INT64_MAX);
  EXPECT_EQ(FLAGS_uint64_1, 0);
  EXPECT_EQ(FLAGS_uint64_2, UINT_LEAST64_MAX);
  EXPECT_DOUBLE_EQ(FLAGS_double_1, -100.5);
  EXPECT_DOUBLE_EQ(FLAGS_double_2, 0);
  EXPECT_DOUBLE_EQ(FLAGS_double_3, 100.5);
  EXPECT_STREQ(FLAGS_string_1.c_str(), "");
  EXPECT_STREQ(FLAGS_string_2.c_str(), "value");
}

// Test that command line flag values are parsed and update the flag
// variable values correctly when using single '-' flags
TEST_F(FlagHelperTest, SetValueSingleDash) {
  DEFINE_bool(bool1, false, "Test bool flag");
  DEFINE_bool(bool2, true, "Test bool flag");
  DEFINE_int32(int32_1, 1, "Test int32 flag");
  DEFINE_int32(int32_2, 1, "Test int32 flag");
  DEFINE_int32(int32_3, 1, "Test int32 flag");
  DEFINE_int64(int64_1, 1, "Test int64 flag");
  DEFINE_int64(int64_2, 1, "Test int64 flag");
  DEFINE_int64(int64_3, 1, "Test int64 flag");
  DEFINE_uint64(uint64_1, 1, "Test uint64 flag");
  DEFINE_uint64(uint64_2, 1, "Test uint64 flag");
  DEFINE_double(double_1, 1, "Test double flag");
  DEFINE_double(double_2, 1, "Test double flag");
  DEFINE_double(double_3, 1, "Test double flag");
  DEFINE_string(string_1, "default", "Test string flag");
  DEFINE_string(string_2, "default", "Test string flag");

  const char* argv[] = {"test_program",
                        "-bool1",
                        "-nobool2",
                        "-int32_1=-2147483648",
                        "-int32_2=0",
                        "-int32_3=2147483647",
                        "-int64_1=-9223372036854775808",
                        "-int64_2=0",
                        "-int64_3=9223372036854775807",
                        "-uint64_1=0",
                        "-uint64_2=18446744073709551615",
                        "-double_1=-100.5",
                        "-double_2=0",
                        "-double_3=100.5",
                        "-string_1=",
                        "-string_2=value"};
  base::CommandLine command_line(arraysize(argv), argv);

  brillo::FlagHelper::GetInstance()->set_command_line_for_testing(
      &command_line);
  brillo::FlagHelper::Init(arraysize(argv), argv, "TestDefaultTrue");

  EXPECT_TRUE(FLAGS_bool1);
  EXPECT_FALSE(FLAGS_bool2);
  EXPECT_EQ(FLAGS_int32_1, INT32_MIN);
  EXPECT_EQ(FLAGS_int32_2, 0);
  EXPECT_EQ(FLAGS_int32_3, INT32_MAX);
  EXPECT_EQ(FLAGS_int64_1, INT64_MIN);
  EXPECT_EQ(FLAGS_int64_2, 0);
  EXPECT_EQ(FLAGS_int64_3, INT64_MAX);
  EXPECT_EQ(FLAGS_uint64_1, 0);
  EXPECT_EQ(FLAGS_uint64_2, UINT_LEAST64_MAX);
  EXPECT_DOUBLE_EQ(FLAGS_double_1, -100.5);
  EXPECT_DOUBLE_EQ(FLAGS_double_2, 0);
  EXPECT_DOUBLE_EQ(FLAGS_double_3, 100.5);
  EXPECT_STREQ(FLAGS_string_1.c_str(), "");
  EXPECT_STREQ(FLAGS_string_2.c_str(), "value");
}

// Test that a duplicated flag on the command line picks up the last
// value set.
TEST_F(FlagHelperTest, DuplicateSetValue) {
  DEFINE_int32(int32_1, 0, "Test in32 flag");

  const char* argv[] = {"test_program", "--int32_1=5", "--int32_1=10"};
  base::CommandLine command_line(arraysize(argv), argv);

  brillo::FlagHelper::GetInstance()->set_command_line_for_testing(
      &command_line);
  brillo::FlagHelper::Init(arraysize(argv), argv, "TestDuplicateSetvalue");

  EXPECT_EQ(FLAGS_int32_1, 10);
}

// Test that flags set after the -- marker are not parsed as command line flags
TEST_F(FlagHelperTest, FlagTerminator) {
  DEFINE_int32(int32_1, 0, "Test int32 flag");

  const char* argv[] = {"test_program", "--int32_1=5", "--", "--int32_1=10"};
  base::CommandLine command_line(arraysize(argv), argv);

  brillo::FlagHelper::GetInstance()->set_command_line_for_testing(
      &command_line);
  brillo::FlagHelper::Init(arraysize(argv), argv, "TestFlagTerminator");

  EXPECT_EQ(FLAGS_int32_1, 5);
}

// Test that help messages are generated correctly when the --help flag
// is passed to the program.
TEST_F(FlagHelperTest, HelpMessage) {
  DEFINE_bool(bool_1, true, "Test bool flag");
  DEFINE_int32(int_1, 0, "Test int flag");
  DEFINE_int64(int64_1, 0, "Test int64 flag");
  DEFINE_uint64(uint64_1, 0, "Test uint64 flag");
  DEFINE_double(double_1, 0, "Test double flag");
  DEFINE_string(string_1, "", "Test string flag");

  const char* argv[] = {"test_program", "--int_1=value", "--help"};
  base::CommandLine command_line(arraysize(argv), argv);

  brillo::FlagHelper::GetInstance()->set_command_line_for_testing(
      &command_line);

  FILE* orig = stdout;
  stdout = stderr;

  ASSERT_EXIT(
      brillo::FlagHelper::Init(arraysize(argv), argv, "TestHelpMessage"),
      ::testing::ExitedWithCode(EX_OK),
      "TestHelpMessage\n\n"
      "  --bool_1  \\(Test bool flag\\)  type: bool  default: true\n"
      "  --double_1  \\(Test double flag\\)  type: double  default: 0\n"
      "  --help  \\(Show this help message\\)  type: bool  default: false\n"
      "  --int64_1  \\(Test int64 flag\\)  type: int64  default: 0\n"
      "  --int_1  \\(Test int flag\\)  type: int  default: 0\n"
      "  --string_1  \\(Test string flag\\)  type: string  default: \"\"\n"
      "  --uint64_1  \\(Test uint64 flag\\)  type: uint64  default: 0\n");

  stdout = orig;
}

// Test that passing in unknown command line flags causes the program
// to exit with EX_USAGE error code and corresponding error message.
TEST_F(FlagHelperTest, UnknownFlag) {
  const char* argv[] = {"test_program", "--flag=value"};
  base::CommandLine command_line(arraysize(argv), argv);

  brillo::FlagHelper::GetInstance()->set_command_line_for_testing(
      &command_line);

  FILE* orig = stdout;
  stdout = stderr;

  ASSERT_EXIT(brillo::FlagHelper::Init(arraysize(argv), argv, "TestIntExit"),
              ::testing::ExitedWithCode(EX_USAGE),
              "ERROR: unknown command line flag 'flag'");

  stdout = orig;
}

// Test that when passing an incorrect/unparsable type to a command line flag,
// the program exits with code EX_DATAERR and outputs a corresponding message.
TEST_F(FlagHelperTest, BoolParseError) {
  DEFINE_bool(bool_1, 0, "Test bool flag");

  const char* argv[] = {"test_program", "--bool_1=value"};
  base::CommandLine command_line(arraysize(argv), argv);

  brillo::FlagHelper::GetInstance()->set_command_line_for_testing(
      &command_line);

  FILE* orig = stdout;
  stdout = stderr;

  ASSERT_EXIT(
      brillo::FlagHelper::Init(arraysize(argv), argv, "TestBoolParseError"),
      ::testing::ExitedWithCode(EX_DATAERR),
      "ERROR: illegal value 'value' specified for bool flag 'bool_1'");

  stdout = orig;
}

// Test that when passing an incorrect/unparsable type to a command line flag,
// the program exits with code EX_DATAERR and outputs a corresponding message.
TEST_F(FlagHelperTest, Int32ParseError) {
  DEFINE_int32(int_1, 0, "Test int flag");

  const char* argv[] = {"test_program", "--int_1=value"};
  base::CommandLine command_line(arraysize(argv), argv);

  brillo::FlagHelper::GetInstance()->set_command_line_for_testing(
      &command_line);

  FILE* orig = stdout;
  stdout = stderr;

  ASSERT_EXIT(brillo::FlagHelper::Init(arraysize(argv),
                                       argv,
                                       "TestInt32ParseError"),
              ::testing::ExitedWithCode(EX_DATAERR),
              "ERROR: illegal value 'value' specified for int flag 'int_1'");

  stdout = orig;
}

// Test that when passing an incorrect/unparsable type to a command line flag,
// the program exits with code EX_DATAERR and outputs a corresponding message.
TEST_F(FlagHelperTest, Int64ParseError) {
  DEFINE_int64(int64_1, 0, "Test int64 flag");

  const char* argv[] = {"test_program", "--int64_1=value"};
  base::CommandLine command_line(arraysize(argv), argv);

  brillo::FlagHelper::GetInstance()->set_command_line_for_testing(
      &command_line);

  FILE* orig = stdout;
  stdout = stderr;

  ASSERT_EXIT(
      brillo::FlagHelper::Init(arraysize(argv), argv, "TestInt64ParseError"),
      ::testing::ExitedWithCode(EX_DATAERR),
      "ERROR: illegal value 'value' specified for int64 flag "
      "'int64_1'");

  stdout = orig;
}

// Test that when passing an incorrect/unparsable type to a command line flag,
// the program exits with code EX_DATAERR and outputs a corresponding message.
TEST_F(FlagHelperTest, UInt64ParseError) {
  DEFINE_uint64(uint64_1, 0, "Test uint64 flag");

  const char* argv[] = {"test_program", "--uint64_1=value"};
  base::CommandLine command_line(arraysize(argv), argv);

  brillo::FlagHelper::GetInstance()->set_command_line_for_testing(
      &command_line);

  FILE* orig = stdout;
  stdout = stderr;

  ASSERT_EXIT(
      brillo::FlagHelper::Init(arraysize(argv), argv, "TestUInt64ParseError"),
      ::testing::ExitedWithCode(EX_DATAERR),
      "ERROR: illegal value 'value' specified for uint64 flag "
      "'uint64_1'");

  stdout = orig;
}

}  // namespace brillo
