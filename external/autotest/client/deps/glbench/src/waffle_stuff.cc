// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#define WAFFLE_API_VERSION 0x0106

#include "base/logging.h"
#include "main.h"
#include "waffle_stuff.h"
#include <stdio.h>

GLint g_width = WINDOW_WIDTH;
GLint g_height = WINDOW_HEIGHT;

scoped_ptr<GLInterface> g_main_gl_interface;

#ifdef USE_OPENGL
namespace gl {
#define F(fun, type) type fun = NULL;
LIST_PROC_FUNCTIONS(F)
#undef F
};
#define GL_API WAFFLE_CONTEXT_OPENGL
#else
#define GL_API WAFFLE_CONTEXT_OPENGL_ES2
#endif

#define ID_PLATFORM_GLX     1
#define ID_PLATFORM_X11_EGL 2
#define ID_PLATFORM_NULL    3

#define CONCAT(a,b) a ## b
#define PLATFORM_ID(x) CONCAT(ID_, x)
#define PLATFORM_ENUM(x) CONCAT(WAFFLE_, x)
#define THIS_IS(x) PLATFORM_ID(x) == PLATFORM_ID(PLATFORM)

#if THIS_IS(PLATFORM_GLX)
#include "waffle_glx.h"
#elif THIS_IS(PLATFORM_X11_EGL)
#include "waffle_x11_egl.h"
#elif THIS_IS(PLATFORM_NULL)
#include "waffle_null.h"
#else
#error "Compile with -DPLATFORM=PLATFORM_<x> where <x> is NULL, GLX or X11_EGL."
#endif

#define WAFFLE_CHECK_ERROR do { CHECK(WaffleOK()); } while (0)

GLInterface* GLInterface::Create() {
  return new WaffleInterface;
}

static bool WaffleOK() {
  const waffle_error_info *info = waffle_error_get_info();
  if (info->code == WAFFLE_NO_ERROR)
    return true;
  printf("# Error: %s: %s\n",
         waffle_error_to_string(info->code),
         info->message);
  return false;
}

void WaffleInterface::GetSurfaceSize(GLint *width, GLint *height) {
  union waffle_native_window *nw = waffle_window_get_native(surface_);

#if THIS_IS(PLATFORM_NULL)
  *width = nw->null->width;
  *height = nw->null->height;
#elif THIS_IS(PLATFORM_GLX)
  unsigned w, h;
#if 0
  // doesn't work with mesa - https://bugs.freedesktop.org/show_bug.cgi?id=54080
  glXQueryDrawable(nw->glx->xlib_display, nw->glx->xlib_window, GLX_WIDTH, &w);
  glXQueryDrawable(nw->glx->xlib_display, nw->glx->xlib_window, GLX_HEIGHT, &h);
#else
   Window root;
   int x, y;
   unsigned bd, depth;
   XGetGeometry(nw->glx->xlib_display, nw->glx->xlib_window,
                &root, &x, &y, &w, &h, &bd, &depth);
#endif
  *width = w;
  *height = h;
#elif THIS_IS(PLATFORM_X11_EGL)
  EGLint w, h;
  eglQuerySurface(nw->x11_egl->display.egl_display, nw->x11_egl->egl_surface,
                  EGL_WIDTH, &w);
  eglQuerySurface(nw->x11_egl->display.egl_display, nw->x11_egl->egl_surface,
                  EGL_HEIGHT, &h);
  *width = w;
  *height = h;
#else
#error "Compile with -DPLATFORM=PLATFORM_<x> where <x> is NULL, GLX or X11_EGL."
#endif

  free(nw);
}

void WaffleInterface::InitOnce() {
  // Prevent multiple initializations.
  if (surface_)
    return;

  int32_t initAttribs[] = {
    WAFFLE_PLATFORM, PLATFORM_ENUM(PLATFORM),
    0
  };

  waffle_init(initAttribs);
  WAFFLE_CHECK_ERROR;

  display_ = waffle_display_connect(NULL);
  WAFFLE_CHECK_ERROR;

  int32_t configAttribs[] = {
    WAFFLE_CONTEXT_API,     GL_API,
    WAFFLE_RED_SIZE,        1,
    WAFFLE_GREEN_SIZE,      1,
    WAFFLE_BLUE_SIZE,       1,
    WAFFLE_ALPHA_SIZE,      1,
    WAFFLE_DEPTH_SIZE,      1,
    WAFFLE_STENCIL_SIZE,    1,
    WAFFLE_DOUBLE_BUFFERED, true,
    0
  };

  config_ = waffle_config_choose(display_, configAttribs);
  WAFFLE_CHECK_ERROR;

  if (g_width == -1 && g_height == -1) {
    const intptr_t attrib[] = {
      WAFFLE_WINDOW_FULLSCREEN, 1,
      0
    };
    surface_ = waffle_window_create2(config_, attrib);
    GetSurfaceSize(&g_width, &g_height);
  } else {
    surface_ = waffle_window_create(config_, g_width, g_height);
  }
  WAFFLE_CHECK_ERROR;

  waffle_window_show(surface_);
  WAFFLE_CHECK_ERROR;
}

bool WaffleInterface::Init() {
  InitOnce();

  context_ = CreateContext();
  CHECK(context_);

  waffle_make_current(display_, surface_, context_);
  WAFFLE_CHECK_ERROR;

#if defined(USE_OPENGL)
#define F(fun, type) fun = reinterpret_cast<type>(waffle_get_proc_address(#fun));
  LIST_PROC_FUNCTIONS(F)
#undef F
#endif

  return true;
}

void WaffleInterface::Cleanup() {
  waffle_make_current(display_, NULL, NULL);
  WAFFLE_CHECK_ERROR;

  waffle_context_destroy(context_);
  WAFFLE_CHECK_ERROR;
}

void WaffleInterface::SwapBuffers() {
  waffle_window_swap_buffers(surface_);
  WAFFLE_CHECK_ERROR;
}

bool WaffleInterface::SwapInterval(int interval) {
  return false;
}

bool WaffleInterface::MakeCurrent(const GLContext& context) {
  return waffle_make_current(display_, surface_, context);
}

const GLContext WaffleInterface::CreateContext() {
  return waffle_context_create(config_, NULL);
}

void WaffleInterface::CheckError() {
}

void WaffleInterface::DeleteContext(const GLContext& context) {
  waffle_context_destroy(context);
  WAFFLE_CHECK_ERROR;
}
