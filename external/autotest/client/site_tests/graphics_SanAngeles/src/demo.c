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
 * $Id: demo.c,v 1.10 2005/02/08 20:54:39 tonic Exp $
 * $Revision: 1.10 $
 */

// The GLES2 implementation is adapted from the javascript implementation
// upon WebGL by kwaters@.

// The OpenGL implementation uses VBO extensions instead.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <assert.h>

#ifdef SAN_ANGELES_OBSERVATION_GLES
#undef IMPORTGL_API
#undef IMPORTGL_FNPTRINIT
#include "importgl.h"
#include "matrixop.h"
#include "shader.h"
#else  // SAN_ANGELES_OBSERVATION_GLES
#undef IMPORTVBO_API
#undef IMPORTVBO_FNPTRINIT
#include "importvbo.h"
#endif  // SAN_ANGELES_OBSERVATION_GLES | !SAN_ANGELES_OBSERVATION_GLES

#include "app.h"
#include "shapes.h"
#include "cams.h"


// Total run length is 20 * camera track base unit length (see cams.h).
#define RUN_LENGTH  (20 * CAMTRACK_LEN)
#undef PI
#define PI 3.1415926535897932f
#define RANDOM_UINT_MAX 65535


static unsigned long sRandomSeed = 0;

static void seedRandom(unsigned long seed)
{
    sRandomSeed = seed;
}

static unsigned long randomUInt()
{
    sRandomSeed = sRandomSeed * 0x343fd + 0x269ec3;
    return sRandomSeed >> 16;
}


// Definition of one GL object in this demo.
typedef struct {
    /* Vertex array and color array are enabled for all objects, so their
     * pointers must always be valid and non-NULL. Normal array is not
     * used by the ground plane, so when its pointer is NULL then normal
     * array usage is disabled.
     *
     * Vertex array is supposed to use GL_FIXED datatype and stride 0
     * (i.e. tightly packed array). Color array is supposed to have 4
     * components per color with GL_UNSIGNED_BYTE datatype and stride 0.
     * Normal array is supposed to use GL_FIXED datatype and stride 0.
     */
    GLfloat *vertexArray;
    GLint vertexArraySize;
    GLintptr vertexArrayOffset;
    GLubyte *colorArray;
    GLint colorArraySize;
    GLintptr colorArrayOffset;
    GLfloat *normalArray;
    GLint normalArraySize;
    GLintptr normalArrayOffset;
    GLint vertexComponents;
    GLsizei count;
#ifdef SAN_ANGELES_OBSERVATION_GLES
    GLuint shaderProgram;
#endif  // SAN_ANGELES_OBSERVATION_GLES
} GLOBJECT;


static long sStartTick = 0;
static long sTick = 0;

static int sCurrentCamTrack = 0;
static long sCurrentCamTrackStartTick = 0;
static long sNextCamTrackStartTick = 0x7fffffff;

static GLOBJECT *sSuperShapeObjects[SUPERSHAPE_COUNT] = { NULL };
static GLOBJECT *sGroundPlane = NULL;
static GLOBJECT *sFadeQuad = NULL;

static GLuint sVBO = 0;

typedef struct {
    float x, y, z;
} VECTOR3;


static void freeGLObject(GLOBJECT *object)
{
    if (object == NULL)
        return;

    free(object->normalArray);
    free(object->colorArray);
    free(object->vertexArray);

    free(object);
}


static GLOBJECT * newGLObject(long vertices, int vertexComponents,
                              int useColorArray, int useNormalArray)
{
    GLOBJECT *result;
    result = malloc(sizeof(GLOBJECT));
    if (result == NULL)
        return NULL;
    result->count = vertices;
    result->vertexComponents = vertexComponents;
    result->vertexArraySize = vertices * vertexComponents * sizeof(GLfloat);
    result->vertexArray = malloc(result->vertexArraySize);
    result->vertexArrayOffset = 0;
    if (useColorArray)
    {
        result->colorArraySize = vertices * 4 * sizeof(GLubyte);
        result->colorArray = malloc(result->colorArraySize);
    }
    else
    {
        result->colorArraySize = 0;
        result->colorArray = NULL;
    }
    result->colorArrayOffset = result->vertexArrayOffset +
                               result->vertexArraySize;
    if (useNormalArray)
    {
        result->normalArraySize = vertices * 3 * sizeof(GLfloat);
        result->normalArray = malloc(result->normalArraySize);
    }
    else
    {
        result->normalArraySize = 0;
        result->normalArray = NULL;
    }
    result->normalArrayOffset = result->colorArrayOffset +
                                result->colorArraySize;
    if (result->vertexArray == NULL ||
        (useColorArray && result->colorArray == NULL) ||
        (useNormalArray && result->normalArray == NULL))
    {
        freeGLObject(result);
        return NULL;
    }
#ifdef SAN_ANGELES_OBSERVATION_GLES
    result->shaderProgram = 0;
#endif  // SAN_ANGELES_OBSERVATION_GLES
    return result;
}


