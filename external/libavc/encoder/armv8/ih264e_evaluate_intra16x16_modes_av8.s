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
//******************************************************************************
//*
//* @brief :Evaluate best intra 16x16 mode (among VERT, HORZ and DC )
//*                and do the prediction.
//*
//* @par Description
//*   This function evaluates  first three 16x16 modes and compute corresponding sad
//*   and return the buffer predicted with best mode.
//*
//* @param[in] pu1_src
//*  UWORD8 pointer to the source
//*
//** @param[in] pu1_ngbr_pels_i16
//*  UWORD8 pointer to neighbouring pels
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
//* @param[in] u4_n_avblty
//* availability of neighbouring pixels
//*
//* @param[in] u4_intra_mode
//* Pointer to the variable in which best mode is returned
//*
//* @param[in] pu4_sadmin
//* Pointer to the variable in which minimum sad is returned
//*
//* @param[in] u4_valid_intra_modes
//* Says what all modes are valid
//*
//*
//* @return      none
//*
//******************************************************************************
//*/
//
//void ih264e_evaluate_intra16x16_modes(UWORD8 *pu1_src,
//                                      UWORD8 *pu1_ngbr_pels_i16,
//                                      UWORD8 *pu1_dst,
//                                      UWORD32 src_strd,
//                                      UWORD32 dst_strd,
//                                      WORD32 u4_n_avblty,
//                                      UWORD32 *u4_intra_mode,
//                                      WORD32 *pu4_sadmin,
//                                       UWORD32 u4_valid_intra_modes)
//
.text
.p2align 2
.include "ih264_neon_macros.s"

.global ih264e_evaluate_intra16x16_modes_av8

ih264e_evaluate_intra16x16_modes_av8:

