// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <GL/gl.h>
#include <string.h>

#include "base/logging.h"

#include "glx_stuff.h"
#include "main.h"
#include "xlib_window.h"

namespace gl {
#define F(fun, type) type fun = NULL;
LIST_PROC_FUNCTIONS(F)
#undef F
};

#ifndef GLX_MESA_swap_control
typedef GLint (* PFNGLXSWAPINTERVALMESAPROC) (unsigned interval);
typedef GLint (* PFNGLXGETSWAPINTERVALMESAPROC) (void);
#endif
PFNGLXSWAPINTERVALMESAPROC _glXSwapIntervalMESA = NULL;

scoped_ptr<GLInterface> g_main_gl_interface;

GLInterface* GLInterface::Create() {
  return new GLXInterface;
}

bool GLXInterface::Init() {
  if (!XlibInit())
    return false;

  context_ = CreateContext();
  if (!context_)
    return false;

  if (!glXMakeCurrent(g_xlib_display, g_xlib_window, context_)) {
    glXDestroyContext(g_xlib_display, context_);
    return false;
  }

  const GLubyte *str = glGetString(GL_EXTENSIONS);
  if (!str || !strstr(reinterpret_cast<const char *>(str),
                      "GL_ARB_vertex_buffer_object"))
    return false;

#define F(fun, type) fun = reinterpret_cast<type>( \
    glXGetProcAddress(reinterpret_cast<const GLubyte *>(#fun)));
  LIST_PROC_FUNCTIONS(F)
#undef F
  _glXSwapIntervalMESA = reinterpret_cast<PFNGLXSWAPINTERVALMESAPROC>(
    glXGetProcAddress(reinterpret_cast<const GLubyte *>(
        "glXSwapIntervalMESA")));

  return true;
}

void GLXInterface::Cleanup() {
  glXMakeCurrent(g_xlib_display, 0, NULL);
  DeleteContext(context_);
}

XVisualInfo* GLXInterface::GetXVisual() {
  if (!fb_config_) {
    int screen = DefaultScreen(g_xlib_display);
    int attrib[] = {
      GLX_DOUBLEBUFFER, True,
      GLX_RED_SIZE, 1,
      GLX_GREEN_SIZE, 1,
      GLX_BLUE_SIZE, 1,
      GLX_DEPTH_SIZE, 1,
      GLX_STENCIL_SIZE, 1,
      GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
      None
    };
    int nelements;
    GLXFBConfig *fb_configs = glXChooseFBConfig(g_xlib_display, screen,
                                                attrib, &nelements);
    CHECK(nelements >= 1);
    fb_config_ = fb_configs[0];
    XFree(fb_configs);
  }

  return glXGetVisualFromFBConfig(g_xlib_display, fb_config_);
}

bool GLXInterface::MakeCurrent(const GLContext& context) {
  return glXMakeCurrent(g_xlib_display, g_xlib_window, context);
}

const GLContext GLXInterface::CreateContext() {
  CHECK(g_xlib_display);
  CHECK(fb_config_);
  return glXCreateNewContext(g_xlib_display, fb_config_, GLX_RGBA_TYPE, 0,
                             True);
}

void GLXInterface::DeleteContext(const GLContext& context) {
  glXDestroyContext(g_xlib_display, context);
}

void GLXInterface::SwapBuffers() {
  glXSwapBuffers(g_xlib_display, g_xlib_window);
}

bool GLXInterface::SwapInterval(int interval) {
  // Strictly, glXSwapIntervalSGI only allows interval > 0, whereas
  // glXSwapIntervalMESA allow 0 with the same semantics as eglSwapInterval.
  if (_glXSwapIntervalMESA) {
    return _glXSwapIntervalMESA(interval) == 0;
  } else {
    return glXSwapIntervalSGI(interval) == 0;
  }
}

void GLXInterface::CheckError() {
  CHECK_EQ(glGetError(), static_cast<GLenum>(GL_NO_ERROR));
};
