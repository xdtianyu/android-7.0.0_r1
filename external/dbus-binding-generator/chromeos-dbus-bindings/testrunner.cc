// Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <base/at_exit.h>
#include <base/command_line.h>
#include <brillo/syslog_logging.h>
#include <gtest/gtest.h>

int main(int argc, char** argv) {
  base::AtExitManager exit_manager;
  base::CommandLine::Init(argc, argv);
  brillo::InitLog(brillo::kLogToStderr);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
