// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brillo/process.h"

#include <unistd.h>

#include <base/files/file_path.h>
#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <gtest/gtest.h>

#include "brillo/process_mock.h"
#include "brillo/unittest_utils.h"
#include "brillo/test_helpers.h"

using base::FilePath;

// This test assumes the following standard binaries are installed.
#if defined(__ANDROID__)
# define SYSTEM_PREFIX "/system"
static const char kBinStat[] = SYSTEM_PREFIX "/bin/stat";
#else
# define SYSTEM_PREFIX ""
static const char kBinStat[] = "/usr/bin/stat";
#endif

static const char kBinSh[] = SYSTEM_PREFIX "/bin/sh";
static const char kBinCat[] = SYSTEM_PREFIX "/bin/cat";
static const char kBinCp[] = SYSTEM_PREFIX "/bin/cp";
static const char kBinEcho[] = SYSTEM_PREFIX "/bin/echo";
static const char kBinFalse[] = SYSTEM_PREFIX "/bin/false";
static const char kBinSleep[] = SYSTEM_PREFIX "/bin/sleep";
static const char kBinTrue[] = SYSTEM_PREFIX "/bin/true";

namespace brillo {

// Test that the mock has all the functions of the interface by
// instantiating it.  This variable is not used elsewhere.
struct CompileMocks {
  ProcessMock process_mock;
};

TEST(SimpleProcess, Basic) {
  // Log must be cleared before running this test, just as ProcessTest::SetUp.
  ClearLog();
  ProcessImpl process;
  process.AddArg(kBinEcho);
  EXPECT_EQ(0, process.Run());
  EXPECT_EQ("", GetLog());
}

TEST(SimpleProcess, NoSearchPath) {
  ProcessImpl process;
  process.AddArg("echo");
  EXPECT_EQ(127, process.Run());
}

TEST(SimpleProcess, SearchPath) {
  ProcessImpl process;
  process.AddArg("echo");
  process.SetSearchPath(true);
  EXPECT_EQ(EXIT_SUCCESS, process.Run());
}

TEST(SimpleProcess, BindFd) {
  int fds[2];
  char buf[16];
  static const char* kMsg = "hello, world!";
  ProcessImpl process;
  EXPECT_EQ(0, pipe(fds));
  process.AddArg(kBinEcho);
  process.AddArg(kMsg);
  process.BindFd(fds[1], 1);
  process.Run();
  memset(buf, 0, sizeof(buf));
  EXPECT_EQ(read(fds[0], buf, sizeof(buf) - 1), strlen(kMsg) + 1);
  EXPECT_EQ(std::string(kMsg) + "\n", std::string(buf));
}

class ProcessTest : public ::testing::Test {
 public:
  void SetUp() {
    CHECK(temp_dir_.CreateUniqueTempDir());
    output_file_ = temp_dir_.path().Append("fork_out").value();
    process_.RedirectOutput(output_file_);
    ClearLog();
  }

  static void SetUpTestCase() {
    base::CommandLine::Init(0, nullptr);
    ::brillo::InitLog(brillo::kLogToStderr);
    ::brillo::LogToString(true);
  }

 protected:
  void CheckStderrCaptured();
  FilePath GetFdPath(int fd);

