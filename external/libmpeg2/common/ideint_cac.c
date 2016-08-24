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
*  ideint_cac.c
*
* @brief
*  This file include the definitions of the combing  artifact check function
* of the de-interlacer and some  variant of that.
*
* @author
*  Ittiam
*
* @par List of Functions:
*  cac_4x8()
*  ideint_cac()
*
* @remarks
*  In the de-interlacer workspace, cac is not a seperate  assembly module as
* it comes along with the  de_int_decision() function. But in C-Model, to
* keep  the things cleaner, it was made to be a separate  function during
* cac experiments long after the  assembly was written by Mudit.
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

/* User include files */
#include "icv_datatypes.h"
#include "icv_macros.h"
#include "icv.h"
#include "icv_variance.h"
#include "icv_sad.h"
#include "ideint.h"
#include "ideint_defs.h"
#include "ideint_structs.h"
#include "ideint_cac.h"

/**
*******************************************************************************
*
* @brief
* Combing artifact check function for 8x4 block
*
* @par   Description
*  Adjacent and alternate SADs are calculated by row based and column-based
*  collapsing. The adjacent and alternate SADs are then compared with some
*  biasing to get CAC
*
* @param[in] pu1_top
*  Top field
*
* @param[in] pu1_bot
*  Bottom field
*
* @param[in] top_strd
*  Top field Stride
*
* @param[in] bot_strd
*  Bottom field stride
*
* @param[in] pi4_adj_sad
*  Pointer to return adjacent SAD
*
* @param[in] pi4_alt_sad
*  Pointer to return alternate SAD
*
* @returns
* combing artifact flag (1 = detected, 0 = not detected)
*
* @remarks
*
*******************************************************************************
*/
static WORD32 cac_4x8(UWORD8 *pu1_top,
                      UWORD8 *pu1_bot,
                      WORD32 top_strd,
                      WORD32 bot_strd)
{
    WORD32 ca;
    WORD32 adj;
    WORD32 alt;
    UWORD8 *pu1_tmp_top;
    UWORD8 *pu1_tmp_bot;
    WORD32 i;
    WORD32 j;
    UWORD8 *pu1_top_0;
    UWORD8 *pu1_top_1;
    UWORD8 *pu1_top_2;
    UWORD8 *pu1_top_3;
    UWORD8 *pu1_bot_0;
    UWORD8 *pu1_bot_1;
    UWORD8 *pu1_bot_2;
    UWORD8 *pu1_bot_3;
    WORD32 rsum_csum_thresh;
    WORD32 sad_bias_mult_shift;
    WORD32 sad_bias_additive;

    WORD32 diff_sum;
    WORD32 top_row_end_incr;
    WORD32 bot_row_end_incr;

    ca = 0;

    adj = 0;
    alt = 0;

    rsum_csum_thresh    = RSUM_CSUM_THRESH;
    sad_bias_additive   = SAD_BIAS_ADDITIVE;
    sad_bias_mult_shift = SAD_BIAS_MULT_SHIFT;

    /*************************************************************************/
    /* In the adjacent sad calculation by row-method, the absolute           */
    /* difference is taken between the adjacent rows. The pixels of the diff */
    /* row, thus obtained, are then summed up. If this sum of absolute       */
    /* differace (sad) is greater than a threshold value, it is added to the */
    /* adjcacent SAD value.                                                  */
    /*************************************************************************/

    /*************************************************************************/
    /* Adj dif: Row based                                                    */
    /*************************************************************************/

    pu1_tmp_top = pu1_top;
    pu1_tmp_bot = pu1_bot;

    top_row_end_incr = top_strd - SUB_BLK_WD;
    bot_row_end_incr = bot_strd - SUB_BLK_WD;

    /*************************************************************************/
    /* The outer-loop runs for BLK_HT/2 times, because one pixel   */
    /* is touched only once.                                                 */
    /*************************************************************************/
    for(j = 0; j < BLK_HT; j += 4)
    {
        WORD32 sum_1, sum_2, sum_3, sum_4;
        WORD32 sum_diff;

        /*********************************************************************/
        /* Because the 8x4 is split into two halves of 4x4, the width of the */
        /* block is now 4.                                                   */
        /*********************************************************************/
        sum_1 = 0;
        sum_2 = 0;

        for(i = 0; i < SUB_BLK_WD; i ++)
        {
            sum_1 += *pu1_tmp_top++;
            sum_2 += *pu1_tmp_bot++;
        }

        sum_diff = ABS_DIF(sum_1, sum_2);

        /*********************************************************************/
        /* Thresholding.                                                     */
        /*********************************************************************/
        if(sum_diff >= rsum_csum_thresh)
            adj += sum_diff;

        pu1_tmp_top += top_row_end_incr;
        pu1_tmp_bot += bot_row_end_incr;


        sum_3 = 0;
        sum_4 = 0;

        for(i = 0; i < SUB_BLK_WD; i ++)
        {
            sum_3 += *pu1_tmp_top++;
            sum_4 += *pu1_tmp_bot++;
        }

        sum_diff = ABS_DIF(sum_3, sum_4);

        /*********************************************************************/
        /* Thresholding.                                                     */
        /*********************************************************************/
        if(sum_diff >= rsum_csum_thresh)
            adj += sum_diff;

        pu1_tmp_top += top_row_end_incr;
        pu1_tmp_bot += bot_row_end_incr;

        /*************************************************************************/
        /* Alt diff : Row based                                                  */
        /*************************************************************************/
        alt += ABS_DIF(sum_1, sum_3);
        alt += ABS_DIF(sum_2, sum_4);

    }

    /*************************************************************************/
    /* In the adjacent sad calculation by column-method, the rows of both    */
    /* the fields are averaged separately and then summed across the column. */
    /* The difference of the two values, thus obtained, is added to the      */
    /* adjacent sad value, if it is beyond the threshold.                    */
    /*************************************************************************/

    pu1_top_0 = pu1_top;
    pu1_top_1 = pu1_top_0 + top_strd;
    pu1_top_2 = pu1_top_1 + top_strd;
    pu1_top_3 = pu1_top_2 + top_strd;

    pu1_bot_0 = pu1_bot;
    pu1_bot_1 = pu1_bot_0 + bot_strd;
    pu1_bot_2 = pu1_bot_1 + bot_strd;
    pu1_bot_3 = pu1_bot_2 + bot_strd;

    /*************************************************************************/
    /* Adj dif: Col based                                                    */
    /*************************************************************************/
    diff_sum = 0;

    /*************************************************************************/
    /* As the DSP implementation of this modules is anyway going to assume   */
    /* the size of the block to the fixed (8x4 or two 4x4's), the height of  */
    /* block is also kept to be 8, to have a clean implementation.           */
    /*************************************************************************/
    for(i = 0; i < SUB_BLK_WD; i ++)
    {
        WORD32 val_1;
        WORD32 val_2;
        WORD32 tmp_1, tmp_2;
        WORD32 tmp_diff;

        tmp_1 = AVG(pu1_top_0[i], pu1_top_1[i]);
        tmp_2 = AVG(pu1_top_2[i], pu1_top_3[i]);
        val_1 = AVG(tmp_1,        tmp_2);

        tmp_1 = AVG(pu1_bot_0[i], pu1_bot_1[i]);
        tmp_2 = AVG(pu1_bot_2[i], pu1_bot_3[i]);
        val_2 = AVG(tmp_1,        tmp_2);

        tmp_diff = ABS_DIF(val_1, val_2);

        if(tmp_diff >= (rsum_csum_thresh >> 2))
            diff_sum += tmp_diff;
    }


    adj += diff_sum << 2;

    /*************************************************************************/
    /* Alt diff : Col based                                                  */
    /*************************************************************************/
    diff_sum = 0;

    for(i = 0; i < SUB_BLK_WD; i ++)
    {
        WORD32 val_1;
        WORD32 val_2;
        WORD32 tmp_1, tmp_2;
        WORD32 tmp_diff;

        tmp_1 = AVG(pu1_top_0[i], pu1_bot_0[i]);
        tmp_2 = AVG(pu1_top_2[i], pu1_bot_2[i]);
        val_1 = AVG(tmp_1,        tmp_2);

        tmp_1 = AVG(pu1_top_1[i], pu1_bot_1[i]);
        tmp_2 = AVG(pu1_top_3[i], pu1_bot_3[i]);
        val_2 = AVG(tmp_1, tmp_2);

        tmp_diff = ABS_DIF(val_1, val_2);

        diff_sum += tmp_diff;
    }

    /*************************************************************************/
    /* because of the averaging used in place of summation, a factor of 4 is */
    /* needed while adding the the diff_sum to the sad.                      */
    /*************************************************************************/

    alt += diff_sum << 2;

    pu1_top += SUB_BLK_WD;
    pu1_bot += SUB_BLK_WD;

    alt += (alt >> sad_bias_mult_shift) + (sad_bias_additive >> 1);
    ca   = (alt < adj);

    return ca;
}

