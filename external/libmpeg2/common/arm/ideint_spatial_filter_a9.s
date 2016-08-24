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
@*  - ideint_spatial_filter_a9()
@*
@* @remarks
@*  None
@*
@*******************************************************************************


@******************************************************************************
@*
@*  @brief Performs spatial filtering
@*
@*  @par   Description
@*   This functions performs edge adaptive spatial filtering on a 8x8 block
@*
@* @param[in] pu1_src
@*  UWORD8 pointer to the source
@*
@* @param[in] pu1_out
@*  UWORD8 pointer to the destination
@*
@* @param[in] src_strd
@*  source stride
@*
@* @param[in] src_strd
@*  destination stride
@*
@* @returns
@*  None
@*
@* @remarks
@*
@******************************************************************************

    .global ideint_spatial_filter_a9

ideint_spatial_filter_a9:

    stmfd       sp!,    {r4-r10, lr}

    vmov.u16    q8,     #0
    vmov.u16    q9,     #0
    vmov.u16    q10,    #0

    @ Backup r0
    mov         r10,    r0

    @ Load from &pu1_row_1[0]
    sub         r5,     r0,     #1
    vld1.8      d0,     [r0],   r2

    @ Load from &pu1_row_1[-1]
    vld1.8      d1,     [r5]
    add         r5,     r5,     #2

    @ Load from &pu1_row_1[1]
    vld1.8      d2,     [r5]

    @ Number of rows
    mov         r4,     #4

    @ EDGE_BIAS_0
    vmov.u32    d30,    #5

    @ EDGE_BIAS_1
    vmov.u32    d31,    #7

detect_edge:
    @ Load from &pu1_row_2[0]
    sub         r5,     r0,     #1
    vld1.8      d3,     [r0],   r2

    @ Load from &pu1_row_2[-1]
    vld1.8      d4,     [r5]
    add         r5,     r5,     #2

    @ Load from &pu1_row_2[1]
    vld1.8      d5,     [r5]

    @ Calculate absolute differences
    @ pu1_row_1[i] - pu1_row_2[i]
    vabal.u8    q8,     d0,     d3

    @ pu1_row_1[i - 1] - pu1_row_2[i + 1]
    vabal.u8    q9,     d1,     d5

    @ pu1_row_1[i + 1] - pu1_row_2[i - 1]
    vabal.u8    q10,    d4,     d2

    vmov        d0,     d3
    vmov        d1,     d4
    vmov        d2,     d5

    subs        r4,     r4,     #1
    bgt         detect_edge

    @ Calculate sum of absolute differeces for each edge
    vpadd.u16   d16,    d16,    d17
    vpadd.u16   d18,    d18,    d19
    vpadd.u16   d20,    d20,    d21

    vpaddl.u16  d16,    d16
    vpaddl.u16  d18,    d18
    vpaddl.u16  d20,    d20

    @ adiff[0] *= EDGE_BIAS_0;
    vmul.u32    d16,    d16,    d30

    @ adiff[1] *= EDGE_BIAS_1;
    vmul.u32    d18,    d18,    d31

    @ adiff[2] *= EDGE_BIAS_1;
    vmul.u32    d20,    d20,    d31

    @ Move the differences to ARM registers


    @ Compute shift for first half of the block
compute_shift_1:
    vmov.u32    r5,     d16[0]
    vmov.u32    r6,     d18[0]
    vmov.u32    r7,     d20[0]

    @ Compute shift
    mov         r8,     #0

    @ adiff[2] <= adiff[1]
    cmp         r7,     r6
    bgt         dir_45_gt_135_1

    @ adiff[2] <= adiff[0]
    cmp         r7,     r5
    movle       r8,     #1

    b           compute_shift_2
dir_45_gt_135_1:

    @ adiff[1] <= adiff[0]
    cmp         r6,     r5
    @ Move -1 if less than or equal to
    mvnle       r8,     #0


compute_shift_2:
    @ Compute shift for first half of the block
    vmov.u32    r5,     d16[1]
    vmov.u32    r6,     d18[1]
    vmov.u32    r7,     d20[1]

    @ Compute shift
    mov         r9,     #0

    @ adiff[2] <= adiff[1]
    cmp         r7,     r6
    bgt         dir_45_gt_135_2

    @ adiff[2] <= adiff[0]
    cmp         r7,     r5
    movle       r9,     #1

    b           interpolate
dir_45_gt_135_2:

    @ adiff[1] <= adiff[0]
    cmp         r6,     r5

    @ Move -1 if less than or equal to
    mvnle       r9,     #0

interpolate:
    add         r4,     r10,    r8
    add         r5,     r10,    r2
    sub         r5,     r5,     r8

    add         r10,    r10,    #4
    add         r6,     r10,    r9
    add         r7,     r10,    r2
    sub         r7,     r7,     r9
    mov         r8,     #4

filter_loop:
    vld1.u32    d0[0],  [r4],   r2
    vld1.u32    d2[0],  [r5],   r2

    vld1.u32    d0[1],  [r6],   r2
    vld1.u32    d2[1],  [r7],   r2

    vrhadd.u8   d4,     d0,     d2
    vst1.u32    d4,     [r1],   r3

    subs        r8,     #1
    bgt         filter_loop

    ldmfd       sp!,    {r4-r10, pc}
