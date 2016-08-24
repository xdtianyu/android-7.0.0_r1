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

@/*
@//----------------------------------------------------------------------------
@// File Name            : impeg2_format_conv.s
@//
@// Description          : This file has the Idct Implementations for the
@//                        MPEG4 SP decoder on neon platform.
@//
@// Reference Document   :
@//
@// Revision History     :
@//      Date            Author                  Detail Description
@//   ------------    ----------------    ----------------------------------
@//   Jul 07, 2008     Naveen Kumar T                Created
@//
@//-------------------------------------------------------------------------
@*/

@/*
@// ----------------------------------------------------------------------------
@// Include Files
@// ----------------------------------------------------------------------------
@*/
.text
.p2align 2
.equ log2_16 ,  4
.equ log2_2  ,  1
@/*
@// ----------------------------------------------------------------------------
@// Struct/Union Types and Define
@// ----------------------------------------------------------------------------
@*/

@/*
@// ----------------------------------------------------------------------------
@// Static Global Data section variables
@// ----------------------------------------------------------------------------
@*/
@//--------------------------- NONE --------------------------------------------

@/*
@// ----------------------------------------------------------------------------
@// Static Prototype Functions
@// ----------------------------------------------------------------------------
@*/
@// -------------------------- NONE --------------------------------------------

@/*
@// ----------------------------------------------------------------------------
@// Exported functions
@// ----------------------------------------------------------------------------
@*/

@/*****************************************************************************
@*                                                                            *
@*  Function Name    : impeg2_fmt_conv_yuv420p_to_yuv420sp_uv_a9q()                      *
@*                                                                            *
@*  Description      : This function conversts the image from YUV420P color   *
@*                     space to 420SP color space(UV interleaved).        *
@*                                                                            *
@*  Arguments        : R0           pu1_y                                     *
@*                     R1           pu1_u                                     *
@*                     R2           pu1_v                                     *
@*                     R3           pu1_dest_y                                *
@*                     [R13 #40]    pu1_dest_uv                               *
@*                     [R13 #44]    u2_height                                 *
@*                     [R13 #48]    u2_width                                  *
@*                     [R13 #52]    u2_stridey                                *
@*                     [R13 #56]    u2_strideu                                *
@*                     [R13 #60]    u2_stridev                                *
@*                     [R13 #64]    u2_dest_stride_y                          *
@*                     [R13 #68]    u2_dest_stride_uv                         *
@*                     [R13 #72]    convert_uv_only                           *
@*                                                                            *
@*  Values Returned  : None                                                   *
@*                                                                            *
@*  Register Usage   : R0 - R8, Q0                                            *
@*                                                                            *
@*  Stack Usage      : 24 Bytes                                               *
@*                                                                            *
@*  Interruptibility : Interruptible                                          *
@*                                                                            *
@*  Known Limitations                                                         *
@*       Assumptions: Image Width:     Assumed to be multiple of 16 and       *
@*                     greater than or equal to 16                *
@*                     Image Height:    Assumed to be even.                   *
@*                                                                            *
@*  Revision History :                                                        *
@*         DD MM YYYY   Author(s)       Changes (Describe the changes made)   *
@*         07 06 2010   Varshita        Draft                                 *
@*         07 06 2010   Naveen Kr T     Completed                             *
@*                                                                            *
@*****************************************************************************/
                .global impeg2_fmt_conv_yuv420p_to_yuv420sp_uv_a9q
