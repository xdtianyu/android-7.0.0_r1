// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// To use runtime linking, uncomment the #define OPENGL_ES_IMPORT_FUNCTIONS,
// and pass libGLESxx.so libEGLxx.so in the command line as input
// parameters.
// Otherwise, comment out #define OPENGL_ES_IMPORT_FUNCTIONS and pass no
// parameters in the command line.

#include <stdio.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define OPENGL_ES_IMPORT_FUNCTIONS

#ifdef OPENGL_ES_IMPORT_FUNCTIONS

#include <dlfcn.h>

EGLDisplay (*FP_eglGetDisplay)(NativeDisplayType display) = NULL;
EGLBoolean (*FP_eglInitialize)(EGLDisplay dpy,
                               EGLint* major,
                               EGLint* minor) = NULL;
EGLBoolean (*FP_eglGetConfigs)(EGLDisplay dpy,
                               EGLConfig* configs,
                               EGLint config_size,
                               EGLint* num_config) = NULL;
EGLBoolean (*FP_eglChooseConfig)(EGLDisplay dpy,
                                 const EGLint* attrib_list,
                                 EGLConfig* configs,
                                 EGLint config_size,
                                 EGLint* num_config) = NULL;
EGLContext (*FP_eglCreateContext)(EGLDisplay dpy,
                                  EGLConfig config,
                                  EGLContext share_list,
                                  const EGLint* attrib_list) = NULL;
EGLBoolean (*FP_eglGetConfigAttrib)(EGLDisplay dpy,
                                    EGLConfig config,
                                    EGLint attribute,
                                    EGLint* value) = NULL;
EGLSurface (*FP_eglCreateWindowSurface)(EGLDisplay dpy,
                                        EGLConfig config,
                                        NativeWindowType window,
                                        const EGLint* attrib_list) = NULL;
EGLBoolean (*FP_eglMakeCurrent)(EGLDisplay dpy,
                                EGLSurface draw,
                                EGLSurface read,
                                EGLContext ctx) = NULL;
EGLBoolean (*FP_eglDestroyContext)(EGLDisplay dpy, EGLContext ctx) = NULL;
EGLBoolean (*FP_eglDestroySurface)(EGLDisplay dpy, EGLSurface surface) = NULL;
EGLBoolean (*FP_eglTerminate)(EGLDisplay dpy) = NULL;
const char* (*FP_eglQueryString)(EGLDisplay dpy, EGLint name) = NULL;
const GLubyte* (*FP_glGetString)(GLenum name) = NULL;

#define eglGetDisplay FP_eglGetDisplay
#define eglInitialize FP_eglInitialize
#define eglGetConfigs FP_eglGetConfigs
#define eglChooseConfig FP_eglChooseConfig
#define eglCreateContext FP_eglCreateContext
#define eglGetConfigAttrib FP_eglGetConfigAttrib
#define eglCreateWindowSurface FP_eglCreateWindowSurface
#define eglMakeCurrent FP_eglMakeCurrent
#define eglDestroyContext FP_eglDestroyContext
#define eglDestroySurface FP_eglDestroySurface
#define eglTerminate FP_eglTerminate
#define eglQueryString FP_eglQueryString
#define glGetString FP_glGetString

typedef EGLDisplay (*FT_eglGetDisplay)(NativeDisplayType);
typedef EGLBoolean (*FT_eglInitialize)(EGLDisplay, EGLint*, EGLint*);
typedef EGLBoolean (*FT_eglGetConfigs)(EGLDisplay, EGLConfig*,
                                       EGLint, EGLint*);
typedef EGLBoolean (*FT_eglChooseConfig)(EGLDisplay, const EGLint*,
                                         EGLConfig*, EGLint, EGLint*);
typedef EGLContext (*FT_eglCreateContext)(EGLDisplay, EGLConfig,
                                          EGLContext, const EGLint*);
typedef EGLBoolean (*FT_eglGetConfigAttrib)(EGLDisplay, EGLConfig,
                                            EGLint, EGLint*);
typedef EGLSurface (*FT_eglCreateWindowSurface)(EGLDisplay, EGLConfig,
                                                NativeWindowType,
                                                const EGLint*);
typedef EGLBoolean (*FT_eglMakeCurrent)(EGLDisplay, EGLSurface,
                                        EGLSurface, EGLContext);
typedef EGLBoolean (*FT_eglDestroyContext)(EGLDisplay, EGLContext);
typedef EGLBoolean (*FT_eglDestroySurface)(EGLDisplay, EGLSurface);
typedef EGLBoolean (*FT_eglTerminate)(EGLDisplay);
typedef const char* (*FT_eglQueryString)(EGLDisplay, EGLint);
typedef const GLubyte* (*FT_glGetString)(GLenum);

