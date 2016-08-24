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

.text
.p2align 2

@/*****************************************************************************
@*                                                                            *
@*  Function Name    : IH264D_CXA8_YUV420toYUV420SP_UV()                      *
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
@*  Register Usage   : R0 - R14                                               *
@*                                                                            *
@*  Stack Usage      : 40 Bytes                                               *
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
    .global ih264e_fmt_conv_420p_to_420sp_a9q

ih264e_fmt_conv_420p_to_420sp_a9q:

    @// push the registers on the stack
    stmfd         sp!, {r4-r12, lr}

    ldr           r4, [sp, #72]         @// Load convert_uv_only

    cmp           r4, #1
    beq           yuv420sp_uv_chroma
    @/* Do the preprocessing before the main loops start */
    @// Load the parameters from stack
    ldr           r4, [sp, #44]         @// Load u2_height from stack
    ldr           r5, [sp, #48]         @// Load u2_width from stack
    ldr           r7, [sp, #52]         @// Load u2_stridey from stack
    ldr           r8, [sp, #64]         @// Load u2_dest_stride_y from stack
    sub           r7, r7, r5            @// Source increment
    sub           r8, r8, r5            @// Destination increment

yuv420sp_uv_row_loop_y:
    mov           r6, r5

yuv420sp_uv_col_loop_y:
    pld           [r0, #128]
    vld1.8        {d0, d1}, [r0]!
    vst1.8        {d0, d1}, [r3]!
    sub           r6, r6, #16
    cmp           r6, #15
    bgt           yuv420sp_uv_col_loop_y

    cmp           r6, #0
    beq           yuv420sp_uv_row_loop_end_y
    @//If non-multiple of 16, then go back by few bytes to ensure 16 bytes can be read
    @//Ex if width is 162, above loop will process 160 pixels. And
    @//Both source and destination will point to 146th pixel and then 16 bytes will be read
    @// and written using VLD1 and VST1
    rsb           r6, r6, #16
    sub           r0, r0, r6
    sub           r3, r3, r6

    vld1.8        {d0, d1}, [r0]!
    vst1.8        {d0, d1}, [r3]!

yuv420sp_uv_row_loop_end_y:
    add           r0, r0, r7
    add           r3, r3, r8
    subs          r4, r4, #1
    bgt           yuv420sp_uv_row_loop_y

yuv420sp_uv_chroma:

    ldr           r3, [sp, #40]         @// Load pu1_dest_uv from stack

    ldr           r4, [sp, #44]         @// Load u2_height from stack

    ldr           r5, [sp, #48]         @// Load u2_width from stack


    ldr           r7, [sp, #56]         @// Load u2_strideu from stack

    ldr           r8, [sp, #68]         @// Load u2_dest_stride_uv from stack

    sub           r7, r7, r5, lsr #1    @// Source increment

    sub           r8, r8, r5            @// Destination increment

    mov           r5, r5, lsr #1
    mov           r4, r4, lsr #1
    ldr           r3, [sp, #40]         @// Load pu1_dest_uv from stack

yuv420sp_uv_row_loop_uv:
    mov           r6, r5


yuv420sp_uv_col_loop_uv:
    pld           [r1, #128]
    pld           [r2, #128]
    vld1.8        d0, [r1]!
    vld1.8        d1, [r2]!
    vst2.8        {d0, d1}, [r3]!
    sub           r6, r6, #8
    cmp           r6, #7
    bgt           yuv420sp_uv_col_loop_uv

    cmp           r6, #0
    beq           yuv420sp_uv_row_loop_end_uv
    @//If non-multiple of 16, then go back by few bytes to ensure 16 bytes can be read
    @//Ex if width is 162, above loop will process 160 pixels. And
    @//Both source and destination will point to 146th pixel and then 16 bytes will be read
    @// and written using VLD1 and VST1
    rsb           r6, r6, #8
    sub           r1, r1, r6
    sub           r2, r2, r6
    sub           r3, r3, r6, lsl #1

    vld1.8        d0, [r1]!
    vld1.8        d1, [r2]!
    vst2.8        {d0, d1}, [r3]!

yuv420sp_uv_row_loop_end_uv:
    add           r1, r1, r7
    add           r2, r2, r7
    add           r3, r3, r8
    subs          r4, r4, #1
    bgt           yuv420sp_uv_row_loop_uv
    @//POP THE REGISTERS
    ldmfd         sp!, {r4-r12, pc}





@ /**
@ *******************************************************************************
@ *
@ * @brief ih264e_fmt_conv_422i_to_420sp_a9q
@ *     Function used from format conversion or frame copy
@ *
@ *
@ *
@ *Inputs             : r0 - pu1_y            -   UWORD8 pointer to y plane.
@ *                     r1 - pu1_u            -   UWORD8 pointer to u plane.
@ *                     r2 - pu1_v            -   UWORD8 pointer to u plane.
@ *                     r3 - pu2_yuv422i      -   UWORD16 pointer to yuv422iimage.
@ *             stack + 40 - u4_width         -   Width of the Y plane.
@ *                     44 - u4_height        -   Height of the Y plane.
@ *                     48 - u4_stride_y      -   Stride in pixels of Y plane.
@ *                     52 - u4_stride_u      -   Stride in pixels of U plane.
@ *                     56 - u4_stride_v      -   Stride in pixels of V plane.
@ *                     60 - u4_stride_yuv422i-   Stride in pixels of yuv422i image.
@ *
@ * @par   Description
@ * Function used from copying or converting a reference frame to display buffer
@ * in non shared mode
@ *
@ * @param[in] pu1_y_dst
@ *   Output Y pointer
@ *
@ * @param[in] pu1_u_dst
@ *   Output U/UV pointer ( UV is interleaved in the same format as that of input)
@ *
@ * @param[in] pu1_v_dst
@ *   Output V pointer ( used in 420P output case)
@ *
@ * @param[in] u4_dst_y_strd
@ *   Stride of destination Y buffer
@ *
@ * @param[in] u4_dst_u_strd
@ *   Stride of destination  U/V buffer
@ *
@ *
@ * @param[in] blocking
@ *   To indicate whether format conversion should wait till frame is reconstructed
@ *   and then return after complete copy is done. To be set to 1 when called at the
@ *   end of frame processing and set to 0 when called between frame processing modules
@ *   in order to utilize available MCPS
@ *
@ * @returns Error from IH264E_ERROR_T
@ *
@ * @remarks
@ * Assumes that the stride of U and V buffers are same.
@ * This is correct in most cases
@ * If a case comes where this is not true we need to modify the fmt conversion funcnions called inside also
@ * Since we read 4 pixels ata time the width should be aligned to 4
@ * In assembly width should be aligned to 16 and height to 2.
@ *
@ *
@ * Revision History :
@ *         DD MM YYYY   Author(s)              Changes (Describe the changes made)
@ *         07 06 2010   Harinarayanan K K       Adapeted to 422p
@ *
@ *******************************************************************************
@ */

@//`
@*/
    .global ih264e_fmt_conv_422i_to_420sp_a9q
ih264e_fmt_conv_422i_to_420sp_a9q:
    stmfd         sp!, {r4-r12, lr}     @// Back the register which are used



    @/* Do the preprocessing before the main loops start */
    @// Load the parameters from stack
    ldr           r4, [sp, #48]         @// Load u4_stride_y       from stack

    ldr           r5, [sp, #60]         @// Load u4_stride_yuv422i from stack
    add           r6, r0, r4            @// pu1_y_nxt_row       = pu1_y + u4_stride_y

    ldr           r7, [sp, #40]         @// Load u4_width          from stack
    add           r8, r3, r5, lsl #1    @// pu2_yuv422i_nxt_row = pu2_yuv422i_y + u4_stride_yuv422i(2 Bytes for each pixel)

    ldr           r9, [sp, #52]         @// Load u4_stride_u       from stack
    sub           r12, r4, r7           @// u2_offset1          = u4_stride_y - u4_width

@LDR            r10,[sp,#56]                ;// Load u4_stride_v       from stack
    sub           r14, r5, r7           @// u2_offset_yuv422i   = u4_stride_yuv422i - u4_width

    ldr           r11, [sp, #44]        @// Load u4_height         from stack
    sub           r9, r9, r7            @// u2_offset2          = u4_stride_u - u4_width >> 1

@   SUB         r10,r10,r7,ASR #1           ;// u2_offset3          = u4_stride_v - u4_width >> 1
    mov           r14, r14, lsl #1      @// u2_offset_yuv422i   = u2_offset_yuv422i * 2

    mov           r11, r11, asr #1      @// u4_width = u4_width / 2 (u4_width >> 1)

    add           r4, r12, r4           @// u2_offset1 = u2_offset1 + u4_stride_y
    add           r5, r14, r5, lsl #1   @// u2_offset_yuv422i = u2_offset_yuv422i + u4_stride_yuv422i

@// Register Assignment
@// pu1_y               - r0
@// pu1_y_nxt_row       - r6
@// pu1_u               - r1
@// pu1_v               - r2
@// pu2_yuv422i         - r3
@// pu2_yuv422i_nxt_row - r8
@// u2_offset1          - r4
@// u2_offset2          - r9
@// u2_offset3          - r10
@// u2_offset_yuv422i   - r5
@// u4_width / 16       - r7
@// u4_height / 2       - r11
@// inner loop count    - r12
yuv422i_to_420sp_height_loop:

    mov           r12, r7               @// Inner loop count = u4_width / 16

yuv422i_to_420sp_width_loop:
    vld4.8        {d0, d1, d2, d3}, [r3]! @// Load the 16 elements of row 1
    vld4.8        {d4, d5, d6, d7}, [r8]! @// Load the 16 elements of row 2
    sub           r12, r12, #16

    vrhadd.u8     d0, d0, d4
    vrhadd.u8     d2, d2, d6

    vst2.8        {d1, d3}, [r0]!       @// Store the 16 elements of row1 Y
    vst2.8        {d5, d7}, [r6]!       @// Store the 16 elements of row2 Y

    vst2.8        {d0, d2}, [r1]!       @// Store the 8 elements of row1/2 U

    cmp           r12, #15
    bgt           yuv422i_to_420sp_width_loop
    cmp           r12, #0
    beq           yuv422i_to_420sp_row_loop_end

    @//If non-multiple of 16, then go back by few bytes to ensure 16 bytes can be read
    @//Ex if width is 162, above loop will process 160 pixels. And
    @//Both source and destination will point to 146th pixel and then 16 bytes will be read
    @// and written using VLD1 and VST1
    rsb           r12, r12, #16
    sub           r3, r3, r12, lsl #1
    sub           r8, r8, r12, lsl #1
    sub           r0, r0, r12
    sub           r6, r6, r12
    sub           r1, r1, r12

    vld4.8        {d0, d1, d2, d3}, [r3]! @// Load the 16 elements of row 1
    vld4.8        {d4, d5, d6, d7}, [r8]! @// Load the 16 elements of row 2

    vrhadd.u8     d0, d0, d4
    vrhadd.u8     d2, d2, d6

    vst2.8        {d1, d3}, [r0]!       @// Store the 16 elements of row1 Y
    vst2.8        {d5, d7}, [r6]!       @// Store the 16 elements of row2 Y

    vst2.8        {d0, d2}, [r1]!       @// Store the 8 elements of row1/2 U

yuv422i_to_420sp_row_loop_end:
    @// Update the buffer pointer so that they will refer to next pair of rows
    add           r0, r0, r4            @// pu1_y               = pu1_y                 + u2_offset1
    add           r6, r6, r4            @// pu1_y_nxt_row       = pu1_y_nxt_row         + u2_offset1

    add           r1, r1, r9            @// pu1_u               = pu1_u                 + u2_offset2
    subs          r11, r11, #1

    add           r3, r3, r5            @// pu2_yuv422i         = pu2_yuv422i           + u2_offset_yuv422i

    add           r8, r8, r5            @// pu2_yuv422i_nxt_row = pu2_yuv422i_nxt_row   + u2_offset_yuv422i
    bgt           yuv422i_to_420sp_height_loop
    ldmfd         sp!, {r4-r12, pc}     @// Restore the register which are used



