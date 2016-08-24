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
//*  This file contains definitions of routines for SAD caclulation
//*
//* @author
//*  Ittiam
//*
//* @par List of Functions:
//*  - icv_sad_8x4_av8()
//*
//* @remarks
//*  None
//*
//*******************************************************************************


//******************************************************************************
//*
//*  @brief computes distortion (SAD) between 2 8x4  blocks
//*
//*
//*  @par   Description
//*   This functions computes SAD between 2 8x4 blocks.
//*
//* @param[in] pu1_src
//*  UWORD8 pointer to the source
//*
//* @param[out] pu1_ref
//*  UWORD8 pointer to the reference buffer
//*
//* @param[in] src_strd
//*  integer source stride
//*
//* @param[in] ref_strd
//*  integer reference stride
//*
//* @param[in] wd
//*  Width (assumed to be 8)
//*
//* @param[in] ht
//*  Height (assumed to be 4)
//*
//* @returns
//*     SAD value in r0
//*
//* @remarks
//*
//******************************************************************************

    .global icv_sad_8x4_av8

icv_sad_8x4_av8:

    // Load 8x4 source
    ld1     {v0.8b},    [x0],     x2
    ld1     {v1.8b},    [x0],     x2
    ld1     {v2.8b},    [x0],     x2
    ld1     {v3.8b},    [x0],     x2

    // Load 8x4 reference
    ld1     {v4.8b},    [x1],     x3
    ld1     {v5.8b},    [x1],     x3
    ld1     {v6.8b},    [x1],     x3
    ld1     {v7.8b},    [x1],     x3

    uabdl   v0.8h,      v0.8b,      v4.8b
    uabal   v0.8h,      v1.8b,      v5.8b
    uabal   v0.8h,      v2.8b,      v6.8b
    uabal   v0.8h,      v3.8b,      v7.8b

    addp    v0.8h,      v0.8h,      v0.8h
    addp    v0.8h,      v0.8h,      v0.8h
    addp    v0.8h,      v0.8h,      v0.8h

    smov    x0,         v0.8h[0]

    ret
