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

///*
////----------------------------------------------------------------------------
//// File Name            : impeg2_format_conv.s
////
//// Description          : This file has the Idct Implementations for the
////                        MPEG4 SP decoder on neon platform.
////
//// Reference Document   :
////
//// Revision History     :
////      Date            Author                  Detail Description
////   ------------    ----------------    ----------------------------------
////   Jul 07, 2008     Naveen Kumar T                Created
////
////-------------------------------------------------------------------------
//*/

///*
//// ----------------------------------------------------------------------------
//// Include Files
//// ----------------------------------------------------------------------------
//*/
.set log2_16                    ,      4
.set log2_2                     ,      1

.text
.include "impeg2_neon_macros.s"
///*
//// ----------------------------------------------------------------------------
//// Struct/Union Types and Define
//// ----------------------------------------------------------------------------
//*/

///*
//// ----------------------------------------------------------------------------
//// Static Global Data section variables
//// ----------------------------------------------------------------------------
//*/
////--------------------------- NONE --------------------------------------------

///*
//// ----------------------------------------------------------------------------
//// Static Prototype Functions
//// ----------------------------------------------------------------------------
//*/
//// -------------------------- NONE --------------------------------------------

///*
//// ----------------------------------------------------------------------------
//// Exported functions
//// ----------------------------------------------------------------------------
//*/


