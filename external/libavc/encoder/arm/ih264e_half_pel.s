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
@ *******************************************************************************
@ * @file
@ *  ih264e_half_pel.s
@ *
@ * @brief
@ *
@ *
@ * @author
@ *  Ittiam
@ *
@ * @par List of Functions:
@ *  ih264e_sixtapfilter_horz
@ *  ih264e_sixtap_filter_2dvh_vert
@
@ *
@ * @remarks
@ *  None
@ *
@ *******************************************************************************
@ */


.text
.p2align 2

@/*******************************************************************************
@*
@* @brief
@*     Interprediction luma filter for horizontal input(Filter run for width = 17 and height =16)
@*
@* @par Description:
@*    Applies a 6 tap horizontal filter .The output is  clipped to 8 bits
@*    sec 8.4.2.2.1 titled "Luma sample interpolation process"
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
@*
@* @returns
@*
@* @remarks
@*  None
@*
@*******************************************************************************
@*/
@void ih264e_sixtapfilter_horz(UWORD8 *pu1_src,
@                                UWORD8 *pu1_dst,
@                                WORD32 src_strd,
@                                WORD32 dst_strd);


.equ HALFPEL_WIDTH ,  17 + 1            @( make it even, two rows are processed at a time)


    .global ih264e_sixtapfilter_horz_a9q
ih264e_sixtapfilter_horz_a9q:
    stmfd         sp!, {lr}

    vmov.i8       d0, #5
    sub           r0, r0, #2

    vmov.i8       d1, #20
    mov           r14, #HALFPEL_WIDTH
    vpush         {d8-d15}

filter_horz_loop:


    vld1.8        {d2, d3, d4}, [r0], r2 @// Load row0
    vld1.8        {d5, d6, d7}, [r0], r2 @// Load row1

    @// Processing row0 and row1

    vext.8        d31, d2, d3, #5       @//extract a[5]                         (column1,row0)
    vext.8        d30, d3, d4, #5       @//extract a[5]                         (column2,row0)

    vaddl.u8      q4, d31, d2           @// a0 + a5                             (column1,row0)
    vext.8        d29, d4, d4, #5       @//extract a[5]                         (column3,row0)
    vaddl.u8      q5, d30, d3           @// a0 + a5                             (column2,row0)
    vext.8        d28, d5, d6, #5       @//extract a[5]                         (column1,row1)
    vaddl.u8      q6, d29, d4           @// a0 + a5                             (column3,row0)
    vext.8        d27, d6, d7, #5       @//extract a[5]                         (column2,row1)
    vaddl.u8      q7, d28, d5           @// a0 + a5                             (column1,row1)
    vext.8        d26, d7, d7, #5       @//extract a[5]                         (column3,row1)

    vaddl.u8      q8, d27, d6           @// a0 + a5                             (column2,row1)
    vext.8        d31, d2, d3, #2       @//extract a[2]                         (column1,row0)
    vaddl.u8      q9, d26, d7           @// a0 + a5                             (column3,row1)
    vext.8        d30, d3, d4, #2       @//extract a[2]                         (column2,row0)
    vmlal.u8      q4, d31, d1           @// a0 + a5 + 20a2                      (column1,row0)
    vext.8        d29, d4, d4, #2       @//extract a[2]                         (column3,row0)
    vmlal.u8      q5, d30, d1           @// a0 + a5 + 20a2                      (column2,row0)
    vext.8        d28, d5, d6, #2       @//extract a[2]                         (column1,row1)
    vmlal.u8      q6, d29, d1           @// a0 + a5 + 20a2                      (column3,row0)
    vext.8        d27, d6, d7, #2       @//extract a[2]                         (column2,row1)
    vmlal.u8      q7, d28, d1           @// a0 + a5 + 20a2                      (column1,row1)
    vext.8        d26, d7, d7, #2       @//extract a[2]                         (column3,row1)

    vmlal.u8      q8, d27, d1           @// a0 + a5 + 20a2                      (column2,row1)
    vext.8        d31, d2, d3, #3       @//extract a[3]                         (column1,row0)
    vmlal.u8      q9, d26, d1           @// a0 + a5 + 20a2                      (column3,row1)
    vext.8        d30, d3, d4, #3       @//extract a[3]                         (column2,row0)
    vmlal.u8      q4, d31, d1           @// a0 + a5 + 20a2 + 20a3               (column1,row0)
    vext.8        d29, d4, d4, #3       @//extract a[3]                         (column3,row0)
    vmlal.u8      q5, d30, d1           @// a0 + a5 + 20a2 + 20a3               (column2,row0)
    vext.8        d28, d5, d6, #3       @//extract a[3]                         (column1,row1)
    vmlal.u8      q6, d29, d1           @// a0 + a5 + 20a2 + 20a3               (column3,row0)
    vext.8        d27, d6, d7, #3       @//extract a[3]                         (column2,row1)
    vmlal.u8      q7, d28, d1           @// a0 + a5 + 20a2 + 20a3               (column1,row1)
    vext.8        d26, d7, d7, #3       @//extract a[3]                         (column3,row1)

    vmlal.u8      q8, d27, d1           @// a0 + a5 + 20a2 + 20a3               (column2,row1)
    vext.8        d31, d2, d3, #1       @//extract a[1]                         (column1,row0)
    vmlal.u8      q9, d26, d1           @// a0 + a5 + 20a2 + 20a3               (column3,row1)
    vext.8        d30, d3, d4, #1       @//extract a[1]                         (column2,row0)
    vmlsl.u8      q4, d31, d0           @// a0 + a5 + 20a2 + 20a3 - 5a1         (column1,row0)
    vext.8        d29, d4, d4, #1       @//extract a[1]                         (column3,row0)
    vmlsl.u8      q5, d30, d0           @// a0 + a5 + 20a2 + 20a3 - 5a1         (column2,row0)
    vext.8        d28, d5, d6, #1       @//extract a[1]                         (column1,row1)
    vmlsl.u8      q6, d29, d0           @// a0 + a5 + 20a2 + 20a3 - 5a1         (column3,row0)
    vext.8        d27, d6, d7, #1       @//extract a[1]                         (column2,row1)
    vmlsl.u8      q7, d28, d0           @// a0 + a5 + 20a2 + 20a3 - 5a1         (column1,row1)
    vext.8        d26, d7, d7, #1       @//extract a[1]                         (column3,row1)

    vmlsl.u8      q8, d27, d0           @// a0 + a5 + 20a2 + 20a3 - 5a1         (column2,row1)
    vext.8        d31, d2, d3, #4       @//extract a[4]                         (column1,row0)
    vmlsl.u8      q9, d26, d0           @// a0 + a5 + 20a2 + 20a3 - 5a1         (column3,row1)
    vext.8        d30, d3, d4, #4       @//extract a[4]                         (column2,row0)
    vmlsl.u8      q4, d31, d0           @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (column1,row0)
    vext.8        d29, d4, d4, #4       @//extract a[4]                         (column3,row0)
    vmlsl.u8      q5, d30, d0           @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (column2,row0)
    vext.8        d28, d5, d6, #4       @//extract a[4]                         (column1,row1)
    vmlsl.u8      q6, d29, d0           @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (column3,row0)
    vext.8        d27, d6, d7, #4       @//extract a[4]                         (column2,row1)
    vmlsl.u8      q7, d28, d0           @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (column1,row1)
    vext.8        d26, d7, d7, #4       @//extract a[4]                         (column3,row1)

    vmlsl.u8      q8, d27, d0           @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (column2,row1)
    vmlsl.u8      q9, d26, d0           @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (column3,row1)

    vqrshrun.s16  d20, q4, #5           @// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5   (column1,row0)
    vqrshrun.s16  d21, q5, #5           @// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5   (column2,row0)
    vqrshrun.s16  d22, q6, #5           @// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5   (column3,row0)
    vqrshrun.s16  d23, q7, #5           @// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5   (column1,row1)
    vqrshrun.s16  d24, q8, #5           @// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5   (column2,row1)
    vqrshrun.s16  d25, q9, #5           @// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5   (column3,row1)

    vst1.8        {d20, d21, d22}, [r1], r3 @//Store dest row0
    vst1.8        {d23, d24, d25}, [r1], r3 @//Store dest row1

    subs          r14, r14, #2          @   decrement counter

    bne           filter_horz_loop

    vpop          {d8-d15}
    ldmfd         sp!, {pc}









