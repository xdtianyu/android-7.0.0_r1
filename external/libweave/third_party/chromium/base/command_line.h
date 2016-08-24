// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This class works with command lines: building and parsing.
// Arguments with prefixes ('--', '-', and on Windows, '/') are switches.
// Switches will precede all other arguments without switch prefixes.
// Switches can optionally have values, delimited by '=', e.g., "-switch=value".
// An argument of "--" will terminate switch parsing during initialization,
// interpreting subsequent tokens as non-switch arguments, regardless of prefix.

// There is a singleton read-only CommandLine that represents the command line
// that the current process was started with.  It must be initialized in main().

#ifndef BASE_COMMAND_LINE_H_
#define BASE_COMMAND_LINE_H_

#include <stddef.h>

#include "base/base_export.h"

namespace base {

class CommandLine {
 public:
  static bool Init(int argc, const char* const* argv) { return true; }
};

}  // namespace base

#endif  // BASE_COMMAND_LINE_H_
