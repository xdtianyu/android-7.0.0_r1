//******************************************************************************
//*
//* Copyright (C) 2015 The Android Open Source Project
//*
//* Licensed under the Apache License, Version 2.0 (the "License");
//* you may not use this file except in compliance with the License.
//* You may obtain a copy of the License at:
//*
//* http://www.apache.org/licenses/LICENSE-2.0
//*
//* Unless required by applicable law or agreed to in writing, software
//* distributed under the License is distributed on an "AS IS" BASIS,
//* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//* See the License for the specific language governing permissions and
//* limitations under the License.
//*
//*****************************************************************************
//* Originally developed and contributed by Ittiam Systems Pvt. Ltd, Bangalore
//*/
///**
// *******************************************************************************
// * @file
// *  ih264e_half_pel.s
// *
// * @brief
// *
// *
// * @author
// *  Ittiam
// *
// * @par List of Functions:
// *  ih264e_sixtapfilter_horz
// *  ih264e_sixtap_filter_2dvh_vert
//
// *
// * @remarks
// *  None
// *
// *******************************************************************************
// */


.text
.p2align 2
.include "ih264_neon_macros.s"

///*******************************************************************************
//*
//* @brief
//*     Interprediction luma filter for horizontal input(Filter run for width = 17 and height =16)
//*
//* @par Description:
//*    Applies a 6 tap horizontal filter .The output is  clipped to 8 bits
//*    sec 8.4.2.2.1 titled "Luma sample interpolation process"
//*
//* @param[in] pu1_src
//*  UWORD8 pointer to the source
//*
//* @param[out] pu1_dst
//*  UWORD8 pointer to the destination
//*
//* @param[in] src_strd
//*  integer source stride
//*
//* @param[in] dst_strd
//*  integer destination stride
//*
//*
//* @returns
//*
//* @remarks
//*  None
//*
//*******************************************************************************
//*/
//void ih264e_sixtapfilter_horz(UWORD8 *pu1_src,
//                                UWORD8 *pu1_dst,
//                                WORD32 src_strd,
//                                WORD32 dst_strd);


.equ halfpel_width ,  17 + 1            //( make it even, two rows are processed at a time)


        .global ih264e_sixtapfilter_horz_av8
