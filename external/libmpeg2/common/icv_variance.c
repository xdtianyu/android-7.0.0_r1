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
*  icv_variance.c
*
* @brief
*  This file contains the functions to compute variance
*
* @author
*  Ittiam
*
* @par List of Functions:
*  icv_variance_8x4()
*
* @remarks
*  None
*
*******************************************************************************
*/
/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/
/* System include files */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>


/* User include files */
#include "icv_datatypes.h"
#include "icv_macros.h"
#include "icv_platform_macros.h"
#include "icv.h"

/**
*******************************************************************************
*
* @brief
*  Computes variance of a given 8x4 block
*
* @par   Description
*  Compute variance of a given 8x4 block
*
* @param[in] pu1_src
*  Source
*
* @param[in] src_strd
*  Source stride
*
* @param[in] wd
*  Assumed to be 8
*
* @param[in] ht
*  Assumed to be 4
*
* @returns
*  Variance
*
* @remarks
*
*******************************************************************************
*/
WORD32 icv_variance_8x4(UWORD8 *pu1_src, WORD32 src_strd, WORD32 wd, WORD32 ht)
{
    WORD32 sum;
    WORD32 sum_sqr;
    WORD32 blk_sz;
    WORD32 vrnc;
    WORD32 i;
    WORD32 j;
    UNUSED(wd);
    UNUSED(ht);

    ASSERT(wd == 8);
    ASSERT(ht == 4);

    sum     = 0;
    sum_sqr = 0;

    blk_sz = 8 * 4;

    /*************************************************************************/
    /* variance                                                              */
    /* var = (n * SUM(x_i^2) - (SUM(x_i))^2) / (n^2);                        */
    /*************************************************************************/

    /*************************************************************************/
    /* The outer-loop runs for BLK_HT/2 times, because it                    */
    /* calculates the variance only for field area not frame one.            */
    /*************************************************************************/
    for(j = 0; j < 4; j ++)
    {
        for(i = 0; i < 8; i++)
        {
            sum_sqr += (*pu1_src) * (*pu1_src);
            sum     +=  *pu1_src++;
        }
        pu1_src += (src_strd - 8);
    }

    vrnc = ((sum_sqr * blk_sz) - (sum * sum)) / (blk_sz * blk_sz);

    return vrnc;
}

