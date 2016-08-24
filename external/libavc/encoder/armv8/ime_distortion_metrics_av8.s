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
//**

///**
//******************************************************************************
//*
//*
//* @brief
//*  This file contains definitions of routines that compute distortion
//*  between two macro/sub blocks of identical dimensions
//*
//* @author
//*  Ittiam
//*
//* @par List of Functions:
//*  - ime_compute_sad_16x16()
//*  - ime_compute_sad_8x8()
//*  - ime_compute_sad_4x4()
//*  - ime_compute_sad_16x8()
//*  - ime_compute_satqd_16x16_lumainter_av8()
//*
//* @remarks
//*  None
//*
//*******************************************************************************
//


///**
//******************************************************************************
//*
//* @brief computes distortion (SAD) between 2 16x16 blocks (fast mode)
//*
//* @par   Description
//*   This functions computes SAD between 2 16x16 blocks. There is a provision
//*   for early exit if the up-to computed SAD exceeds maximum allowed SAD. To
//*   compute the distortion of the entire block set u4_max_sad to USHRT_MAX.
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
//* @param[in] i4_max_sad
//*  integer maximum allowed distortion
//*
//* @param[in] pi4_mb_distortion
//*  integer evaluated sad
//*
//* @remarks
//*
//******************************************************************************
//*/
.text
.p2align 2

