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

#include <algos/vec.h>
#include <nanohub_math.h>


void findOrthogonalVector( float inX, float inY, float inZ, float *outX, float *outY, float *outZ) {

    float x, y, z;

    // discard the one with the smallest absolute value
    if (fabsf(inX) <= fabsf(inY) && fabsf(inX) <= fabsf(inZ)) {
        x = 0.0f;
        y = inZ;
        z = -inY;
    } else if (fabsf(inY) <= fabsf(inZ)) {
        x = inZ;
        y = 0.0f;
        z = -inX;
    } else {
        x = inY;
        y = -inX;
        z = 0.0f;
    }

    float magSquared = x * x + y * y + z * z;
    float invMag = 1.0f / sqrtf(magSquared);

    *outX = x * invMag;
    *outY = y * invMag;
    *outZ = z * invMag;
}
