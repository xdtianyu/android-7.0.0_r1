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
//* @brief :Evaluate best intr chroma mode (among VERT, HORZ and DC )
//*                and do the prediction.
//*
//* @par Description
//*   This function evaluates  first three intra chroma modes and compute corresponding sad
//*   and return the buffer predicted with best mode.
//*
//* @param[in] pu1_src
//*  UWORD8 pointer to the source
//*
//** @param[in] pu1_ngbr_pels
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
//void ih264e_evaluate_intra_chroma_modes(UWORD8 *pu1_src,
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

.global ih264e_evaluate_intra_chroma_modes_av8

ih264e_evaluate_intra_chroma_modes_av8:

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
    //-----------------------
    ldr       x16, [sp, #80]
    mov       x17, x4
    mov       x18, x5
    mov       x14, x6
    mov       x15, x7

    mov       x19, #5
    ands      x6, x5, x19
    beq       none_available
    cmp       x6, #1
    beq       left_only_available
    cmp       x6, #4
    beq       top_only_available

all_available:
    ld1       {v0.8b, v1.8b}, [x1]
    add       x6, x1, #18
    ld1       {v2.8b, v3.8b}, [x6]
    uxtl      v0.8h, v0.8b
    uxtl      v1.8h, v1.8b
    addp      v0.4s, v0.4s , v0.4s
    addp      v1.4s, v1.4s , v1.4s
    addp      v0.4s, v0.4s , v0.4s
    addp      v1.4s, v1.4s , v1.4s
    uxtl      v2.8h, v2.8b
    uxtl      v3.8h, v3.8b
    addp      v2.4s, v2.4s , v2.4s
    addp      v3.4s, v3.4s , v3.4s
    addp      v2.4s, v2.4s , v2.4s
    addp      v3.4s, v3.4s , v3.4s
    rshrn     v5.8b, v0.8h, #2
    dup       v21.8h, v5.h[0]
    rshrn     v6.8b, v3.8h, #2
    dup       v20.8h, v6.h[0]
    add       v1.8h, v1.8h, v2.8h
    rshrn     v1.8b, v1.8h, #3
    dup       v23.8h, v1.h[0]
    mov       v20.d[0], v23.d[0]
    add       v0.8h, v0.8h, v3.8h
    rshrn     v0.8b, v0.8h, #3
    dup       v23.8h, v0.h[0]
    mov       v31.d[0], v23.d[0]
    mov       v28.d[0], v20.d[0]
    mov       v29.d[0], v20.d[1]
    mov       v30.d[0], v21.d[0]
    b         sad_comp

left_only_available:
    ld1       {v0.8b, v1.8b}, [x1]
    uxtl      v0.8h, v0.8b
    uxtl      v1.8h, v1.8b
    addp      v0.4s, v0.4s , v0.4s
    addp      v1.4s, v1.4s , v1.4s
    addp      v0.4s, v0.4s , v0.4s
    addp      v1.4s, v1.4s , v1.4s
    rshrn     v0.8b, v0.8h, #2
    rshrn     v1.8b, v1.8h, #2

    dup       v28.8h , v1.h[0]
    dup       v29.8h , v1.h[0]
    dup       v30.8h, v0.h[0]
    dup       v31.8h, v0.h[0]
    b         sad_comp

top_only_available:
    add       x6, x1, #18
    ld1       {v0.8b, v1.8b}, [x6]
    uxtl      v0.8h, v0.8b
    uxtl      v1.8h, v1.8b
    addp      v0.4s, v0.4s , v0.4s
    addp      v1.4s, v1.4s , v1.4s
    addp      v0.4s, v0.4s , v0.4s
    addp      v1.4s, v1.4s , v1.4s
    rshrn     v0.8b, v0.8h, #2
    rshrn     v1.8b, v1.8h, #2
    dup       v28.8h , v0.h[0]
    dup       v30.8h, v1.h[0]
    mov       v29.d[0], v30.d[1]
    mov       v30.d[0], v28.d[0]
    mov       v31.d[0], v30.d[1]
    b         sad_comp
none_available:
    mov       w20, #128
    dup       v28.16b, w20
    dup       v29.16b, w20
    dup       v30.16b, w20
    dup       v31.16b, w20



sad_comp:
    add       x6, x1, #18
    ld1       {v10.8b, v11.8b}, [x6]    // vertical values

    ld1       {v27.8h}, [x1]

    dup       v20.8h, v27.h[7]          ///HORIZONTAL VALUE ROW=0//
    dup       v21.8h, v27.h[7]

    ld1       { v0.8b, v1.8b}, [x0], x3


    ///vertical row 0@
    uabdl     v16.8h, v0.8b, v10.8b
    uabdl     v18.8h, v1.8b, v11.8b

    ///HORZ row 0@
    uabdl     v26.8h, v0.8b, v20.8b
    uabdl     v14.8h, v1.8b, v21.8b

    ld1       {v2.8b, v3.8b}, [x0], x3



    ///dc row 0@
    uabdl     v22.8h, v0.8b, v28.8b
    uabdl     v24.8h, v1.8b, v29.8b


    dup       v20.8h, v27.h[6]
    dup       v21.8h, v27.h[6]          ///HORIZONTAL VALUE ROW=1//

    ///vertical row 1@
    uabal     v16.8h, v2.8b, v10.8b
    uabal     v18.8h, v3.8b, v11.8b

    ld1       { v4.8b, v5.8b}, [x0], x3

    ///HORZ row 1@
    uabal     v26.8h, v2.8b, v20.8b
    uabal     v14.8h, v3.8b, v21.8b

    ///dc row 1@
    uabal     v22.8h, v2.8b, v28.8b
    uabal     v24.8h, v3.8b, v29.8b

    dup       v20.8h, v27.h[5]
    dup       v21.8h, v27.h[5]          ///HORIZONTAL VALUE ROW=2//

    ///vertical row 2@
    uabal     v16.8h, v4.8b, v10.8b
    uabal     v18.8h, v5.8b, v11.8b

    ld1       { v6.8b, v7.8b}, [x0], x3
    ///HORZ row 2@
    uabal     v26.8h, v4.8b, v20.8b
    uabal     v14.8h, v5.8b, v21.8b

    ///dc row 2@
    uabal     v22.8h, v4.8b, v28.8b
    uabal     v24.8h, v5.8b, v29.8b

    dup       v20.8h, v27.h[4]
    dup       v21.8h, v27.h[4]          ///HORIZONTAL VALUE ROW=3//

    ///vertical row 3@
    uabal     v16.8h, v6.8b, v10.8b
    uabal     v18.8h, v7.8b, v11.8b

    ///HORZ row 3@
    uabal     v26.8h, v6.8b, v20.8b
    uabal     v14.8h, v7.8b, v21.8b

    ///dc row 3@
    uabal     v22.8h, v6.8b, v28.8b
    uabal     v24.8h, v7.8b, v29.8b

    //----------------------------------------------------------------------------------------------
    ld1       { v0.8b, v1.8b}, [x0], x3


    dup       v20.8h, v27.h[3]
    dup       v21.8h, v27.h[3]          ///HORIZONTAL VALUE ROW=0//

    ///vertical row 0@
    uabal     v16.8h, v0.8b, v10.8b
    uabal     v18.8h, v1.8b, v11.8b

    ///HORZ row 0@
    uabal     v26.8h, v0.8b, v20.8b
    uabal     v14.8h, v1.8b, v21.8b

    ld1       { v2.8b, v3.8b}, [x0], x3

    ///dc row 0@
    uabal     v22.8h, v0.8b, v30.8b
    uabal     v24.8h, v1.8b, v31.8b

    dup       v20.8h, v27.h[2]
    dup       v21.8h, v27.h[2]          ///HORIZONTAL VALUE ROW=1//

    ///vertical row 1@
    uabal     v16.8h, v2.8b, v10.8b
    uabal     v18.8h, v3.8b, v11.8b

    ///HORZ row 1@
    uabal     v26.8h, v2.8b, v20.8b
    uabal     v14.8h, v3.8b, v21.8b

    ld1       { v4.8b, v5.8b}, [x0], x3

    ///dc row 1@
    uabal     v22.8h, v2.8b, v30.8b
    uabal     v24.8h, v3.8b, v31.8b

    dup       v20.8h, v27.h[1]
    dup       v21.8h, v27.h[1]          ///HORIZONTAL VALUE ROW=2//

    ///vertical row 2@
    uabal     v16.8h, v4.8b, v10.8b
    uabal     v18.8h, v5.8b, v11.8b

    ///HORZ row 2@
    uabal     v26.8h, v4.8b, v20.8b
    uabal     v14.8h, v5.8b, v21.8b

    ld1       {v6.8b, v7.8b}, [x0], x3

    ///dc row 2@
    uabal     v22.8h, v4.8b, v30.8b
    uabal     v24.8h, v5.8b, v31.8b

    dup       v20.8h, v27.h[0]
    dup       v21.8h, v27.h[0]          ///HORIZONTAL VALUE ROW=3//

    ///vertical row 3@
    uabal     v16.8h, v6.8b, v10.8b
    uabal     v18.8h, v7.8b, v11.8b

    ///HORZ row 3@
    uabal     v26.8h, v6.8b, v20.8b
    uabal     v14.8h, v7.8b, v21.8b

    ///dc row 3@
    uabal     v22.8h, v6.8b, v30.8b
    uabal     v24.8h, v7.8b, v31.8b


//-------------------------------------------


//vert sum

    add       v16.8h, v16.8h , v18.8h
    mov       v18.d[0], v16.d[1]
    add       v16.4h, v16.4h , v18.4h
    uaddlp    v16.2s, v16.4h
    addp      v16.2s, v16.2s, v16.2s
    smov      x8, v16.s[0]


    //horz sum

    add       v26.8h, v26.8h , v14.8h
    mov       v14.d[0], v26.d[1]
    add       v26.4h, v26.4h , v14.4h
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




    mov       x11, #1
//-----------------------
    mov       x0, x16 // u4_valid_intra_modes

//--------------------------------------------


    lsl       x11, x11, #30

    ands      x7, x0, #04               // vert mode valid????????????
    csel      x8, x11, x8, eq

    ands      x6, x0, #02               // horz mode valid????????????
    csel      x9, x11, x9, eq

    ands      x6, x0, #01               // dc mode valid????????????
    csel      x10, x11, x10, eq


    //---------------------------

    mov       x4, x17
    mov       x6, x14
    mov       x7, x15

    //--------------------------

    cmp       x10, x9
    bgt       not_dc
    cmp       x10, x8
    bgt       do_vert

    ///----------------------
    //DO DC PREDICTION
    str       w10 , [x7]                //MIN SAD

    mov       w10, #0
    str       w10 , [x6]                // MODE

    b         do_dc_vert
    //-----------------------------

not_dc:
    cmp       x9, x8
    bgt       do_vert
    ///----------------------
    //DO HORIZONTAL
    str       w9 , [x7]                 //MIN SAD

    mov       w10, #1
    str       w10 , [x6]                // MODE
    ld1       {v0.8h}, [x1]

    dup       v10.8h, v0.h[7]
    dup       v11.8h, v0.h[6]
    dup       v12.8h, v0.h[5]
    dup       v13.8h, v0.h[4]
    st1       {v10.8h}, [x2], x4
    dup       v14.8h, v0.h[3]
    st1       {v11.8h}, [x2], x4
    dup       v15.8h, v0.h[2]
    st1       {v12.8h}, [x2], x4
    dup       v16.8h, v0.h[1]
    st1       {v13.8h}, [x2], x4
    dup       v17.8h, v0.h[0]
    st1       {v14.8h}, [x2], x4
    st1       {v15.8h}, [x2], x4
    st1       {v16.8h}, [x2], x4
    st1       {v17.8h}, [x2], x4

    b         end_func

do_vert:
    //DO VERTICAL PREDICTION
    str       w8 , [x7]                 //MIN SAD
    mov       w8, #2
    str       w8 , [x6]                 // MODE
    add       x6, x1, #18
    ld1       {v28.8b, v29.8b}, [x6]    // vertical values
    ld1       {v30.8b, v31.8b}, [x6]    // vertical values

do_dc_vert:
    st1       {v28.2s, v29.2s} , [x2], x4 //0
    st1       {v28.2s, v29.2s} , [x2], x4 //1
    st1       {v28.2s, v29.2s} , [x2], x4 //2
    st1       {v28.2s, v29.2s} , [x2], x4 //3
    st1       {v30.2s, v31.2s} , [x2], x4 //4
    st1       {v30.2s, v31.2s} , [x2], x4 //5
    st1       {v30.2s, v31.2s} , [x2], x4 //6
    st1       {v30.2s, v31.2s} , [x2], x4 //7

end_func:
    // LDMFD sp!,{x4-x12,PC}         //Restoring registers from stack
    ldp       x19, x20, [sp], #16
    pop_v_regs
    ret


