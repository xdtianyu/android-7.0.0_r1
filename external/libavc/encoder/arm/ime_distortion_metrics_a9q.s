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
@**

@**
@******************************************************************************
@*
@*
@* @brief
@*  This file contains definitions of routines that compute distortion
@*  between two macro/sub blocks of identical dimensions
@*
@* @author
@*  Ittiam
@*
@* @par List of Functions:
@*  - ime_compute_sad_16x16_a9q()
@*  - ime_compute_sad_16x16_fast_a9q()
@*  - ime_compute_sad_16x8_a9q()
@*  - ime_compute_sad_16x16_ea8_a9q()
@*  - ime_calculate_sad2_prog_a9q()
@*  - ime_calculate_sad3_prog_a9q()
@*  - ime_calculate_sad4_prog_a9q()
@*  - ime_sub_pel_compute_sad_16x16_a9q()
@*  - ime_compute_satqd_16x16_lumainter_a9q()
@*  -
@* @remarks
@*  None
@*
@*******************************************************************************
@


@**
@******************************************************************************
@*
@* @brief computes distortion (SAD) between 2 16x16 blocks (fast mode)
@*
@* @par   Description
@*   This functions computes SAD between 2 16x16 blocks. There is a provision
@*   for early exit if the up-to computed SAD exceeds maximum allowed SAD. To
@*   compute the distortion of the entire block set u4_max_sad to USHRT_MAX.
@*
@* @param[in] pu1_src
@*  UWORD8 pointer to the source
@*
@* @param[out] pu1_dst
@*  UWORD8 pointer to the destination
@*
@* @param[in] src_strd
@*  integer source stride
@*
@* @param[in] dst_strd
@*  integer destination stride
@*
@* @param[in] i4_max_sad
@*  integer maximum allowed distortion
@*
@* @param[in] pi4_mb_distortion
@*  integer evaluated sad
@*
@* @remarks
@*
@******************************************************************************
@*
.text
.p2align 2

    .global ime_compute_sad_16x16_fast_a9q

ime_compute_sad_16x16_fast_a9q:

    stmfd         sp!, {r12, lr}
    vpush         {d8-d15}
    lsl           r2, r2, #1
    lsl           r3, r3, #1

    @for bringing buffer2 into cache..., dummy load instructions
    @LDR         r12,[r1]

    vld1.8        {d4, d5}, [r0], r2
    vld1.8        {d6, d7}, [r1], r3
    mov           r12, #6
    vld1.8        {d8, d9}, [r0], r2
    vabdl.u8      q0, d6, d4
    vabdl.u8      q1, d7, d5
    vld1.8        {d10, d11}, [r1], r3

