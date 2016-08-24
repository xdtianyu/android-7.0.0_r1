// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>

#include <GL/gl.h>
#include <GL/glx.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <string>

bool InitGraphics(Display** display,
                  Window* window,
                  GLXContext* context) {
  const int kWindowWidth = 100;
  const int kWindowHeight = 100;

  *display = XOpenDisplay(NULL);
  if (*display == NULL) {
    printf("ERROR: XOpenDisplay failed\n");
    return false;
  }

  Window root_window = DefaultRootWindow(*display);
  GLint att[] = { GLX_RGBA,
                  GLX_DEPTH_SIZE,
                  24,
                  None };
  XVisualInfo* vi = glXChooseVisual(*display, 0, att);
  if (vi == NULL) {
    printf("ERROR: glXChooseVisual failed\n");
    return false;
  }

  XSetWindowAttributes swa;
  swa.colormap = XCreateColormap(*display,
                                 root_window,
                                 vi->visual,
                                 AllocNone);
  *window = XCreateWindow(*display, root_window,
                          0, 0, kWindowWidth, kWindowHeight,
                          0, vi->depth, InputOutput, vi->visual,
                          CWColormap,
                          &swa);
  XMapWindow(*display, *window);

  *context = glXCreateContext(*display, vi, NULL, GL_TRUE);
  if (*context == NULL) {
    printf("ERROR: glXCreateContext failed\n");
  } else {
    glXMakeCurrent(*display, *window, *context);
  }

  XFree(vi);
  return (*context != NULL);
}

void ExitGraphics(Display* display,
                  Window window,
                  GLXContext context) {
  if (display != NULL) {
    glXMakeCurrent(display, None, NULL);
    if (context != NULL)
      glXDestroyContext(display, context);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
  }
}

bool GetGLVersion() {
  const GLubyte* version_string = glGetString(GL_VERSION);
  if (version_string == NULL) {
    printf("ERROR: glGetString(GL_VERSION) failed\n");
    return false;
  }
  printf("GL_VERSION = %s\n", version_string);
  return true;
}

bool GetGLExtensions() {
  const GLubyte* ext_string = glGetString(GL_EXTENSIONS);
  if (ext_string == NULL) {
    printf("ERROR: glGetString(GL_EXTENSIONS) failed\n");
    return false;
  }
  printf("GL_EXTENSIONS = %s\n", ext_string);
  return true;
}

bool GetGLXExtensions(Display* display) {
  const char* ext_string = glXQueryExtensionsString(display, 0);
  if (ext_string == NULL) {
    printf("ERROR: glXQueryExtensionsString failed\n");
    return false;
  }
  printf("GLX_EXTENSIONS = %s\n", ext_string);
  return true;
}

bool GetXExtensions(Display* display) {
  int ext_num;
  char** ext_list = XListExtensions(display, &ext_num);
  printf("X_EXTENSIONS =");
  for (int i = 0; i < ext_num; ++i) {
    printf(" %s", ext_list[i]);
  }
  printf("\n");
  XFreeExtensionList(ext_list);
  return true;
}

int main(int argc, char* argv[]) {
  // Initialize graphics.
  Display* display = NULL;
  Window window = 0;
  GLXContext context = NULL;
  bool rt_code = InitGraphics(&display, &window, &context);

  // Get OpenGL major/minor version number.
  if (rt_code)
    rt_code = GetGLVersion();

  // Get OpenGL extentions.
  if (rt_code)
    rt_code = GetGLExtensions();

  // Get GLX extensions.
  if (rt_code)
    rt_code = GetGLXExtensions(display);

  // Get X11 extensions.
  if (rt_code)
    rt_code = GetXExtensions(display);

  ExitGraphics(display, window, context);
  printf("SUCCEED: run to the end\n");
  return 0;
}

