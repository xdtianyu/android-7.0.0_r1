// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _BSDIFF_EXTENTS_H_
#define _BSDIFF_EXTENTS_H_

#include <vector>

#include "extents_file.h"

namespace bsdiff {

// Parses a string representation |ex_str| and populates the vector |extents|
// of ex_t. The string is expected to be a comma-separated list of pairs of the
// form "offset:length". An offset may be -1 or a non-negative integer; the
// former indicates a sparse extent (consisting of zeros). A length is a
// positive integer. Returns whether the parsing was successful.
bool ParseExtentStr(const char* ex_str, std::vector<ex_t>* extents);

}  // namespace bsdiff

#endif  // _BSDIFF_EXTENTS_H_