static void appendObjectVBO(GLOBJECT *object, GLint *offset)
{
    assert(object != NULL);

    object->vertexArrayOffset += *offset;
    object->colorArrayOffset += *offset;
    object->normalArrayOffset += *offset;
    *offset += object->vertexArraySize + object->colorArraySize +
               object->normalArraySize;

    glBufferSubData(GL_ARRAY_BUFFER, object->vertexArrayOffset,
                    object->vertexArraySize, object->vertexArray);
    if (object->colorArray)
        glBufferSubData(GL_ARRAY_BUFFER, object->colorArrayOffset,
                        object->colorArraySize, object->colorArray);
    if (object->normalArray)
        glBufferSubData(GL_ARRAY_BUFFER, object->normalArrayOffset,
                        object->normalArraySize, object->normalArray);

    free(object->normalArray);
    object->normalArray = NULL;
    free(object->colorArray);
    object->colorArray = NULL;
    free(object->vertexArray);
    object->vertexArray = NULL;
}


static GLuint createVBO(GLOBJECT **superShapes, int superShapeCount,
                        GLOBJECT *groundPlane, GLOBJECT *fadeQuad)
{
    GLuint vbo;
    GLint totalSize = 0;
    int a;
    for (a = 0; a < superShapeCount; ++a)
    {
        assert(superShapes[a] != NULL);
        totalSize += superShapes[a]->vertexArraySize +
                     superShapes[a]->colorArraySize +
                     superShapes[a]->normalArraySize;
    }
    totalSize += groundPlane->vertexArraySize +
                 groundPlane->colorArraySize +
                 groundPlane->normalArraySize;
    totalSize += fadeQuad->vertexArraySize +
                 fadeQuad->colorArraySize +
                 fadeQuad->normalArraySize;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, totalSize, 0, GL_STATIC_DRAW);
    GLint offset = 0;
    for (a = 0; a < superShapeCount; ++a)
        appendObjectVBO(superShapes[a], &offset);
    appendObjectVBO(groundPlane, &offset);
    appendObjectVBO(fadeQuad, &offset);
    assert(offset == totalSize);
    return vbo;
}


static void drawGLObject(GLOBJECT *object)
{
#ifdef SAN_ANGELES_OBSERVATION_GLES
    int loc_pos = -1;
    int loc_colorIn = -1;
    int loc_normal = -1;
#endif  // SAN_ANGELES_OBSERVATION_GLES

    assert(object != NULL);

#ifdef SAN_ANGELES_OBSERVATION_GLES
    bindShaderProgram(object->shaderProgram);
    if (object->shaderProgram == sShaderLit.program)
    {
        loc_pos = sShaderLit.pos;
        loc_colorIn = sShaderLit.colorIn;
        loc_normal = sShaderLit.normal;
    }
    else if (object->shaderProgram == sShaderFlat.program)
    {
        loc_pos = sShaderFlat.pos;
        loc_colorIn = sShaderFlat.colorIn;
    }
    else
    {
        assert(0);
    }
    glVertexAttribPointer(loc_pos, object->vertexComponents, GL_FLOAT,
                          GL_FALSE, 0, (GLvoid *)object->vertexArrayOffset);
    glEnableVertexAttribArray(loc_pos);
    glVertexAttribPointer(loc_colorIn, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0,
                          (GLvoid *)object->colorArrayOffset);
    glEnableVertexAttribArray(loc_colorIn);
    if (object->normalArraySize > 0)
    {
        glVertexAttribPointer(loc_normal, 3, GL_FLOAT, GL_FALSE, 0,
                              (GLvoid *)object->normalArrayOffset);
        glEnableVertexAttribArray(loc_normal);
    }
    glDrawArrays(GL_TRIANGLES, 0, object->count);

    if (object->normalArraySize > 0)
        glDisableVertexAttribArray(loc_normal);
    glDisableVertexAttribArray(loc_colorIn);
    glDisableVertexAttribArray(loc_pos);
#else  // !SAN_ANGELES_OBSERVATION_GLES
    glVertexPointer(object->vertexComponents, GL_FLOAT, 0,
                    (GLvoid *)object->vertexArrayOffset);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, (GLvoid *)object->colorArrayOffset);
    if (object->normalArraySize > 0)
    {
        glNormalPointer(GL_FLOAT, 0, (GLvoid *)object->normalArrayOffset);
        glEnableClientState(GL_NORMAL_ARRAY);
    }
    else
        glDisableClientState(GL_NORMAL_ARRAY);
    glDrawArrays(GL_TRIANGLES, 0, object->count);