/**
*******************************************************************************
*
* @brief
* Combing artifact check function for 8x8 block
*
* @par   Description
* Determines CAC for 8x8 block by calling 8x4 CAC function
*
* @param[in] pu1_top
*  Top field
*
* @param[in] pu1_bot
*  Bottom field
*
* @param[in] top_strd
*  Top field Stride
*
* @param[in] bot_strd
*  Bottom field stride
*
* @returns
* combing artifact flag (1 = detected, 0 = not detected)
*
* @remarks
*
*******************************************************************************
*/
WORD32 ideint_cac_8x8(UWORD8 *pu1_top,
                      UWORD8 *pu1_bot,
                      WORD32 top_strd,
                      WORD32 bot_strd)
{
    WORD32 ca;        /* combing artifact result                          */
    WORD32 k;

    ca = 0;
    /*************************************************************************/
    /* This loop runs for the two halves of the 4x8 block.                   */
    /*************************************************************************/
    for(k = 0; k < 2; k ++)
    {
        ca |= cac_4x8(pu1_top, pu1_bot, top_strd, bot_strd);

        pu1_top += SUB_BLK_WD;
        pu1_bot += SUB_BLK_WD;

        /* If Combing Artifact is detected, then return. Else continue to
         * check the next half
         */
        if(ca)
            return ca;
    }

    return ca;
}