ih264e_sixtapfilter_horz_av8:
    // STMFD sp!,{x14}
    push_v_regs
    stp       x19, x20, [sp, #-16]!

    movi      v0.8b, #5
    sub       x0, x0, #2
    sub       x3, x3, #16
    movi      v1.8b, #20
    mov       x14, #16

filter_horz_loop:


    ld1       {v2.8b, v3.8b, v4.8b}, [x0], x2 //// Load row0
    ld1       {v5.8b, v6.8b, v7.8b}, [x0], x2 //// Load row1

    //// Processing row0 and row1

    ext       v31.8b, v2.8b , v3.8b , #5
    ext       v30.8b, v3.8b , v4.8b , #5

    uaddl     v8.8h, v31.8b, v2.8b      //// a0 + a5                             (column1,row0)
    ext       v29.8b, v4.8b , v4.8b , #5
    uaddl     v10.8h, v30.8b, v3.8b     //// a0 + a5                             (column2,row0)
    ext       v28.8b, v5.8b , v6.8b , #5
    uaddl     v12.8h, v29.8b, v4.8b     //// a0 + a5                             (column3,row0)
    ext       v27.8b, v6.8b , v7.8b , #5
    uaddl     v14.8h, v28.8b, v5.8b     //// a0 + a5                             (column1,row1)
    ext       v26.8b, v7.8b , v7.8b , #5

    uaddl     v16.8h, v27.8b, v6.8b     //// a0 + a5                             (column2,row1)
    ext       v31.8b, v2.8b , v3.8b , #2
    uaddl     v18.8h, v26.8b, v7.8b     //// a0 + a5                             (column3,row1)
    ext       v30.8b, v3.8b , v4.8b , #2
    umlal     v8.8h, v31.8b, v1.8b      //// a0 + a5 + 20a2                         (column1,row0)
    ext       v29.8b, v4.8b , v4.8b , #2
    umlal     v10.8h, v30.8b, v1.8b     //// a0 + a5 + 20a2                         (column2,row0)
    ext       v28.8b, v5.8b , v6.8b , #2
    umlal     v12.8h, v29.8b, v1.8b     //// a0 + a5 + 20a2                         (column3,row0)
    ext       v27.8b, v6.8b , v7.8b , #2
    umlal     v14.8h, v28.8b, v1.8b     //// a0 + a5 + 20a2                         (column1,row1)
    ext       v26.8b, v7.8b , v7.8b , #2

    umlal     v16.8h, v27.8b, v1.8b     //// a0 + a5 + 20a2                         (column2,row1)
    ext       v31.8b, v2.8b , v3.8b , #3
    umlal     v18.8h, v26.8b, v1.8b     //// a0 + a5 + 20a2                         (column3,row1)
    ext       v30.8b, v3.8b , v4.8b , #3
    umlal     v8.8h, v31.8b, v1.8b      //// a0 + a5 + 20a2 + 20a3                  (column1,row0)
    ext       v29.8b, v4.8b , v4.8b , #3
    umlal     v10.8h, v30.8b, v1.8b     //// a0 + a5 + 20a2 + 20a3                  (column2,row0)
    ext       v28.8b, v5.8b , v6.8b , #3
    umlal     v12.8h, v29.8b, v1.8b     //// a0 + a5 + 20a2 + 20a3                  (column3,row0)
    ext       v27.8b, v6.8b , v7.8b , #3
    umlal     v14.8h, v28.8b, v1.8b     //// a0 + a5 + 20a2 + 20a3                  (column1,row1)
    ext       v26.8b, v7.8b , v7.8b , #3

    umlal     v16.8h, v27.8b, v1.8b     //// a0 + a5 + 20a2 + 20a3                  (column2,row1)
    ext       v31.8b, v2.8b , v3.8b , #1
    umlal     v18.8h, v26.8b, v1.8b     //// a0 + a5 + 20a2 + 20a3                  (column3,row1)
    ext       v30.8b, v3.8b , v4.8b , #1
    umlsl     v8.8h, v31.8b, v0.8b      //// a0 + a5 + 20a2 + 20a3 - 5a1           (column1,row0)
    ext       v29.8b, v4.8b , v4.8b , #1
    umlsl     v10.8h, v30.8b, v0.8b     //// a0 + a5 + 20a2 + 20a3 - 5a1           (column2,row0)
    ext       v28.8b, v5.8b , v6.8b , #1
    umlsl     v12.8h, v29.8b, v0.8b     //// a0 + a5 + 20a2 + 20a3 - 5a1           (column3,row0)
    ext       v27.8b, v6.8b , v7.8b , #1
    umlsl     v14.8h, v28.8b, v0.8b     //// a0 + a5 + 20a2 + 20a3 - 5a1           (column1,row1)
    ext       v26.8b, v7.8b , v7.8b , #1

    umlsl     v16.8h, v27.8b, v0.8b     //// a0 + a5 + 20a2 + 20a3 - 5a1           (column2,row1)
    ext       v31.8b, v2.8b , v3.8b , #4
    umlsl     v18.8h, v26.8b, v0.8b     //// a0 + a5 + 20a2 + 20a3 - 5a1           (column3,row1)
    ext       v30.8b, v3.8b , v4.8b , #4
    umlsl     v8.8h, v31.8b, v0.8b      //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4     (column1,row0)
    ext       v29.8b, v4.8b , v4.8b , #4
    umlsl     v10.8h, v30.8b, v0.8b     //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4     (column2,row0)
    ext       v28.8b, v5.8b , v6.8b , #4
    umlsl     v12.8h, v29.8b, v0.8b     //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4     (column3,row0)
    ext       v27.8b, v6.8b , v7.8b , #4
    umlsl     v14.8h, v28.8b, v0.8b     //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4     (column1,row1)
    ext       v26.8b, v7.8b , v7.8b , #4

    umlsl     v16.8h, v27.8b, v0.8b     //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4     (column2,row1)
    umlsl     v18.8h, v26.8b, v0.8b     //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4     (column3,row1)

    sqrshrun  v20.8b, v8.8h, #5         //// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5    (column1,row0)
    sqrshrun  v21.8b, v10.8h, #5        //// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5    (column2,row0)
    sqrshrun  v22.8b, v12.8h, #5        //// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5    (column3,row0)
    sqrshrun  v23.8b, v14.8h, #5        //// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5    (column1,row1)
    sqrshrun  v24.8b, v16.8h, #5        //// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5    (column2,row1)
    sqrshrun  v25.8b, v18.8h, #5        //// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5    (column3,row1)

    st1       {v20.8b, v21.8b}, [x1], #16 ////Store dest row0
    st1       {v22.h}[0], [x1], x3
    st1       {v23.8b, v24.8b}, [x1], #16 ////Store dest row1
    st1       {v25.h}[0], [x1], x3

    subs      x14, x14, #2              //    decrement counter

    bne       filter_horz_loop


    // LDMFD sp!,{pc}
    ldp       x19, x20, [sp], #16
    pop_v_regs
    ret









///**
//*******************************************************************************
//*
//* @brief
//*   This function implements a two stage cascaded six tap filter. It
//*    applies the six tap filter in the vertical direction on the
//*    predictor values, followed by applying the same filter in the
//*    horizontal direction on the output of the first stage. The six tap
//*    filtering operation is described in sec 8.4.2.2.1 titled "Luma sample
//*    interpolation process"
//*    (Filter run for width = 17 and height =17)
//* @par Description:
//*    The function interpolates
//*    the predictors first in the vertical direction and then in the
//*    horizontal direction to output the (1/2,1/2). The output of the first
//*    stage of the filter is stored in the buffer pointed to by pi16_pred1(only in C)
//*    in 16 bit precision.
//*
//*
//* @param[in] pu1_src
//*  UWORD8 pointer to the source
//*
//* @param[out] pu1_dst1
//*  UWORD8 pointer to the destination(vertical filtered output)
//*
//* @param[out] pu1_dst2
//*  UWORD8 pointer to the destination(out put after applying horizontal filter to the intermediate vertical output)
//*
//* @param[in] src_strd
//*  integer source stride
//*
//* @param[in] dst_strd
//*  integer destination stride of pu1_dst
//*
//* @param[in]pi16_pred1
//*  Pointer to 16bit intermediate buffer(used only in c)
//*
//* @param[in] pi16_pred1_strd
//*  integer destination stride of pi16_pred1
//*
//*
//* @returns
//*
//* @remarks
//*  None
//*
//*******************************************************************************
//*/
//void ih264e_sixtap_filter_2dvh_vert(UWORD8 *pu1_src,
//                                UWORD8 *pu1_dst1,
//                                UWORD8 *pu1_dst2,
//                                WORD32 src_strd,
//                                WORD32 dst_strd,
//                                WORD32 *pi16_pred1,/* Pointer to 16bit intermmediate buffer (used only in c)*/
//                                WORD32 pi16_pred1_strd)




        .global ih264e_sixtap_filter_2dvh_vert_av8

ih264e_sixtap_filter_2dvh_vert_av8:
    // STMFD sp!,{x10,x11,x12,x14}
    push_v_regs
    stp       x19, x20, [sp, #-16]!

////x0 - pu1_ref
////x3 - u4_ref_width

    //// Load six rows for vertical interpolation
    lsl       x12, x3, #1
    sub       x0, x0, x12
    sub       x0, x0, #2
    ld1       {v2.8b, v3.8b, v4.8b}, [x0], x3
    ld1       {v5.8b, v6.8b, v7.8b}, [x0], x3
    ld1       {v8.8b, v9.8b, v10.8b}, [x0], x3
    mov       x12, #5
    ld1       {v11.8b, v12.8b, v13.8b}, [x0], x3
    mov       x14, #20
    ld1       {v14.8b, v15.8b, v16.8b}, [x0], x3
    mov       v0.h[0], w12
    mov       v0.h[1], w14
    ld1       {v17.8b, v18.8b, v19.8b}, [x0], x3
    movi      v1.8b, #20

//// x12 - u2_buff1_width
//// x14 - u2_buff2_width
    mov       x12, x4
    add       x11, x1, #16

    mov       x14, x12

    mov       x10, #3 //loop counter
    sub       x16 , x12, #8
    sub       x19, x14, #16
filter_2dvh_loop:

    //// ////////////// ROW 1 ///////////////////////

//// Process first vertical interpolated row
//// each column is
    uaddl     v20.8h, v2.8b, v17.8b     //// a0 + a5                             (column1,row0)
    movi      v31.8b, #5
    umlal     v20.8h, v8.8b, v1.8b      //// a0 + a5 + 20a2                         (column1,row0)
    umlal     v20.8h, v11.8b, v1.8b     //// a0 + a5 + 20a2 + 20a3                  (column1,row0)
    umlsl     v20.8h, v5.8b, v31.8b     //// a0 + a5 + 20a2 + 20a3 - 5a1           (column1,row0)
    umlsl     v20.8h, v14.8b, v31.8b    //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4     (column1,row0)
    mov       v21.d[0], v20.d[1]

    uaddl     v22.8h, v3.8b, v18.8b     //// a0 + a5                                (column2,row0)
    umlal     v22.8h, v9.8b, v1.8b      //// a0 + a5 + 20a2                        (column2,row0)
    umlal     v22.8h, v12.8b, v1.8b     //// a0 + a5 + 20a2 + 20a3                (column2,row0)
    umlsl     v22.8h, v6.8b, v31.8b     //// a0 + a5 + 20a2 + 20a3 - 5a1            (column2,row0)
    umlsl     v22.8h, v15.8b, v31.8b    //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (column2,row0)
    ext       v30.8b, v20.8b , v21.8b , #4
    mov       v23.d[0], v22.d[1]


    uaddl     v24.8h, v4.8b, v19.8b     //// a0 + a5                                (column3,row0)
    ext       v29.8b, v20.8b , v21.8b , #6
    umlal     v24.8h, v10.8b, v1.8b     //// a0 + a5 + 20a2                        (column3,row0)
    umlal     v24.8h, v13.8b, v1.8b     //// a0 + a5 + 20a2 + 20a3                (column3,row0)
    umlsl     v24.8h, v7.8b, v31.8b     //// a0 + a5 + 20a2 + 20a3 - 5a1            (column3,row0)
    umlsl     v24.8h, v16.8b, v31.8b    //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (column3,row0)
    mov       v25.d[0], v24.d[1]

    sqrshrun  v2.8b, v20.8h, #5         //// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5    (column1,row0)
    ext       v31.8b, v21.8b , v22.8b , #2
    sqrshrun  v3.8b, v22.8h, #5         //// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5    (column2,row0)
    ext       v28.8b, v20.8b , v21.8b , #2

    saddl     v26.4s, v31.4h, v20.4h    //// a0 + a5                             (set1)
    ext       v31.8b, v22.8b , v23.8b , #2
    smlal     v26.4s, v30.4h, v0.h[1]   //// a0 + a5 + 20a2                         (set1)
    smlal     v26.4s, v29.4h, v0.h[1]   //// a0 + a5 + 20a2 + 20a3                  (set1)
    smlsl     v26.4s, v28.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1          (set1)
    smlsl     v26.4s, v21.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set1)
    ext       v30.8b, v21.8b , v22.8b , #4

    sqrshrun  v4.8b, v24.8h, #5         //// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5    (column3,row0)
    ext       v29.8b, v21.8b , v22.8b , #6

    ext       v28.8b, v21.8b , v22.8b , #2
    saddl     v20.4s, v31.4h, v21.4h    //// a0 + a5                             (set2)
    smlal     v20.4s, v30.4h, v0.h[1]   //// a0 + a5 + 20a2                         (set2)
    smlal     v20.4s, v29.4h, v0.h[1]   //// a0 + a5 + 20a2 + 20a3                  (set2)
    smlsl     v20.4s, v28.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1          (set2)
    smlsl     v20.4s, v22.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set2)
    ext       v31.8b, v23.8b , v24.8b , #2
    mov       v21.d[0], v20.d[1]
    ext       v2.8b, v2.8b , v3.8b , #2
    ext       v3.8b, v3.8b , v4.8b , #2
    ext       v4.8b, v4.8b , v4.8b , #2

    st1       {v2.8b, v3.8b}, [x1], x12 //// store row1 - 1,1/2 grid
    st1       {v4.h}[0], [x11], x12     //// store row1 - 1,1/2 grid

    ext       v30.8b, v22.8b , v23.8b , #4
    ext       v29.8b, v22.8b , v23.8b , #6

    saddl     v2.4s, v31.4h, v22.4h     //// a0 + a5                             (set3)
    ext       v28.8b, v22.8b , v23.8b , #2
    smlal     v2.4s, v30.4h, v0.h[1]    //// a0 + a5 + 20a2                         (set3)
    smlal     v2.4s, v29.4h, v0.h[1]    //// a0 + a5 + 20a2 + 20a3                  (set3)
    smlsl     v2.4s, v28.4h, v0.h[0]    //// a0 + a5 + 20a2 + 20a3 - 5a1          (set3)
    smlsl     v2.4s, v23.4h, v0.h[0]    //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set3)
    ext       v31.8b, v24.8b , v25.8b , #2

    shrn      v21.4h, v20.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set2)
    ext       v30.8b, v23.8b , v24.8b , #4
    shrn      v20.4h, v26.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set1)
    ext       v29.8b, v23.8b , v24.8b , #6

    saddl     v26.4s, v31.4h, v23.4h    //// a0 + a5                             (set4)
    ext       v28.8b, v23.8b , v24.8b , #2
    ext       v31.8b, v25.8b , v25.8b , #2
    smlal     v26.4s, v30.4h, v0.h[1]   //// a0 + a5 + 20a2                         (set4)
    smlal     v26.4s, v29.4h, v0.h[1]   //// a0 + a5 + 20a2 + 20a3                  (set4)
    smlsl     v26.4s, v28.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1          (set4)
    smlsl     v26.4s, v24.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set4)
    ext       v30.8b, v24.8b , v25.8b , #4

    saddl     v22.4s, v31.4h, v24.4h    //// a0 + a5                             (set5)
    ext       v29.8b, v24.8b , v25.8b , #6

    ext       v31.8b, v24.8b , v25.8b , #2
    shrn      v28.4h, v2.4s, #8         //// shift by 8 and later we will shift by 2 more with rounding     (set3)

    ld1       {v2.8b, v3.8b, v4.8b}, [x0], x3 //// Load next Row data
    smlal     v22.4s, v30.4h, v0.h[1]   //// a0 + a5 + 20a2                         (set5)
    smlal     v22.4s, v29.4h, v0.h[1]   //// a0 + a5 + 20a2 + 20a3                  (set5)
    smlsl     v22.4s, v31.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1          (set5)
    smlsl     v22.4s, v25.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set5)
    shrn      v29.4h, v26.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set4)
    mov       v20.d[1], v21.d[0]
    sqrshrun  v26.8b, v20.8h, #2        //// half,half gird set1,2


    ////VQRSHRUN.s16    D27,Q14,#2            ;// half,half gird set3,4
    ////VSHRN.s32        D28,Q11,#8            ;// shift by 8 and later we will shift by 2 more with rounding     (set5)

    ////VQRSHRUN.s16    D28,Q14,#2            ;// half,half gird set5

    ////VST1.8        {D26,D27,D28},[x2],x14    ;// store 1/2,1,2 grif values
    //// ////////////// ROW 2 ///////////////////////

