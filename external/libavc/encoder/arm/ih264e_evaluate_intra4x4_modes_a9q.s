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


.data
.p2align 2

scratch_intrapred_luma_4x4_prediction:
    .long ver, hor, d_c, dia_dl
    .long dia_dr, ver_r, hor_d, ver_l
    .long hor_u


.text
.p2align 2

scratch_intrapred_luma_4x4_prediction_addr1:
    .long scratch_intrapred_luma_4x4_prediction - scrintra_4x4 - 8



@/**
@******************************************************************************
@*
@* @brief :Evaluate best intra 4x4 mode
@*                and do the prediction.
@*
@* @par Description
@*   This function evaluates  4x4 modes and compute corresponding sad
@*   and return the buffer predicted with best mode.
@*
@* @param[in] pu1_src
@*  UWORD8 pointer to the source
@*
@** @param[in] pu1_ngbr_pels
@*  UWORD8 pointer to neighbouring pels
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
@* @param[in] u4_n_avblty
@* availability of neighbouring pixels
@*
@* @param[in] u4_intra_mode
@* Pointer to the variable in which best mode is returned
@*
@* @param[in] pu4_sadmin
@* Pointer to the variable in which minimum cost is returned
@*
@* @param[in] u4_valid_intra_modes
@* Says what all modes are valid
@*
@* * @param[in] u4_lambda
@* Lamda value for computing cost from SAD
@*
@* @param[in] u4_predictd_mode
@* Predicted mode for cost computation
@*
@*
@*
@* @return      none
@*
@******************************************************************************
@*/
@void ih264e_evaluate_intra_4x4_modes(UWORD8 *pu1_src,
@                                     UWORD8 *pu1_ngbr_pels,
@                                     UWORD8 *pu1_dst,
@                                     UWORD32 src_strd,
@                                    UWORD32 dst_strd,
@                                     WORD32 u4_n_avblty,
@                                     UWORD32 *u4_intra_mode,
@                                     WORD32 *pu4_sadmin,
@                                     UWORD32 u4_valid_intra_modes,
@                                     UWORD32  u4_lambda,
@                                     UWORD32 u4_predictd_mode)



    .global ih264e_evaluate_intra_4x4_modes_a9q

ih264e_evaluate_intra_4x4_modes_a9q:

@r0 = pu1_src,
@r1 = pu1_ngbr_pels_i16,
@r2 = pu1_dst,
@r3 = src_strd,
@r4 = dst_strd,
@r5 = u4_n_avblty,
@r6 = u4_intra_mode,
@r7 = pu4_sadmin
@r8 = u4_valid_intra_modes
@r0 =u4_lambda
@r1 = u4_predictd_mode


    stmfd         sp!, {r4-r12, r14}    @store register values to stack

