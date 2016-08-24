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
@* @brief
@*  This file contains definitions of routines for spatial filter
@*
@* @author
@*  Ittiam
@*
@* @par List of Functions:
@*  - ideint_cac_8x8_a9()
@*
@* @remarks
@*  None
@*
@*******************************************************************************


@******************************************************************************
@*
@*  @brief Calculates Combing Artifact
@*
@*  @par   Description
@*   This functions calculates combing artifact check (CAC) for given two fields
@*
@* @param[in] pu1_top
@*  UWORD8 pointer to top field
@*
@* @param[in] pu1_bot
@*  UWORD8 pointer to bottom field
@*
@* @param[in] top_strd
@*  Top field stride
@*
@* @param[in] bot_strd
@*  Bottom field stride
@*
@* @returns
@*  None
@*
@* @remarks
@*
@******************************************************************************

    .global ideint_cac_8x8_a9

ideint_cac_8x8_a9:

    stmfd       sp!,    {r4-r10, lr}

    @ Load first row of top
    vld1.u8     d28,    [r0],   r2

    @ Load first row of bottom
    vld1.u8     d29,    [r1],   r3

    @ Load second row of top
    vld1.u8     d30,    [r0],   r2

    @ Load second row of bottom
    vld1.u8     d31,    [r1],   r3


    @ Calculate row based adj and alt values
    @ Get row sums
    vpaddl.u8   q0,     q14

    vpaddl.u8   q1,     q15

    vpaddl.u16  q0,     q0

    vpaddl.u16  q1,     q1

    @ Both q0 and q1 have four 32 bit sums corresponding to first 4 rows
    @ Pack q0 and q1 into a single register (sum does not exceed 16bits)

    vshl.u32    q8,     q1,     #16
    vorr.u32    q8,     q0,     q8
    @ q8 now contains 8 sums

    @ Load third row of top
    vld1.u8     d24,    [r0],   r2

    @ Load third row of bottom
    vld1.u8     d25,    [r1],   r3

    @ Load fourth row of top
    vld1.u8     d26,    [r0],   r2

    @ Load fourth row of bottom
    vld1.u8     d27,    [r1],   r3

    @ Get row sums
    vpaddl.u8   q2,     q12

    vpaddl.u8   q3,     q13

    vpaddl.u16  q2,     q2

    vpaddl.u16  q3,     q3
    @ Both q2 and q3 have four 32 bit sums corresponding to last 4 rows
    @ Pack q2 and q3 into a single register (sum does not exceed 16bits)

    vshl.u32    q9,     q3,     #16
    vorr.u32    q9,     q2,     q9
    @ q9 now contains 8 sums

    @ Compute absolute diff between top and bottom row sums
    vabd.u16    d16,    d16,    d17
    vabd.u16    d17,    d18,    d19

    @ RSUM_CSUM_THRESH
    vmov.u16    q9,     #20

    @ Eliminate values smaller than RSUM_CSUM_THRESH
    vcge.u16    q10,    q8,     q9
    vand.u16    q10,    q8,     q10
    @ q10 now contains 8 absolute diff of sums above the threshold


    @ Compute adj
    vadd.u16    d20,    d20,    d21

    @ d20 has four adj values for two sub-blocks

    @ Compute alt
    vabd.u32    q0,     q0,     q1
    vabd.u32    q2,     q2,     q3

    vadd.u32    q0,     q0,     q2
    vadd.u32    d21,    d0,     d1
    @ d21 has two values for two sub-blocks


    @ Calculate column based adj and alt values

    vrhadd.u8   q0,     q14,    q15
    vrhadd.u8   q1,     q12,    q13
    vrhadd.u8   q0,     q0,     q1

    vabd.u8     d0,     d0,     d1

    @ RSUM_CSUM_THRESH >> 2
    vmov.u8     d9,     #5

    @ Eliminate values smaller than RSUM_CSUM_THRESH >> 2
    vcge.u8     d1,     d0,     d9
    vand.u8     d0,     d0,     d1
    @ d0 now contains 8 absolute diff of sums above the threshold


    vpaddl.u8   d0,     d0
    vshl.u16    d0,     d0,     #2

    @ Add row based adj
    vadd.u16    d20,    d0,     d20

    vpaddl.u16  d20,    d20
    @ d20 now contains 2 adj values


    vrhadd.u8   d0,     d28,    d29
    vrhadd.u8   d2,     d24,    d25
    vrhadd.u8   d0,     d0,     d2

    vrhadd.u8   d1,     d30,    d31
    vrhadd.u8   d3,     d26,    d27
    vrhadd.u8   d1,     d1,     d3

    vabd.u8     d0,     d0,     d1
    vpaddl.u8   d0,     d0

    vshl.u16    d0,     d0,     #2
    vpaddl.u16  d0,     d0
    vadd.u32    d21,    d0,     d21


    @ d21 now contains 2 alt values

    @ SAD_BIAS_MULT_SHIFT
    vshr.u32    d0,     d21,    #3
    vadd.u32    d21,    d21,    d0

    @ SAD_BIAS_ADDITIVE >> 1
    vmov.u32    d0,     #4
    vadd.u32    d21,    d21,    d0

    vclt.u32    d0,     d21,    d20
    vpaddl.u32  d0,     d0

    vmov.u32    r0,     d0[0]
    cmp         r0,     #0
    movne       r0,     #1
    ldmfd       sp!,    {r4-r10, pc}