//// Process first vertical interpolated row
//// each column is
    uaddl     v20.8h, v5.8b, v2.8b      //// a0 + a5                             (column1,row0)
    movi      v31.8b, #5
    umlal     v20.8h, v11.8b, v1.8b     //// a0 + a5 + 20a2                         (column1,row0)
    umlal     v20.8h, v14.8b, v1.8b     //// a0 + a5 + 20a2 + 20a3                  (column1,row0)
    umlsl     v20.8h, v8.8b, v31.8b     //// a0 + a5 + 20a2 + 20a3 - 5a1           (column1,row0)
    umlsl     v20.8h, v17.8b, v31.8b    //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4     (column1,row0)
    mov       v21.d[0], v20.d[1]

    mov       v28.d[1], v29.d[0]
    sqrshrun  v27.8b, v28.8h, #2        //// half,half gird set3,4

    shrn      v28.4h, v22.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set5)

    uaddl     v22.8h, v6.8b, v3.8b      //// a0 + a5                                (column2,row0)
    umlal     v22.8h, v12.8b, v1.8b     //// a0 + a5 + 20a2                        (column2,row0)
    umlal     v22.8h, v15.8b, v1.8b     //// a0 + a5 + 20a2 + 20a3                (column2,row0)
    umlsl     v22.8h, v9.8b, v31.8b     //// a0 + a5 + 20a2 + 20a3 - 5a1            (column2,row0)
    umlsl     v22.8h, v18.8b, v31.8b    //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (column2,row0)
    mov       v23.d[0], v22.d[1]

    sqrshrun  v28.8b, v28.8h, #2        //// half,half gird set5
    ext       v30.8b, v20.8b , v21.8b , #4

    uaddl     v24.8h, v7.8b, v4.8b      //// a0 + a5                                (column3,row0)
    ext       v29.8b, v20.8b , v21.8b , #6
    umlal     v24.8h, v13.8b, v1.8b     //// a0 + a5 + 20a2                        (column3,row0)
    umlal     v24.8h, v16.8b, v1.8b     //// a0 + a5 + 20a2 + 20a3                (column3,row0)
    umlsl     v24.8h, v10.8b, v31.8b    //// a0 + a5 + 20a2 + 20a3 - 5a1            (column3,row0)
    umlsl     v24.8h, v19.8b, v31.8b    //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (column3,row0)
    mov       v25.d[0], v24.d[1]

    st1       {v26.8b, v27.8b}, [x2], #16 //// store 1/2,1,2 grif values
    st1       {v28.h}[0], [x2], x19     //// store 1/2,1,2 grif values

    sqrshrun  v5.8b, v20.8h, #5         //// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5    (column1,row0)
    ext       v31.8b, v21.8b , v22.8b , #2
    sqrshrun  v6.8b, v22.8h, #5         //// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5    (column2,row0)
    ext       v28.8b, v20.8b , v21.8b , #2

    saddl     v26.4s, v31.4h, v20.4h    //// a0 + a5                             (set1)
    ext       v31.8b, v22.8b , v23.8b , #2
    smlal     v26.4s, v30.4h, v0.h[1]   //// a0 + a5 + 20a2                         (set1)
    smlal     v26.4s, v29.4h, v0.h[1]   //// a0 + a5 + 20a2 + 20a3                  (set1)
    smlsl     v26.4s, v28.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1          (set1)
    smlsl     v26.4s, v21.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set1)
    ext       v30.8b, v21.8b , v22.8b , #4

    sqrshrun  v7.8b, v24.8h, #5         //// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5    (column3,row0)
    ext       v29.8b, v21.8b , v22.8b , #6

    ext       v28.8b, v21.8b , v22.8b , #2
    saddl     v20.4s, v31.4h, v21.4h    //// a0 + a5                             (set2)
    smlal     v20.4s, v30.4h, v0.h[1]   //// a0 + a5 + 20a2                         (set2)
    smlal     v20.4s, v29.4h, v0.h[1]   //// a0 + a5 + 20a2 + 20a3                  (set2)
    smlsl     v20.4s, v28.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1          (set2)
    smlsl     v20.4s, v22.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set2)
    ext       v31.8b, v23.8b , v24.8b , #2

    ext       v5.8b, v5.8b , v6.8b , #2
    ext       v6.8b, v6.8b , v7.8b , #2
    ext       v7.8b, v7.8b , v7.8b , #2

    st1       {v5.8b, v6.8b}, [x1], x12 //// store row1 - 1,1/2 grid
    st1       {v7.h}[0], [x11], x12     //// store row1 - 1,1/2 grid

    ext       v30.8b, v22.8b , v23.8b , #4
    ext       v29.8b, v22.8b , v23.8b , #6

    saddl     v6.4s, v31.4h, v22.4h     //// a0 + a5                             (set3)
    ext       v28.8b, v22.8b , v23.8b , #2
    smlal     v6.4s, v30.4h, v0.h[1]    //// a0 + a5 + 20a2                         (set3)
    smlal     v6.4s, v29.4h, v0.h[1]    //// a0 + a5 + 20a2 + 20a3                  (set3)
    smlsl     v6.4s, v28.4h, v0.h[0]    //// a0 + a5 + 20a2 + 20a3 - 5a1          (set3)
    smlsl     v6.4s, v23.4h, v0.h[0]    //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set3)
    ext       v31.8b, v24.8b , v25.8b , #2

    shrn      v21.4h, v20.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set2)
    ext       v30.8b, v23.8b , v24.8b , #4
    shrn      v20.4h, v26.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set1)
    ext       v29.8b, v23.8b , v24.8b , #6

    saddl     v26.4s, v31.4h, v23.4h    //// a0 + a5                             (set4)
    ext       v28.8b, v23.8b , v24.8b , #2
    ext       v31.8b, v25.8b , v25.8b , #2
    smlal     v26.4s, v30.4h, v0.h[1]   //// a0 + a5 + 20a2                         (set4)
    smlal     v26.4s, v29.4h, v0.h[1]   //// a0 + a5 + 20a2 + 20a3                  (set4)
    smlsl     v26.4s, v28.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1          (set4)
    smlsl     v26.4s, v24.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set4)
    ext       v30.8b, v24.8b , v25.8b , #4

    saddl     v22.4s, v31.4h, v24.4h    //// a0 + a5                             (set5)
    ext       v29.8b, v24.8b , v25.8b , #6

    ext       v31.8b, v24.8b , v25.8b , #2
    shrn      v28.4h, v6.4s, #8         //// shift by 8 and later we will shift by 2 more with rounding     (set3)

    ld1       {v5.8b, v6.8b, v7.8b}, [x0], x3 //// Load next Row data
    smlal     v22.4s, v30.4h, v0.h[1]   //// a0 + a5 + 20a2                         (set5)
    smlal     v22.4s, v29.4h, v0.h[1]   //// a0 + a5 + 20a2 + 20a3                  (set5)
    smlsl     v22.4s, v31.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1          (set5)
    smlsl     v22.4s, v25.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set5)
    shrn      v29.4h, v26.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set4)
    mov       v20.d[1], v21.d[0]
    sqrshrun  v26.8b, v20.8h, #2        //// half,half gird set1,2


    ////VQRSHRUN.s16    D27,Q14,#2            ;// half,half gird set3,4
    ////VSHRN.s32        D28,Q11,#8            ;// shift by 8 and later we will shift by 2 more with rounding     (set5)

    ////VQRSHRUN.s16    D28,Q14,#2            ;// half,half gird set5

    ////VST1.8        {D26,D27,D28},[x2],x14    ;// store 1/2,1,2 grif values
    //// ////////////// ROW 3 ///////////////////////

