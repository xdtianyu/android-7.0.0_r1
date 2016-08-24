// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gflags/gflags.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctime>

#include "base/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"

#include "glinterface.h"
#include "main.h"
#include "utils.h"

#include "all_tests.h"
#include "testbase.h"

using std::string;
using std::vector;

DEFINE_int32(duration, 0,
             "Run all tests again and again in a loop for at least this many seconds.");
DEFINE_string(tests, "",
              "Colon-separated list of tests to run; all tests if omitted.");
DEFINE_string(blacklist, "", "colon-separated list of tests to disable");
DEFINE_bool(hasty, false,
            "Run a smaller set of tests with less accurate results. "
            "Useful for running in BVT or debugging a failure.  Implies notemp");
DEFINE_bool(list, false, "List available tests");
DEFINE_bool(notemp, false, "Skip temperature checking");
DEFINE_bool(verbose, false, "Print extra debugging messages");

bool g_verbose;
GLint g_max_texture_size;
bool g_hasty;
bool g_notemp;

bool test_is_enabled(glbench::TestBase* test,
                     const vector<string>& enabled_tests) {
  if (enabled_tests.empty())
    return true;

  const char* test_name = test->Name();
  for (vector<string>::const_iterator i = enabled_tests.begin();
       i != enabled_tests.end(); ++i) {
    // This is not very precise, but will do until there's a need for something
    // more flexible.
    if (strstr(test_name, i->c_str()))
      return true;
  }

  return false;
}

bool test_is_disabled(glbench::TestBase* test,
                     const vector<string>& disabled_tests) {
  if (disabled_tests.empty())
    return false;

  const char* test_name = test->Name();
  for (vector<string>::const_iterator i = disabled_tests.begin();
       i != disabled_tests.end(); ++i) {
    // This is not very precise, but will do until there's a need for something
    // more flexible.
    if (strstr(test_name, i->c_str()))
      return true;
  }

  return false;
}

void printDateTime(void) {
  struct tm *ttime;
  time_t tm = time(0);
  char time_string[64];
  ttime = localtime(&tm);
  strftime(time_string, 63, "%c",ttime);
  printf("# DateTime: %s\n", time_string);
}

bool PassesSanityCheck(void) {
  GLint size[2];
  glGetIntegerv(GL_MAX_VIEWPORT_DIMS, size);
  printf("# MAX_VIEWPORT_DIMS=(%d, %d)\n", size[0], size[1]);
  if (size[0] < g_width || size[1] < g_height) {
    printf("# Error: MAX_VIEWPORT_DIMS=(%d, %d) are too small.\n",
           size[0], size[1]);
    return false;
  }
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, size);
  printf("# GL_MAX_TEXTURE_SIZE=%d\n", size[0]);
  if (size[0] < g_width || size[0] < g_height) {
    printf("# Error: MAX_TEXTURE_SIZE=%d is too small.\n",
           size[0]);
    return false;
  }
  g_max_texture_size = size[0];

  return true;
}

int main(int argc, char *argv[]) {
  SetBasePathFromArgv0(argv[0], "src");
  google::ParseCommandLineFlags(&argc, &argv, false);

  g_verbose = FLAGS_verbose;

  g_main_gl_interface.reset(GLInterface::Create());
  if (!g_main_gl_interface->Init()) {
    printf("# Error: Failed to initialize %s.\n", argv[0]);
    return 1;
  }

  printf("# board_id: %s - %s\n",
         glGetString(GL_VENDOR), glGetString(GL_RENDERER));
  if (!PassesSanityCheck())
    return 1;
  g_main_gl_interface->Cleanup();

  if (argc == 1) {
    printf("# Usage: %s [-save [-outdir=<directory>]] to save images\n", argv[0]);
  } else {
    printf("# Running: ");
    for (int i = 0; i < argc; i++) printf("%s ", argv[i]);
    printf("\n");
  }
  printDateTime();

  g_hasty = FLAGS_hasty;
  g_notemp = FLAGS_notemp || g_hasty;

  if (!g_notemp)
    g_initial_temperature = GetMachineTemperature();

  vector<string> enabled_tests =
      base::SplitString(FLAGS_tests, ":", base::TRIM_WHITESPACE,
                        base::SPLIT_WANT_ALL);
  vector<string> disabled_tests =
      base::SplitString(FLAGS_blacklist, ":", base::TRIM_WHITESPACE,
                        base::SPLIT_WANT_ALL);

  glbench::TestBase* tests[] = {
    // Please add new tests at the end of this list as tests are known to bleed
    // state. Reordering them or inserting a new test may cause a change in the
    // output images and MD5 causing graphics_GLBench failures.
    // TODO(ihf): Fix this.
    glbench::GetSwapTest(),
    glbench::GetContextTest(),
    glbench::GetClearTest(),
    glbench::GetFillRateTest(),
    glbench::GetWindowManagerCompositingTest(false),
    glbench::GetWindowManagerCompositingTest(true),
    glbench::GetTriangleSetupTest(),
    glbench::GetYuvToRgbTest(),
    glbench::GetReadPixelTest(),
    glbench::GetAttributeFetchShaderTest(),
    glbench::GetVaryingsAndDdxyShaderTest(),
    glbench::GetTextureReuseTest(),
    glbench::GetTextureUpdateTest(),
    glbench::GetTextureUploadTest(),
    glbench::GetFboFillRateTest(),
  };

  if (FLAGS_list) {
    for (unsigned int i = 0; i < arraysize(tests); i++)
      printf("%s\n", tests[i]->Name());
    return 0;
  }

  uint64_t done = GetUTime() + 1000000ULL * FLAGS_duration;
  do {
    for (unsigned int i = 0; i < arraysize(tests); i++) {
      if (!test_is_enabled(tests[i], enabled_tests) ||
          test_is_disabled(tests[i], disabled_tests))
        continue;
      if (!g_main_gl_interface->Init()) {
        printf("Initialize failed\n");
        return 1;
      }
      glbench::ClearBuffers();
      tests[i]->Run();
      g_main_gl_interface->Cleanup();
    }
  } while (GetUTime() < done);

  for (unsigned int i = 0; i < arraysize(tests); i++) {
    delete tests[i];
    tests[i] = NULL;
  }

  printDateTime();
  // Signal to harness that we finished normally.
  printf("@TEST_END\n");

  return 0;
}
