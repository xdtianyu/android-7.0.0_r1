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

@/**
@******************************************************************************
@*
@* @brief :Evaluate best intr chroma mode (among VERT, HORZ and DC )
@*                and do the prediction.
@*
@* @par Description
@*   This function evaluates  first three intra chroma modes and compute corresponding sad
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
@* Pointer to the variable in which minimum sad is returned
@*
@* @param[in] u4_valid_intra_modes
@* Says what all modes are valid
@*
@*
@* @return      none
@*
@******************************************************************************
@*/
@
@void ih264e_evaluate_intra_chroma_modes(UWORD8 *pu1_src,
@                                      UWORD8 *pu1_ngbr_pels_i16,
@                                      UWORD8 *pu1_dst,
@                                      UWORD32 src_strd,
@                                      UWORD32 dst_strd,
@                                      WORD32 u4_n_avblty,
@                                      UWORD32 *u4_intra_mode,
@                                      WORD32 *pu4_sadmin,
@                                      UWORD32 u4_valid_intra_modes)
@
.text
.p2align 2

    .global ih264e_evaluate_intra_chroma_modes_a9q

ih264e_evaluate_intra_chroma_modes_a9q:

@r0 = pu1_src,
@r1 = pu1_ngbr_pels_i16,
@r2 = pu1_dst,
@r3 = src_strd,
@r4 = dst_strd,
@r5 = u4_n_avblty,
@r6 = u4_intra_mode,
@r7 = pu4_sadmin



    stmfd         sp!, {r4-r12, r14}    @store register values to stack
    @-----------------------
    ldr           r5, [sp, #44]         @r5 = u4_n_avblty,
    @-------------------------
    mov           r12, r1               @
    vpush         {d8-d15}
    vld1.32       {q4}, [r1]!
    add           r1, r1, #2
    vld1.32       {q5}, [r1]!

    vuzp.u8       q4, q5                @

    vpaddl.u8     d8, d8
    vpadd.u16     d8, d8

    vpaddl.u8     d9, d9
    vpadd.u16     d9, d9

    vpaddl.u8     d10, d10
    vpadd.u16     d10, d10

    vpaddl.u8     d11, d11

    and           r7, r5, #5
    vpadd.u16     d11, d11
    subs          r8, r7, #5
    beq           all_available
    subs          r8, r7, #4
    beq           top_available
    subs          r8, r7, #1
    beq           left_available
    mov           r10, #128
    vdup.8        q14, r10
    vdup.8        q15, r10
    b             sad

all_available:
    vzip.u16      q4, q5
    vext.16       q6, q4, q4, #2
    vadd.u16      q7, q5, q6
    vqrshrn.u16   d14, q7, #3
    vqrshrn.u16   d15, q4, #2
    vqrshrn.u16   d16, q5, #2
    vdup.16       d28, d14[0]
    vdup.16       d29, d16[1]
    vdup.16       d30, d15[0]
    vdup.16       d31, d14[1]
    b             sad
top_available:
    vzip.u16      q4, q5
    vqrshrn.u16   d16, q5, #2
    vdup.16       d28, d16[0]
    vdup.16       d29, d16[1]
    vdup.16       d30, d16[0]
    vdup.16       d31, d16[1]
    b             sad
left_available:
    vzip.u16      q4, q5
    vqrshrn.u16   d16, q4, #2
    vdup.16       d28, d16[3]
    vdup.16       d29, d16[3]
    vdup.16       d30, d16[2]
    vdup.16       d31, d16[2]


sad:
    vld1.32       {q4}, [r12]!
    sub           r8, r12, #2
    add           r12, r12, #2
    vld1.32       {q5}, [r12]!
    add           r12, r0, r3, lsl  #2
    sub           r10, r8, #8
    vld1.32       {q0}, [r0], r3
    ldrh          r9, [r8]
    vdup.16       q10, r9               @ row 0

    @/vertical row 0;
    vabdl.u8      q8, d0, d10
    vabdl.u8      q9, d1, d11
    sub           r8, r8, #2
    vld1.32       {q1}, [r12], r3

    @/HORZ row 0;
    vabdl.u8      q13, d0, d20
    vabdl.u8      q7, d1, d21
    ldrh          r9, [r10]
    @/dc row 0;
    vabdl.u8      q11, d0, d28
    vabdl.u8      q12, d1, d29


    vdup.16       q10, r9               @ row 4
    @/vertical row 4;
    vabal.u8      q8, d2, d10
    vabal.u8      q9, d3, d11
    sub           r10, r10, #2

    @/HORZ row 4;
    vabal.u8      q13, d2, d20
    vabal.u8      q7, d3, d21
    @/dc row 4;
    vabal.u8      q11, d2, d30
    vabal.u8      q12, d3, d31

    mov           r11, #3

loop:
    vld1.32       {q0}, [r0], r3
    ldrh          r9, [r8]


    @/vertical row i;
    vabal.u8      q8, d0, d10
    vabal.u8      q9, d1, d11

    vdup.16       q10, r9               @ row i
    vld1.32       {q1}, [r12], r3
    sub           r8, r8, #2
    @/HORZ row i;
    vabal.u8      q13, d0, d20
    vabal.u8      q7, d1, d21
    ldrh          r9, [r10]
    @/dc row i;
    vabal.u8      q11, d0, d28
    vabal.u8      q12, d1, d29
    sub           r10, r10, #2

    vdup.16       q10, r9               @ row i+4
    @/vertical row 4;
    vabal.u8      q8, d2, d10
    vabal.u8      q9, d3, d11
    subs          r11, r11, #1

    @/HORZ row i+4;
    vabal.u8      q13, d2, d20
    vabal.u8      q7, d3, d21
    @/dc row i+4;
    vabal.u8      q11, d2, d30
    vabal.u8      q12, d3, d31
    bne           loop



@-------------------------------------------

    vadd.i16      q9, q9, q8            @/VERT
    vadd.i16      q7, q13, q7           @/HORZ
    vadd.i16      q12, q11, q12         @/DC
    vadd.i16      d18, d19, d18         @/VERT
    vadd.i16      d14, d15, d14         @/HORZ
    vadd.i16      d24, d24, d25         @/DC
    vpaddl.u16    d18, d18              @/VERT
    vpaddl.u16    d14, d14              @/HORZ
    vpaddl.u16    d24, d24              @/DC
    vpaddl.u32    d18, d18              @/VERT
    vpaddl.u32    d14, d14              @/HORZ
    vpaddl.u32    d24, d24              @/DC



    vmov.u32      r8, d18[0]            @ vert
    vmov.u32      r9, d14[0]            @horz
    vmov.u32      r10, d24[0]           @dc

    mov           r11, #1
@-----------------------
    ldr           r0, [sp, #120]        @ u4_valid_intra_modes
@--------------------------------------------


    lsl           r11 , #30

    ands          r7, r0, #04           @ vert mode valid????????????
    moveq         r8, r11

    ands          r6, r0, #02           @ horz mode valid????????????
    moveq         r9, r11

    ands          r6, r0, #01           @ dc mode valid????????????
    moveq         r10, r11


    @---------------------------
    ldr           r4, [sp, #104]        @r4 = dst_strd,
    ldr           r6, [sp, #112]        @ R6 =MODE
    ldr           r7, [sp, #116]        @r7 = pu4_sadmin

    @--------------------------

    cmp           r10, r9
    bgt           not_dc
    cmp           r10, r8
    bgt           do_vert

    @/----------------------
    @DO DC PREDICTION
    str           r10 , [r7]            @MIN SAD
    mov           r10, #0
    str           r10 , [r6]            @ MODE
    b             do_dc_vert
    @-----------------------------

not_dc:
    cmp           r9, r8
    bgt           do_vert
    @/----------------------
    @DO HORIZONTAL

    vdup.16       q10, d9[3]            @/HORIZONTAL VALUE ROW=0;
    str           r9 , [r7]             @MIN SAD
    mov           r9, #1
    vdup.16       q11, d9[2]            @/HORIZONTAL VALUE ROW=1;
    str           r9 , [r6]             @ MODE
    vdup.16       q12, d9[1]            @/HORIZONTAL VALUE ROW=2;
    vst1.32       {d20, d21} , [r2], r4 @0
    vdup.16       q13, d9[0]            @/HORIZONTAL VALUE ROW=3;
    vst1.32       {d22, d23} , [r2], r4 @1
    vdup.16       q14, d8[3]            @/HORIZONTAL VALUE ROW=4;
    vst1.32       {d24, d25} , [r2], r4 @2
    vdup.16       q15, d8[2]            @/HORIZONTAL VALUE ROW=5;
    vst1.32       {d26, d27} , [r2], r4 @3
    vdup.16       q1, d8[1]             @/HORIZONTAL VALUE ROW=6;
    vst1.32       {d28, d29} , [r2], r4 @4
    vdup.16       q2, d8[0]             @/HORIZONTAL VALUE ROW=7;
    vst1.32       {d30, d31} , [r2], r4 @5
    vst1.32       {d2, d3} , [r2], r4   @6
    vst1.32       {d4, d5} , [r2], r4   @7
    b             end_func

do_vert:
    @DO VERTICAL PREDICTION
    str           r8 , [r7]             @MIN SAD
    mov           r8, #2
    str           r8 , [r6]             @ MODE
    vmov          q15, q5
    vmov          q14, q5

do_dc_vert:
    vst1.32       {d28, d29} , [r2], r4 @0
    vst1.32       {d28, d29} , [r2], r4 @1
    vst1.32       {d28, d29} , [r2], r4 @2
    vst1.32       {d28, d29} , [r2], r4 @3
    vst1.32       {d30, d31} , [r2], r4 @4
    vst1.32       {d30, d31} , [r2], r4 @5
    vst1.32       {d30, d31} , [r2], r4 @6
    vst1.32       {d30, d31} , [r2], r4 @7


end_func:
    vpop          {d8-d15}
    ldmfd         sp!, {r4-r12, pc}     @Restoring registers from stack



