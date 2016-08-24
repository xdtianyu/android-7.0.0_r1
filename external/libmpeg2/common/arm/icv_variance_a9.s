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
@*  This file contains definitions of routines for variance caclulation
@*
@* @author
@*  Ittiam
@*
@* @par List of Functions:
@*  - icv_variance_8x4_a9()
@*
@* @remarks
@*  None
@*
@*******************************************************************************


@******************************************************************************
@*
@*  @brief computes variance of a 8x4  block
@*
@*
@*  @par   Description
@*   This functions computes variance of a 8x4  block
@*
@* @param[in] pu1_src
@*  UWORD8 pointer to the source
@*
@* @param[in] src_strd
@*  integer source stride
@*
@* @param[in] wd
@*  Width (assumed to be 8)
@*
@* @param[in] ht
@*  Height (assumed to be 4)
@*
@* @returns
@*  variance value in r0
@*
@* @remarks
@*
@******************************************************************************

    .global icv_variance_8x4_a9

icv_variance_8x4_a9:

    push        {lr}

    @ Load 8x4 source
    vld1.8      d0,     [r0],   r1
    vld1.8      d1,     [r0],   r1
    vld1.8      d2,     [r0],   r1
    vld1.8      d3,     [r0],   r1

    @ Calculate Sum(values)
    vaddl.u8    q2,     d0,     d1
    vaddl.u8    q3,     d2,     d3
    vadd.u16    q2,     q2,     q3

    vadd.u16    d4,     d4,     d5
    vpadd.u16   d4,     d4,     d4
    vpadd.u16   d4,     d4,     d4

    @ Calculate SumOfSquares
    vmull.u8    q10,    d0,     d0
    vmull.u8    q11,    d1,     d1
    vmull.u8    q12,    d2,     d2
    vmull.u8    q13,    d3,     d3

    vaddl.u16   q10,    d20,    d21
    vaddl.u16   q11,    d22,    d23
    vaddl.u16   q12,    d24,    d25
    vaddl.u16   q13,    d26,    d27

    vadd.u32    q10,    q10,    q11
    vadd.u32    q11,    q12,    q13
    vadd.u32    q10,    q10,    q11
    vadd.u32    d20,    d20,    d21
    vpadd.u32   d20,    d20,    d20

    @ Sum(values)
    vmov.u16     r0,    d4[0]

    @ SumOfSquares
    vmov.u32     r1,    d20[0]

    @ SquareOfSums
    mul         r3,     r0,     r0

    @ SumOfSquares * 8 * 4 - SquareOfSums
    rsb         r0,     r3,     r1,     LSL #5

    @ Divide by 32 * 32

    mov         r0,     r0,     ASR #10
    pop         {pc}
