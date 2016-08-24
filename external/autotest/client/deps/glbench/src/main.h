// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BENCH_GL_MAIN_H_
#define BENCH_GL_MAIN_H_

#include <gflags/gflags.h>
#include <stdarg.h>
#include <sys/time.h>

#if defined(USE_OPENGLES)
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <GLES2/gl2ext.h>
#elif defined(USE_OPENGL)
#include <GL/gl.h>
#include <GL/glx.h>

#ifdef WORKAROUND_CROSBUG14304
#define LIST_PROC_FUNCTIONS(F) \
    F(glActiveTexture, PFNGLACTIVETEXTUREPROC) \
    F(glAttachShader, PFNGLATTACHSHADERPROC) \
    F(glBindBuffer, PFNGLBINDBUFFERPROC) \
    F(glBufferData, PFNGLBUFFERDATAPROC) \
    F(glCompileShader, PFNGLCOMPILESHADERPROC) \
    F(glCreateProgram, PFNGLCREATEPROGRAMPROC) \
    F(glCreateShader, PFNGLCREATESHADERPROC) \
    F(glDeleteBuffers, PFNGLDELETEBUFFERSPROC) \
    F(glDeleteProgram, PFNGLDELETEPROGRAMPROC) \
    F(glDeleteShader, PFNGLDELETESHADERPROC) \
    F(glDisableVertexAttribArray, PFNGLDISABLEVERTEXATTRIBARRAYPROC) \
    F(glEnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYPROC) \
    F(glGenBuffers, PFNGLGENBUFFERSPROC) \
    F(glGetAttribLocation, PFNGLGETATTRIBLOCATIONPROC) \
    F(glGetInfoLogARB, PFNGLGETPROGRAMINFOLOGPROC) \
    F(glGetProgramInfoLog, PFNGLGETPROGRAMINFOLOGPROC) \
    F(glGetShaderInfoLog, PFNGLGETSHADERINFOLOGPROC) \
    F(glGetUniformLocation, PFNGLGETUNIFORMLOCATIONPROC) \
    F(glLinkProgram, PFNGLLINKPROGRAMPROC) \
    F(glShaderSource, PFNGLSHADERSOURCEPROC) \
    F(glUniform1f, PFNGLUNIFORM1FPROC) \
    F(glUniform1i, PFNGLUNIFORM1IPROC) \
    F(glUniform4fv, PFNGLUNIFORM4FVPROC) \
    F(glUniformMatrix4fv, PFNGLUNIFORMMATRIX4FVPROC) \
    F(glUseProgram, PFNGLUSEPROGRAMPROC) \
    F(glVertexAttribPointer, PFNGLVERTEXATTRIBPOINTERPROC) \
    F(glXSwapIntervalSGI, PFNGLXSWAPINTERVALSGIPROC) \
    F(glXBindTexImageEXT, PFNGLXBINDTEXIMAGEEXTPROC) \
    F(glXReleaseTexImageEXT, PFNGLXRELEASETEXIMAGEEXTPROC)
#else
#define LIST_PROC_FUNCTIONS(F) \
    F(glAttachShader, PFNGLATTACHSHADERPROC) \
    F(glBindBuffer, PFNGLBINDBUFFERPROC) \
    F(glBindFramebuffer, PFNGLBINDFRAMEBUFFERPROC) \
    F(glBindRenderbuffer, PFNGLBINDRENDERBUFFERPROC) \
    F(glBufferData, PFNGLBUFFERDATAPROC) \
    F(glCheckFramebufferStatus, PFNGLCHECKFRAMEBUFFERSTATUSPROC) \
    F(glCompileShader, PFNGLCOMPILESHADERPROC) \
    F(glCreateProgram, PFNGLCREATEPROGRAMPROC) \
    F(glCreateShader, PFNGLCREATESHADERPROC) \
    F(glDeleteBuffers, PFNGLDELETEBUFFERSPROC) \
    F(glDeleteFramebuffers, PFNGLDELETEFRAMEBUFFERSPROC) \
    F(glDeleteProgram, PFNGLDELETEPROGRAMPROC) \
    F(glDeleteRenderbuffers, PFNGLDELETERENDERBUFFERSPROC) \
    F(glDeleteShader, PFNGLDELETESHADERPROC) \
    F(glDisableVertexAttribArray, PFNGLDISABLEVERTEXATTRIBARRAYPROC) \
    F(glEnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYPROC) \
    F(glFramebufferRenderbuffer, PFNGLFRAMEBUFFERRENDERBUFFERPROC) \
    F(glFramebufferTexture2D, PFNGLFRAMEBUFFERTEXTURE2DPROC) \
    F(glGenBuffers, PFNGLGENBUFFERSPROC) \
    F(glGenFramebuffers, PFNGLGENFRAMEBUFFERSPROC) \
    F(glGenRenderbuffers, PFNGLGENRENDERBUFFERSPROC) \
    F(glGetAttribLocation, PFNGLGETATTRIBLOCATIONPROC) \
    F(glGetInfoLogARB, PFNGLGETPROGRAMINFOLOGPROC) \
    F(glGetProgramInfoLog, PFNGLGETPROGRAMINFOLOGPROC) \
    F(glGetShaderInfoLog, PFNGLGETSHADERINFOLOGPROC) \
    F(glGetUniformLocation, PFNGLGETUNIFORMLOCATIONPROC) \
    F(glLinkProgram, PFNGLLINKPROGRAMPROC) \
    F(glRenderbufferStorage, PFNGLRENDERBUFFERSTORAGEPROC) \
    F(glShaderSource, PFNGLSHADERSOURCEPROC) \
    F(glUniform1f, PFNGLUNIFORM1FPROC) \
    F(glUniform1i, PFNGLUNIFORM1IPROC) \
    F(glUniform4fv, PFNGLUNIFORM4FVPROC) \
    F(glUniformMatrix4fv, PFNGLUNIFORMMATRIX4FVPROC) \
    F(glUseProgram, PFNGLUSEPROGRAMPROC) \
    F(glVertexAttribPointer, PFNGLVERTEXATTRIBPOINTERPROC) \
    F(glXBindTexImageEXT, PFNGLXBINDTEXIMAGEEXTPROC) \
    F(glXReleaseTexImageEXT, PFNGLXRELEASETEXIMAGEEXTPROC) \
    F(glXSwapIntervalSGI, PFNGLXSWAPINTERVALSGIPROC)
#endif

namespace gl {
#define F(fun, type) extern type fun;
LIST_PROC_FUNCTIONS(F)
#undef F
};

using namespace gl;
#else
#error bad graphics backend
#endif

inline uint64_t GetUTime() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return static_cast<uint64_t>(tv.tv_usec) +
    1000000ULL*static_cast<uint64_t>(tv.tv_sec);
}

extern bool g_verbose;
extern bool g_hasty;
extern GLint g_width;
extern GLint g_height;
extern GLint g_max_texture_size;

DECLARE_bool(override_redirect);

// This size is for a window that is very large but will fit on all
// the displays we care about.
#define WINDOW_WIDTH 512
#define WINDOW_HEIGHT 512

inline void dbg_printf(const char* format, ...) {
  if (!g_verbose)
    return;
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
}

#endif  // BENCH_GL_MAIN_H_
