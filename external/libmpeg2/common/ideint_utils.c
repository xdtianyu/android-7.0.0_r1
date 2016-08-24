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
*  ideint_utils.c
*
* @brief
*  This file contains the definitions of the core  processing of the de
* interlacer.
*
* @author
*  Ittiam
*
* @par List of Functions:
*  ideint_weave_pic()
*  init_bob_indices()
*  ideint_weave_blk()
*  ideint_spatial_filter()
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
#include "icv_variance.h"
#include "icv_sad.h"
#include "ideint.h"
#include "ideint_defs.h"
#include "ideint_structs.h"
#include "ideint_utils.h"
#include "ideint_cac.h"

/**
*******************************************************************************
*
* @brief
*  Weaves two fields to produce a frame
*
* @par   Description
*  Weaves two fields to produce a frame
*
* @param[in] ps_src_top
*  Top field source
*
* @param[in] ps_src_bot
*  Bottom field source
*
* @param[in] ps_dst_frm
*  Destination frame
*
* @returns
*   0 on Success
*
* @remarks
*
*******************************************************************************
*/
WORD32 ideint_weave_pic(icv_pic_t *ps_src_top,
                        icv_pic_t *ps_src_bot,
                        icv_pic_t *ps_dst_frm,
                        WORD32 start_row,
                        WORD32 num_rows)
{
    UWORD8 *pu1_src, *pu1_dst;
    WORD32 i, j, num_comp;
    icv_pic_t *ps_src_fld;
    WORD32 fld;
    icv_pic_t *ps_src_flds[2];

    num_comp = 3;
    ps_src_flds[0] = ps_src_top;
    ps_src_flds[1] = ps_src_bot;

    for(fld = 0; fld < 2; fld++)
    {
        ps_src_fld = ps_src_flds[fld];
        for(i = 0; i < num_comp; i++)
        {
            WORD32 src_strd;
            WORD32 dst_strd;
            WORD32 comp_row_start, comp_row_end;
            comp_row_start = start_row;
            comp_row_end = comp_row_start + num_rows;
            if(i)
            {
                comp_row_start >>= 1;
                comp_row_end >>= 1;
            }

            comp_row_end = MIN(comp_row_end, ps_dst_frm->ai4_ht[i]);

            pu1_src = ps_src_fld->apu1_buf[i];
            pu1_dst = ps_dst_frm->apu1_buf[i];

            src_strd = ps_src_fld->ai4_strd[i];
            dst_strd = ps_dst_frm->ai4_strd[i];

            /* If source field is bottom, increment destination */
            pu1_dst += fld * dst_strd;

            /* In case input and output are pointing to same buffer, then no need to copy */
            if((pu1_src != pu1_dst) || ((2 * dst_strd) != src_strd))
            {
                pu1_dst += ps_dst_frm->ai4_strd[i] * comp_row_start;
                pu1_src += ps_src_fld->ai4_strd[i] * comp_row_start / 2;

                for(j = comp_row_start; j < comp_row_end; j += 2)
                {
                    memcpy(pu1_dst, pu1_src, ps_dst_frm->ai4_wd[i]);
                    pu1_dst += ps_dst_frm->ai4_strd[i] * 2;
                    pu1_src += ps_src_fld->ai4_strd[i];
                }
            }
        }
    }
    return 0;
}


/**
*******************************************************************************
*
* @brief
*  Weaves a 8x8 block
*
* @par   Description
*  Weaves a 8x8 block from two fields
*
* @param[in] pu1_top
*  Top field source
*
* @param[in] pu1_bot
*  Bottom field source
*
* @param[in] pu1_dst
*  Destination
*
* @param[in] dst_strd
*  Destination stride
*
* @param[in] src_strd
*  Source stride
*
* @returns
*  0 on success
*
* @remarks
*
*******************************************************************************
*/
WORD32 ideint_weave_blk(UWORD8 *pu1_top,
                        UWORD8 *pu1_bot,
                        UWORD8 *pu1_dst,
                        WORD32 dst_strd,
                        WORD32 src_strd,
                        WORD32 wd,
                        WORD32 ht)
{
    WORD32 j;

    for(j = 0; j < ht; j += 2)
    {
        memcpy(pu1_dst, pu1_top, wd);
        pu1_dst += dst_strd;
        pu1_top += src_strd;

        memcpy(pu1_dst, pu1_bot, wd);
        pu1_dst += dst_strd;
        pu1_bot += src_strd;
    }
    return 0;
}

