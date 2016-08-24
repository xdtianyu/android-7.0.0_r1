/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MAT_H_

#define MAT_H_

#include "vec.h"
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Mat33 {
    float elem[3][3];

};

struct Size3 {
    uint32_t elem[3];

};

struct Mat44 {
    float elem[4][4];

};

struct Size4 {
    uint32_t elem[4];

};

void initZeroMatrix(struct Mat33 *A);
void initDiagonalMatrix(struct Mat33 *A, float x);

void initMatrixColumns(
        struct Mat33 *A, const struct Vec3 *v1, const struct Vec3 *v2, const struct Vec3 *v3);

void mat33Apply(struct Vec3 *out, const struct Mat33 *A, const struct Vec3 *v);
void mat33Multiply(struct Mat33 *out, const struct Mat33 *A, const struct Mat33 *B);
void mat33ScalarMul(struct Mat33 *A, float c);

void mat33Add(struct Mat33 *out, const struct Mat33 *A);
void mat33Sub(struct Mat33 *out, const struct Mat33 *A);

int mat33IsPositiveSemidefinite(const struct Mat33 *A, float tolerance);

// out = A^(-1)
void mat33Invert(struct Mat33 *out, const struct Mat33 *A);

// out = A^T B
void mat33MultiplyTransposed(
        struct Mat33 *out, const struct Mat33 *A, const struct Mat33 *B);

// out = A B^T
void mat33MultiplyTransposed2(
        struct Mat33 *out, const struct Mat33 *A, const struct Mat33 *B);

// out = A^T
void mat33Transpose(struct Mat33 *out, const struct Mat33 *A);

void mat33DecomposeLup(struct Mat33 *LU, struct Size3 *pivot);

void mat33SwapRows(struct Mat33 *A, const uint32_t i, const uint32_t j);

void mat33Solve(const struct Mat33 *A, struct Vec3 *x, const struct Vec3 *b, const struct Size3 *pivot);

void mat33GetEigenbasis(struct Mat33 *S, struct Vec3 *eigenvals, struct Mat33 *eigenvecs);

uint32_t mat33Maxind(const struct Mat33 *A, uint32_t k);

void mat33Rotate(struct Mat33 *A, float c, float s, uint32_t k, uint32_t l, uint32_t i, uint32_t j);

void mat44Apply(struct Vec4 *out, const struct Mat44 *A, const struct Vec4 *v);

void mat44DecomposeLup(struct Mat44 *LU, struct Size4 *pivot);

void mat44SwapRows(struct Mat44 *A, const uint32_t i, const uint32_t j);

void mat44Solve(const struct Mat44 *A, struct Vec4 *x, const struct Vec4 *b, const struct Size4 *pivot);

#ifdef __cplusplus
}
#endif

#endif  // MAT_H_