@--------------------
    ldr           r5, [sp, #44]         @r5 = u4_n_avblty,
@----------------------
    vpush         {d8-d15}
@Loading neighbours
    vld1.32       {q0}, [r1]
    add           r4, r1, #12
    vld1.8        d1[5], [r4]
    vld1.8        d1[7], [r1]
    @--------------------------------
    ldr           r8, [sp, #120]        @u4_valid_intra_modes
@----------------------------------------------



@ LOADING pu1_src
    vld1.32       {d20[0]}, [r0], r3
    vext.8        q1, q0, q0, #1
    vld1.32       {d20[1]}, [r0], r3
    mov           r11, #1
    vld1.32       {d21[0]}, [r0], r3
    lsl           r11, r11, #30
    vld1.32       {d21[1]}, [r0], r3



@--------------------------------
    ldr           r0, [sp, #124]        @r0 =u4_lambda
    ldr           r1, [sp, #128]        @r1 = u4_predictd_mode
@------


vert:
    ands          r10, r8, #01          @VERT sad ??
    beq           horz
    vdup.32       q2, d2[1]
    vabdl.u8      q14, d4, d20
    vabal.u8      q14, d4, d21
    vadd.i16      d28, d29, d28
    subs          r6, r1, #0
    vpaddl.u16    d28, d28              @
    lslne         r6, r0, #2
    vpaddl.u32    d28, d28              @/
    moveq         r6, r0                @
    vmov.u32      r9, d28[0]            @ vert
    add           r9, r6, r9

    subs          r6, r11, r9
    movgt         r11, r9
    movgt         r12, #0

horz:
    ands          r10, r8, #02          @HORZ sad ??
    beq           dc
    vdup.32       q3, d0[0]
    vmov.32       q4, q3
    vtrn.8        q3, q4
    vtrn.16       d7, d6
    vtrn.16       d9, d8
    vtrn.32       d9, d7
    vtrn.32       d8, d6
    vabdl.u8      q14, d6, d20
    subs          r6, r1, #1
    vabal.u8      q14, d7, d21
    vadd.i16      d28, d29, d28
    lslne         r6, r0, #2
    vpaddl.u16    d28, d28              @
    vpaddl.u32    d28, d28              @/
    vmov.u32      r9, d28[0]            @
    moveq         r6, r0                @
    add           r9, r6, r9

    subs          r6, r11, r9
    movgt         r11, r9
    movgt         r12, #1

dc:
    ands          r10, r8, #04          @DC sad ??
    beq           diags
    vext.8        q4, q0, q0, #5
    vaddl.u8      q4, d0, d8
    vpaddl.u16    d8, d8                @
    vpaddl.u32    d8, d8                @/
    vmov.u32      r4, d8[0]             @
    mov           r14, #1
    ands          r10, r5, #1
    addne         r4, r4, #2
    addne         r14, r14, #1
    ands          r10, r5, #4
    addne         r4, r4, #2
    addne         r14, r14, #1
    ands          r10, r5, #5
    moveq         r4, #128
    moveq         r14, #0
    subs          r6, r1, #2
    lsr           r4, r4, r14
    vdup.8        q4, r4
    lslne         r6, r0, #2
    vabdl.u8      q14, d8, d20
    vabal.u8      q14, d9, d21
    vadd.i16      d28, d29, d28
    vpaddl.u16    d28, d28              @
    vpaddl.u32    d28, d28              @/
    vmov.u32      r9, d28[0]            @

    moveq         r6, r0                @
    add           r9, r6, r9

    subs          r6, r11, r9
    movgt         r11, r9
    movgt         r12, #2

diags:
    ands          r10, r8, #504         @/* if modes other than VERT, HORZ and DC are  valid ????*/
    beq           pred
    @/* Performing FILT11 and FILT121 operation for all neighbour values*/
    vext.8        q5, q0, q0, #2
    vaddl.u8      q6, d0, d2
    vaddl.u8      q7, d1, d3
    vaddl.u8      q8, d10, d2
    vaddl.u8      q9, d11, d3
    vadd.u16      q12, q10, q11
    vqrshrun.s16  d10, q6, #1
    vqrshrun.s16  d11, q7, #1
    vadd.u16      q11, q6, q8
    vadd.u16      q12, q7, q9
    vqrshrun.s16  d12, q11, #2
    vqrshrun.s16  d13, q12, #2
    mov           r14, #0
    vdup.32       q13 , r14
    mov           r14, #-1
    vmov.i32      d26[0], r14

diag_dl:
    ands          r10, r8, #0x08        @DIAG_DL sad ??
    beq           diag_dr

    vext.8        q15, q6, q6, #5
    vbit.32       d14, d30, d26
    vext.8        q15, q6, q6, #15
    vbit.32       d15, d31, d26
    vext.8        q15, q6, q6, #2
    vext.32       q14, q13, q13, #3
    vbit.32       d14, d30, d28
    vext.8        q15, q6, q6, #4
    vbit.32       d15, d30, d28
    vabdl.u8      q14, d14, d20
    subs          r6, r1, #3
    vabal.u8      q14, d15, d21
    vadd.i16      d28, d29, d28
    vpaddl.u16    d28, d28              @
    lslne         r6, r0, #2
    vpaddl.u32    d28, d28              @/
    vmov.u32      r9, d28[0]            @

    moveq         r6, r0                @
    add           r9, r6, r9

    subs          r6, r11, r9
    movgt         r11, r9
    movgt         r12, #3

diag_dr:
    ands          r10, r8, #16          @DIAG_DR sad ??
    beq           vert_r

    vext.8        q15, q6, q6, #3
    vbit.32       d16, d30, d26
    vext.8        q15, q6, q6, #1
    vbit.32       d17, d30, d26
    vext.8        q15, q6, q6, #4
    vext.32       q14, q13, q13, #3
    vbit.32       d17, d31, d28
    vext.8        q15, q6, q6, #6
    vbit.32       d16, d31, d28
    vabdl.u8      q14, d16, d20
    subs          r6, r1, #4
    vabal.u8      q14, d17, d21
    vadd.i16      d28, d29, d28
    vpaddl.u16    d28, d28              @
    lslne         r6, r0, #2
    vpaddl.u32    d28, d28              @/
    vmov.u32      r9, d28[0]            @

    moveq         r6, r0                @
    add           r9, r6, r9

    subs          r6, r11, r9
    movgt         r11, r9
    movgt         r12, #4

vert_r:
    ands          r10, r8, #32          @VERT_R sad ??
    beq           horz_d
    vext.8        q15, q5, q5, #4
    vbit.32       d18, d30, d26
    vext.8        q15, q5, q5, #3
    vbit.32       d19, d30, d26
    vext.32       q14, q13, q13, #3
    vext.8        q15, q6, q6, #15
    vbit.32       d18, d30, d28
    vext.8        q15, q6, q6, #14
    vbit.32       d19, d30, d28
    mov           r14, #0
    vdup.32       q14 , r14
    mov           r14, #0xff
    vmov.i8       d28[0], r14
    vext.8        q15, q6, q6, #2
    vbit.32       d19, d30, d28
    vext.32       q14, q14, q14, #3
    subs          r6, r1, #5
    vext.8        q15, q6, q6, #13
    vbit.32       d19, d30, d28
    lslne         r6, r0, #2
    vabdl.u8      q14, d18, d20
    vabal.u8      q14, d19, d21
    vadd.i16      d28, d29, d28
    vpaddl.u16    d28, d28              @
    vpaddl.u32    d28, d28              @/
    vmov.u32      r9, d28[0]            @


    moveq         r6, r0                @
    add           r9, r6, r9

    subs          r6, r11, r9
    movgt         r11, r9
    movgt         r12, #5

horz_d:
    vmov.8        q1, q5
    vmov.8        q15, q6
    vzip.8        q1, q15

    ands          r10, r8, #64          @HORZ_D sad ??
    beq           vert_l
    vext.8        q15, q6, q6, #2
    vbit.32       d8, d30, d26
    mov           r14, #0
    vdup.32       q14 , r14
    mov           r14, #0xff
    vmov.i8       d28[0], r14
    vext.8        q15, q5, q5, #3
    vbit.32       d8, d30, d28
    vext.8        q15, q1, q1, #2
    vbit.32       d9, d30, d26
    vext.32       q14, q13, q13, #3
    vbit.32       d8, d2, d28
    subs          r6, r1, #6
    vext.8        q15, q1, q1, #12
    vbit.32       d9, d30, d28
    vabdl.u8      q14, d8, d20
    vabal.u8      q14, d9, d21
    vadd.i16      d28, d29, d28
    vpaddl.u16    d28, d28              @
    lslne         r6, r0, #2
    vpaddl.u32    d28, d28              @/
    vmov.u32      r9, d28[0]            @


    moveq         r6, r0                @
    add           r9, r6, r9

    subs          r6, r11, r9
    movgt         r11, r9
    movgt         r12, #6
vert_l:
    ands          r10, r8, #128         @VERT_L sad ??
    beq           horz_u
    vext.8        q15, q5, q5, #5
    vbit.32       d24, d30, d26
    vext.8        q15, q15, q15, #1
    vbit.32       d25, d30, d26
    vext.8        q15, q6, q6, #1
    vext.32       q14, q13, q13, #3
    vbit.32       d24, d30, d28
    vext.8        q15, q15, q15, #1
    subs          r6, r1, #7
    vbit.32       d25, d30, d28
    vabdl.u8      q14, d24, d20
    vabal.u8      q14, d25, d21
    vadd.i16      d28, d29, d28
    vpaddl.u16    d28, d28              @
    lslne         r6, r0, #2
    vpaddl.u32    d28, d28              @/
    vmov.u32      r9, d28[0]            @

    moveq         r6, r0                @
    add           r9, r6, r9

    subs          r6, r11, r9
    movgt         r11, r9
    movgt         r12, #7

horz_u:
    ands          r10, r8, #256         @HORZ_U sad ??
    beq           pred
    vrev64.8      q5, q1
    vdup.8        q1, d0[0]
    vext.8        q6, q6, #7
    mov           r14, #0
    vdup.32       q14 , r14
    mov           r14, #0xff
    vmov.i8       d28[0], r14
    vbit.32       d11, d13, d28
    movw          r14, #0xffff
    vmov.i16      d28[0], r14
    vext.8        q6, q5, q5, #7
    subs          r6, r1, #8
    vbit.32       d3, d12, d28
    vext.8        q6, q5, q5, #3
    vbit.32       d2, d12, d26
    vext.32       q14, q13, q13, #3
    vext.8        q6, q5, q5, #1
    vbit.32       d2, d12, d28
    vabdl.u8      q14, d2, d20
    vabal.u8      q14, d3, d21
    vadd.i16      d28, d29, d28
    vpaddl.u16    d28, d28              @
    lslne         r6, r0, #2
    vpaddl.u32    d28, d28              @/
    vmov.u32      r9, d28[0]            @


    moveq         r6, r0                @
    add           r9, r6, r9

    subs          r6, r11, r9
    movgt         r11, r9
    movgt         r12, #8

pred: @/*dOING FINAL PREDICTION*/
@---------------------------
    ldr           r7, [sp, #116]        @r7 = pu4_sadmin
    ldr           r6, [sp, #112]        @ R6 =MODE
@--------------------------
    str           r11, [r7]             @/STORING MIN SAD*/
    str           r12, [r6]             @/FINAL MODE*/


    ldr           r3, scratch_intrapred_luma_4x4_prediction_addr1
scrintra_4x4:
    add           r3, r3, pc
    lsl           r12, r12, #2
    add           r3, r3, r12

    ldr           r5, [r3]
    and           r5, r5, #0xfffffffe

    bx            r5


ver:
    vext.8        q0, q0, q0, #1
    vdup.32       q15, d0[1]
    b             store

hor:
    vmov.32       q15, q3
    b             store

d_c:
    vdup.8        q15, r4
    b             store

dia_dl:
    vmov.32       q15, q7
    b             store

dia_dr:
    vmov.32       q15, q8
    b             store

ver_r:
    vmov.32       q15, q9
    b             store

hor_d:
    vmov.32       q15, q4
    b             store

ver_l:
    vmov.32       q15, q12
    b             store

hor_u:
    vmov.32       q15, q1

store: @/* storing to pu1_dst*/

    ldr           r4, [sp, #104]        @r4 = dst_strd,

    vst1.32       {d30[0]}, [r2], r4
    vst1.32       {d30[1]}, [r2], r4
    vst1.32       {d31[0]}, [r2], r4
    vst1.32       {d31[1]}, [r2], r4


end_func:
    vpop          {d8-d15}
    ldmfd         sp!, {r4-r12, pc}     @Restoring registers from stack