  ProcessImpl process_;
  std::vector<const char*> args_;
  std::string output_file_;
  base::ScopedTempDir temp_dir_;
};

TEST_F(ProcessTest, Basic) {
  process_.AddArg(kBinEcho);
  process_.AddArg("hello world");
  EXPECT_EQ(0, process_.Run());
  ExpectFileEquals("hello world\n", output_file_.c_str());
  EXPECT_EQ("", GetLog());
}

TEST_F(ProcessTest, AddStringOption) {
  process_.AddArg(kBinEcho);
  process_.AddStringOption("--hello", "world");
  EXPECT_EQ(0, process_.Run());
  ExpectFileEquals("--hello world\n", output_file_.c_str());
}

TEST_F(ProcessTest, AddIntValue) {
  process_.AddArg(kBinEcho);
  process_.AddIntOption("--answer", 42);
  EXPECT_EQ(0, process_.Run());
  ExpectFileEquals("--answer 42\n", output_file_.c_str());
}

TEST_F(ProcessTest, NonZeroReturnValue) {
  process_.AddArg(kBinFalse);
  EXPECT_EQ(1, process_.Run());
  ExpectFileEquals("", output_file_.c_str());
  EXPECT_EQ("", GetLog());
}

TEST_F(ProcessTest, BadOutputFile) {
  process_.AddArg(kBinEcho);
  process_.RedirectOutput("/bad/path");
  EXPECT_EQ(static_cast<pid_t>(Process::kErrorExitStatus), process_.Run());
}

TEST_F(ProcessTest, BadExecutable) {
  process_.AddArg("false");
  EXPECT_EQ(static_cast<pid_t>(Process::kErrorExitStatus), process_.Run());
}

void ProcessTest::CheckStderrCaptured() {
  std::string contents;
  process_.AddArg(kBinSh);
  process_.AddArg("-c");
  process_.AddArg("echo errormessage 1>&2 && exit 1");
  EXPECT_EQ(1, process_.Run());
  EXPECT_TRUE(base::ReadFileToString(FilePath(output_file_), &contents));
  EXPECT_NE(std::string::npos, contents.find("errormessage"));
  EXPECT_EQ("", GetLog());
}

TEST_F(ProcessTest, StderrCaptured) {
  CheckStderrCaptured();
}

TEST_F(ProcessTest, StderrCapturedWhenPreviouslyClosed) {
  int saved_stderr = dup(STDERR_FILENO);
  close(STDERR_FILENO);
  CheckStderrCaptured();
  dup2(saved_stderr, STDERR_FILENO);
}

FilePath ProcessTest::GetFdPath(int fd) {
  return FilePath(base::StringPrintf("/proc/self/fd/%d", fd));
}

TEST_F(ProcessTest, RedirectStderrUsingPipe) {
  std::string contents;
  process_.RedirectOutput("");
  process_.AddArg(kBinSh);
  process_.AddArg("-c");
  process_.AddArg("echo errormessage >&2 && exit 1");
  process_.RedirectUsingPipe(STDERR_FILENO, false);
  EXPECT_EQ(-1, process_.GetPipe(STDERR_FILENO));
  EXPECT_EQ(1, process_.Run());
  int pipe_fd = process_.GetPipe(STDERR_FILENO);
  EXPECT_GE(pipe_fd, 0);
  EXPECT_EQ(-1, process_.GetPipe(STDOUT_FILENO));
  EXPECT_EQ(-1, process_.GetPipe(STDIN_FILENO));
  EXPECT_TRUE(base::ReadFileToString(GetFdPath(pipe_fd), &contents));
  EXPECT_NE(std::string::npos, contents.find("errormessage"));
  EXPECT_EQ("", GetLog());
}

TEST_F(ProcessTest, RedirectStderrUsingPipeWhenPreviouslyClosed) {
  int saved_stderr = dup(STDERR_FILENO);
  close(STDERR_FILENO);
  process_.RedirectOutput("");
  process_.AddArg(kBinCp);
  process_.RedirectUsingPipe(STDERR_FILENO, false);
  EXPECT_FALSE(process_.Start());
  EXPECT_TRUE(FindLog("Unable to fstat fd 2:"));
  dup2(saved_stderr, STDERR_FILENO);
}

TEST_F(ProcessTest, RedirectStdoutUsingPipe) {
  std::string contents;
  process_.RedirectOutput("");
  process_.AddArg(kBinEcho);
  process_.AddArg("hello world\n");
  process_.RedirectUsingPipe(STDOUT_FILENO, false);
  EXPECT_EQ(-1, process_.GetPipe(STDOUT_FILENO));
  EXPECT_EQ(0, process_.Run());
  int pipe_fd = process_.GetPipe(STDOUT_FILENO);
  EXPECT_GE(pipe_fd, 0);
  EXPECT_EQ(-1, process_.GetPipe(STDERR_FILENO));
  EXPECT_EQ(-1, process_.GetPipe(STDIN_FILENO));
  EXPECT_TRUE(base::ReadFileToString(GetFdPath(pipe_fd), &contents));
  EXPECT_NE(std::string::npos, contents.find("hello world\n"));
  EXPECT_EQ("", GetLog());
}

TEST_F(ProcessTest, RedirectStdinUsingPipe) {
  std::string contents;
  const char kMessage[] = "made it!\n";
  process_.AddArg(kBinCat);
  process_.RedirectUsingPipe(STDIN_FILENO, true);
  process_.RedirectOutput(output_file_);
  EXPECT_TRUE(process_.Start());
  int write_fd = process_.GetPipe(STDIN_FILENO);
  EXPECT_EQ(-1, process_.GetPipe(STDERR_FILENO));
  EXPECT_TRUE(base::WriteFile(GetFdPath(write_fd), kMessage, strlen(kMessage)));
  close(write_fd);
  EXPECT_EQ(0, process_.Wait());
  ExpectFileEquals(kMessage, output_file_.c_str());
}

TEST_F(ProcessTest, WithSameUid) {
  gid_t uid = geteuid();
  process_.AddArg(kBinEcho);
  process_.SetUid(uid);
  EXPECT_EQ(0, process_.Run());
}

TEST_F(ProcessTest, WithSameGid) {
  gid_t gid = getegid();
  process_.AddArg(kBinEcho);
  process_.SetGid(gid);
  EXPECT_EQ(0, process_.Run());
}

TEST_F(ProcessTest, WithIllegalUid) {
  ASSERT_NE(0, geteuid());
  process_.AddArg(kBinEcho);
  process_.SetUid(0);
  EXPECT_EQ(static_cast<pid_t>(Process::kErrorExitStatus), process_.Run());
  std::string contents;
  EXPECT_TRUE(base::ReadFileToString(FilePath(output_file_), &contents));
  EXPECT_NE(std::string::npos, contents.find("Unable to set UID to 0: 1\n"));
}

TEST_F(ProcessTest, WithIllegalGid) {
  ASSERT_NE(0, getegid());
  process_.AddArg(kBinEcho);
  process_.SetGid(0);
  EXPECT_EQ(static_cast<pid_t>(Process::kErrorExitStatus), process_.Run());
  std::string contents;
  EXPECT_TRUE(base::ReadFileToString(FilePath(output_file_), &contents));
  EXPECT_NE(std::string::npos, contents.find("Unable to set GID to 0: 1\n"));
}

TEST_F(ProcessTest, NoParams) {
  EXPECT_EQ(-1, process_.Run());
}

#if !defined(__BIONIC__)  // Bionic intercepts the segfault on Android.
TEST_F(ProcessTest, SegFaultHandling) {
  process_.AddArg(kBinSh);
  process_.AddArg("-c");
  process_.AddArg("kill -SEGV $$");
  EXPECT_EQ(-1, process_.Run());
  EXPECT_TRUE(FindLog("did not exit normally: 11"));
}
#endif

TEST_F(ProcessTest, KillHandling) {
  process_.AddArg(kBinSh);
  process_.AddArg("-c");
  process_.AddArg("kill -KILL $$");
  EXPECT_EQ(-1, process_.Run());
  EXPECT_TRUE(FindLog("did not exit normally: 9"));
}


TEST_F(ProcessTest, KillNoPid) {
  process_.Kill(SIGTERM, 0);
  EXPECT_TRUE(FindLog("Process not running"));
}

TEST_F(ProcessTest, ProcessExists) {
  EXPECT_FALSE(Process::ProcessExists(0));
  EXPECT_TRUE(Process::ProcessExists(1));
  EXPECT_TRUE(Process::ProcessExists(getpid()));
}

TEST_F(ProcessTest, ResetPidByFile) {
  FilePath pid_path = temp_dir_.path().Append("pid");
  EXPECT_FALSE(process_.ResetPidByFile(pid_path.value()));
  EXPECT_TRUE(base::WriteFile(pid_path, "456\n", 4));
  EXPECT_TRUE(process_.ResetPidByFile(pid_path.value()));
  EXPECT_EQ(456, process_.pid());
  // The purpose of this unit test is to check if Process::ResetPidByFile() can
  // properly read a pid from a file. We don't really want to kill the process
  // with pid 456, so update the pid to 0 to prevent the Process destructor from
  // killing any innocent process.
  process_.UpdatePid(0);
}

TEST_F(ProcessTest, KillSleeper) {
  process_.AddArg(kBinSleep);
  process_.AddArg("10000");
  ASSERT_TRUE(process_.Start());
  pid_t pid = process_.pid();
  ASSERT_GT(pid, 1);
  EXPECT_TRUE(process_.Kill(SIGTERM, 1));
  EXPECT_EQ(0, process_.pid());
}

TEST_F(ProcessTest, Reset) {
  process_.AddArg(kBinFalse);
  process_.Reset(0);
  process_.AddArg(kBinEcho);
  EXPECT_EQ(0, process_.Run());
}

bool ReturnFalse() { return false; }

TEST_F(ProcessTest, PreExecCallback) {
  process_.AddArg(kBinTrue);
  process_.SetPreExecCallback(base::Bind(&ReturnFalse));
  ASSERT_NE(0, process_.Run());
}

TEST_F(ProcessTest, LeakUnusedFileDescriptors) {
  ScopedPipe pipe;
  process_.AddArg(kBinStat);
  process_.AddArg(GetFdPath(pipe.reader).value());
  process_.AddArg(GetFdPath(pipe.writer).value());
  process_.SetCloseUnusedFileDescriptors(false);
  EXPECT_EQ(0, process_.Run());
}

TEST_F(ProcessTest, CloseUnusedFileDescriptors) {
  ScopedPipe pipe;
  process_.AddArg(kBinStat);
  process_.AddArg(GetFdPath(pipe.reader).value());
  process_.AddArg(GetFdPath(pipe.writer).value());
  process_.SetCloseUnusedFileDescriptors(true);
  // Stat should fail when running on these file descriptor because the files
  // should not be there.
  EXPECT_EQ(1, process_.Run());
}

}  // namespace brillo
