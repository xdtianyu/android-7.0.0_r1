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
//*  - ideint_cac_8x8_av8()
//*
//* @remarks
//*  None
//*
//*******************************************************************************


//******************************************************************************
//*
//*  @brief Calculates Combing Artifact
//*
//*  @par   Description
//*   This functions calculates combing artifact check (CAC) for given two fields
//*
//* @param[in] pu1_top
//*  UWORD8 pointer to top field
//*
//* @param[in] pu1_bot
//*  UWORD8 pointer to bottom field
//*
//* @param[in] top_strd
//*  Top field stride
//*
//* @param[in] bot_strd
//*  Bottom field stride
//*
//* @returns
//*     None
//*
//* @remarks
//*
//******************************************************************************

    .global ideint_cac_8x8_av8

ideint_cac_8x8_av8:

    // Load first row of top
    ld1     {v28.8b},       [x0],       x2

    // Load first row of bottom
    ld1     {v29.8b},       [x1],       x3
    mov     v28.d[1],       v29.d[0]

    // Load second row of top
    ld1     {v30.8b},       [x0],       x2

    // Load second row of bottom
    ld1     {v31.8b},       [x1],       x3
    mov     v30.d[1],       v31.d[0]


    // Calculate row based adj and alt values
    // Get row sums
    uaddlp  v0.8h,          v28.16b

    uaddlp  v2.8h,          v30.16b

    uaddlp  v0.4s,          v0.8h

    uaddlp  v2.4s,          v2.8h

    // Both v0 and v2 have four 32 bit sums corresponding to first 4 rows
    // Pack v0 and v2 into a single register (sum does not exceed 16bits)

    shl     v16.4s,         v2.4s,      #16
    orr     v16.16b,        v0.16b,     v16.16b
    // v16 now contains 8 sums

    // Load third row of top
    ld1     {v24.8b},       [x0],       x2

    // Load third row of bottom
    ld1     {v25.8b},       [x1],       x3
    mov     v24.d[1],       v25.d[0]

    // Load fourth row of top
    ld1     {v26.8b},       [x0],       x2

    // Load fourth row of bottom
    ld1     {v27.8b},       [x1],       x3
    mov     v26.d[1],       v27.d[0]

    // Get row sums
    uaddlp  v4.8h,          v24.16b

    uaddlp  v6.8h,          v26.16b

    uaddlp  v4.4s,          v4.8h

    uaddlp  v6.4s,          v6.8h
    // Both v4 and v6 have four 32 bit sums corresponding to last 4 rows
    // Pack v4 and v6 into a single register (sum does not exceed 16bits)

    shl     v18.4s,         v6.4s,      #16
    orr     v18.16b,        v4.16b,     v18.16b
    // v18 now contains 8 sums

    // Compute absolute diff between top and bottom row sums
    mov     v17.d[0],       v16.d[1]
    uabd    v16.4h,         v16.4h,     v17.4h

    mov     v19.d[0],       v18.d[1]
    uabd    v17.4h,         v18.4h,     v19.4h

    mov     v16.d[1],       v17.d[0]

    // RSUM_CSUM_THRESH
    movi    v18.8h,         #20

    // Eliminate values smaller than RSUM_CSUM_THRESH
    cmhs    v20.8h,         v16.8h,     v18.8h
    and     v20.16b,        v16.16b,    v20.16b

    // v20 now contains 8 absolute diff of sums above the threshold

    // Compute adj
    mov     v21.d[0],       v20.d[1]
    add     v20.4h,         v20.4h,     v21.4h

    // v20 has four adj values for two sub-blocks

    // Compute alt
    uabd    v0.4s,      v0.4s,      v2.4s
    uabd    v4.4s,      v4.4s,      v6.4s

    add     v0.4s,      v0.4s,      v4.4s

    mov     v1.d[0],    v0.d[1]
    add     v21.4s,     v0.4s,      v1.4s
    // d21 has two values for two sub-blocks


    // Calculate column based adj and alt values

    urhadd  v0.16b,     v28.16b,    v30.16b
    urhadd  v2.16b,     v24.16b,    v26.16b
    urhadd  v0.16b,     v0.16b,     v2.16b

    mov     v1.d[0],    v0.d[1]
    uabd    v0.8b,      v0.8b,      v1.8b

    // RSUM_CSUM_THRESH >> 2
    movi    v22.16b,        #5

    // Eliminate values smaller than RSUM_CSUM_THRESH >> 2
    cmhs    v1.16b,      v0.16b,        v22.16b
    and     v0.16b,      v0.16b,        v1.16b
    // d0 now contains 8 absolute diff of sums above the threshold


    uaddlp  v0.4h,      v0.8b
    shl     v0.4h,      v0.4h,#2

    // Add row based adj
    add     v20.4h,     v0.4h,      v20.4h

    uaddlp  v20.2s,     v20.4h
    // d20 now contains 2 adj values


    urhadd  v0.8b,      v28.8b,     v29.8b
    urhadd  v2.8b,      v24.8b,     v25.8b
    urhadd  v0.8b,      v0.8b,      v2.8b

    urhadd  v1.8b,      v30.8b,     v31.8b
    urhadd  v3.8b,      v26.8b,     v27.8b
    urhadd  v1.8b,      v1.8b,      v3.8b

    uabd    v0.8b,      v0.8b,      v1.8b
    uaddlp  v0.4h,      v0.8b

    shl     v0.4h,      v0.4h,      #2
    uaddlp  v0.2s,      v0.4h
    add     v21.2s,     v0.2s,      v21.2s


    // d21 now contains 2 alt values

    // SAD_BIAS_MULT_SHIFT
    ushr    v0.2s,      v21.2s,     #3
    add     v21.2s,     v21.2s,     v0.2s

    // SAD_BIAS_ADDITIVE >> 1
    movi    v0.2s,      #4
    add     v21.2s,     v21.2s,     v0.2s

    cmhi    v0.2s,      v20.2s,     v21.2s
    uaddlp  v0.1d,      v0.2s

    smov    x0,         v0.2s[0]
    cmp     x0,         #0
    mov     x4,         #1
    csel    x0,         x4,         x0,         ne
    ret
