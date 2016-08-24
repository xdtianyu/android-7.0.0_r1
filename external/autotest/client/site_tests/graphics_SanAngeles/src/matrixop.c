// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "matrixop.h"

#include <math.h>

#define PI 3.1415926535897932384626433832795f


void Matrix4x4_Copy(Matrix4x4 dst, Matrix4x4 src)
{
    int i;
    for (i = 0; i < 16; ++i)
        dst[i] = src[i];
}


void Matrix4x4_Multiply(Matrix4x4 result, Matrix4x4 mat1, Matrix4x4 mat2)
{
    Matrix4x4 tmp;
    int i, j, k;
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; ++j) {
            tmp[i*4 + j] = 0;
            for (k = 0; k < 4; ++k)
                tmp[i*4 + j] += mat1[i*4 + k] * mat2[k*4 + j];
        }
    }
    Matrix4x4_Copy(result, tmp);
}


void Matrix4x4_LoadIdentity(Matrix4x4 mat)
{
    int i;
    for (i = 0; i < 16; ++i)
        mat[i] = 0;
    for (i = 0; i < 4; ++i)
        mat[i*4 + i] = 1.f;
}


void Matrix4x4_Scale(Matrix4x4 mat, float sx, float sy, float sz)
{
    int i;
    for (i = 0; i < 4; ++i)
    {
        mat[0*4 + i] *= sx;
        mat[1*4 + i] *= sy;
        mat[2*4 + i] *= sz;
    }
}


void Matrix4x4_Translate(Matrix4x4 mat, float tx, float ty, float tz)
{
    int i;
    for (i = 0; i < 4; ++i)
        mat[3*4 + i] += mat[0*4 + i] * tx + 
                        mat[1*4 + i] * ty +
                        mat[2*4 + i] * tz;
}


static float normalize(float *ax, float *ay, float *az)
{
    float norm = sqrtf((*ax) * (*ax) + (*ay) * (*ay) + (*az) * (*az));
    if (norm > 0)
    {
        *ax /= norm;
        *ay /= norm;
        *az /= norm;
    }
    return norm;
}


void Matrix4x4_Rotate(Matrix4x4 mat, float angle,
                      float ax, float ay, float az)
{
    Matrix4x4 rot;
    float r = angle * PI / 180.f;
    float s = sinf(r);
    float c = cosf(r);
    float one_c = 1.f - c;
    float xx, yy, zz, xy, yz, xz, xs, ys, zs;
    float norm = normalize(&ax, &ay, &az);

    if (norm == 0 || angle == 0)
        return;

    xx = ax * ax;
    yy = ay * ay;
    zz = az * az;
    xy = ax * ay;
    yz = ay * az;
    xz = ax * az;
    xs = ax * s;
    ys = ay * s;
    zs = az * s;

    rot[0*4 + 0] = xx + (1.f - xx) * c;
    rot[1*4 + 0] = xy * one_c - zs;
    rot[2*4 + 0] = xz * one_c + ys;
    rot[3*4 + 0] = 0.f;

    rot[0*4 + 1] = xy * one_c + zs;
    rot[1*4 + 1] = yy + (1.f - yy) * c;
    rot[2*4 + 1] = yz * one_c - xs;
    rot[3*4 + 1] = 0.f;

    rot[0*4 + 2] = xz * one_c - ys;
    rot[1*4 + 2] = yz * one_c + xs;
    rot[2*4 + 2] = zz + (1.f - zz) * c;
    rot[3*4 + 2] = 0.f;

    rot[0*4 + 3] = 0.f;
    rot[1*4 + 3] = 0.f;
    rot[2*4 + 3] = 0.f;
    rot[3*4 + 3] = 1.f;

    Matrix4x4_Multiply(mat, rot, mat);
}


void Matrix4x4_Frustum(Matrix4x4 mat,
                       float left, float right,
                       float bottom, float top,
                       float near, float far)
{
    float dx = right - left;
    float dy = top - bottom;
    float dz = far - near;
    Matrix4x4 frust;

    if (near <= 0.f || far <= 0.f || dx <= 0.f || dy <= 0.f || dz <= 0.f)
        return;

    frust[0*4 + 0] = 2.f * near / dx;
    frust[0*4 + 1] = 0.f;
    frust[0*4 + 2] = 0.f;
    frust[0*4 + 3] = 0.f;

    frust[1*4 + 0] = 0.f;
    frust[1*4 + 1] = 2.f * near / dy;
    frust[1*4 + 2] = 0.f;
    frust[1*4 + 3] = 0.f;

    frust[2*4 + 0] = (right + left) / dx;
    frust[2*4 + 1] = (top + bottom) / dy;
    frust[2*4 + 2] = -(near + far) / dz;
    frust[2*4 + 3] = -1.f;

    frust[3*4 + 0] = 0.f;
    frust[3*4 + 1] = 0.f;
    frust[3*4 + 2] = -2.f * near * far / dz;
    frust[3*4 + 3] = 0.f;

    Matrix4x4_Multiply(mat, frust, mat);
}


void Matrix4x4_Perspective(Matrix4x4 mat,
                           float fovy, float aspect,
                           float nearZ, float farZ)
{
    float frustumW, frustumH;
    frustumH = tanf(fovy / 360.f * PI) * nearZ;
    frustumW = frustumH * aspect;

    Matrix4x4_Frustum(mat, -frustumW, frustumW,
                      -frustumH, frustumH, nearZ, farZ);
}


void Matrix4x4_Transform(Matrix4x4 mat, float *x, float *y, float *z)
{
    float tx = mat[0*4 + 0] * (*x) + mat[1*4 + 0] * (*y) + mat[2*4 + 0] * (*z);
    float ty = mat[0*4 + 1] * (*x) + mat[1*4 + 1] * (*y) + mat[2*4 + 1] * (*z);
    float tz = mat[0*4 + 2] * (*x) + mat[1*4 + 2] * (*y) + mat[2*4 + 2] * (*z);
    *x = tx;
    *y = ty;
    *z = tz;
}

