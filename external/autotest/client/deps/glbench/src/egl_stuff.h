// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BENCH_GL_EGL_STUFF_H_
#define BENCH_GL_EGL_STUFF_H_

#include "base/logging.h"
#include "glinterface.h"
#include <EGL/egl.h>

class EGLInterface : public GLInterface {
 public:

  EGLInterface() : display_(EGL_NO_DISPLAY),
                   config_(NULL),
                   surface_(NULL),
                   context_(NULL) { }
  virtual ~EGLInterface() {}

  virtual bool Init();
  virtual void Cleanup();
  virtual XVisualInfo* GetXVisual();

  virtual void SwapBuffers();
  virtual bool SwapInterval(int interval);

  virtual void CheckError();

  virtual bool MakeCurrent(const GLContext& context);
  virtual const GLContext CreateContext();
  virtual void DeleteContext(const GLContext& context);
  virtual const GLContext& GetMainContext() {
    return context_;
  }

  void TerminateGL();

  const EGLDisplay display() const {
    return display_;
  }

  const EGLSurface surface() const {
    return surface_;
  }

 private:
  EGLDisplay display_;
  EGLConfig config_;
  EGLSurface surface_;
  EGLContext context_;
};

#endif  // BENCH_GL_EGL_STUFF_H_
