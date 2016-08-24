@/******************************************************************************
@ *
@ * Copyright (C) 2015 The Android Open Source Project
@ *
@ * Licensed under the Apache License, Version 2.0 (the "License");
@ * you may not use this file except in compliance with the License.
@ * You may obtain a copy of the License at:
@ *
@ * http://www.apache.org/licenses/LICENSE-2.0
@ *
@ * Unless required by applicable law or agreed to in writing, software
@ * distributed under the License is distributed on an "AS IS" BASIS,
@ * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
@ * See the License for the specific language governing permissions and
@ * limitations under the License.
@ *
@ *****************************************************************************
@ * Originally developed and contributed by Ittiam Systems Pvt. Ltd, Bangalore
@*/


@******************************************************************************
@*
@*
@* @brief
@*  This file contains definitions of routines for SAD caclulation
@*
@* @author
@*  Ittiam
@*
@* @par List of Functions:
@*  - icv_sad_8x4_a9()
@*
@* @remarks
@*  None
@*
@*******************************************************************************


@******************************************************************************
@*
@*  @brief computes distortion (SAD) between 2 8x4  blocks
@*
@*
@*  @par   Description
@*   This functions computes SAD between 2 8x4 blocks.
@*
@* @param[in] pu1_src
@*  UWORD8 pointer to the source
@*
@* @param[out] pu1_ref
@*  UWORD8 pointer to the reference buffer
@*
@* @param[in] src_strd
@*  integer source stride
@*
@* @param[in] ref_strd
@*  integer reference stride
@*
@* @param[in] wd
@*  Width (assumed to be 8)
@*
@* @param[in] ht
@*  Height (assumed to be 4)
@*
@* @returns
@*  SAD value in r0
@*
@* @remarks
@*
@******************************************************************************

    .global icv_sad_8x4_a9

icv_sad_8x4_a9:

    push          {lr}

    vld1.8        d4, [r0], r2
    vld1.8        d5, [r1], r3

    vld1.8        d6, [r0], r2
    vabdl.u8      q0, d5, d4

    vld1.8        d7, [r1], r3
    vabal.u8      q0, d7, d6

    vld1.8        d4, [r0], r2
    vld1.8        d5, [r1], r3

    vld1.8        d6, [r0], r2
    vabal.u8      q0, d5, d4

    vld1.8        d7, [r1], r3
    vabal.u8      q0, d7, d6

    vadd.i16      d0, d1, d0
    vpaddl.u16    d0, d0
    vpaddl.u32    d0, d0

    vmov.32       r0, d0[0]

    pop           {pc}