bool LoadDLFunction(void** func_handle,
                    const char* func_name,
                    void* dl_handle) {
  *func_handle = dlsym(dl_handle, func_name);
  if (*func_handle == NULL) {
    printf("ERROR: fail to load %s\n", func_name);
    return false;
  }
  return true;
}

bool EntryImportGL(char* lib_gles, char* lib_egl,
                   void** handle_gles, void** handle_egl) {
  *handle_gles = dlopen(lib_gles, RTLD_LAZY);
  if (*handle_gles == NULL) {
    printf("ERROR: %s\n", dlerror());
    return false;
  }
  *handle_egl = dlopen(lib_egl, RTLD_LAZY);
  if (*handle_egl == NULL) {
    printf("ERROR: %s\n", dlerror());
    return false;
  }

  bool rt = true;
  void* tmp;
  rt &= LoadDLFunction(&tmp, "eglGetDisplay", *handle_egl);
  FP_eglGetDisplay = reinterpret_cast<FT_eglGetDisplay>(tmp);
  rt &= LoadDLFunction(&tmp, "eglInitialize", *handle_egl);
  FP_eglInitialize = reinterpret_cast<FT_eglInitialize>(tmp);
  rt &= LoadDLFunction(&tmp, "eglGetConfigs", *handle_egl);
  FP_eglGetConfigs = reinterpret_cast<FT_eglGetConfigs>(tmp);
  rt &= LoadDLFunction(&tmp, "eglChooseConfig", *handle_egl);
  FP_eglChooseConfig = reinterpret_cast<FT_eglChooseConfig>(tmp);
  rt &= LoadDLFunction(&tmp, "eglCreateContext", *handle_egl);
  FP_eglCreateContext = reinterpret_cast<FT_eglCreateContext>(tmp);
  rt &= LoadDLFunction(&tmp, "eglGetConfigAttrib", *handle_egl);
  FP_eglGetConfigAttrib = reinterpret_cast<FT_eglGetConfigAttrib>(tmp);
  rt &= LoadDLFunction(&tmp, "eglCreateWindowSurface", *handle_egl);
  FP_eglCreateWindowSurface = reinterpret_cast<FT_eglCreateWindowSurface>(tmp);
  rt &= LoadDLFunction(&tmp, "eglMakeCurrent", *handle_egl);
  FP_eglMakeCurrent = reinterpret_cast<FT_eglMakeCurrent>(tmp);
  rt &= LoadDLFunction(&tmp, "eglDestroyContext", *handle_egl);
  FP_eglDestroyContext = reinterpret_cast<FT_eglDestroyContext>(tmp);
  rt &= LoadDLFunction(&tmp, "eglDestroySurface", *handle_egl);
  FP_eglDestroySurface = reinterpret_cast<FT_eglDestroySurface>(tmp);
  rt &= LoadDLFunction(&tmp, "eglTerminate", *handle_egl);
  FP_eglTerminate = reinterpret_cast<FT_eglTerminate>(tmp);
  rt &= LoadDLFunction(&tmp, "eglQueryString", *handle_egl);
  FP_eglQueryString = reinterpret_cast<FT_eglQueryString>(tmp);
  rt &= LoadDLFunction(&tmp, "glGetString", *handle_gles);
  FP_glGetString = reinterpret_cast<FT_glGetString>(tmp);
  return rt;
}

void ExitImportGL(void* handle_gles, void* handle_egl) {
  if (handle_gles != NULL)
    dlclose(handle_gles);
  if (handle_egl != NULL)
    dlclose(handle_egl);
}

#endif  // OPENGL_ES_IMPORT_FUNCTIONS

