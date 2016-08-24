// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IMPORTVBO_H_INCLUDED
#define IMPORTVBO_H_INCLUDED

#include <GL/gl.h>
#include <GL/glext.h>

#ifndef IMPORTVBO_API
#define IMPORTVBO_API extern
#endif  // IMPORTVBO_API

#ifndef IMPORTVBO_FNPTRINIT
#define IMPORTVBO_FNPTRINIT
#endif  // IMPORTVBO_FNPTRINT

IMPORTVBO_API void (*FP_glGenBuffersARB)(GLsizei, GLuint *) IMPORTVBO_FNPTRINIT;
IMPORTVBO_API void (*FP_glBindBufferARB)(GLenum, GLuint) IMPORTVBO_FNPTRINIT;
IMPORTVBO_API void (*FP_glBufferDataARB)(GLenum, GLsizeiptrARB, const GLvoid *, GLenum) IMPORTVBO_FNPTRINIT;
IMPORTVBO_API void (*FP_glBufferSubDataARB)(GLenum, GLintptrARB, GLsizeiptrARB, const GLvoid *) IMPORTVBO_FNPTRINIT;
IMPORTVBO_API void (*FP_glDeleteBuffersARB)(GLsizei, const GLuint *) IMPORTVBO_FNPTRINIT;

typedef void (*FT_glGenBuffersARB)(GLsizei, GLuint *);
typedef void (*FT_glBindBufferARB)(GLenum, GLuint);
typedef void (*FT_glBufferDataARB)(GLenum, GLsizeiptrARB, const GLvoid *, GLenum);
typedef void (*FT_glBufferSubDataARB)(GLenum, GLintptrARB, GLsizeiptrARB, const GLvoid *);
typedef void (*FT_glDeleteBuffersARB)(GLsizei, const GLuint *);

// We define gl functions without the ARB postfix so GL and GLES code
// will have the same look.
#define glGenBuffers FP_glGenBuffersARB
#define glBindBuffer FP_glBindBufferARB
#define glBufferData FP_glBufferDataARB
#define glBufferSubData FP_glBufferSubDataARB
#define glDeleteBuffers FP_glDeleteBuffersARB

extern int loadVBOProcs();

#endif  // IMPORTVBO_H_INCLUDED

