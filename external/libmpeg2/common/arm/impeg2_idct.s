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

@/*
@//----------------------------------------------------------------------------
@// File Name            : impeg2_idct.s
@//
@// Description          : This file has the Idct Implementations for the
@//                        MPEG2 SP decoder on neon platform.
@//
@// Reference Document   :
@//
@// Revision History     :
@//      Date            Author                  Detail Description
@//   ------------    ----------------    ----------------------------------
@//   Feb 22, 2008     Naveen Kumar T                Created
@//
@//-------------------------------------------------------------------------
@*/

@/*
@// ----------------------------------------------------------------------------
@// Include Files
@// ----------------------------------------------------------------------------
@*/

.text
.p2align 2
.equ idct_stg1_shift       ,            12
.equ idct_stg2_shift       ,            16
.equ idct_stg1_round     ,          (1 << (idct_stg1_shift - 1))
.equ idct_stg2_round     ,          (1 << (idct_stg2_shift - 1))
@/*
@// ----------------------------------------------------------------------------
@// Struct/Union Types and Define
@// ----------------------------------------------------------------------------
@*/

@/*
@// ----------------------------------------------------------------------------
@// Static Global Data section variables
@// ----------------------------------------------------------------------------
@*/
@//--------------------------- NONE --------------------------------------------

@/*
@// ----------------------------------------------------------------------------
@// Static Prototype Functions
@// ----------------------------------------------------------------------------
@*/
@// -------------------------- NONE --------------------------------------------

@/*
@// ----------------------------------------------------------------------------
@// Exported functions
@// ----------------------------------------------------------------------------
@*/

    .extern gai2_impeg2_idct_q15
.hidden gai2_impeg2_idct_q15
    .extern gai2_impeg2_idct_q11
.hidden gai2_impeg2_idct_q11
    .extern gai2_impeg2_idct_first_col_q15
.hidden gai2_impeg2_idct_first_col_q15
    .extern gai2_impeg2_idct_first_col_q11
.hidden gai2_impeg2_idct_first_col_q11
    .extern gai2_impeg2_mismatch_stg2_additive
.hidden gai2_impeg2_mismatch_stg2_additive

gai2_impeg2_idct_q15_addr1:
    .long gai2_impeg2_idct_q15 - q15lbl1 - 8
gai2_impeg2_idct_q15_addr2:
    .long gai2_impeg2_idct_q15 - q15lbl2 - 8
gai2_impeg2_idct_q11_addr1:
    .long gai2_impeg2_idct_q11 - q11lbl1 - 8
gai2_impeg2_idct_q11_addr2:
    .long gai2_impeg2_idct_q11 - q11lbl2 - 8
gai2_impeg2_idct_first_col_q15_addr1:
    .long gai2_impeg2_idct_first_col_q15 - fcq15_lbl1 - 8
gai2_impeg2_idct_first_col_q15_addr2:
    .long gai2_impeg2_idct_first_col_q15 - fcq15_lbl2 - 8
gai2_impeg2_idct_first_col_q15_addr3:
    .long gai2_impeg2_idct_first_col_q15 - fcq15_lbl3 - 8
gai2_impeg2_mismatch_stg2_additive_addr:
    .long gai2_impeg2_mismatch_stg2_additive - additive_lbl - 8
gai2_impeg2_idct_first_col_q11_addr1:
    .long gai2_impeg2_idct_first_col_q11 - fcq11_lbl1 - 8
gai2_impeg2_idct_first_col_q11_addr2:
    .long gai2_impeg2_idct_first_col_q11 - fcq11_lbl2 - 8

    .global impeg2_idct_recon_dc_a9q