#endif  // SAN_ANGELES_OBSERVATION_GLES | !SAN_ANGELES_OBSERVATION_GLES
}


static void vector3Sub(VECTOR3 *dest, VECTOR3 *v1, VECTOR3 *v2)
{
    dest->x = v1->x - v2->x;
    dest->y = v1->y - v2->y;
    dest->z = v1->z - v2->z;
}


static void superShapeMap(VECTOR3 *point, float r1, float r2, float t, float p)
{
    // sphere-mapping of supershape parameters
    point->x = (float)(cos(t) * cos(p) / r1 / r2);
    point->y = (float)(sin(t) * cos(p) / r1 / r2);
    point->z = (float)(sin(p) / r2);
}


static float ssFunc(const float t, const float *p)
{
    return (float)(pow(pow(fabs(cos(p[0] * t / 4)) / p[1], p[4]) +
                       pow(fabs(sin(p[0] * t / 4)) / p[2], p[5]), 1 / p[3]));
}


// Creates and returns a supershape object.
// Based on Paul Bourke's POV-Ray implementation.
// http://astronomy.swin.edu.au/~pbourke/povray/supershape/
static GLOBJECT * createSuperShape(const float *params)
{
    const int resol1 = (int)params[SUPERSHAPE_PARAMS - 3];
    const int resol2 = (int)params[SUPERSHAPE_PARAMS - 2];
    // latitude 0 to pi/2 for no mirrored bottom
    // (latitudeBegin==0 for -pi/2 to pi/2 originally)
    const int latitudeBegin = resol2 / 4;
    const int latitudeEnd = resol2 / 2;    // non-inclusive
    const int longitudeCount = resol1;
    const int latitudeCount = latitudeEnd - latitudeBegin;
    const long triangleCount = longitudeCount * latitudeCount * 2;
    const long vertices = triangleCount * 3;
    GLOBJECT *result;
    float baseColor[3];
    int a, longitude, latitude;
    long currentVertex, currentQuad;

    result = newGLObject(vertices, 3, 1, 1);
    if (result == NULL)
        return NULL;

    for (a = 0; a < 3; ++a)
        baseColor[a] = ((randomUInt() % 155) + 100) / 255.f;

    currentQuad = 0;
    currentVertex = 0;

    // longitude -pi to pi
    for (longitude = 0; longitude < longitudeCount; ++longitude)
    {

        // latitude 0 to pi/2
        for (latitude = latitudeBegin; latitude < latitudeEnd; ++latitude)
        {
            float t1 = -PI + longitude * 2 * PI / resol1;
            float t2 = -PI + (longitude + 1) * 2 * PI / resol1;
            float p1 = -PI / 2 + latitude * 2 * PI / resol2;
            float p2 = -PI / 2 + (latitude + 1) * 2 * PI / resol2;
            float r0, r1, r2, r3;

            r0 = ssFunc(t1, params);
            r1 = ssFunc(p1, &params[6]);
            r2 = ssFunc(t2, params);
            r3 = ssFunc(p2, &params[6]);

            if (r0 != 0 && r1 != 0 && r2 != 0 && r3 != 0)
            {
                VECTOR3 pa, pb, pc, pd;
                VECTOR3 v1, v2, n;
                float ca;
                int i;
                //float lenSq, invLenSq;

                superShapeMap(&pa, r0, r1, t1, p1);
                superShapeMap(&pb, r2, r1, t2, p1);
                superShapeMap(&pc, r2, r3, t2, p2);
                superShapeMap(&pd, r0, r3, t1, p2);

                // kludge to set lower edge of the object to fixed level
                if (latitude == latitudeBegin + 1)
                    pa.z = pb.z = 0;

                vector3Sub(&v1, &pb, &pa);
                vector3Sub(&v2, &pd, &pa);

                // Calculate normal with cross product.
                /*   i    j    k      i    j
                 * v1.x v1.y v1.z | v1.x v1.y
                 * v2.x v2.y v2.z | v2.x v2.y
                 */

                n.x = v1.y * v2.z - v1.z * v2.y;
                n.y = v1.z * v2.x - v1.x * v2.z;
                n.z = v1.x * v2.y - v1.y * v2.x;

                /* Pre-normalization of the normals is disabled here because
                 * they will be normalized anyway later due to automatic
                 * normalization (GL_NORMALIZE). It is enabled because the
                 * objects are scaled with glScale.
                 */
                /*
                lenSq = n.x * n.x + n.y * n.y + n.z * n.z;
                invLenSq = (float)(1 / sqrt(lenSq));
                n.x *= invLenSq;
                n.y *= invLenSq;
                n.z *= invLenSq;
                */

                ca = pa.z + 0.5f;

                for (i = currentVertex * 3;
                     i < (currentVertex + 6) * 3;
                     i += 3)
                {
                    result->normalArray[i] = n.x;
                    result->normalArray[i + 1] = n.y;
                    result->normalArray[i + 2] = n.z;
                }
                for (i = currentVertex * 4;
                     i < (currentVertex + 6) * 4;
                     i += 4)
                {
                    int a, color[3];
                    for (a = 0; a < 3; ++a)
                    {
                        color[a] = (int)(ca * baseColor[a] * 255);
                        if (color[a] > 255) color[a] = 255;
                    }
                    result->colorArray[i] = (GLubyte)color[0];
                    result->colorArray[i + 1] = (GLubyte)color[1];
                    result->colorArray[i + 2] = (GLubyte)color[2];
                    result->colorArray[i + 3] = 0;
                }
                result->vertexArray[currentVertex * 3] = pa.x;
                result->vertexArray[currentVertex * 3 + 1] = pa.y;
                result->vertexArray[currentVertex * 3 + 2] = pa.z;
                ++currentVertex;
                result->vertexArray[currentVertex * 3] = pb.x;
                result->vertexArray[currentVertex * 3 + 1] = pb.y;
                result->vertexArray[currentVertex * 3 + 2] = pb.z;
                ++currentVertex;
                result->vertexArray[currentVertex * 3] = pd.x;
                result->vertexArray[currentVertex * 3 + 1] = pd.y;
                result->vertexArray[currentVertex * 3 + 2] = pd.z;
                ++currentVertex;
                result->vertexArray[currentVertex * 3] = pb.x;
                result->vertexArray[currentVertex * 3 + 1] = pb.y;
                result->vertexArray[currentVertex * 3 + 2] = pb.z;
                ++currentVertex;
                result->vertexArray[currentVertex * 3] = pc.x;
                result->vertexArray[currentVertex * 3 + 1] = pc.y;
                result->vertexArray[currentVertex * 3 + 2] = pc.z;
                ++currentVertex;
                result->vertexArray[currentVertex * 3] = pd.x;
                result->vertexArray[currentVertex * 3 + 1] = pd.y;
                result->vertexArray[currentVertex * 3 + 2] = pd.z;
                ++currentVertex;
            } // r0 && r1 && r2 && r3
            ++currentQuad;
        } // latitude
    } // longitude

    // Set number of vertices in object to the actual amount created.
    result->count = currentVertex;
#ifdef SAN_ANGELES_OBSERVATION_GLES
    result->shaderProgram = sShaderLit.program;
#endif  // SAN_ANGELES_OBSERVATION_GLES
    return result;
}