/**
*******************************************************************************
*
* @brief
*  Copy a boundary block and pad
*
* @par   Description
*  Copies a block on one of the boundaries and pads
*
* @param[in] pu1_top
*  Top field source
*
* @param[in] pu1_bot
*  Bottom field source
*
* @param[in] pu1_pad
*  Padded destination
*
* @param[in] cur_strd
*  Stride for pu1_top and pu1_bot
*
* @param[in] row
*  Current block's row
*
* @param[in] col
*  Current block's column
*
* @param[in] num_blks_y
*  Number of blocks in Y direction
*
* @param[in] num_blks_x
*  Number of blocks in X direction

* @returns
*  None
*
* @remarks
*
*******************************************************************************
*/
void ideint_pad_blk(UWORD8 *pu1_top,
                    UWORD8 *pu1_bot,
                    UWORD8 *pu1_pad,
                    WORD32 cur_strd,
                    WORD32 row,
                    WORD32 col,
                    WORD32 num_blks_y,
                    WORD32 num_blks_x,
                    WORD32 blk_wd,
                    WORD32 blk_ht)
{
    WORD32 i;
    WORD32 num_cols, num_rows;
    UWORD8 *pu1_dst;
    UWORD8 *pu1_src_top;
    UWORD8 *pu1_src_bot;

    num_rows = blk_ht + 4;
    num_cols = blk_wd + 4;

    pu1_src_top = pu1_top - cur_strd - 2;
    pu1_src_bot = pu1_bot - cur_strd - 2;
    pu1_dst = pu1_pad;

    if(0 == col)
    {
        num_cols -= 2;
        pu1_dst += 2;
        pu1_src_top += 2;
        pu1_src_bot += 2;
    }

    if(0 == row)
    {
        num_rows -= 2;
        pu1_dst += 2 * (BLK_WD + 4);
        pu1_src_top += cur_strd;
        pu1_src_bot += cur_strd;
    }

    if((num_blks_x - 1) == col)
        num_cols -= 2;

    if((num_blks_y - 1) == row)
        num_rows -= 2;

    for(i = 0; i < num_rows; i += 2)
    {
        memcpy(pu1_dst, pu1_src_top, num_cols);
        pu1_dst += (BLK_WD + 4);

        memcpy(pu1_dst, pu1_src_bot, num_cols);
        pu1_dst += (BLK_WD + 4);

        pu1_src_top += cur_strd;
        pu1_src_bot += cur_strd;
    }


    /* Pad Left */
    if(0 == col)
    {
       for(i = 0; i < (BLK_HT + 4); i++)
       {
           WORD32 ofst = i * (BLK_WD + 4) + 2;
           pu1_pad[ofst - 1] = pu1_pad[ofst];
           pu1_pad[ofst - 2] = pu1_pad[ofst];
       }
    }

    /* Pad right */
    if((num_blks_x - 1) == col)
    {
       for(i = 0; i < (BLK_HT + 4); i++)
       {
           WORD32 ofst =  i * (BLK_WD + 4) + 2 + blk_wd - 1;
           WORD32 size = (BLK_WD - blk_wd) + 2;
           /* Padding on right should include padding for boundary
            * blocks when width is non-multiple of 8
            */
           memset(&pu1_pad[ofst + 1], pu1_pad[ofst], size);
       }
    }

    /* Pad Top */
    if(0 == row)
    {
        WORD32 src_ofst = 2 * (BLK_WD + 4);
        WORD32 dst_ofst = 0;
        memcpy(pu1_pad + dst_ofst, pu1_pad + src_ofst, (BLK_WD + 4));
        src_ofst += (BLK_WD + 4);
        dst_ofst += (BLK_WD + 4);
        memcpy(pu1_pad + dst_ofst, pu1_pad + src_ofst, (BLK_WD + 4));
    }

    /* Pad Bottom */
    if((num_blks_y - 1) == row)
    {
        WORD32 src_ofst = (0 + blk_ht) * (BLK_WD + 4);
        WORD32 dst_ofst = (1 + blk_ht) * (BLK_WD + 4);
        WORD32 size = (BLK_HT - blk_ht) + 2;

        /* Padding on bottom should include padding for boundary
         * blocks when height is non-multiple of 8
         */
        for(i = 0; i < size; i++)
        {
            memcpy(pu1_pad + dst_ofst, pu1_pad + src_ofst, (BLK_WD + 4));
            dst_ofst += (BLK_WD + 4);
        }
    }
}

