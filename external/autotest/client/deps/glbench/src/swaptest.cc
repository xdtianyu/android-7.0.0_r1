// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/callback.h"

#include "glinterface.h"
#include "glinterfacetest.h"
#include "main.h"

namespace glbench {

class SwapTest : public GLInterfaceTest {
 public:
  SwapTest() {}
  virtual ~SwapTest() {}
  virtual bool TestFunc(uint64_t iterations);
  virtual const char* Name() const { return "swap"; }

 private:
  DISALLOW_COPY_AND_ASSIGN(SwapTest);
};

bool SwapTest::TestFunc(uint64_t iterations) {
  for (uint64_t i = 0 ; i < iterations; ++i) {
    if (!render_func_.is_null())
      render_func_.Run();
    g_main_gl_interface->SwapBuffers();
  }
  return true;
}

TestBase* GetSwapTest() {
  return new SwapTest;
}

} // namespace glbench
