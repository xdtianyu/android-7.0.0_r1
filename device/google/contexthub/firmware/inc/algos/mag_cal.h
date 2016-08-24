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

#ifndef MAG_CAL_H_

#define MAG_CAL_H_

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct MagCal {
    uint64_t start_time;
    uint64_t update_time;

    float acc_x, acc_y, acc_z, acc_w;
    float acc_xx, acc_xy, acc_xz, acc_xw;
    float acc_yy, acc_yz, acc_yw, acc_zz, acc_zw;

    float x_bias, y_bias, z_bias;
    float radius;

    float c00, c01, c02, c10, c11, c12, c20, c21, c22;

    size_t nsamples;
};

void initMagCal(struct MagCal *moc,
                  float x_bias, float y_bias, float z_bias,
                  float c00, float c01, float c02,
                  float c10, float c11, float c12,
                  float c20, float c21, float c22);

void destroy_mag_cal(struct MagCal *moc);

bool magCalUpdate(struct MagCal *moc, uint64_t sample_time_us,
                   float x, float y, float z);

void magCalGetBias(struct MagCal *moc, float *x, float *y, float *z);

void magCalAddBias(struct MagCal *moc, float x, float y, float z);

void magCalRemoveBias(struct MagCal *moc, float xi, float yi, float zi,
                         float *xo, float *yo, float *zo);

void magCalSetSoftiron(struct MagCal *moc,
                          float c00, float c01, float c02,
                          float c10, float c11, float c12,
                          float c20, float c21, float c22);

void magCalRemoveSoftiron(struct MagCal *moc, float xi, float yi, float zi,
                             float *xo, float *yo, float *zo);

#ifdef __cplusplus
}
#endif

#endif  // MAG_CAL_H_
