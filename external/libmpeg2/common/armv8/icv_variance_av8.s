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
//*
//* @brief
//*  This file contains definitions of routines for variance caclulation
//*
//* @author
//*  Ittiam
//*
//* @par List of Functions:
//*  - icv_variance_8x4_av8()
//*
//* @remarks
//*  None
//*
//*******************************************************************************


//******************************************************************************
//*
//*  @brief computes variance of a 8x4  block
//*
//*
//*  @par   Description
//*   This functions computes variance of a 8x4  block
//*
//* @param[in] pu1_src
//*  UWORD8 pointer to the source
//*
//* @param[in] src_strd
//*  integer source stride
//*
//* @param[in] wd
//*  Width (assumed to be 8)
//*
//* @param[in] ht
//*  Height (assumed to be 4)
//*
//* @returns
//*     variance value in x0
//*
//* @remarks
//*
//******************************************************************************

    .global icv_variance_8x4_av8

icv_variance_8x4_av8:

    // Load 8x4 source
    ld1     {v0.8b},    [x0],     x1
    ld1     {v1.8b},    [x0],     x1
    ld1     {v2.8b},    [x0],     x1
    ld1     {v3.8b},    [x0],     x1

    // Calculate Sum(values)
    uaddl   v4.8h,  v0.8b,  v1.8b
    uaddl   v6.8h,  v2.8b,  v3.8b
    add     v4.8h,  v4.8h,  v6.8h

    addp    v4.8h,  v4.8h,  v4.8h
    addp    v4.4h,  v4.4h,  v4.4h
    addp    v4.4h,  v4.4h,  v4.4h

    // Calculate SumOfSquares
    umull   v20.8h, v0.8b,  v0.8b
    umull   v22.8h, v1.8b,  v1.8b
    umull   v24.8h, v2.8b,  v2.8b
    umull   v26.8h, v3.8b,  v3.8b

    uaddl   v21.4s,    v20.4h,    v22.4h
    uaddl   v25.4s,    v24.4h,    v26.4h
    uaddl2  v20.4s,    v20.8h,    v22.8h
    uaddl2  v24.4s,    v24.8h,    v26.8h

    add     v20.4s,     v20.4s,  v21.4s
    add     v22.4s,     v24.4s,  v25.4s
    add     v20.4s,     v20.4s,  v22.4s
    addp    v20.4s,     v20.4s,  v20.4s
    addp    v20.2s,     v20.2s,  v20.2s

    // Sum(values)
    smov    x0,     v4.4h[0]

    // SumOfSquares
    smov    x1,     v20.2s[0]

    // SquareOfSums
    mul     x3,     x0,     x0

    // SumOfSquares * 8 * 4 - SquareOfSums
    sub     x1,     x3,     x1,        LSL #5
    neg     x0,     x1

    // Divide by 32 * 32

    ASR     x0,     x0,     #10
    ret
