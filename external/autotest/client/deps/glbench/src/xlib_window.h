// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BENCH_GL_XLIB_H_
#define BENCH_GL_XLIB_H_

#include <X11/Xlib.h>

extern Display *g_xlib_display;
extern Window g_xlib_window;

bool XlibInit();

#endif // BENCH_GL_XLIB_H_
