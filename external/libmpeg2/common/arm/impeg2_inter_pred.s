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
@// File Name            : impeg2_inter_pred.s
@//
@// Description          : This file has motion compensation related
@//                        interpolation functions on Neon + CortexA-8 platform
@//
@// Reference Document   :
@//
@// Revision History     :
@//      Date            Author                  Detail Description
@//   ------------    ----------------    ----------------------------------
@//   18 jun 2010     S Hamsalekha              Created
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
@// -------------------------- NONE --------------------------------------------


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

@//---------------------------------------------------------------------------
@// Function Name      :   impeg2_copy_mb_a9q()
@//
@// Detail Description : Copies one MB worth of data from src to the dst
@//
@// Inputs             : r0 - pointer to src
@//                      r1 - pointer to dst
@//                      r2 - source width
@//                      r3 - destination width
@// Registers Used     : r4, r5, d0, d1
@//
@// Stack Usage        : 12 bytes
@//
@// Outputs            :
@//
@// Return Data        : None
@//
@// Programming Note   : <program limitation>
@//-----------------------------------------------------------------------------
@*/



        .global impeg2_copy_mb_a9q


impeg2_copy_mb_a9q:

    stmfd           r13!, {r4, r5, r14}


    ldr             r4, [r0]            @src->y
    ldr             r5, [r1]            @dst->y
    @Read one row of data from the src
    vld1.8          {d0, d1}, [r4], r2  @Load and increment src
    vst1.8          {d0, d1}, [r5], r3  @Store and increment dst

    @//Repeat 15 times for y
    vld1.8          {d0, d1}, [r4], r2  @Load and increment src
    vst1.8          {d0, d1}, [r5], r3  @Store and increment dst
    vld1.8          {d0, d1}, [r4], r2  @Load and increment src
    vst1.8          {d0, d1}, [r5], r3  @Store and increment dst
    vld1.8          {d0, d1}, [r4], r2  @Load and increment src
    vst1.8          {d0, d1}, [r5], r3  @Store and increment dst
    vld1.8          {d0, d1}, [r4], r2  @Load and increment src
    vst1.8          {d0, d1}, [r5], r3  @Store and increment dst
    vld1.8          {d0, d1}, [r4], r2  @Load and increment src
    vst1.8          {d0, d1}, [r5], r3  @Store and increment dst
    vld1.8          {d0, d1}, [r4], r2  @Load and increment src
    vst1.8          {d0, d1}, [r5], r3  @Store and increment dst
    vld1.8          {d0, d1}, [r4], r2  @Load and increment src
    vst1.8          {d0, d1}, [r5], r3  @Store and increment dst
    vld1.8          {d0, d1}, [r4], r2  @Load and increment src
    vst1.8          {d0, d1}, [r5], r3  @Store and increment dst
    vld1.8          {d0, d1}, [r4], r2  @Load and increment src
    vst1.8          {d0, d1}, [r5], r3  @Store and increment dst
    vld1.8          {d0, d1}, [r4], r2  @Load and increment src
    vst1.8          {d0, d1}, [r5], r3  @Store and increment dst
    vld1.8          {d0, d1}, [r4], r2  @Load and increment src
    vst1.8          {d0, d1}, [r5], r3  @Store and increment dst
    vld1.8          {d0, d1}, [r4], r2  @Load and increment src
    vst1.8          {d0, d1}, [r5], r3  @Store and increment dst
    vld1.8          {d0, d1}, [r4], r2  @Load and increment src
    vst1.8          {d0, d1}, [r5], r3  @Store and increment dst
    vld1.8          {d0, d1}, [r4], r2  @Load and increment src
    vst1.8          {d0, d1}, [r5], r3  @Store and increment dst
    vld1.8          {d0, d1}, [r4], r2  @Load and increment src
    vst1.8          {d0, d1}, [r5], r3  @Store and increment dst

    mov             r2, r2, lsr #1      @src_offset /= 2
    mov             r3, r3, lsr #1      @dst_offset /= 2

    ldr             r4, [r0, #4]        @src->u
    ldr             r5, [r1, #4]        @dst->u
    @Read one row of data from the src
    vld1.8          {d0}, [r4], r2      @Load and increment src
    vst1.8          {d0}, [r5], r3      @Store and increment dst

    @//Repeat 7 times for u
    vld1.8          {d0}, [r4], r2      @Load and increment src
    vst1.8          {d0}, [r5], r3      @Store and increment dst
    vld1.8          {d0}, [r4], r2      @Load and increment src
    vst1.8          {d0}, [r5], r3      @Store and increment dst
    vld1.8          {d0}, [r4], r2      @Load and increment src
    vst1.8          {d0}, [r5], r3      @Store and increment dst
    vld1.8          {d0}, [r4], r2      @Load and increment src
    vst1.8          {d0}, [r5], r3      @Store and increment dst
    vld1.8          {d0}, [r4], r2      @Load and increment src
    vst1.8          {d0}, [r5], r3      @Store and increment dst
    vld1.8          {d0}, [r4], r2      @Load and increment src
    vst1.8          {d0}, [r5], r3      @Store and increment dst
    vld1.8          {d0}, [r4], r2      @Load and increment src
    vst1.8          {d0}, [r5], r3      @Store and increment dst

    ldr             r4, [r0, #8]        @src->v
    ldr             r5, [r1, #8]        @dst->v
    @Read one row of data from the src
    vld1.8          {d0}, [r4], r2      @Load and increment src
    vst1.8          {d0}, [r5], r3      @Store and increment dst

    @//Repeat 7 times for v
    vld1.8          {d0}, [r4], r2      @Load and increment src
    vst1.8          {d0}, [r5], r3      @Store and increment dst
    vld1.8          {d0}, [r4], r2      @Load and increment src
    vst1.8          {d0}, [r5], r3      @Store and increment dst
    vld1.8          {d0}, [r4], r2      @Load and increment src
    vst1.8          {d0}, [r5], r3      @Store and increment dst
    vld1.8          {d0}, [r4], r2      @Load and increment src
    vst1.8          {d0}, [r5], r3      @Store and increment dst
    vld1.8          {d0}, [r4], r2      @Load and increment src
    vst1.8          {d0}, [r5], r3      @Store and increment dst
    vld1.8          {d0}, [r4], r2      @Load and increment src
    vst1.8          {d0}, [r5], r3      @Store and increment dst
    vld1.8          {d0}, [r4], r2      @Load and increment src
    vst1.8          {d0}, [r5], r3      @Store and increment dst

    ldmfd           r13!, {r4, r5, pc}




@/*
@//---------------------------------------------------------------------------
@// Function Name      :   impeg2_mc_fullx_halfy_8x8_a9q()
@//
@// Detail Description : This function pastes the reference block in the
@//                      current frame buffer.This function is called for
@//                      blocks that are not coded and have motion vectors
@//                      with a half pel resolution.
@//
@// Inputs             : r0 - out    : Current Block Pointer
@//                      r1 - ref     : Refernce Block Pointer
@//                      r2 - ref_wid   : Refernce Block Width
@//                      r3 - out_wid   ; Current Block Width
@//
@// Registers Used     : D0-D9
@//
@// Stack Usage        : 4 bytes
@//
@// Outputs            : The Motion Compensated Block
@//
@// Return Data        : None
@//
@// Programming Note   : <program limitation>
@//-----------------------------------------------------------------------------
@*/

        .global impeg2_mc_fullx_halfy_8x8_a9q

impeg2_mc_fullx_halfy_8x8_a9q:

    stmfd           r13!, {r14}
    add             r14, r1, r2
    mov             r2, r2, lsl #1

@/* Load 8 + 1 rows from reference block */
@/* Do the addition with out rounding off as rounding value is 1 */
    vld1.8          {d0}, [r1], r2      @// first row hence r1 = D0
    vld1.8          {d2}, [r14], r2     @// second row hence r2 = D2
    vld1.8          {d4}, [r1], r2      @// third row hence r3 = D4
    vld1.8          {d6}, [r14], r2     @// fourth row hence r4 = D6
    vld1.8          {d1}, [r1], r2      @// fifth row hence r5 = D1
    vld1.8          {d3}, [r14], r2     @// sixth row hence r6 = D3
    vrhadd.u8       d9, d1, d6          @// estimated row 4 = D9
    vld1.8          {d5}, [r1], r2      @// seventh row hence r7 = D5
    vrhadd.u8       q0, q0, q1          @// estimated row 1 = D0, row 5 = D1
    vld1.8          {d7}, [r14], r2     @// eighth row hence r8 = D7
    vrhadd.u8       q1, q1, q2          @// estimated row 2 = D2, row 6 = D3
    vld1.8          {d8}, [r1], r2      @// ninth row hence r9 = D8
    vrhadd.u8       q2, q2, q3          @// estimated row 3 = D4, row 7 = D5

    add             r14, r0, r3
    mov             r3, r3, lsl #1

@/* Store the eight rows calculated above */
    vst1.8          {d2}, [r14], r3     @// second row hence D2
    vrhadd.u8       d7, d7, d8          @// estimated row 8 = D7
    vst1.8          {d0}, [r0], r3      @// first row hence D0
    vst1.8          {d9}, [r14], r3     @// fourth row hence D9
    vst1.8          {d4}, [r0], r3      @// third row hence D4
    vst1.8          {d3}, [r14], r3     @// sixth row hence r6 = D3
    vst1.8          {d1}, [r0], r3      @// fifth row hence r5 = D1
    vst1.8          {d7}, [r14], r3     @// eighth row hence r8 = D7
    vst1.8          {d5}, [r0], r3      @// seventh row hence r7 = D5

    ldmfd           sp!, {pc}






@/*
@//---------------------------------------------------------------------------
@// Function Name      :   impeg2_mc_halfx_fully_8x8_a9q()
@//
@// Detail Description : This function pastes the reference block in the
@//                      current frame buffer.This function is called for
@//                      blocks that are not coded and have motion vectors
@//                      with a half pel resolutionand VopRoundingType is 0 ..
@//
@// Inputs             : r0 - out    : Current Block Pointer
@//                      r1 - ref     : Refernce Block Pointer
@//                      r2 - ref_wid   : Refernce Block Width
@//                      r3 - out_wid   ; Current Block Width
@//
@// Registers Used     : r12, r14, d0-d10, d12-d14, d16-d18, d20-d22

@//
@// Stack Usage        : 8 bytes
@//
@// Outputs            : The Motion Compensated Block
@//
@// Return Data        : None
@//
@// Programming Note   : <program limitation>
@//-----------------------------------------------------------------------------
@*/



        .global impeg2_mc_halfx_fully_8x8_a9q



impeg2_mc_halfx_fully_8x8_a9q:

    stmfd           sp!, {r12, lr}

    add             r14, r1, r2, lsl #2

    add             r12, r0, r3, lsl#2

    vld1.8          {d0, d1}, [r1], r2  @load 16 pixels of  row1

    vld1.8          {d2, d3}, [r14], r2 @ row5


    vld1.8          {d4, d5}, [r1], r2  @load 16 pixels row2

    vld1.8          {d6, d7}, [r14], r2 @row6


    vext.8          d8, d0, d1, #1      @Extract pixels (1-8) of row1

    vext.8          d12, d2, d3, #1     @Extract pixels (1-8) of row5

    vext.8          d16, d4, d5, #1     @Extract pixels (1-8) of row2

    vext.8          d20, d6, d7, #1     @Extract pixels (1-8) of row6


    vld1.8          {d9, d10}, [r1], r2 @load row3

    vld1.8          {d13, d14}, [r14], r2 @load row7

    vld1.8          {d17, d18}, [r1], r2 @load  row4

    vld1.8          {d21, d22}, [r14], r2 @load  row8


    vext.8          d1, d9, d10, #1     @Extract pixels (1-8) of row3

    vext.8          d3, d13, d14, #1    @Extract pixels (1-8) of row7



    vext.8          d5, d17, d18, #1    @Extract pixels (1-8) of row4

    vext.8          d7, d21, d22, #1    @Extract pixels (1-8) of row8


    vrhadd.u8       q0, q0, q4          @operate on row1 and row3

    vrhadd.u8       q1, q1, q6          @operate on row5 and row7


    vrhadd.u8       q2, q2, q8          @operate on row2 and row4



    vrhadd.u8       q3, q3, q10         @operate on row6 and row8

    vst1.8          d0, [r0], r3        @store row1

    vst1.8          d2, [r12], r3       @store row5

    vst1.8          d4, [r0], r3        @store row2

    vst1.8          d6, [r12], r3       @store row6

    vst1.8          d1, [r0], r3        @store row3

    vst1.8          d3, [r12], r3       @store row7

    vst1.8          d5, [r0], r3        @store row4

    vst1.8          d7, [r12], r3       @store row8



    ldmfd           sp!, {r12, pc}








@/*
@//---------------------------------------------------------------------------
@// Function Name      :   impeg2_mc_halfx_halfy_8x8_a9q()
@//
@// Detail Description : This function pastes the reference block in the
@//                      current frame buffer.This function is called for
@//                      blocks that are not coded and have motion vectors
@//                      with a half pel resolutionand VopRoundingType is 0 ..
@//
@// Inputs             : r0 - out    : Current Block Pointer
@//                      r1 - ref     : Refernce Block Pointer
@//                      r2 - ref_wid   : Refernce Block Width
@//                      r3 - out_wid   ; Current Block Width
@//
@// Registers Used     : r14, q0-q15

@//
@// Stack Usage        : 4 bytes
@//
@// Outputs            : The Motion Compensated Block
@//
@// Return Data        : None
@//
@// Programming Note   : <program limitation>
@//-----------------------------------------------------------------------------
@*/


        .global impeg2_mc_halfx_halfy_8x8_a9q

impeg2_mc_halfx_halfy_8x8_a9q:

    stmfd           sp!, {r14}

    add             r14, r1, r2, lsl #2

    vld1.8          {d0, d1}, [r1], r2  @load 16 pixels of  row1

    vld1.8          {d2, d3}, [r14], r2 @ row5

    vld1.8          {d4, d5}, [r1], r2  @load 16 pixels row2

    vld1.8          {d6, d7}, [r14], r2 @row6

    vext.8          d1, d0, d1, #1      @Extract pixels (1-8) of row1



    vext.8          d3, d2, d3, #1      @Extract pixels (1-8) of row5



    vext.8          d5, d4, d5, #1      @Extract pixels (1-8) of row2

    vext.8          d7, d6, d7, #1      @Extract pixels (1-8) of row6




    vld1.8          {d8, d9}, [r1], r2  @load row3



    vld1.8          {d10, d11}, [r14], r2 @load row7

    vld1.8          {d12, d13}, [r1], r2 @load  row4

    vld1.8          {d14, d15}, [r14], r2 @load  row8

    vext.8          d9, d8, d9, #1      @Extract pixels (1-8) of row3

    vld1.8          {d16, d17}, [r14], r2 @load  row9





    vext.8          d11, d10, d11, #1   @Extract pixels (1-8) of row7



    vext.8          d13, d12, d13, #1   @Extract pixels (1-8) of row4



    vext.8          d15, d14, d15, #1   @Extract pixels (1-8) of row8

    vext.8          d17, d16, d17, #1   @Extract pixels (1-8) of row9


    @interpolation in x direction

    vaddl.u8        q0, d0, d1          @operate row1

    vaddl.u8        q1, d2, d3          @operate row5

    vaddl.u8        q2, d4, d5          @operate row2

    vaddl.u8        q3, d6, d7          @operate row6

    vaddl.u8        q4, d8, d9          @operate row3

    vaddl.u8        q5, d10, d11        @operate row7

    vaddl.u8        q6, d12, d13        @operate row4

    vaddl.u8        q7, d14, d15        @operate row8

    vaddl.u8        q8, d16, d17        @operate row9

    @interpolation in y direction

    add             r14, r0, r3, lsl #2



    vadd.u16        q9, q0, q2          @operate row1 and row2

    vadd.u16        q13, q1, q3         @operate row5 and row6

    vadd.u16        q10, q2, q4         @operate row2 and row3

    vadd.u16        q14, q3, q5         @operate row6 and row7

    vrshrn.u16      d18, q9, #2         @row1

    vrshrn.u16      d26, q13, #2        @row5

    vrshrn.u16      d20, q10, #2        @row2

    vrshrn.u16      d28, q14, #2        @row6

    vadd.u16        q11, q4, q6         @operate row3 and row4

    vst1.8          d18, [r0], r3       @store row1

    vadd.u16        q15, q5, q7         @operate row7 and row8

    vst1.8          d26, [r14], r3      @store row5

    vadd.u16        q12, q6, q1         @operate row4 and row5

    vst1.8          d20, [r0], r3       @store row2

    vadd.u16        q7, q7, q8          @operate row8 and row9

    vst1.8          d28, [r14], r3      @store row6



    vrshrn.u16      d22, q11, #2        @row3

    vrshrn.u16      d30, q15, #2        @row7

    vrshrn.u16      d24, q12, #2        @row4

    vrshrn.u16      d14, q7, #2         @row8


    vst1.8          d22, [r0], r3       @store row3
    vst1.8          d30, [r14], r3      @store row7
    vst1.8          d24, [r0], r3       @store row4
    vst1.8          d14, [r14], r3      @store row8



    ldmfd           sp!, {pc}





@/*
@//---------------------------------------------------------------------------
@// Function Name      :   impeg2_mc_fullx_fully_8x8_a9q()
@//
@// Detail Description : This function pastes the reference block in the
@//                      current frame buffer.This function is called for
@//                      blocks that are not coded and have motion vectors
@//                      with a half pel resolutionand ..
@//
@// Inputs             : r0 - out    : Current Block Pointer
@//                      r1 - ref     : Refernce Block Pointer
@//                      r2 - ref_wid   : Refernce Block Width
@//                      r3 - out_wid   ; Current Block Width
@//
@// Registers Used     : r12, r14, d0-d3

@//
@// Stack Usage        : 8 bytes
@//
@// Outputs            : The Motion Compensated Block
@//
@// Return Data        : None
@//
@// Programming Note   : <program limitation>
@//-----------------------------------------------------------------------------
@*/


        .global impeg2_mc_fullx_fully_8x8_a9q
impeg2_mc_fullx_fully_8x8_a9q:


    stmfd           sp!, {r12, lr}

    add             r14, r1, r2, lsl #2

    add             r12, r0, r3, lsl #2


    vld1.8          d0, [r1], r2        @load row1

    vld1.8          d1, [r14], r2       @load row4

    vld1.8          d2, [r1], r2        @load row2

    vld1.8          d3, [r14], r2       @load row5


    vst1.8          d0, [r0], r3        @store row1

    vst1.8          d1, [r12], r3       @store row4

    vst1.8          d2, [r0], r3        @store row2

    vst1.8          d3, [r12], r3       @store row5


    vld1.8          d0, [r1], r2        @load row3

    vld1.8          d1, [r14], r2       @load row6

    vld1.8          d2, [r1], r2        @load row4

    vld1.8          d3, [r14], r2       @load row8


    vst1.8          d0, [r0], r3        @store row3

    vst1.8          d1, [r12], r3       @store row6

    vst1.8          d2, [r0], r3        @store row4

    vst1.8          d3, [r12], r3       @store row8


    ldmfd           sp!, {r12, pc}





@/*
@//---------------------------------------------------------------------------
@// Function Name      :   impeg2_interpolate_a9q()
@//
@// Detail Description : interpolates two buffers and adds pred
@//
@// Inputs             : r0 - pointer to src1
@//                      r1 - pointer to src2
@//                      r2 - dest buf
@//                      r3 - dst stride
@// Registers Used     : r4, r5, r7, r14, d0-d15
@//
@// Stack Usage        : 20 bytes
@//
@// Outputs            : The Motion Compensated Block
@//
@// Return Data        : None
@//
@// Programming Note   : <program limitation>
@//-----------------------------------------------------------------------------
@*/


        .global impeg2_interpolate_a9q


impeg2_interpolate_a9q:

    stmfd           r13!, {r4, r5, r7, r12, r14}

    ldr             r4, [r0, #0]        @ptr_y src1

    ldr             r5, [r1, #0]        @ptr_y src2

    ldr             r7, [r2, #0]        @ptr_y dst buf

    mov             r12, #4             @counter for number of blocks


interp_lumablocks_stride:

    vld1.8          {d0, d1}, [r4]!     @row1 src1

    vld1.8          {d2, d3}, [r4]!     @row2 src1

    vld1.8          {d4, d5}, [r4]!     @row3 src1

    vld1.8          {d6, d7}, [r4]!     @row4 src1


    vld1.8          {d8, d9}, [r5]!     @row1 src2

    vld1.8          {d10, d11}, [r5]!   @row2 src2

    vld1.8          {d12, d13}, [r5]!   @row3 src2

    vld1.8          {d14, d15}, [r5]!   @row4 src2




    vrhadd.u8       q0, q0, q4          @operate on row1

    vrhadd.u8       q1, q1, q5          @operate on row2

    vrhadd.u8       q2, q2, q6          @operate on row3

    vrhadd.u8       q3, q3, q7          @operate on row4



    vst1.8          {d0, d1}, [r7], r3  @row1

    vst1.8          {d2, d3}, [r7], r3  @row2

    vst1.8          {d4, d5}, [r7], r3  @row3

    vst1.8          {d6, d7}, [r7], r3  @row4

    subs            r12, r12, #1

    bne             interp_lumablocks_stride


    mov             r3, r3, lsr #1      @stride >> 1

    ldr             r4, [r0, #4]        @ptr_u src1

    ldr             r5, [r1, #4]        @ptr_u src2

    ldr             r7 , [r2, #4]       @ptr_u dst buf

    mov             r12, #2             @counter for number of blocks



@chroma blocks

interp_chromablocks_stride:

    vld1.8          {d0, d1}, [r4]!     @row1 & 2 src1

    vld1.8          {d2, d3}, [r4]!     @row3 & 4 src1

    vld1.8          {d4, d5}, [r4]!     @row5 & 6 src1

    vld1.8          {d6, d7}, [r4]!     @row7 & 8 src1


    vld1.8          {d8, d9}, [r5]!     @row1 & 2 src2

    vld1.8          {d10, d11}, [r5]!   @row3 & 4 src2

    vld1.8          {d12, d13}, [r5]!   @row5 & 6 src2

    vld1.8          {d14, d15}, [r5]!   @row7 & 8 src2




    vrhadd.u8       q0, q0, q4          @operate on row1 & 2

    vrhadd.u8       q1, q1, q5          @operate on row3 & 4

    vrhadd.u8       q2, q2, q6          @operate on row5 & 6

    vrhadd.u8       q3, q3, q7          @operate on row7 & 8


    vst1.8          {d0}, [r7], r3      @row1

    vst1.8          {d1}, [r7], r3      @row2

    vst1.8          {d2}, [r7], r3      @row3

    vst1.8          {d3}, [r7], r3      @row4

    vst1.8          {d4}, [r7], r3      @row5

    vst1.8          {d5}, [r7], r3      @row6

    vst1.8          {d6}, [r7], r3      @row7

    vst1.8          {d7}, [r7], r3      @row8



    ldr             r4, [r0, #8]        @ptr_v src1

    ldr             r5, [r1, #8]        @ptr_v src2

    ldr             r7, [r2, #8]        @ptr_v dst buf

    subs            r12, r12, #1

    bne             interp_chromablocks_stride


    ldmfd           r13!, {r4, r5, r7, r12, pc}