impeg2_idct_recon_dc_a9q:
    stmfd           sp!, {r4, r6, r12, lr}
    @//r0: pi2_src
    @//r1: pi2_tmp - not used, used as pred_strd
    @//r2: pu1_pred
    @//r3: pu1_dst
    @//r4: used as scratch
    @//r5:

    ldr             r1, [sp, #20]       @//pred_strd
    ldr             r6, [sp, #24]       @//dst_strd

    ldr             r14, gai2_impeg2_idct_q15_addr1
q15lbl1:
    add             r14, r14, pc
    ldrsh           r12, [r14]
    ldrsh           r4, [r0]

    vld1.8          d0, [r2], r1
    mul             r4, r4, r12

    vld1.8          d1, [r2], r1
    add             r4, #idct_stg1_round

    vld1.8          d2, [r2], r1
    asr             r4, r4, #idct_stg1_shift

    ldr             r14, gai2_impeg2_idct_q11_addr1
q11lbl1:
    add             r14, r14, pc
    ldrsh           r12, [r14]

    vld1.8          d3, [r2], r1
    mul             r4, r4, r12

    vld1.8          d4, [r2], r1
    add             r4, #idct_stg2_round

    vld1.8          d5, [r2], r1
    asr             r4, r4, #idct_stg2_shift

    vld1.8          d6, [r2], r1
    vdup.s16        q15, r4


    vld1.8          d7, [r2], r1

    vaddw.u8        q4, q15, d0

    vaddw.u8        q5, q15, d1
    vqmovun.s16     d0, q4

    vaddw.u8        q6, q15, d2
    vqmovun.s16     d1, q5
    vst1.8          d0, [r3], r6

    vaddw.u8        q7, q15, d3
    vqmovun.s16     d2, q6
    vst1.8          d1, [r3], r6

    vaddw.u8        q8, q15, d4
    vqmovun.s16     d3, q7
    vst1.8          d2, [r3], r6

    vaddw.u8        q9, q15, d5
    vqmovun.s16     d4, q8
    vst1.8          d3, [r3], r6

    vaddw.u8        q10, q15, d6
    vqmovun.s16     d5, q9
    vst1.8          d4, [r3], r6

    vaddw.u8        q11, q15, d7
    vqmovun.s16     d6, q10
    vst1.8          d5, [r3], r6

    vqmovun.s16     d7, q11
    vst1.8          d6, [r3], r6


    vst1.8          d7, [r3], r6

    ldmfd           sp!, {r4, r6, r12, pc}




    .global impeg2_idct_recon_dc_mismatch_a9q
impeg2_idct_recon_dc_mismatch_a9q:
    stmfd           sp!, {r4-r12, lr}

    ldr             r1, [sp, #44]       @//pred_strd
    ldr             r6, [sp, #48]       @//dst_strd

    ldr             r14, gai2_impeg2_idct_q15_addr2
q15lbl2:
    add             r14, r14, pc
    ldrsh           r12, [r14]
    ldrsh           r4, [r0]

    mul             r4, r4, r12
    add             r4, #idct_stg1_round
    asr             r4, r4, #idct_stg1_shift

    ldr             r14, gai2_impeg2_idct_q11_addr2
q11lbl2:
    add             r14, r14, pc
    ldrsh           r12, [r14]
    mul             r4, r4, r12
    vdup.s32        q0, r4

    mov             r14, #16            @//Increment for table read
    ldr             r4, gai2_impeg2_mismatch_stg2_additive_addr
additive_lbl:
    add             r4, r4, pc

    vld1.16         {q1}, [r4], r14

    vld1.8          d30, [r2], r1
    vmovl.s16       q4, d2
    vmovl.s16       q5, d3
    vraddhn.s32     d12, q0, q4
    vraddhn.s32     d13, q0, q5
    vaddw.u8        q7, q6, d30
    vqmovun.s16     d30, q7
    vst1.8          d30, [r3], r6

    vld1.16         {q1}, [r4], r14
    vld1.8          d30, [r2], r1
    vmovl.s16       q4, d2
    vmovl.s16       q5, d3
    vraddhn.s32     d12, q0, q4
    vraddhn.s32     d13, q0, q5
    vaddw.u8        q7, q6, d30
    vqmovun.s16     d30, q7
    vst1.8          d30, [r3], r6

    vld1.16         {q1}, [r4], r14
    vld1.8          d30, [r2], r1
    vmovl.s16       q4, d2
    vmovl.s16       q5, d3
    vraddhn.s32     d12, q0, q4
    vraddhn.s32     d13, q0, q5
    vaddw.u8        q7, q6, d30
    vqmovun.s16     d30, q7
    vst1.8          d30, [r3], r6

    vld1.16         {q1}, [r4], r14
    vld1.8          d30, [r2], r1
    vmovl.s16       q4, d2
    vmovl.s16       q5, d3
    vraddhn.s32     d12, q0, q4
    vraddhn.s32     d13, q0, q5
    vaddw.u8        q7, q6, d30
    vqmovun.s16     d30, q7
    vst1.8          d30, [r3], r6

    vld1.16         {q1}, [r4], r14
    vld1.8          d30, [r2], r1
    vmovl.s16       q4, d2
    vmovl.s16       q5, d3
    vraddhn.s32     d12, q0, q4
    vraddhn.s32     d13, q0, q5
    vaddw.u8        q7, q6, d30
    vqmovun.s16     d30, q7
    vst1.8          d30, [r3], r6

    vld1.16         {q1}, [r4], r14
    vld1.8          d30, [r2], r1
    vmovl.s16       q4, d2
    vmovl.s16       q5, d3
    vraddhn.s32     d12, q0, q4
    vraddhn.s32     d13, q0, q5
    vaddw.u8        q7, q6, d30
    vqmovun.s16     d30, q7
    vst1.8          d30, [r3], r6

    vld1.16         {q1}, [r4], r14
    vld1.8          d30, [r2], r1
    vmovl.s16       q4, d2
    vmovl.s16       q5, d3
    vraddhn.s32     d12, q0, q4
    vraddhn.s32     d13, q0, q5
    vaddw.u8        q7, q6, d30
    vqmovun.s16     d30, q7
    vst1.8          d30, [r3], r6

    vld1.16         {q1}, [r4], r14
    vld1.8          d30, [r2], r1
    vmovl.s16       q4, d2
    vmovl.s16       q5, d3
    vraddhn.s32     d12, q0, q4
    vraddhn.s32     d13, q0, q5
    vaddw.u8        q7, q6, d30
    vqmovun.s16     d30, q7
    vst1.8          d30, [r3], r6


    ldmfd           sp!, {r4-r12, pc}




@/**
@ *******************************************************************************
@ *
@ * ;brief
@ *  This function performs Inverse transform  and reconstruction for 8x8
@ * input block
@ *
@ * ;par Description:
@ *  Performs inverse transform and adds the prediction  data and clips output
@ * to 8 bit
@ *
@ * ;param[in] pi2_src
@ *  Input 8x8 coefficients
@ *
@ * ;param[in] pi2_tmp
@ *  Temporary 8x8 buffer for storing inverse
@ *
@ *  transform
@ *  1st stage output
@ *
@ * ;param[in] pu1_pred
@ *  Prediction 8x8 block
@ *
@ * ;param[out] pu1_dst
@ *  Output 8x8 block
@ *
@ * ;param[in] src_strd
@ *  Input stride
@ *
@ * ;param[in] pred_strd
@ *  Prediction stride
@ *
@ * ;param[in] dst_strd
@ *  Output Stride
@ *
@ * ;param[in] shift
@ *  Output shift
@ *
@ * ;param[in] zero_cols
@ *  Zero columns in pi2_src
@ *
@ * ;returns  Void
@ *
@ * ;remarks
@ *  None
@ *
@ *******************************************************************************
@ */

@void impeg2_itrans_recon_8x8(WORD16 *pi2_src,
@                            WORD16 *pi2_tmp,
@                            UWORD8 *pu1_pred,
@                            UWORD8 *pu1_dst,
@                            WORD32 src_strd,
@                            WORD32 pred_strd,
@                            WORD32 dst_strd,
@                            WORD32 zero_cols
@                            WORD32 zero_rows               )

@**************Variables Vs Registers*************************
@   r0 => *pi2_src
@   r1 => *pi2_tmp
@   r2 => *pu1_pred
@   r3 => *pu1_dst
@   src_strd
@   pred_strd
@   dst_strd
@   zero_cols



    .global impeg2_idct_recon_a9q
impeg2_idct_recon_a9q:
@//Register Usage Reference     - loading and Until IDCT of columns
@// Cosine Constants    -   D0
@// Sine Constants      -   D1
@// Row 0 First Half    -   D2      -   y0
@// Row 1 First Half    -   D6      -   y1
@// Row 2 First Half    -   D3      -   y2
@// Row 3 First Half    -   D7      -   y3
@// Row 4 First Half    -   D10     -   y4
@// Row 5 First Half    -   D14     -   y5
@// Row 6 First Half    -   D11     -   y6
@// Row 7 First Half    -   D15     -   y7

@// Row 0 Second Half   -   D4      -   y0
@// Row 1 Second Half   -   D8      -   y1
@// Row 2 Second Half   -   D5      -   y2
@// Row 3 Second Half   -   D9      -   y3
@// Row 4 Second Half   -   D12     -   y4
@// Row 5 Second Half   -   D16     -   y5
@// Row 6 Second Half   -   D13     -   y6
@// Row 7 Second Half   -   D17     -   y7

    @// Copy the input pointer to another register
    @// Step 1 : load all constants
    stmfd           sp!, {r4-r12, lr}

    ldr             r8, [sp, #44]        @ prediction stride
    ldr             r7, [sp, #48]        @ destination stride
    ldr             r6, [sp, #40]            @ src stride
    ldr             r12, [sp, #52]
    ldr             r11, [sp, #56]
    mov             r6, r6, lsl #1      @ x sizeof(word16)
    add             r9, r0, r6, lsl #1  @ 2 rows

    add             r10, r6, r6, lsl #1 @ 3 rows

    sub             r10, r10, #8        @ - 4 cols * sizeof(WORD16)
    sub             r5, r6, #8          @ src_strd - 4 cols * sizeof(WORD16)


    ldr             r14, gai2_impeg2_idct_first_col_q15_addr1
fcq15_lbl1:
    add             r14, r14, pc
    vld1.16         {d0, d1}, [r14]     @//D0,D1 are used for storing the constant data

    @//Step 2 Load all the input data
    @//Step 3 Operate first 4 colums at a time

    and             r11, r11, #0xff
    and             r12, r12, #0xff

    cmp             r11, #0xf0
    bge             skip_last4_rows


    vld1.16         d2, [r0]!
    vld1.16         d3, [r9]!
    vld1.16         d4, [r0], r5
    vmull.s16       q10, d2, d0[0]      @// y0 * cos4(part of c0 and c1)
    vld1.16         d5, [r9], r5
    vmull.s16       q9, d3, d1[2]       @// y2 * sin2 (Q3 is freed by this time)(part of d1)
    vld1.16         d6, [r0]!
    vld1.16         d7, [r9]!
    vmull.s16       q12, d6, d0[1]      @// y1 * cos1(part of b0)
    vld1.16         d8, [r0], r10
    vmull.s16       q13, d6, d0[3]      @// y1 * cos3(part of b1)
    vld1.16         d9, [r9], r10
    vmull.s16       q14, d6, d1[1]      @// y1 * sin3(part of b2)
    vld1.16         d10, [r0]!
    vmull.s16       q15, d6, d1[3]      @// y1 * sin1(part of b3)
    vld1.16         d11, [r9]!
    vmlal.s16       q12, d7, d0[3]      @// y1 * cos1 + y3 * cos3(part of b0)
    vld1.16         d12, [r0], r5
    vmlsl.s16       q13, d7, d1[3]      @// y1 * cos3 - y3 * sin1(part of b1)
    vld1.16         d13, [r9], r5
    vmlsl.s16       q14, d7, d0[1]      @// y1 * sin3 - y3 * cos1(part of b2)
    vld1.16         d14, [r0]!
    vmlsl.s16       q15, d7, d1[1]      @// y1 * sin1 - y3 * sin3(part of b3)
    vld1.16         d15, [r9]!
    vmull.s16       q11, d10, d0[0]     @// y4 * cos4(part of c0 and c1)
    vld1.16         d16, [r0], r10
    vmull.s16       q3, d3, d0[2]       @// y2 * cos2(part of d0)
    vld1.16         d17, [r9], r10

    @/* This following was activated when alignment is not there */
@// VLD1.16     D2,[r0]!
@// VLD1.16     D3,[r2]!
@// VLD1.16     D4,[r0]!
@// VLD1.16     D5,[r2]!
@// VLD1.16     D6,[r0]!
@// VLD1.16     D7,[r2]!
@// VLD1.16     D8,[r0],r3
@// VLD1.16     D9,[r2],r3
@// VLD1.16     D10,[r0]!
@// VLD1.16     D11,[r2]!
@// VLD1.16     D12,[r0]!
@// VLD1.16     D13,[r2]!
@// VLD1.16     D14,[r0]!
@// VLD1.16     D15,[r2]!
@// VLD1.16     D16,[r0],r3
@// VLD1.16     D17,[r2],r3




    vmlal.s16       q12, d14, d1[1]     @// y1 * cos1 + y3 * cos3 + y5 * sin3(part of b0)
    vmlsl.s16       q13, d14, d0[1]     @// y1 * cos3 - y3 * sin1 - y5 * cos1(part of b1)
    vmlal.s16       q14, d14, d1[3]     @// y1 * sin3 - y3 * cos1 + y5 * sin1(part of b2)
    vmlal.s16       q15, d14, d0[3]     @// y1 * sin1 - y3 * sin3 + y5 * cos3(part of b3)

    vmlsl.s16       q9, d11, d0[2]      @// d1 = y2 * sin2 - y6 * cos2(part of a0 and a1)
    vmlal.s16       q3, d11, d1[2]      @// d0 = y2 * cos2 + y6 * sin2(part of a0 and a1)

    vadd.s32        q5, q10, q11        @// c0 = y0 * cos4 + y4 * cos4(part of a0 and a1)
    vsub.s32        q10, q10, q11       @// c1 = y0 * cos4 - y4 * cos4(part of a0 and a1)

    vmlal.s16       q12, d15, d1[3]     @// b0 = y1 * cos1 + y3 * cos3 + y5 * sin3 + y7 * sin1(part of r0,r7)
    vmlsl.s16       q13, d15, d1[1]     @// b1 = y1 * cos3 - y3 * sin1 - y5 * cos1 - y7 * sin3(part of r1,r6)
    vmlal.s16       q14, d15, d0[3]     @// b2 = y1 * sin3 - y3 * cos1 + y5 * sin1 + y7 * cos3(part of r2,r5)
    vmlsl.s16       q15, d15, d0[1]     @// b3 = y1 * sin1 - y3 * sin3 + y5 * cos3 - y7 * cos1(part of r3,r4)

    vadd.s32        q7, q5, q3          @// a0 = c0 + d0(part of r0,r7)
    vsub.s32        q5, q5, q3          @// a3 = c0 - d0(part of r3,r4)
    vsub.s32        q11, q10, q9        @// a2 = c1 - d1(part of r2,r5)
    vadd.s32        q9, q10, q9         @// a1 = c1 + d1(part of r1,r6)

    vadd.s32        q10, q7, q12        @// a0 + b0(part of r0)
    vsub.s32        q3, q7, q12         @// a0 - b0(part of r7)

    vadd.s32        q12, q11, q14       @// a2 + b2(part of r2)
    vsub.s32        q11, q11, q14       @// a2 - b2(part of r5)

    vadd.s32        q14, q9, q13        @// a1 + b1(part of r1)
    vsub.s32        q9, q9, q13         @// a1 - b1(part of r6)

    vadd.s32        q13, q5, q15        @// a3 + b3(part of r3)
    vsub.s32        q15, q5, q15        @// a3 - b3(part of r4)

    vqrshrn.s32     d2, q10, #idct_stg1_shift @// r0 = (a0 + b0 + rnd) >> 7(IDCT_STG1_SHIFT)
    vqrshrn.s32     d15, q3, #idct_stg1_shift @// r7 = (a0 - b0 + rnd) >> 7(IDCT_STG1_SHIFT)
    vqrshrn.s32     d3, q12, #idct_stg1_shift @// r2 = (a2 + b2 + rnd) >> 7(IDCT_STG1_SHIFT)
    vqrshrn.s32     d14, q11, #idct_stg1_shift @// r5 = (a2 - b2 + rnd) >> 7(IDCT_STG1_SHIFT)
    vqrshrn.s32     d6, q14, #idct_stg1_shift @// r1 = (a1 + b1 + rnd) >> 7(IDCT_STG1_SHIFT)
    vqrshrn.s32     d11, q9, #idct_stg1_shift @// r6 = (a1 - b1 + rnd) >> 7(IDCT_STG1_SHIFT)
    vqrshrn.s32     d7, q13, #idct_stg1_shift @// r3 = (a3 + b3 + rnd) >> 7(IDCT_STG1_SHIFT)
    vqrshrn.s32     d10, q15, #idct_stg1_shift @// r4 = (a3 - b3 + rnd) >> 7(IDCT_STG1_SHIFT)


    b               last4_cols



skip_last4_rows:


    ldr             r14, gai2_impeg2_idct_first_col_q15_addr2
fcq15_lbl2:
    add             r14, r14, pc
    vld1.16         {d0, d1}, [r14]     @//D0,D1 are used for storing the constant data

    vld1.16         d2, [r0]!
    vld1.16         d3, [r9]!
    vld1.16         d4, [r0], r5
    vld1.16         d5, [r9], r5
    vld1.16         d6, [r0]!
    vld1.16         d7, [r9]!
    vld1.16         d8, [r0], r10
    vld1.16         d9, [r9], r10



    vmov.s16        q6, #0
    vmov.s16        q8, #0




    vmull.s16       q12, d6, d0[1]      @// y1 * cos1(part of b0)
    vmull.s16       q13, d6, d0[3]      @// y1 * cos3(part of b1)
    vmull.s16       q14, d6, d1[1]      @// y1 * sin3(part of b2)
    vmull.s16       q15, d6, d1[3]      @// y1 * sin1(part of b3)

    vmlal.s16       q12, d7, d0[3]      @// y1 * cos1 + y3 * cos3(part of b0)
    vmlsl.s16       q13, d7, d1[3]      @// y1 * cos3 - y3 * sin1(part of b1)
    vmlsl.s16       q14, d7, d0[1]      @// y1 * sin3 - y3 * cos1(part of b2)
    vmlsl.s16       q15, d7, d1[1]      @// y1 * sin1 - y3 * sin3(part of b3)

    vmull.s16       q9, d3, d1[2]       @// y2 * sin2 (Q3 is freed by this time)(part of d1)
    vmull.s16       q3, d3, d0[2]       @// y2 * cos2(part of d0)

    vmull.s16       q10, d2, d0[0]      @// y0 * cos4(part of c0 and c1)


    vadd.s32        q7, q10, q3         @// a0 = c0 + d0(part of r0,r7)
    vsub.s32        q5, q10, q3         @// a3 = c0 - d0(part of r3,r4)
    vsub.s32        q11, q10, q9        @// a2 = c1 - d1(part of r2,r5)
    vadd.s32        q9, q10, q9         @// a1 = c1 + d1(part of r1,r6)

    vadd.s32        q10, q7, q12        @// a0 + b0(part of r0)
    vsub.s32        q3, q7, q12         @// a0 - b0(part of r7)

    vadd.s32        q12, q11, q14       @// a2 + b2(part of r2)
    vsub.s32        q11, q11, q14       @// a2 - b2(part of r5)

    vadd.s32        q14, q9, q13        @// a1 + b1(part of r1)
    vsub.s32        q9, q9, q13         @// a1 - b1(part of r6)

    vadd.s32        q13, q5, q15        @// a3 + b3(part of r3)
    vsub.s32        q15, q5, q15        @// a3 - b3(part of r4)

    vqrshrn.s32     d2, q10, #idct_stg1_shift @// r0 = (a0 + b0 + rnd) >> 7(IDCT_STG1_SHIFT)
    vqrshrn.s32     d15, q3, #idct_stg1_shift @// r7 = (a0 - b0 + rnd) >> 7(IDCT_STG1_SHIFT)
    vqrshrn.s32     d3, q12, #idct_stg1_shift @// r2 = (a2 + b2 + rnd) >> 7(IDCT_STG1_SHIFT)
    vqrshrn.s32     d14, q11, #idct_stg1_shift @// r5 = (a2 - b2 + rnd) >> 7(IDCT_STG1_SHIFT)
    vqrshrn.s32     d6, q14, #idct_stg1_shift @// r1 = (a1 + b1 + rnd) >> 7(IDCT_STG1_SHIFT)
    vqrshrn.s32     d11, q9, #idct_stg1_shift @// r6 = (a1 - b1 + rnd) >> 7(IDCT_STG1_SHIFT)
    vqrshrn.s32     d7, q13, #idct_stg1_shift @// r3 = (a3 + b3 + rnd) >> 7(IDCT_STG1_SHIFT)
    vqrshrn.s32     d10, q15, #idct_stg1_shift @// r4 = (a3 - b3 + rnd) >> 7(IDCT_STG1_SHIFT)


last4_cols:


    cmp             r12, #0xf0
    bge             skip_last4cols

    ldr             r14, gai2_impeg2_idct_first_col_q15_addr3
fcq15_lbl3:
    add             r14, r14, pc
    vld1.16         {d0, d1}, [r14]     @//D0,D1 are used for storing the constant data

    vmull.s16       q12, d8, d0[1]      @// y1 * cos1(part of b0)
    vmull.s16       q13, d8, d0[3]      @// y1 * cos3(part of b1)
    vmull.s16       q14, d8, d1[1]      @// y1 * sin3(part of b2)
    vmull.s16       q15, d8, d1[3]      @// y1 * sin1(part of b3)

    vmlal.s16       q12, d9, d0[3]      @// y1 * cos1 + y3 * cos3(part of b0)
    vmlsl.s16       q13, d9, d1[3]      @// y1 * cos3 - y3 * sin1(part of b1)
    vmlsl.s16       q14, d9, d0[1]      @// y1 * sin3 - y3 * cos1(part of b2)
    vmlsl.s16       q15, d9, d1[1]      @// y1 * sin1 - y3 * sin3(part of b3)

    vmull.s16       q9, d5, d1[2]       @// y2 * sin2 (Q4 is freed by this time)(part of d1)
    vmull.s16       q4, d5, d0[2]       @// y2 * cos2(part of d0)

    vmull.s16       q10, d4, d0[0]      @// y0 * cos4(part of c0 and c1)
    vmull.s16       q11, d12, d0[0]     @// y4 * cos4(part of c0 and c1)

    vmlal.s16       q12, d16, d1[1]     @// y1 * cos1 + y3 * cos3 + y5 * sin3(part of b0)
    vmlsl.s16       q13, d16, d0[1]     @// y1 * cos3 - y3 * sin1 - y5 * cos1(part of b1)
    vmlal.s16       q14, d16, d1[3]     @// y1 * sin3 - y3 * cos1 + y5 * sin1(part of b2)
    vmlal.s16       q15, d16, d0[3]     @// y1 * sin1 - y3 * sin3 + y5 * cos3(part of b3)

    vmlsl.s16       q9, d13, d0[2]      @// d1 = y2 * sin2 - y6 * cos2(part of a0 and a1)
    vmlal.s16       q4, d13, d1[2]      @// d0 = y2 * cos2 + y6 * sin2(part of a0 and a1)

    vadd.s32        q6, q10, q11        @// c0 = y0 * cos4 + y4 * cos4(part of a0 and a1)
    vsub.s32        q10, q10, q11       @// c1 = y0 * cos4 - y4 * cos4(part of a0 and a1)

    vmlal.s16       q12, d17, d1[3]     @// b0 = y1 * cos1 + y3 * cos3 + y5 * sin3 + y7 * sin1(part of e0,e7)
    vmlsl.s16       q13, d17, d1[1]     @// b1 = y1 * cos3 - y3 * sin1 - y5 * cos1 - y7 * sin3(part of e1,e6)
    vmlal.s16       q14, d17, d0[3]     @// b2 = y1 * sin3 - y3 * cos1 + y5 * sin1 + y7 * cos3(part of e2,e5)
    vmlsl.s16       q15, d17, d0[1]     @// b3 = y1 * sin1 - y3 * sin3 + y5 * cos3 - y7 * cos1(part of e3,e4)

    vadd.s32        q8, q6, q4          @// a0 = c0 + d0(part of e0,e7)
    vsub.s32        q6, q6, q4          @// a3 = c0 - d0(part of e3,e4)
    vsub.s32        q11, q10, q9        @// a2 = c1 - d1(part of e2,e5)
    vadd.s32        q9, q10, q9         @// a1 = c1 + d1(part of e1,e6)

    vadd.s32        q10, q8, q12        @// a0 + b0(part of e0)
    vsub.s32        q4, q8, q12         @// a0 - b0(part of e7)

    vadd.s32        q12, q11, q14       @// a2 + b2(part of e2)
    vsub.s32        q11, q11, q14       @// a2 - b2(part of e5)

    vadd.s32        q14, q9, q13        @// a1 + b1(part of e1)
    vsub.s32        q9, q9, q13         @// a1 - b1(part of e6)

    vadd.s32        q13, q6, q15        @// a3 + b3(part of e3)
    vsub.s32        q15, q6, q15        @// a3 - b3(part of r4)

    vqrshrn.s32     d4, q10, #idct_stg1_shift @// r0 = (a0 + b0 + rnd) >> 7(IDCT_STG1_SHIFT)
    vqrshrn.s32     d17, q4, #idct_stg1_shift @// r7 = (a0 - b0 + rnd) >> 7(IDCT_STG1_SHIFT)
    vqrshrn.s32     d5, q12, #idct_stg1_shift @// r2 = (a2 + b2 + rnd) >> 7(IDCT_STG1_SHIFT)
    vqrshrn.s32     d16, q11, #idct_stg1_shift @// r5 = (a2 - b2 + rnd) >> 7(IDCT_STG1_SHIFT)
    vqrshrn.s32     d8, q14, #idct_stg1_shift @// r1 = (a1 + b1 + rnd) >> 7(IDCT_STG1_SHIFT)
    vqrshrn.s32     d13, q9, #idct_stg1_shift @// r6 = (a1 - b1 + rnd) >> 7(IDCT_STG1_SHIFT)
    vqrshrn.s32     d9, q13, #idct_stg1_shift @// r3 = (a3 + b3 + rnd) >> 7(IDCT_STG1_SHIFT)
    vqrshrn.s32     d12, q15, #idct_stg1_shift @// r4 = (a3 - b3 + rnd) >> 7(IDCT_STG1_SHIFT)
    b               end_skip_last4cols



skip_last4cols:



    ldr             r14, gai2_impeg2_idct_first_col_q11_addr1
fcq11_lbl1:
    add             r14, r14, pc
    vld1.16         {d0, d1}, [r14]     @//D0,D1 are used for storing the constant data



    vtrn.16         q1, q3              @//[r3,r1],[r2,r0] first qudrant transposing

    vtrn.16         q5, q7              @//[r7,r5],[r6,r4] third qudrant transposing


    vtrn.32         d6, d7              @//r0,r1,r2,r3 first qudrant transposing continued.....
    vtrn.32         d2, d3              @//r0,r1,r2,r3 first qudrant transposing continued.....

    vtrn.32         d10, d11            @//r4,r5,r6,r7 third qudrant transposing continued.....
    vtrn.32         d14, d15            @//r4,r5,r6,r7 third qudrant transposing continued.....


    vmull.s16       q12, d6, d0[1]      @// y1 * cos1(part of b0)
    vmull.s16       q13, d6, d0[3]      @// y1 * cos3(part of b1)
    vmull.s16       q14, d6, d1[1]      @// y1 * sin3(part of b2)
    vmull.s16       q15, d6, d1[3]      @// y1 * sin1(part of b3)

    vmlal.s16       q12, d7, d0[3]      @// y1 * cos1 + y3 * cos3(part of b0)
    vmlsl.s16       q13, d7, d1[3]      @// y1 * cos3 - y3 * sin1(part of b1)
    vmlsl.s16       q14, d7, d0[1]      @// y1 * sin3 - y3 * cos1(part of b2)
    vmlsl.s16       q15, d7, d1[1]      @// y1 * sin1 - y3 * sin3(part of b3)

    vmull.s16       q10, d2, d0[0]      @// y0 * cos4(part of c0 and c1)
@   VMULL.S16   Q11,D4,D0[0]                    ;// y4 * cos4(part of c0 and c1)

    vmull.s16       q9, d3, d1[2]       @// y2 * sin2 (Q3 is freed by this time)(part of d1)
    vmull.s16       q3, d3, d0[2]       @// y2 * cos2(part of d0)




    vsub.s32        q11, q10, q3        @// a3 = c0 - d0(part of r3,r4)
    vadd.s32        q2, q10, q3         @// a0 = c0 + d0(part of r0,r7)


    vadd.s32        q1, q2, q12

    vsub.s32        q3, q2, q12

    vadd.s32        q4, q11, q15

    vsub.s32        q12, q11, q15

    vqrshrn.s32     d5, q4, #idct_stg2_shift
    vqrshrn.s32     d2, q1, #idct_stg2_shift
    vqrshrn.s32     d9, q3, #idct_stg2_shift
    vqrshrn.s32     d6, q12, #idct_stg2_shift

    vsub.s32        q11, q10, q9        @// a2 = c1 - d1(part of r2,r5)
    vadd.s32        q9, q10, q9         @// a1 = c1 + d1(part of r1,r6)


    vadd.s32        q15, q11, q14

    vsub.s32        q12, q11, q14

    vadd.s32        q14, q9, q13

    vsub.s32        q11, q9, q13
    vqrshrn.s32     d4, q15, #idct_stg2_shift
    vqrshrn.s32     d7, q12, #idct_stg2_shift
    vqrshrn.s32     d3, q14, #idct_stg2_shift
    vqrshrn.s32     d8, q11, #idct_stg2_shift










    vmull.s16       q12, d14, d0[1]     @// y1 * cos1(part of b0)

    vmull.s16       q13, d14, d0[3]     @// y1 * cos3(part of b1)
    vmull.s16       q14, d14, d1[1]     @// y1 * sin3(part of b2)
    vmull.s16       q15, d14, d1[3]     @// y1 * sin1(part of b3)

    vmlal.s16       q12, d15, d0[3]     @// y1 * cos1 + y3 * cos3(part of b0)
    vtrn.16         d2, d3
    vmlsl.s16       q13, d15, d1[3]     @// y1 * cos3 - y3 * sin1(part of b1)
    vtrn.16         d4, d5
    vmlsl.s16       q14, d15, d0[1]     @// y1 * sin3 - y3 * cos1(part of b2)
    vtrn.16         d6, d7
    vmlsl.s16       q15, d15, d1[1]     @// y1 * sin1 - y3 * sin3(part of b3)
    vtrn.16         d8, d9
    vmull.s16       q10, d10, d0[0]     @// y0 * cos4(part of c0 and c1)
    vtrn.32         d2, d4

    vtrn.32         d3, d5
    vmull.s16       q9, d11, d1[2]      @// y2 * sin2 (Q7 is freed by this time)(part of d1)
    vtrn.32         d6, d8
    vmull.s16       q7, d11, d0[2]      @// y2 * cos2(part of d0)
    vtrn.32         d7, d9


    add             r4, r2, r8, lsl #1  @ r4 = r2 + pred_strd * 2    => r4 points to 3rd row of pred data


    add             r5, r8, r8, lsl #1  @


    add             r0, r3, r7, lsl #1  @ r0 points to 3rd row of dest data


    add             r10, r7, r7, lsl #1 @


    vswp            d3, d6


    vswp            d5, d8


    vsub.s32        q11, q10, q7        @// a3 = c0 - d0(part of r3,r4)
    vadd.s32        q6, q10, q7         @// a0 = c0 + d0(part of r0,r7)


    vadd.s32        q0, q6, q12


    vsub.s32        q12, q6, q12


    vadd.s32        q6, q11, q15


    vsub.s32        q7, q11, q15

    vqrshrn.s32     d10, q0, #idct_stg2_shift
    vqrshrn.s32     d17, q12, #idct_stg2_shift
    vqrshrn.s32     d13, q6, #idct_stg2_shift
    vqrshrn.s32     d14, q7, #idct_stg2_shift

    vsub.s32        q11, q10, q9        @// a2 = c1 - d1(part of r2,r5)
    vadd.s32        q9, q10, q9         @// a1 = c1 + d1(part of r1,r6)


    vadd.s32        q0, q11, q14


    vsub.s32        q12, q11, q14


    vadd.s32        q14, q9, q13


    vsub.s32        q13, q9, q13
    vld1.8          d18, [r2], r8

    vqrshrn.s32     d12, q0, #idct_stg2_shift
    vld1.8          d20, [r2], r5


    vqrshrn.s32     d15, q12, #idct_stg2_shift
    vld1.8          d19, [r2], r8




    vqrshrn.s32     d11, q14, #idct_stg2_shift
    vld1.8          d22, [r4], r8




    vqrshrn.s32     d16, q13, #idct_stg2_shift
    vld1.8          d21, [r2], r5


    b               pred_buff_addition
end_skip_last4cols:

    ldr             r14, gai2_impeg2_idct_first_col_q11_addr2
fcq11_lbl2:
    add             r14, r14, pc
    vld1.16         {d0, d1}, [r14]     @//D0,D1 are used for storing the constant data


@/* Now the Idct of columns is done, transpose so that row idct done efficiently(step5) */
    vtrn.16         q1, q3              @//[r3,r1],[r2,r0] first qudrant transposing
    vtrn.16         q2, q4              @//[r3,r1],[r2,r0] second qudrant transposing
    vtrn.16         q5, q7              @//[r7,r5],[r6,r4] third qudrant transposing
    vtrn.16         q6, q8              @//[r7,r5],[r6,r4] fourth qudrant transposing

    vtrn.32         d6, d7              @//r0,r1,r2,r3 first qudrant transposing continued.....
    vtrn.32         d2, d3              @//r0,r1,r2,r3 first qudrant transposing continued.....
    vtrn.32         d4, d5              @//r0,r1,r2,r3 second qudrant transposing continued.....
    vtrn.32         d8, d9              @//r0,r1,r2,r3 second qudrant transposing continued.....
    vtrn.32         d10, d11            @//r4,r5,r6,r7 third qudrant transposing continued.....
    vtrn.32         d14, d15            @//r4,r5,r6,r7 third qudrant transposing continued.....
    vtrn.32         d12, d13            @//r4,r5,r6,r7 fourth qudrant transposing continued.....
    vtrn.32         d16, d17            @//r4,r5,r6,r7 fourth qudrant transposing continued.....

    @//step6 Operate on first four rows and find their idct
    @//Register Usage Reference     - storing and IDCT of rows
@// Cosine Constants    -   D0
@// Sine Constants      -   D1
@// Element 0 First four    -   D2      -   y0
@// Element 1 First four    -   D6      -   y1
@// Element 2 First four    -   D3      -   y2
@// Element 3 First four    -   D7      -   y3
@// Element 4 First four    -   D4      -   y4
@// Element 5 First four    -   D8      -   y5
@// Element 6 First four    -   D5      -   y6
@// Element 7 First four    -   D9      -   y7
@// Element 0 Second four   -   D10     -   y0
@// Element 1 Second four   -   D14     -   y1
@// Element 2 Second four   -   D11     -   y2
@// Element 3 Second four   -   D15     -   y3
@// Element 4 Second four   -   D12     -   y4
@// Element 5 Second four   -   D16     -   y5
@// Element 6 Second four   -   D13     -   y6
@// Element 7 Second four   -   D17     -   y7

    @// Map between first kernel code seq and current
@//     D2  ->  D2
@//     D6  ->  D6
@//     D3  ->  D3
@//     D7  ->  D7
@//     D10 ->  D4
@//     D14 ->  D8
@//     D11 ->  D5
@//     D15 ->  D9
@//     Q3  ->  Q3
@//     Q5  ->  Q2
@//     Q7  ->  Q4

    vmull.s16       q12, d6, d0[1]      @// y1 * cos1(part of b0)
    vmull.s16       q13, d6, d0[3]      @// y1 * cos3(part of b1)
    vmull.s16       q14, d6, d1[1]      @// y1 * sin3(part of b2)
    vmull.s16       q15, d6, d1[3]      @// y1 * sin1(part of b3)

    vmlal.s16       q12, d7, d0[3]      @// y1 * cos1 + y3 * cos3(part of b0)
    vmlsl.s16       q13, d7, d1[3]      @// y1 * cos3 - y3 * sin1(part of b1)
    vmlsl.s16       q14, d7, d0[1]      @// y1 * sin3 - y3 * cos1(part of b2)
    vmlsl.s16       q15, d7, d1[1]      @// y1 * sin1 - y3 * sin3(part of b3)

    vmull.s16       q10, d2, d0[0]      @// y0 * cos4(part of c0 and c1)
    vmull.s16       q11, d4, d0[0]      @// y4 * cos4(part of c0 and c1)

    vmull.s16       q9, d3, d1[2]       @// y2 * sin2 (Q3 is freed by this time)(part of d1)
    vmull.s16       q3, d3, d0[2]       @// y2 * cos2(part of d0)


    vmlal.s16       q12, d8, d1[1]      @// y1 * cos1 + y3 * cos3 + y5 * sin3(part of b0)
    vmlsl.s16       q13, d8, d0[1]      @// y1 * cos3 - y3 * sin1 - y5 * cos1(part of b1)
    vmlal.s16       q14, d8, d1[3]      @// y1 * sin3 - y3 * cos1 + y5 * sin1(part of b2)
    vmlal.s16       q15, d8, d0[3]      @// y1 * sin1 - y3 * sin3 + y5 * cos3(part of b3)

    vmlsl.s16       q9, d5, d0[2]       @// d1 = y2 * sin2 - y6 * cos2(part of a0 and a1)
    vmlal.s16       q3, d5, d1[2]       @// d0 = y2 * cos2 + y6 * sin2(part of a0 and a1)

    vadd.s32        q1, q10, q11        @// c0 = y0 * cos4 + y4 * cos4(part of a0 and a1)
    vsub.s32        q10, q10, q11       @// c1 = y0 * cos4 - y4 * cos4(part of a0 and a1)

    vmlal.s16       q12, d9, d1[3]      @// b0 = y1 * cos1 + y3 * cos3 + y5 * sin3 + y7 * sin1(part of r0,r7)
    vmlsl.s16       q13, d9, d1[1]      @// b1 = y1 * cos3 - y3 * sin1 - y5 * cos1 - y7 * sin3(part of r1,r6)
    vmlal.s16       q14, d9, d0[3]      @// b2 = y1 * sin3 - y3 * cos1 + y5 * sin1 + y7 * cos3(part of r2,r5)
    vmlsl.s16       q15, d9, d0[1]      @// b3 = y1 * sin1 - y3 * sin3 + y5 * cos3 - y7 * cos1(part of r3,r4)

    vsub.s32        q11, q1, q3         @// a3 = c0 - d0(part of r3,r4)
    vadd.s32        q2, q1, q3          @// a0 = c0 + d0(part of r0,r7)


    vadd.s32        q1, q2, q12

    vsub.s32        q3, q2, q12

    vadd.s32        q4, q11, q15

    vsub.s32        q12, q11, q15

    vqrshrn.s32     d5, q4, #idct_stg2_shift
    vqrshrn.s32     d2, q1, #idct_stg2_shift
    vqrshrn.s32     d9, q3, #idct_stg2_shift
    vqrshrn.s32     d6, q12, #idct_stg2_shift

    vsub.s32        q11, q10, q9        @// a2 = c1 - d1(part of r2,r5)
    vadd.s32        q9, q10, q9         @// a1 = c1 + d1(part of r1,r6)


    vadd.s32        q15, q11, q14

    vsub.s32        q12, q11, q14

    vadd.s32        q14, q9, q13

    vsub.s32        q11, q9, q13
    vqrshrn.s32     d4, q15, #idct_stg2_shift
    vqrshrn.s32     d7, q12, #idct_stg2_shift
    vqrshrn.s32     d3, q14, #idct_stg2_shift
    vqrshrn.s32     d8, q11, #idct_stg2_shift










    vmull.s16       q12, d14, d0[1]     @// y1 * cos1(part of b0)

    vmull.s16       q13, d14, d0[3]     @// y1 * cos3(part of b1)
    vmull.s16       q14, d14, d1[1]     @// y1 * sin3(part of b2)
    vmull.s16       q15, d14, d1[3]     @// y1 * sin1(part of b3)

    vmlal.s16       q12, d15, d0[3]     @// y1 * cos1 + y3 * cos3(part of b0)
    vtrn.16         d2, d3
    vmlsl.s16       q13, d15, d1[3]     @// y1 * cos3 - y3 * sin1(part of b1)
    vtrn.16         d4, d5
    vmlsl.s16       q14, d15, d0[1]     @// y1 * sin3 - y3 * cos1(part of b2)
    vtrn.16         d6, d7
    vmlsl.s16       q15, d15, d1[1]     @// y1 * sin1 - y3 * sin3(part of b3)
    vtrn.16         d8, d9
    vmull.s16       q10, d10, d0[0]     @// y0 * cos4(part of c0 and c1)
    vtrn.32         d2, d4
    vmull.s16       q11, d12, d0[0]     @// y4 * cos4(part of c0 and c1)
    vtrn.32         d3, d5
    vmull.s16       q9, d11, d1[2]      @// y2 * sin2 (Q7 is freed by this time)(part of d1)
    vtrn.32         d6, d8
    vmull.s16       q7, d11, d0[2]      @// y2 * cos2(part of d0)
    vtrn.32         d7, d9
    vmlal.s16       q12, d16, d1[1]     @// y1 * cos1 + y3 * cos3 + y5 * sin3(part of b0)

    add             r4, r2, r8, lsl #1  @ r4 = r2 + pred_strd * 2    => r4 points to 3rd row of pred data
    vmlsl.s16       q13, d16, d0[1]     @// y1 * cos3 - y3 * sin1 - y5 * cos1(part of b1)

    add             r5, r8, r8, lsl #1  @
    vmlal.s16       q14, d16, d1[3]     @// y1 * sin3 - y3 * cos1 + y5 * sin1(part of b2)

    add             r0, r3, r7, lsl #1  @ r0 points to 3rd row of dest data
    vmlal.s16       q15, d16, d0[3]     @// y1 * sin1 - y3 * sin3 + y5 * cos3(part of b3)

    add             r10, r7, r7, lsl #1 @
    vmlsl.s16       q9, d13, d0[2]      @// d1 = y2 * sin2 - y6 * cos2(part of a0 and a1)


    vmlal.s16       q7, d13, d1[2]      @// d0 = y2 * cos2 + y6 * sin2(part of a0 and a1)

    vadd.s32        q6, q10, q11        @// c0 = y0 * cos4 + y4 * cos4(part of a0 and a1)
    vsub.s32        q10, q10, q11       @// c1 = y0 * cos4 - y4 * cos4(part of a0 and a1)

    vmlal.s16       q12, d17, d1[3]     @// b0 = y1 * cos1 + y3 * cos3 + y5 * sin3 + y7 * sin1(part of r0,r7)
    vswp            d3, d6
    vmlsl.s16       q13, d17, d1[1]     @// b1 = y1 * cos3 - y3 * sin1 - y5 * cos1 - y7 * sin3(part of r1,r6)

    vswp            d5, d8
    vmlal.s16       q14, d17, d0[3]     @// b2 = y1 * sin3 - y3 * cos1 + y5 * sin1 + y7 * cos3(part of r2,r5)
    vmlsl.s16       q15, d17, d0[1]     @// b3 = y1 * sin1 - y3 * sin3 + y5 * cos3 - y7 * cos1(part of r3,r4)

    vsub.s32        q11, q6, q7         @// a3 = c0 - d0(part of r3,r4)
    vadd.s32        q6, q6, q7          @// a0 = c0 + d0(part of r0,r7)


    vadd.s32        q0, q6, q12


    vsub.s32        q12, q6, q12


    vadd.s32        q6, q11, q15


    vsub.s32        q7, q11, q15

    vqrshrn.s32     d10, q0, #idct_stg2_shift
    vqrshrn.s32     d17, q12, #idct_stg2_shift
    vqrshrn.s32     d13, q6, #idct_stg2_shift
    vqrshrn.s32     d14, q7, #idct_stg2_shift

    vsub.s32        q11, q10, q9        @// a2 = c1 - d1(part of r2,r5)
    vadd.s32        q9, q10, q9         @// a1 = c1 + d1(part of r1,r6)


    vadd.s32        q0, q11, q14


    vsub.s32        q12, q11, q14


    vadd.s32        q14, q9, q13


    vsub.s32        q13, q9, q13
    vld1.8          d18, [r2], r8

    vqrshrn.s32     d12, q0, #idct_stg2_shift
    vld1.8          d20, [r2], r5


    vqrshrn.s32     d15, q12, #idct_stg2_shift
    vld1.8          d19, [r2], r8




    vqrshrn.s32     d11, q14, #idct_stg2_shift
    vld1.8          d22, [r4], r8




    vqrshrn.s32     d16, q13, #idct_stg2_shift
    vld1.8          d21, [r2], r5




pred_buff_addition:


    vtrn.16         d10, d11
    vld1.8          d24, [r4], r5

    vtrn.16         d12, d13
    vld1.8          d23, [r4], r8

    vaddw.u8        q1, q1, d18
    vld1.8          d25, [r4], r5

    vtrn.16         d14, d15
    vaddw.u8        q2, q2, d22

    vtrn.16         d16, d17
    vaddw.u8        q3, q3, d20

    vtrn.32         d10, d12
    vaddw.u8        q4, q4, d24

    vtrn.32         d11, d13
    vtrn.32         d14, d16
    vtrn.32         d15, d17

    vswp            d11, d14
    vswp            d13, d16

@ Row values stored in the q register.

@Q1 :r0
@Q3: r1
@Q2: r2
@Q4: r3
@Q5: r4
@Q7: r5
@Q6: r6
@Q8: r7



@/// Adding the prediction buffer









    @ Load prediction data





    @Adding recon with prediction





    vaddw.u8        q5, q5, d19
    vqmovun.s16     d2, q1
    vaddw.u8        q7, q7, d21
    vqmovun.s16     d4, q2
    vaddw.u8        q6, q6, d23
    vqmovun.s16     d6, q3
    vaddw.u8        q8, q8, d25
    vqmovun.s16     d8, q4







    vst1.8          {d2}, [r3], r7
    vqmovun.s16     d10, q5
    vst1.8          {d6}, [r3], r10
    vqmovun.s16     d14, q7
    vst1.8          {d4}, [r0], r7
    vqmovun.s16     d12, q6
    vst1.8          {d8}, [r0], r10
    vqmovun.s16     d16, q8







    vst1.8          {d10}, [r3], r7
    vst1.8          {d14}, [r3], r10
    vst1.8          {d12}, [r0], r7
    vst1.8          {d16}, [r0], r10





    ldmfd           sp!, {r4-r12, pc}



