/* San Angeles Observation OpenGL ES version example
 * Copyright 2004-2005 Jetro Lauha
 * All rights reserved.
 * Web: http://iki.fi/jetro/
 *
 * This source is free software; you can redistribute it and/or
 * modify it under the terms of EITHER:
 *   (1) The GNU Lesser General Public License as published by the Free
 *       Software Foundation; either version 2.1 of the License, or (at
 *       your option) any later version. The text of the GNU Lesser
 *       General Public License is included with this source in the
 *       file LICENSE-LGPL.txt.
 *   (2) The BSD-style license that is included with this source in
 *       the file LICENSE-BSD.txt.
 *
 * This source is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the files
 * LICENSE-LGPL.txt and LICENSE-BSD.txt for more details.
 *
 * $Id: importgl.h,v 1.4 2005/02/24 20:29:33 tonic Exp $
 * $Revision: 1.4 $
 */

#ifndef IMPORTGL_H_INCLUDED
#define IMPORTGL_H_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif

#include <GLES2/gl2.h>

/* Use DISABLE_IMPORTGL if you want to link the OpenGL ES at
 * compile/link time and not import it dynamically runtime.
 */
#ifndef DISABLE_IMPORTGL


/* Dynamically fetches pointers to the gl functions.
 * Should be called once on application initialization.
 * Returns non-zero on success and 0 on failure.
 */
extern int importGLInit();


#ifndef IMPORTGL_API
#define IMPORTGL_API extern
#endif
#ifndef IMPORTGL_FNPTRINIT
#define IMPORTGL_FNPTRINIT
#endif

#define FNDEF(retType, funcName, args) \
        IMPORTGL_API retType (*funcPtr_##funcName) args IMPORTGL_FNPTRINIT;\
        typedef retType (*funcType_##funcName) args


FNDEF(void, glAttachShader, (GLuint program, GLuint shader));
FNDEF(void, glBindBuffer, (GLenum target, GLuint buffer));
FNDEF(void, glBlendFunc, (GLenum sfactor, GLenum dfactor));
FNDEF(void, glBufferData, (GLenum target, GLsizeiptr size,
                           const void* data, GLenum usage));
FNDEF(void, glBufferSubData, (GLenum target, GLintptr offset,
                              GLsizeiptr size, const void* data));
FNDEF(void, glClear, (GLbitfield mask));
FNDEF(void, glClearColor, (GLclampf red, GLclampf green, GLclampf blue,
                           GLclampf alpha));
FNDEF(void, glCompileShader, (GLuint shader));
FNDEF(GLuint, glCreateProgram, (void));
FNDEF(GLuint, glCreateShader, (GLenum type));
FNDEF(void, glDeleteBuffers, (GLsizei n, const GLuint* buffers));
FNDEF(void, glDeleteProgram, (GLuint program));
FNDEF(void, glDeleteShader, (GLuint shader));
FNDEF(void, glDisable, (GLenum cap));
FNDEF(void, glDisableVertexAttribArray, (GLuint index));
FNDEF(void, glDrawArrays, (GLenum mode, GLint first, GLsizei count));
FNDEF(void, glEnable, (GLenum cap));
FNDEF(void, glEnableVertexAttribArray, (GLuint index));
FNDEF(void, glGenBuffers, (GLsizei n, GLuint* buffers));
FNDEF(int, glGetAttribLocation, (GLuint program, const char* name));
FNDEF(GLenum, glGetError, (void));
FNDEF(void, glGetShaderiv, (GLuint shader, GLenum pname, GLint* params));
FNDEF(void, glGetShaderInfoLog, (GLuint shader, GLsizei bufsize,
                                 GLsizei* length, char* infolog));
FNDEF(int, glGetUniformLocation, (GLuint program, const char* name));
FNDEF(void, glLinkProgram, (GLuint program));
FNDEF(void, glShaderSource, (GLuint shader, GLsizei count,
                             const char** string, const GLint* length));
FNDEF(void, glUniform1f, (GLint location, GLfloat x));
FNDEF(void, glUniform3fv, (GLint location, GLsizei count, const GLfloat* v));
FNDEF(void, glUniform4fv, (GLint location, GLsizei count, const GLfloat* v));
FNDEF(void, glUniformMatrix3fv, (GLint location, GLsizei count,
                                 GLboolean transpose, const GLfloat* value));
FNDEF(void, glUniformMatrix4fv, (GLint location, GLsizei count,
                                 GLboolean transpose, const GLfloat* value));
FNDEF(void, glUseProgram, (GLuint program));
FNDEF(void, glVertexAttribPointer, (GLuint indx, GLint size, GLenum type,
                                    GLboolean normalized, GLsizei stride,
                                    const void* ptr));
FNDEF(void, glViewport, (GLint x, GLint y, GLsizei width, GLsizei height));

#undef FN
#define FNPTR(name) funcPtr_##name

#ifndef IMPORTGL_NO_FNPTR_DEFS

// Redirect gl* function calls to funcPtr_gl*.

#define glAttachShader              FNPTR(glAttachShader)
#define glBindBuffer                FNPTR(glBindBuffer)
#define glBlendFunc                 FNPTR(glBlendFunc)
#define glBufferData                FNPTR(glBufferData)
#define glBufferSubData             FNPTR(glBufferSubData)
#define glClear                     FNPTR(glClear)
#define glClearColor                FNPTR(glClearColor)
#define glCompileShader             FNPTR(glCompileShader)
#define glCreateProgram             FNPTR(glCreateProgram)
#define glCreateShader              FNPTR(glCreateShader)
#define glDeleteBuffers             FNPTR(glDeleteBuffers)
#define glDeleteProgram             FNPTR(glDeleteProgram)
#define glDeleteShader              FNPTR(glDeleteShader)
#define glDisable                   FNPTR(glDisable)
#define glDisableVertexAttribArray  FNPTR(glDisableVertexAttribArray)
#define glDrawArrays                FNPTR(glDrawArrays)
#define glEnable                    FNPTR(glEnable)
#define glEnableVertexAttribArray   FNPTR(glEnableVertexAttribArray)
#define glGenBuffers                FNPTR(glGenBuffers)
#define glGetAttribLocation         FNPTR(glGetAttribLocation)
#define glGetError                  FNPTR(glGetError)
#define glGetShaderiv               FNPTR(glGetShaderiv)
#define glGetShaderInfoLog          FNPTR(glGetShaderInfoLog)
#define glGetUniformLocation        FNPTR(glGetUniformLocation)

#define glLinkProgram               FNPTR(glLinkProgram)
#define glShaderSource              FNPTR(glShaderSource)
#define glUniform1f                 FNPTR(glUniform1f)
#define glUniform3fv                FNPTR(glUniform3fv)
#define glUniform4fv                FNPTR(glUniform4fv)
#define glUniformMatrix3fv          FNPTR(glUniformMatrix3fv)
#define glUniformMatrix4fv          FNPTR(glUniformMatrix4fv)
#define glUseProgram                FNPTR(glUseProgram)
#define glViewport                  FNPTR(glViewport)
#define glVertexAttribPointer       FNPTR(glVertexAttribPointer)

#endif // !IMPORTGL_NO_FNPTR_DEFS


#endif // !DISABLE_IMPORTGL


#ifdef __cplusplus
}
#endif


#endif // !IMPORTGL_H_INCLUDED
