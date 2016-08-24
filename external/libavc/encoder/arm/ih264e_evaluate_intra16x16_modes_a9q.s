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
@* @brief :Evaluate best intra 16x16 mode (among VERT, HORZ and DC )
@*                and do the prediction.
@*
@* @par Description
@*   This function evaluates  first three 16x16 modes and compute corresponding sad
@*   and return the buffer predicted with best mode.
@*
@* @param[in] pu1_src
@*  UWORD8 pointer to the source
@*
@** @param[in] pu1_ngbr_pels_i16
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
@void ih264e_evaluate_intra16x16_modes(UWORD8 *pu1_src,
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

    .global ih264e_evaluate_intra16x16_modes_a9q

ih264e_evaluate_intra16x16_modes_a9q:

@r0 = pu1_src,
@r1 = pu1_ngbr_pels_i16,
@r2 = pu1_dst,
@r3 = src_strd,
@r4 = dst_strd,
@r5 = u4_n_avblty,
@r6 = u4_intra_mode,
@r7 = pu4_sadmin



    stmfd         sp!, {r4-r12, r14}    @store register values to stack
    ldr           r5, [sp, #44]


    vpush         {d8-d15}
    vld1.32       {q4}, [r1]!
    sub           r6, r1, #1
    add           r1, r1, #1
    mov           r10, #0
    vld1.32       {q5}, [r1]!
    mov           r11, #0
    mov           r4, #0
    @/* Left available ???? */
    ands          r7, r5, #01
    movne         r10, #1

    @/* Top  available ???? */
    ands          r8, r5, #04
    lsl           r9, r10, #3
    movne         r11, #1
    lsl           r12, r11, #3
    adds          r8, r9, r12


    @/* None available :( */
    moveq         r4, #128



@/fINDING dc val*/
    @----------------------
    vaddl.u8      q15, d8, d9

    vaddl.u8      q14, d10, d11

    vadd.u16      q15, q14, q15
    @ VLD1.32  {q2},[r0],r3;row 2
    vadd.u16      d30, d31, d30
    vpadd.u16     d30, d30
    @ VLD1.32  {q3},[r0],r3 ;row 3
    vpadd.u16     d30, d30
    @---------------------


    vmov.u16      r7, d30[0]
    add           r7, r7, r8
    add           r11, r11, #3
    add           r8, r10, r11

    lsr           r7, r8
    add           r7, r4, r7
    vld1.32       {q0}, [r0], r3        @ source r0w 0
    vdup.8        q15, r7               @dc val

@/* computing SADs for all three modes*/
    ldrb          r7, [r6]
    vdup.8        q10, r7               @/HORIZONTAL VALUE ROW=0;
    @/vertical row 0;
    vabdl.u8      q8, d0, d10
    vabdl.u8      q9, d1, d11
    sub           r6, r6, #1
    @/HORZ row 0;
    vabdl.u8      q13, d0, d20
    vabdl.u8      q14, d1, d21
    mov           r1, #15
    @/dc row 0;
    vabdl.u8      q11, d0, d30
    vabdl.u8      q12, d1, d31


loop:
    vld1.32       {q1}, [r0], r3        @row i
    @/dc row i;
    vabal.u8      q11, d2, d30
    ldrb          r7, [r6]
    vabal.u8      q12, d3, d31

    @/vertical row i;
    vabal.u8      q8, d2, d10
    vdup.8        q10, r7               @/HORIZONTAL VALUE ROW=i;
    sub           r6, r6, #1
    vabal.u8      q9, d3, d11

    subs          r1, r1, #1
    @/HORZ row i;
    vabal.u8      q13, d2, d20
    vabal.u8      q14, d3, d21
    bne           loop

    @------------------------------------------------------------------------------

    vadd.i16      q9, q9, q8            @/VERT
    vadd.i16      d18, d19, d18         @/VERT
    vpaddl.u16    d18, d18              @/VERT
    vadd.i16      q14, q13, q14         @/HORZ
    vadd.i16      d28, d29, d28         @/HORZ
    vpaddl.u32    d18, d18              @/VERT
    vpaddl.u16    d28, d28              @/HORZ

    vpaddl.u32    d28, d28              @/HORZ
    vmov.u32      r8, d18[0]            @ vert
    vadd.i16      q12, q11, q12         @/DC
    vmov.u32      r9, d28[0]            @horz
    mov           r11, #1
    vadd.i16      d24, d24, d25         @/DC
    lsl           r11 , #30

    @-----------------------
    ldr           r0, [sp, #120]        @ u4_valid_intra_modes
    @--------------------------------------------
    ands          r7, r0, #01           @ vert mode valid????????????
    moveq         r8, r11
    vpaddl.u16    d24, d24              @/DC

    ands          r6, r0, #02           @ horz mode valid????????????
    moveq         r9, r11
    vpaddl.u32    d24, d24              @/DC

    vmov.u32      r10, d24[0]           @dc
@--------------------------------
    ldr           r4, [sp, #104]        @r4 = dst_strd,
    ldr           r7, [sp, #116]        @r7 = pu4_sadmin
@----------------------------------------------
    ands          r6, r0, #04           @ dc mode valid????????????
    moveq         r10, r11

    @---------------------------
    ldr           r6, [sp, #112]        @ R6 =MODE
    @--------------------------

    cmp           r8, r9
    bgt           not_vert
    cmp           r8, r10
    bgt           do_dc

    @/----------------------
    @DO VERTICAL PREDICTION
    str           r8 , [r7]             @MIN SAD
    mov           r8, #0
    str           r8 , [r6]             @ MODE
    vmov          q15, q5

    b             do_dc_vert
    @-----------------------------
not_vert:
    cmp           r9, r10
    bgt           do_dc

    @/----------------------
    @DO HORIZONTAL
    vdup.8        q5, d9[7]             @0
    str           r9 , [r7]             @MIN SAD
    vdup.8        q6, d9[6]             @1
    mov           r9, #1
    vdup.8        q7, d9[5]             @2
    vst1.32       {d10, d11} , [r2], r4 @0
    vdup.8        q8, d9[4]             @3
    str           r9 , [r6]             @ MODE
    vdup.8        q9, d9[3]             @4
    vst1.32       {d12, d13} , [r2], r4 @1
    vdup.8        q10, d9[2]            @5
    vst1.32       {d14, d15} , [r2], r4 @2
    vdup.8        q11, d9[1]            @6
    vst1.32       {d16, d17} , [r2], r4 @3
    vdup.8        q12, d9[0]            @7
    vst1.32       {d18, d19} , [r2], r4 @4
    vdup.8        q13, d8[7]            @8
    vst1.32       {d20, d21} , [r2], r4 @5
    vdup.8        q14, d8[6]            @9
    vst1.32       {d22, d23} , [r2], r4 @6
    vdup.8        q15, d8[5]            @10
    vst1.32       {d24, d25} , [r2], r4 @7
    vdup.8        q1, d8[4]             @11
    vst1.32       {d26, d27} , [r2], r4 @8
    vdup.8        q2, d8[3]             @12
    vst1.32       {d28, d29} , [r2], r4 @9
    vdup.8        q3, d8[2]             @13
    vst1.32       {d30, d31}, [r2], r4  @10
    vdup.8        q5, d8[1]             @14
    vst1.32       {d2, d3} , [r2], r4   @11
    vdup.8        q6, d8[0]             @15
    vst1.32       {d4, d5} , [r2], r4   @12

    vst1.32       {d6, d7} , [r2], r4   @13

    vst1.32       {d10, d11} , [r2], r4 @14

    vst1.32       {d12, d13} , [r2], r4 @15
    b             end_func


    @/-----------------------------

do_dc: @/---------------------------------
    @DO DC
    str           r10 , [r7]            @MIN SAD
    mov           r10, #2
    str           r10 , [r6]            @ MODE
do_dc_vert:
    vst1.32       {d30, d31}, [r2], r4  @0
    vst1.32       {d30, d31}, [r2], r4  @1
    vst1.32       {d30, d31}, [r2], r4  @2
    vst1.32       {d30, d31}, [r2], r4  @3
    vst1.32       {d30, d31}, [r2], r4  @4
    vst1.32       {d30, d31}, [r2], r4  @5
    vst1.32       {d30, d31}, [r2], r4  @6
    vst1.32       {d30, d31}, [r2], r4  @7
    vst1.32       {d30, d31}, [r2], r4  @8
    vst1.32       {d30, d31}, [r2], r4  @9
    vst1.32       {d30, d31}, [r2], r4  @10
    vst1.32       {d30, d31}, [r2], r4  @11
    vst1.32       {d30, d31}, [r2], r4  @12
    vst1.32       {d30, d31}, [r2], r4  @13
    vst1.32       {d30, d31}, [r2], r4  @14
    vst1.32       {d30, d31}, [r2], r4  @15
    @/------------------
end_func:
    vpop          {d8-d15}
    ldmfd         sp!, {r4-r12, pc}     @Restoring registers from stack


