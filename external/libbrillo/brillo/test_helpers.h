// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_TEST_HELPERS_H_
#define LIBBRILLO_BRILLO_TEST_HELPERS_H_

#include "gtest/gtest.h"

#include <string>

#include <base/command_line.h>
#include <base/files/file_path.h>
#include <base/files/file_util.h>
#include <base/logging.h>

#include "brillo/syslog_logging.h"

inline void ExpectFileEquals(const char* golden, const char* file_path) {
  std::string contents;
  EXPECT_TRUE(base::ReadFileToString(base::FilePath(file_path), &contents));
  EXPECT_EQ(golden, contents);
}

inline void SetUpTests(int* argc, char** argv, bool log_to_stderr) {
  base::CommandLine::Init(*argc, argv);
  ::brillo::InitLog(log_to_stderr ? brillo::kLogToStderr : 0);
  ::brillo::LogToString(true);
  ::testing::InitGoogleTest(argc, argv);
}

#endif  // LIBBRILLO_BRILLO_TEST_HELPERS_H_
