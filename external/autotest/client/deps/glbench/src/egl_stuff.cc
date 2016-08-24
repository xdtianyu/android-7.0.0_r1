// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "egl_stuff.h"
#include "main.h"
#include "xlib_window.h"

scoped_ptr<GLInterface> g_main_gl_interface;

GLInterface* GLInterface::Create() {
  return new EGLInterface;
}

bool EGLInterface::Init() {
  if (!XlibInit())
    return false;

  EGLNativeWindowType native_window =
      static_cast<EGLNativeWindowType>(g_xlib_window);
  surface_ = eglCreateWindowSurface(display_, config_, native_window, NULL);
  CheckError();

  context_ = CreateContext();
  CheckError();

  eglMakeCurrent(display_, surface_, surface_, context_);
  CheckError();

  eglQuerySurface(display_, surface_, EGL_WIDTH, &g_width);
  eglQuerySurface(display_, surface_, EGL_HEIGHT, &g_height);

  return true;
}

void EGLInterface::Cleanup() {
  eglMakeCurrent(display_, NULL, NULL, NULL);
  DeleteContext(context_);
  eglDestroySurface(display_, surface_);
}

XVisualInfo* EGLInterface::GetXVisual() {
  if (!config_) {
    EGLint attribs[] = {
      EGL_RED_SIZE, 1,
      EGL_GREEN_SIZE, 1,
      EGL_BLUE_SIZE, 1,
      EGL_DEPTH_SIZE, 1,
      EGL_STENCIL_SIZE, 1,
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
      EGL_NONE
    };

    EGLNativeDisplayType native_display =
      static_cast<EGLNativeDisplayType>(g_xlib_display);

    display_ = eglGetDisplay(native_display);
    CheckError();

    eglInitialize(display_, NULL, NULL);
    CheckError();

    EGLint num_configs = -1;
    eglGetConfigs(display_, NULL, 0, &num_configs);
    CheckError();

    eglChooseConfig(display_, attribs, &config_, 1, &num_configs);
    CheckError();
  }

  // TODO: for some reason on some systems EGL_NATIVE_VISUAL_ID returns an ID
  // that XVisualIDFromVisual cannot find.  Use default visual until this is
  // resolved.
#if 0
  EGLint visual_id;
  eglGetConfigAttrib(display_, config_, EGL_NATIVE_VISUAL_ID, &visual_id);
  CheckError();
  XVisualInfo vinfo_template;
  vinfo_template.visualid = static_cast<VisualID>(visual_id);
#else
  XVisualInfo vinfo_template;
  vinfo_template.visualid = XVisualIDFromVisual(DefaultVisual(
      g_xlib_display, DefaultScreen(g_xlib_display)));
#endif

  int nitems = 0;
  XVisualInfo* ret = XGetVisualInfo(g_xlib_display, VisualIDMask,
                                    &vinfo_template, &nitems);
  CHECK(nitems == 1);
  return ret;
}

void EGLInterface::SwapBuffers() {
  eglSwapBuffers(display_, surface_);
}

bool EGLInterface::SwapInterval(int interval) {
  return (eglSwapInterval(display_, interval) == EGL_TRUE);
}

bool EGLInterface::MakeCurrent(const GLContext& context) {
  return eglMakeCurrent(display_, surface_, surface_, context);
}

const GLContext EGLInterface::CreateContext() {
  EGLint attribs[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };
  CHECK(display_ != EGL_NO_DISPLAY);
  CHECK(config_);
  return eglCreateContext(display_, config_, NULL, attribs);
}

void EGLInterface::CheckError() {
  CHECK_EQ(eglGetError(), EGL_SUCCESS);
}

void EGLInterface::DeleteContext(const GLContext& context) {
  eglDestroyContext(display_, context);
}

void EGLInterface::TerminateGL() {
  eglDestroySurface(display_, surface_);
  eglTerminate(display_);
}