static GLOBJECT * createGroundPlane()
{
    const int scale = 4;
    const int yBegin = -15, yEnd = 15;    // ends are non-inclusive
    const int xBegin = -15, xEnd = 15;
    const long triangleCount = (yEnd - yBegin) * (xEnd - xBegin) * 2;
    const long vertices = triangleCount * 3;
    GLOBJECT *result;
    int x, y;
    long currentVertex, currentQuad;

    result = newGLObject(vertices, 2, 1, 0);
    if (result == NULL)
        return NULL;

    currentQuad = 0;
    currentVertex = 0;

    for (y = yBegin; y < yEnd; ++y)
    {
        for (x = xBegin; x < xEnd; ++x)
        {
            GLubyte color;
            int i, a;
            color = (GLubyte)((randomUInt() & 0x5f) + 81);  // 101 1111
            for (i = currentVertex * 4; i < (currentVertex + 6) * 4; i += 4)
            {
                result->colorArray[i] = color;
                result->colorArray[i + 1] = color;
                result->colorArray[i + 2] = color;
                result->colorArray[i + 3] = 0;
            }

            // Axis bits for quad triangles:
            // x: 011100 (0x1c), y: 110001 (0x31)  (clockwise)
            // x: 001110 (0x0e), y: 100011 (0x23)  (counter-clockwise)
            for (a = 0; a < 6; ++a)
            {
                const int xm = x + ((0x1c >> a) & 1);
                const int ym = y + ((0x31 >> a) & 1);
                const float m = (float)(cos(xm * 2) * sin(ym * 4) * 0.75f);
                result->vertexArray[currentVertex * 2] = xm * scale + m;
                result->vertexArray[currentVertex * 2 + 1] = ym * scale + m;
                ++currentVertex;
            }
            ++currentQuad;
        }
    }
#ifdef SAN_ANGELES_OBSERVATION_GLES
    result->shaderProgram = sShaderFlat.program;
#endif  // SAN_ANGELES_OBSERVATION_GLES
    return result;
}


static void drawGroundPlane()
{
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO, GL_SRC_COLOR);
#ifndef SAN_ANGELES_OBSERVATION_GLES
    glDisable(GL_LIGHTING);
