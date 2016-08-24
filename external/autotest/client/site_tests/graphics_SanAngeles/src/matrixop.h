// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Matrix implementation here is col-major, both storage-wise and
// operation-wise.

#ifndef MATRIXOP_H_INCLUDED
#define MATRIXOP_H_INCLUDED

typedef float Matrix4x4[16];
typedef float Matrix3x3[9];

// result = mat1 * mat2
extern void Matrix4x4_Multiply(Matrix4x4 result,
                               Matrix4x4 mat1, Matrix4x4 mat2);

// dst = src
extern void Matrix4x4_Copy(Matrix4x4 dst, Matrix4x4 src);

extern void Matrix4x4_LoadIdentity(Matrix4x4 mat);

// mat = ScaleMatrix(sx, sy, sz) * mat
extern void Matrix4x4_Scale(Matrix4x4 mat,
                            float sx, float sy, float sz);

// mat = TranslateMatrix(tx, ty, tz) * mat
extern void Matrix4x4_Translate(Matrix4x4 mat,
                                float tx, float ty, float tz);

// mat = RotateMatrix(angle, ax, ay, az) * mat
extern void Matrix4x4_Rotate(Matrix4x4 mat, float angle,
                             float ax, float ay, float az);

// mat = FrustumMatrix(left, right, bottom, top, near, far) * mat
extern void Matrix4x4_Frustum(Matrix4x4 mat,
                              float left, float right,
                              float bottom, float top,
                              float near, float far);

extern void Matrix4x4_Perspective(Matrix4x4 mat, float fovy, float aspect,
                                  float nearZ, float farZ);

// [x,y,z] = mat(3x3) * [x,y,z]
extern void Matrix4x4_Transform(Matrix4x4 mat, float *x, float *y, float *z);

#endif  // MATRIXOP_H_INCLUDED