.macro push_v_regs
    stp       d8, d9, [sp, #-16]!
    stp       d10, d11, [sp, #-16]!
    stp       d12, d13, [sp, #-16]!
    stp       d14, d15, [sp, #-16]!
.endm
.macro pop_v_regs
    ldp       d14, d15, [sp], #16
    ldp       d12, d13, [sp], #16
    ldp       d10, d11, [sp], #16
    ldp       d8, d9, [sp], #16
.endm

    .global ime_compute_sad_16x16_fast_av8
ime_compute_sad_16x16_fast_av8:
    push_v_regs
    lsl       x2, x2, #1
    lsl       x3, x3, #1

    mov       x6, #2
    movi      v30.8h, #0

core_loop_ime_compute_sad_16x16_fast_av8:

    ld1       {v0.16b}, [x0], x2
    ld1       {v1.16b}, [x1], x3
    ld1       {v2.16b}, [x0], x2
    ld1       {v3.16b}, [x1], x3

    uabal     v30.8h, v0.8b, v1.8b
    uabal2    v30.8h, v0.16b, v1.16b

    uabal     v30.8h, v2.8b, v3.8b
    uabal2    v30.8h, v2.16b, v3.16b

    ld1       {v4.16b}, [x0], x2
    ld1       {v5.16b}, [x1], x3
    ld1       {v6.16b}, [x0], x2
    ld1       {v7.16b}, [x1], x3

    uabal     v30.8h, v4.8b, v5.8b
    uabal2    v30.8h, v4.16b, v5.16b

    uabal     v30.8h, v6.8b, v7.8b
    uabal2    v30.8h, v6.16b, v7.16b

    subs      x6, x6, #1
    bne       core_loop_ime_compute_sad_16x16_fast_av8


    addp      v30.8h, v30.8h, v30.8h
    uaddlp    v30.4s, v30.8h
    addp      v30.2s, v30.2s, v30.2s
    shl       v30.2s, v30.2s, #1

    st1       {v30.s}[0], [x5]
    pop_v_regs
    ret


///**
//******************************************************************************
//*
//*  @brief computes distortion (SAD) between 2 16x8  blocks
//*
//*
//*  @par   Description
//*   This functions computes SAD between 2 16x8 blocks. There is a provision
//*   for early exit if the up-to computed SAD exceeds maximum allowed SAD. To
//*   compute the distortion of the entire block set u4_max_sad to USHRT_MAX.
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
//* @param[in] u4_max_sad
//*  integer maximum allowed distortion
//*
//* @param[in] pi4_mb_distortion
//*  integer evaluated sad
//*
//* @remarks
//*
//******************************************************************************
//*/
//
    .global ime_compute_sad_16x8_av8
ime_compute_sad_16x8_av8:

    //chheck what stride incremtn to use
    //earlier code did not have this lsl
    push_v_regs
    mov       x6, #2
    movi      v30.8h, #0

core_loop_ime_compute_sad_16x8_av8:

    ld1       {v0.16b}, [x0], x2
    ld1       {v1.16b}, [x1], x3
    ld1       {v2.16b}, [x0], x2
    ld1       {v3.16b}, [x1], x3

    uabal     v30.8h, v0.8b, v1.8b
    uabal2    v30.8h, v0.16b, v1.16b

    uabal     v30.8h, v2.8b, v3.8b
    uabal2    v30.8h, v2.16b, v3.16b

    ld1       {v4.16b}, [x0], x2
    ld1       {v5.16b}, [x1], x3
    ld1       {v6.16b}, [x0], x2
    ld1       {v7.16b}, [x1], x3

    uabal     v30.8h, v4.8b, v5.8b
    uabal2    v30.8h, v4.16b, v5.16b

    uabal     v30.8h, v6.8b, v7.8b
    uabal2    v30.8h, v6.16b, v7.16b

    subs      x6, x6, #1
    bne       core_loop_ime_compute_sad_16x8_av8


    addp      v30.8h, v30.8h, v30.8h
    uaddlp    v30.4s, v30.8h
    addp      v30.2s, v30.2s, v30.2s

    st1       {v30.s}[0], [x5]
    pop_v_regs
    ret

///**
//******************************************************************************
//*
//* @brief computes distortion (SAD) between 2 16x16 blocks with early exit
//*
//* @par   Description
//*   This functions computes SAD between 2 16x16 blocks. There is a provision
//*   for early exit if the up-to computed SAD exceeds maximum allowed SAD. To
//*   compute the distortion of the entire block set u4_max_sad to USHRT_MAX.
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
//* @param[in] i4_max_sad
//*  integer maximum allowed distortion
//*
//* @param[in] pi4_mb_distortion
//*  integer evaluated sad
//*
//* @remarks
//*
//******************************************************************************
//*/

    .global ime_compute_sad_16x16_ea8_av8
ime_compute_sad_16x16_ea8_av8:

    push_v_regs
    movi      v30.8h, #0

    add       x7, x0, x2
    add       x8, x1, x3

    lsl       x2, x2, #1
    lsl       x3, x3, #1

    ld1       {v0.16b}, [x0], x2
    ld1       {v1.16b}, [x1], x3
    ld1       {v2.16b}, [x0], x2
    ld1       {v3.16b}, [x1], x3
    ld1       {v8.16b}, [x0], x2
    ld1       {v9.16b}, [x1], x3
    ld1       {v10.16b}, [x0], x2
    ld1       {v11.16b}, [x1], x3
    ld1       {v12.16b}, [x0], x2
    ld1       {v13.16b}, [x1], x3
    ld1       {v14.16b}, [x0], x2
    ld1       {v15.16b}, [x1], x3
    ld1       {v16.16b}, [x0], x2
    ld1       {v17.16b}, [x1], x3
    ld1       {v18.16b}, [x0], x2
    ld1       {v19.16b}, [x1], x3

    uabal     v30.8h, v0.8b, v1.8b
    uabal2    v30.8h, v0.16b, v1.16b

    uabal     v30.8h, v2.8b, v3.8b
    uabal2    v30.8h, v2.16b, v3.16b

    uabal     v30.8h, v8.8b, v9.8b
    uabal2    v30.8h, v8.16b, v9.16b

    uabal     v30.8h, v10.8b, v11.8b
    uabal2    v30.8h, v10.16b, v11.16b

    uabal     v30.8h, v12.8b, v13.8b
    uabal2    v30.8h, v12.16b, v13.16b

    uabal     v30.8h, v14.8b, v15.8b
    uabal2    v30.8h, v14.16b, v15.16b

    uabal     v30.8h, v16.8b, v17.8b
    uabal2    v30.8h, v16.16b, v17.16b

    uabal     v30.8h, v18.8b, v19.8b
    uabal2    v30.8h, v18.16b, v19.16b

    addp      v31.8h, v30.8h, v30.8h
    uaddlp    v31.4s, v31.8h
    addp      v31.2s, v31.2s, v31.2s
    mov       w6, v31.s[0]
    cmp       w6, w4
    bgt       end_func_16x16

    //do the stuff again
    ld1       {v0.16b}, [x7], x2
    ld1       {v1.16b}, [x8], x3
    ld1       {v2.16b}, [x7], x2
    ld1       {v3.16b}, [x8], x3
    ld1       {v8.16b}, [x7], x2
    ld1       {v9.16b}, [x8], x3
    ld1       {v10.16b}, [x7], x2
    ld1       {v11.16b}, [x8], x3
    ld1       {v12.16b}, [x7], x2
    ld1       {v13.16b}, [x8], x3
    ld1       {v14.16b}, [x7], x2
    ld1       {v15.16b}, [x8], x3
    ld1       {v16.16b}, [x7], x2
    ld1       {v17.16b}, [x8], x3
    ld1       {v18.16b}, [x7], x2
    ld1       {v19.16b}, [x8], x3

    uabal     v30.8h, v0.8b, v1.8b
    uabal2    v30.8h, v0.16b, v1.16b

    uabal     v30.8h, v2.8b, v3.8b
    uabal2    v30.8h, v2.16b, v3.16b

    uabal     v30.8h, v8.8b, v9.8b
    uabal2    v30.8h, v8.16b, v9.16b

    uabal     v30.8h, v10.8b, v11.8b
    uabal2    v30.8h, v10.16b, v11.16b

    uabal     v30.8h, v12.8b, v13.8b
    uabal2    v30.8h, v12.16b, v13.16b

    uabal     v30.8h, v14.8b, v15.8b
    uabal2    v30.8h, v14.16b, v15.16b

    uabal     v30.8h, v16.8b, v17.8b
    uabal2    v30.8h, v16.16b, v17.16b

    uabal     v30.8h, v18.8b, v19.8b
    uabal2    v30.8h, v18.16b, v19.16b

    addp      v31.8h, v30.8h, v30.8h
    uaddlp    v31.4s, v31.8h
    addp      v31.2s, v31.2s, v31.2s

end_func_16x16:
    st1       {v31.s}[0], [x5]
    pop_v_regs
    ret


///*
////---------------------------------------------------------------------------
//// Function Name      : ime_calculate_sad2_prog_av8()
////
//// Detail Description : This function find the sad values of 4 Progressive MBs
////                        at one shot
////
//// Platform           : CortexAv8/NEON            .
////
////-----------------------------------------------------------------------------
//*/

    .global ime_calculate_sad2_prog_av8
ime_calculate_sad2_prog_av8:

    // x0    = ref1     <UWORD8 *>
    // x1    = ref2     <UWORD8 *>
    // x2    = src     <UWORD8 *>
    // x3    = RefBufferWidth <UWORD32>
    // stack = CurBufferWidth <UWORD32>, psad <UWORD32 *>
    push_v_regs
    mov       x6, #8
    movi      v30.8h, #0
    movi      v31.8h, #0

core_loop_ime_calculate_sad2_prog_av8:

    ld1       {v0.16b}, [x0], x3
    ld1       {v1.16b}, [x1], x3
    ld1       {v2.16b}, [x3], x4

    ld1       {v3.16b}, [x0], x3
    ld1       {v4.16b}, [x1], x3
    ld1       {v5.16b}, [x3], x4


    uabal     v30.8h, v0.8b, v2.8b
    uabal2    v30.8h, v0.16b, v2.16b
    uabal     v31.8h, v1.8b, v2.8b
    uabal2    v31.8h, v1.16b, v2.16b

    uabal     v30.8h, v3.8b, v5.8b
    uabal2    v30.8h, v3.16b, v5.16b
    uabal     v31.8h, v4.8b, v5.8b
    uabal2    v31.8h, v4.16b, v5.16b


    ld1       {v6.16b}, [x0], x3
    ld1       {v7.16b}, [x1], x3
    ld1       {v8.16b}, [x3], x4

    ld1       {v9.16b}, [x0], x3
    ld1       {v10.16b}, [x1], x3
    ld1       {v11.16b}, [x3], x4

    uabal     v30.8h, v6.8b, v8.8b
    uabal2    v30.8h, v6.16b, v8.16b
    uabal     v31.8h, v7.8b, v8.8b
    uabal2    v31.8h, v7.16b, v8.16b

    uabal     v30.8h, v9.8b, v11.8b
    uabal2    v30.8h, v9.16b, v11.16b
    uabal     v31.8h, v10.8b, v11.8b
    uabal2    v31.8h, v0.16b, v11.16b

    subs      x6, x6, #1
    bne       core_loop_ime_calculate_sad2_prog_av8

    addp      v30.8h, v30.8h, v31.8h
    uaddlp    v30.4s, v30.8h
    addp      v30.2s, v30.2s, v30.2s
    shl       v30.2s, v30.2s, #1

    st1       {v30.2s}, [x5]
    pop_v_regs
    ret

///*
////---------------------------------------------------------------------------
//// Function Name      : Calculate_Mad3_prog()
////
//// Detail Description : This function find the sad values of 4 Progressive MBs
////                        at one shot
////
//// Platform           : CortexA8/NEON            .
////
////-----------------------------------------------------------------------------
//*/

    .global ime_calculate_sad3_prog_av8
ime_calculate_sad3_prog_av8:

    // x0    = ref1     <UWORD8 *>
    // x1    = ref2     <UWORD8 *>
    // x2    = ref3     <UWORD8 *>
    // x3    = src     <UWORD8 *>
    // stack = RefBufferWidth <UWORD32>, CurBufferWidth <UWORD32>, psad <UWORD32 *>


    // x0    = ref1     <UWORD8 *>
    // x1    = ref2     <UWORD8 *>
    // x2    = src     <UWORD8 *>
    // x3    = RefBufferWidth <UWORD32>
    // stack = CurBufferWidth <UWORD32>, psad <UWORD32 *>
    push_v_regs
    mov       x6, #16
    movi      v29.8h, #0
    movi      v30.8h, #0
    movi      v31.8h, #0

core_loop_ime_calculate_sad3_prog_av8:

    ld1       {v0.16b}, [x0], x4
    ld1       {v1.16b}, [x1], x4
    ld1       {v2.16b}, [x2], x4
    ld1       {v3.16b}, [x3], x5

    uabal     v29.8h, v0.8b, v3.8b
    uabal2    v29.8h, v0.16b, v3.16b
    uabal     v30.8h, v1.8b, v3.8b
    uabal2    v30.8h, v1.16b, v3.16b
    uabal     v31.8h, v2.8b, v3.8b
    uabal2    v31.8h, v2.16b, v3.16b

    ld1       {v4.16b}, [x0], x4
    ld1       {v5.16b}, [x1], x4
    ld1       {v6.16b}, [x2], x4
    ld1       {v7.16b}, [x3], x5

    uabal     v29.8h, v4.8b, v7.8b
    uabal2    v29.8h, v4.16b, v7.16b
    uabal     v30.8h, v5.8b, v7.8b
    uabal2    v30.8h, v5.16b, v7.16b
    uabal     v31.8h, v6.8b, v7.8b
    uabal2    v31.8h, v6.16b, v7.16b

    subs      x6, x6, #1
    bne       core_loop_ime_calculate_sad2_prog_av8

    addp      v30.8h, v30.8h, v31.8h
    uaddlp    v30.4s, v30.8h
    addp      v30.2s, v30.2s, v30.2s
    shl       v30.2s, v30.2s, #1

    st1       {v30.2s}, [x5]
    pop_v_regs
    ret




///**
//******************************************************************************
//*
//* @brief computes distortion (SAD) for sub-pel motion estimation
//*
//* @par   Description
//*   This functions computes SAD for all the 8 half pel points
//*
//* @param[out] pi4_sad
//*  integer evaluated sad
//*  pi4_sad[0] - half x
//*  pi4_sad[1] - half x - 1
//*  pi4_sad[2] - half y
//*  pi4_sad[3] - half y - 1
//*  pi4_sad[4] - half xy
//*  pi4_sad[5] - half xy - 1
//*  pi4_sad[6] - half xy - strd
//*  pi4_sad[7] - half xy - 1 - strd
//*
//* @remarks
//*
//******************************************************************************
//*/

.text
.p2align 2

    .global ime_sub_pel_compute_sad_16x16_av8
ime_sub_pel_compute_sad_16x16_av8:
    push_v_regs
    sub       x7, x1, #1                //x left
    sub       x8, x2, x5                //y top
    sub       x9, x3, #1                //xy  left
    sub       x10, x3, x5               //xy top
    sub       x11, x10, #1              //xy top left

    movi      v24.8h, #0
    movi      v25.8h, #0
    movi      v26.8h, #0
    movi      v27.8h, #0
    movi      v28.8h, #0
    movi      v29.8h, #0
    movi      v30.8h, #0
    movi      v31.8h, #0

    mov       x12, #16
core_loop_ime_sub_pel_compute_sad_16x16_av8:

    ld1       {v0.16b}, [x0], x4        //src
    ld1       {v1.16b}, [x1], x5        //x
    ld1       {v2.16b}, [x7], x5        //x left
    ld1       {v3.16b}, [x2], x5        //y
    ld1       {v9.16b}, [x8], x5        //y top
    ld1       {v10.16b}, [x3], x5       //xy
    ld1       {v11.16b}, [x9], x5       //xy left
    ld1       {v12.16b}, [x10], x5      //xy top
    ld1       {v13.16b}, [x11], x5      //xy top left

    uabal     v24.8h, v0.8b, v1.8b
    uabal2    v24.8h, v0.16b, v1.16b
    uabal     v25.8h, v0.8b, v2.8b
    uabal2    v25.8h, v0.16b, v2.16b
    uabal     v26.8h, v0.8b, v3.8b
    uabal2    v26.8h, v0.16b, v3.16b
    uabal     v27.8h, v0.8b, v9.8b
    uabal2    v27.8h, v0.16b, v9.16b
    uabal     v28.8h, v0.8b, v10.8b
    uabal2    v28.8h, v0.16b, v10.16b
    uabal     v29.8h, v0.8b, v11.8b
    uabal2    v29.8h, v0.16b, v11.16b
    uabal     v30.8h, v0.8b, v12.8b
    uabal2    v30.8h, v0.16b, v12.16b
    uabal     v31.8h, v0.8b, v13.8b
    uabal2    v31.8h, v0.16b, v13.16b

    subs      x12, x12, #1
    bne       core_loop_ime_sub_pel_compute_sad_16x16_av8

    addp      v24.8h, v24.8h, v25.8h
    addp      v26.8h, v26.8h, v27.8h
    addp      v28.8h, v28.8h, v29.8h
    addp      v30.8h, v30.8h, v31.8h

    uaddlp    v24.4s, v24.8h
    uaddlp    v26.4s, v26.8h
    uaddlp    v28.4s, v28.8h
    uaddlp    v30.4s, v30.8h

    addp      v24.4s, v24.4s, v26.4s
    addp      v25.4s, v28.4s, v30.4s

    st1       {v24.4s-v25.4s}, [x6]


    pop_v_regs
    ret


///**
//******************************************************************************
//*
//* @brief computes distortion (SAD) between 2 16x16 blocks
//*
//* @par   Description
//*   This functions computes SAD between 2 16x16 blocks. There is a provision
//*   for early exit if the up-to computed SAD exceeds maximum allowed SAD. To
//*   compute the distortion of the entire block set u4_max_sad to USHRT_MAX.
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
//* @param[in] i4_max_sad
//*  integer maximum allowed distortion
//*
//* @param[in] pi4_mb_distortion
//*  integer evaluated sad
//*
//* @remarks
//*
//******************************************************************************
//*/
    .global ime_compute_sad_16x16_av8
ime_compute_sad_16x16_av8:
    push_v_regs
    mov       x6, #4
    movi      v30.8h, #0

core_loop_ime_compute_sad_16x16_av8:

    ld1       {v0.16b}, [x0], x2
    ld1       {v1.16b}, [x1], x3
    ld1       {v2.16b}, [x0], x2
    ld1       {v3.16b}, [x1], x3

    uabal     v30.8h, v0.8b, v1.8b
    uabal2    v30.8h, v0.16b, v1.16b

    uabal     v30.8h, v2.8b, v3.8b
    uabal2    v30.8h, v2.16b, v3.16b

    ld1       {v4.16b}, [x0], x2
    ld1       {v5.16b}, [x1], x3
    ld1       {v6.16b}, [x0], x2
    ld1       {v7.16b}, [x1], x3

    uabal     v30.8h, v4.8b, v5.8b
    uabal2    v30.8h, v4.16b, v5.16b

    uabal     v30.8h, v6.8b, v7.8b
    uabal2    v30.8h, v6.16b, v7.16b

    subs      x6, x6, #1
    bne       core_loop_ime_compute_sad_16x16_av8


    addp      v30.8h, v30.8h, v30.8h
    uaddlp    v30.4s, v30.8h
    addp      v30.2s, v30.2s, v30.2s

    st1       {v30.s}[0], [x5]
    pop_v_regs
    ret


///*
////---------------------------------------------------------------------------
//// Function Name      : Calculate_Mad4_prog()
////
//// Detail Description : This function find the sad values of 4 Progressive MBs
////                        at one shot
////
//// Platform           : CortexA8/NEON            .
////
////-----------------------------------------------------------------------------
//*/

    .global ime_calculate_sad4_prog_av8
ime_calculate_sad4_prog_av8:
    push_v_regs
    sub       x5, x0, #1                //left
    add       x6, x0, #1                //right
    sub       x7, x0, x2                //top
    add       x8, x0, x2                //bottom

    movi      v28.8h, #0
    movi      v29.8h, #0
    movi      v30.8h, #0
    movi      v31.8h, #0

    mov       x9, #16
core_loop_ime_calculate_sad4_prog_av8:

    ld1       {v0.16b}, [x1], x3
    ld1       {v1.16b}, [x5], x2
    ld1       {v2.16b}, [x6], x2
    ld1       {v3.16b}, [x7], x2
    ld1       {v9.16b}, [x8], x2

    uabal     v28.8h, v0.8b, v1.8b
    uabal2    v28.8h, v0.16b, v1.16b
    uabal     v29.8h, v0.8b, v2.8b
    uabal2    v29.8h, v0.16b, v2.16b
    uabal     v30.8h, v0.8b, v3.8b
    uabal2    v30.8h, v0.16b, v3.16b
    uabal     v31.8h, v0.8b, v9.8b
    uabal2    v31.8h, v0.16b, v9.16b

    subs      x9, x9, #1
    bne       core_loop_ime_calculate_sad4_prog_av8

    addp      v28.8h, v28.8h, v29.8h
    addp      v30.8h, v30.8h, v31.8h

    uaddlp    v28.4s, v28.8h
    uaddlp    v30.4s, v30.8h

    addp      v28.4s, v28.4s, v30.4s
    st1       {v28.4s}, [x4]
    pop_v_regs
    ret



//*****************************************************************************
//*
//* Function Name         : ime_compute_satqd_16x16_lumainter_av8
//* Description           : This fucntion computes SAD for a 16x16 block.
//                        : It also computes if any 4x4 block will have a nonzero coefficent after transform and quant
//
//  Arguments             :   x0 :pointer to src buffer
//                            x1 :pointer to est buffer
//                            x2 :source stride
//                            x3 :est stride
//                            STACk :Threshold,distotion,is_nonzero
//*
//* Values Returned   : NONE
//*
//* Register Usage    : x0-x11
//* Stack Usage       :
//* Cycles            : Around
//* Interruptiaility  : Interruptable
//*
//* Known Limitations
//*   \Assumptions    :
//*
//* Revision History  :
//*         DD MM YYYY    Author(s)           Changes
//*         14 04 2014    Harinarayanan K K  First version
//*
//*****************************************************************************
    .global ime_compute_satqd_16x16_lumainter_av8
ime_compute_satqd_16x16_lumainter_av8:
    //x0 :pointer to src buffer
    //x1 :pointer to est buffer
    //x2 :Source stride
    //x3 :Pred stride
    //x4 :Threshold pointer
    //x5 :Distortion,ie SAD
    //x6 :is nonzero
    //x7 :loop counter
    push_v_regs
    stp       d8, d9, [sp, #-16]!
    stp       d10, d11, [sp, #-16]!
    stp       d12, d13, [sp, #-16]!
    stp       d14, d15, [sp, #-16]!

    ld1       {v30.8h}, [x4]

    dup       v20.4h, v30.h[1]          //ls1
    dup       v24.4h, v30.h[0]          //ls2
    dup       v21.4h, v30.h[5]          //ls3
    dup       v25.4h, v30.h[7]          //ls4
    dup       v22.4h, v30.h[3]          //ls5
    dup       v26.4h, v30.h[4]          //ls6
    dup       v23.4h, v30.h[6]          //ls7
    dup       v27.4h, v30.h[2]          //ls8

    mov       v20.d[1], v24.d[0]
    mov       v21.d[1], v25.d[0]
    mov       v22.d[1], v26.d[0]
    mov       v23.d[1], v27.d[0]

    add       x4, x4, #16
    ld1       {v29.h}[0], [x4]
    dup       v29.4h, v29.h[0]

    movi      v31.8h, #0

    mov       x7, #4
core_loop_satqd_ime_compute_satqd_16x16_lumainter:
    ld1       {v0.16b}, [x0], x2
    ld1       {v1.16b}, [x1], x3
    ld1       {v2.16b}, [x0], x2
    ld1       {v3.16b}, [x1], x3
    ld1       {v4.16b}, [x0], x2
    ld1       {v5.16b}, [x1], x3
    ld1       {v6.16b}, [x0], x2
    ld1       {v7.16b}, [x1], x3

    uabdl     v10.8h, v0.8b, v1.8b
    uabdl2    v15.8h, v0.16b, v1.16b
    uabdl     v11.8h, v2.8b, v3.8b
    uabdl2    v16.8h, v2.16b, v3.16b
    uabdl     v12.8h, v4.8b, v5.8b
    uabdl2    v17.8h, v4.16b, v5.16b
    uabdl     v13.8h, v6.8b, v7.8b
    uabdl2    v18.8h, v6.16b, v7.16b

    add       v0.8h, v10.8h, v13.8h
    add       v1.8h, v11.8h, v12.8h
    add       v2.8h, v15.8h, v18.8h
    add       v3.8h, v16.8h, v17.8h

    //v0 : S1     S4     S4     S1        A1    A4    A4    A1
    //v1 : S2     S3     S3     S2        A2    A3    A3    A2
    //v2 : B1     B4     B4     B1        X1    X4    X4    X1
    //v3 : B3     B2     B2     B3        X3    X2    X2    X3

    trn1      v4.8h, v0.8h, v1.8h
    trn2      v5.8h, v0.8h, v1.8h
    trn1      v6.8h, v2.8h, v3.8h
    trn2      v7.8h, v2.8h, v3.8h

    trn1      v0.4s, v4.4s, v6.4s
    trn2      v2.4s, v4.4s, v6.4s
    trn1      v1.4s, v5.4s, v7.4s
    trn2      v3.4s, v5.4s, v7.4s

    add       v4.8h, v0.8h, v3.8h
    add       v5.8h, v1.8h, v2.8h
    //v4 : S1     S2     B1     B2      A1    A2    X1    X2
    //v5 : S4     S3     B4     B3      A4    A3    X4    X3

    //compute sad for each 4x4 block
    add       v6.8h, v4.8h, v5.8h
    addp      v19.8h, v6.8h, v6.8h
    //duplicate the sad into 128 bit so that we can compare using 128bit
    add       v31.4h, v31.4h, v19.4h

    //sad_2 = sad_1<<1;
    shl       v28.8h, v19.8h, #1

    //sad_2 - pu2_thrsh
    sub       v24.8h, v28.8h, v20.8h
    sub       v25.8h, v28.8h, v21.8h
    sub       v26.8h, v28.8h, v22.8h
    sub       v27.8h, v28.8h, v23.8h

    trn1      v0.4s, v4.4s, v5.4s
    trn2      v1.4s, v4.4s, v5.4s
    //v0 : S1     S2     S4     S3      A1    A2    A4    A3
    //v1 : B1     B2     B4     B3      X1    X2    X4    X3

    trn1      v4.8h, v0.8h, v1.8h
    trn2      v5.8h, v0.8h, v1.8h
    //v4 : S1     B1     S4     B4      A1    X1    A4    X4
    //v5 : S2     B2     S3     B3      A2    X2    A3    X3

    mov       v7.s[0], v4.s[1]
    mov       v7.s[1], v4.s[3]
    mov       v6.s[0], v5.s[1]          // V4 //S1 B1 A1 X1
    mov       v6.s[1], v5.s[3]          // V5 //S2 B2 A2 X2
    mov       v4.s[1], v4.s[2]          // V6 //S3 B3 A3 X3
    mov       v5.s[1], v5.s[2]          // V7 //S4 B4 A4 X4

    shl       v0.4h, v4.4h, #1          //S1<<1
    shl       v1.4h, v5.4h, #1          //S2<<1
    shl       v2.4h, v6.4h, #1          //S3<<1
    shl       v3.4h, v7.4h, #1          //S4<<1

    add       v8.4h, v5.4h, v6.4h       //(s2[j] + s3[j]))
    add       v9.4h, v4.4h, v7.4h       //(s1[j] + s4[j]))
    add       v10.4h, v6.4h, v7.4h      //(s3[j] + s4[j]))
    sub       v11.4h, v6.4h, v0.4h      //(s3[j] - (s1[j]<<1))
    sub       v12.4h, v7.4h, v1.4h      //(s4[j] - (s2[j]<<1))
    add       v13.4h, v4.4h, v5.4h      //(s1[j] + s2[j]))
    sub       v14.4h, v5.4h, v3.4h      //(s2[j] - (s4[j]<<1)))
    sub       v15.4h, v4.4h, v2.4h      //(s1[j] - (s3[j]<<1)))

    mov       v8.d[1], v9.d[0]
    mov       v10.d[1], v11.d[0]
    mov       v12.d[1], v13.d[0]
    mov       v14.d[1], v15.d[0]

    cmge      v0.8h, v24.8h, v8.8h      //ls1 ls2
    cmge      v1.8h, v25.8h, v10.8h     //ls3 ls4
    cmge      v2.8h, v26.8h, v12.8h     //ls5 ls6
    cmge      v3.8h, v27.8h, v14.8h     //ls7 ls8
    cmge      v4.4h, v19.4h, v29.4h     //sad

    orr       v0.16b, v0.16b, v1.16b
    orr       v2.16b, v2.16b, v3.16b
    orr       v2.16b, v0.16b, v2.16b
    xtn       v2.8b, v2.8h
    orr       v2.8b, v2.8b, v4.8b

    //if the comparison is non zero, out
    mov       x4, v2.d[0]
    cmp       x4, #0
    bne       core_loop_compute_sad_pre

    subs      x7, x7, #1
    bne       core_loop_satqd_ime_compute_satqd_16x16_lumainter
    b         satdq_end_func


core_loop_compute_sad:
    ld1       {v0.16b}, [x0], x2
    ld1       {v1.16b}, [x1], x3
    ld1       {v2.16b}, [x0], x2
    ld1       {v3.16b}, [x1], x3

    uabal     v31.8h, v0.8b, v1.8b
    uabal2    v31.8h, v0.16b, v1.16b

    uabal     v31.8h, v2.8b, v3.8b
    uabal2    v31.8h, v2.16b, v3.16b

    ld1       {v4.16b}, [x0], x2
    ld1       {v5.16b}, [x1], x3
    ld1       {v6.16b}, [x0], x2
    ld1       {v7.16b}, [x1], x3

    uabal     v31.8h, v4.8b, v5.8b
    uabal2    v31.8h, v4.16b, v5.16b

    uabal     v31.8h, v6.8b, v7.8b
    uabal2    v31.8h, v6.16b, v7.16b

core_loop_compute_sad_pre:
    subs      x7, x7, #1
    bne       core_loop_compute_sad

satdq_end_func:

    mov       x7, #1
    cmp       x4, #0
    csel      x7, x4, x7, eq
    str       w7, [x6]

    addp      v31.8h, v31.8h, v31.8h
    uaddlp    v31.4s, v31.8h
    addp      v31.2s, v31.2s, v31.2s
    st1       {v31.s}[0], [x5]


    ldp       d14, d15, [sp], #16
    ldp       d12, d13, [sp], #16
    ldp       d10, d11, [sp], #16
    ldp       d8, d9, [sp], #16
    pop_v_regs
    ret
