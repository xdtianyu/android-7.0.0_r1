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
*  ih264e_half_pel.c
*
* @brief
*  This file contains functions that are used for computing subpixel planes
*
* @author
*  ittiam
*
* @par List of Functions:
*  - ih264e_sixtapfilter_horz
*  - ih264e_sixtap_filter_2dvh_vert
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
#include <assert.h>
#include <limits.h>

/* User include files */
#include "ih264_typedefs.h"
#include "ithread.h"
#include "ih264_platform_macros.h"
#include "ih264_defs.h"
#include "ih264e_half_pel.h"
#include "ih264_macros.h"
#include "ih264e_debug.h"
#include "ih264_inter_pred_filters.h"
#include "ih264_mem_fns.h"
#include "ih264_padding.h"
#include "ih264_intra_pred_filters.h"
#include "ih264_deblk_edge_filters.h"


/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/**
*******************************************************************************
*
* @brief
*  Interprediction luma filter for horizontal input (Filter run for width = 17
*  and height =16)
*
* @par Description:
*  Applies a 6 tap horizontal filter .The output is  clipped to 8 bits
*  sec 8.4.2.2.1 titled "Luma sample interpolation process"
*
* @param[in] pu1_src
*  UWORD8 pointer to the source
*
* @param[out] pu1_dst
*  UWORD8 pointer to the destination
*
* @param[in] src_strd
*  integer source stride
*
* @param[in] dst_strd
*  integer destination stride
*
* @returns
*
* @remarks
*  None
*
*******************************************************************************
*/
void ih264e_sixtapfilter_horz(UWORD8 *pu1_src,
                              UWORD8 *pu1_dst,
                              WORD32 src_strd,
                              WORD32 dst_strd)
{
    UWORD32  u4_i, u4_j;
    UWORD32  u4_w, u4_h;

    /* width and height of interpolation */
    u4_w = HP_PL_WD;
    u4_h = MB_SIZE;

    pu1_src -= 2;

    for (u4_i = 0; u4_i < u4_h; u4_i++)
    {
        for (u4_j = 0; u4_j < u4_w; u4_j++, pu1_dst++, pu1_src++)
        {
            WORD16 i16_temp;

            i16_temp = ih264_g_six_tap[0] * (*pu1_src + pu1_src[5])
                            + ih264_g_six_tap[1] * (pu1_src[1] + pu1_src[4])
                            + ih264_g_six_tap[2] * (pu1_src[2] + pu1_src[3]);

            i16_temp = (i16_temp + 16) >> 5;

            *pu1_dst = CLIP_U8(i16_temp);
        }
        pu1_src += src_strd - u4_w;
        pu1_dst += dst_strd - u4_w;
    }
}

/**
*******************************************************************************
*
* @brief
*  This function implements a two stage cascaded six tap filter. It applies
*  the six tap filter in the vertical direction on the predictor values,
*  followed by applying the same filter in the horizontal direction on the
*  output of the first stage. The six tap filtering operation is described in
*  sec 8.4.2.2.1 titled "Luma sample interpolation process" (Filter run for
*  width = 17 and height = 17)
*
* @par Description:
*  The function interpolates the predictors first in the vertical direction and
*  then in the horizontal direction to output the (1/2,1/2). The output of the
*  first stage of the filter is stored in the buffer pointed to by
*  pi16_pred1(only in C) in 16 bit precision.
*
* @param[in] pu1_src
*  UWORD8 pointer to the source
*
* @param[out] pu1_dst1
*  UWORD8 pointer to the destination (Horizontal filtered output)
*
* @param[out] pu1_dst2
*  UWORD8 pointer to the destination (output after applying vertical filter to
*  the intermediate horizontal output)
*
* @param[in] src_strd
*  integer source stride

* @param[in] dst_strd
*  integer destination stride of pu1_dst
*
* @param[in] pi4_pred
*  Pointer to 16bit intermediate buffer (used only in c)
*
* @param[in] i4_pred_strd
*  integer destination stride of pi16_pred1
*
* @returns
*
* @remarks
*  None
*
*******************************************************************************
*/
void ih264e_sixtap_filter_2dvh_vert(UWORD8 *pu1_src,
                                    UWORD8 *pu1_dst1,
                                    UWORD8 *pu1_dst2,
                                    WORD32 src_strd,
                                    WORD32 dst_strd,
                                    WORD32 *pi4_pred,
                                    WORD32 i4_pred_strd)
{
    WORD32 row, col;
    WORD32 tmp;
    WORD32 *pi4_pred_temp = pi4_pred;
    WORD32 ht = HP_PL_HT, wd = HP_PL_WD;

    for (row = 0; row < ht; row++)
    {
        for (col = -2; col < wd + 3; col++)
        {
            tmp = ih264_g_six_tap[0] * (pu1_src[col - 2 * src_strd] + pu1_src[col + 3 * src_strd]) +
                            ih264_g_six_tap[1] * (pu1_src[col - 1 * src_strd] + pu1_src[col + 2 * src_strd]) +
                            ih264_g_six_tap[2] * (pu1_src[col] + pu1_src[col + 1 * src_strd]);

            pi4_pred_temp[col] = tmp;
        }

        pu1_src += src_strd;
        pi4_pred_temp += i4_pred_strd;
    }

    for (row = 0; row < ht; row++)
    {
        for (col = 0; col < wd; col++)
        {
            tmp = (pi4_pred[col - 2] + pi4_pred[col + 3]) +
                            ih264_g_six_tap[1] * (pi4_pred[col - 1] + pi4_pred[col + 2]) +
                            ih264_g_six_tap[2] * (pi4_pred[col] + pi4_pred[col + 1]);

            tmp = (tmp + 512) >> 10;

            pu1_dst2[col] = CLIP_U8(tmp);
            pu1_dst1[col] = CLIP_U8((pi4_pred[col] + 16) >> 5);
        }
        pi4_pred += i4_pred_strd;
        pu1_dst2 += dst_strd;
        pu1_dst1 += dst_strd;
    }
}

