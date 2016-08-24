//******************************************************************************
//*
//* Copyright (C) 2015 The Android Open Source Project
//*
//* Licensed under the Apache License, Version 2.0 (the "License");
//* you may not use this file except in compliance with the License.
//* You may obtain a copy of the License at:
//*
//* http://www.apache.org/licenses/LICENSE-2.0
//*
//* Unless required by applicable law or agreed to in writing, software
//* distributed under the License is distributed on an "AS IS" BASIS,
//* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//* See the License for the specific language governing permissions and
//* limitations under the License.
//*
//*****************************************************************************
//* Originally developed and contributed by Ittiam Systems Pvt. Ltd, Bangalore
//*/

//******************************************************************************
//*
//* @brief
//*  This file contains definitions of routines for spatial filter
//*
//* @author
//*  Ittiam
//*
//* @par List of Functions:
//*  - ideint_spatial_filter_a9()
//*
//* @remarks
//*  None
//*
//*******************************************************************************


//******************************************************************************
//*
//*  @brief Performs spatial filtering
//*
//*  @par   Description
//*   This functions performs edge adaptive spatial filtering on a 8x8 block
//*
//* @param[in] pu1_src
//*  UWORD8 pointer to the source
//*
//* @param[in] pu1_out
//*  UWORD8 pointer to the destination
//*
//* @param[in] src_strd
//*  source stride
//*
//* @param[in] src_strd
//*  destination stride
//*
//* @returns
//*     None
//*
//* @remarks
//*
//******************************************************************************

    .global ideint_spatial_filter_av8

ideint_spatial_filter_av8:

    movi  v16.8h, #0
    movi  v18.8h, #0
    movi  v20.8h, #0

    // Backup x0
    mov     x10,    x0

    // Load from &pu1_row_1[0]
    sub     x5,         x0,         #1
    ld1     {v0.8b},    [x0],       x2

    // Load from &pu1_row_1[-1]
    ld1     {v1.8b},    [x5]
    add     x5,         x5,        #2

    // Load from &pu1_row_1[1]
    ld1     {v2.8b},    [x5]

    // Number of rows
    mov     x4,         #4

    // EDGE_BIAS_0
    movi    v30.2s,     #5

    // EDGE_BIAS_1
    movi    v31.2s,     #7

detect_edge:
    // Load from &pu1_row_2[0]
    sub     x5,         x0,         #1
    ld1     {v3.8b},    [x0],       x2

    // Load from &pu1_row_2[-1]
    ld1     {v4.8b},    [x5]
    add     x5,         x5,         #2

    // Load from &pu1_row_2[1]
    ld1     {v5.8b},    [x5]

    // Calculate absolute differences
    // pu1_row_1[i] - pu1_row_2[i]
    uabal   v16.8h,      v0.8b,        v3.8b

    // pu1_row_1[i - 1] - pu1_row_2[i + 1]
    uabal   v18.8h,      v1.8b,        v5.8b

    // pu1_row_1[i + 1] - pu1_row_2[i - 1]
    uabal   v20.8h,      v2.8b,        v4.8b

    mov     v0.8b,      v3.8b
    mov     v1.8b,      v4.8b
    mov     v2.8b,      v5.8b

    subs    x4,         x4,             #1
    bgt            detect_edge

    // Calculate sum of absolute differeces for each edge
    addp  v16.8h,       v16.8h,         v16.8h
    addp  v18.8h,       v18.8h,         v18.8h
    addp  v20.8h,       v20.8h,         v20.8h

    uaddlp  v16.2s,     v16.4h
    uaddlp  v18.2s,     v18.4h
    uaddlp  v20.2s,     v20.4h

    // adiff[0] *= EDGE_BIAS_0;
    mul     v16.2s,     v16.2s,         v30.2s

    // adiff[1] *= EDGE_BIAS_1;
    mul     v18.2s,     v18.2s,         v31.2s

    // adiff[2] *= EDGE_BIAS_1;
    mul     v20.2s,     v20.2s,         v31.2s

    // Move the differences to ARM registers


    // Compute shift for first half of the block
compute_shift_1:
    smov    x5,         v16.2s[0]
    smov    x6,         v18.2s[0]
    smov    x7,         v20.2s[0]

    // Compute shift
    mov     x8,         #0

    // adiff[2] <= adiff[1]
    cmp     x7,         x6
    bgt     dir_45_gt_135_1

    // adiff[2] <= adiff[0]
    cmp     x7,         x5
    mov     x11,        #1
    csel    x8,         x11,        x8,     le

    b       compute_shift_2
dir_45_gt_135_1:

    // adiff[1] <= adiff[0]
    cmp     x6,         x5
    // Move -1 if less than or equal to
    movn    x11,        #0
    csel    x8,         x11,        x8,     le


compute_shift_2:
    // Compute shift for first half of the block
    smov    x5,         v16.2s[1]
    smov    x6,         v18.2s[1]
    smov    x7,         v20.2s[1]

    // Compute shift
    mov     x9,         #0

    // adiff[2] <= adiff[1]
    cmp     x7,         x6
    bgt     dir_45_gt_135_2

    // adiff[2] <= adiff[0]
    cmp     x7,         x5
    mov     x11,        #1
    csel    x9,         x11,        x9,     le

    b       interpolate

dir_45_gt_135_2:
    // adiff[1] <= adiff[0]
    cmp     x6,         x5

    // Move -1 if less than or equal to
    movn    x11,        #0
    csel    x9,         x11,        x9,     le

interpolate:
    add     x4,         x10,        x8
    add     x5,         x10,        x2
    sub     x5,         x5,         x8

    add     x10,        x10,        #4
    add     x6,         x10,        x9
    add     x7,         x10,        x2
    sub     x7,         x7,         x9
    mov     x8,         #4

filter_loop:
    ld1     {v0.s}[0],  [x4],       x2
    ld1     {v2.s}[0],  [x5],       x2

    ld1     {v0.s}[1],  [x6],       x2
    ld1     {v2.s}[1],  [x7],       x2

    urhadd  v4.8b,      v0.8b,      v2.8b
    st1     {v4.2s},    [x1],       x3

    subs    x8,         x8,         #1
    bgt     filter_loop

    ret