loop_sad_16x16_fast:

    vld1.8        {d4, d5}, [r0], r2
    vabal.u8      q0, d10, d8
    vabal.u8      q1, d11, d9
    vld1.8        {d6, d7}, [r1], r3
    subs          r12, #2
    vld1.8        {d8, d9}, [r0], r2
    vabal.u8      q0, d6, d4
    vabal.u8      q1, d7, d5
    vld1.8        {d10, d11}, [r1], r3

    bne           loop_sad_16x16_fast

    vabal.u8      q0, d10, d8
    vabal.u8      q1, d11, d9

    vadd.i16      q0, q0, q1
    vadd.i16      d0, d1, d0
    vpop          {d8-d15}
    ldr           r12, [sp, #12]
    vpaddl.u16    d0, d0
    vpaddl.u32    d0, d0
    vshl.u32      d0, d0, #1
    vst1.32       {d0[0]}, [r12]

    ldmfd         sp!, {r12, pc}




@**
@******************************************************************************
@*
@*  @brief computes distortion (SAD) between 2 16x8  blocks
@*
@*
@*  @par   Description
@*   This functions computes SAD between 2 16x8 blocks. There is a provision
@*   for early exit if the up-to computed SAD exceeds maximum allowed SAD. To
@*   compute the distortion of the entire block set u4_max_sad to USHRT_MAX.
@*
@* @param[in] pu1_src
@*  UWORD8 pointer to the source
@*
@* @param[out] pu1_dst
@*  UWORD8 pointer to the destination
@*
@* @param[in] src_strd
@*  integer source stride
@*
@* @param[in] dst_strd
@*  integer destination stride
@*
@* @param[in] u4_max_sad
@*  integer maximum allowed distortion
@*
@* @param[in] pi4_mb_distortion
@*  integer evaluated sad
@*
@* @remarks
@*
@******************************************************************************
@*
@
    .global ime_compute_sad_16x8_a9q

ime_compute_sad_16x8_a9q:

    stmfd         sp!, {r12, lr}

    @for bringing buffer2 into cache..., dummy load instructions
    @LDR      r12,[r1]

    vld1.8        {d4, d5}, [r0], r2
    vld1.8        {d6, d7}, [r1], r3
    mov           r12, #6
    vpush         {d8-d15}
    vld1.8        {d8, d9}, [r0], r2
    vabdl.u8      q0, d6, d4
    vabdl.u8      q1, d7, d5
    vld1.8        {d10, d11}, [r1], r3

loop_sad_16x8:

    vld1.8        {d4, d5}, [r0], r2
    vabal.u8      q0, d10, d8
    vabal.u8      q1, d11, d9
    vld1.8        {d6, d7}, [r1], r3
    subs          r12, #2
    vld1.8        {d8, d9}, [r0], r2
    vabal.u8      q0, d6, d4
    vabal.u8      q1, d7, d5
    vld1.8        {d10, d11}, [r1], r3

    bne           loop_sad_16x8

    vabal.u8      q0, d10, d8
    vabal.u8      q1, d11, d9

    vadd.i16      q0, q0, q1
    vadd.i16      d0, d1, d0
    vpop          {d8-d15}
    ldr           r12, [sp, #12]
    vpaddl.u16    d0, d0
    vpaddl.u32    d0, d0

    vst1.32       {d0[0]}, [r12]

    ldmfd         sp!, {r12, pc}



@**
@******************************************************************************
@*
@* @brief computes distortion (SAD) between 2 16x16 blocks with early exit
@*
@* @par   Description
@*   This functions computes SAD between 2 16x16 blocks. There is a provision
@*   for early exit if the up-to computed SAD exceeds maximum allowed SAD. To
@*   compute the distortion of the entire block set u4_max_sad to USHRT_MAX.
@*
@* @param[in] pu1_src
@*  UWORD8 pointer to the source
@*
@* @param[out] pu1_dst
@*  UWORD8 pointer to the destination
@*
@* @param[in] src_strd
@*  integer source stride
@*
@* @param[in] dst_strd
@*  integer destination stride
@*
@* @param[in] i4_max_sad
@*  integer maximum allowed distortion
@*
@* @param[in] pi4_mb_distortion
@*  integer evaluated sad
@*
@* @remarks
@*
@******************************************************************************
@*

    .global ime_compute_sad_16x16_ea8_a9q

ime_compute_sad_16x16_ea8_a9q:

    stmfd         sp!, {r5-r7, lr}
    lsl           r2, r2, #1
    lsl           r3, r3, #1

    @for bringing buffer2 into cache..., dummy load instructions
    @LDR         r12,[r1]

    vld1.8        {d4, d5}, [r0], r2
    vld1.8        {d6, d7}, [r1], r3
    mov           r5, #6
    ldrd          r6, r7, [sp, #16]
    vpush         {d8-d15}
    vld1.8        {d8, d9}, [r0], r2
    vabdl.u8      q0, d6, d4
    vabdl.u8      q1, d7, d5
    vld1.8        {d10, d11}, [r1], r3

    @r6 = i4_max_sad, r7 = pi4_mb_distortion

loop_sad_16x16_ea8_1:

    vld1.8        {d4, d5}, [r0], r2
    vabal.u8      q0, d10, d8
    vabal.u8      q1, d11, d9
    vld1.8        {d6, d7}, [r1], r3
    subs          r5, #2
    vld1.8        {d8, d9}, [r0], r2
    vabal.u8      q0, d6, d4
    vabal.u8      q1, d7, d5
    vld1.8        {d10, d11}, [r1], r3

    bne           loop_sad_16x16_ea8_1

    vabal.u8      q0, d10, d8
    sub           r0, r0, r2, lsl #3
    vabal.u8      q1, d11, d9
    sub           r1, r1, r3, lsl #3

    vadd.i16      q6, q0, q1
    add           r0, r0, r2, asr #1
    vadd.i16      d12, d12, d13
    add           r1, r1, r3, asr #1

    vpaddl.u16    d12, d12
    vld1.8        {d4, d5}, [r0], r2
    vld1.8        {d6, d7}, [r1], r3
    vpaddl.u32    d12, d12
    vld1.8        {d8, d9}, [r0], r2
    vabal.u8      q0, d6, d4
    vabal.u8      q1, d7, d5

    vst1.32       {d12[0]}, [r7]
    ldr           r5, [r7]
    cmp           r5, r6
    bgt           end_func_16x16_ea8

    vld1.8        {d10, d11}, [r1], r3
    mov           r5, #6

loop_sad_16x16_ea8_2:

    vld1.8        {d4, d5}, [r0], r2
    vabal.u8      q0, d10, d8
    vabal.u8      q1, d11, d9
    vld1.8        {d6, d7}, [r1], r3
    subs          r5, #2
    vld1.8        {d8, d9}, [r0], r2
    vabal.u8      q0, d6, d4
    vabal.u8      q1, d7, d5
    vld1.8        {d10, d11}, [r1], r3

    bne           loop_sad_16x16_ea8_2

    vabal.u8      q0, d10, d8
    vabal.u8      q1, d11, d9

    vadd.i16      q0, q0, q1
    vadd.i16      d0, d1, d0

    vpaddl.u16    d0, d0
    vpaddl.u32    d0, d0

    vst1.32       {d0[0]}, [r7]

end_func_16x16_ea8:
    vpop          {d8-d15}
    ldmfd         sp!, {r5-r7, pc}



@*
@//---------------------------------------------------------------------------
@// Function Name      : Calculate_Mad2_prog()
@//
@// Detail Description : This function find the sad values of 4 Progressive MBs
@//                        at one shot
@//
@// Platform           : CortexA8/NEON            .
@//
@//-----------------------------------------------------------------------------
@*

    .global ime_calculate_sad2_prog_a9q

ime_calculate_sad2_prog_a9q:

    @ r0    = ref1     <UWORD8 *>
    @ r1    = ref2     <UWORD8 *>
    @ r2    = src     <UWORD8 *>
    @ r3    = RefBufferWidth <UWORD32>
    @ stack = CurBufferWidth <UWORD32>, psad <UWORD32 *>

    stmfd         sp!, {r4-r5, lr}

    ldr           r4, [sp, #8]          @ load src stride to r4
    mov           r5, #14
    vpush         {d8-d15}
    @Row 1
    vld1.8        {d0, d1}, [r2], r4    @ load src Row 1
    vld1.8        {d2, d3}, [r0], r3    @ load ref1 Row 1
    vld1.8        {d4, d5}, [r1], r3    @ load ref2 Row 1

    @Row 2
    vld1.8        {d6, d7}, [r2], r4    @ load src Row 2
    vabdl.u8      q6, d2, d0
    vabdl.u8      q7, d3, d1
    vld1.8        {d8, d9}, [r0], r3    @ load ref1 Row 2
    vabdl.u8      q8, d4, d0
    vabdl.u8      q9, d5, d1
    vld1.8        {d10, d11}, [r1], r3  @ load ref2 Row 2

loop_sad2_prog:

    subs          r5, #2
    @Row 1
    vld1.8        {d0, d1}, [r2], r4    @ load src Row 1
    vabal.u8      q6, d8, d6
    vabal.u8      q7, d9, d7
    vld1.8        {d2, d3}, [r0], r3    @ load ref1 Row 1
    vabal.u8      q8, d10, d6
    vabal.u8      q9, d11, d7
    vld1.8        {d4, d5}, [r1], r3    @ load ref2 Row 1

    @Row 2
    vld1.8        {d6, d7}, [r2], r4    @ load src Row 2
    vabal.u8      q6, d2, d0
    vabal.u8      q7, d3, d1
    vld1.8        {d8, d9}, [r0], r3    @ load ref1 Row 2
    vabal.u8      q8, d4, d0
    vabal.u8      q9, d5, d1
    vld1.8        {d10, d11}, [r1], r3  @ load ref2 Row 2

    bne           loop_sad2_prog

    vabal.u8      q6, d8, d6
    vabal.u8      q7, d9, d7
    vabal.u8      q8, d10, d6
    vabal.u8      q9, d11, d7

    @ Compute SAD

    vadd.u16      q6, q6, q7            @ Q6  : sad_ref1
    vadd.u16      q8, q8, q9            @ Q8  : sad_ref2

    vadd.u16      d12, d12, d13
    ldr           r5, [sp, #16]         @ loading pi4_sad to r5
    vadd.u16      d16, d16, d17

    vpadd.u16     d12, d12, d16
    vpaddl.u16    d12, d12

    vst1.64       {d12}, [r5]!
    vpop          {d8-d15}
    ldmfd         sp!, {r4-r5, pc}



@*
@//---------------------------------------------------------------------------
@// Function Name      : Calculate_Mad3_prog()
@//
@// Detail Description : This function find the sad values of 4 Progressive MBs
@//                        at one shot
@//
@// Platform           : CortexA8/NEON            .
@//
@//-----------------------------------------------------------------------------
@*

    .global ime_calculate_sad3_prog_a9q

ime_calculate_sad3_prog_a9q:

    @ r0    = ref1     <UWORD8 *>
    @ r1    = ref2     <UWORD8 *>
    @ r2    = ref3     <UWORD8 *>
    @ r3    = src      <UWORD8 *>
    @ stack = RefBufferWidth <UWORD32>, CurBufferWidth <UWORD32>, psad <UWORD32 *>


    stmfd         sp!, {r4-r6, lr}

    ldrd          r4, r5, [sp, #16]     @ load ref stride to r4, src stride to r5
    mov           r6, #14
    vpush         {d8-d15}
    @Row 1
    vld1.8        {d0, d1}, [r3], r5    @ load src Row 1
    vld1.8        {d2, d3}, [r0], r4    @ load ref1 Row 1
    vld1.8        {d4, d5}, [r1], r4    @ load ref2 Row 1
    vabdl.u8      q8, d2, d0
    vabdl.u8      q9, d3, d1
    vld1.8        {d6, d7}, [r2], r4    @ load ref3 Row 1
    vabdl.u8      q10, d4, d0
    vabdl.u8      q11, d5, d1

    @Row 2
    vld1.8        {d8, d9}, [r3], r5    @ load src Row 1
    vabdl.u8      q12, d6, d0
    vabdl.u8      q13, d7, d1
    vld1.8        {d10, d11}, [r0], r4  @ load ref1 Row 1
    vld1.8        {d12, d13}, [r1], r4  @ load ref2 Row 1
    vabal.u8      q8, d10, d8
    vabal.u8      q9, d11, d9
    vld1.8        {d14, d15}, [r2], r4  @ load ref3 Row 1
    vabal.u8      q10, d12, d8
    vabal.u8      q11, d13, d9

loop_sad3_prog:

    @Row 1
    vld1.8        {d0, d1}, [r3], r5    @ load src Row 1
    vabal.u8      q12, d14, d8
    vabal.u8      q13, d15, d9
    vld1.8        {d2, d3}, [r0], r4    @ load ref1 Row 1
    vld1.8        {d4, d5}, [r1], r4    @ load ref2 Row 1
    vabal.u8      q8, d2, d0
    vabal.u8      q9, d3, d1
    vld1.8        {d6, d7}, [r2], r4    @ load ref3 Row 1
    vabal.u8      q10, d4, d0
    vabal.u8      q11, d5, d1

    @Row 2
    vld1.8        {d8, d9}, [r3], r5    @ load src Row 1
    vabal.u8      q12, d6, d0
    vabal.u8      q13, d7, d1
    vld1.8        {d10, d11}, [r0], r4  @ load ref1 Row 1
    subs          r6, #2
    vld1.8        {d12, d13}, [r1], r4  @ load ref2 Row 1
    vabal.u8      q8, d10, d8
    vabal.u8      q9, d11, d9
    vld1.8        {d14, d15}, [r2], r4  @ load ref3 Row 1
    vabal.u8      q10, d12, d8
    vabal.u8      q11, d13, d9

    bne           loop_sad3_prog

    vabal.u8      q12, d14, d8
    vabal.u8      q13, d15, d9

    @ Compute SAD

    vadd.u16      q8, q8, q9            @ Q8  : sad_ref1
    vadd.u16      q10, q10, q11         @ Q10 : sad_ref2
    vadd.u16      q12, q12, q13         @ Q12 : sad_ref3

    vadd.u16      d16, d16, d17
    vadd.u16      d20, d20, d21
    vadd.u16      d24, d24, d25

    vpadd.u16     d16, d16, d20
    vpadd.u16     d24, d24, d24

    ldr           r6, [sp, #24]         @ loading pi4_sad to r6
    vpaddl.u16    d16, d16
    vpaddl.u16    d24, d24

    vst1.64       {d16}, [r6]!
    vst1.32       {d24[0]}, [r6]
    vpop          {d8-d15}
    ldmfd         sp!, {r4-r6, pc}



@**
@******************************************************************************
@*
@* @brief computes distortion (SAD) for sub-pel motion estimation
@*
@* @par   Description
@*   This functions computes SAD for all the 8 half pel points
@*
@* @param[out] pi4_sad
@*  integer evaluated sad
@*  pi4_sad[0] - half x
@*  pi4_sad[1] - half x - 1
@*  pi4_sad[2] - half y
@*  pi4_sad[3] - half y - 1
@*  pi4_sad[4] - half xy
@*  pi4_sad[5] - half xy - 1
@*  pi4_sad[6] - half xy - strd
@*  pi4_sad[7] - half xy - 1 - strd
@*
@* @remarks
@*
@******************************************************************************
@*

.text
.p2align 2

    .global ime_sub_pel_compute_sad_16x16_a9q

ime_sub_pel_compute_sad_16x16_a9q:

    stmfd         sp!, {r4-r11, lr}     @store register values to stack

    ldr           r9, [sp, #36]
    ldr           r10, [sp, #40]
    vpush         {d8-d15}
    sub           r4, r1, #1            @ x left
    sub           r5, r2, r10           @ y top

    sub           r6, r3, #1            @ xy left
    sub           r7, r3, r10           @ xy top

    sub           r8, r7, #1            @ xy top-left
    mov           r11, #15

    @for bringing buffer2 into cache..., dummy load instructions
    @ LDR         r12,[r1]
    @ LDR         r12,[sp,#12]

    vld1.8        {d0, d1}, [r0], r9    @ src
    vld1.8        {d2, d3}, [r5], r10   @ y top LOAD
    vld1.8        {d4, d5}, [r7], r10   @ xy top LOAD
    vld1.8        {d6, d7}, [r8], r10   @ xy top-left LOAD

    vabdl.u8      q6, d2, d0            @ y top ABS1
    vabdl.u8      q7, d4, d0            @ xy top ABS1
    vld1.8        {d8, d9}, [r1], r10   @ x LOAD
    vabdl.u8      q8, d6, d0            @ xy top-left ABS1
    vabdl.u8      q9, d8, d0            @ x ABS1
    vld1.8        {d10, d11}, [r4], r10 @ x left LOAD

    vabal.u8      q6, d3, d1            @ y top ABS2
    vabal.u8      q7, d5, d1            @ xy top ABS2
    vld1.8        {d2, d3}, [r2], r10   @ y LOAD
    vabal.u8      q8, d7, d1            @ xy top-left ABS2
    vabal.u8      q9, d9, d1            @ x ABS2
    vld1.8        {d4, d5}, [r3], r10   @ xy LOAD

    vabdl.u8      q10, d10, d0          @ x left ABS1
    vabdl.u8      q11, d2, d0           @ y ABS1
    vld1.8        {d6, d7}, [r6], r10   @ xy left LOAD
    vabdl.u8      q12, d4, d0           @ xy ABS1
    vabdl.u8      q13, d6, d0           @ xy left ABS1

loop_sub_pel_16x16:

    vabal.u8      q10, d11, d1          @ x left ABS2
    vabal.u8      q11, d3, d1           @ y ABS2
    subs          r11, #1
    vabal.u8      q12, d5, d1           @ xy ABS2
    vabal.u8      q13, d7, d1           @ xy left ABS2

    vld1.8        {d0, d1}, [r0], r9    @ src
    vabal.u8      q6, d2, d0            @ y top ABS1
    vabal.u8      q7, d4, d0            @ xy top ABS1
    vld1.8        {d8, d9}, [r1], r10   @ x LOAD
    vabal.u8      q8, d6, d0            @ xy top-left ABS1
    vabal.u8      q9, d8, d0            @ x ABS1
    vld1.8        {d10, d11}, [r4], r10 @ x left LOAD

    vabal.u8      q6, d3, d1            @ y top ABS2
    vabal.u8      q7, d5, d1            @ xy top ABS2
    vld1.8        {d2, d3}, [r2], r10   @ y LOAD
    vabal.u8      q8, d7, d1            @ xy top-left ABS2
    vabal.u8      q9, d9, d1            @ x ABS2
    vld1.8        {d4, d5}, [r3], r10   @ xy LOAD

    vabal.u8      q10, d10, d0          @ x left ABS1
    vabal.u8      q11, d2, d0           @ y ABS1
    vld1.8        {d6, d7}, [r6], r10   @ xy left LOAD
    vabal.u8      q12, d4, d0           @ xy ABS1
    vabal.u8      q13, d6, d0           @ xy left ABS1

    bne           loop_sub_pel_16x16

    vabal.u8      q10, d11, d1          @ x left ABS2
    vabal.u8      q11, d3, d1           @ y ABS2
    vabal.u8      q12, d5, d1           @ xy ABS2
    vabal.u8      q13, d7, d1           @ xy left ABS2

    vadd.i16      d0, d18, d19          @ x
    vadd.i16      d3, d12, d13          @ y top
    vadd.i16      d6, d14, d15          @ xy top
    vadd.i16      d5, d26, d27          @ xy left
    vadd.i16      d1, d20, d21          @ x left
    vadd.i16      d2, d22, d23          @ y
    vadd.i16      d4, d24, d25          @ xy
    vadd.i16      d7, d16, d17          @ xy top left

    vpadd.i16     d0, d0, d1
    vpadd.i16     d2, d2, d3
    vpadd.i16     d4, d4, d5
    vpadd.i16     d6, d6, d7

    vpaddl.u16    d0, d0
    vpaddl.u16    d2, d2
    vpop          {d8-d15}
    ldr           r11, [sp, #44]
    vpaddl.u16    d4, d4
    vpaddl.u16    d6, d6

    vst1.32       {d0}, [r11]!
    vst1.32       {d2}, [r11]!
    vst1.32       {d4}, [r11]!
    vst1.32       {d6}, [r11]!

    ldmfd         sp!, {r4-r11, pc}     @Restoring registers from stack



@**
@******************************************************************************
@*
@* @brief computes distortion (SAD) between 2 16x16 blocks
@*
@* @par   Description
@*   This functions computes SAD between 2 16x16 blocks. There is a provision
@*   for early exit if the up-to computed SAD exceeds maximum allowed SAD. To
@*   compute the distortion of the entire block set u4_max_sad to USHRT_MAX.
@*
@* @param[in] pu1_src
@*  UWORD8 pointer to the source
@*
@* @param[out] pu1_dst
@*  UWORD8 pointer to the destination
@*
@* @param[in] src_strd
@*  integer source stride
@*
@* @param[in] dst_strd
@*  integer destination stride
@*
@* @param[in] i4_max_sad
@*  integer maximum allowed distortion
@*
@* @param[in] pi4_mb_distortion
@*  integer evaluated sad
@*
@* @remarks
@*
@******************************************************************************
@*

.text
.p2align 2

    .global ime_compute_sad_16x16_a9q

ime_compute_sad_16x16_a9q:


    @STMFD       sp!,{r12,lr}
    stmfd         sp!, {r12, r14}       @store register values to stack

    @for bringing buffer2 into cache..., dummy load instructions
    @ LDR         r12,[r1]
    @ LDR         r12,[sp,#12]

    vld1.8        {d4, d5}, [r0], r2
    vld1.8        {d6, d7}, [r1], r3
    vpush         {d8-d15}
    mov           r12, #14
    vld1.8        {d8, d9}, [r0], r2
    vabdl.u8      q0, d4, d6
    vld1.8        {d10, d11}, [r1], r3
    vabdl.u8      q1, d5, d7

loop_sad_16x16:

    vld1.8        {d4, d5}, [r0], r2
    vabal.u8      q0, d8, d10
    vld1.8        {d6, d7}, [r1], r3
    vabal.u8      q1, d9, d11

    vld1.8        {d8, d9}, [r0], r2
    vabal.u8      q0, d4, d6
    subs          r12, #2
    vld1.8        {d10, d11}, [r1], r3
    vabal.u8      q1, d5, d7

    bne           loop_sad_16x16

    vabal.u8      q0, d8, d10
    vabal.u8      q1, d9, d11

    vadd.i16      q0, q0, q1
    vadd.i16      d0, d1, d0
    vpop          {d8-d15}
    ldr           r12, [sp, #12]

    vpaddl.u16    d0, d0
    vpaddl.u32    d0, d0
    vst1.32       {d0[0]}, [r12]

    ldmfd         sp!, {r12, pc}        @Restoring registers from stack


@*
@//---------------------------------------------------------------------------
@// Function Name      : Calculate_Mad4_prog()
@//
@// Detail Description : This function find the sad values of 4 Progressive MBs
@//                        at one shot
@//
@// Platform           : CortexA8/NEON            .
@//
@//-----------------------------------------------------------------------------
@*

    .global ime_calculate_sad4_prog_a9q

ime_calculate_sad4_prog_a9q:
    @ r0    = temp_frame     <UWORD8 *>
    @ r1    = buffer_ptr     <UWORD8 *>
    @ r2    = RefBufferWidth <UWORD32>
    @ r3    = CurBufferWidth <UWORD32>
    @ stack = psad           <UWORD32 *> {at 0x34}

    stmfd         sp!, {r4-r7, lr}

    @UWORD8 *left_ptr       = temp_frame - 1;
    @UWORD8 *right_ptr      = temp_frame + 1;
    @UWORD8 *top_ptr        = temp_frame - RefBufferWidth;
    @UWORD8 *bot_ptr        = temp_frame + RefBufferWidth;

    mov           r7, #14
    sub           r4, r0, #0x01         @r4 = left_ptr
    add           r5, r0, #0x1          @r5 = right_ptr
    sub           r6, r0, r2            @r6 = top_ptr
    add           r0, r0, r2            @r0 = bot_ptr
                                        @r1 = buffer_ptr
    vpush         {d8-d15}
    @D0:D1  : buffer
    @D2:D3  : top
    @D4:D5  : left
    @D6:D7  : right
    @D8:D9  : bottom

    @Row 1
    vld1.8        {d0, d1}, [r1], r3    @ load src Row 1
    vld1.8        {d2, d3}, [r6], r2    @ load top Row 1
    vld1.8        {d4, d5}, [r4], r2    @ load left Row 1

    vabdl.u8      q5, d2, d0
    vld1.8        {d6, d7}, [r5], r2    @ load right Row 1
    vabdl.u8      q6, d3, d1

    vabdl.u8      q7, d0, d4
    vld1.8        {d8, d9}, [r0], r2    @ load bottom Row 1
    vabdl.u8      q8, d1, d5

    @Row 2
    vabdl.u8      q9, d0, d6
    vld1.8        {d26, d27}, [r1], r3  @ load src Row 2
    vabdl.u8      q10, d1, d7

    vabdl.u8      q11, d0, d8
    vld1.8        {d2, d3}, [r6], r2    @ load top Row 2
    vabdl.u8      q12, d1, d9

loop_sad4_prog:

    vabal.u8      q5, d26, d2
    vld1.8        {d4, d5}, [r4], r2    @ load left Row 2
    vabal.u8      q6, d27, d3

    vabal.u8      q7, d26, d4
    vld1.8        {d6, d7}, [r5], r2    @ load right Row 2
    vabal.u8      q8, d27, d5

    vabal.u8      q9, d26, d6
    vld1.8        {d8, d9}, [r0], r2    @ load bottom Row 2
    vabal.u8      q10, d27, d7

    @Row 1
    vabal.u8      q11, d26, d8
    vld1.8        {d0, d1}, [r1], r3    @ load src Row 1
    vabal.u8      q12, d27, d9

    vld1.8        {d2, d3}, [r6], r2    @ load top Row 1
    subs          r7, #2
    vld1.8        {d4, d5}, [r4], r2    @ load left Row 1

    vabal.u8      q5, d0, d2
    vld1.8        {d6, d7}, [r5], r2    @ load right Row 1
    vabal.u8      q6, d1, d3

    vabal.u8      q7, d0, d4
    vld1.8        {d8, d9}, [r0], r2    @ load bottom Row 1
    vabal.u8      q8, d1, d5

    @Row 2
    vabal.u8      q9, d0, d6
    vld1.8        {d26, d27}, [r1], r3  @ load src Row 2
    vabal.u8      q10, d1, d7

    vabal.u8      q11, d0, d8
    vld1.8        {d2, d3}, [r6], r2    @ load top Row 2
    vabal.u8      q12, d1, d9

    bne           loop_sad4_prog

    vabal.u8      q5, d26, d2
    vld1.8        {d4, d5}, [r4], r2    @ load left Row 2
    vabal.u8      q6, d27, d3

    vabal.u8      q7, d26, d4
    vld1.8        {d6, d7}, [r5], r2    @ load right Row 2
    vabal.u8      q8, d27, d5

    vabal.u8      q9, d26, d6
    vld1.8        {d8, d9}, [r0], r2    @ load bottom Row 2
    vabal.u8      q10, d27, d7

    vabal.u8      q11, d26, d8
    vabal.u8      q12, d27, d9

    @;Q5:Q6   : sad_top
    @;Q7:Q8   : sad_left
    @;Q9:Q10  : sad_right
    @;Q11:Q12 : sad_bot

    vadd.u16      q5, q5, q6
    vadd.u16      q7, q7, q8
    vadd.u16      q9, q9, q10
    vadd.u16      q11, q11, q12

    @; Free :-
    @; Q6,Q8,Q10,Q12

    @;Q5  -> D10:D11
    @;Q7  -> D14:D15
    @;Q9  -> D18:D19
    @;Q11 -> D22:D23

    vadd.u16      d10, d10, d11
    vadd.u16      d14, d14, d15
    vadd.u16      d18, d18, d19
    vadd.u16      d22, d22, d23

    @;D10  : sad_top
    @;D14  : sad_left
    @;D18  : sad_right
    @;D22  : sad_bot


    vpaddl.u16    d11, d10
    vpaddl.u16    d15, d14
    vpaddl.u16    d19, d18
    vpaddl.u16    d23, d22

    @;D11  : sad_top
    @;D15  : sad_left
    @;D19  : sad_right
    @;D23  : sad_bot

    vpaddl.u32    d10, d11
    vpaddl.u32    d22, d23
    vpaddl.u32    d14, d15
    vpaddl.u32    d18, d19

    @;D10  : sad_top
    @;D14  : sad_left
    @;D18  : sad_right
    @;D22  : sad_bot

    ldr           r4, [sp, #84]         @;Can be rearranged

    vsli.64       d10, d22, #32
    vsli.64       d14, d18, #32

    vst1.64       {d14}, [r4]!
    vst1.64       {d10}, [r4]!
    vpop          {d8-d15}
    ldmfd         sp!, {r4-r7, pc}




@*****************************************************************************
@*
@* Function Name        : ime_compute_satqd_16x16_lumainter_a9
@* Description          : This fucntion computes SAD for a 16x16 block.
@                       : It also computes if any 4x4 block will have a nonzero coefficent after transform and quant
@
@  Arguments            :   R0 :pointer to src buffer
@                           R1 :pointer to est buffer
@                           R2 :source stride
@                           R3 :est stride
@                           STACk :Threshold,distotion,is_nonzero
@*
@* Values Returned   : NONE
@*
@* Register Usage    : R0-R11
@* Stack Usage       :
@* Cycles            : Around
@* Interruptiaility  : Interruptable
@*
@* Known Limitations
@*   \Assumptions    :
@*
@* Revision History  :
@*         DD MM YYYY    Author(s)          Changes
@*         14 04 2014    Harinarayanan K K  First version
@*
@*****************************************************************************
    .global ime_compute_satqd_16x16_lumainter_a9q
ime_compute_satqd_16x16_lumainter_a9q:
    @R0 :pointer to src buffer
    @R1 :pointer to est buffer
    @R2 :Source stride
    @R3 :Pred stride
    @R4 :Threshold pointer
    @R5 :Distortion,ie SAD
    @R6 :is nonzero

    push          {r4-r12, lr}          @push all the variables first
    @ADD      SP,SP,#40         ;decrement stack pointer,to accomodate two variables
    ldr           r4, [sp, #40]         @load the threshold address
    vpush         {d8-d15}
    mov           r8, #8                @Number of 4x8 blocks to be processed
    mov           r10, #0               @Sad
    mov           r7, #0                @Nonzero info
    @----------------------------------------------------

    vld1.u8       d30, [r0], r2         @I  load 8 pix src row 1

    vld1.u8       d31, [r1], r3         @I  load 8 pix pred row 1

    vld1.u8       d28, [r0], r2         @I  load 8 pix src row 2

    vld1.u8       d29, [r1], r3         @I  load 8 pix pred row 2

    vld1.u8       d26, [r0], r2         @I  load 8 pix src row 3
    vabdl.u8      q0, d30, d31          @I  Abs diff r1 blk 12

    vld1.u8       d27, [r1], r3         @I  load 8 pix pred row 3

    vld1.u8       d24, [r0], r2         @I  load 8 pix src row 4

    vld1.u8       d25, [r1], r3         @I  load 8 pix pred row 4
    vabdl.u8      q1, d28, d29          @I  Abs diff r1 blk 12

    vld1.u16      {q11}, [r4]           @I  load the threhold
    vabdl.u8      q2, d26, d27          @I  Abs diff r1 blk 12

    vabdl.u8      q3, d24, d25          @I  Abs diff r1 blk 12



core_loop:
                                        @S1  S2  S3  S4     A1  A2  A3  A4
                                        @S5  S6  S7  S8     A5  A6  A7  A8
                                        @S9  S10 S11 S12    A9  A10 A11 A12
                                        @S13 S14 S15 S16    A13 A14 A15 A16
    ands          r11, r8, #1           @II See if we are at even or odd block
    vadd.u16      q4 , q0, q3           @I  Add r1 r4
    lsl           r11, r2, #2           @II Move back src 4 rows

    subeq         r0, r0, r11           @II Move back src 4 rows if we are at even block
    vadd.u16      q5 , q1, q2           @I  Add r2 r3
    addeq         r0, r0, #8            @II Move src 8 cols forward if we are at even block

    lsl           r11, r3, #2           @II Move back pred 4 rows
    vtrn.16       d8 , d10              @I trnspse 1
    subeq         r1, r1, r11           @II Move back pred 4 rows if we are at even block

    addeq         r1, r1, #8            @II Move pred 8 cols forward if we are at even block
    vtrn.16       d9 , d11              @I trnspse 2
    subne         r0, r0, #8            @II Src 8clos back for odd rows

    subne         r1, r1, #8            @II Pred 8 cols back for odd rows
    vtrn.32       d10, d11              @I trnspse 4


    vtrn.32       d8 , d9               @I trnspse 3
    vswp          d10, d11              @I rearrange so that the q4 and q5 add properly
                                        @D8     S1 S4 A1 A4
                                        @D9     S2 S3 A2 A3
                                        @D11    S1 S4 A1 A4
                                        @D10    S2 S3 A2 A3

    vadd.s16      q6, q4, q5            @I  Get s1 s4
    vld1.u8       d30, [r0], r2         @II load first 8 pix src row 1

    vtrn.s16      d12, d13              @I  Get s2 s3
                                        @D12 S1 S4 A1 A4
                                        @D13 S2 S3 A2 A3

    vshl.s16      q7, q6 , #1           @I  si  = si<<1
    vld1.u8       d31, [r1], r3         @II load first 8 pix pred row 1

    vpadd.s16     d16, d12, d13         @I  (s1 + s4) (s2 + s3)
    vld1.u8       d28, [r0], r2         @II load first 8 pix src row 2
                                        @   D16  S14 A14 S23 A23
    vrev32.16     d0, d16               @I
    vuzp.s16      d16, d0               @I
                                        @D16  S14 S23 A14 A23
    vadd.s16      d17, d12, d13         @I  (s1 + s2) (s3 + s4)
    vld1.u8       d29, [r1], r3         @II load first 8 pix pred row 2
                                        @D17  S12 S34 A12 A34

    vrev32.16     q9, q7                @I  Rearrange si's
                                        @Q9  Z4,Z1,Y4,Y1,Z3,Z2,Y3,Y2

                                        @D12    S1 S4 A1 A4
                                        @D19    Z3 Z2 Y3 Y2
    vsub.s16      d8, d12, d19          @I  (s1 - (s3<<1)) (s4 - (s2<<1))
    vld1.u8       d26, [r0], r2         @II load first 8 pix src row 3
                                        @D13    S2 S3 A2 A3
                                        @D18    Z4 Z1 Y4 Y1
    vsub.s16      d9, d13, d18          @I  (s2 - (s4<<1)) (s3 - (s1<<1))
    vld1.u8       d27, [r1], r3         @II load first 8 pix pred row 3
                                        @Q10    S8 S5 A8 A5 S7 S4 A7 A4

                                        @D16  S14 S23 A14 A23
    vpadd.s16     d10, d16, d17         @I  Get sad by adding s1 s2 s3 s4
    vld1.u8       d24, [r0], r2         @II load first 8 pix src row 4
                                        @D22 SAD1 SAD2 junk junk


                                        @Q8     S2 S1 A2 A1 S6 S3 A6 A3
                                        @Q10    S8 S5 A8 A5 S7 S4 A7 A4
    vtrn.32       q8, q4                @I  Rearrange to make ls of each block togather
                                        @Q8     S2 S1 S8 S5 S6 S3 S7 S4
                                        @Q10    A2 A1 A8 A5 A6 A3 A7 A4


    ldrh          r11, [r4, #16]        @I  Load the threshold for DC val blk 1
    vdup.s16      q6, d10[0]            @I  Get the sad blk 1
    vabdl.u8      q0, d30, d31          @II Abs diff r1 blk 12

    vshl.s16      q7, q6, #1            @I  sad_2 = sad_1<<1
    vmov.s16      r9, d10[0]            @I  Get the sad for block 1

    vsub.s16      q9, q7, q8            @I  Add to the lss
    vmov.s16      r5, d10[1]            @I  Get the sad for block 2

    vcle.s16      q7, q11, q9           @I  Add to the lss
    vld1.u8       d25, [r1], r3         @II load first 8 pix pred row 4

    vdup.s16      q15, d10[1]           @I  Get the sad blk 1
    vabdl.u8      q1, d28, d29          @II Abs diff r1 blk 12


    vshl.s16      q14, q15, #1          @I  sad_2 = sad_1<<1
    vsub.s16      q3, q14, q4           @I  Add to the lss
    vcle.s16      q15, q11, q3          @I  Add to the lss

    ADD           R10, R10, R9          @I  Add to  the global sad blk 1
    vtrn.u8       q15, q7               @I  get all comparison bits to one reg
    vabdl.u8      q2, d26, d27          @II Abs diff r1 blk 12

    ADD           R10, R10, R5          @I  Add to  the global sad blk 2
    vshr.u8       q14, q15, #7          @I  Shift the bits so that no  overflow occurs
    cmp           r11, r9

    movle         r7, #0xf              @I  If not met mark it by mvoing non zero val to R7 blk 1                   ;I  Compare with threshold blk 1
    vadd.u8       d28, d28, d29         @I  Add the bits
    cmp           r11, r5               @I  Compare with threshold blk 2

    movle         r7, #0xf              @I  If not met mark it by mvoing non zero val to R7 blk 2
    vpadd.u8      d28, d28, d29         @I  Add the bits

    vmov.u32      r11, d28[0]           @I  Since a set bit now represents a unstatisofrd contifon store it in r11
    vabdl.u8      q3, d24, d25          @II Abs diff r1 blk 12

    orr           r7, r7, r11           @I  get the guy to r11


    sub           r8, r8, #1            @I  Decremrnt block count

    cmp           r7, #0                @I  If we have atlest one non zero block
    bne           compute_sad_only      @I  if a non zero block is der,From now on compute sad only

    cmp           r8, #1                @I  See if we are at the last block
    bne           core_loop             @I  If the blocks are zero, lets continue the satdq


    @EPILOUGE for core loop
                                        @S1  S2  S3  S4     A1  A2  A3  A4
                                        @S5  S6  S7  S8     A5  A6  A7  A8
                                        @S9  S10 S11 S12    A9  A10 A11 A12
                                        @S13 S14 S15 S16    A13 A14 A15 A16
    vadd.u16      q4 , q0, q3           @Add r1 r4
    vadd.u16      q5 , q1, q2           @Add r2 r3
                                        @D8     S1 S2 S2 S1
                                        @D10    S4 S3 S3 S4
                                        @D9     A1 A2 A2 A1
                                        @D11    A4 A3 A3 A4
    vtrn.16       d8 , d10              @I trnspse 1
    vtrn.16       d9 , d11              @I trnspse 2
    vtrn.32       d8 , d9               @I trnspse 3
    vtrn.32       d10, d11              @I trnspse 4

    vswp          d10, d11              @I rearrange so that the q4 and q5 add properly
                                        @D8     S1 S4 A1 A4
                                        @D9     S2 S3 A2 A3
                                        @D11    S1 S4 A1 A4
                                        @D10    S2 S3 A2 A3
    vadd.s16      q6, q4, q5            @Get s1 s4
    vtrn.s16      d12, d13              @Get s2 s3
                                        @D12 S1 S4 A1 A4
                                        @D13 S2 S3 A2 A3

    vshl.s16      q7, q6 , #1           @si  = si<<1
    vmov.s16      r9, d10[0]            @Get the sad for block 1

    vpadd.s16     d16, d12, d13         @(s1 + s4) (s2 + s3)
    vmov.s16      r5, d10[1]            @Get the sad for block 2
                                        @D16  S14 A14 S23 A23
    vrev32.16     d30, d16              @
    vuzp.s16      d16, d30              @
                                        @D16  S14 S23 A14 A23
    vadd.s16      d17, d12, d13         @(s1 + s2) (s3 + s4)
                                        @D17  S12 S34 A12 A34

    vrev32.16     q9, q7                @Rearrange si's
                                        @Q9  Z4,Z1,Y4,Y1,Z3,Z2,Y3,Y2

                                        @D12    S1 S4 A1 A4
                                        @D19    Z3 Z2 Y3 Y2
    vsub.s16      d8, d12, d19          @(s1 - (s3<<1)) (s4 - (s2<<1))
                                        @D13    S2 S3 A2 A3
                                        @D18    Z4 Z1 Y4 Y1
    vsub.s16      d9, d13, d18          @(s2 - (s4<<1)) (s3 - (s1<<1))
                                        @Q10    S8 S5 A8 A5 S7 S4 A7 A4

                                        @D16  S14 S23 A14 A23
    vpadd.s16     d10, d16, d17         @I  Get sad by adding s1 s2 s3 s4
                                        @D22 SAD1 SAD2 junk junk
    vmov.u16      r9, d10[0]            @Get the sad for block 1
    vmov.u16      r5, d10[1]            @Get the sad for block 2

                                        @Q8     S2 S1 A2 A1 S6 S3 A6 A3
                                        @Q10    S8 S5 A8 A5 S7 S4 A7 A4
    ldrh          r11, [r4, #16]        @Load the threshold for DC val blk 1
    vtrn.32       q8, q4                @Rearrange to make ls of each block togather
    ADD           R10, R10, R9          @Add to  the global sad blk 1

                                        @Q8     S2 S1 S8 S5 S6 S3 S7 S4
                                        @Q10    A2 A1 A8 A5 A6 A3 A7 A4

    vld1.u16      {q11}, [r4]           @load the threhold
    ADD           R10, R10, R5          @Add to  the global sad blk 2

    vdup.u16      q6, d10[0]            @Get the sad blk 1

    cmp           r11, r9               @Compare with threshold blk 1
    vshl.u16      q7, q6, #1            @sad_2 = sad_1<<1

    vsub.s16      q9, q7, q8            @Add to the lss

    vcle.s16      q15, q11, q9          @Add to the lss
    movle         r7, #0xf              @If not met mark it by mvoing non zero val to R7 blk 1

    cmp           r11, r5               @Compare with threshold blk 2
    vdup.u16      q14, d10[1]           @Get the sad blk 1

    vshl.u16      q13, q14, #1          @sad_2 = sad_1<<1
    vsub.s16      q12, q13, q4          @Add to the lss
    vcle.s16      q14, q11, q12         @Add to the lss
    movle         r7, #0xf              @If not met mark it by mvoing non zero val to R7 blk 2

    vtrn.u8       q14, q15              @get all comparison bits to one reg
    vshr.u8       q14, q14, #7          @Shift the bits so that no  overflow occurs
    vadd.u8       d28, d28, d29         @Add the bits
    vpadd.u8      d28, d28, d29         @Add the bits
    vmov.u32      r11, d28[0]           @Since a set bit now represents a unstatisofrd contifon store it in r11
    orr           r7, r7, r11           @get the guy to r11

    b             funcend_sad_16x16     @Since all blocks ar processed nw, got to end

compute_sad_only:                       @This block computes SAD only, so will be lighter
                                        @IT will start processign at n odd block
                                        @It will compute sad for odd blok,
                                        @and then for two blocks at a time
                                        @The counter is r7, hence r7 blocks will be processed

    and           r11, r8, #1           @Get the last bit of counter
    cmp           r11, #0               @See if we are at even or odd block
                                        @iif the blk is even we just have to set the pointer to the
                                        @start of current row

    lsleq         r11, r2, #2           @I  Move back src 4 rows
    subeq         r0, r0, r11           @I  Move back src 4 rows if we are at even block

    lsleq         r11, r3, #2           @I  Move back pred 4 rows
    subeq         r1, r1, r11           @I  Move back pred 4 rows if we are at even block
    @ADDEQ R8,R8,#2         ;Inc counter
    beq           skip_odd_blk          @If the blk is odd we have to compute sad


    vadd.u16      q4, q0, q1            @Add SAD of row1 and row2
    vadd.u16      q5, q2, q3            @Add SAD of row3 and row4
    vadd.u16      q6, q4, q5            @Add SAD of row 1-4
    vadd.u16      d14, d12, d13         @Add Blk1 and blk2
    vpadd.u16     d16, d14, d15         @Add col 1-2 and 3-4
    vpadd.u16     d18, d16, d17         @Add col 12-34

    vmov.u16      r9, d18[0]            @Move sad to arm
    ADD           R10, R10, R9          @Add to  the global sad

    sub           r8, r8, #1            @Dec counter
    cmp           r8, #0                @See if we processed last block
    beq           funcend_sad_16x16     @if lprocessed last block goto end of func

    sub           r0, r0, #8            @Since we processed od block move back src by 8 cols
    sub           r1, r1, #8            @Since we processed od block move back pred by 8 cols

skip_odd_blk:

    vmov.s16      q0, #0                @Initialize the accumulator
    vmov.s16      q1, #0                @Initialize the accumulator

    vld1.u8       {q15}, [r0], r2       @load src r1
    vld1.u8       {q14}, [r1], r3       @load pred r1

    vld1.u8       {q13}, [r0], r2       @load src r2
    vld1.u8       {q12}, [r1], r3       @load pred r2

    vld1.u8       {q11}, [r0], r2       @load src r3
    vld1.u8       {q10}, [r1], r3       @load pred r2

    vld1.u8       {q9}, [r0], r2        @load src r4
    vld1.u8       {q8}, [r1], r3        @load pred r4

    cmp           r8, #2
    beq           sad_epilouge

sad_loop:

    vabal.u8      q0, d30, d28          @I  accumulate Abs diff R1
    vabal.u8      q1, d31, d29          @I  accumulate Abs diff R1

    vld1.u8       {q15}, [r0], r2       @II load r1 src
    vabal.u8      q0, d26, d24          @I  accumulate Abs diff R2

    vld1.u8       {q14}, [r1], r3       @II load r1 pred
    vabal.u8      q1, d27, d25          @I  accumulate Abs diff R2

    vld1.u8       {q13}, [r0], r2       @II load r3 src
    vabal.u8      q0, d22, d20          @I  accumulate Abs diff R3

    vld1.u8       {q12}, [r1], r3       @II load r2 pred
    vabal.u8      q1, d23, d21          @I  accumulate Abs diff R3

    vld1.u8       {q11}, [r0], r2       @II load r3 src
    vabal.u8      q0, d18, d16          @I  accumulate Abs diff R4


    sub           r8, r8, #2            @Since we processe 16 pix @a time, dec by 2
    vld1.u8       {q10}, [r1], r3       @II load r3 pred
    vabal.u8      q1, d19, d17          @I  accumulate Abs diff R4

    cmp           r8, #2                @Check if last loop
    vld1.u8       {q9}, [r0], r2        @II load r4 src
    vld1.u8       {q8}, [r1], r3        @II load r4 pred

    bne           sad_loop              @Go back to SAD computation

sad_epilouge:
    vabal.u8      q0, d30, d28          @Accumulate Abs diff R1
    vabal.u8      q1, d31, d29          @Accumulate Abs diff R1

    vabal.u8      q0, d26, d24          @Accumulate Abs diff R2
    vabal.u8      q1, d27, d25          @Accumulate Abs diff R2

    vabal.u8      q0, d22, d20          @Accumulate Abs diff R3
    vabal.u8      q1, d23, d21          @Aaccumulate Abs diff R3

    vabal.u8      q0, d18, d16          @Accumulate Abs diff R4
    vabal.u8      q1, d19, d17          @Accumulate Abs diff R4

    vadd.u16      q2, q0, q1            @ADD two accumulators
    vadd.u16      d6, d4, d5            @Add two blk sad
    vpadd.u16     d8, d6, d7            @Add col 1-2 and 3-4 sad
    vpadd.u16     d10, d8, d9           @Add col 12-34 sad

    vmov.u16      r9, d10[0]            @move SAD to ARM
    ADD           R10, R10, R9          @Add to  the global sad

funcend_sad_16x16:                      @End of fucntion process

    vpop          {d8-d15}
    ldr           r5, [sp, #44]
    ldr           r6, [sp, #48]

    str           r7, [r6]              @Store the is zero reg
    str           r10, [r5]             @Store sad

    @SUB SP,SP,#40
    pop           {r4-r12, pc}