//// Process first vertical interpolated row
//// each column is
    uaddl     v20.8h, v8.8b, v5.8b      //// a0 + a5                             (column1,row0)
    movi      v31.8b, #5
    umlal     v20.8h, v14.8b, v1.8b     //// a0 + a5 + 20a2                         (column1,row0)
    umlal     v20.8h, v17.8b, v1.8b     //// a0 + a5 + 20a2 + 20a3                  (column1,row0)
    umlsl     v20.8h, v11.8b, v31.8b    //// a0 + a5 + 20a2 + 20a3 - 5a1           (column1,row0)
    umlsl     v20.8h, v2.8b, v31.8b     //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4     (column1,row0)
    mov       v21.d[0], v20.d[1]

    mov       v28.d[1], v29.d[0]
    sqrshrun  v27.8b, v28.8h, #2        //// half,half gird set3,4
    shrn      v28.4h, v22.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set5)

    uaddl     v22.8h, v9.8b, v6.8b      //// a0 + a5                                (column2,row0)
    umlal     v22.8h, v15.8b, v1.8b     //// a0 + a5 + 20a2                        (column2,row0)
    umlal     v22.8h, v18.8b, v1.8b     //// a0 + a5 + 20a2 + 20a3                (column2,row0)
    umlsl     v22.8h, v12.8b, v31.8b    //// a0 + a5 + 20a2 + 20a3 - 5a1            (column2,row0)
    umlsl     v22.8h, v3.8b, v31.8b     //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (column2,row0)
    mov       v23.d[0], v22.d[1]

    sqrshrun  v28.8b, v28.8h, #2        //// half,half gird set5
    ext       v30.8b, v20.8b , v21.8b , #4

    uaddl     v24.8h, v10.8b, v7.8b     //// a0 + a5                                (column3,row0)
    ext       v29.8b, v20.8b , v21.8b , #6
    umlal     v24.8h, v16.8b, v1.8b     //// a0 + a5 + 20a2                        (column3,row0)
    umlal     v24.8h, v19.8b, v1.8b     //// a0 + a5 + 20a2 + 20a3                (column3,row0)
    umlsl     v24.8h, v13.8b, v31.8b    //// a0 + a5 + 20a2 + 20a3 - 5a1            (column3,row0)
    umlsl     v24.8h, v4.8b, v31.8b     //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (column3,row0)
    mov       v25.d[0], v24.d[1]

    st1       {v26.8b, v27.8b}, [x2], #16 //// store 1/2,1,2 grif values
    st1       { v28.h}[0], [x2], x19    //// store 1/2,1,2 grif values

    sqrshrun  v8.8b, v20.8h, #5         //// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5    (column1,row0)
    ext       v31.8b, v21.8b , v22.8b , #2
    sqrshrun  v9.8b, v22.8h, #5         //// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5    (column2,row0)
    ext       v28.8b, v20.8b , v21.8b , #2

    saddl     v26.4s, v31.4h, v20.4h    //// a0 + a5                             (set1)
    ext       v31.8b, v22.8b , v23.8b , #2
    smlal     v26.4s, v30.4h, v0.h[1]   //// a0 + a5 + 20a2                         (set1)
    smlal     v26.4s, v29.4h, v0.h[1]   //// a0 + a5 + 20a2 + 20a3                  (set1)
    smlsl     v26.4s, v28.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1          (set1)
    smlsl     v26.4s, v21.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set1)
    ext       v30.8b, v21.8b , v22.8b , #4

    sqrshrun  v10.8b, v24.8h, #5        //// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5    (column3,row0)
    ext       v29.8b, v21.8b , v22.8b , #6

    ext       v28.8b, v21.8b , v22.8b , #2
    saddl     v20.4s, v31.4h, v21.4h    //// a0 + a5                             (set2)
    smlal     v20.4s, v30.4h, v0.h[1]   //// a0 + a5 + 20a2                         (set2)
    smlal     v20.4s, v29.4h, v0.h[1]   //// a0 + a5 + 20a2 + 20a3                  (set2)
    smlsl     v20.4s, v28.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1          (set2)
    smlsl     v20.4s, v22.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set2)
    ext       v31.8b, v23.8b , v24.8b , #2

    ext       v8.8b, v8.8b , v9.8b , #2
    ext       v9.8b, v9.8b , v10.8b , #2
    ext       v10.8b, v10.8b , v10.8b , #2

    st1       {v8.8b, v9.8b}, [x1], x12 //// store row1 - 1,1/2 grid
    st1       {v10.h}[0], [x11], x12    //// store row1 - 1,1/2 grid

    ext       v30.8b, v22.8b , v23.8b , #4
    ext       v29.8b, v22.8b , v23.8b , #6

    saddl     v8.4s, v31.4h, v22.4h     //// a0 + a5                             (set3)
    ext       v28.8b, v22.8b , v23.8b , #2
    smlal     v8.4s, v30.4h, v0.h[1]    //// a0 + a5 + 20a2                         (set3)
    smlal     v8.4s, v29.4h, v0.h[1]    //// a0 + a5 + 20a2 + 20a3                  (set3)
    smlsl     v8.4s, v28.4h, v0.h[0]    //// a0 + a5 + 20a2 + 20a3 - 5a1          (set3)
    smlsl     v8.4s, v23.4h, v0.h[0]    //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set3)
    ext       v31.8b, v24.8b , v25.8b , #2

    shrn      v21.4h, v20.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set2)
    ext       v30.8b, v23.8b , v24.8b , #4
    shrn      v20.4h, v26.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set1)
    ext       v29.8b, v23.8b , v24.8b , #6

    saddl     v26.4s, v31.4h, v23.4h    //// a0 + a5                             (set4)
    ext       v28.8b, v23.8b , v24.8b , #2
    ext       v31.8b, v25.8b , v25.8b , #2
    smlal     v26.4s, v30.4h, v0.h[1]   //// a0 + a5 + 20a2                         (set4)
    smlal     v26.4s, v29.4h, v0.h[1]   //// a0 + a5 + 20a2 + 20a3                  (set4)
    smlsl     v26.4s, v28.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1          (set4)
    smlsl     v26.4s, v24.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set4)
    ext       v30.8b, v24.8b , v25.8b , #4

    saddl     v22.4s, v31.4h, v24.4h    //// a0 + a5                             (set5)
    ext       v29.8b, v24.8b , v25.8b , #6

    ext       v31.8b, v24.8b , v25.8b , #2
    shrn      v28.4h, v8.4s, #8         //// shift by 8 and later we will shift by 2 more with rounding     (set3)

    ld1       {v8.8b, v9.8b, v10.8b}, [x0], x3 //// Load next Row data
    smlal     v22.4s, v30.4h, v0.h[1]   //// a0 + a5 + 20a2                         (set5)
    smlal     v22.4s, v29.4h, v0.h[1]   //// a0 + a5 + 20a2 + 20a3                  (set5)
    smlsl     v22.4s, v31.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1          (set5)
    smlsl     v22.4s, v25.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set5)
    shrn      v29.4h, v26.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set4)
    mov       v20.d[1], v21.d[0]
    sqrshrun  v26.8b, v20.8h, #2        //// half,half gird set1,2


    ////VQRSHRUN.s16    D27,Q14,#2            ;// half,half gird set3,4
    ////VSHRN.s32        D28,Q11,#8            ;// shift by 8 and later we will shift by 2 more with rounding     (set5)

    ////VQRSHRUN.s16    D28,Q14,#2            ;// half,half gird set5

    ////VST1.8        {D26,D27,D28},[x2],x14    ;// store 1/2,1,2 grif values
    //// ////////////// ROW 4 ///////////////////////