@/**
@*******************************************************************************
@*
@* @brief
@*   This function implements a two stage cascaded six tap filter. It
@*    applies the six tap filter in the vertical direction on the
@*    predictor values, followed by applying the same filter in the
@*    horizontal direction on the output of the first stage. The six tap
@*    filtering operation is described in sec 8.4.2.2.1 titled "Luma sample
@*    interpolation process"
@*    (Filter run for width = 17 and height =17)
@* @par Description:
@*    The function interpolates
@*    the predictors first in the vertical direction and then in the
@*    horizontal direction to output the (1/2,1/2). The output of the first
@*    stage of the filter is stored in the buffer pointed to by pi16_pred1(only in C)
@*    in 16 bit precision.
@*
@*
@* @param[in] pu1_src
@*  UWORD8 pointer to the source
@*
@* @param[out] pu1_dst1
@*  UWORD8 pointer to the destination(vertical filtered output)
@*
@* @param[out] pu1_dst2
@*  UWORD8 pointer to the destination(out put after applying horizontal filter to the intermediate vertical output)
@*
@* @param[in] src_strd
@*  integer source stride
@*
@* @param[in] dst_strd
@*  integer destination stride of pu1_dst
@*
@* @param[in]pi16_pred1
@*  Pointer to 16bit intermediate buffer(used only in c)
@*
@* @param[in] pi16_pred1_strd
@*  integer destination stride of pi16_pred1
@*
@*
@* @returns
@*
@* @remarks
@*  None
@*
@*******************************************************************************
@*/
@void ih264e_sixtap_filter_2dvh_vert(UWORD8 *pu1_src,
@                                UWORD8 *pu1_dst1,
@                                UWORD8 *pu1_dst2,
@                                WORD32 src_strd,
@                                WORD32 dst_strd,
@                                WORD32 *pi16_pred1,/* Pointer to 16bit intermmediate buffer (used only in c)*/
@                                WORD32 pi16_pred1_strd)




    .global ih264e_sixtap_filter_2dvh_vert_a9q

ih264e_sixtap_filter_2dvh_vert_a9q:
    stmfd         sp!, {r10, r11, r12, lr}

@//r0 - pu1_ref
@//r3 - u4_ref_width
    vpush         {d8-d15}
    @// Load six rows for vertical interpolation
    lsl           r12, r3, #1
    sub           r0, r0, r12
    sub           r0, r0, #2
    vld1.8        {d2, d3, d4}, [r0], r3
    vld1.8        {d5, d6, d7}, [r0], r3
    vld1.8        {d8, d9, d10}, [r0], r3
    mov           r12, #5
    vld1.8        {d11, d12, d13}, [r0], r3
    mov           r14, #20
    vld1.8        {d14, d15, d16}, [r0], r3
    vmov.16       d0[0], r12
    vmov.16       d0[1], r14
    vld1.8        {d17, d18, d19}, [r0], r3
    vmov.i8       d1, #20

