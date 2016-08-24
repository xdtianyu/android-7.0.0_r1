// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_PROCESS_INFORMATION_H_
#define LIBBRILLO_BRILLO_PROCESS_INFORMATION_H_

#include <set>
#include <string>
#include <vector>

#include <brillo/brillo_export.h>

namespace brillo {

// Information for a single running process. Stores its command line, set of
// open files, process id and working directory.
class BRILLO_EXPORT ProcessInformation {
 public:
  ProcessInformation();
  virtual ~ProcessInformation();

  std::string GetCommandLine();

  // Set the command line array.  This method DOES swap out the contents of
  // |value|.  The caller should expect an empty vector on return.
  void set_cmd_line(std::vector<std::string>* value) {
    cmd_line_.clear();
    cmd_line_.swap(*value);
  }

  const std::vector<std::string>& get_cmd_line() { return cmd_line_; }

  // Set the command line array.  This method DOES swap out the contents of
  // |value|.  The caller should expect an empty set on return.
  void set_open_files(std::set<std::string>* value) {
    open_files_.clear();
    open_files_.swap(*value);
  }

  const std::set<std::string>& get_open_files() { return open_files_; }

  // Set the command line array.  This method DOES swap out the contents of
  // |value|.  The caller should expect an empty string on return.
  void set_cwd(std::string* value) {
    cwd_.clear();
    cwd_.swap(*value);
  }

  const std::string& get_cwd() { return cwd_; }

  void set_process_id(int value) { process_id_ = value; }

  int get_process_id() { return process_id_; }

 private:
  std::vector<std::string> cmd_line_;
  std::set<std::string> open_files_;
  std::string cwd_;
  int process_id_;
};

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_PROCESS_INFORMATION_H_