#endif  // !SAN_ANGELES_OBSERVATION_GLES

    drawGLObject(sGroundPlane);

#ifndef SAN_ANGELES_OBSERVATION_GLES
    glEnable(GL_LIGHTING);
#endif  // !SAN_ANGELES_OBSERVATION_GLES
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}


static GLOBJECT * createFadeQuad()
{
    static const GLfloat quadVertices[] = {
        -1, -1,
         1, -1,
        -1,  1,
         1, -1,
         1,  1,
        -1,  1
    };

    GLOBJECT *result;
    int i;

    result = newGLObject(6, 2, 0, 0);
    if (result == NULL)
        return NULL;

    for (i = 0; i < 12; ++i)
        result->vertexArray[i] = quadVertices[i];

#ifdef SAN_ANGELES_OBSERVATION_GLES
    result->shaderProgram = sShaderFade.program;
#endif  // SAN_ANGELES_OBSERVATION_GLES
    return result;
}


static void drawFadeQuad()
{
    const int beginFade = sTick - sCurrentCamTrackStartTick;
    const int endFade = sNextCamTrackStartTick - sTick;
    const int minFade = beginFade < endFade ? beginFade : endFade;

    if (minFade < 1024)
    {
        const GLfloat fadeColor = minFade / 1024.f;
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ZERO, GL_SRC_COLOR);
#ifdef SAN_ANGELES_OBSERVATION_GLES
        bindShaderProgram(sShaderFade.program);
        glUniform1f(sShaderFade.minFade, fadeColor);
        glVertexAttribPointer(sShaderFade.pos, 2, GL_FLOAT, GL_FALSE, 0,
                              (GLvoid *)sFadeQuad->vertexArrayOffset);
        glEnableVertexAttribArray(sShaderFade.pos);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(sShaderFade.pos);
#else  // !SAN_ANGELES_OBSERVATION_GLES
        glColor4f(fadeColor, fadeColor, fadeColor, 0);

        glDisable(GL_LIGHTING);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, (GLvoid *)sFadeQuad->vertexArrayOffset);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glEnableClientState(GL_COLOR_ARRAY);

        glMatrixMode(GL_MODELVIEW);

        glEnable(GL_LIGHTING);
#endif  // SAN_ANGELES_OBSERVATION_GLES | !SAN_ANGELES_OBSERVATION_GLES
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }
}


// Called from the app framework.
int appInit()
{
    int a;
    static GLfloat light0Diffuse[] = { 1.f, 0.4f, 0, 1.f };
    static GLfloat light1Diffuse[] = { 0.07f, 0.14f, 0.35f, 1.f };
    static GLfloat light2Diffuse[] = { 0.07f, 0.17f, 0.14f, 1.f };
    static GLfloat materialSpecular[] = { 1.f, 1.f, 1.f, 1.f };
#ifdef SAN_ANGELES_OBSERVATION_GLES
    static GLfloat lightAmbient[] = { 0.2f, 0.2f, 0.2f, 1.f };
#endif  // SAN_ANGELES_OBSERVATION_GLES

    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
#ifdef SAN_ANGELES_OBSERVATION_GLES
    if (initShaderPrograms() == 0)
    {
        fprintf(stderr, "Error: initShaderPrograms failed\n");
        return 0;
    }
#else  // !SAN_ANGELES_OBSERVATION_GLES
    glShadeModel(GL_FLAT);
    glEnable(GL_NORMALIZE);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHT2);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
#endif  // SAN_ANGELES_OBSERVATION_GLES | !SAN_ANGELES_OBSERVATION_GLES
    seedRandom(15);

    for (a = 0; a < SUPERSHAPE_COUNT; ++a)
    {
        sSuperShapeObjects[a] = createSuperShape(sSuperShapeParams[a]);
        assert(sSuperShapeObjects[a] != NULL);
    }
    sGroundPlane = createGroundPlane();
    assert(sGroundPlane != NULL);
    sFadeQuad = createFadeQuad();
    assert(sFadeQuad != NULL);
    sVBO = createVBO(sSuperShapeObjects, SUPERSHAPE_COUNT,
                     sGroundPlane, sFadeQuad);

    // setup non-changing lighting parameters
#ifdef SAN_ANGELES_OBSERVATION_GLES
    bindShaderProgram(sShaderLit.program);
    glUniform4fv(sShaderLit.ambient, 1, lightAmbient);
    glUniform4fv(sShaderLit.light_0_diffuse, 1, light0Diffuse);
    glUniform4fv(sShaderLit.light_1_diffuse, 1, light1Diffuse);
    glUniform4fv(sShaderLit.light_2_diffuse, 1, light2Diffuse);
    glUniform4fv(sShaderLit.light_0_specular, 1, materialSpecular);
    glUniform1f(sShaderLit.shininess, 60.f);
