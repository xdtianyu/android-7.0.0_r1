// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "main.h"
#include "testbase.h"


namespace glbench {


class ClearTest : public TestBase {
 public:
  ClearTest() : mask_(0) {}
  virtual ~ClearTest() {}
  virtual bool TestFunc(uint64_t iterations);
  virtual bool Run();
  virtual const char* Name() const { return "clear"; }
  virtual bool IsDrawTest() const { return true; }
  virtual const char* Unit() const { return "mpixels_sec"; }

 private:
  GLbitfield mask_;
  DISALLOW_COPY_AND_ASSIGN(ClearTest);
};


bool ClearTest::TestFunc(uint64_t iterations) {
  GLbitfield mask = mask_;
  glClear(mask);
  glFlush();  // Kick GPU as soon as possible
  for (uint64_t i = 0; i < iterations - 1; ++i) {
    glClear(mask);
  }
  return true;
}


bool ClearTest::Run() {
  mask_ = GL_COLOR_BUFFER_BIT;
  RunTest(this, "clear_color", g_width * g_height, g_width, g_height, true);

  mask_ = GL_DEPTH_BUFFER_BIT;
  RunTest(this, "clear_depth", g_width * g_height, g_width, g_height, true);

  mask_ = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
  RunTest(this, "clear_colordepth",
          g_width * g_height, g_width, g_height, true);

  mask_ = GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
  RunTest(this, "clear_depthstencil",
          g_width * g_height, g_width, g_height, true);

  mask_ = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
  RunTest(this, "clear_colordepthstencil",
          g_width * g_height, g_width, g_height, true);
  return true;
}


TestBase* GetClearTest() {
  return new ClearTest;
}

} // namespace glbench
