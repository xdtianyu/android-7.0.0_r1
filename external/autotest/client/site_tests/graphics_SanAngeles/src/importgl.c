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
 * $Id: importgl.c,v 1.4 2005/02/08 18:42:55 tonic Exp $
 * $Revision: 1.4 $
 */

#ifndef DISABLE_IMPORTGL

#include <stdlib.h>
#include <dlfcn.h>
#include "waffle.h"

#define IMPORTGL_NO_FNPTR_DEFS
#define IMPORTGL_API
#define IMPORTGL_FNPTRINIT = NULL
#include "importgl.h"


/* Imports function pointers to selected function calls in GLES DLL
 * or shared object. The function pointers are stored as global symbols with
 * equivalent function name but prefixed with "funcPtr_". Standard gl
 * calls are redirected to the function pointers with preprocessor macros
 * (see importgl.h).
 */
int importGLInit()
{
    int result = 1;

#undef IMPORT_FUNC_GL

#define IMPORT_FUNC_GL(funcName) do { \
        void *procAddress = waffle_dl_sym(WAFFLE_DL_OPENGL_ES2, #funcName); \
        if (procAddress == NULL) result = 0; \
        FNPTR(funcName) = (funcType_##funcName)procAddress; } while (0)

    IMPORT_FUNC_GL(glAttachShader);
    IMPORT_FUNC_GL(glBindBuffer);
    IMPORT_FUNC_GL(glBlendFunc);
    IMPORT_FUNC_GL(glBufferData);
    IMPORT_FUNC_GL(glBufferSubData);
    IMPORT_FUNC_GL(glClear);
    IMPORT_FUNC_GL(glClearColor);
    IMPORT_FUNC_GL(glCompileShader);
    IMPORT_FUNC_GL(glCreateProgram);
    IMPORT_FUNC_GL(glCreateShader);
    IMPORT_FUNC_GL(glDeleteBuffers);
    IMPORT_FUNC_GL(glDeleteProgram);
    IMPORT_FUNC_GL(glDeleteShader);
    IMPORT_FUNC_GL(glDisable);
    IMPORT_FUNC_GL(glDisableVertexAttribArray);
    IMPORT_FUNC_GL(glDrawArrays);
    IMPORT_FUNC_GL(glEnable);
    IMPORT_FUNC_GL(glEnableVertexAttribArray);
    IMPORT_FUNC_GL(glGenBuffers);
    IMPORT_FUNC_GL(glGetAttribLocation);
    IMPORT_FUNC_GL(glGetError);
    IMPORT_FUNC_GL(glGetShaderiv);
    IMPORT_FUNC_GL(glGetShaderInfoLog);
    IMPORT_FUNC_GL(glGetUniformLocation);
    IMPORT_FUNC_GL(glLinkProgram);
    IMPORT_FUNC_GL(glShaderSource);
    IMPORT_FUNC_GL(glUniform1f);
    IMPORT_FUNC_GL(glUniform3fv);
    IMPORT_FUNC_GL(glUniform4fv);
    IMPORT_FUNC_GL(glUniformMatrix3fv);
    IMPORT_FUNC_GL(glUniformMatrix4fv);
    IMPORT_FUNC_GL(glUseProgram);
    IMPORT_FUNC_GL(glVertexAttribPointer);
    IMPORT_FUNC_GL(glViewport);

    return result;
}

#endif  // !DISABLE_IMPORTGL

