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
*  ideint_cac_ssse3.c
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
#include <immintrin.h>

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
WORD32 ideint_cac_8x8_ssse3(UWORD8 *pu1_top,
                            UWORD8 *pu1_bot,
                            WORD32 top_strd,
                            WORD32 bot_strd)
{
    WORD32 ca;        /* combing artifact result                          */
    WORD32 i;
    WORD32 adj[2] = {0};
    WORD32 alt[2] = {0};
    WORD32 sum_1, sum_2, sum_3, sum_4;
    WORD32 sum_diff, diff_sum;

    __m128i top[4];
    __m128i bot[4];
    __m128i sum_t[4];
    __m128i sum_b[4];
    __m128i zero;


    zero = _mm_setzero_si128();

    for(i = 0; i < 4; i++)
    {
        /* Load top */
        top[i] = (__m128i)_mm_loadl_epi64((__m128i *) (pu1_top));
        pu1_top += top_strd;

        /* Load bottom */
        bot[i] = (__m128i)_mm_loadl_epi64((__m128i *) (pu1_bot));
        pu1_bot += bot_strd;

        /* Unpack */
        top[i] = _mm_unpacklo_epi8(top[i], zero);
        bot[i] = _mm_unpacklo_epi8(bot[i], zero);

        /* Compute row sums */
        sum_t[i]  = _mm_sad_epu8(top[i], zero);
        sum_b[i]  = _mm_sad_epu8(bot[i], zero);
    }

    /* Compute row based alt and adj */
    for(i = 0; i < 4; i += 2)
    {
        sum_1 = _mm_cvtsi128_si32(sum_t[i + 0]);
        sum_2 = _mm_cvtsi128_si32(sum_b[i + 0]);
        sum_diff = ABS_DIF(sum_1, sum_2);
        if(sum_diff >= RSUM_CSUM_THRESH)
            adj[0] += sum_diff;

        sum_3 = _mm_cvtsi128_si32(sum_t[i + 1]);
        sum_4 = _mm_cvtsi128_si32(sum_b[i + 1]);
        sum_diff = ABS_DIF(sum_3, sum_4);
        if(sum_diff >= RSUM_CSUM_THRESH)
            adj[0] += sum_diff;

        alt[0] += ABS_DIF(sum_1, sum_3);
        alt[0] += ABS_DIF(sum_2, sum_4);

        sum_1 = _mm_cvtsi128_si32(_mm_srli_si128(sum_t[i + 0], 8));
        sum_2 = _mm_cvtsi128_si32(_mm_srli_si128(sum_b[i + 0], 8));
        sum_diff = ABS_DIF(sum_1, sum_2);
        if(sum_diff >= RSUM_CSUM_THRESH)
            adj[1] += sum_diff;

        sum_3 = _mm_cvtsi128_si32(_mm_srli_si128(sum_t[i + 1], 8));
        sum_4 = _mm_cvtsi128_si32(_mm_srli_si128(sum_b[i + 1], 8));
        sum_diff = ABS_DIF(sum_3, sum_4);
        if(sum_diff >= RSUM_CSUM_THRESH)
            adj[1] += sum_diff;

        alt[1] += ABS_DIF(sum_1, sum_3);
        alt[1] += ABS_DIF(sum_2, sum_4);
    }

    /* Compute column based adj */
    {
        __m128i avg1, avg2;
        __m128i top_avg, bot_avg;
        __m128i min, max, diff, thresh;
        __m128i mask;
        avg1 = _mm_avg_epu8(top[0], top[1]);
        avg2 = _mm_avg_epu8(top[2], top[3]);
        top_avg = _mm_avg_epu8(avg1, avg2);

        avg1 = _mm_avg_epu8(bot[0], bot[1]);
        avg2 = _mm_avg_epu8(bot[2], bot[3]);
        bot_avg = _mm_avg_epu8(avg1, avg2);

        min = _mm_min_epu8(top_avg, bot_avg);
        max = _mm_max_epu8(top_avg, bot_avg);

        diff = _mm_sub_epi16(max, min);
        thresh = _mm_set1_epi16((RSUM_CSUM_THRESH >> 2) - 1);

        mask = _mm_cmpgt_epi16(diff, thresh);
        diff = _mm_and_si128(diff, mask);

        diff_sum = _mm_extract_epi16(diff, 0);
        diff_sum += _mm_extract_epi16(diff, 1);
        diff_sum += _mm_extract_epi16(diff, 2);
        diff_sum += _mm_extract_epi16(diff, 3);

        adj[0] += diff_sum << 2;

        diff_sum = _mm_extract_epi16(diff, 4);
        diff_sum += _mm_extract_epi16(diff, 5);
        diff_sum += _mm_extract_epi16(diff, 6);
        diff_sum += _mm_extract_epi16(diff, 7);

        adj[1] += diff_sum << 2;

    }

    /* Compute column based alt */
    {
        __m128i avg1, avg2;
        __m128i even_avg, odd_avg, diff;
        avg1 = _mm_avg_epu8(top[0], bot[0]);
        avg2 = _mm_avg_epu8(top[2], bot[2]);
        even_avg = _mm_avg_epu8(avg1, avg2);

        avg1 = _mm_avg_epu8(top[1], bot[1]);
        avg2 = _mm_avg_epu8(top[3], bot[3]);
        odd_avg = _mm_avg_epu8(avg1, avg2);

        diff = _mm_sad_epu8(even_avg, odd_avg);


        diff_sum = _mm_cvtsi128_si32(diff);
        alt[0] += diff_sum << 2;

        diff_sum = _mm_cvtsi128_si32(_mm_srli_si128(diff, 8));
        alt[1] += diff_sum << 2;

    }
    alt[0] += (alt[0] >> SAD_BIAS_MULT_SHIFT) + (SAD_BIAS_ADDITIVE >> 1);
    alt[1] += (alt[1] >> SAD_BIAS_MULT_SHIFT) + (SAD_BIAS_ADDITIVE >> 1);

    ca    = (alt[0] < adj[0]);
    ca   |= (alt[1] < adj[1]);

    return ca;
}