#else  // !SAN_ANGELES_OBSERVATION_GLES
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0Diffuse);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light1Diffuse);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, light2Diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, materialSpecular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 60);
#endif  // SAN_ANGELES_OBSERVATION_GLES | !SAN_ANGELES_OBSERVATION_GLES
    return 1;
}


// Called from the app framework.
void appDeinit()
{
    int a;
    for (a = 0; a < SUPERSHAPE_COUNT; ++a)
        freeGLObject(sSuperShapeObjects[a]);
    freeGLObject(sGroundPlane);
    freeGLObject(sFadeQuad);
    glDeleteBuffers(1, &sVBO);
#ifdef SAN_ANGELES_OBSERVATION_GLES
    deInitShaderPrograms();
#endif  // SAN_ANGELES_OBSERVATION_GLES
}

#ifndef SAN_ANGELES_OBSERVATION_GLES
static void gluPerspective(GLfloat fovy, GLfloat aspect,
                           GLfloat zNear, GLfloat zFar)
{
    GLfloat xmin, xmax, ymin, ymax;

    ymax = zNear * (GLfloat)tan(fovy * PI / 360);
    ymin = -ymax;
    xmin = ymin * aspect;
    xmax = ymax * aspect;

    glFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
}
#endif  // !SAN_ANGELES_OBSERVATION_GLES

static void prepareFrame(int width, int height)
{
    glViewport(0, 0, width, height);

    glClearColor(0.1f, 0.2f, 0.3f, 1.f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

#ifdef SAN_ANGELES_OBSERVATION_GLES
    Matrix4x4_LoadIdentity(sProjection);
    Matrix4x4_Perspective(sProjection,
                          45.f, (float)width / height, 0.5f, 150);

    Matrix4x4_LoadIdentity(sModelView);
#else  // !SAN_ANGELES_OBSERVATION_GLES
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45, (float)width / height, 0.5f, 150);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
#endif  // SAN_ANGELES_OBSERVATION_GLES | !SAN_ANGELES_OBSERVATION_GLES
}


static void configureLightAndMaterial()
{
    GLfloat light0Position[] = { -4.f, 1.f, 1.f, 0 };
    GLfloat light1Position[] = { 1.f, -2.f, -1.f, 0 };
    GLfloat light2Position[] = { -1.f, 0, -4.f, 0 };

#ifdef SAN_ANGELES_OBSERVATION_GLES
    Matrix4x4_Transform(sModelView,
                        light0Position, light0Position + 1, light0Position + 2);
    Matrix4x4_Transform(sModelView,
                        light1Position, light1Position + 1, light1Position + 2);
    Matrix4x4_Transform(sModelView,
                        light2Position, light2Position + 1, light2Position + 2);

    bindShaderProgram(sShaderLit.program);
    glUniform3fv(sShaderLit.light_0_direction, 1, light0Position);
    glUniform3fv(sShaderLit.light_1_direction, 1, light1Position);
    glUniform3fv(sShaderLit.light_2_direction, 1, light2Position);
#else  // !SAN_ANGELES_OBSERVATION_GLES
    glLightfv(GL_LIGHT0, GL_POSITION, light0Position);
    glLightfv(GL_LIGHT1, GL_POSITION, light1Position);
    glLightfv(GL_LIGHT2, GL_POSITION, light2Position);

    glEnable(GL_COLOR_MATERIAL);
#endif  // SAN_ANGELES_OBSERVATION_GLES | !SAN_ANGELES_OBSERVATION_GLES
}