@// r12 - u2_buff1_width
@// r14 - u2_buff2_width
    ldr           r12, [sp, #80]
    add           r11, r1, #6

    mov           r14, r12

    mov           r10, #3               @loop counter


filter_2dvh_loop:

    @// ////////////// ROW 1 ///////////////////////

@// Process first vertical interpolated row
@// each column is
    vaddl.u8      q10, d2, d17          @// a0 + a5                             (column1,row0)
    vmov.i8       d31, #5
    vmlal.u8      q10, d8, d1           @// a0 + a5 + 20a2                      (column1,row0)
    vmlal.u8      q10, d11, d1          @// a0 + a5 + 20a2 + 20a3               (column1,row0)
    vmlsl.u8      q10, d5, d31          @// a0 + a5 + 20a2 + 20a3 - 5a1         (column1,row0)
    vmlsl.u8      q10, d14, d31         @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (column1,row0)


    vaddl.u8      q11, d3, d18          @// a0 + a5                             (column2,row0)
    vmlal.u8      q11, d9, d1           @// a0 + a5 + 20a2                      (column2,row0)
    vmlal.u8      q11, d12, d1          @// a0 + a5 + 20a2 + 20a3               (column2,row0)
    vmlsl.u8      q11, d6, d31          @// a0 + a5 + 20a2 + 20a3 - 5a1         (column2,row0)
    vmlsl.u8      q11, d15, d31         @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (column2,row0)
    vext.16       d30, d20, d21, #2     @//extract a[2]                         (set1)

    vaddl.u8      q12, d4, d19          @// a0 + a5                             (column3,row0)
    vext.16       d29, d20, d21, #3     @//extract a[3]                         (set1)
    vmlal.u8      q12, d10, d1          @// a0 + a5 + 20a2                      (column3,row0)
    vmlal.u8      q12, d13, d1          @// a0 + a5 + 20a2 + 20a3               (column3,row0)
    vmlsl.u8      q12, d7, d31          @// a0 + a5 + 20a2 + 20a3 - 5a1         (column3,row0)
    vmlsl.u8      q12, d16, d31         @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (column3,row0)

    vqrshrun.s16  d2, q10, #5           @// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5   (column1,row0)
    vext.16       d31, d21, d22, #1     @//extract a[5]                         (set1)
    vqrshrun.s16  d3, q11, #5           @// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5   (column2,row0)
    vext.16       d28, d20, d21, #1     @//extract a[1]                         (set1)

    vaddl.s16     q13, d31, d20         @// a0 + a5                             (set1)
    vext.16       d31, d22, d23, #1     @//extract a[5]                         (set2)
    vmlal.s16     q13, d30, d0[1]       @// a0 + a5 + 20a2                      (set1)
    vmlal.s16     q13, d29, d0[1]       @// a0 + a5 + 20a2 + 20a3               (set1)
    vmlsl.s16     q13, d28, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1         (set1)
    vmlsl.s16     q13, d21, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set1)
    vext.16       d30, d21, d22, #2     @//extract a[2]                         (set2)

    vqrshrun.s16  d4, q12, #5           @// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5   (column3,row0)
    vext.16       d29, d21, d22, #3     @//extract a[3]                         (set2)

    vext.16       d28, d21, d22, #1     @//extract a[1]                         (set2)
    vaddl.s16     q10, d31, d21         @// a0 + a5                             (set2)
    vmlal.s16     q10, d30, d0[1]       @// a0 + a5 + 20a2                      (set2)
    vmlal.s16     q10, d29, d0[1]       @// a0 + a5 + 20a2 + 20a3               (set2)
    vmlsl.s16     q10, d28, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1         (set2)
    vmlsl.s16     q10, d22, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set2)
    vext.16       d31, d23, d24, #1     @//extract a[5]                         (set3)

    vext.8        d2, d2, d3, #2
    vst1.8        {d3, d4}, [r11], r12  @// store row1 - 1,1/2 grid
    vst1.8        {d2}, [r1], r12       @// store row1 - 1,1/2 grid

    vext.16       d30, d22, d23, #2     @//extract a[2]                         (set3)
    vext.16       d29, d22, d23, #3     @//extract a[3]                         (set3)

    vaddl.s16     q1, d31, d22          @// a0 + a5                             (set3)
    vext.16       d28, d22, d23, #1     @//extract a[1]                         (set3)
    vmlal.s16     q1, d30, d0[1]        @// a0 + a5 + 20a2                      (set3)
    vmlal.s16     q1, d29, d0[1]        @// a0 + a5 + 20a2 + 20a3               (set3)
    vmlsl.s16     q1, d28, d0[0]        @// a0 + a5 + 20a2 + 20a3 - 5a1         (set3)
    vmlsl.s16     q1, d23, d0[0]        @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set3)
    vext.16       d31, d24, d25, #1     @//extract a[5]                         (set4)

    vshrn.s32     d21, q10, #8          @// shift by 8 and later we will shift by 2 more with rounding  (set2)
    vext.16       d30, d23, d24, #2     @//extract a[2]                         (set4)
    vshrn.s32     d20, q13, #8          @// shift by 8 and later we will shift by 2 more with rounding  (set1)
    vext.16       d29, d23, d24, #3     @//extract a[3]                         (set4)

    vaddl.s16     q13, d31, d23         @// a0 + a5                             (set4)
    vext.16       d28, d23, d24, #1     @//extract a[1]                         (set4)
    vext.16       d31, d25, d25, #1     @//extract a[5]                         (set5) ;//here only first element in the row is valid
    vmlal.s16     q13, d30, d0[1]       @// a0 + a5 + 20a2                      (set4)
    vmlal.s16     q13, d29, d0[1]       @// a0 + a5 + 20a2 + 20a3               (set4)
    vmlsl.s16     q13, d28, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1         (set4)
    vmlsl.s16     q13, d24, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set4)
    vext.16       d30, d24, d25, #2     @//extract a[2]                         (set5)

    vaddl.s16     q11, d31, d24         @// a0 + a5                             (set5)
    vext.16       d29, d24, d25, #3     @//extract a[3]                         (set5)

    vext.16       d31, d24, d25, #1     @//extract a[1]                         (set5)
    vshrn.s32     d28, q1, #8           @// shift by 8 and later we will shift by 2 more with rounding  (set3)

    vld1.8        {d2, d3, d4}, [r0], r3 @// Load next Row data
    vmlal.s16     q11, d30, d0[1]       @// a0 + a5 + 20a2                      (set5)
    vmlal.s16     q11, d29, d0[1]       @// a0 + a5 + 20a2 + 20a3               (set5)
    vmlsl.s16     q11, d31, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1         (set5)
    vmlsl.s16     q11, d25, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set5)
    vshrn.s32     d29, q13, #8          @// shift by 8 and later we will shift by 2 more with rounding  (set4)
    vqrshrun.s16  d26, q10, #2          @// half,half gird set1,2


    @//VQRSHRUN.s16 D27,Q14,#2          ;// half,half gird set3,4
    @//VSHRN.s32        D28,Q11,#8          ;// shift by 8 and later we will shift by 2 more with rounding  (set5)

    @//VQRSHRUN.s16 D28,Q14,#2          ;// half,half gird set5

    @//VST1.8       {D26,D27,D28},[r2],r14  ;// store 1/2,1,2 grif values
    @// ////////////// ROW 2 ///////////////////////

