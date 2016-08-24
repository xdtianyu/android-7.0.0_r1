// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BENCH_GL_GLX_STUFF_H_
#define BENCH_GL_GLX_STUFF_H_

#include <GL/glx.h>

#include "glinterface.h"

class GLXInterface : public GLInterface {
 public:
  GLXInterface() : context_(NULL),
                   fb_config_(NULL) {}
  virtual ~GLXInterface() {}

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

  const GLXFBConfig fb_config() const {
    return fb_config_;
  }

 private:
  GLXContext context_;
  GLXFBConfig fb_config_;
};

#endif // BENCH_GL_GLX_STUFF_H_