///*****************************************************************************
//*                                                                            *
//*  Function Name    : impeg2_fmt_conv_yuv420p_to_yuv420sp_uv_av8()                      *
//*                                                                            *
//*  Description      : This function conversts the image from YUV420P color   *
//*                     space to 420SP color space(UV interleaved).           *
//*                                                                            *
//*  Arguments        : x0          pu1_y                                     *
//*                     x1          pu1_u                                     *
//*                     x2          pu1_v                                     *
//*                     x3          pu1_dest_y                                *
//*                     x4          pu1_dest_uv                               *
//*                     x5          u2_height                                 *
//*                     x6          u2_width                                  *
//*                     x7          u2_stridey                                *
//*                     sp, #80     u2_strideu                                *
//*                     sp, #88     u2_stridev                                *
//*                     sp, #96     u2_dest_stride_y                          *
//*                     sp, #104    u2_dest_stride_uv                         *
//*                     sp, #112    convert_uv_only                           *
//*                                                                            *
//*  Values Returned  : None                                                   *
//*                                                                            *
//*  Register Usage   : x8, x10, x16, x20, v0, v1                              *
//*                                                                            *
//*  Stack Usage      : 80 Bytes                                               *
//*                                                                            *
//*  Interruptibility : Interruptible                                          *
//*                                                                            *
//*  Known Limitations                                                         *
//*       Assumptions: Image Width:     Assumed to be multiple of 16 and       *
//*                     greater than or equal to 16                  *
//*                     Image Height:    Assumed to be even.                   *
//*                                                                            *
//*  Revision History :                                                        *
//*         DD MM YYYY   Author(s)       Changes (Describe the changes made)   *
//*         07 06 2010   Varshita        Draft                                 *
//*         07 06 2010   Naveen Kr T     Completed                             *
//*                                                                            *
//*****************************************************************************/
.global impeg2_fmt_conv_yuv420p_to_yuv420sp_uv_av8
impeg2_fmt_conv_yuv420p_to_yuv420sp_uv_av8:

    //// push the registers on the stack
    //    pu1_y,                - x0
    //    pu1_u,                - x1
    //    pu1_v,                - x2
    //    pu1_dest_y,           - x3
    //    pu1_dest_uv,          - x4
    //    u2_height,            - x5
    //    u2_width,             - x6
    //    u2_stridey,           - x7
    //    u2_strideu,           - sp, #80
    //    u2_stridev,           - sp, #88
    //    u2_dest_stride_y,     - sp, #96
    //    u2_dest_stride_uv,    - sp, #104
    //    convert_uv_only       - sp, #112
    // STMFD sp!,{x4-x12,x14}
    push_v_regs
    stp             x19, x20, [sp, #-16]!

    ldr             w14, [sp, #112]     //// Load convert_uv_only

    cmp             w14, #1
    beq             yuv420sp_uv_chroma
    ///* Do the preprocessing before the main loops start */
    //// Load the parameters from stack

    ldr             w8, [sp, #96]       //// Load u2_dest_stride_y from stack
    uxtw            x8, w8

    sub             x7, x7, x6          //// Source increment

    sub             x8, x8, x6          //// Destination increment


yuv420sp_uv_row_loop_y:
    mov             x16, x6

yuv420sp_uv_col_loop_y:
    prfm            pldl1keep, [x0, #128]
    ld1             {v0.8b, v1.8b}, [x0], #16
    st1             {v0.8b, v1.8b}, [x3], #16
    sub             x16, x16, #16
    cmp             x16, #15
    bgt             yuv420sp_uv_col_loop_y

    cmp             x16, #0
    beq             yuv420sp_uv_row_loop__y
    ////If non-multiple of 16, then go back by few bytes to ensure 16 bytes can be read
    ////Ex if width is 162, above loop will process 160 pixels. And
    ////Both source and destination will point to 146th pixel and then 16 bytes will be read
    //// and written using VLD1 and VST1
    sub             x20, x16, #16
    neg             x16, x20
    sub             x0, x0, x16
    sub             x3, x3, x16

    ld1             {v0.8b, v1.8b}, [x0], #16
    st1             {v0.8b, v1.8b}, [x3], #16

yuv420sp_uv_row_loop__y:
    add             x0, x0, x7
    add             x3, x3, x8
    subs            x5, x5, #1
    bgt             yuv420sp_uv_row_loop_y

yuv420sp_uv_chroma:
    ldr             w7, [sp, #88]       //// Load u2_strideu from stack
    sxtw            x7, w7

    ldr             w8, [sp, #104]      //// Load u2_dest_stride_uv from stack
    sxtw            x8, w8

    sub             x7, x7, x6, lsr #1  //// Source increment

    sub             x8, x8, x6          //// Destination increment

    lsr             x6, x6, #1
    lsr             x5, x5, #1
yuv420sp_uv_row_loop_uv:
    mov             x16, x6


yuv420sp_uv_col_loop_uv:
    prfm            pldl1keep, [x1, #128]
    prfm            pldl1keep, [x2, #128]

    ld1             {v0.8b}, [x1], #8
    ld1             {v1.8b}, [x2], #8
    st2             {v0.8b, v1.8b}, [x4], #16

    sub             x16, x16, #8
    cmp             x16, #7
    bgt             yuv420sp_uv_col_loop_uv

    cmp             x16, #0
    beq             yuv420sp_uv_row_loop__uv
    ////If non-multiple of 16, then go back by few bytes to ensure 16 bytes can be read
    ////Ex if width is 162, above loop will process 160 pixels. And
    ////Both source and destination will point to 146th pixel and then 16 bytes will be read
    //// and written using VLD1 and VST1
    sub             x20, x16, #8
    neg             x16, x20
    sub             x1, x1, x16
    sub             x2, x2, x16
    sub             x4, x4, x16, lsl #1

    ld1             {v0.8b}, [x1], #8
    ld1             {v1.8b}, [x2], #8
    st2             {v0.8b, v1.8b}, [x4], #16

yuv420sp_uv_row_loop__uv:
    add             x1, x1, x7
    add             x2, x2, x7
    add             x4, x4, x8
    subs            x5, x5, #1
    bgt             yuv420sp_uv_row_loop_uv
    ////POP THE REGISTERS
    // LDMFD sp!,{x4-x12,PC}
    ldp             x19, x20, [sp], #16
    pop_v_regs
    ret





///*****************************************************************************
//*                                                                            *
//*  Function Name    : impeg2_fmt_conv_yuv420p_to_yuv420sp_vu_av8()                      *
//*                                                                            *
//*  Description      : This function conversts the image from YUV420P color   *
//*                     space to 420SP color space(VU interleaved).           *
//*               This function is similar to above function          *
//*               IMP4D_CXA8_YUV420toYUV420SP_VU with a difference in   *
//*               VLD1.8 for chroma - order of registers is different    *
//*                                                                            *
//*  Arguments        : x0          pu1_y                                     *
//*                     x1          pu1_u                                     *
//*                     x2          pu1_v                                     *
//*                     x3          pu1_dest_y                                *
//*                     x4          pu1_dest_uv                               *
//*                     x5          u2_height                                 *
//*                     x6          u2_width                                  *
//*                     x7          u2_stridey                                *
//*                     sp, #80     u2_strideu                                *
//*                     sp, #88     u2_stridev                                *
//*                     sp, #96     u2_dest_stride_y                          *
//*                     sp, #104    u2_dest_stride_uv                         *
//*                     sp, #112    convert_uv_only                           *
//*                                                                            *
//*  Values Returned  : None                                                   *
//*                                                                            *
//*  Register Usage   : x8, x14, x16, x20, v0, v1                              *
//*                                                                            *
//*  Stack Usage      : 80 Bytes                                               *
//*                                                                            *
//*  Interruptibility : Interruptible                                          *
//*                                                                            *
//*  Known Limitations                                                         *
//*       Assumptions: Image Width:     Assumed to be multiple of 16 and       *
//*                     greater than or equal to 16                  *
//*                     Image Height:    Assumed to be even.                   *
//*                                                                            *
//*  Revision History :                                                        *
//*         DD MM YYYY   Author(s)       Changes (Describe the changes made)   *
//*         07 06 2010   Varshita        Draft                                 *
//*         07 06 2010   Naveen Kr T     Completed                             *
//*                                                                            *
//*****************************************************************************/

.global impeg2_fmt_conv_yuv420p_to_yuv420sp_vu_av8
impeg2_fmt_conv_yuv420p_to_yuv420sp_vu_av8:

    //// push the registers on the stack
    //    pu1_y,                - x0
    //    pu1_u,                - x1
    //    pu1_v,                - x2
    //    pu1_dest_y,           - x3
    //    pu1_dest_uv,          - x4
    //    u2_height,            - x5
    //    u2_width,             - x6
    //    u2_stridey,           - x7
    //    u2_strideu,           - sp, #80
    //    u2_stridev,           - sp, #88
    //    u2_dest_stride_y,     - sp, #96
    //    u2_dest_stride_uv,    - sp, #104
    //    convert_uv_only       - sp, #112
    // STMFD sp!,{x4-x12,x14}
    push_v_regs
    stp             x19, x20, [sp, #-16]!

    ldr             w14, [sp, #112]     //// Load convert_uv_only

    cmp             w14, #1
    beq             yuv420sp_vu_chroma

    ///* Do the preprocessing before the main loops start */
    //// Load the parameters from stack

    ldr             w8, [sp, #96]       //// Load u2_dest_stride_y from stack
    uxtw            x8, w8

    sub             x7, x7, x6          //// Source increment

    sub             x8, x8, x6          //// Destination increment


yuv420sp_vu_row_loop_y:
    mov             x16, x6

yuv420sp_vu_col_loop_y:
    prfm            pldl1keep, [x0, #128]
    ld1             {v0.8b, v1.8b}, [x0], #16
    st1             {v0.8b, v1.8b}, [x3], #16
    sub             x16, x16, #16
    cmp             x16, #15
    bgt             yuv420sp_vu_col_loop_y

    cmp             x16, #0
    beq             yuv420sp_vu_row_loop__y
    ////If non-multiple of 16, then go back by few bytes to ensure 16 bytes can be read
    ////Ex if width is 162, above loop will process 160 pixels. And
    ////Both source and destination will point to 146th pixel and then 16 bytes will be read
    //// and written using VLD1 and VST1
    sub             x20, x16, #16
    neg             x16, x20
    sub             x0, x0, x16
    sub             x3, x3, x16

    ld1             {v0.8b, v1.8b}, [x0], #16
    st1             {v0.8b, v1.8b}, [x3], #16

yuv420sp_vu_row_loop__y:
    add             x0, x0, x7
    add             x3, x3, x8
    subs            x5, x5, #1
    bgt             yuv420sp_vu_row_loop_y

yuv420sp_vu_chroma:
    ldr             w7, [sp, #80]       //// Load u2_strideu from stack
    sxtw            x7, w7

    ldr             w8, [sp, #104]      //// Load u2_dest_stride_uv from stack
    sxtw            x8, w8

    sub             x7, x7, x6, lsr #1  //// Source increment

    sub             x8, x8, x6          //// Destination increment

    lsr             x6, x6, #1
    lsr             x5, x5, #1
yuv420sp_vu_row_loop_uv:
    mov             x16, x6


yuv420sp_vu_col_loop_uv:
    prfm            pldl1keep, [x1, #128]
    prfm            pldl1keep, [x2, #128]
    ld1             {v1.8b}, [x1], #8
    ld1             {v0.8b}, [x2], #8
    st2             {v0.8b, v1.8b}, [x4], #16
    sub             x16, x16, #8
    cmp             x16, #7
    bgt             yuv420sp_vu_col_loop_uv

    cmp             x16, #0
    beq             yuv420sp_vu_row_loop__uv
    ////If non-multiple of 16, then go back by few bytes to ensure 16 bytes can be read
    ////Ex if width is 162, above loop will process 160 pixels. And
    ////Both source and destination will point to 146th pixel and then 16 bytes will be read
    //// and written using VLD1 and VST1
    sub             x20, x16, #8
    neg             x16, x20
    sub             x1, x1, x16
    sub             x2, x2, x16
    sub             x4, x4, x16, lsl #1

    ld1             {v1.8b}, [x1], #8
    ld1             {v0.8b}, [x2], #8
    st2             {v0.8b, v1.8b}, [x4], #16

yuv420sp_vu_row_loop__uv:
    add             x1, x1, x7
    add             x2, x2, x7
    add             x4, x4, x8
    subs            x5, x5, #1
    bgt             yuv420sp_vu_row_loop_uv
    ////POP THE REGISTERS
    // LDMFD sp!,{x4-x12,PC}
    ldp             x19, x20, [sp], #16
    pop_v_regs
    ret