static void drawModels(float zScale)
{
    const int translationScale = 9;
    int x, y;

    seedRandom(9);

#ifdef SAN_ANGELES_OBSERVATION_GLES
    Matrix4x4_Scale(sModelView, 1.f, 1.f, zScale);
#else  // !SAN_ANGELES_OBSERVATION_GLES
    glScalef(1.f, 1.f, zScale);
#endif  // SAN_ANGELES_OBSERVATION_GLES | !SAN_ANGELES_OBSERVATION_GLES

    for (y = -5; y <= 5; ++y)
    {
        for (x = -5; x <= 5; ++x)
        {
            float buildingScale;
#ifdef SAN_ANGELES_OBSERVATION_GLES
            Matrix4x4 tmp;
#endif  // SAN_ANGELES_OBSERVATION_GLES

            int curShape = randomUInt() % SUPERSHAPE_COUNT;
            buildingScale = sSuperShapeParams[curShape][SUPERSHAPE_PARAMS - 1];
#ifdef SAN_ANGELES_OBSERVATION_GLES
            Matrix4x4_Copy(tmp, sModelView);
            Matrix4x4_Translate(sModelView, x * translationScale,
                                y * translationScale, 0);
            Matrix4x4_Rotate(sModelView, randomUInt() % 360, 0, 0, 1.f);
            Matrix4x4_Scale(sModelView,
                            buildingScale, buildingScale, buildingScale);

            drawGLObject(sSuperShapeObjects[curShape]);
            Matrix4x4_Copy(sModelView, tmp);
#else  // !SAN_ANGELES_OBSERVATION_GLES
            glPushMatrix();
            glTranslatef(x * translationScale, y * translationScale, 0);
            glRotatef(randomUInt() % 360, 0, 0, 1.f);
            glScalef(buildingScale, buildingScale, buildingScale);

            drawGLObject(sSuperShapeObjects[curShape]);
            glPopMatrix();
#endif  // SAN_ANGELES_OBSERVATION_GLES | !SAN_ANGELES_OBSERVATION_GLES
        }
    }

    for (x = -2; x <= 2; ++x)
    {
        const int shipScale100 = translationScale * 500;
        const int offs100 = x * shipScale100 + (sTick % shipScale100);
        float offs = offs100 * 0.01f;
#ifdef SAN_ANGELES_OBSERVATION_GLES
        Matrix4x4 tmp;
        Matrix4x4_Copy(tmp, sModelView);
        Matrix4x4_Translate(sModelView, offs, -4.f, 2.f);
        drawGLObject(sSuperShapeObjects[SUPERSHAPE_COUNT - 1]);
        Matrix4x4_Copy(sModelView, tmp);
        Matrix4x4_Translate(sModelView, -4.f, offs, 4.f);
        Matrix4x4_Rotate(sModelView, 90.f, 0, 0, 1.f);
        drawGLObject(sSuperShapeObjects[SUPERSHAPE_COUNT - 1]);
        Matrix4x4_Copy(sModelView, tmp);
#else  // !SAN_ANGELES_OBSERVATION_GLES
        glPushMatrix();
        glTranslatef(offs, -4.f, 2.f);
        drawGLObject(sSuperShapeObjects[SUPERSHAPE_COUNT - 1]);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(-4.f, offs, 4.f);
        glRotatef(90.f, 0, 0, 1.f);
        drawGLObject(sSuperShapeObjects[SUPERSHAPE_COUNT - 1]);
        glPopMatrix();
#endif  // SAN_ANGELES_OBSERVATION_GLES | !SAN_ANGELES_OBSERVATION_GLES
    }
}

/* Following gluLookAt implementation is adapted from the
 * Mesa 3D Graphics library. http://www.mesa3d.org
 */
static void gluLookAt(GLfloat eyex, GLfloat eyey, GLfloat eyez,
	              GLfloat centerx, GLfloat centery, GLfloat centerz,
	              GLfloat upx, GLfloat upy, GLfloat upz)
{
#ifdef SAN_ANGELES_OBSERVATION_GLES
    Matrix4x4 m;
#else  // !SAN_ANGELES_OBSERVATION_GLES
    GLfloat m[16];
#endif  // SAN_ANGELES_OBSERVATION_GLES | !SAN_ANGELES_OBSERVATION_GLES
    GLfloat x[3], y[3], z[3];
    GLfloat mag;

    /* Make rotation matrix */

    /* Z vector */
    z[0] = eyex - centerx;
    z[1] = eyey - centery;
    z[2] = eyez - centerz;
    mag = (float)sqrt(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);
    if (mag) {			/* mpichler, 19950515 */
        z[0] /= mag;
        z[1] /= mag;
        z[2] /= mag;
    }

    /* Y vector */
    y[0] = upx;
    y[1] = upy;
    y[2] = upz;

    /* X vector = Y cross Z */
    x[0] = y[1] * z[2] - y[2] * z[1];
    x[1] = -y[0] * z[2] + y[2] * z[0];
    x[2] = y[0] * z[1] - y[1] * z[0];

    /* Recompute Y = Z cross X */
    y[0] = z[1] * x[2] - z[2] * x[1];
    y[1] = -z[0] * x[2] + z[2] * x[0];
    y[2] = z[0] * x[1] - z[1] * x[0];

    /* mpichler, 19950515 */
    /* cross product gives area of parallelogram, which is < 1.0 for
     * non-perpendicular unit-length vectors; so normalize x, y here
     */

    mag = (float)sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
    if (mag) {
        x[0] /= mag;
        x[1] /= mag;
        x[2] /= mag;
    }

    mag = (float)sqrt(y[0] * y[0] + y[1] * y[1] + y[2] * y[2]);
    if (mag) {
        y[0] /= mag;
        y[1] /= mag;
        y[2] /= mag;
    }

#ifdef SAN_ANGELES_OBSERVATION_GLES
#define M(row, col) m[col*4 + row]
    M(0, 0) = x[0];
    M(0, 1) = x[1];
    M(0, 2) = x[2];
    M(0, 3) = 0.0;
    M(1, 0) = y[0];
    M(1, 1) = y[1];
    M(1, 2) = y[2];
    M(1, 3) = 0.0;
    M(2, 0) = z[0];
    M(2, 1) = z[1];
    M(2, 2) = z[2];
    M(2, 3) = 0.0;
    M(3, 0) = 0.0;
    M(3, 1) = 0.0;
    M(3, 2) = 0.0;
    M(3, 3) = 1.0;
#undef M

    Matrix4x4_Multiply(sModelView, m, sModelView);

    Matrix4x4_Translate(sModelView, -eyex, -eyey, -eyez);
#else  // !SAN_ANGELES_OBSERVATION_GLES
#define M(row, col)  m[col*4 + row]
    M(0, 0) = x[0];
    M(0, 1) = x[1];
    M(0, 2) = x[2];
    M(0, 3) = 0.0;
    M(1, 0) = y[0];
    M(1, 1) = y[1];
    M(1, 2) = y[2];
    M(1, 3) = 0.0;
    M(2, 0) = z[0];
    M(2, 1) = z[1];
    M(2, 2) = z[2];
    M(2, 3) = 0.0;
    M(3, 0) = 0.0;
    M(3, 1) = 0.0;
    M(3, 2) = 0.0;
    M(3, 3) = 1.0;
#undef M

    glMultMatrixf(m);

    /* Translate Eye to Origin */
    glTranslatef(-eyex, -eyey, -eyez);
#endif  // SAN_ANGELES_OBSERVATION_GLES | !SAN_ANGELES_OBSERVATION_GLES
}

