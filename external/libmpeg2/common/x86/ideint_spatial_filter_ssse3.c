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
*   ideint_spatial_filter_ssse3()
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
#include <immintrin.h>

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
void ideint_spatial_filter_ssse3(UWORD8 *pu1_src,
                           UWORD8 *pu1_out,
                           WORD32 src_strd,
                           WORD32 out_strd)
{
    WORD32 i;

    WORD32 adiff[6];
    WORD32 *pi4_diff;
    WORD32 shifts[2];
    WORD32 dir_45_le_90, dir_45_le_135, dir_135_le_90;

    __m128i row1_0, row1_m1, row1_p1;
    __m128i row2_0, row2_m1, row2_p1;
    __m128i diff, diffs[3];
    __m128i zero;

    /*****************************************************************/
    /* Direction detection                                           */
    /*****************************************************************/

    zero = _mm_setzero_si128();
    diffs[0] = _mm_setzero_si128();
    diffs[1]  = _mm_setzero_si128();
    diffs[2] = _mm_setzero_si128();

    /* Load source */
    row1_m1 = _mm_loadl_epi64((__m128i *) (pu1_src - 1));
    row1_0  = _mm_loadl_epi64((__m128i *) (pu1_src));
    row1_p1 = _mm_loadl_epi64((__m128i *) (pu1_src + 1));
    pu1_src += src_strd;

    /* Unpack to 16 bits */
    row1_m1 = _mm_unpacklo_epi8(row1_m1, zero);
    row1_0  = _mm_unpacklo_epi8(row1_0,  zero);
    row1_p1 = _mm_unpacklo_epi8(row1_p1, zero);

    /*****************************************************************/
    /* Calculating the difference along each of the 3 directions.    */
    /*****************************************************************/
    for(i = 0; i < SUB_BLK_HT; i ++)
    {
        row2_m1 = _mm_loadl_epi64((__m128i *) (pu1_src - 1));
        row2_0  = _mm_loadl_epi64((__m128i *) (pu1_src));
        row2_p1 = _mm_loadl_epi64((__m128i *) (pu1_src + 1));
        pu1_src += src_strd;

        /* Unpack to 16 bits */
        row2_m1 = _mm_unpacklo_epi8(row2_m1, zero);
        row2_0  = _mm_unpacklo_epi8(row2_0,  zero);
        row2_p1 = _mm_unpacklo_epi8(row2_p1, zero);

        diff    = _mm_sad_epu8(row1_0, row2_0);
        diffs[0]  = _mm_add_epi64(diffs[0], diff);

        diff    = _mm_sad_epu8(row1_m1, row2_p1);
        diffs[1] = _mm_add_epi64(diffs[1], diff);

        diff    = _mm_sad_epu8(row1_p1, row2_m1);
        diffs[2]  = _mm_add_epi64(diffs[2], diff);

        row1_m1 = row2_m1;
        row1_0 = row2_0;
        row1_p1 = row2_p1;
    }
    /* Revert pu1_src increment */
    pu1_src -= (SUB_BLK_HT + 1) * src_strd;


    adiff[0] = _mm_cvtsi128_si32(diffs[0]);
    adiff[1] = _mm_cvtsi128_si32(diffs[1]);
    adiff[2] = _mm_cvtsi128_si32(diffs[2]);
    adiff[3] = _mm_cvtsi128_si32(_mm_srli_si128(diffs[0], 8));
    adiff[4] = _mm_cvtsi128_si32(_mm_srli_si128(diffs[1], 8));
    adiff[5] = _mm_cvtsi128_si32(_mm_srli_si128(diffs[2], 8));
    pi4_diff = adiff;

    for(i = 0; i < 2; i++)
    {
        /*****************************************************************/
        /* Applying bias, to make the diff comparision more robust.      */
        /*****************************************************************/
        pi4_diff[0] *= EDGE_BIAS_0;
        pi4_diff[1] *= EDGE_BIAS_1;
        pi4_diff[2] *= EDGE_BIAS_1;

        /*****************************************************************/
        /* comapring the diffs */
        /*****************************************************************/
        dir_45_le_90  = (pi4_diff[2] <= pi4_diff[0]);
        dir_45_le_135 = (pi4_diff[2] <= pi4_diff[1]);
        dir_135_le_90 = (pi4_diff[1] <= pi4_diff[0]);

        /*****************************************************************/
        /* Direction selection. */
        /*****************************************************************/
        shifts[i] = 0;
        if(1 == dir_45_le_135)
        {
            if(1 == dir_45_le_90)
                shifts[i] = 1;
        }
        else
        {
            if(1 == dir_135_le_90)
                shifts[i] = -1;
        }
        pi4_diff += 3;
    }
    /*****************************************************************/
    /* Directional interpolation */
    /*****************************************************************/
    for(i = 0; i < SUB_BLK_HT / 2; i++)
    {
        __m128i dst;
        __m128i row1, row2;

        UWORD32 *pu4_row1th, *pu4_row1tl;
        UWORD32 *pu4_row2th, *pu4_row2tl;
        UWORD32 *pu4_row1bh, *pu4_row1bl;
        UWORD32 *pu4_row2bh, *pu4_row2bl;

        pu4_row1th  = (UWORD32 *)(pu1_src + shifts[0]);
        pu4_row1tl  = (UWORD32 *)(pu1_src + SUB_BLK_WD + shifts[1]);

        pu1_src += src_strd;
        pu4_row2th  = (UWORD32 *)(pu1_src + shifts[0]);
        pu4_row2tl  = (UWORD32 *)(pu1_src + SUB_BLK_WD + shifts[1]);

        pu4_row1bh  = (UWORD32 *)(pu1_src - shifts[0]);
        pu4_row1bl  = (UWORD32 *)(pu1_src + SUB_BLK_WD - shifts[1]);

        pu1_src += src_strd;
        pu4_row2bh  = (UWORD32 *)(pu1_src - shifts[0]);
        pu4_row2bl  = (UWORD32 *)(pu1_src + SUB_BLK_WD - shifts[1]);

        row1 = _mm_set_epi32(*pu4_row1tl, *pu4_row1th, *pu4_row2tl, *pu4_row2th);
        row2 = _mm_set_epi32(*pu4_row1bl, *pu4_row1bh, *pu4_row2bl, *pu4_row2bh);

        dst = _mm_avg_epu8(row1, row2);

        _mm_storel_epi64((__m128i *)pu1_out, _mm_srli_si128(dst, 8));
        pu1_out += out_strd;

        _mm_storel_epi64((__m128i *)pu1_out, dst);
        pu1_out += out_strd;
    }
}

