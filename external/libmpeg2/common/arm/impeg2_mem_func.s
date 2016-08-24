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
@// File Name            : impeg2_mem_func.s
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

@/*
@//---------------------------------------------------------------------------
@// Function Name      : impeg2_memset_8bit_8x8_block_a9q()
@//
@// Detail Description : This routine intialises the Block matrix buffer contents to a
@//                      particular Value. This function also assumes the buffer size
@//                      to be set is 64 Bytes fixed. It also assumes that blk matrix
@//                      used is 64 bit aligned.
@//
@// Inputs             : r0: pi2_blk_mat : Block Pointer
@//                      r1: u2_val      : Value with which the block is initialized
@//                      r2: u4_dst_width: Destination Width
@//
@// Registers Used     : q0
@//
@// Stack Usage        : 4 bytes
@//
@// Outputs            : Block Matrix Initialized to given value
@//
@// Return Data        : None
@//
@// Programming Note   : None
@//-----------------------------------------------------------------------------
@*/
        .global impeg2_memset_8bit_8x8_block_a9q
impeg2_memset_8bit_8x8_block_a9q:
    str             lr, [sp, #-4]!

    vdup.8          d0, r1              @//r1 is the 8-bit value to be set into

    vst1.8          {d0}, [r0], r2      @//Store the row 1
    vst1.8          {d0}, [r0], r2      @//Store the row 2
    vst1.8          {d0}, [r0], r2      @//Store the row 3
    vst1.8          {d0}, [r0], r2      @//Store the row 4
    vst1.8          {d0}, [r0], r2      @//Store the row 5
    vst1.8          {d0}, [r0], r2      @//Store the row 6
    vst1.8          {d0}, [r0], r2      @//Store the row 7
    vst1.8          {d0}, [r0], r2      @//Store the row 8

    ldr             pc, [sp], #4







@/*
@//---------------------------------------------------------------------------
@// Function Name      :   impeg2_memset0_16bit_8x8_linear_block_a9q()
@//
@// Detail Description : memsets 128 byte long linear buf to 0
@//
@// Inputs             : r0 - Buffer
@// Registers Used     : q0

@//
@// Stack Usage        : 4 bytes
@//
@// Outputs            : None
@//
@// Return Data        : None
@//
@// Programming Note   : <program limitation>
@//-----------------------------------------------------------------------------
@*/



        .global impeg2_memset0_16bit_8x8_linear_block_a9q


impeg2_memset0_16bit_8x8_linear_block_a9q:

    stmfd           r13!, {r14}

    vmov.i16        q0, #0

@Y data

    vst1.16         {d0, d1} , [r0]!    @row1

    vst1.16         {d0, d1} , [r0]!    @row2

    vst1.16         {d0, d1} , [r0]!    @row3

    vst1.16         {d0, d1} , [r0]!    @row4

    vst1.16         {d0, d1} , [r0]!    @row5

    vst1.16         {d0, d1} , [r0]!    @row6

    vst1.16         {d0, d1} , [r0]!    @row7

    vst1.16         {d0, d1} , [r0]!    @row8



    ldmfd           r13!, {pc}




