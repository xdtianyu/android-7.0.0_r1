// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_BINDINGS_TEST_UTILS_H_
#define CHROMEOS_DBUS_BINDINGS_TEST_UTILS_H_

#include <string>

#include <base/location.h>

namespace chromeos_dbus_bindings {
namespace test_utils {

// Helper macro to call ExpectTextContained().
#define EXPECT_TEXT_CONTAINED(expected, actual) \
  ExpectTextContained(FROM_HERE, expected, #expected, actual, #actual)

// Checks that the text |actual_str| is contained in the text |expected_str| and
// fails the current test if not. If the |actual_str| text is not contained, a
// meaningful line diff between |actual_str| and |expected_str| is displayed in
// stderr. Use this function instead of EXPECT_EQ() when the compared values are
// long texts.
void ExpectTextContained(const tracked_objects::Location& from_here,
                         const std::string& expected_str,
                         const std::string& expected_expr,
                         const std::string& actual_str,
                         const std::string& actual_expr);

}  // namespace test_utils
}  // namespace chromeos_dbus_bindings

#endif  // CHROMEOS_DBUS_BINDINGS_TEST_UTILS_H_