@// Process first vertical interpolated row
@// each column is
    vaddl.u8      q10, d5, d2           @// a0 + a5                             (column1,row0)
    vmov.i8       d31, #5
    vmlal.u8      q10, d11, d1          @// a0 + a5 + 20a2                      (column1,row0)
    vmlal.u8      q10, d14, d1          @// a0 + a5 + 20a2 + 20a3               (column1,row0)
    vmlsl.u8      q10, d8, d31          @// a0 + a5 + 20a2 + 20a3 - 5a1         (column1,row0)
    vmlsl.u8      q10, d17, d31         @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (column1,row0)

    vqrshrun.s16  d27, q14, #2          @// half,half gird set3,4
    vshrn.s32     d28, q11, #8          @// shift by 8 and later we will shift by 2 more with rounding  (set5)

    vaddl.u8      q11, d6, d3           @// a0 + a5                             (column2,row0)
    vmlal.u8      q11, d12, d1          @// a0 + a5 + 20a2                      (column2,row0)
    vmlal.u8      q11, d15, d1          @// a0 + a5 + 20a2 + 20a3               (column2,row0)
    vmlsl.u8      q11, d9, d31          @// a0 + a5 + 20a2 + 20a3 - 5a1         (column2,row0)
    vmlsl.u8      q11, d18, d31         @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (column2,row0)

    vqrshrun.s16  d28, q14, #2          @// half,half gird set5
    vext.16       d30, d20, d21, #2     @//extract a[2]                         (set1)

    vaddl.u8      q12, d7, d4           @// a0 + a5                             (column3,row0)
    vext.16       d29, d20, d21, #3     @//extract a[3]                         (set1)
    vmlal.u8      q12, d13, d1          @// a0 + a5 + 20a2                      (column3,row0)
    vmlal.u8      q12, d16, d1          @// a0 + a5 + 20a2 + 20a3               (column3,row0)
    vmlsl.u8      q12, d10, d31         @// a0 + a5 + 20a2 + 20a3 - 5a1         (column3,row0)
    vmlsl.u8      q12, d19, d31         @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (column3,row0)
    vst1.8        {d26, d27, d28}, [r2], r14 @// store 1/2,1,2 grif values

    vqrshrun.s16  d5, q10, #5           @// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5   (column1,row0)
    vext.16       d31, d21, d22, #1     @//extract a[5]                         (set1)
    vqrshrun.s16  d6, q11, #5           @// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5   (column2,row0)
    vext.16       d28, d20, d21, #1     @//extract a[1]                         (set1)

    vaddl.s16     q13, d31, d20         @// a0 + a5                             (set1)
    vext.16       d31, d22, d23, #1     @//extract a[5]                         (set2)
    vmlal.s16     q13, d30, d0[1]       @// a0 + a5 + 20a2                      (set1)
    vmlal.s16     q13, d29, d0[1]       @// a0 + a5 + 20a2 + 20a3               (set1)
    vmlsl.s16     q13, d28, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1         (set1)
    vmlsl.s16     q13, d21, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set1)
    vext.16       d30, d21, d22, #2     @//extract a[2]                         (set2)

    vqrshrun.s16  d7, q12, #5           @// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5   (column3,row0)
    vext.16       d29, d21, d22, #3     @//extract a[3]                         (set2)

    vext.16       d28, d21, d22, #1     @//extract a[1]                         (set2)
    vaddl.s16     q10, d31, d21         @// a0 + a5                             (set2)
    vmlal.s16     q10, d30, d0[1]       @// a0 + a5 + 20a2                      (set2)
    vmlal.s16     q10, d29, d0[1]       @// a0 + a5 + 20a2 + 20a3               (set2)
    vmlsl.s16     q10, d28, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1         (set2)
    vmlsl.s16     q10, d22, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set2)
    vext.16       d31, d23, d24, #1     @//extract a[5]                         (set3)

    vext.8        d5, d5, d6, #2
    vst1.8        {d6, d7}, [r11], r12  @// store row1 - 1,1/2 grid
    vst1.8        {d5}, [r1], r12       @// store row1 - 1,1/2 grid

    vext.16       d30, d22, d23, #2     @//extract a[2]                         (set3)
    vext.16       d29, d22, d23, #3     @//extract a[3]                         (set3)

    vaddl.s16     q3, d31, d22          @// a0 + a5                             (set3)
    vext.16       d28, d22, d23, #1     @//extract a[1]                         (set3)
    vmlal.s16     q3, d30, d0[1]        @// a0 + a5 + 20a2                      (set3)
    vmlal.s16     q3, d29, d0[1]        @// a0 + a5 + 20a2 + 20a3               (set3)
    vmlsl.s16     q3, d28, d0[0]        @// a0 + a5 + 20a2 + 20a3 - 5a1         (set3)
    vmlsl.s16     q3, d23, d0[0]        @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set3)
    vext.16       d31, d24, d25, #1     @//extract a[5]                         (set4)

    vshrn.s32     d21, q10, #8          @// shift by 8 and later we will shift by 2 more with rounding  (set2)
    vext.16       d30, d23, d24, #2     @//extract a[2]                         (set4)
    vshrn.s32     d20, q13, #8          @// shift by 8 and later we will shift by 2 more with rounding  (set1)
    vext.16       d29, d23, d24, #3     @//extract a[3]                         (set4)

    vaddl.s16     q13, d31, d23         @// a0 + a5                             (set4)
    vext.16       d28, d23, d24, #1     @//extract a[1]                         (set4)
    vext.16       d31, d25, d25, #1     @//extract a[5]                         (set5) ;//here only first element in the row is valid
    vmlal.s16     q13, d30, d0[1]       @// a0 + a5 + 20a2                      (set4)
    vmlal.s16     q13, d29, d0[1]       @// a0 + a5 + 20a2 + 20a3               (set4)
    vmlsl.s16     q13, d28, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1         (set4)
    vmlsl.s16     q13, d24, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set4)
    vext.16       d30, d24, d25, #2     @//extract a[2]                         (set5)

    vaddl.s16     q11, d31, d24         @// a0 + a5                             (set5)
    vext.16       d29, d24, d25, #3     @//extract a[3]                         (set5)

    vext.16       d31, d24, d25, #1     @//extract a[1]                         (set5)
    vshrn.s32     d28, q3, #8           @// shift by 8 and later we will shift by 2 more with rounding  (set3)

    vld1.8        {d5, d6, d7}, [r0], r3 @// Load next Row data
    vmlal.s16     q11, d30, d0[1]       @// a0 + a5 + 20a2                      (set5)
    vmlal.s16     q11, d29, d0[1]       @// a0 + a5 + 20a2 + 20a3               (set5)
    vmlsl.s16     q11, d31, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1         (set5)
    vmlsl.s16     q11, d25, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set5)
    vshrn.s32     d29, q13, #8          @// shift by 8 and later we will shift by 2 more with rounding  (set4)
    vqrshrun.s16  d26, q10, #2          @// half,half gird set1,2


    @//VQRSHRUN.s16 D27,Q14,#2          ;// half,half gird set3,4
    @//VSHRN.s32        D28,Q11,#8          ;// shift by 8 and later we will shift by 2 more with rounding  (set5)

    @//VQRSHRUN.s16 D28,Q14,#2          ;// half,half gird set5

    @//VST1.8       {D26,D27,D28},[r2],r14  ;// store 1/2,1,2 grif values
    @// ////////////// ROW 3 ///////////////////////