//// Process first vertical interpolated row
//// each column is
    uaddl     v20.8h, v11.8b, v8.8b     //// a0 + a5                             (column1,row0)
    movi      v31.8b, #5
    umlal     v20.8h, v17.8b, v1.8b     //// a0 + a5 + 20a2                         (column1,row0)
    umlal     v20.8h, v2.8b, v1.8b      //// a0 + a5 + 20a2 + 20a3                  (column1,row0)
    umlsl     v20.8h, v14.8b, v31.8b    //// a0 + a5 + 20a2 + 20a3 - 5a1           (column1,row0)
    umlsl     v20.8h, v5.8b, v31.8b     //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4     (column1,row0)
    mov       v21.d[0], v20.d[1]
    mov       v28.d[1], v29.d[0]
    sqrshrun  v27.8b, v28.8h, #2        //// half,half gird set3,4
    shrn      v28.4h, v22.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set5)

    uaddl     v22.8h, v12.8b, v9.8b     //// a0 + a5                                (column2,row0)
    umlal     v22.8h, v18.8b, v1.8b     //// a0 + a5 + 20a2                        (column2,row0)
    umlal     v22.8h, v3.8b, v1.8b      //// a0 + a5 + 20a2 + 20a3                (column2,row0)
    umlsl     v22.8h, v15.8b, v31.8b    //// a0 + a5 + 20a2 + 20a3 - 5a1            (column2,row0)
    umlsl     v22.8h, v6.8b, v31.8b     //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (column2,row0)
    mov       v23.d[0], v22.d[1]

    sqrshrun  v28.8b, v28.8h, #2        //// half,half gird set5
    ext       v30.8b, v20.8b , v21.8b , #4

    uaddl     v24.8h, v13.8b, v10.8b    //// a0 + a5                                (column3,row0)
    ext       v29.8b, v20.8b , v21.8b , #6
    umlal     v24.8h, v19.8b, v1.8b     //// a0 + a5 + 20a2                        (column3,row0)
    umlal     v24.8h, v4.8b, v1.8b      //// a0 + a5 + 20a2 + 20a3                (column3,row0)
    umlsl     v24.8h, v16.8b, v31.8b    //// a0 + a5 + 20a2 + 20a3 - 5a1            (column3,row0)
    umlsl     v24.8h, v7.8b, v31.8b     //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (column3,row0)
    mov       v25.d[0], v24.d[1]

    st1       {v26.8b, v27.8b}, [x2], #16 //// store 1/2,1,2 grif values
    st1       {v28.h}[0], [x2], x19     //// store 1/2,1,2 grif values

    sqrshrun  v11.8b, v20.8h, #5        //// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5    (column1,row0)
    ext       v31.8b, v21.8b , v22.8b , #2
    sqrshrun  v12.8b, v22.8h, #5        //// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5    (column2,row0)
    ext       v28.8b, v20.8b , v21.8b , #2

    saddl     v26.4s, v31.4h, v20.4h    //// a0 + a5                             (set1)
    ext       v31.8b, v22.8b , v23.8b , #2
    smlal     v26.4s, v30.4h, v0.h[1]   //// a0 + a5 + 20a2                         (set1)
    smlal     v26.4s, v29.4h, v0.h[1]   //// a0 + a5 + 20a2 + 20a3                  (set1)
    smlsl     v26.4s, v28.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1          (set1)
    smlsl     v26.4s, v21.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set1)
    ext       v30.8b, v21.8b , v22.8b , #4

    sqrshrun  v13.8b, v24.8h, #5        //// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5    (column3,row0)
    ext       v29.8b, v21.8b , v22.8b , #6

    ext       v28.8b, v21.8b , v22.8b , #2
    saddl     v20.4s, v31.4h, v21.4h    //// a0 + a5                             (set2)
    smlal     v20.4s, v30.4h, v0.h[1]   //// a0 + a5 + 20a2                         (set2)
    smlal     v20.4s, v29.4h, v0.h[1]   //// a0 + a5 + 20a2 + 20a3                  (set2)
    smlsl     v20.4s, v28.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1          (set2)
    smlsl     v20.4s, v22.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set2)
    ext       v31.8b, v23.8b , v24.8b , #2

    ext       v11.8b, v11.8b , v12.8b , #2
    ext       v12.8b, v12.8b , v13.8b , #2
    ext       v13.8b, v13.8b , v13.8b , #2

    st1       {v11.8b, v12.8b}, [x1], x12 //// store row1 - 1,1/2 grid
    st1       {v13.h}[0], [x11], x12    //// store row1 - 1,1/2 grid

    ext       v30.8b, v22.8b , v23.8b , #4
    ext       v29.8b, v22.8b , v23.8b , #6

    saddl     v12.4s, v31.4h, v22.4h    //// a0 + a5                             (set3)
    ext       v28.8b, v22.8b , v23.8b , #2
    smlal     v12.4s, v30.4h, v0.h[1]   //// a0 + a5 + 20a2                         (set3)
    smlal     v12.4s, v29.4h, v0.h[1]   //// a0 + a5 + 20a2 + 20a3                  (set3)
    smlsl     v12.4s, v28.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1          (set3)
    smlsl     v12.4s, v23.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set3)
    ext       v31.8b, v24.8b , v25.8b , #2

    shrn      v21.4h, v20.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set2)
    ext       v30.8b, v23.8b , v24.8b , #4
    shrn      v20.4h, v26.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set1)
    ext       v29.8b, v23.8b , v24.8b , #6

    saddl     v26.4s, v31.4h, v23.4h    //// a0 + a5                             (set4)
    ext       v28.8b, v23.8b , v24.8b , #2
    ext       v31.8b, v25.8b , v25.8b , #2
    smlal     v26.4s, v30.4h, v0.h[1]   //// a0 + a5 + 20a2                         (set4)
    smlal     v26.4s, v29.4h, v0.h[1]   //// a0 + a5 + 20a2 + 20a3                  (set4)
    smlsl     v26.4s, v28.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1          (set4)
    smlsl     v26.4s, v24.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set4)
    ext       v30.8b, v24.8b , v25.8b , #4

    saddl     v22.4s, v31.4h, v24.4h    //// a0 + a5                             (set5)
    ext       v29.8b, v24.8b , v25.8b , #6

    ext       v31.8b, v24.8b , v25.8b , #2
    shrn      v28.4h, v12.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set3)

    ld1       {v11.8b, v12.8b, v13.8b}, [x0], x3 //// Load next Row data
    smlal     v22.4s, v30.4h, v0.h[1]   //// a0 + a5 + 20a2                         (set5)
    smlal     v22.4s, v29.4h, v0.h[1]   //// a0 + a5 + 20a2 + 20a3                  (set5)
    smlsl     v22.4s, v31.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1          (set5)
    smlsl     v22.4s, v25.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set5)
    shrn      v29.4h, v26.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set4)
    mov       v20.d[1], v21.d[0]
    sqrshrun  v26.8b, v20.8h, #2        //// half,half gird set1,2


    ////VQRSHRUN.s16    D27,Q14,#2            ;// half,half gird set3,4
    ////VSHRN.s32        D28,Q11,#8            ;// shift by 8 and later we will shift by 2 more with rounding     (set5)

    ////VQRSHRUN.s16    D28,Q14,#2            ;// half,half gird set5

    ////VST1.8        {D26,D27,D28},[x2],x14    ;// store 1/2,1,2 grif values
    //// ////////////// ROW 5 ///////////////////////

