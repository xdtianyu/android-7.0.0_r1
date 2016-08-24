// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos-dbus-bindings/test_utils.h"

#include <string>
#include <vector>

#include <base/files/file_path.h>
#include <base/files/file_util.h>
#include <base/logging.h>
#include <brillo/process.h>
#include <gtest/gtest.h>

using std::string;
using std::vector;

namespace {

// Return the diff between the texts |a| and |b|.
string GetUnifiedDiff(const string& a, const string& b) {
  base::FilePath path_a, path_b;
  if (!base::CreateTemporaryFile(&path_a) ||
      !base::CreateTemporaryFile(&path_b)) {
    return "Error creating temporary file";
  }
  WriteFile(path_a, a.data(), a.size());
  WriteFile(path_b, b.data(), b.size());

  brillo::ProcessImpl proc;
  proc.AddArg("diff");
  proc.AddArg("-u");
  proc.AddArg(path_a.value());
  proc.AddArg(path_b.value());
  proc.SetSearchPath(true);
  proc.RedirectUsingPipe(STDOUT_FILENO, false);
  proc.Start();

  int fd = proc.GetPipe(STDOUT_FILENO);
  vector<char> buffer(32 * 1024);
  string output;
  while (true) {
    int rc = read(fd, buffer.data(), buffer.size());
    if (rc < 0) {
      PLOG(ERROR) << "Reading from diff.";
      break;
    } else if (rc == 0) {
      break;
    } else {
      output.append(buffer.data(), rc);
    }
  }
  proc.Wait();

  base::DeleteFile(path_a, false);
  base::DeleteFile(path_b, false);
  return output;
}

}  // namespace

namespace chromeos_dbus_bindings {
namespace test_utils {

void ExpectTextContained(const tracked_objects::Location& from_here,
                         const string& expected_str,
                         const string& expected_expr,
                         const string& actual_str,
                         const string& actual_expr) {
  if (string::npos != actual_str.find(expected_str))
    return;

  ADD_FAILURE_AT(from_here.file_name(), from_here.line_number())
      << "Expected to find " << expected_expr << " within " << actual_expr
      << ".\nHere is the diff:\n"
      << GetUnifiedDiff(expected_str, actual_str);
}

}  // namespace test_utils
}  // namespace chromeos_dbus_bindings