bool InitGraphics(Display** display,
                  EGLDisplay* egl_display,
                  EGLContext* egl_context,
                  EGLSurface* egl_surface) {
  const int kWindowWidth = 100;
  const int kWindowHeight = 100;
  const EGLint config_attribs[] = {
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
  };
  const EGLint context_attribs[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };

  // XWindow init.
  *display = XOpenDisplay(NULL);
  if (*display == NULL) {
    printf("ERROR: XOpenDisplay failed\n");
    return false;
  }

  int screen = XDefaultScreen(*display);
  Window window = XCreateSimpleWindow(*display, RootWindow(*display, screen),
                                      0, 0, kWindowWidth, kWindowHeight,
                                      0, 0, WhitePixel(*display, screen));
  XMapWindow(*display, window);
  XSync(*display, True);

  // EGL init.
  *egl_display = eglGetDisplay((EGLNativeDisplayType)*display);
  EGLint no_major, no_minor;
  EGLBoolean rt_code = eglInitialize(*egl_display, &no_major, &no_minor);
  if (rt_code == EGL_FALSE) {
    printf("ERROR: eglInitialize failed\n");
    return false;
  }
  // Print out version info.
  printf("EGL_VERSION = %d.%d\n",
         static_cast<int>(no_major),
         static_cast<int>(no_minor));
  // Config.
  EGLint num_configs;
  EGLConfig egl_config;
  rt_code = eglChooseConfig(*egl_display, config_attribs,
                            &egl_config, 1, &num_configs);
  if (rt_code == EGL_FALSE || num_configs != 1) {
    printf("ERROR: eglChooseConfig failed\n");
    return false;
  }
  // Surface.
  *egl_surface = eglCreateWindowSurface(*egl_display, egl_config,
                                        (NativeWindowType)window, NULL);
  if (*egl_surface == EGL_NO_SURFACE) {
    printf("ERROR: eglCreateWindowSurface failed\n");
    return false;
  }
  // Context.
  *egl_context = eglCreateContext(*egl_display, egl_config, EGL_NO_CONTEXT,
                                  context_attribs);
  if (*egl_context == EGL_NO_CONTEXT) {
    printf("ERROR: eglCreateContext failed\n");
    return false;
  }
  // Make current.
  rt_code = eglMakeCurrent(*egl_display, *egl_surface,
                           *egl_surface, *egl_context);
  if (rt_code == EGL_FALSE) {
    printf("ERROR: eglMakeCurrent failed\n");
    return false;
  }
  return true;
}

void ExitGraphics(EGLDisplay egl_display,
                  EGLContext egl_context,
                  EGLSurface egl_surface) {
  if (egl_display != EGL_NO_DISPLAY) {
    eglMakeCurrent(egl_display, NULL, NULL, NULL);
    if (egl_context != EGL_NO_CONTEXT)
      eglDestroyContext(egl_display, egl_context);
    if (egl_surface != EGL_NO_SURFACE)
      eglDestroySurface(egl_display, egl_surface);
    eglTerminate(egl_display);
  }
}

bool GetGLESVersion() {
  const GLubyte* version_string = glGetString(GL_VERSION);
  if (version_string == NULL) {
    printf("ERROR: glGetString(GL_VERSION) failed\n");
    return false;
  }
  printf("GLES_VERSION = %s\n", version_string);
  return true;
}

bool GetGLESExtensions() {
  const GLubyte* ext_string = glGetString(GL_EXTENSIONS);
  if (ext_string == NULL) {
    printf("ERROR: glGetString(GL_EXTENSIONS) failed\n");
    return false;
  }
  printf("GLES_EXTENSIONS = %s\n", ext_string);
  return true;
}

bool GetEGLExtensions(EGLDisplay egl_display) {
  const char* ext_string = eglQueryString(egl_display, EGL_EXTENSIONS);
  if (ext_string == NULL) {
    printf("ERROR: eglQueryString(EGL_EXTENSIONS) failed\n");
    return false;
  }
  printf("EGL_EXTENSIONS = %s\n", ext_string);
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
  Display* display;
  EGLDisplay egl_display = EGL_NO_DISPLAY;
  EGLContext egl_context = EGL_NO_CONTEXT;
  EGLSurface egl_surface = EGL_NO_SURFACE;

  bool rt_code = true;

#ifdef OPENGL_ES_IMPORT_FUNCTIONS
  if (argc != 3) {
    printf("ERROR: Usage: gles_APICheck libGLESxx.so libEGLxx.so\n");
    return 0;
  }
  void* handle_gles = NULL;
  void* handle_egl = NULL;
  rt_code = EntryImportGL(argv[1], argv[2], &handle_gles, &handle_egl);
#endif  // OPENGL_ES_IMPORT_FUNCTIONS

  // EGL version is printed out in InitGraphics
  if (rt_code)
    rt_code = InitGraphics(&display, &egl_display,
                           &egl_context, &egl_surface);

  // Get GLES version.
  if (rt_code)
    rt_code = GetGLESVersion();

  // Get GLES extentions.
  if (rt_code)
    rt_code = GetGLESExtensions();

  // Get EGL extentions.
  if (rt_code)
    rt_code = GetEGLExtensions(egl_display);

  // Get X11 extensions.
  if (rt_code)
    rt_code = GetXExtensions(display);

  ExitGraphics(egl_display, egl_context, egl_surface);
#ifdef OPENGL_ES_IMPORT_FUNCTIONS
  ExitImportGL(handle_gles, handle_egl);
#endif  // OPENGL_ES_IMPORT_FUNCTIONS
  printf("SUCCEED: run to the end\n");
  return 0;
}