//// Process first vertical interpolated row
//// each column is
    uaddl     v20.8h, v14.8b, v11.8b    //// a0 + a5                             (column1,row0)
    movi      v31.8b, #5
    umlal     v20.8h, v2.8b, v1.8b      //// a0 + a5 + 20a2                         (column1,row0)
    umlal     v20.8h, v5.8b, v1.8b      //// a0 + a5 + 20a2 + 20a3                  (column1,row0)
    umlsl     v20.8h, v17.8b, v31.8b    //// a0 + a5 + 20a2 + 20a3 - 5a1           (column1,row0)
    umlsl     v20.8h, v8.8b, v31.8b     //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4     (column1,row0)
    mov       v21.d[0], v20.d[1]
    mov       v28.d[1], v29.d[0]
    sqrshrun  v27.8b, v28.8h, #2        //// half,half gird set3,4
    shrn      v28.4h, v22.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set5)

    uaddl     v22.8h, v15.8b, v12.8b    //// a0 + a5                                (column2,row0)
    umlal     v22.8h, v3.8b, v1.8b      //// a0 + a5 + 20a2                        (column2,row0)
    umlal     v22.8h, v6.8b, v1.8b      //// a0 + a5 + 20a2 + 20a3                (column2,row0)
    umlsl     v22.8h, v18.8b, v31.8b    //// a0 + a5 + 20a2 + 20a3 - 5a1            (column2,row0)
    umlsl     v22.8h, v9.8b, v31.8b     //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (column2,row0)
    mov       v23.d[0], v22.d[1]

    sqrshrun  v28.8b, v28.8h, #2        //// half,half gird set5
    ext       v30.8b, v20.8b , v21.8b , #4

    uaddl     v24.8h, v16.8b, v13.8b    //// a0 + a5                                (column3,row0)
    ext       v29.8b, v20.8b , v21.8b , #6
    umlal     v24.8h, v4.8b, v1.8b      //// a0 + a5 + 20a2                        (column3,row0)
    umlal     v24.8h, v7.8b, v1.8b      //// a0 + a5 + 20a2 + 20a3                (column3,row0)
    umlsl     v24.8h, v19.8b, v31.8b    //// a0 + a5 + 20a2 + 20a3 - 5a1            (column3,row0)
    umlsl     v24.8h, v10.8b, v31.8b    //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (column3,row0)
    mov       v25.d[0], v24.d[1]

    st1       {v26.8b, v27.8b}, [x2], #16 //// store 1/2,1,2 grif values
    st1       {v28.h}[0], [x2], x19     //// store 1/2,1,2 grif values

    sqrshrun  v14.8b, v20.8h, #5        //// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5    (column1,row0)
    ext       v31.8b, v21.8b , v22.8b , #2
    sqrshrun  v15.8b, v22.8h, #5        //// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5    (column2,row0)
    ext       v28.8b, v20.8b , v21.8b , #2

    saddl     v26.4s, v31.4h, v20.4h    //// a0 + a5                             (set1)
    ext       v31.8b, v22.8b , v23.8b , #2
    smlal     v26.4s, v30.4h, v0.h[1]   //// a0 + a5 + 20a2                         (set1)
    smlal     v26.4s, v29.4h, v0.h[1]   //// a0 + a5 + 20a2 + 20a3                  (set1)
    smlsl     v26.4s, v28.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1          (set1)
    smlsl     v26.4s, v21.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set1)
    ext       v30.8b, v21.8b , v22.8b , #4

    sqrshrun  v16.8b, v24.8h, #5        //// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5    (column3,row0)
    ext       v29.8b, v21.8b , v22.8b , #6

    ext       v28.8b, v21.8b , v22.8b , #2
    saddl     v20.4s, v31.4h, v21.4h    //// a0 + a5                             (set2)
    smlal     v20.4s, v30.4h, v0.h[1]   //// a0 + a5 + 20a2                         (set2)
    smlal     v20.4s, v29.4h, v0.h[1]   //// a0 + a5 + 20a2 + 20a3                  (set2)
    smlsl     v20.4s, v28.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1          (set2)
    smlsl     v20.4s, v22.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set2)
    ext       v31.8b, v23.8b , v24.8b , #2

    ext       v14.8b, v14.8b , v15.8b , #2
    ext       v15.8b, v15.8b , v16.8b , #2
    ext       v16.8b, v16.8b , v16.8b , #2

    st1       {v14.8b, v15.8b}, [x1], x12 //// store row1 - 1,1/2 grid
    st1       {v16.h}[0], [x11], x12    //// store row1 - 1,1/2 grid

    ext       v30.8b, v22.8b , v23.8b , #4
    ext       v29.8b, v22.8b , v23.8b , #6

    saddl     v14.4s, v31.4h, v22.4h    //// a0 + a5                             (set3)
    ext       v28.8b, v22.8b , v23.8b , #2
    smlal     v14.4s, v30.4h, v0.h[1]   //// a0 + a5 + 20a2                         (set3)
    smlal     v14.4s, v29.4h, v0.h[1]   //// a0 + a5 + 20a2 + 20a3                  (set3)
    smlsl     v14.4s, v28.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1          (set3)
    smlsl     v14.4s, v23.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set3)
    ext       v31.8b, v24.8b , v25.8b , #2

    shrn      v21.4h, v20.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set2)
    ext       v30.8b, v23.8b , v24.8b , #4
    shrn      v20.4h, v26.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set1)
    ext       v29.8b, v23.8b , v24.8b , #6

    saddl     v26.4s, v31.4h, v23.4h    //// a0 + a5                             (set4)
    ext       v28.8b, v23.8b , v24.8b , #2
    ext       v31.8b, v25.8b , v25.8b , #2
    smlal     v26.4s, v30.4h, v0.h[1]   //// a0 + a5 + 20a2                         (set4)
    smlal     v26.4s, v29.4h, v0.h[1]   //// a0 + a5 + 20a2 + 20a3                  (set4)
    smlsl     v26.4s, v28.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1          (set4)
    smlsl     v26.4s, v24.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set4)
    ext       v30.8b, v24.8b , v25.8b , #4

    saddl     v22.4s, v31.4h, v24.4h    //// a0 + a5                             (set5)
    ext       v29.8b, v24.8b , v25.8b , #6

    ext       v31.8b, v24.8b , v25.8b , #2
    shrn      v28.4h, v14.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set3)

    ld1       {v14.8b, v15.8b, v16.8b}, [x0], x3 //// Load next Row data
    smlal     v22.4s, v30.4h, v0.h[1]   //// a0 + a5 + 20a2                         (set5)
    smlal     v22.4s, v29.4h, v0.h[1]   //// a0 + a5 + 20a2 + 20a3                  (set5)
    smlsl     v22.4s, v31.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1          (set5)
    smlsl     v22.4s, v25.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set5)
    shrn      v29.4h, v26.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set4)
    mov       v20.d[1], v21.d[0]
    sqrshrun  v26.8b, v20.8h, #2        //// half,half gird set1,2


    ////VQRSHRUN.s16    D27,Q14,#2            ;// half,half gird set3,4
    ////VSHRN.s32        D28,Q11,#8            ;// shift by 8 and later we will shift by 2 more with rounding     (set5)

    ////VQRSHRUN.s16    D28,Q14,#2            ;// half,half gird set5

    ////VST1.8        {D26,D27,D28},[x2],x14    ;// store 1/2,1,2 grif values
    //// ////////////// ROW 6 ///////////////////////

