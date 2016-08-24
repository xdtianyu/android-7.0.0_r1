// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <GL/glx.h>

#define IMPORTVBO_API
#define IMPORTVBO_FNPTRINIT = NULL
#include "importvbo.h"

#ifdef GLX_VERSION_1_4  // GLX_VERSION >= 1.4
#define GLEXT_GetProcAddress glXGetProcAddress
#else  // GLX_VERSION < 1.4
#define GLEXT_GetProcAddress glXGetProcAddressARB
#endif  // GLX_VERSION

int loadVBOProcs()
{
    FP_glGenBuffersARB = (FT_glGenBuffersARB)GLEXT_GetProcAddress(
                         (const GLubyte *)"glGenBuffersARB");
    FP_glBindBufferARB = (FT_glBindBufferARB)GLEXT_GetProcAddress(
                         (const GLubyte *)"glBindBufferARB");
    FP_glBufferDataARB = (FT_glBufferDataARB)GLEXT_GetProcAddress(
                         (const GLubyte *)"glBufferDataARB");
    FP_glBufferSubDataARB = (FT_glBufferSubDataARB)GLEXT_GetProcAddress(
                            (const GLubyte *)"glBufferSubDataARB");
    FP_glDeleteBuffersARB = (FT_glDeleteBuffersARB)GLEXT_GetProcAddress(
                            (const GLubyte *)"glDeleteBuffersARB");
    if (FP_glGenBuffersARB == NULL || FP_glBindBufferARB == NULL ||
        FP_glBufferDataARB == NULL || FP_glBufferSubDataARB == NULL ||
        FP_glDeleteBuffersARB == NULL)
        return 0;
    return 1;
}