@// Process first vertical interpolated row
@// each column is
    vaddl.u8      q10, d8, d5           @// a0 + a5                             (column1,row0)
    vmov.i8       d31, #5
    vmlal.u8      q10, d14, d1          @// a0 + a5 + 20a2                      (column1,row0)
    vmlal.u8      q10, d17, d1          @// a0 + a5 + 20a2 + 20a3               (column1,row0)
    vmlsl.u8      q10, d11, d31         @// a0 + a5 + 20a2 + 20a3 - 5a1         (column1,row0)
    vmlsl.u8      q10, d2, d31          @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (column1,row0)

    vqrshrun.s16  d27, q14, #2          @// half,half gird set3,4
    vshrn.s32     d28, q11, #8          @// shift by 8 and later we will shift by 2 more with rounding  (set5)

    vaddl.u8      q11, d9, d6           @// a0 + a5                             (column2,row0)
    vmlal.u8      q11, d15, d1          @// a0 + a5 + 20a2                      (column2,row0)
    vmlal.u8      q11, d18, d1          @// a0 + a5 + 20a2 + 20a3               (column2,row0)
    vmlsl.u8      q11, d12, d31         @// a0 + a5 + 20a2 + 20a3 - 5a1         (column2,row0)
    vmlsl.u8      q11, d3, d31          @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (column2,row0)

    vqrshrun.s16  d28, q14, #2          @// half,half gird set5
    vext.16       d30, d20, d21, #2     @//extract a[2]                         (set1)

    vaddl.u8      q12, d10, d7          @// a0 + a5                             (column3,row0)
    vext.16       d29, d20, d21, #3     @//extract a[3]                         (set1)
    vmlal.u8      q12, d16, d1          @// a0 + a5 + 20a2                      (column3,row0)
    vmlal.u8      q12, d19, d1          @// a0 + a5 + 20a2 + 20a3               (column3,row0)
    vmlsl.u8      q12, d13, d31         @// a0 + a5 + 20a2 + 20a3 - 5a1         (column3,row0)
    vmlsl.u8      q12, d4, d31          @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (column3,row0)

    vst1.8        {d26, d27, d28}, [r2], r14 @// store 1/2,1,2 grif values

    vqrshrun.s16  d8, q10, #5           @// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5   (column1,row0)
    vext.16       d31, d21, d22, #1     @//extract a[5]                         (set1)
    vqrshrun.s16  d9, q11, #5           @// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5   (column2,row0)
    vext.16       d28, d20, d21, #1     @//extract a[1]                         (set1)

    vaddl.s16     q13, d31, d20         @// a0 + a5                             (set1)
    vext.16       d31, d22, d23, #1     @//extract a[5]                         (set2)
    vmlal.s16     q13, d30, d0[1]       @// a0 + a5 + 20a2                      (set1)
    vmlal.s16     q13, d29, d0[1]       @// a0 + a5 + 20a2 + 20a3               (set1)
    vmlsl.s16     q13, d28, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1         (set1)
    vmlsl.s16     q13, d21, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set1)
    vext.16       d30, d21, d22, #2     @//extract a[2]                         (set2)

    vqrshrun.s16  d10, q12, #5          @// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5   (column3,row0)
    vext.16       d29, d21, d22, #3     @//extract a[3]                         (set2)

    vext.16       d28, d21, d22, #1     @//extract a[1]                         (set2)
    vaddl.s16     q10, d31, d21         @// a0 + a5                             (set2)
    vmlal.s16     q10, d30, d0[1]       @// a0 + a5 + 20a2                      (set2)
    vmlal.s16     q10, d29, d0[1]       @// a0 + a5 + 20a2 + 20a3               (set2)
    vmlsl.s16     q10, d28, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1         (set2)
    vmlsl.s16     q10, d22, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set2)
    vext.16       d31, d23, d24, #1     @//extract a[5]                         (set3)

    vext.8        d8, d8, d9, #2
    vst1.8        {d9, d10}, [r11], r12 @// store row1 - 1,1/2 grid
    vst1.8        {d8}, [r1], r12       @// store row1 - 1,1/2 grid

    vext.16       d30, d22, d23, #2     @//extract a[2]                         (set3)
    vext.16       d29, d22, d23, #3     @//extract a[3]                         (set3)

    vaddl.s16     q4, d31, d22          @// a0 + a5                             (set3)
    vext.16       d28, d22, d23, #1     @//extract a[1]                         (set3)
    vmlal.s16     q4, d30, d0[1]        @// a0 + a5 + 20a2                      (set3)
    vmlal.s16     q4, d29, d0[1]        @// a0 + a5 + 20a2 + 20a3               (set3)
    vmlsl.s16     q4, d28, d0[0]        @// a0 + a5 + 20a2 + 20a3 - 5a1         (set3)
    vmlsl.s16     q4, d23, d0[0]        @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set3)
    vext.16       d31, d24, d25, #1     @//extract a[5]                         (set4)

    vshrn.s32     d21, q10, #8          @// shift by 8 and later we will shift by 2 more with rounding  (set2)
    vext.16       d30, d23, d24, #2     @//extract a[2]                         (set4)
    vshrn.s32     d20, q13, #8          @// shift by 8 and later we will shift by 2 more with rounding  (set1)
    vext.16       d29, d23, d24, #3     @//extract a[3]                         (set4)

    vaddl.s16     q13, d31, d23         @// a0 + a5                             (set4)
    vext.16       d28, d23, d24, #1     @//extract a[1]                         (set4)
    vext.16       d31, d25, d25, #1     @//extract a[5]                         (set5) ;//here only first element in the row is valid
    vmlal.s16     q13, d30, d0[1]       @// a0 + a5 + 20a2                      (set4)
    vmlal.s16     q13, d29, d0[1]       @// a0 + a5 + 20a2 + 20a3               (set4)
    vmlsl.s16     q13, d28, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1         (set4)
    vmlsl.s16     q13, d24, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set4)
    vext.16       d30, d24, d25, #2     @//extract a[2]                         (set5)

    vaddl.s16     q11, d31, d24         @// a0 + a5                             (set5)
    vext.16       d29, d24, d25, #3     @//extract a[3]                         (set5)

    vext.16       d31, d24, d25, #1     @//extract a[1]                         (set5)
    vshrn.s32     d28, q4, #8           @// shift by 8 and later we will shift by 2 more with rounding  (set3)

    vld1.8        {d8, d9, d10}, [r0], r3 @// Load next Row data
    vmlal.s16     q11, d30, d0[1]       @// a0 + a5 + 20a2                      (set5)
    vmlal.s16     q11, d29, d0[1]       @// a0 + a5 + 20a2 + 20a3               (set5)
    vmlsl.s16     q11, d31, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1         (set5)
    vmlsl.s16     q11, d25, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set5)
    vshrn.s32     d29, q13, #8          @// shift by 8 and later we will shift by 2 more with rounding  (set4)
    vqrshrun.s16  d26, q10, #2          @// half,half gird set1,2


    @//VQRSHRUN.s16 D27,Q14,#2          ;// half,half gird set3,4
    @//VSHRN.s32        D28,Q11,#8          ;// shift by 8 and later we will shift by 2 more with rounding  (set5)

    @//VQRSHRUN.s16 D28,Q14,#2          ;// half,half gird set5

    @//VST1.8       {D26,D27,D28},[r2],r14  ;// store 1/2,1,2 grif values
    @// ////////////// ROW 4 ///////////////////////

