// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BENCH_GL_TESTBASE_H_
#define BENCH_GL_TESTBASE_H_

#include "base/macros.h"

#include "main.h"

#define DISABLE_SOME_TESTS_FOR_INTEL_DRIVER 1

#define IS_NOT_POWER_OF_2(v) (((v) & ((v) - 1)) && (v))

namespace glbench {

class TestBase;

// Runs test->TestFunc() passing it sequential powers of two recording time it
// took until reaching a minimum amount of testing time. The last two runs are
// then averaged.
double Bench(TestBase* test);

// Runs Bench on an instance of TestBase and prints out results.
//
// coefficient is multiplied (if inverse is false) or divided (if inverse is
// true) by the measured unit runtime and the result is printed.
//
// Examples:
//   coefficient = width * height (measured in pixels), inverse = true
//       returns the throughput in megapixels per second;
//
//   coefficient = 1, inverse = false
//       returns number of operations per second.
void RunTest(TestBase* test,
             const char *name,
             double coefficient,
             const int width,
             const int height,
             bool inverse);

class TestBase {
 public:
  virtual ~TestBase() {}
  // Runs the test case n times.
  virtual bool TestFunc(uint64_t n) = 0;
  // Main entry point into the test.
  virtual bool Run() = 0;
  // Name of test case group
  virtual const char* Name() const = 0;
  // Returns true if a test draws some output.
  // If so, testbase will read back pixels, compute its MD5 hash and optionally
  // save them to a file on disk.
  virtual bool IsDrawTest() const = 0;
  // Name of unit for benchmark score (e.g., mtexel_sec, us, etc.)
  virtual const char* Unit() const = 0;
};

// Helper class to time glDrawArrays.
class DrawArraysTestFunc : public TestBase {
 public:
  virtual ~DrawArraysTestFunc() {}
  virtual bool TestFunc(uint64_t);
  virtual bool IsDrawTest() const { return true; }
  virtual const char* Unit() const { return "mpixels_sec"; }

  // Runs the test and reports results in mpixels per second, assuming each
  // iteration updates the whole window (its size is g_width by g_height).
  void FillRateTestNormal(const char* name);
  // Runs the test and reports results in mpixels per second, assuming each
  // iteration updates a window of width by height pixels.
  void FillRateTestNormalSubWindow(const char* name,
                                   const int width, const int height);
  // Runs the test three times: with blending on; with depth test enabled and
  // depth function of GL_NOTEQUAL; with depth function GL_NEVER.  Results are
  // reported as in FillRateTestNormal.
  void FillRateTestBlendDepth(const char *name);
};

// Helper class to time glDrawElements.
class DrawElementsTestFunc : public TestBase {
 public:
  DrawElementsTestFunc() : count_(0) {}
  virtual ~DrawElementsTestFunc() {}
  virtual bool TestFunc(uint64_t);
  virtual bool IsDrawTest() const { return true; }
  virtual const char* Unit() const { return "mtri_sec"; }

 protected:
  // Passed to glDrawElements.
  GLsizei count_;
};

} // namespace glbench

#endif // BENCH_GL_TESTBASE_H_
