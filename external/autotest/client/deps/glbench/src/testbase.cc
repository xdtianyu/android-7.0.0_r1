// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gflags/gflags.h>
#include <png.h>
#include <stdio.h>
#include <unistd.h>

#include <base/files/file_util.h>
#include <base/memory/scoped_ptr.h>

#include "glinterface.h"
#include "md5.h"
#include "png_helper.h"
#include "testbase.h"
#include "utils.h"

extern bool g_hasty;
extern bool g_notemp;

DEFINE_bool(save, false, "save images after each test case");
DEFINE_string(outdir, "", "directory to save images");

namespace glbench {

uint64_t TimeTest(TestBase* test, uint64_t iterations) {
    g_main_gl_interface->SwapBuffers();
    glFinish();
    uint64_t time1 = GetUTime();
    if (!test->TestFunc(iterations))
        return ~0;
    glFinish();
    uint64_t time2 = GetUTime();
    return time2 - time1;
}

// Target minimum iteration duration of 1s. This means the final/longest
// iteration is between 1s and 2s and the machine is active for 2s to 4s.
// Notice as of March 2014 the BVT suite has a hard limit per job of 20 minutes.
#define MIN_ITERATION_DURATION_US 1000000

#define MAX_TESTNAME 45

// Benchmark some draw commands, by running it many times. We want to measure
// the marginal cost, so we try more and more iterations until we reach the
// minimum specified iteration time.
double Bench(TestBase* test) {
  // Try to wait a bit to let machine cool down for next test. We allow for a
  // bit of hysteresis as it might take too long to do a perfect job, which is
  // probably not required. But these parameters could be tuned.
  double initial_temperature = GetInitialMachineTemperature();
  double temperature = 0;
  double wait = 0;

  // By default we try to cool to initial + 5'C but don't wait longer than 30s.
  // But in hasty mode we really don't want to spend too much time to get the
  // numbers right, so we don't wait at all.
  if (!::g_notemp) {
    wait = WaitForCoolMachine(initial_temperature + 5.0, 30.0, &temperature);
    printf("Bench: Cooled down to %.1f'C (initial=%.1f'C) after waiting %.1fs.\n",
           temperature, initial_temperature, wait);
    if (temperature > initial_temperature + 10.0)
      printf("Warning: Machine did not cool down enough for next test!");
  }

  // Do two iterations because initial timings can vary wildly.
  TimeTest(test, 2);

  // We average the times for the last two runs to reduce noise. We could
  // sum up all runs but the initial measurements have high CPU overhead,
  // while the last two runs are both on the order of MIN_ITERATION_DURATION_US.
  uint64_t iterations = 1;
  uint64_t iterations_prev = 0;
  uint64_t time = 0;
  uint64_t time_prev = 0;
  do {
    time = TimeTest(test, iterations);
    dbg_printf("iterations: %llu: time: %llu time/iter: %llu\n",
           iterations, time, time / iterations);

    // If we are running in hasty mode we will stop after a fraction of the
    // testing time and return much more noisy performance numbers. The MD5s
    // of the images should stay the same though.
    if (time > MIN_ITERATION_DURATION_US / (::g_hasty ? 20.0 : 1.0))
      return (static_cast<double>(time + time_prev) /
              (iterations + iterations_prev));

    time_prev = time;
    iterations_prev = iterations;
    iterations *= 2;
  } while (iterations < (1ULL<<40));

  return 0.0;
}

void SaveImage(const char* name, const int width, const int height) {
  const int size = width * height * 4;
  scoped_ptr<char[]> pixels(new char[size]);
  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.get());
  // I really think we want to use outdir as a straight argument
  base::FilePath dirname = base::FilePath(FLAGS_outdir);
  base::CreateDirectory(dirname);
  base::FilePath filename = dirname.Append(name);
  write_png_file(filename.value().c_str(),
                 pixels.get(), width, height);
}

void ComputeMD5(unsigned char digest[16], const int width, const int height) {
  MD5Context ctx;
  MD5Init(&ctx);
  const int size = width * height * 4;
  scoped_ptr<char[]> pixels(new char[size]);
  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.get());
  MD5Update(&ctx, (unsigned char *)pixels.get(), size);
  MD5Final(digest, &ctx);
}

