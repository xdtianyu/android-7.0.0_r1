// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brillo/process_information.h"

namespace brillo {

ProcessInformation::ProcessInformation() : cmd_line_(), process_id_(-1) {
}
ProcessInformation::~ProcessInformation() {
}

std::string ProcessInformation::GetCommandLine() {
  std::string result;
  for (std::vector<std::string>::iterator cmd_itr = cmd_line_.begin();
       cmd_itr != cmd_line_.end();
       cmd_itr++) {
    if (result.length()) {
      result.append(" ");
    }
    result.append((*cmd_itr));
  }
  return result;
}

}  // namespace brillo