@// Process first vertical interpolated row
@// each column is
    vaddl.u8      q10, d11, d8          @// a0 + a5                             (column1,row0)
    vmov.i8       d31, #5
    vmlal.u8      q10, d17, d1          @// a0 + a5 + 20a2                      (column1,row0)
    vmlal.u8      q10, d2, d1           @// a0 + a5 + 20a2 + 20a3               (column1,row0)
    vmlsl.u8      q10, d14, d31         @// a0 + a5 + 20a2 + 20a3 - 5a1         (column1,row0)
    vmlsl.u8      q10, d5, d31          @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (column1,row0)

    vqrshrun.s16  d27, q14, #2          @// half,half gird set3,4
    vshrn.s32     d28, q11, #8          @// shift by 8 and later we will shift by 2 more with rounding  (set5)

    vaddl.u8      q11, d12, d9          @// a0 + a5                             (column2,row0)
    vmlal.u8      q11, d18, d1          @// a0 + a5 + 20a2                      (column2,row0)
    vmlal.u8      q11, d3, d1           @// a0 + a5 + 20a2 + 20a3               (column2,row0)
    vmlsl.u8      q11, d15, d31         @// a0 + a5 + 20a2 + 20a3 - 5a1         (column2,row0)
    vmlsl.u8      q11, d6, d31          @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (column2,row0)

    vqrshrun.s16  d28, q14, #2          @// half,half gird set5
    vext.16       d30, d20, d21, #2     @//extract a[2]                         (set1)

    vaddl.u8      q12, d13, d10         @// a0 + a5                             (column3,row0)
    vext.16       d29, d20, d21, #3     @//extract a[3]                         (set1)
    vmlal.u8      q12, d19, d1          @// a0 + a5 + 20a2                      (column3,row0)
    vmlal.u8      q12, d4, d1           @// a0 + a5 + 20a2 + 20a3               (column3,row0)
    vmlsl.u8      q12, d16, d31         @// a0 + a5 + 20a2 + 20a3 - 5a1         (column3,row0)
    vmlsl.u8      q12, d7, d31          @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (column3,row0)

    vst1.8        {d26, d27, d28}, [r2], r14 @// store 1/2,1,2 grif values

    vqrshrun.s16  d11, q10, #5          @// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5   (column1,row0)
    vext.16       d31, d21, d22, #1     @//extract a[5]                         (set1)
    vqrshrun.s16  d12, q11, #5          @// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5   (column2,row0)
    vext.16       d28, d20, d21, #1     @//extract a[1]                         (set1)

    vaddl.s16     q13, d31, d20         @// a0 + a5                             (set1)
    vext.16       d31, d22, d23, #1     @//extract a[5]                         (set2)
    vmlal.s16     q13, d30, d0[1]       @// a0 + a5 + 20a2                      (set1)
    vmlal.s16     q13, d29, d0[1]       @// a0 + a5 + 20a2 + 20a3               (set1)
    vmlsl.s16     q13, d28, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1         (set1)
    vmlsl.s16     q13, d21, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set1)
    vext.16       d30, d21, d22, #2     @//extract a[2]                         (set2)

    vqrshrun.s16  d13, q12, #5          @// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5   (column3,row0)
    vext.16       d29, d21, d22, #3     @//extract a[3]                         (set2)

    vext.16       d28, d21, d22, #1     @//extract a[1]                         (set2)
    vaddl.s16     q10, d31, d21         @// a0 + a5                             (set2)
    vmlal.s16     q10, d30, d0[1]       @// a0 + a5 + 20a2                      (set2)
    vmlal.s16     q10, d29, d0[1]       @// a0 + a5 + 20a2 + 20a3               (set2)
    vmlsl.s16     q10, d28, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1         (set2)
    vmlsl.s16     q10, d22, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set2)
    vext.16       d31, d23, d24, #1     @//extract a[5]                         (set3)

    vext.8        d11, d11, d12, #2
    vst1.8        {d12, d13}, [r11], r12 @// store row1 - 1,1/2 grid
    vst1.8        {d11}, [r1], r12      @// store row1 - 1,1/2 grid

    vext.16       d30, d22, d23, #2     @//extract a[2]                         (set3)
    vext.16       d29, d22, d23, #3     @//extract a[3]                         (set3)

    vaddl.s16     q6, d31, d22          @// a0 + a5                             (set3)
    vext.16       d28, d22, d23, #1     @//extract a[1]                         (set3)
    vmlal.s16     q6, d30, d0[1]        @// a0 + a5 + 20a2                      (set3)
    vmlal.s16     q6, d29, d0[1]        @// a0 + a5 + 20a2 + 20a3               (set3)
    vmlsl.s16     q6, d28, d0[0]        @// a0 + a5 + 20a2 + 20a3 - 5a1         (set3)
    vmlsl.s16     q6, d23, d0[0]        @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set3)
    vext.16       d31, d24, d25, #1     @//extract a[5]                         (set4)

    vshrn.s32     d21, q10, #8          @// shift by 8 and later we will shift by 2 more with rounding  (set2)
    vext.16       d30, d23, d24, #2     @//extract a[2]                         (set4)
    vshrn.s32     d20, q13, #8          @// shift by 8 and later we will shift by 2 more with rounding  (set1)
    vext.16       d29, d23, d24, #3     @//extract a[3]                         (set4)

    vaddl.s16     q13, d31, d23         @// a0 + a5                             (set4)
    vext.16       d28, d23, d24, #1     @//extract a[1]                         (set4)
    vext.16       d31, d25, d25, #1     @//extract a[5]                         (set5) ;//here only first element in the row is valid
    vmlal.s16     q13, d30, d0[1]       @// a0 + a5 + 20a2                      (set4)
    vmlal.s16     q13, d29, d0[1]       @// a0 + a5 + 20a2 + 20a3               (set4)
    vmlsl.s16     q13, d28, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1         (set4)
    vmlsl.s16     q13, d24, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set4)
    vext.16       d30, d24, d25, #2     @//extract a[2]                         (set5)

    vaddl.s16     q11, d31, d24         @// a0 + a5                             (set5)
    vext.16       d29, d24, d25, #3     @//extract a[3]                         (set5)

    vext.16       d31, d24, d25, #1     @//extract a[1]                         (set5)
    vshrn.s32     d28, q6, #8           @// shift by 8 and later we will shift by 2 more with rounding  (set3)

    vld1.8        {d11, d12, d13}, [r0], r3 @// Load next Row data
    vmlal.s16     q11, d30, d0[1]       @// a0 + a5 + 20a2                      (set5)
    vmlal.s16     q11, d29, d0[1]       @// a0 + a5 + 20a2 + 20a3               (set5)
    vmlsl.s16     q11, d31, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1         (set5)
    vmlsl.s16     q11, d25, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set5)
    vshrn.s32     d29, q13, #8          @// shift by 8 and later we will shift by 2 more with rounding  (set4)
    vqrshrun.s16  d26, q10, #2          @// half,half gird set1,2


    @//VQRSHRUN.s16 D27,Q14,#2          ;// half,half gird set3,4
    @//VSHRN.s32        D28,Q11,#8          ;// shift by 8 and later we will shift by 2 more with rounding  (set5)

    @//VQRSHRUN.s16 D28,Q14,#2          ;// half,half gird set5

    @//VST1.8       {D26,D27,D28},[r2],r14  ;// store 1/2,1,2 grif values
    @// ////////////// ROW 5 ///////////////////////