/**
*******************************************************************************
*
* @brief
*  Performs spatial edge adaptive filtering
*
* @par   Description
*  Performs spatial edge adaptive filtering by detecting edge direction
*
* @param[in] pu1_src
*  Source buffer
*
* @param[in] pu1_out
*  Destination buffer
*
* @param[in] src_strd
*  Source stride
*
* @param[in] out_strd
*  Destination stride

* @returns
* None
*
* @remarks
*
*******************************************************************************
*/
void ideint_spatial_filter(UWORD8 *pu1_src,
                           UWORD8 *pu1_out,
                           WORD32 src_strd,
                           WORD32 out_strd)
{
    WORD32 i;
    WORD32 j;
    WORD32 k;

    /*********************************************************************/
    /* This loop is for the two halves inside the 8x4 block.             */
    /*********************************************************************/
    for(k = 0; k < 2; k++)
    {
        WORD32 adiff[3] = {0, 0, 0};
        WORD32 shift;
        WORD32 dir_45_le_90, dir_45_le_135, dir_135_le_90;
        UWORD8 *pu1_row_1, *pu1_row_2, *pu1_dst;

        /*****************************************************************/
        /* Direction detection                                           */
        /*****************************************************************/
        pu1_row_1 = pu1_src;
        pu1_row_2 = pu1_src + src_strd;

        /*****************************************************************/
        /* Calculating the difference along each of the 3 directions.    */
        /*****************************************************************/
        for(j = 0; j < SUB_BLK_HT; j ++)
        {
            for(i = 0; i < SUB_BLK_WD; i++)
            {
                adiff[0] += ABS_DIF(pu1_row_1[i], pu1_row_2[i]); /*  90 */

                adiff[1] += ABS_DIF(pu1_row_1[i - 1], pu1_row_2[i + 1]); /* 135 */

                adiff[2] += ABS_DIF(pu1_row_1[i + 1], pu1_row_2[i - 1]); /*  45 */
            }
            pu1_row_1 += src_strd;
            pu1_row_2 += src_strd;
        }

        /*****************************************************************/
        /* Applying bias, to make the diff comparision more robust.      */
        /*****************************************************************/
        adiff[0] *= EDGE_BIAS_0;
        adiff[1] *= EDGE_BIAS_1;
        adiff[2] *= EDGE_BIAS_1;

        /*****************************************************************/
        /* comapring the diffs */
        /*****************************************************************/
        dir_45_le_90  = (adiff[2] <= adiff[0]);
        dir_45_le_135 = (adiff[2] <= adiff[1]);
        dir_135_le_90 = (adiff[1] <= adiff[0]);

        /*****************************************************************/
        /* Direction selection. */
        /*****************************************************************/
        shift = 0;
        if(1 == dir_45_le_135)
        {
            if(1 == dir_45_le_90)
                shift = 1;
        }
        else
        {
            if(1 == dir_135_le_90)
                shift = -1;
        }

        /*****************************************************************/
        /* Directional interpolation */
        /*****************************************************************/
        pu1_row_1 = pu1_src + shift;
        pu1_row_2 = pu1_src + src_strd - shift;
        pu1_dst   = pu1_out;

        for(j = 0; j < SUB_BLK_HT; j++)
        {
            for(i = 0; i < SUB_BLK_WD; i++)
            {
                pu1_dst[i] = (UWORD8)AVG(pu1_row_1[i], pu1_row_2[i]);
            }
            pu1_row_1 += src_strd;
            pu1_row_2 += src_strd;
            pu1_dst   += out_strd;
        }

        pu1_out += SUB_BLK_WD;
        pu1_src += SUB_BLK_WD;
    }
}

