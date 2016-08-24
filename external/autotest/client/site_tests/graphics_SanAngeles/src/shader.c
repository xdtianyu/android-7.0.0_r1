// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Adapted from the javascript implementation upon WebGL by kwaters@.

#include "shader.h"

#include <stdio.h>
#include <stdlib.h>

#include "shadersrc.h"

#undef IMPORTGL_API
#undef IMPORTGL_FNPTRINIT
#include "importgl.h"


SHADERLIT sShaderLit;
SHADERFLAT sShaderFlat;
SHADERFADE sShaderFade;

Matrix4x4 sModelView;
Matrix4x4 sProjection;


static void printShaderLog(GLuint shader)
{
    int infoLogSize, infoWritten;
    char *infoLog;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogSize);
    infoLog = malloc(infoLogSize);
    glGetShaderInfoLog(shader, infoLogSize, &infoWritten, infoLog);
    fprintf(stderr, "Error: glCompileShader failed: %s\n", infoLog);
    free(infoLog);
}


static GLuint createShader(const char *src, GLenum shaderType)
{
    GLint bShaderCompiled;
    GLuint shader = glCreateShader(shaderType);
    if (shader == 0)
        return 0;
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &bShaderCompiled);
    if (!bShaderCompiled)
    {
        printShaderLog(shader);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}


static GLuint createProgram(const char *srcVertex, const char * srcFragment)
{
    GLuint program = glCreateProgram();
    if (program == 0)
        return 0;

    GLuint shaderVertex = createShader(srcVertex, GL_VERTEX_SHADER);
    if (shaderVertex == 0)
    {
        glDeleteProgram(program);
        return 0;
    }
    glAttachShader(program, shaderVertex);
    glDeleteShader(shaderVertex);

    GLuint shaderFragment = createShader(srcFragment, GL_FRAGMENT_SHADER);
    if (shaderFragment == 0)
    {
        glDeleteProgram(program);
        return 0;
    }
    glAttachShader(program, shaderFragment);
    glDeleteShader(shaderFragment);

    glLinkProgram(program);
    return program;
}


static void computeNormalMatrix(Matrix4x4 m, Matrix3x3 normal)
{
    float det = m[0*4+0] * (m[1*4+1] * m[2*4+2] - m[2*4+1] * m[1*4+2]) -
                m[0*4+1] * (m[1*4+0] * m[2*4+2] - m[1*4+2] * m[2*4+0]) +
                m[0*4+2] * (m[1*4+0] * m[2*4+1] - m[1*4+1] * m[2*4+0]);
    float invDet = 1.f / det;
    normal[0*3+0] = invDet * (m[1*4+1] * m[2*4+2] - m[2*4+1] * m[1*4+2]);
    normal[1*3+0] = invDet * -(m[0*4+1] * m[2*4+2] - m[0*4+2] * m[2*4+1]);
    normal[2*3+0] = invDet * (m[0*4+1] * m[1*4+2] - m[0*4+2] * m[1*4+1]);
    normal[0*3+1] = invDet * -(m[1*4+0] * m[2*4+2] - m[1*4+2] * m[2*4+0]);
    normal[1*3+1] = invDet * (m[0*4+0] * m[2*4+2] - m[0*4+2] * m[2*4+0]);
    normal[2*3+1] = invDet * -(m[0*4+0] * m[1*4+2] - m[1*4+0] * m[0*4+2]);
    normal[0*3+2] = invDet * (m[1*4+0] * m[2*4+1] - m[2*4+0] * m[1*4+1]);
    normal[1*3+2] = invDet * -(m[0*4+0] * m[2*4+1] - m[2*4+0] * m[0*4+1]);
    normal[2*3+2] = invDet * (m[0*4+0] * m[1*4+1] - m[1*4+0] * m[0*4+1]);
}


static int getLocations()
{
    int rt = 1;
#define GET_ATTRIBUTE_LOC(programName, varName) \
        sShader##programName.varName = \
        glGetAttribLocation(sShader##programName.program, #varName); \
        if (sShader##programName.varName == -1) rt = 0
#define GET_UNIFORM_LOC(programName, varName) \
        sShader##programName.varName = \
        glGetUniformLocation(sShader##programName.program, #varName); \
        if (sShader##programName.varName == -1) rt = 0
    GET_ATTRIBUTE_LOC(Lit, pos);
    GET_ATTRIBUTE_LOC(Lit, normal);
    GET_ATTRIBUTE_LOC(Lit, colorIn);
    GET_UNIFORM_LOC(Lit, mvp);
    GET_UNIFORM_LOC(Lit, normalMatrix);
    GET_UNIFORM_LOC(Lit, ambient);
    GET_UNIFORM_LOC(Lit, shininess);
    GET_UNIFORM_LOC(Lit, light_0_direction);
    GET_UNIFORM_LOC(Lit, light_0_diffuse);
    GET_UNIFORM_LOC(Lit, light_0_specular);
    GET_UNIFORM_LOC(Lit, light_1_direction);
    GET_UNIFORM_LOC(Lit, light_1_diffuse);
    GET_UNIFORM_LOC(Lit, light_2_direction);
    GET_UNIFORM_LOC(Lit, light_2_diffuse);

    GET_ATTRIBUTE_LOC(Flat, pos);
    GET_ATTRIBUTE_LOC(Flat, colorIn);
    GET_UNIFORM_LOC(Flat, mvp);

    GET_ATTRIBUTE_LOC(Fade, pos);
    GET_UNIFORM_LOC(Fade, minFade);
#undef GET_ATTRIBUTE_LOC
#undef GET_UNIFORM_LOC
    return rt;
}


int initShaderPrograms()
{
    Matrix4x4_LoadIdentity(sModelView);
    Matrix4x4_LoadIdentity(sProjection);

    sShaderFlat.program = createProgram(sFlatVertexSource,
                                        sFlatFragmentSource);
    sShaderLit.program = createProgram(sLitVertexSource,
                                       sFlatFragmentSource);
    sShaderFade.program = createProgram(sFadeVertexSource,
                                        sFlatFragmentSource);
    if (sShaderFlat.program == 0 || sShaderLit.program == 0 ||
        sShaderFade.program == 0)
        return 0;

    return getLocations();
}


void deInitShaderPrograms()
{
    glDeleteProgram(sShaderFlat.program);
    glDeleteProgram(sShaderLit.program);
    glDeleteProgram(sShaderFade.program);
}


void bindShaderProgram(GLuint program)
{
    int loc_mvp = -1;
    int loc_normalMatrix = -1;

    glUseProgram(program);

    if (program == sShaderLit.program)
    {
        loc_mvp = sShaderLit.mvp;
        loc_normalMatrix = sShaderLit.normalMatrix;
    }
    else if (program == sShaderFlat.program)
    {
        loc_mvp = sShaderFlat.mvp;
    }

    if (loc_mvp != -1)
    {
        Matrix4x4 mvp;
        Matrix4x4_Multiply(mvp, sModelView, sProjection);
        glUniformMatrix4fv(loc_mvp, 1, GL_FALSE, (GLfloat *)mvp);
    }
    if (loc_normalMatrix != -1)
    {
        Matrix3x3 normalMatrix;
        computeNormalMatrix(sModelView, normalMatrix);
        glUniformMatrix3fv(loc_normalMatrix, 1, GL_FALSE,
                           (GLfloat *)normalMatrix);
    }
}

