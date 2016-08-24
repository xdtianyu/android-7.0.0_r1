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

#include <algos/mag_cal.h>
#include <algos/mat.h>

#include <errno.h>
#include <nanohub_math.h>
#include <string.h>

#define MAX_EIGEN_RATIO     25.0f
#define MAX_EIGEN_MAG       80.0f   // uT
#define MIN_EIGEN_MAG       10.0f   // uT

#define MAX_FIT_MAG         80.0f
#define MIN_FIT_MAG         10.0f

#define MIN_BATCH_WINDOW    1000000UL   // 1 sec
#define MAX_BATCH_WINDOW    15000000UL  // 15 sec
#define MIN_BATCH_SIZE      25      // samples

// eigen value magnitude and ratio test
static int moc_eigen_test(struct MagCal *moc)
{
    // covariance matrix
    struct Mat33 S;
    S.elem[0][0] = moc->acc_xx - moc->acc_x * moc->acc_x;
    S.elem[0][1] = S.elem[1][0] = moc->acc_xy - moc->acc_x * moc->acc_y;
    S.elem[0][2] = S.elem[2][0] = moc->acc_xz - moc->acc_x * moc->acc_z;
    S.elem[1][1] = moc->acc_yy - moc->acc_y * moc->acc_y;
    S.elem[1][2] = S.elem[2][1] = moc->acc_yz - moc->acc_y * moc->acc_z;
    S.elem[2][2] = moc->acc_zz - moc->acc_z * moc->acc_z;

    struct Vec3 eigenvals;
    struct Mat33 eigenvecs;
    mat33GetEigenbasis(&S, &eigenvals, &eigenvecs);

    float evmax = (eigenvals.x > eigenvals.y) ? eigenvals.x : eigenvals.y;
    evmax = (eigenvals.z > evmax) ? eigenvals.z : evmax;

    float evmin = (eigenvals.x < eigenvals.y) ? eigenvals.x : eigenvals.y;
    evmin = (eigenvals.z < evmin) ? eigenvals.z : evmin;

    float evmag = sqrtf(eigenvals.x + eigenvals.y + eigenvals.z);

    int eigen_pass = (evmin * MAX_EIGEN_RATIO > evmax)
                        && (evmag > MIN_EIGEN_MAG)
                        && (evmag < MAX_EIGEN_MAG);

    return eigen_pass;
}

//Kasa sphere fitting with normal equation
static int moc_fit(struct MagCal *moc, struct Vec3 *bias, float *radius)
{
    //    A    *   out   =    b
    // (4 x 4)   (4 x 1)   (4 x 1)
    struct Mat44 A;
    A.elem[0][0] = moc->acc_xx; A.elem[0][1] = moc->acc_xy;
    A.elem[0][2] = moc->acc_xz; A.elem[0][3] = moc->acc_x;
    A.elem[1][0] = moc->acc_xy; A.elem[1][1] = moc->acc_yy;
    A.elem[1][2] = moc->acc_yz; A.elem[1][3] = moc->acc_y;
    A.elem[2][0] = moc->acc_xz; A.elem[2][1] = moc->acc_yz;
    A.elem[2][2] = moc->acc_zz; A.elem[2][3] = moc->acc_z;
    A.elem[3][0] = moc->acc_x;  A.elem[3][1] = moc->acc_y;
    A.elem[3][2] = moc->acc_z;  A.elem[3][3] = 1.0f;

    struct Vec4 b;
    initVec4(&b, -moc->acc_xw, -moc->acc_yw, -moc->acc_zw, -moc->acc_w);

    struct Size4 pivot;
    mat44DecomposeLup(&A, &pivot);

    struct Vec4 out;
    mat44Solve(&A, &out, &b, &pivot);

    // sphere: (x - xc)^2 + (y - yc)^2 + (z - zc)^2 = r^2
    //
    // xc = -out[0] / 2, yc = -out[1] / 2, zc = -out[2] / 2
    // r = sqrt(xc^2 + yc^2 + zc^2 - out[3])

    struct Vec3 v;
    initVec3(&v, out.x, out.y, out.z);
    vec3ScalarMul(&v, -0.5f);

    float r = sqrtf(vec3Dot(&v, &v) - out.w);

    initVec3(bias, v.x, v.y, v.z);
    *radius = r;

    int success = 0;
    if (r > MIN_FIT_MAG && r < MAX_FIT_MAG) {
        success = 1;
    }

    return success;
}

static void moc_reset(struct MagCal *moc)
{
    moc->acc_x = moc->acc_y = moc->acc_z = moc->acc_w = 0.0f;
    moc->acc_xx = moc->acc_xy = moc->acc_xz = moc->acc_xw = 0.0f;
    moc->acc_yy = moc->acc_yz = moc->acc_yw = 0.0f;
    moc->acc_zz = moc->acc_zw = 0.0f;

    moc->nsamples = 0;
    moc->start_time = 0;
}