static void camTrack()
{
    float lerp[5];
    float eX, eY, eZ, cX, cY, cZ;
    float trackPos;
    CAMTRACK *cam;
    long currentCamTick;
    int a;

    if (sNextCamTrackStartTick <= sTick)
    {
        ++sCurrentCamTrack;
        sCurrentCamTrackStartTick = sNextCamTrackStartTick;
    }
    sNextCamTrackStartTick = sCurrentCamTrackStartTick +
                             sCamTracks[sCurrentCamTrack].len * CAMTRACK_LEN;

    cam = &sCamTracks[sCurrentCamTrack];
    currentCamTick = sTick - sCurrentCamTrackStartTick;
    trackPos = (float)currentCamTick / (CAMTRACK_LEN * cam->len);

    for (a = 0; a < 5; ++a)
        lerp[a] = (cam->src[a] + cam->dest[a] * trackPos) * 0.01f;

    if (cam->dist)
    {
        float dist = cam->dist * 0.1f;
        cX = lerp[0];
        cY = lerp[1];
        cZ = lerp[2];
        eX = cX - (float)cos(lerp[3]) * dist;
        eY = cY - (float)sin(lerp[3]) * dist;
        eZ = cZ - lerp[4];
    }
    else
    {
        eX = lerp[0];
        eY = lerp[1];
        eZ = lerp[2];
        cX = eX + (float)cos(lerp[3]);
        cY = eY + (float)sin(lerp[3]);
        cZ = eZ + lerp[4];
    }
    gluLookAt(eX, eY, eZ, cX, cY, cZ, 0, 0, 1);
}


// Called from the app framework.
/* The tick is current time in milliseconds, width and height
 * are the image dimensions to be rendered.
 */
void appRender(long tick, int width, int height)
{
#ifdef SAN_ANGELES_OBSERVATION_GLES
    Matrix4x4 tmp;
#endif  // SAN_ANGELES_OBSERVATION_GLES

    if (sStartTick == 0)
        sStartTick = tick;
    if (!gAppAlive)
        return;

    // Actual tick value is "blurred" a little bit.
    sTick = (sTick + tick - sStartTick) >> 1;

    // Terminate application after running through the demonstration once.
    if (sTick >= RUN_LENGTH)
    {
        gAppAlive = 0;
        return;
    }

    // Prepare OpenGL ES for rendering of the frame.
    prepareFrame(width, height);

    // Update the camera position and set the lookat.
    camTrack();

    // Configure environment.
    configureLightAndMaterial();

    // Draw the reflection by drawing models with negated Z-axis.
#ifdef SAN_ANGELES_OBSERVATION_GLES
    Matrix4x4_Copy(tmp, sModelView);
    drawModels(-1);
    Matrix4x4_Copy(sModelView, tmp);
#else  // !SAN_ANGELES_OBSERVATION_GLES
    glPushMatrix();
    drawModels(-1);
    glPopMatrix();
#endif  // SAN_ANGELES_OBSERVATION_GLES | !SAN_ANGELES_OBSERVATION_GLES

    // Blend the ground plane to the window.
    drawGroundPlane();

    // Draw all the models normally.
    drawModels(1);

    // Draw fade quad over whole window (when changing cameras).
    drawFadeQuad();
}