void RunTest(TestBase* test, const char* testname, const double coefficient,
             const int width, const int height, bool inverse) {
  double value;
  char name_png[512] = "";
  GLenum error = glGetError();

  if (error != GL_NO_ERROR) {
    value = -1.0;
    printf("# Error: %s aborted, glGetError returned 0x%02x.\n",
           testname, error);
    sprintf(name_png, "glGetError=0x%02x", error);
  } else {
    value = Bench(test);

    // Bench returns 0.0 if it ran max iterations in less than a min test time.
    if (value == 0.0) {
      strcpy(name_png, "no_score");
    } else {
      value = coefficient * (inverse ? 1.0 / value : value);

      if (!test->IsDrawTest()) {
        strcpy(name_png, "none");
      } else {
        // save as png with MD5 as hex string attached
        char          pixmd5[33];
        unsigned char d[16];
        ComputeMD5(d, width, height);
        // translate to hexadecimal ASCII of MD5
        sprintf(pixmd5,
          "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
          d[ 0],d[ 1],d[ 2],d[ 3],d[ 4],d[ 5],d[ 6],d[ 7],
          d[ 8],d[ 9],d[10],d[11],d[12],d[13],d[14],d[15]);
        sprintf(name_png, "%s.pixmd5-%s.png", testname, pixmd5);

        if (FLAGS_save)
          SaveImage(name_png, width, height);
      }
    }
  }

  // TODO(ihf) adjust string length based on longest test name
  int name_length = strlen(testname);
  if (name_length > MAX_TESTNAME)
    printf("# Warning: adjust string formatting to length = %d\n",
           name_length);
  // Results are marked using a leading '@RESULT: ' to allow parsing.
  printf("@RESULT: %-*s = %10.2f %-15s [%s]\n",
         MAX_TESTNAME, testname, value, test->Unit(), name_png);
}

bool DrawArraysTestFunc::TestFunc(uint64_t iterations) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glFlush();
  for (uint64_t i = 0; i < iterations - 1; ++i) {
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }
  return true;
}


void DrawArraysTestFunc::FillRateTestNormal(const char* name) {
  FillRateTestNormalSubWindow(name, g_width, g_height);
}


void DrawArraysTestFunc::FillRateTestNormalSubWindow(const char* name,
                                                     const int width,
                                                     const int height)
{
  RunTest(this, name, width * height, width, height, true);
}


void DrawArraysTestFunc::FillRateTestBlendDepth(const char *name) {
  const int buffer_len = 64;
  char buffer[buffer_len];

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  snprintf(buffer, buffer_len, "%s_blended", name);
  RunTest(this, buffer, g_width * g_height, g_width, g_height, true);
  glDisable(GL_BLEND);

  // We are relying on the default depth clear value of 1 here.
  // Fragments should have depth 0.
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_NOTEQUAL);
  snprintf(buffer, buffer_len, "%s_depth_neq", name);
  RunTest(this, buffer, g_width * g_height, g_width, g_height, true);

  // The DrawArrays call invoked by this test shouldn't render anything
  // because every fragment will fail the depth test.  Therefore we
  // should see the clear color.
  glDepthFunc(GL_NEVER);
  snprintf(buffer, buffer_len, "%s_depth_never", name);
  RunTest(this, buffer, g_width * g_height, g_width, g_height, true);
  glDisable(GL_DEPTH_TEST);
}


bool DrawElementsTestFunc::TestFunc(uint64_t iterations) {
  glClearColor(0, 1.f, 0, 1.f);
  glClear(GL_COLOR_BUFFER_BIT);
  glDrawElements(GL_TRIANGLES, count_, GL_UNSIGNED_SHORT, 0);
  glFlush();
  for (uint64_t i = 0 ; i < iterations - 1; ++i) {
    glDrawElements(GL_TRIANGLES, count_, GL_UNSIGNED_SHORT, 0);
  }
  return true;
}

} // namespace glbench
