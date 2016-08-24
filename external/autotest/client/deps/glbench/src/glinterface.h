// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BENCH_GL_GLINTERFACE_H_
#define BENCH_GL_GLINTERFACE_H_

#include "base/memory/scoped_ptr.h"

//TODO: Remove cases where PLATFORM is not defined, when no longer needed.
//      Only synccontroltest and teartest get compiled that way, and we plan
//      to remove them.
#ifdef PLATFORM

typedef struct waffle_context *GLContext;  // Forward declaration from waffle.h.

#else

#include <X11/Xutil.h>

#if defined(USE_OPENGL)

struct __GLXcontextRec;  // Forward declaration from GLX.h.
typedef struct __GLXcontextRec *GLXContext;
typedef GLXContext GLContext;

#elif defined(USE_OPENGLES)

typedef void *EGLContext;  // Forward declaration from EGL.h.
typedef EGLContext GLContext;

#endif
#endif

//TODO: Once synccontroltest and teartest are removed, only the waffle
//      implementation of this interface will remain.  At that time consider
//      removing this class as waffle itself provides platform-independence.
class GLInterface {
 public:
  GLInterface() {}
  virtual ~GLInterface() {}
  virtual bool Init() = 0;
  virtual void Cleanup() = 0;
#ifndef PLATFORM
  virtual XVisualInfo* GetXVisual() = 0;
#endif

  virtual void SwapBuffers() = 0;
  //TODO: Remove this when it is no longer used.
  //      Only teartest calls this, and we plan to remove that.
  virtual bool SwapInterval(int interval) = 0;

  //TODO: Remove this when it is no longer used.
  //      Only synccontroltest_egl calls this, and we plan to remove that.
  virtual void CheckError() = 0;

  virtual bool MakeCurrent(const GLContext& context) = 0;
  virtual const GLContext CreateContext() = 0;
  virtual void DeleteContext(const GLContext& context) = 0;
  virtual const GLContext& GetMainContext() = 0;

  static GLInterface* Create();
};

extern scoped_ptr<GLInterface> g_main_gl_interface;

#endif  // BENCH_GL_GLINTERFACE_H_