//// Process first vertical interpolated row
//// each column is

    cmp       x10, #1                   //// if it 17 rows are complete skip
    beq       filter_2dvh_skip_row
    uaddl     v20.8h, v17.8b, v14.8b    //// a0 + a5                             (column1,row0)
    movi      v31.8b, #5
    umlal     v20.8h, v5.8b, v1.8b      //// a0 + a5 + 20a2                         (column1,row0)
    umlal     v20.8h, v8.8b, v1.8b      //// a0 + a5 + 20a2 + 20a3                  (column1,row0)
    umlsl     v20.8h, v2.8b, v31.8b     //// a0 + a5 + 20a2 + 20a3 - 5a1           (column1,row0)
    umlsl     v20.8h, v11.8b, v31.8b    //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4     (column1,row0)
    mov       v21.d[0], v20.d[1]
    mov       v28.d[1], v29.d[0]
    sqrshrun  v27.8b, v28.8h, #2        //// half,half gird set3,4
    shrn      v28.4h, v22.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set5)

    uaddl     v22.8h, v18.8b, v15.8b    //// a0 + a5                                (column2,row0)
    umlal     v22.8h, v6.8b, v1.8b      //// a0 + a5 + 20a2                        (column2,row0)
    umlal     v22.8h, v9.8b, v1.8b      //// a0 + a5 + 20a2 + 20a3                (column2,row0)
    umlsl     v22.8h, v3.8b, v31.8b     //// a0 + a5 + 20a2 + 20a3 - 5a1            (column2,row0)
    umlsl     v22.8h, v12.8b, v31.8b    //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (column2,row0)
    mov       v23.d[0], v22.d[1]

    sqrshrun  v28.8b, v28.8h, #2        //// half,half gird set5
    ext       v30.8b, v20.8b , v21.8b , #4

    uaddl     v24.8h, v19.8b, v16.8b    //// a0 + a5                                (column3,row0)
    ext       v29.8b, v20.8b , v21.8b , #6
    umlal     v24.8h, v7.8b, v1.8b      //// a0 + a5 + 20a2                        (column3,row0)
    umlal     v24.8h, v10.8b, v1.8b     //// a0 + a5 + 20a2 + 20a3                (column3,row0)
    umlsl     v24.8h, v4.8b, v31.8b     //// a0 + a5 + 20a2 + 20a3 - 5a1            (column3,row0)
    umlsl     v24.8h, v13.8b, v31.8b    //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (column3,row0)
    mov       v25.d[0], v24.d[1]

    st1       {v26.8b, v27.8b}, [x2], #16 //// store 1/2,1,2 grif values
    st1       {v28.h}[0], [x2], x19     //// store 1/2,1,2 grif values

    sqrshrun  v17.8b, v20.8h, #5        //// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5    (column1,row0)
    ext       v31.8b, v21.8b , v22.8b , #2
    sqrshrun  v18.8b, v22.8h, #5        //// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5    (column2,row0)
    ext       v28.8b, v20.8b , v21.8b , #2

    saddl     v26.4s, v31.4h, v20.4h    //// a0 + a5                             (set1)
    ext       v31.8b, v22.8b , v23.8b , #2
    smlal     v26.4s, v30.4h, v0.h[1]   //// a0 + a5 + 20a2                         (set1)
    smlal     v26.4s, v29.4h, v0.h[1]   //// a0 + a5 + 20a2 + 20a3                  (set1)
    smlsl     v26.4s, v28.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1          (set1)
    smlsl     v26.4s, v21.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set1)
    ext       v30.8b, v21.8b , v22.8b , #4

    sqrshrun  v19.8b, v24.8h, #5        //// (a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4 + 16) >> 5    (column3,row0)
    ext       v29.8b, v21.8b , v22.8b , #6

    ext       v28.8b, v21.8b , v22.8b , #2
    saddl     v20.4s, v31.4h, v21.4h    //// a0 + a5                             (set2)
    smlal     v20.4s, v30.4h, v0.h[1]   //// a0 + a5 + 20a2                         (set2)
    smlal     v20.4s, v29.4h, v0.h[1]   //// a0 + a5 + 20a2 + 20a3                  (set2)
    smlsl     v20.4s, v28.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1          (set2)
    smlsl     v20.4s, v22.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set2)
    ext       v31.8b, v23.8b , v24.8b , #2

    ext       v17.8b, v17.8b , v18.8b , #2
    ext       v18.8b, v18.8b , v19.8b , #2
    ext       v19.8b, v19.8b , v19.8b , #2

    st1       {v17.8b, v18.8b}, [x1], x12 //// store row1 - 1,1/2 grid
    st1       {v19.h}[0], [x11], x12    //// store row1 - 1,1/2 grid

    ext       v30.8b, v22.8b , v23.8b , #4
    ext       v29.8b, v22.8b , v23.8b , #6

    saddl     v18.4s, v31.4h, v22.4h    //// a0 + a5                             (set3)
    ext       v28.8b, v22.8b , v23.8b , #2
    smlal     v18.4s, v30.4h, v0.h[1]   //// a0 + a5 + 20a2                         (set3)
    smlal     v18.4s, v29.4h, v0.h[1]   //// a0 + a5 + 20a2 + 20a3                  (set3)
    smlsl     v18.4s, v28.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1          (set3)
    smlsl     v18.4s, v23.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set3)
    ext       v31.8b, v24.8b , v25.8b , #2

    shrn      v21.4h, v20.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set2)
    ext       v30.8b, v23.8b , v24.8b , #4
    shrn      v20.4h, v26.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set1)
    ext       v29.8b, v23.8b , v24.8b , #6

    saddl     v26.4s, v31.4h, v23.4h    //// a0 + a5                             (set4)
    ext       v28.8b, v23.8b , v24.8b , #2
    ext       v31.8b, v25.8b , v25.8b , #2
    smlal     v26.4s, v30.4h, v0.h[1]   //// a0 + a5 + 20a2                         (set4)
    smlal     v26.4s, v29.4h, v0.h[1]   //// a0 + a5 + 20a2 + 20a3                  (set4)
    smlsl     v26.4s, v28.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1          (set4)
    smlsl     v26.4s, v24.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set4)
    ext       v30.8b, v24.8b , v25.8b , #4

    saddl     v22.4s, v31.4h, v24.4h    //// a0 + a5                             (set5)
    ext       v29.8b, v24.8b , v25.8b , #6

    ext       v31.8b, v24.8b , v25.8b , #2
    shrn      v28.4h, v18.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set3)

    ld1       {v17.8b, v18.8b, v19.8b}, [x0], x3 //// Load next Row data
    smlal     v22.4s, v30.4h, v0.h[1]   //// a0 + a5 + 20a2                         (set5)
    smlal     v22.4s, v29.4h, v0.h[1]   //// a0 + a5 + 20a2 + 20a3                  (set5)
    smlsl     v22.4s, v31.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1          (set5)
    smlsl     v22.4s, v25.4h, v0.h[0]   //// a0 + a5 + 20a2 + 20a3 - 5a1 - 5a4    (set5)
    shrn      v29.4h, v26.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set4)
    mov       v20.d[1], v21.d[0]
    sqrshrun  v26.8b, v20.8h, #2        //// half,half gird set1,2

    mov       v28.d[1], v29.d[0]
    sqrshrun  v27.8b, v28.8h, #2        //// half,half gird set3,4
    shrn      v28.4h, v22.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set5)

    sqrshrun  v28.8b, v28.8h, #2        //// half,half gird set5

    st1       {v26.8b, v27.8b}, [x2], #16 //// store 1/2,1,2 grif values
    st1       {v28.h}[0], [x2], x19     //// store 1/2,1,2 grif values

    subs      x10, x10, #1              ////decrement loop counter

    bne       filter_2dvh_loop


//// Process first vertical interpolated row
//// each column is
    //// ////////////// ROW 13 ///////////////////////

//// Process first vertical interpolated row
//// each column is

    // LDMFD sp!,{x10,x11,x12,pc}
    ldp       x19, x20, [sp], #16
    pop_v_regs
    ret

filter_2dvh_skip_row:
    mov       v28.d[1], v29.d[0]
    sqrshrun  v27.8b, v28.8h, #2        //// half,half gird set3,4
    shrn      v28.4h, v22.4s, #8        //// shift by 8 and later we will shift by 2 more with rounding     (set5)

    sqrshrun  v28.8b, v28.8h, #2        //// half,half gird set5

    st1       {v26.8b, v27.8b}, [x2], #16 //// store 1/2,1,2 grif values
    st1       {v28.h}[0], [x2], x19     //// store 1/2,1,2 grif values
    // LDMFD sp!,{x10,x11,x12,pc}
    ldp       x19, x20, [sp], #16
    pop_v_regs
    ret


///*****************************************
