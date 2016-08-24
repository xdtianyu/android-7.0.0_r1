// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_TEST_MOCK_CLOCK_H_
#define LIBWEAVE_SRC_TEST_MOCK_CLOCK_H_

#include <base/time/clock.h>
#include <gmock/gmock.h>

namespace weave {
namespace test {

class MockClock : public base::Clock {
 public:
  MOCK_METHOD0(Now, base::Time());
};

}  // namespace test
}  // namespace weave

#endif  // LIBWEAVE_SRC_TEST_MOCK_CLOCK_H_