static int moc_batch_complete(struct MagCal *moc, uint64_t sample_time_us)
{
    int complete = 0;

    if ((sample_time_us - moc->start_time > MIN_BATCH_WINDOW)
        && (moc->nsamples > MIN_BATCH_SIZE)) {

        complete = 1;

    } else if (sample_time_us - moc->start_time > MAX_BATCH_WINDOW) {
        // not enough samples collected in MAX_BATCH_WINDOW
        moc_reset(moc);
    }

    return complete;
}

void initMagCal(struct MagCal *moc,
                  float x_bias, float y_bias, float z_bias,
                  float c00, float c01, float c02,
                  float c10, float c11, float c12,
                  float c20, float c21, float c22)
{
    moc_reset(moc);
    moc->update_time = 0;
    moc->radius = 0.0f;

    moc->x_bias = x_bias;
    moc->y_bias = y_bias;
    moc->z_bias = z_bias;

    moc->c00 = c00; moc->c01 = c01; moc->c02 = c02;
    moc->c10 = c10; moc->c11 = c11; moc->c12 = c12;
    moc->c20 = c20; moc->c21 = c21; moc->c22 = c22;
}

void destroy_mag_cal(struct MagCal *moc)
{
    (void)moc;
}

bool magCalUpdate(struct MagCal *moc, uint64_t sample_time_us,
                   float x, float y, float z)
{
    bool new_bias = false;

    // 1. run accumulators
    float w = x * x + y * y + z * z;

    moc->acc_x += x;
    moc->acc_y += y;
    moc->acc_z += z;
    moc->acc_w += w;

    moc->acc_xx += x * x;
    moc->acc_xy += x * y;
    moc->acc_xz += x * z;
    moc->acc_xw += x * w;

    moc->acc_yy += y * y;
    moc->acc_yz += y * z;
    moc->acc_yw += y * w;

    moc->acc_zz += z * z;
    moc->acc_zw += z * w;

    if (++moc->nsamples == 1) {
        moc->start_time = sample_time_us;
    }

    // 2. batch has enough samples?
    if (moc_batch_complete(moc, sample_time_us)) {

        float inv = 1.0f / moc->nsamples;

        moc->acc_x *= inv;
        moc->acc_y *= inv;
        moc->acc_z *= inv;
        moc->acc_w *= inv;

        moc->acc_xx *= inv;
        moc->acc_xy *= inv;
        moc->acc_xz *= inv;
        moc->acc_xw *= inv;

        moc->acc_yy *= inv;
        moc->acc_yz *= inv;
        moc->acc_yw *= inv;

        moc->acc_zz *= inv;
        moc->acc_zw *= inv;

        // 3. eigen test
        if (moc_eigen_test(moc)) {

            struct Vec3 bias;
            float radius;

            // 4. Kasa sphere fitting
            if (moc_fit(moc, &bias, &radius)) {

                moc->x_bias = bias.x;
                moc->y_bias = bias.y;
                moc->z_bias = bias.z;

                moc->radius = radius;
                moc->update_time = sample_time_us;

                new_bias = true;
            }
        }

        // 5. reset for next batch
        moc_reset(moc);
    }

    return new_bias;
}

void magCalGetBias(struct MagCal *moc, float *x, float *y, float *z)
{
    *x = moc->x_bias;
    *y = moc->y_bias;
    *z = moc->z_bias;
}

void magCalAddBias(struct MagCal *moc, float x, float y, float z)
{
    moc->x_bias += x;
    moc->y_bias += y;
    moc->z_bias += z;
}

void magCalRemoveBias(struct MagCal *moc, float xi, float yi, float zi,
                         float *xo, float *yo, float *zo)
{
    *xo = xi - moc->x_bias;
    *yo = yi - moc->y_bias;
    *zo = zi - moc->z_bias;
}

void magCalSetSoftiron(struct MagCal *moc,
                          float c00, float c01, float c02,
                          float c10, float c11, float c12,
                          float c20, float c21, float c22)
{
    moc->c00 = c00; moc->c01 = c01; moc->c02 = c02;
    moc->c10 = c10; moc->c11 = c11; moc->c12 = c12;
    moc->c20 = c20; moc->c21 = c21; moc->c22 = c22;
}

void magCalRemoveSoftiron(struct MagCal *moc, float xi, float yi, float zi,
                             float *xo, float *yo, float *zo)
{
    *xo = moc->c00 * xi + moc->c01 * yi + moc->c02 * zi;
    *yo = moc->c10 * xi + moc->c11 * yi + moc->c12 * zi;
    *zo = moc->c20 * xi + moc->c21 * yi + moc->c22 * zi;
}