@// Process first vertical interpolated row
@// each column is
    vaddl.u8      q10, d14, d11         @// a0 + a5                             (column1,row0)
    vmov.i8       d31, #5
    vmlal.u8      q10, d2, d1           @// a0 + a5 + 20a2                      (column1,row0)
    vmlal.u8      q10, d5, d1           @// a0 + a5 + 20a2 + 20a3               (column1,row0)
    vmlsl.u8      q10, d17, d31         @// a0 + a5 + 20a2 + 20a3 - 5a1         (column1,row0)
    vmlsl.u8      q10, d8, d31          @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (column1,row0)

    vqrshrun.s16  d27, q14, #2          @// half,half gird set3,4
    vshrn.s32     d28, q11, #8          @// shift by 8 and later we will shift by 2 more with rounding  (set5)

    vaddl.u8      q11, d15, d12         @// a0 + a5                             (column2,row0)
    vmlal.u8      q11, d3, d1           @// a0 + a5 + 20a2                      (column2,row0)
    vmlal.u8      q11, d6, d1           @// a0 + a5 + 20a2 + 20a3               (column2,row0)
    vmlsl.u8      q11, d18, d31         @// a0 + a5 + 20a2 + 20a3 - 5a1         (column2,row0)
    vmlsl.u8      q11, d9, d31          @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (column2,row0)

    vqrshrun.s16  d28, q14, #2          @// half,half gird set5
    vext.16       d30, d20, d21, #2     @//extract a[2]                         (set1)

    vaddl.u8      q12, d16, d13         @// a0 + a5                             (column3,row0)
    vext.16       d29, d20, d21, #3     @//extract a[3]                         (set1)
    vmlal.u8      q12, d4, d1           @// a0 + a5 + 20a2                      (column3,row0)
    vmlal.u8      q12, d7, d1           @// a0 + a5 + 20a2 + 20a3               (column3,row0)
    vmlsl.u8      q12, d19, d31         @// a0 + a5 + 20a2 + 20a3 - 5a1         (column3,row0)
    vmlsl.u8      q12, d10, d31         @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (column3,row0)

    vst1.8        {d26, d27, d28}, [r2], r14 @// store 1/2,1,2 grif values

    vqrshrun.s16  d14, q10, #5          @// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5   (column1,row0)
    vext.16       d31, d21, d22, #1     @//extract a[5]                         (set1)
    vqrshrun.s16  d15, q11, #5          @// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5   (column2,row0)
    vext.16       d28, d20, d21, #1     @//extract a[1]                         (set1)

    vaddl.s16     q13, d31, d20         @// a0 + a5                             (set1)
    vext.16       d31, d22, d23, #1     @//extract a[5]                         (set2)
    vmlal.s16     q13, d30, d0[1]       @// a0 + a5 + 20a2                      (set1)
    vmlal.s16     q13, d29, d0[1]       @// a0 + a5 + 20a2 + 20a3               (set1)
    vmlsl.s16     q13, d28, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1         (set1)
    vmlsl.s16     q13, d21, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set1)
    vext.16       d30, d21, d22, #2     @//extract a[2]                         (set2)

    vqrshrun.s16  d16, q12, #5          @// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5   (column3,row0)
    vext.16       d29, d21, d22, #3     @//extract a[3]                         (set2)

    vext.16       d28, d21, d22, #1     @//extract a[1]                         (set2)
    vaddl.s16     q10, d31, d21         @// a0 + a5                             (set2)
    vmlal.s16     q10, d30, d0[1]       @// a0 + a5 + 20a2                      (set2)
    vmlal.s16     q10, d29, d0[1]       @// a0 + a5 + 20a2 + 20a3               (set2)
    vmlsl.s16     q10, d28, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1         (set2)
    vmlsl.s16     q10, d22, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set2)
    vext.16       d31, d23, d24, #1     @//extract a[5]                         (set3)

    vext.8        d14, d14, d15, #2
    vst1.8        {d15, d16}, [r11], r12 @// store row1 - 1,1/2 grid
    vst1.8        {d14}, [r1], r12      @// store row1 - 1,1/2 grid

    vext.16       d30, d22, d23, #2     @//extract a[2]                         (set3)
    vext.16       d29, d22, d23, #3     @//extract a[3]                         (set3)

    vaddl.s16     q7, d31, d22          @// a0 + a5                             (set3)
    vext.16       d28, d22, d23, #1     @//extract a[1]                         (set3)
    vmlal.s16     q7, d30, d0[1]        @// a0 + a5 + 20a2                      (set3)
    vmlal.s16     q7, d29, d0[1]        @// a0 + a5 + 20a2 + 20a3               (set3)
    vmlsl.s16     q7, d28, d0[0]        @// a0 + a5 + 20a2 + 20a3 - 5a1         (set3)
    vmlsl.s16     q7, d23, d0[0]        @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set3)
    vext.16       d31, d24, d25, #1     @//extract a[5]                         (set4)

    vshrn.s32     d21, q10, #8          @// shift by 8 and later we will shift by 2 more with rounding  (set2)
    vext.16       d30, d23, d24, #2     @//extract a[2]                         (set4)
    vshrn.s32     d20, q13, #8          @// shift by 8 and later we will shift by 2 more with rounding  (set1)
    vext.16       d29, d23, d24, #3     @//extract a[3]                         (set4)

    vaddl.s16     q13, d31, d23         @// a0 + a5                             (set4)
    vext.16       d28, d23, d24, #1     @//extract a[1]                         (set4)
    vext.16       d31, d25, d25, #1     @//extract a[5]                         (set5) ;//here only first element in the row is valid
    vmlal.s16     q13, d30, d0[1]       @// a0 + a5 + 20a2                      (set4)
    vmlal.s16     q13, d29, d0[1]       @// a0 + a5 + 20a2 + 20a3               (set4)
    vmlsl.s16     q13, d28, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1         (set4)
    vmlsl.s16     q13, d24, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set4)
    vext.16       d30, d24, d25, #2     @//extract a[2]                         (set5)

    vaddl.s16     q11, d31, d24         @// a0 + a5                             (set5)
    vext.16       d29, d24, d25, #3     @//extract a[3]                         (set5)

    vext.16       d31, d24, d25, #1     @//extract a[1]                         (set5)
    vshrn.s32     d28, q7, #8           @// shift by 8 and later we will shift by 2 more with rounding  (set3)

    vld1.8        {d14, d15, d16}, [r0], r3 @// Load next Row data
    vmlal.s16     q11, d30, d0[1]       @// a0 + a5 + 20a2                      (set5)
    vmlal.s16     q11, d29, d0[1]       @// a0 + a5 + 20a2 + 20a3               (set5)
    vmlsl.s16     q11, d31, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1         (set5)
    vmlsl.s16     q11, d25, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set5)
    vshrn.s32     d29, q13, #8          @// shift by 8 and later we will shift by 2 more with rounding  (set4)
    vqrshrun.s16  d26, q10, #2          @// half,half gird set1,2


    @//VQRSHRUN.s16 D27,Q14,#2          ;// half,half gird set3,4
    @//VSHRN.s32        D28,Q11,#8          ;// shift by 8 and later we will shift by 2 more with rounding  (set5)

    @//VQRSHRUN.s16 D28,Q14,#2          ;// half,half gird set5

    @//VST1.8       {D26,D27,D28},[r2],r14  ;// store 1/2,1,2 grif values
    @// ////////////// ROW 6 ///////////////////////