impeg2_fmt_conv_yuv420p_to_yuv420sp_uv_a9q:

    @// push the registers on the stack
    stmfd           sp!, {r4-r8, lr}

    ldr             r4, [sp, #56]       @// Load convert_uv_only

    cmp             r4, #1
    beq             yuv420sp_uv_chroma
    @/* Do the preprocessing before the main loops start */
    @// Load the parameters from stack
    ldr             r4, [sp, #28]       @// Load u2_height from stack

    ldr             r5, [sp, #32]       @// Load u2_width from stack

    ldr             r7, [sp, #36]       @// Load u2_stridey from stack

    ldr             r8, [sp, #48]       @// Load u2_dest_stride_y from stack

    sub             r7, r7, r5          @// Source increment

    sub             r8, r8, r5          @// Destination increment


yuv420sp_uv_row_loop_y:
    mov             r6, r5

yuv420sp_uv_col_loop_y:
    pld             [r0, #128]
    vld1.8          {q0}, [r0]!
    vst1.8          {q0}, [r3]!
    sub             r6, r6, #16
    cmp             r6, #15
    bgt             yuv420sp_uv_col_loop_y

    cmp             r6, #0
    beq             yuv420sp_uv_row_loop_end_y
    @//If non-multiple of 16, then go back by few bytes to ensure 16 bytes can be read
    @//Ex if width is 162, above loop will process 160 pixels. And
    @//Both source and destination will point to 146th pixel and then 16 bytes will be read
    @// and written using VLD1 and VST1
    rsb             r6, r6, #16
    sub             r0, r0, r6
    sub             r3, r3, r6

    vld1.8          {q0}, [r0]!
    vst1.8          {q0}, [r3]!

yuv420sp_uv_row_loop_end_y:
    add             r0, r0, r7
    add             r3, r3, r8
    subs            r4, r4, #1
    bgt             yuv420sp_uv_row_loop_y

yuv420sp_uv_chroma:

    ldr             r3, [sp, #24]       @// Load pu1_dest_uv from stack

    ldr             r4, [sp, #28]       @// Load u2_height from stack

    ldr             r5, [sp, #32]       @// Load u2_width from stack


    ldr             r7, [sp, #40]       @// Load u2_strideu from stack

    ldr             r8, [sp, #52]       @// Load u2_dest_stride_uv from stack

    sub             r7, r7, r5, lsr #1  @// Source increment

    sub             r8, r8, r5          @// Destination increment

    mov             r5, r5, lsr #1
    mov             r4, r4, lsr #1
    ldr             r3, [sp, #24]       @// Load pu1_dest_uv from stack
yuv420sp_uv_row_loop_uv:
    mov             r6, r5


yuv420sp_uv_col_loop_uv:
    pld             [r1, #128]
    pld             [r2, #128]
    vld1.8          d0, [r1]!
    vld1.8          d1, [r2]!
    vst2.8          {d0, d1}, [r3]!
    sub             r6, r6, #8
    cmp             r6, #7
    bgt             yuv420sp_uv_col_loop_uv

    cmp             r6, #0
    beq             yuv420sp_uv_row_loop_end_uv
    @//If non-multiple of 16, then go back by few bytes to ensure 16 bytes can be read
    @//Ex if width is 162, above loop will process 160 pixels. And
    @//Both source and destination will point to 146th pixel and then 16 bytes will be read
    @// and written using VLD1 and VST1
    rsb             r6, r6, #8
    sub             r1, r1, r6
    sub             r2, r2, r6
    sub             r3, r3, r6, lsl #1

    vld1.8          d0, [r1]!
    vld1.8          d1, [r2]!
    vst2.8          {d0, d1}, [r3]!

yuv420sp_uv_row_loop_end_uv:
    add             r1, r1, r7
    add             r2, r2, r7
    add             r3, r3, r8
    subs            r4, r4, #1
    bgt             yuv420sp_uv_row_loop_uv
    @//POP THE REGISTERS
    ldmfd           sp!, {r4-r8, pc}





@/*****************************************************************************
@*                                                                            *
@*  Function Name    : impeg2_fmt_conv_yuv420p_to_yuv420sp_vu_a9q()                      *
@*                                                                            *
@*  Description      : This function conversts the image from YUV420P color   *
@*                     space to 420SP color space(VU interleaved).        *
@*             This function is similar to above function         *
@*             IMP4D_CXA8_YUV420toYUV420SP_VU with a difference in   *
@*             VLD1.8 for chroma - order of registers is different    *
@*                                                                            *
@*  Arguments        : R0           pu1_y                                     *
@*                     R1           pu1_u                                     *
@*                     R2           pu1_v                                     *
@*                     R3           pu1_dest_y                                *
@*                     [R13 #40]    pu1_dest_uv                               *
@*                     [R13 #44]    u2_height                                 *
@*                     [R13 #48]    u2_width                                  *
@*                     [R13 #52]    u2_stridey                                *
@*                     [R13 #56]    u2_strideu                                *
@*                     [R13 #60]    u2_stridev                                *
@*                     [R13 #64]    u2_dest_stride_y                          *
@*                     [R13 #68]    u2_dest_stride_uv                         *
@*                     [R13 #72]    convert_uv_only                           *
@*                                                                            *
@*  Values Returned  : None                                                   *
@*                                                                            *
@*  Register Usage   : R0 - R8, Q0                                            *
@*                                                                            *
@*  Stack Usage      : 24 Bytes                                               *
@*                                                                            *
@*  Interruptibility : Interruptible                                          *
@*                                                                            *
@*  Known Limitations                                                         *
@*       Assumptions: Image Width:     Assumed to be multiple of 16 and       *
@*                     greater than or equal to 16                *
@*                     Image Height:    Assumed to be even.                   *
@*                                                                            *
@*  Revision History :                                                        *
@*         DD MM YYYY   Author(s)       Changes (Describe the changes made)   *
@*         07 06 2010   Varshita        Draft                                 *
@*         07 06 2010   Naveen Kr T     Completed                             *
@*                                                                            *
@*****************************************************************************/

                .global impeg2_fmt_conv_yuv420p_to_yuv420sp_vu_a9q
impeg2_fmt_conv_yuv420p_to_yuv420sp_vu_a9q:

    @// push the registers on the stack
    stmfd           sp!, {r4-r8, lr}

    ldr             r4, [sp, #56]       @// Load convert_uv_only

    cmp             r4, #1
    beq             yuv420sp_vu_chroma

    @/* Do the preprocessing before the main loops start */
    @// Load the parameters from stack
    ldr             r4, [sp, #28]       @// Load u2_height from stack

    ldr             r5, [sp, #32]       @// Load u2_width from stack

    ldr             r7, [sp, #36]       @// Load u2_stridey from stack

    ldr             r8, [sp, #48]       @// Load u2_dest_stride_y from stack

    sub             r7, r7, r5          @// Source increment

    sub             r8, r8, r5          @// Destination increment


yuv420sp_vu_row_loop_y:
    mov             r6, r5

yuv420sp_vu_col_loop_y:
    pld             [r0, #128]
    vld1.8          {q0}, [r0]!
    vst1.8          {q0}, [r3]!
    sub             r6, r6, #16
    cmp             r6, #15
    bgt             yuv420sp_vu_col_loop_y

    cmp             r6, #0
    beq             yuv420sp_vu_row_loop_end_y
    @//If non-multiple of 16, then go back by few bytes to ensure 16 bytes can be read
    @//Ex if width is 162, above loop will process 160 pixels. And
    @//Both source and destination will point to 146th pixel and then 16 bytes will be read
    @// and written using VLD1 and VST1
    rsb             r6, r6, #16
    sub             r0, r0, r6
    sub             r3, r3, r6

    vld1.8          {q0}, [r0]!
    vst1.8          {q0}, [r3]!

yuv420sp_vu_row_loop_end_y:
    add             r0, r0, r7
    add             r3, r3, r8
    subs            r4, r4, #1
    bgt             yuv420sp_vu_row_loop_y

yuv420sp_vu_chroma:

    ldr             r3, [sp, #24]       @// Load pu1_dest_uv from stack

    ldr             r4, [sp, #28]       @// Load u2_height from stack

    ldr             r5, [sp, #32]       @// Load u2_width from stack


    ldr             r7, [sp, #40]       @// Load u2_strideu from stack

    ldr             r8, [sp, #52]       @// Load u2_dest_stride_uv from stack

    sub             r7, r7, r5, lsr #1  @// Source increment

    sub             r8, r8, r5          @// Destination increment

    mov             r5, r5, lsr #1
    mov             r4, r4, lsr #1
    ldr             r3, [sp, #24]       @// Load pu1_dest_uv from stack
yuv420sp_vu_row_loop_uv:
    mov             r6, r5


yuv420sp_vu_col_loop_uv:
    pld             [r1, #128]
    pld             [r2, #128]
    vld1.8          d1, [r1]!
    vld1.8          d0, [r2]!
    vst2.8          {d0, d1}, [r3]!
    sub             r6, r6, #8
    cmp             r6, #7
    bgt             yuv420sp_vu_col_loop_uv

    cmp             r6, #0
    beq             yuv420sp_vu_row_loop_end_uv
    @//If non-multiple of 16, then go back by few bytes to ensure 16 bytes can be read
    @//Ex if width is 162, above loop will process 160 pixels. And
    @//Both source and destination will point to 146th pixel and then 16 bytes will be read
    @// and written using VLD1 and VST1
    rsb             r6, r6, #8
    sub             r1, r1, r6
    sub             r2, r2, r6
    sub             r3, r3, r6, lsl #1

    vld1.8          d1, [r1]!
    vld1.8          d0, [r2]!
    vst2.8          {d0, d1}, [r3]!

yuv420sp_vu_row_loop_end_uv:
    add             r1, r1, r7
    add             r2, r2, r7
    add             r3, r3, r8
    subs            r4, r4, #1
    bgt             yuv420sp_vu_row_loop_uv
    @//POP THE REGISTERS
    ldmfd           sp!, {r4-r8, pc}





