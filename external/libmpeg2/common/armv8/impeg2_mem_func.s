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
//// File Name            : mot_comp_neon.s
////
//// Description          : This file has motion compensation related
////                        interpolation functions on Neon + CortexA-8 platform
////
//// Reference Document   :
////
//// Revision History     :
////      Date            Author                  Detail Description
////   ------------    ----------------    ----------------------------------
////   18 jun 2010      S Hamsalekha              Created
////
////-------------------------------------------------------------------------
//*/

///*
//// ----------------------------------------------------------------------------
//// Include Files
//// ----------------------------------------------------------------------------
//*/
//              PRESERVE8
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
//// -------------------------- NONE --------------------------------------------


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

///*
////---------------------------------------------------------------------------
//// Function Name      : impeg2_memset_8bit_8x8_block_av8()
////
//// Detail Description : This routine intialises the Block matrix buffer contents to a
////                      particular Value. This function also assumes the buffer size
////                         to be set is 64 Bytes fixed. It also assumes that blk matrix
////                         used is 64 bit aligned.
////
//// Inputs             : pi2_blk_mat : Block Pointer
////                         u2_val      : Value with which the block is initialized
////
//// Registers Used     : v0
////
//// Stack Usage        : 64 bytes
////
//// Outputs            : Block Matrix Iniliazed to given value
////
//// Return Data        : None
////
//// Programming Note   : This implementation assumes that blk matrix buffer
////                         is 128 bit aligned
////-----------------------------------------------------------------------------
//*/
.global impeg2_memset_8bit_8x8_block_av8
impeg2_memset_8bit_8x8_block_av8:
    push_v_regs

//        ADD            x3,x0,#WIDTH_X_SIZE            @//x3 is another copy address offsetted

    dup             v0.8b, w1           ////x1 is the 8-bit value to be set into

    st1             {v0.8b}, [x0], x2   ////Store the row 1
    st1             {v0.8b}, [x0], x2   ////Store the row 2
    st1             {v0.8b}, [x0], x2   ////Store the row 3
    st1             {v0.8b}, [x0], x2   ////Store the row 4
    st1             {v0.8b}, [x0], x2   ////Store the row 5
    st1             {v0.8b}, [x0], x2   ////Store the row 6
    st1             {v0.8b}, [x0], x2   ////Store the row 7
    st1             {v0.8b}, [x0], x2   ////Store the row 8

    pop_v_regs
    ret






///*
////---------------------------------------------------------------------------
//// Function Name      :   impeg2_memset0_16bit_8x8_linear_block_av8()
////
//// Detail Description : memsets resudual buf to 0
////
//// Inputs             : x0 - pointer to y
////                      x1 - pointer to u
////                      x2 - pointer to v
//// Registers Used     : v0

////
//// Stack Usage        : 64 bytes
////
//// Outputs            : The Motion Compensated Block
////
//// Return Data        : None
////
//// Programming Note   : <program limitation>
////-----------------------------------------------------------------------------
//*/



.global impeg2_memset0_16bit_8x8_linear_block_av8


impeg2_memset0_16bit_8x8_linear_block_av8:

    push_v_regs

    movi            v0.8h, #0

    //Y data

    st1             {v0.8h} , [x0], #16 //row1

    st1             {v0.8h} , [x0], #16 //row2

    st1             {v0.8h} , [x0], #16 //row3

    st1             {v0.8h} , [x0], #16 //row4

    st1             {v0.8h} , [x0], #16 //row5

    st1             {v0.8h} , [x0], #16 //row6

    st1             {v0.8h} , [x0], #16 //row7

    st1             {v0.8h} , [x0], #16 //row8



    pop_v_regs
    ret