@// Process first vertical interpolated row
@// each column is

    cmp           r10, #1               @// if it 17 rows are complete skip
    beq           filter_2dvh_skip_row
    vaddl.u8      q10, d17, d14         @// a0 + a5                             (column1,row0)
    vmov.i8       d31, #5
    vmlal.u8      q10, d5, d1           @// a0 + a5 + 20a2                      (column1,row0)
    vmlal.u8      q10, d8, d1           @// a0 + a5 + 20a2 + 20a3               (column1,row0)
    vmlsl.u8      q10, d2, d31          @// a0 + a5 + 20a2 + 20a3 - 5a1         (column1,row0)
    vmlsl.u8      q10, d11, d31         @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (column1,row0)

    vqrshrun.s16  d27, q14, #2          @// half,half gird set3,4
    vshrn.s32     d28, q11, #8          @// shift by 8 and later we will shift by 2 more with rounding  (set5)

    vaddl.u8      q11, d18, d15         @// a0 + a5                             (column2,row0)
    vmlal.u8      q11, d6, d1           @// a0 + a5 + 20a2                      (column2,row0)
    vmlal.u8      q11, d9, d1           @// a0 + a5 + 20a2 + 20a3               (column2,row0)
    vmlsl.u8      q11, d3, d31          @// a0 + a5 + 20a2 + 20a3 - 5a1         (column2,row0)
    vmlsl.u8      q11, d12, d31         @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (column2,row0)

    vqrshrun.s16  d28, q14, #2          @// half,half gird set5
    vext.16       d30, d20, d21, #2     @//extract a[2]                         (set1)

    vaddl.u8      q12, d19, d16         @// a0 + a5                             (column3,row0)
    vext.16       d29, d20, d21, #3     @//extract a[3]                         (set1)
    vmlal.u8      q12, d7, d1           @// a0 + a5 + 20a2                      (column3,row0)
    vmlal.u8      q12, d10, d1          @// a0 + a5 + 20a2 + 20a3               (column3,row0)
    vmlsl.u8      q12, d4, d31          @// a0 + a5 + 20a2 + 20a3 - 5a1         (column3,row0)
    vmlsl.u8      q12, d13, d31         @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (column3,row0)

    vst1.8        {d26, d27, d28}, [r2], r14 @// store 1/2,1,2 grif values

    vqrshrun.s16  d17, q10, #5          @// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5   (column1,row0)
    vext.16       d31, d21, d22, #1     @//extract a[5]                         (set1)
    vqrshrun.s16  d18, q11, #5          @// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5   (column2,row0)
    vext.16       d28, d20, d21, #1     @//extract a[1]                         (set1)

    vaddl.s16     q13, d31, d20         @// a0 + a5                             (set1)
    vext.16       d31, d22, d23, #1     @//extract a[5]                         (set2)
    vmlal.s16     q13, d30, d0[1]       @// a0 + a5 + 20a2                      (set1)
    vmlal.s16     q13, d29, d0[1]       @// a0 + a5 + 20a2 + 20a3               (set1)
    vmlsl.s16     q13, d28, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1         (set1)
    vmlsl.s16     q13, d21, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set1)
    vext.16       d30, d21, d22, #2     @//extract a[2]                         (set2)

    vqrshrun.s16  d19, q12, #5          @// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5   (column3,row0)
    vext.16       d29, d21, d22, #3     @//extract a[3]                         (set2)

    vext.16       d28, d21, d22, #1     @//extract a[1]                         (set2)
    vaddl.s16     q10, d31, d21         @// a0 + a5                             (set2)
    vmlal.s16     q10, d30, d0[1]       @// a0 + a5 + 20a2                      (set2)
    vmlal.s16     q10, d29, d0[1]       @// a0 + a5 + 20a2 + 20a3               (set2)
    vmlsl.s16     q10, d28, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1         (set2)
    vmlsl.s16     q10, d22, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set2)
    vext.16       d31, d23, d24, #1     @//extract a[5]                         (set3)

    vext.8        d17, d17, d18, #2
    vst1.8        {d18, d19}, [r11], r12 @// store row1 - 1,1/2 grid
    vst1.8        {d17}, [r1], r12      @// store row1 - 1,1/2 grid

    vext.16       d30, d22, d23, #2     @//extract a[2]                         (set3)
    vext.16       d29, d22, d23, #3     @//extract a[3]                         (set3)

    vaddl.s16     q9, d31, d22          @// a0 + a5                             (set3)
    vext.16       d28, d22, d23, #1     @//extract a[1]                         (set3)
    vmlal.s16     q9, d30, d0[1]        @// a0 + a5 + 20a2                      (set3)
    vmlal.s16     q9, d29, d0[1]        @// a0 + a5 + 20a2 + 20a3               (set3)
    vmlsl.s16     q9, d28, d0[0]        @// a0 + a5 + 20a2 + 20a3 - 5a1         (set3)
    vmlsl.s16     q9, d23, d0[0]        @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set3)
    vext.16       d31, d24, d25, #1     @//extract a[5]                         (set4)

    vshrn.s32     d21, q10, #8          @// shift by 8 and later we will shift by 2 more with rounding  (set2)
    vext.16       d30, d23, d24, #2     @//extract a[2]                         (set4)
    vshrn.s32     d20, q13, #8          @// shift by 8 and later we will shift by 2 more with rounding  (set1)
    vext.16       d29, d23, d24, #3     @//extract a[3]                         (set4)

    vaddl.s16     q13, d31, d23         @// a0 + a5                             (set4)
    vext.16       d28, d23, d24, #1     @//extract a[1]                         (set4)
    vext.16       d31, d25, d25, #1     @//extract a[5]                         (set5) ;//here only first element in the row is valid
    vmlal.s16     q13, d30, d0[1]       @// a0 + a5 + 20a2                      (set4)
    vmlal.s16     q13, d29, d0[1]       @// a0 + a5 + 20a2 + 20a3               (set4)
    vmlsl.s16     q13, d28, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1         (set4)
    vmlsl.s16     q13, d24, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set4)
    vext.16       d30, d24, d25, #2     @//extract a[2]                         (set5)

    vaddl.s16     q11, d31, d24         @// a0 + a5                             (set5)
    vext.16       d29, d24, d25, #3     @//extract a[3]                         (set5)

    vext.16       d31, d24, d25, #1     @//extract a[1]                         (set5)
    vshrn.s32     d28, q9, #8           @// shift by 8 and later we will shift by 2 more with rounding  (set3)

    vld1.8        {d17, d18, d19}, [r0], r3 @// Load next Row data
    vmlal.s16     q11, d30, d0[1]       @// a0 + a5 + 20a2                      (set5)
    vmlal.s16     q11, d29, d0[1]       @// a0 + a5 + 20a2 + 20a3               (set5)
    vmlsl.s16     q11, d31, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1         (set5)
    vmlsl.s16     q11, d25, d0[0]       @// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4   (set5)
    vshrn.s32     d29, q13, #8          @// shift by 8 and later we will shift by 2 more with rounding  (set4)
    vqrshrun.s16  d26, q10, #2          @// half,half gird set1,2


    vqrshrun.s16  d27, q14, #2          @// half,half gird set3,4
    vshrn.s32     d28, q11, #8          @// shift by 8 and later we will shift by 2 more with rounding  (set5)

    vqrshrun.s16  d28, q14, #2          @// half,half gird set5

    vst1.8        {d26, d27, d28}, [r2], r14 @// store 1/2,1,2 grif values

    subs          r10, r10, #1          @//decrement loop counter

    bne           filter_2dvh_loop


@// Process first vertical interpolated row
@// each column is
    @// ////////////// ROW 13 ///////////////////////

@// Process first vertical interpolated row
@// each column is
    vpop          {d8-d15}
    ldmfd         sp!, {r10, r11, r12, pc}

filter_2dvh_skip_row:

    vqrshrun.s16  d27, q14, #2          @// half,half gird set3,4
    vshrn.s32     d28, q11, #8          @// shift by 8 and later we will shift by 2 more with rounding  (set5)

    vqrshrun.s16  d28, q14, #2          @// half,half gird set5

    vst1.8        {d26, d27, d28}, [r2], r14 @// store 1/2,1,2 grif values
    vpop          {d8-d15}
    ldmfd         sp!, {r10, r11, r12, pc}




