// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Adapted from the javascript implementation upon WebGL by kwaters@.

#ifndef SHADER_H_INCLUDED
#define SHADER_H_INCLUDED

#include <GLES2/gl2.h>

#include "matrixop.h"

typedef struct {
    // program
    GLuint program;
    // attribute
    GLint pos;
    GLint normal;
    GLint colorIn;
    // uniform
    GLint mvp;
    GLint normalMatrix;
    GLint ambient;
    GLint shininess;
    GLint light_0_direction;
    GLint light_0_diffuse;
    GLint light_0_specular;
    GLint light_1_direction;
    GLint light_1_diffuse;
    GLint light_2_direction;
    GLint light_2_diffuse;
} SHADERLIT;

typedef struct {
    // program
    GLuint program;
    // attribute
    GLint pos;
    GLint colorIn;
    // uniform
    GLint mvp;
} SHADERFLAT;

typedef struct {
    // program
    GLuint program;
    // attribute
    GLint pos;
    // uniform
    GLint minFade;
} SHADERFADE;

extern Matrix4x4 sModelView;
extern Matrix4x4 sProjection;

extern SHADERLIT sShaderLit;
extern SHADERFLAT sShaderFlat;
extern SHADERFADE sShaderFade;

extern int initShaderPrograms();
extern void deInitShaderPrograms();
extern void bindShaderProgram(GLuint program);

#endif  // SHADER_H_INCLUDED