//x0 = pu1_src,
//x1 = pu1_ngbr_pels_i16,
//x2 = pu1_dst,
//x3 = src_strd,
//x4 = dst_strd,
//x5 = u4_n_avblty,
//x6 = u4_intra_mode,
//x7 = pu4_sadmin



    // STMFD sp!, {x4-x12, x14}          //store register values to stack
    push_v_regs
    stp       x19, x20, [sp, #-16]!

    ldr       x16, [sp, #80]
    mov       x17, x4
    mov       x14, x6
    mov       x15, x7


    sub       v0.16b, v0.16b, v0.16b
    sub       v1.16b, v1.16b, v1.16b
    mov       w10, #0
    mov       w11 , #3

    ands      x6, x5, #0x01
    beq       top_available             //LEFT NOT AVAILABLE
    ld1       {v0.16b}, [x1]
    add       w10, w10, #8
    add       w11, w11, #1
top_available:
    ands      x6, x5, #0x04
    beq       none_available
    add       x6, x1, #17
    ld1       {v1.16b}, [x6]
    add       w10, w10, #8
    add       w11, w11, #1
    b         summation
none_available:
    cmp       x5, #0
    bne       summation
    mov       w6, #128
    dup       v30.16b, w6
    dup       v31.16b, w6
    b         sad_comp
summation:
    uaddl     v2.8h, v0.8b, v1.8b
    uaddl2    v3.8h, v0.16b, v1.16b
    dup       v10.8h, w10
    neg       w11, w11
    dup       v20.8h, w11
    add       v0.8h, v2.8h, v3.8h
    mov       v1.d[0], v0.d[1]
    add       v0.4h, v0.4h, v1.4h
    addp      v0.4h, v0.4h , v0.4h
    addp      v0.4h, v0.4h , v0.4h
    add       v0.4h, v0.4h, v10.4h
    uqshl     v0.8h, v0.8h, v20.8h
    sqxtun    v0.8b, v0.8h

    dup       v30.16b, v0.b[0]
    dup       v31.16b, v0.b[0]


sad_comp:
    ld1       { v0.2s, v1.2s }, [x0], x3 // source x0w 0

    ld1       { v2.2s, v3.2s}, [x0], x3 //row 1

    ld1       { v4.2s, v5.2s}, [x0], x3 //row 2

    ld1       { v6.2s, v7.2s}, [x0], x3 //row 3

    //---------------------

    //values for vertical prediction
    add       x6, x1, #17
    ld1       {v10.8b}, [x6], #8
    ld1       {v11.8b}, [x6], #8
    ld1       {v9.16b}, [x1]



    dup       v20.8b, v9.b[15]          ///HORIZONTAL VALUE ROW=0//
    dup       v21.8b, v9.b[15]          ///HORIZONTAL VALUE ROW=0//


///* computing SADs for all three modes*/
    ///vertical row 0@
    uabdl     v16.8h, v0.8b, v10.8b
    uabdl     v18.8h, v1.8b, v11.8b

    ///HORZ row 0@
    uabdl     v26.8h, v0.8b, v20.8b
    uabdl     v28.8h, v1.8b, v21.8b

    ///dc row 0@
    uabdl     v22.8h, v0.8b, v30.8b
    uabdl     v24.8h, v1.8b, v31.8b





    dup       v20.8b, v9.b[14]          ///HORIZONTAL VALUE ROW=1//
    dup       v21.8b, v9.b[14]


    ///vertical row 1@
    uabal     v16.8h, v2.8b, v10.8b
    uabal     v18.8h, v3.8b, v11.8b

    ld1       { v0.2s, v1.2s }, [x0], x3 //row 4
    ///HORZ row 1@
    uabal     v26.8h, v2.8b, v20.8b
    uabal     v28.8h, v3.8b, v21.8b

    ///dc row 1@
    uabal     v22.8h, v2.8b, v30.8b
    uabal     v24.8h, v3.8b, v31.8b

    dup       v20.8b, v9.b[13]          ///HORIZONTAL VALUE ROW=2//
    dup       v21.8b, v9.b[13]

    ///vertical row 2@
    uabal     v16.8h, v4.8b, v10.8b
    uabal     v18.8h, v5.8b, v11.8b

    ld1       { v2.2s, v3.2s}, [x0], x3 //row 5
    ///HORZ row 2@
    uabal     v26.8h, v4.8b, v20.8b
    uabal     v28.8h, v5.8b, v21.8b

    ///dc row 2@
    uabal     v22.8h, v4.8b, v30.8b
    uabal     v24.8h, v5.8b, v31.8b

    dup       v20.8b, v9.b[12]          ///HORIZONTAL VALUE ROW=3//
    dup       v21.8b, v9.b[12]

    ///vertical row 3@
    uabal     v16.8h, v6.8b, v10.8b
    uabal     v18.8h, v7.8b, v11.8b

    ld1       { v4.2s, v5.2s}, [x0], x3 //row 6
    ///HORZ row 3@
    uabal     v26.8h, v6.8b, v20.8b
    uabal     v28.8h, v7.8b, v21.8b

    ///dc row 3@
    uabal     v22.8h, v6.8b, v30.8b
    uabal     v24.8h, v7.8b, v31.8b
//----------------------------------------------------------------------------------------------

    dup       v20.8b, v9.b[11]          ///HORIZONTAL VALUE ROW=0//
    dup       v21.8b, v9.b[11]

    ///vertical row 0@
    uabal     v16.8h, v0.8b, v10.8b
    uabal     v18.8h, v1.8b, v11.8b

    ld1       {  v6.2s, v7.2s}, [x0], x3 //row 7
    ///HORZ row 0@
    uabal     v26.8h, v0.8b, v20.8b
    uabal     v28.8h, v1.8b, v21.8b

    ///dc row 0@
    uabal     v22.8h, v0.8b, v30.8b
    uabal     v24.8h, v1.8b, v31.8b

    dup       v20.8b, v9.b[10]          ///HORIZONTAL VALUE ROW=1//
    dup       v21.8b, v9.b[10]

    ///vertical row 1@
    uabal     v16.8h, v2.8b, v10.8b
    uabal     v18.8h, v3.8b, v11.8b

    ld1       { v0.2s, v1.2s }, [x0], x3 //row 8
    ///HORZ row 1@
    uabal     v26.8h, v2.8b, v20.8b
    uabal     v28.8h, v3.8b, v21.8b

    ///dc row 1@
    uabal     v22.8h, v2.8b, v30.8b
    uabal     v24.8h, v3.8b, v31.8b

    dup       v20.8b, v9.b[9]           ///HORIZONTAL VALUE ROW=2//
    dup       v21.8b, v9.b[9]

    ///vertical row 2@
    uabal     v16.8h, v4.8b, v10.8b
    uabal     v18.8h, v5.8b, v11.8b

    ld1       { v2.2s, v3.2s}, [x0], x3 //row 9

    ///HORZ row 2@
    uabal     v26.8h, v4.8b, v20.8b
    uabal     v28.8h, v5.8b, v21.8b

    ///dc row 2@
    uabal     v22.8h, v4.8b, v30.8b
    uabal     v24.8h, v5.8b, v31.8b

    dup       v20.8b, v9.b[8]           ///HORIZONTAL VALUE ROW=3//
    dup       v21.8b, v9.b[8]

    ///vertical row 3@
    uabal     v16.8h, v6.8b, v10.8b
    uabal     v18.8h, v7.8b, v11.8b

    ld1       { v4.2s, v5.2s}, [x0], x3 //row 10

    ///HORZ row 3@
    uabal     v26.8h, v6.8b, v20.8b
    uabal     v28.8h, v7.8b, v21.8b

    ///dc row 3@
    uabal     v22.8h, v6.8b, v30.8b
    uabal     v24.8h, v7.8b, v31.8b


//-------------------------------------------

    dup       v20.8b, v9.b[7]           ///HORIZONTAL VALUE ROW=0//
    dup       v21.8b, v9.b[7]

    ///vertical row 0@
    uabal     v16.8h, v0.8b, v10.8b
    uabal     v18.8h, v1.8b, v11.8b

    ld1       {  v6.2s, v7.2s}, [x0], x3 //row11

    ///HORZ row 0@
    uabal     v26.8h, v0.8b, v20.8b
    uabal     v28.8h, v1.8b, v21.8b

    ///dc row 0@
    uabal     v22.8h, v0.8b, v30.8b
    uabal     v24.8h, v1.8b, v31.8b

    dup       v20.8b, v9.b[6]           ///HORIZONTAL VALUE ROW=1//
    dup       v21.8b, v9.b[6]

    ///vertical row 1@
    uabal     v16.8h, v2.8b, v10.8b
    uabal     v18.8h, v3.8b, v11.8b

    ld1       { v0.2s, v1.2s }, [x0], x3 //row12

    ///HORZ row 1@
    uabal     v26.8h, v2.8b, v20.8b
    uabal     v28.8h, v3.8b, v21.8b

    ///dc row 1@
    uabal     v22.8h, v2.8b, v30.8b
    uabal     v24.8h, v3.8b, v31.8b

    dup       v20.8b, v9.b[5]           ///HORIZONTAL VALUE ROW=2//
    dup       v21.8b, v9.b[5]

    ///vertical row 2@
    uabal     v16.8h, v4.8b, v10.8b
    uabal     v18.8h, v5.8b, v11.8b

    ld1       { v2.2s, v3.2s}, [x0], x3 //row13

    ///HORZ row 2@
    uabal     v26.8h, v4.8b, v20.8b
    uabal     v28.8h, v5.8b, v21.8b

    ///dc row 2@
    uabal     v22.8h, v4.8b, v30.8b
    uabal     v24.8h, v5.8b, v31.8b

    dup       v20.8b, v9.b[4]           ///HORIZONTAL VALUE ROW=3//
    dup       v21.8b, v9.b[4]

    ///vertical row 3@
    uabal     v16.8h, v6.8b, v10.8b
    uabal     v18.8h, v7.8b, v11.8b

    ld1       { v4.2s, v5.2s}, [x0], x3 //row14

    ///HORZ row 3@
    uabal     v26.8h, v6.8b, v20.8b
    uabal     v28.8h, v7.8b, v21.8b

    ///dc row 3@
    uabal     v22.8h, v6.8b, v30.8b
    uabal     v24.8h, v7.8b, v31.8b
    //-----------------------------------------------------------------

    dup       v20.8b, v9.b[3]           ///HORIZONTAL VALUE ROW=0//
    dup       v21.8b, v9.b[3]

    ///vertical row 0@
    uabal     v16.8h, v0.8b, v10.8b
    uabal     v18.8h, v1.8b, v11.8b

    ld1       {  v6.2s, v7.2s}, [x0], x3 //row15

    ///HORZ row 0@
    uabal     v26.8h, v0.8b, v20.8b
    uabal     v28.8h, v1.8b, v21.8b

    ///dc row 0@
    uabal     v22.8h, v0.8b, v30.8b
    uabal     v24.8h, v1.8b, v31.8b

    dup       v20.8b, v9.b[2]           ///HORIZONTAL VALUE ROW=1//
    dup       v21.8b, v9.b[2]

    ///vertical row 1@
    uabal     v16.8h, v2.8b, v10.8b
    uabal     v18.8h, v3.8b, v11.8b

    ///HORZ row 1@
    uabal     v26.8h, v2.8b, v20.8b
    uabal     v28.8h, v3.8b, v21.8b

    ///dc row 1@
    uabal     v22.8h, v2.8b, v30.8b
    uabal     v24.8h, v3.8b, v31.8b

    dup       v20.8b, v9.b[1]           ///HORIZONTAL VALUE ROW=2//
    dup       v21.8b, v9.b[1]

    ///vertical row 2@
    uabal     v16.8h, v4.8b, v10.8b
    uabal     v18.8h, v5.8b, v11.8b

    ///HORZ row 2@
    uabal     v26.8h, v4.8b, v20.8b
    uabal     v28.8h, v5.8b, v21.8b

    ///dc row 2@
    uabal     v22.8h, v4.8b, v30.8b
    uabal     v24.8h, v5.8b, v31.8b

    dup       v20.8b, v9.b[0]           ///HORIZONTAL VALUE ROW=3//
    dup       v21.8b, v9.b[0]

    ///vertical row 3@
    uabal     v16.8h, v6.8b, v10.8b
    uabal     v18.8h, v7.8b, v11.8b

    ///HORZ row 3@
    uabal     v26.8h, v6.8b, v20.8b
    uabal     v28.8h, v7.8b, v21.8b

    ///dc row 3@
    uabal     v22.8h, v6.8b, v30.8b
    uabal     v24.8h, v7.8b, v31.8b
    //------------------------------------------------------------------------------


    //vert sum

    add       v16.8h, v16.8h , v18.8h
    mov       v18.d[0], v16.d[1]
    add       v16.4h, v16.4h , v18.4h
    uaddlp    v16.2s, v16.4h
    addp      v16.2s, v16.2s, v16.2s
    smov      x8, v16.s[0]              //dc


    //horz sum

    add       v26.8h, v26.8h , v28.8h
    mov       v28.d[0], v26.d[1]
    add       v26.4h, v26.4h , v28.4h
    uaddlp    v26.2s, v26.4h
    addp      v26.2s, v26.2s, v26.2s
    smov      x9, v26.s[0]

    //dc sum

    add       v24.8h, v22.8h , v24.8h   ///DC
    mov       v25.d[0], v24.d[1]
    add       v24.4h, v24.4h , v25.4h   ///DC
    uaddlp    v24.2s, v24.4h            ///DC
    addp      v24.2s, v24.2s, v24.2s    ///DC
    smov      x10, v24.s[0]             //dc


    //-----------------------
    mov       x11, #1
    lsl       x11, x11, #30

    mov       x0, x16
    //--------------------------------------------
    ands      x7, x0, #01               // vert mode valid????????????
    csel      x8, x11, x8, eq


    ands      x6, x0, #02               // horz mode valid????????????
    csel      x9, x11, x9, eq

    ands      x6, x0, #04               // dc mode valid????????????
    csel      x10, x11, x10, eq




//--------------------------------

    mov       x4, x17
    mov       x7, x15
    mov       x6, x14

    //---------------------------

    //--------------------------

    cmp       x8, x9
    bgt       not_vert
    cmp       x8, x10
    bgt       do_dc

    ///----------------------
    //DO VERTICAL PREDICTION
    str       w8 , [x7]                 //MIN SAD
    mov       w8, #0
    str       w8 , [x6]                 // MODE
    add       x6, x1, #17
    ld1       {v30.16b}, [x6]
    b         do_dc_vert
    //-----------------------------
not_vert: cmp x9, x10
    bgt       do_dc

    ///----------------------
    //DO HORIZONTAL
    str       w9 , [x7]                 //MIN SAD
    mov       w9, #1
    str       w9 , [x6]                 // MODE

    ld1       {v0.16b}, [x1]
    dup       v10.16b, v0.b[15]
    dup       v11.16b, v0.b[14]
    dup       v12.16b, v0.b[13]
    dup       v13.16b, v0.b[12]
    st1       {v10.16b}, [x2], x4
    dup       v14.16b, v0.b[11]
    st1       {v11.16b}, [x2], x4
    dup       v15.16b, v0.b[10]
    st1       {v12.16b}, [x2], x4
    dup       v16.16b, v0.b[9]
    st1       {v13.16b}, [x2], x4
    dup       v17.16b, v0.b[8]
    st1       {v14.16b}, [x2], x4
    dup       v18.16b, v0.b[7]
    st1       {v15.16b}, [x2], x4
    dup       v19.16b, v0.b[6]
    st1       {v16.16b}, [x2], x4
    dup       v20.16b, v0.b[5]
    st1       {v17.16b}, [x2], x4
    dup       v21.16b, v0.b[4]
    st1       {v18.16b}, [x2], x4
    dup       v22.16b, v0.b[3]
    st1       {v19.16b}, [x2], x4
    dup       v23.16b, v0.b[2]
    st1       {v20.16b}, [x2], x4
    dup       v24.16b, v0.b[1]
    st1       {v21.16b}, [x2], x4
    dup       v25.16b, v0.b[0]
    st1       {v22.16b}, [x2], x4
    st1       {v23.16b}, [x2], x4
    st1       {v24.16b}, [x2], x4
    st1       {v25.16b}, [x2], x4



    b         end_func


    ///-----------------------------

do_dc: ///---------------------------------
    //DO DC
    str       w10 , [x7]                //MIN SAD
    mov       w10, #2
    str       w10 , [x6]                // MODE
do_dc_vert:
    st1       {v30.4s}, [x2], x4        //0
    st1       {v30.4s}, [x2], x4        //1
    st1       {v30.4s}, [x2], x4        //2
    st1       {v30.4s}, [x2], x4        //3
    st1       {v30.4s}, [x2], x4        //4
    st1       {v30.4s}, [x2], x4        //5
    st1       {v30.4s}, [x2], x4        //6
    st1       {v30.4s}, [x2], x4        //7
    st1       {v30.4s}, [x2], x4        //8
    st1       {v30.4s}, [x2], x4        //9
    st1       {v30.4s}, [x2], x4        //10
    st1       {v30.4s}, [x2], x4        //11
    st1       {v30.4s}, [x2], x4        //12
    st1       {v30.4s}, [x2], x4        //13
    st1       {v30.4s}, [x2], x4        //14
    st1       {v30.4s}, [x2], x4        //15
    ///------------------
end_func:
    // LDMFD sp!,{x4-x12,PC}         //Restoring registers from stack
    ldp       x19, x20, [sp], #16
    pop_v_regs
    ret


