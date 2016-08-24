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
//// File Name            : impeg2_inter_pred.s
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
//// Function Name      :   impeg2_copy_mb_av8()
////
//// Detail Description : Copies one MB worth of data from src to the dst
////
//// Inputs             : x0 - pointer to src
////                      x1 - pointer to dst
////                      x2 - source width
////                      x3 - destination width
//// Registers Used     : v0, v1
////
//// Stack Usage        : 64 bytes
////
//// Outputs            :
////
//// Return Data        : None
////
//// Programming Note   : <program limitation>
////-----------------------------------------------------------------------------
//*/



.global impeg2_copy_mb_av8


impeg2_copy_mb_av8:

//STMFD   x13!,{x4,x5,x12,x14}
    push_v_regs


    ldr             x4, [x0]            //src->y
    ldr             x5, [x1]            //dst->y

    //Read one row of data from the src
    ld1             {v0.8b, v1.8b}, [x4], x2 //Load and increment src
    st1             {v0.8b, v1.8b}, [x5], x3 //Store and increment dst

    ////Repeat 15 times for y
    ld1             {v0.8b, v1.8b}, [x4], x2 //Load and increment src
    st1             {v0.8b, v1.8b}, [x5], x3 //Store and increment dst
    ld1             {v0.8b, v1.8b}, [x4], x2 //Load and increment src
    st1             {v0.8b, v1.8b}, [x5], x3 //Store and increment dst
    ld1             {v0.8b, v1.8b}, [x4], x2 //Load and increment src
    st1             {v0.8b, v1.8b}, [x5], x3 //Store and increment dst
    ld1             {v0.8b, v1.8b}, [x4], x2 //Load and increment src
    st1             {v0.8b, v1.8b}, [x5], x3 //Store and increment dst
    ld1             {v0.8b, v1.8b}, [x4], x2 //Load and increment src
    st1             {v0.8b, v1.8b}, [x5], x3 //Store and increment dst
    ld1             {v0.8b, v1.8b}, [x4], x2 //Load and increment src
    st1             {v0.8b, v1.8b}, [x5], x3 //Store and increment dst
    ld1             {v0.8b, v1.8b}, [x4], x2 //Load and increment src
    st1             {v0.8b, v1.8b}, [x5], x3 //Store and increment dst
    ld1             {v0.8b, v1.8b}, [x4], x2 //Load and increment src
    st1             {v0.8b, v1.8b}, [x5], x3 //Store and increment dst
    ld1             {v0.8b, v1.8b}, [x4], x2 //Load and increment src
    st1             {v0.8b, v1.8b}, [x5], x3 //Store and increment dst
    ld1             {v0.8b, v1.8b}, [x4], x2 //Load and increment src
    st1             {v0.8b, v1.8b}, [x5], x3 //Store and increment dst
    ld1             {v0.8b, v1.8b}, [x4], x2 //Load and increment src
    st1             {v0.8b, v1.8b}, [x5], x3 //Store and increment dst
    ld1             {v0.8b, v1.8b}, [x4], x2 //Load and increment src
    st1             {v0.8b, v1.8b}, [x5], x3 //Store and increment dst
    ld1             {v0.8b, v1.8b}, [x4], x2 //Load and increment src
    st1             {v0.8b, v1.8b}, [x5], x3 //Store and increment dst
    ld1             {v0.8b, v1.8b}, [x4], x2 //Load and increment src
    st1             {v0.8b, v1.8b}, [x5], x3 //Store and increment dst
    ld1             {v0.8b, v1.8b}, [x4], x2 //Load and increment src
    st1             {v0.8b, v1.8b}, [x5], x3 //Store and increment dst

    lsr             x2, x2, #1          //src_offset /= 2
    lsr             x3, x3, #1          //dst_offset /= 2

    ldr             x4, [x0, #8]        //src->u
    ldr             x5, [x1, #8]        //dst->u

    //Read one row of data from the src
    ld1             {v0.8b}, [x4], x2   //Load and increment src
    st1             {v0.8b}, [x5], x3   //Store and increment dst

    ////Repeat 7 times for u
    ld1             {v0.8b}, [x4], x2   //Load and increment src
    st1             {v0.8b}, [x5], x3   //Store and increment dst
    ld1             {v0.8b}, [x4], x2   //Load and increment src
    st1             {v0.8b}, [x5], x3   //Store and increment dst
    ld1             {v0.8b}, [x4], x2   //Load and increment src
    st1             {v0.8b}, [x5], x3   //Store and increment dst
    ld1             {v0.8b}, [x4], x2   //Load and increment src
    st1             {v0.8b}, [x5], x3   //Store and increment dst
    ld1             {v0.8b}, [x4], x2   //Load and increment src
    st1             {v0.8b}, [x5], x3   //Store and increment dst
    ld1             {v0.8b}, [x4], x2   //Load and increment src
    st1             {v0.8b}, [x5], x3   //Store and increment dst
    ld1             {v0.8b}, [x4], x2   //Load and increment src
    st1             {v0.8b}, [x5], x3   //Store and increment dst

    ldr             x4, [x0, #16]       //src->v
    ldr             x5, [x1, #16]       //dst->v

    //Read one row of data from the src
    ld1             {v0.8b}, [x4], x2   //Load and increment src
    st1             {v0.8b}, [x5], x3   //Store and increment dst

    ////Repeat 7 times for v
    ld1             {v0.8b}, [x4], x2   //Load and increment src
    st1             {v0.8b}, [x5], x3   //Store and increment dst
    ld1             {v0.8b}, [x4], x2   //Load and increment src
    st1             {v0.8b}, [x5], x3   //Store and increment dst
    ld1             {v0.8b}, [x4], x2   //Load and increment src
    st1             {v0.8b}, [x5], x3   //Store and increment dst
    ld1             {v0.8b}, [x4], x2   //Load and increment src
    st1             {v0.8b}, [x5], x3   //Store and increment dst
    ld1             {v0.8b}, [x4], x2   //Load and increment src
    st1             {v0.8b}, [x5], x3   //Store and increment dst
    ld1             {v0.8b}, [x4], x2   //Load and increment src
    st1             {v0.8b}, [x5], x3   //Store and increment dst
    ld1             {v0.8b}, [x4], x2   //Load and increment src
    st1             {v0.8b}, [x5], x3   //Store and increment dst

//LDMFD   x13!,{x4,x5,x12,PC}
    pop_v_regs
    ret


///*
////---------------------------------------------------------------------------
//// Function Name      :   impeg2_mc_fullx_halfy_8x8_av8()
////
//// Detail Description : This function pastes the reference block in the
////                      current frame buffer.This function is called for
////                      blocks that are not coded and have motion vectors
////                      with a half pel resolution.
////
//// Inputs             : x0 - out    : Current Block Pointer
////                      x1 - ref     : Refernce Block Pointer
////                      x2 - ref_wid   : Refernce Block Width
////                      x3 - out_wid    @ Current Block Width
////
//// Registers Used     : x14, D0-D9
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

.global impeg2_mc_fullx_halfy_8x8_av8

impeg2_mc_fullx_halfy_8x8_av8:

//STMFD       x13!,{x12,x14}
    push_v_regs
    add             x14, x1, x2
    lsl             x2, x2, #1

///* Load 8 + 1 rows from reference block */
///* Do the addition with out rounding off as rounding value is 1 */
    ld1             {v0.8b}, [x1], x2   //// first row hence x1 = D0
    ld1             {v2.8b}, [x14], x2  //// second row hence x2 = D2
    ld1             {v4.8b}, [x1], x2   //// third row hence x3 = D4
    ld1             {v6.8b}, [x14], x2  //// fourth row hence x4 = D6
    ld1             {v1.8b}, [x1], x2   //// fifth row hence x5 = D1
    ld1             {v3.8b}, [x14], x2  //// sixth row hence x6 = D3
    urhadd          v9.8b, v1.8b , v6.8b //// estimated row 4 = D9
    ld1             {v5.8b}, [x1], x2   //// seventh row hence x7 = D5
    urhadd          v0.16b, v0.16b , v2.16b //// estimated row 1 = D0, row 5 = D1
    urhadd          v1.16b, v1.16b , v3.16b //// estimated row 1 = D0, row 5 = D1
    ld1             {v7.8b}, [x14], x2  //// eighth row hence x8 = D7
    urhadd          v2.16b, v2.16b , v4.16b //// estimated row 2 = D2, row 6 = D3
    urhadd          v3.16b, v3.16b , v5.16b //// estimated row 2 = D2, row 6 = D3
    ld1             {v8.8b}, [x1], x2   //// ninth row hence x9 = D8
    urhadd          v4.16b, v4.16b , v6.16b //// estimated row 3 = D4, row 7 = D5
    urhadd          v5.16b, v5.16b , v7.16b //// estimated row 3 = D4, row 7 = D5

    add             x14, x0, x3
    lsl             x3, x3, #1

///* Store the eight rows calculated above */
    st1             {v2.8b}, [x14], x3  //// second row hence D2
    urhadd          v7.8b, v7.8b , v8.8b //// estimated row 8 = D7
    st1             {v0.8b}, [x0], x3   //// first row hence D0
    st1             {v9.8b}, [x14], x3  //// fourth row hence D9
    st1             {v4.8b}, [x0], x3   //// third row hence D4
    st1             {v3.8b}, [x14], x3  //// sixth row hence x6 = D3
    st1             {v1.8b}, [x0], x3   //// fifth row hence x5 = D1
    st1             {v7.8b}, [x14], x3  //// eighth row hence x8 = D7
    st1             {v5.8b}, [x0], x3   //// seventh row hence x7 = D5

// LDMFD sp!,{x12,pc}
    pop_v_regs
    ret





///*
////---------------------------------------------------------------------------
//// Function Name      :   impeg2_mc_halfx_fully_8x8_av8()
////
//// Detail Description : This function pastes the reference block in the
////                      current frame buffer.This function is called for
////                      blocks that are not coded and have motion vectors
////                      with a half pel resolutionand VopRoundingType is 0 ..
////
//// Inputs             : x0 - out    : Current Block Pointer
////                      x1 - ref     : Refernce Block Pointer
////                      x2 - ref_wid   : Refernce Block Width
////                      x3 - out_wid    @ Current Block Width
////
//// Registers Used     : x12, x14, v0-v10, v12-v14, v16-v18, v20-v22

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



.global impeg2_mc_halfx_fully_8x8_av8



impeg2_mc_halfx_fully_8x8_av8:

    // STMFD sp!,{x12,x14}
    push_v_regs

    add             x14, x1, x2, lsl #2

    add             x12, x0, x3, lsl#2

    ld1             {v0.8b, v1.8b}, [x1], x2 //load 16 pixels of  row1

    ld1             {v2.8b, v3.8b}, [x14], x2 // row5


    ld1             {v4.8b, v5.8b}, [x1], x2 //load 16 pixels row2

    ld1             {v6.8b, v7.8b}, [x14], x2 //row6


    ext             v8.8b, v0.8b , v1.8b , #1

    ext             v12.8b, v2.8b , v3.8b , #1

    ext             v16.8b, v4.8b , v5.8b , #1

    ext             v20.8b, v6.8b , v7.8b , #1


    ld1             {v9.8b, v10.8b}, [x1], x2 //load row3

    ld1             {v13.8b, v14.8b}, [x14], x2 //load row7

    ld1             {v17.8b, v18.8b}, [x1], x2 //load  row4

    ld1             {v21.8b, v22.8b}, [x14], x2 //load  row8


    ext             v1.8b, v9.8b , v10.8b , #1

    ext             v3.8b, v13.8b , v14.8b , #1



    ext             v5.8b, v17.8b , v18.8b , #1

    ext             v7.8b, v21.8b , v22.8b , #1


    urhadd          v0.16b, v0.16b , v8.16b //operate on row1 and row3
    urhadd          v1.16b, v1.16b , v9.16b //operate on row1 and row3

    urhadd          v2.16b, v2.16b , v12.16b //operate on row5 and row7
    urhadd          v3.16b, v3.16b , v13.16b //operate on row5 and row7


    urhadd          v4.16b, v4.16b , v16.16b //operate on row2 and row4
    urhadd          v5.16b, v5.16b , v17.16b //operate on row2 and row4


    urhadd          v6.16b, v6.16b , v20.16b //operate on row6 and row8
    urhadd          v7.16b, v7.16b , v21.16b //operate on row6 and row8

    st1             {v0.8b}, [x0], x3   //store row1

    st1             {v2.8b}, [x12], x3  //store row5

    st1             {v4.8b}, [x0], x3   //store row2

    st1             {v6.8b}, [x12], x3  //store row6

    st1             {v1.8b}, [x0], x3   //store row3

    st1             {v3.8b}, [x12], x3  //store row7

    st1             {v5.8b}, [x0], x3   //store row4

    st1             {v7.8b}, [x12], x3  //store row8



    // LDMFD sp!,{x12,pc}
    pop_v_regs
    ret







///*
////---------------------------------------------------------------------------
//// Function Name      :   impeg2_mc_halfx_halfy_8x8_av8()
////
//// Detail Description : This function pastes the reference block in the
////                      current frame buffer.This function is called for
////                      blocks that are not coded and have motion vectors
////                      with a half pel resolutionand VopRoundingType is 0 ..
////
//// Inputs             : x0 - out    : Current Block Pointer
////                      x1 - ref     : Refernce Block Pointer
////                      x2 - ref_wid   : Refernce Block Width
////                      x3 - out_wid    @ Current Block Width
////
//// Registers Used     : x14, v0-v18, v22, v24, v26, v28, v30

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


.global impeg2_mc_halfx_halfy_8x8_av8

impeg2_mc_halfx_halfy_8x8_av8:

    // STMFD sp!,{x12,x14}
    push_v_regs

    add             x14, x1, x2, lsl #2

    ld1             {v0.8b, v1.8b}, [x1], x2 //load 16 pixels of  row1

    ld1             {v2.8b, v3.8b}, [x14], x2 // row5

    ld1             {v4.8b, v5.8b}, [x1], x2 //load 16 pixels row2

    ld1             {v6.8b, v7.8b}, [x14], x2 //row6

    ext             v1.8b, v0.8b , v1.8b , #1



    ext             v3.8b, v2.8b , v3.8b , #1



    ext             v5.8b, v4.8b , v5.8b , #1

    ext             v7.8b, v6.8b , v7.8b , #1




    ld1             {v8.8b, v9.8b}, [x1], x2 //load row3



    ld1             {v10.8b, v11.8b}, [x14], x2 //load row7

    ld1             {v12.8b, v13.8b}, [x1], x2 //load  row4

    ld1             {v14.8b, v15.8b}, [x14], x2 //load  row8

    ext             v9.8b, v8.8b , v9.8b , #1

    ld1             {v16.8b, v17.8b}, [x14], x2 //load  row9





    ext             v11.8b, v10.8b , v11.8b , #1



    ext             v13.8b, v12.8b , v13.8b , #1



    ext             v15.8b, v14.8b , v15.8b , #1

    ext             v17.8b, v16.8b , v17.8b , #1


    //interpolation in x direction

    uaddl           v0.8h, v0.8b, v1.8b //operate row1

    uaddl           v2.8h, v2.8b, v3.8b //operate row5

    uaddl           v4.8h, v4.8b, v5.8b //operate row2

    uaddl           v6.8h, v6.8b, v7.8b //operate row6

    uaddl           v8.8h, v8.8b, v9.8b //operate row3

    uaddl           v10.8h, v10.8b, v11.8b //operate row7

    uaddl           v12.8h, v12.8b, v13.8b //operate row4

    uaddl           v14.8h, v14.8b, v15.8b //operate row8

    uaddl           v16.8h, v16.8b, v17.8b //operate row9

    //interpolation in y direction

    add             x14, x0, x3, lsl #2



    add             v18.8h, v0.8h , v4.8h //operate row1 and row2

    add             v26.8h, v2.8h , v6.8h //operate row5 and row6

    add             v20.8h, v4.8h , v8.8h //operate row2 and row3

    add             v28.8h, v6.8h , v10.8h //operate row6 and row7

    rshrn           v18.8b, v18.8h, #2  //row1

    rshrn           v26.8b, v26.8h, #2  //row5

    rshrn           v20.8b, v20.8h, #2  //row2

    rshrn           v28.8b, v28.8h, #2  //row6

    add             v22.8h, v8.8h , v12.8h //operate row3 and row4

    st1             {v18.8b}, [x0], x3  //store row1

    add             v30.8h, v10.8h , v14.8h //operate row7 and row8

    st1             {v26.8b}, [x14], x3 //store row5

    add             v24.8h, v12.8h , v2.8h //operate row4 and row5

    st1             {v20.8b}, [x0], x3  //store row2

    add             v14.8h, v14.8h , v16.8h //operate row8 and row9

    st1             {v28.8b}, [x14], x3 //store row6



    rshrn           v22.8b, v22.8h, #2  //row3

    rshrn           v30.8b, v30.8h, #2  //row7

    rshrn           v24.8b, v24.8h, #2  //row4

    rshrn           v14.8b, v14.8h, #2  //row8


    st1             {v22.8b}, [x0], x3  //store row3
    st1             {v30.8b}, [x14], x3 //store row7
    st1             {v24.8b}, [x0], x3  //store row4
    st1             {v14.8b}, [x14], x3 //store row8



    // LDMFD sp!,{x12,pc}
    pop_v_regs
    ret




///*
////---------------------------------------------------------------------------
//// Function Name      :   impeg2_mc_fullx_fully_8x8_av8()
////
//// Detail Description : This function pastes the reference block in the
////                      current frame buffer.This function is called for
////                      blocks that are not coded and have motion vectors
////                      with a half pel resolutionand ..
////
//// Inputs             : x0 - out    : Current Block Pointer
////                      x1 - ref     : Refernce Block Pointer
////                      x2 - ref_wid   : Refernce Block Width
////                      x3 - out_wid    @ Current Block Width
////
//// Registers Used     : x12, x14, v0-v3

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


.global impeg2_mc_fullx_fully_8x8_av8
impeg2_mc_fullx_fully_8x8_av8:


    // STMFD sp!,{x12,x14}
    push_v_regs

    add             x14, x1, x2, lsl #2

    add             x12, x0, x3, lsl #2


    ld1             {v0.8b}, [x1], x2   //load row1

    ld1             {v1.8b}, [x14], x2  //load row4

    ld1             {v2.8b}, [x1], x2   //load row2

    ld1             {v3.8b}, [x14], x2  //load row5


    st1             {v0.8b}, [x0], x3   //store row1

    st1             {v1.8b}, [x12], x3  //store row4

    st1             {v2.8b}, [x0], x3   //store row2

    st1             {v3.8b}, [x12], x3  //store row5


    ld1             {v0.8b}, [x1], x2   //load row3

    ld1             {v1.8b}, [x14], x2  //load row6

    ld1             {v2.8b}, [x1], x2   //load row4

    ld1             {v3.8b}, [x14], x2  //load row8


    st1             {v0.8b}, [x0], x3   //store row3

    st1             {v1.8b}, [x12], x3  //store row6

    st1             {v2.8b}, [x0], x3   //store row4

    st1             {v3.8b}, [x12], x3  //store row8


    // LDMFD sp!,{x12,pc}
    pop_v_regs
    ret




///*
////---------------------------------------------------------------------------
//// Function Name      :   impeg2_interpolate_av8()
////
//// Detail Description : interpolates two buffers and adds pred
////
//// Inputs             : x0 - pointer to src1
////                      x1 - pointer to src2
////                      x2 - dest buf
////                         x3 - dst stride
//// Registers Used     : x12, v0-v15
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


.global impeg2_interpolate_av8


impeg2_interpolate_av8:

//STMFD    x13!,{x4-x7,x12,x14}
    push_v_regs

    ldr             x4, [x0, #0]        //ptr_y src1

    ldr             x5, [x1, #0]        //ptr_y src2

    ldr             x7, [x2, #0]        //ptr_y dst buf

    mov             x12, #4             //counter for number of blocks


interp_lumablocks_stride:
    ld1             {v0.16b}, [x4], #16 //row1 src1

    ld1             {v2.16b}, [x4], #16 //row2 src1

    ld1             {v4.16b}, [x4], #16 //row3 src1

    ld1             {v6.16b}, [x4], #16 //row4 src1


    ld1             {v8.16b}, [x5], #16 //row1 src2

    ld1             {v10.16b}, [x5], #16 //row2 src2

    ld1             {v12.16b}, [x5], #16 //row3 src2

    ld1             {v14.16b}, [x5], #16 //row4 src2

    urhadd          v0.16b, v0.16b , v8.16b //operate on row1

    urhadd          v2.16b, v2.16b , v10.16b //operate on row2

    urhadd          v4.16b, v4.16b , v12.16b //operate on row3

    urhadd          v6.16b, v6.16b , v14.16b //operate on row4
    st1             {v0.16b}, [x7], x3  //row1

    st1             {v2.16b}, [x7], x3  //row2

    st1             {v4.16b}, [x7], x3  //row3

    st1             {v6.16b}, [x7], x3  //row4

    subs            x12, x12, #1

    bne             interp_lumablocks_stride


    lsr             x3, x3, #1          //stride >> 1

    ldr             x4, [x0, #8]        //ptr_u src1

    ldr             x5, [x1, #8]        //ptr_u src2

    ldr             x7 , [x2, #8]       //ptr_u dst buf

    mov             x12, #2             //counter for number of blocks



//chroma blocks

interp_chromablocks_stride:
    ld1             {v0.8b, v1.8b}, [x4], #16 //row1 & 2 src1

    ld1             {v2.8b, v3.8b}, [x4], #16 //row3 & 4 src1

    ld1             {v4.8b, v5.8b}, [x4], #16 //row5 & 6 src1

    ld1             {v6.8b, v7.8b}, [x4], #16 //row7 & 8 src1


    ld1             {v8.8b, v9.8b}, [x5], #16 //row1 & 2 src2

    ld1             {v10.8b, v11.8b}, [x5], #16 //row3 & 4 src2

    ld1             {v12.8b, v13.8b}, [x5], #16 //row5 & 6 src2

    ld1             {v14.8b, v15.8b}, [x5], #16 //row7 & 8 src2

    urhadd          v0.16b, v0.16b , v8.16b //operate on row1 & 2
    urhadd          v1.16b, v1.16b , v9.16b //operate on row1 & 2

    urhadd          v2.16b, v2.16b , v10.16b //operate on row3 & 4
    urhadd          v3.16b, v3.16b , v11.16b //operate on row3 & 4

    urhadd          v4.16b, v4.16b , v12.16b //operate on row5 & 6
    urhadd          v5.16b, v5.16b , v13.16b //operate on row5 & 6

    urhadd          v6.16b, v6.16b , v14.16b //operate on row7 & 8
    urhadd          v7.16b, v7.16b , v15.16b //operate on row7 & 8

    st1             {v0.8b}, [x7], x3   //row1

    st1             {v1.8b}, [x7], x3   //row2

    st1             {v2.8b}, [x7], x3   //row3

    st1             {v3.8b}, [x7], x3   //row4

    st1             {v4.8b}, [x7], x3   //row5

    st1             {v5.8b}, [x7], x3   //row6

    st1             {v6.8b}, [x7], x3   //row7

    st1             {v7.8b}, [x7], x3   //row8


    ldr             x4, [x0, #16]       //ptr_v src1

    ldr             x5, [x1, #16]       //ptr_v src2

    ldr             x7, [x2, #16]       //ptr_v dst buf

    subs            x12, x12, #1

    bne             interp_chromablocks_stride


    //LDMFD  x13!,{x4-x7,x12,PC}
    pop_v_regs
    ret




