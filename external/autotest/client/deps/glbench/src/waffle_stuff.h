// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BENCH_GL_WAFFLE_STUFF_H_
#define BENCH_GL_WAFFLE_STUFF_H_

#include <waffle.h>

#include "glinterface.h"

class WaffleInterface : public GLInterface {
 public:

  WaffleInterface() : display_(NULL),
                   config_(NULL),
                   surface_(NULL),
                   context_(NULL) { }
  virtual ~WaffleInterface() {}

  virtual bool Init();
  virtual void Cleanup();

  virtual void SwapBuffers();
  virtual bool SwapInterval(int interval);

  virtual void CheckError();

  virtual bool MakeCurrent(const GLContext& context);
  virtual const GLContext CreateContext();
  virtual void DeleteContext(const GLContext& context);
  virtual const GLContext& GetMainContext() {
    return context_;
  }

  const struct waffle_display* display() const {
    return display_;
  }

  const struct waffle_window* surface() const {
    return surface_;
  }

 private:
  void InitOnce();
  void GetSurfaceSize(GLint *width, GLint *height);

  struct waffle_display *display_;
  struct waffle_config *config_;
  struct waffle_window *surface_;
  struct waffle_context *context_;
};

#endif // BENCH_GL_WAFFLE_STUFF_H_
