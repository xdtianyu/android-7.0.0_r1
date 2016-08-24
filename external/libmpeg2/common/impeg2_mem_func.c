/******************************************************************************
 *
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************
 * Originally developed and contributed by Ittiam Systems Pvt. Ltd, Bangalore
*/
/**
*******************************************************************************
* @file
*  impeg2_utils.c
*
* @brief
*  Contains utility function definitions for MPEG2 codec
*
* @author
*  Harish
*
* @par List of Functions:
* - impeg2_memset0_16bit_8x8_linear_block()
* - impeg2_memset_8bit_8x8_block()
*
* @remarks
*  None
*
*******************************************************************************
*/

#include <stdio.h>
#include <string.h>
#include "iv_datatypedef.h"
#include "impeg2_defs.h"

/*******************************************************************************
*  Function Name   : impeg2_memset0_16bit_8x8_linear_block
*
*  Description     : memsets resudial buf to 0
*
*  Arguments       : destination buffer
*
*  Values Returned : None
*******************************************************************************/


void impeg2_memset0_16bit_8x8_linear_block (WORD16 *pi2_buf)
{
        memset(pi2_buf,0,64 * sizeof(WORD16));
}



/*******************************************************************************
*  Function Name   : impeg2_memset_8bit_8x8_block
*
*  Description     : memsets residual buf to value
*
*  Arguments       : destination buffer, value and stride
*
*  Values Returned : None
*******************************************************************************/


void impeg2_memset_8bit_8x8_block(UWORD8 *pu1_dst, WORD32 u4_dc_val, WORD32 u4_dst_wd)
{
    WORD32 j;

    for(j = BLK_SIZE; j > 0; j--)
    {
        memset(pu1_dst, u4_dc_val, BLK_SIZE);
        pu1_dst += u4_dst_wd;
    }
}



